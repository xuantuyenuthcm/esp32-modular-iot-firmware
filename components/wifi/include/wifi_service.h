#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include <stdbool.h>
#include "esp_err.h"

typedef enum
{
    WIFI_SVC_STS_STA_IDLE = 0,
    WIFI_SVC_STS_STA_CONNECTING,
    WIFI_SVC_STS_STA_CONNECTED,
    WIFI_SVC_STS_STA_GOT_IP,
    WIFI_SVC_STS_STA_DISCONNECTED,
    WIFI_SVC_STS_STA_CONNECT_FAILED,
} wifi_svc_sts_t;

esp_err_t wifi_service_init(void);

esp_err_t wifi_service_deinit(void);

esp_err_t wifi_service_sta_connect(const char *ssid, const char *password);

esp_err_t wifi_service_sta_disconnect(void);

wifi_svc_sts_t wifi_service_sta_get_sts(void);

bool wifi_service_is_connected(void);

bool wifi_service_test_internet(void);

bool wifi_service_has_internet(void);

#endif