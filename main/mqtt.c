#include "mqtt.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor.h"
#include "tasks.h"
#include "camera.h"

static const char *TAG_MQTT = "MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

// 証明書データの外部参照宣言
extern const uint8_t aws_root_ca_pem_start[] asm("_binary_AmazonRootCA1_pem_start");
#include "driver/gpio.h"
extern const uint8_t device_cert_pem_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t device_key_pem_start[] asm("_binary_private_pem_key_start");

void mqtt_init(void)
{
    // すでに初期化済みの場合は何もしない
    if (mqtt_client) return;

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .broker.verification.certificate = (const char *)aws_root_ca_pem_start,
        .buffer.size = 40960,       // 40KB
        .buffer.out_size = 40960,   // 送信バッファ
        .credentials = {
            .client_id = MQTT_CLIENT_ID,
            .authentication = {
                .certificate = (const char *)device_cert_pem_start,
                .key = (const char *)device_key_pem_start,
            },
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    
    if (mqtt_client != NULL) {
        // --- 1. ハードウェア初期化 (LED) ---
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << MQTT_LED_GPIO),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        gpio_set_level(MQTT_LED_GPIO, 1); // 初期状態を消灯(High)に設定
        // --- 2. イベント登録 ---
        // 第4引数に mqtt_client を渡すことで、ハンドラ内で event->client として使えます
        esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, mqtt_client);

        esp_err_t err = esp_mqtt_client_start(mqtt_client);
        if (err == ESP_OK) {
            ESP_LOGI(TAG_MQTT, "AWS IoT MQTT client started successfully");
        } else {
            ESP_LOGE(TAG_MQTT, "Failed to start MQTT client: %d", err);
        }

    } else {
        ESP_LOGE(TAG_MQTT, "Failed to initialize MQTT client structure");
    }
}

int mqtt_publish(const char *topic, const char *data)
{
    if (!mqtt_client) {
        ESP_LOGW(TAG_MQTT, "Publish called before mqtt_init");
        return -1;
    }
    size_t len = data ? strlen(data) : 0;
    const char *use_topic = topic ? topic : MQTT_DEFAULT_TOPIC;
    int msg_id = esp_mqtt_client_publish(mqtt_client, use_topic, data, len, MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN);
    return msg_id;
}

static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // event_data をキャストして event 変数を作る（これで既存の event->... が動きます）
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_subscribe(client, MQTT_SUBSCRIBE_TOPIC, 0);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA: topic=%.*s data=%.*s", event->topic_len, event->topic, event->data_len, event->data);
            if (event->data && event->data_len > 0) {
                char buf[32];
                int copy_len = event->data_len < (int)(sizeof(buf)-1) ? event->data_len : (int)(sizeof(buf)-1);
                memcpy(buf, event->data, copy_len);
                buf[copy_len] = '\0';
                for (int i = 0; i < copy_len; ++i) {
                    if (buf[i] >= 'A' && buf[i] <= 'Z') buf[i] = buf[i] - 'A' + 'a';
                }
                if (strcmp(buf, "on") == 0) {
                    ESP_LOGI(TAG_MQTT, "LED ON (GPIO LOW)");
                    gpio_set_level(MQTT_LED_GPIO, 0);  // 0で点灯
                } else if (strcmp(buf, "off") == 0) {
                    ESP_LOGI(TAG_MQTT, "LED OFF (GPIO HIGH)");
                    gpio_set_level(MQTT_LED_GPIO, 1);  // 1で消灯
                }
            }
            break;
        default:
            break;
    }
}

static void mqtt_task(void *arg)
{
    for (;;) {
        camera_fb_t *fb = NULL;
        if (camera_capture(&fb) == ESP_OK && fb) {
            
            // ログで画像サイズを確認（重要：128KBを超えていないか）
            ESP_LOGI(TAG_MQTT, "Captured image size: %zu bytes", fb->len);

            // バイナリのまま直接Publish! (Base64変換は不要)
            int msg_id = esp_mqtt_client_publish(mqtt_client, 
                                               "/test/topic/mic", 
                                               (const char *)fb->buf, 
                                               fb->len, 
                                               1,  // QoS 1
                                               0);

            if (msg_id != -1) {
                ESP_LOGI(TAG_MQTT, "Binary image published (msg_id: %d)", msg_id);
            } else {
                ESP_LOGW(TAG_MQTT, "Publish failed - buffer size may be too small");
            }

            camera_return_fb(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(MQTT_TASK_PERIOD_MS));
    }
}

void mqtt_task_start(void)
{
    xTaskCreate(mqtt_task, "mqtt_task", 8192, NULL, 5, NULL);
}
