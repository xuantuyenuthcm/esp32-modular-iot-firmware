#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

// include components
#include "storage_service.h"
#include "wifi_service.h"
#include "ota_service.h"
#include "ble_manager.h"
#include "sensor_manager.h"
#include "app_version.h"
// #include "mqtt_service.h"

static const char *TAG = "APP_MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Firmware version: %s", FW_VERSION_STRING);

    // init NVS
    ESP_ERROR_CHECK(storage_service_init());

    // init sensor
    ESP_ERROR_CHECK(sensor_manager_init());

    // init BLE
    ble_manager_init();

    // init Wifi service
    ESP_ERROR_CHECK(wifi_service_init());

    //  Try connecting to Wifi
    char ssid[65] = {0};
    char password[65] = {0};
    if (storage_service_read_wifi(ssid, sizeof(ssid), password, sizeof(password)))
    {
        wifi_service_sta_connect(ssid, password);
    }

    // wait for Wifi to connect
    int retry_wifi_connect_count = 0;
    while (!wifi_service_is_connected() && retry_wifi_connect_count <= 10)
    {
        ESP_LOGI(TAG, "waiting for wifi connection...%d", retry_wifi_connect_count);
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry_wifi_connect_count++;
    }

    if (wifi_service_is_connected())
    {
        ESP_LOGI(TAG, "Wifi successfully connected. Device is running in online mode");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to connect to Wifi. Device is running in offline mode");
    }

    // init OTA service
    ESP_ERROR_CHECK(ota_service_init());

    uint32_t uptime_internet_sec = 0;

    while (1)
    {
        // check wifi + internet
        if (wifi_service_is_connected())
        {
            if (uptime_internet_sec % 30 == 0)
            {
                bool has_internet = wifi_service_test_internet();
                if (!has_internet)
                {
                    ESP_LOGW(TAG, "Connected to WiFi but have NO Internet access");
                }
                else
                {
                    ESP_LOGI(TAG, "Internet access is avaialbe");
                }
            }
        }
        else
        {
            ESP_LOGW(TAG, "Wifi is disconnected");
            if (storage_service_read_wifi(ssid, sizeof(ssid), password, sizeof(password)))
            {
                wifi_service_sta_connect(ssid, password);
            }
        }

        // OTA download
        if (ota_service_is_busy())
        {
            ESP_LOGI(TAG, "OTA is running");
        }

        //
        vTaskDelay(pdMS_TO_TICKS(5000));
        uptime_internet_sec += 5;
    }
}