#include "camera.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h" // vTaskDelayのために追加
#include "freertos/task.h"

static const char *TAG = "CAMERA";

esp_err_t camera_init(void)
{
    camera_config_t config = {0};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;

    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    config.xclk_freq_hz = 10000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 7;
    config.fb_count = 2;

    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: %d", err);
        return err;
    }

    // --- センサー詳細設定の追加 ---
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_brightness(s, 0);         
        s->set_contrast(s, 0);           
        s->set_saturation(s, 0);         
        s->set_whitebal(s, 1);           // ホワイトバランス有効
        s->set_awb_gain(s, 1);           
        s->set_wb_mode(s, 0);            // 0:Auto
        s->set_exposure_ctrl(s, 1);      // 自動露出有効
        s->set_aec2(s, 0);               
        s->set_ae_level(s, 0);           
        s->set_gain_ctrl(s, 1);          // 自動ゲイン有効
        s->set_bpc(s, 0);                
        s->set_wpc(s, 1);                
        s->set_raw_gma(s, 1);            
        s->set_lenc(s, 1);               
        s->set_dcw(s, 1);                

        // XIAO S3 Senseで画像が逆さまならここを 1 に変更してください
        s->set_hmirror(s, 1);            
        s->set_vflip(s, 0);              
    }

    // --- ここから追加: ウォーミングアップ (NO-SOI対策) ---
    ESP_LOGI(TAG, "Camera initialized. Starting warming up...");
    for (int i = 0; i < 10; i++) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            esp_camera_fb_return(fb);
            vTaskDelay(pdMS_TO_TICKS(100)); // 少し待ってから次を撮る
        }
    }
    ESP_LOGI(TAG, "Camera warming up finished");
    // --- ここまで追加 ---

    return ESP_OK;
}

// camera_capture と camera_return_fb はそのままでOK
esp_err_t camera_capture(camera_fb_t **fb)
{
    if (!fb) return ESP_ERR_INVALID_ARG;
    camera_fb_t *frame = esp_camera_fb_get();
    if (!frame) {
        ESP_LOGW(TAG, "Camera capture failed: no frame");
        return ESP_FAIL;
    }
    *fb = frame;
    return ESP_OK;
}

void camera_return_fb(camera_fb_t *fb)
{
    if (fb) esp_camera_fb_return(fb);
}
