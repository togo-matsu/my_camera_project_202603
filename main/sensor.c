#include <stdio.h>
#include <stdlib.h>
#include "sensor.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "driver/i2s_pdm.h" // 追加
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdint.h>
#include "tasks.h"

#define I2S_PORT            I2S_NUM_0
#define SAMPLE_RATE         16000

// グローバル変数（sensor.hでextern宣言されているもの）
i2s_chan_handle_t rx_handle;

static const char *TAG_SENSOR = "SENSOR";

static SemaphoreHandle_t vol_mutex = NULL;
static int latest_volume = 0;
static char latest_volume_str[16] = "0";
static int16_t samples[SAMPLE_BUFFER_SIZE];


void mic_init(void)
{
    // チャンネル設定
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, NULL, &rx_handle);

    // PDM RX専用の設定
    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        // 16bitで取得するのが一般的です
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = GPIO_NUM_42, // Senseのマイククロックは42
            .din = GPIO_NUM_41, // Senseのマイクデータは41
            .invert_flags = { .clk_inv = false },
        },
    };

    // PDMモードとして初期化
    i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_rx_cfg);
    i2s_channel_enable(rx_handle);
}


static void mic_task(void *arg)
{
    (void)arg;
    for (;;) {
        size_t bytes_read = 0;
        esp_err_t err = i2s_channel_read(rx_handle, samples, sizeof(samples), &bytes_read, pdMS_TO_TICKS(100));

        if (err == ESP_OK && bytes_read > 0) {
            int samples_count = bytes_read / sizeof(int16_t);
            int64_t sum = 0;
            for (int i = 0; i < samples_count; i++) {
                sum += abs(samples[i]);
            }
            int avg_volume = (samples_count > 0) ? (int)(sum / samples_count) : 0;

            if (vol_mutex && xSemaphoreTake(vol_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                latest_volume = avg_volume;
                snprintf(latest_volume_str, sizeof(latest_volume_str), "%d", latest_volume);
                xSemaphoreGive(vol_mutex);
            }
        } else {
            ESP_LOGW(TAG_SENSOR, "I2S Read Error or No Data in mic_task");
        }

        vTaskDelay(pdMS_TO_TICKS(MIC_TASK_PERIOD_MS)); // MIC_TASK_PERIOD_MS
    }
}

int sensor_get_latest_volume_str(char *buf, size_t bufsize)
{
    if (!buf || bufsize == 0) return 0;
    if (vol_mutex && xSemaphoreTake(vol_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        strncpy(buf, latest_volume_str, bufsize - 1);
        buf[bufsize - 1] = '\0';
        xSemaphoreGive(vol_mutex);
        return 1;
    }
    return 0;
}

void sensor_task_start(void)
{
    if (!vol_mutex) vol_mutex = xSemaphoreCreateMutex();
    if (!vol_mutex) {
        ESP_LOGE(TAG_SENSOR, "Failed to create volume mutex");
    }
    xTaskCreate(mic_task, "mic_task", 4096, NULL, 5, NULL);
}