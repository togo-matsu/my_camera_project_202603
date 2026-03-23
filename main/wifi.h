#ifndef WIFI_H
#define WIFI_H

// デフォルトのSSID/PASSをヘッダで定義（必要ならビルド時に置換してください）
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

void wifi_init(void);
void wifi_connect(const char* ssid, const char* password);
void wifi_disconnect(void);
// SNTP (時刻同期) 初期化と同期待ち
void sntp_initialize(void);
int sntp_wait_for_sync(int max_retries);

#endif // WIFI_H