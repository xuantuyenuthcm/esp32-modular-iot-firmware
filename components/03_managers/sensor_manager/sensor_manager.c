#include "sensor_manager.h"
#include "aht20.h"
#include "bh1750.h"
#include "bmp280.h"
#include "bno055.h"
#include "ina226.h"
#include "rtos_config.h"

#define AWS_DEVICE_ID "esp32-001"
#define TOPIC_CMD     "devices/" AWS_DEVICE_ID "/ota/command"
#define TOPIC_OTA_CMD "devices/" AWS_DEVICE_ID "/ota/status"

/**
 * @brief Packing Json
 * @param[out]  buffer packed data buffer 
 * @param[in]   buffer_size raw data size 
 * @param[in]   humidity %
 * @param[in]   lux lux
 * @param[in]   temperature Celcius 
 * @param[in]   pressure Pa
 * @param[in]   accel 
 * @param[in]   battery %
 */
static void sensor_json(char *buffer, uint8_t buffer_size, sensor_data_t sensor_data) {
    snprintf(buffer, buffer_size,
                "{"
                "\"humidity\":%u,"
                "\"lux\":%.1f,"
                "\"temperature\":%.1f,"
                "\"pressure\":%.2f,"
                "\"accel\":%.2f,"
                "\"battery\":%.1f"                
                "}",
            sensor_data.humidity, sensor_data.lux, 
            sensor_data.temperature, sensor_data.pressure, 
            sensor_data.accel, sensor_data.battery);
}

// Initialize all sensors
void sensor_init() {
    aht20_full_init();
    bh1750fvi_full_init();
    bmp280_full_init();
    bno055_full_init();
    ina226_full_init();
}

// Read data
static sensor_data_t sensor_read_all() {
    sensor_data_t sensor_data = {0};

    // Humidity
    if (aht20_app_read_hum(&sensor_data.humidity) != 0) {
        sensor_data.humidity = 0;
    }

    // Lux
    if (bh1750fvi_basic_read(&sensor_data.lux) != 0) {
        sensor_data.lux = 0;
    }
    
    // Temperature + pressure
    if (bmp280_app_read(&sensor_data.temperature, &sensor_data.pressure) != 0) {
        sensor_data.temperature = 0;
        sensor_data.pressure = 0;
    }

    // Accelarate
    if (bno055_accel_read(&sensor_data.accel) != 0) {
        sensor_data.accel = 0;
    }

    // Battery
    if (ina226_read_get_battery(&sensor_data.battery) != 0) {
        sensor_data.battery = 0;
    }

    return sensor_data;
}

// Read and get all data and converse to JSON
void sensor_get_data_json_packet(char *buffer, int buffer_size) {
    sensor_data_t sensor_data;
    
    sensor_data = sensor_read_all();
    sensor_json(buffer, buffer_size, sensor_data);
}

void sensor_task(void *pvParameter) {

    xEventGroupWaitBits(g_system_event_group, EVT_MQTT_CONNECTED, pdFALSE, pdTRUE, portMAX_DELAY);

    i2c_init();
    sensor_init();

    char buffer[128];
    memset(buffer, 0, sizeof(buffer));

    mqtt_publish_msg_t msg;

    while (1) {
        sensor_get_data_json_packet(buffer, sizeof(buffer));
        printf("%s\n", buffer);

        // Package data for publish
        strncpy(msg.topic, TOPIC_CMD, sizeof(msg.topic) - 1);
        msg.topic[sizeof(msg.topic) - 1] = '\0';    
        strncpy(msg.payload, buffer, sizeof(msg.payload) - 1);
        msg.payload[sizeof(msg.payload) - 1] = '\0';
        msg.payload_len = strlen(msg.payload);

        if (xQueueSend(g_mqtt_publish_queue, &msg, 0) != pdTRUE) {
            ESP_LOGE("Sensor_task", "Publish queue full - data droped !");
        }
        if (xQueueSend(g_ble_send_queue, &msg, 0) != pdTRUE) {
            ESP_LOGE("Sensor_task", "BLE queue full - data droped !");
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_PROCESS_LOOP_MS));
    }
}