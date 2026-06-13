#include "wifi_service.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_client.h"
#include <string.h>

#define MAXIMUM_RETRY 5

static const char *TAG = "WIFI_SERVICE";

static wifi_svc_sts_t current_status = WIFI_SVC_STS_STA_IDLE;
static int retry_num = 0;
static bool is_wifi_initialized = false;
static bool wifi_has_internet = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        current_status = WIFI_SVC_STS_STA_CONNECTING;
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_has_internet = false;

        if (retry_num < MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP...");
            current_status = WIFI_SVC_STS_STA_CONNECTING;
        }
        else
        {
            current_status = WIFI_SVC_STS_STA_CONNECT_FAILED;
            ESP_LOGI(TAG, "Connection to the AP failed");
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
        current_status = WIFI_SVC_STS_STA_GOT_IP;
    }
}

esp_err_t wifi_service_init(void)
{
    if (is_wifi_initialized)
    {
        return ESP_OK;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // check, if there is any stored id => connect
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    is_wifi_initialized = true;
    current_status = WIFI_SVC_STS_STA_IDLE;
    ESP_LOGI(TAG, "Wifi service initialized");
    return ESP_OK;
}

esp_err_t wifi_service_deinit(void)
{
    if (!is_wifi_initialized)
    {
        return ESP_OK;
    }

    esp_wifi_stop();
    esp_wifi_deinit();

    is_wifi_initialized = false;
    current_status = WIFI_SVC_STS_STA_IDLE;
    return ESP_OK;
}

esp_err_t wifi_service_sta_connect(const char *ssid, const char *password)
{
    if (!is_wifi_initialized)
    {
        ESP_LOGE(TAG, "Wifi service not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (ssid == NULL || strlen(ssid) == 0)
    {
        ESP_LOGE(TAG, "SSID cannot be empty");
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };

    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password != NULL)
    {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }

    if (password == NULL || strlen(password) == 0)
    {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    retry_num = 0;
    esp_wifi_connect();

    ESP_LOGI(TAG, "Connecting to AP: %s", ssid);
    return ESP_OK;
}

esp_err_t wifi_service_sta_disconnect(void)
{
    if (!is_wifi_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_wifi_disconnect();
}

bool wifi_service_is_connected(void)
{
    return (current_status == WIFI_SVC_STS_STA_GOT_IP);
}

wifi_svc_sts_t wifi_service_sta_get_sts(void)
{
    return current_status;
}

bool wifi_service_test_internet(void)
{
    if (!wifi_service_is_connected())
    {
        wifi_has_internet = false;
        return false;
    }

    ESP_LOGI(TAG, "Testing internet connection...");
    esp_http_client_config_t config = {
        .url = "http://www.google.com",
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    bool is_connected = false;

    if (err == ESP_OK)
    {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Internet connection test successful. HTTP Status = %d", status_code);
        is_connected = (status_code >= 200 && status_code < 400);
    }
    else
    {
        ESP_LOGE(TAG, "Internet connection test failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    wifi_has_internet = is_connected;
    return is_connected;
}

bool wifi_service_has_internet(void)
{
    return wifi_has_internet;
}