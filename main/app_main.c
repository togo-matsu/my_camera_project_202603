#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi.h"
#include "mqtt.h"
#include "sensor.h"
#include "camera.h"
#include "driver/i2s_std.h"
#include "driver/i2s_pdm.h"
#include "http_server.h"
static const char *TAG_MIC = "MIC_DEBUG";

void app_main(void) {
    // 1. 各種初期化
    wifi_init();
    //mic_init(); 

    // 2. Wi-Fi接続 (ヘッダ定義のSSID/PASSを使用)
    wifi_connect(WIFI_SSID, WIFI_PASS);

    // IPアドレスが取れるまで少し待つ
    ESP_LOGI(TAG_MIC, "Waiting for Wi-Fi...");
    vTaskDelay(pdMS_TO_TICKS(10000));

    // 2.5 時刻同期 (SNTP) を初期化して同期を待つ
    sntp_initialize();
    if (!sntp_wait_for_sync(10)) {
        ESP_LOGW(TAG_MIC, "SNTP sync failed or timed out; TLS connections may fail if time is incorrect");
    }

    // 3. MQTT初期化
    mqtt_init();

    // 3.5 カメラ初期化
    if (camera_init() != ESP_OK) {
        ESP_LOGW(TAG_MIC, "Camera init failed or not configured");
    }

    // 4. センサー/ MQTT/HTTP サーバ起動
    //sensor_task_start();
    mqtt_task_start();
    //http_server_start();

    // app_mainはタスクへ処理を委譲し、アイドルループに入る
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(20000));
        }
}