#include <stdio.h>
#include "i2c_manager.h"
#include "bh1750.h"
#include "bmp280.h"
#include "aht20.h"
#include "ina226.h"
#include "bno055.h"
#include "i2c_scan.h"
#include "mqtt_task.h"
#include "wifi_task.h"
#include "core_mqtt.h"
#include "core_mqtt_config.h"
#include "app_types.h"
#include "sensor_manager.h"
#include "nvs_manager.h"
#include "ble_manager.h"
#include "ble_gatts_svc.h"

static const char *TAG = "MAIN";

EventGroupHandle_t g_system_event_group     = NULL;
SemaphoreHandle_t  g_mqtt_ready_sem         = NULL;
QueueHandle_t      g_mqtt_publish_queue     = NULL;
QueueHandle_t      g_mqtt_subscribe_queue   = NULL;
QueueHandle_t      g_control_queue          = NULL;
QueueHandle_t      g_sensor_queue           = NULL;
QueueHandle_t      g_ble_send_queue         = NULL; 

QueueHandle_t      g_pressure_notify_queue  = NULL;

static esp_err_t rtos_resources_create(void)
{
    g_system_event_group = xEventGroupCreate();
    if (g_system_event_group == NULL) return ESP_ERR_NO_MEM;

    g_mqtt_ready_sem = xSemaphoreCreateBinary();
    if (g_mqtt_ready_sem == NULL) return ESP_ERR_NO_MEM;

    g_mqtt_publish_queue = xQueueCreate(MQTT_PUBLISH_QUEUE_SIZE, sizeof(mqtt_publish_msg_t));
    if (g_mqtt_publish_queue == NULL) return ESP_ERR_NO_MEM;

    g_mqtt_subscribe_queue = xQueueCreate(MQTT_SUBSCRIBE_QUEUE_SIZE, sizeof(mqtt_subscribe_msg_t));
    if (g_mqtt_subscribe_queue == NULL) return ESP_ERR_NO_MEM;

    g_control_queue = xQueueCreate(10, sizeof(control_cmd_t));    
    if (g_control_queue == NULL) return ESP_ERR_NO_MEM;
    
    g_ble_send_queue = xQueueCreate(10, sizeof(mqtt_publish_msg_t));
    if (g_ble_send_queue == NULL) return ESP_ERR_NO_MEM;

    g_pressure_notify_queue = xQueueCreate(10, sizeof(float));
    if (g_pressure_notify_queue == NULL) return ESP_ERR_NO_MEM;

    ESP_LOGI(TAG, "RTOS resources created");
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "System init");
    nvs_manager_init();

    ble_manager_init();
    gatt_svc_init();
    ble_manager_start();

    esp_err_t ret = rtos_resources_create();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RTOS resources: %d", ret);
        return;
    }

    ret = wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %d", ret);
        return;
    }

    xTaskCreate(mqtt_task, "mqtt", TASK_STACK_MQTT, NULL, TASK_PRIO_MQTT, NULL);
    xEventGroupWaitBits(g_system_event_group, EVT_MQTT_CONNECTED, pdFALSE, pdTRUE, portMAX_DELAY);

    // xTaskCreate(sensor_task, "sensor", TASK_STACK_SENSOR, NULL, TASK_PRIO_SENSOR, NULL);

}