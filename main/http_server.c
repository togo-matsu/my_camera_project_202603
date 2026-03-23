#include "http_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <string.h>
#include "camera.h"

static const char *TAG = "HTTP_SERVER";
static httpd_handle_t server = NULL;

static esp_err_t root_get_handler(httpd_req_t *req)
{
    // <meta http-equiv="refresh" content="10"> を追加（10秒リフレッシュ）
    const char* resp = "<html>"
                       "<head><meta http-equiv='refresh' content='10'></head>"
                       "<body>"
                       "<h3>ESP32-S3 Camera (10s Refresh)</h3>"
                       "<img src='/capture' style='width:100%; max-width:640px;'>"
                       "<p>Next update in 10 seconds...</p>"
                       "</body>"
                       "</html>";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}



static esp_err_t capture_get_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    if (camera_capture(&fb) == ESP_OK && fb) {
        httpd_resp_set_type(req, "image/jpeg");
        httpd_resp_send(req, (const char *)fb->buf, fb->len);
        camera_return_fb(fb);
    } else {
        httpd_resp_set_type(req, "application/json");
        const char *err = "{\"error\": \"camera_capture_failed\"}";
        httpd_resp_send(req, err, HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

static httpd_uri_t capture_uri = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = capture_get_handler,
    .user_ctx = NULL
};

static httpd_uri_t root_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
    .user_ctx = NULL
};

void http_server_start(void)
{
    if (server) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    config.stack_size = 10240;      // 必須：4096だと画像処理で落ちることがあります
    config.max_uri_handlers = 8;    // 少し余裕を持たせる
    config.max_open_sockets = 5;    // 同時接続数を少し絞って安定させる
    config.lru_purge_enable = true; // 古い接続を自動で切断する
    config.task_priority = 5;       // MQTTタスクより少し優先度を上げるか同等にする

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        server = NULL;
        return;
    }
    httpd_register_uri_handler(server, &root_uri);
    httpd_register_uri_handler(server, &capture_uri);
    ESP_LOGI(TAG, "HTTP server started");
}

void http_server_stop(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}
