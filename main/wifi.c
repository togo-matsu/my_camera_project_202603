#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_sntp.h"
#include <time.h>
#include <sys/time.h>

static const char *TAG = "WiFi";

void wifi_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // --- ここから追加 ---
    ESP_ERROR_CHECK(esp_netif_init());      // TCP/IPスタックの初期化
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // イベントループの作成（重要！）
    esp_netif_create_default_wifi_sta();    // STAモードのネットワークインターフェース作成
    // --- ここまで追加 ---

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

// SNTPを初期化して時刻同期を行う（日本時間に設定）
void sntp_initialize(void)
{
    setenv("TZ", "JST-9", 1);
    tzset();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.nict.jp"); 
    esp_sntp_setservername(1, "time.google.com"); 
    esp_sntp_setservername(2, "time.aws.com");
    esp_sntp_init();

    ESP_LOGI(TAG, "SNTP initialized (ntp.nict.jp)");
}

// SNTPが同期されるまで待つ。戻り値: 1=同期成功, 0=失敗
int sntp_wait_for_sync(int max_retries)
{
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;

    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < max_retries) {
        ESP_LOGI(TAG, "Waiting for SNTP sync... (%d/%d)", retry + 1, max_retries);
        vTaskDelay(pdMS_TO_TICKS(2000));
        retry++;
    }

    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGW(TAG, "Time not set after SNTP");
        return 0;
    }

    ESP_LOGI(TAG, "Time synchronized: %s", asctime(&timeinfo));
    return 1;
}

void wifi_connect(const char* ssid, const char* password) {
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Connecting to %s...", ssid);
}

void wifi_disconnect(void) {
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_LOGI(TAG, "Disconnected from Wi-Fi");
}