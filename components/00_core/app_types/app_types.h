#ifndef RTOS_RESOURCES_H
#define RTOS_RESOURCES_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TASK_STACK_SENSOR       4096
#define TASK_PRIO_SENSOR        5
#define TASK_STACK_MQTT         4096
#define TASK_PRIO_MQTT          5

#define OTA_URL_MAX_LEN     1024
#define OTA_VERSION_MAX_LEN 48

#define LED_MQTT_GPIO 2
#define EVT_WIFI_CONNECTED  (1 << 0)
#define EVT_MQTT_CONNECTED  (1 << 1)
#define EVT_OTA_IN_PROGRESS (1 << 2)

typedef enum {
    CMD_UNKNOWN = 0,
    CMD_GPIO_SET,
    CMD_GPIO_RESET,
    CMD_REBOOT,
} cmd_type_t;

typedef struct {
    cmd_type_t cmd;
    uint8_t    gpio_pin;
    uint32_t   duration_ms;
} control_cmd_t;

typedef struct {
    char   topic[128];
    char   payload[512];
    size_t payload_len;
} mqtt_publish_msg_t;

typedef struct {
    char   topic[128];
    char   payload[512];
    size_t payload_len;
} mqtt_subscribe_msg_t;

typedef struct {
    uint8_t humidity;       // AHT20
    float lux;              // BH1750
    float temperature;      // BMP280
    float pressure;         // BMP280
    float accel;            // BNO055
    float battery;          // INA226
} sensor_data_t;

typedef struct {
    char *device;
    uint8_t addr;
    bool i2c_init_flag;
    bool sensor_init_flag;
}sensor_state_t;

extern EventGroupHandle_t g_system_event_group;
extern SemaphoreHandle_t  g_mqtt_ready_sem;
extern QueueHandle_t      g_mqtt_publish_queue;
extern QueueHandle_t      g_mqtt_subscribe_queue;
extern QueueHandle_t      g_control_queue;
extern QueueHandle_t      g_ble_send_queue;

extern char g_ota_url[OTA_URL_MAX_LEN];
extern char g_ota_version[OTA_VERSION_MAX_LEN];

cmd_type_t parse_cmd_name(const char *name);

#endif // RTOS_RESOURCES_H
