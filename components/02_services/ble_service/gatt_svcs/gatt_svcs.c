#include "gatt_svcs.h"
#include "nvs_manager.h"
#include "sensor_manager.h"

#include <string.h>
#include "esp_log.h"

static char wifi_ssid[32 + 1] = {0};
static char wifi_pass[64 + 1] = {0};

static const char *TAG = "GATT_SVC";

// Callback of SSID's wifi throught BLE
int ble_wifi_ssid_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
        return BLE_ATT_ERR_UNLIKELY;

    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    ble_hs_mbuf_to_flat(ctxt->om, wifi_ssid, sizeof(wifi_ssid) - 1, NULL);
    wifi_ssid[len] = '\0';

    ESP_LOGI(TAG, "SSID received: \"%s\"", wifi_ssid);
    nvs_manager_save_ssid(wifi_ssid);
    return 0;
}

// Callback of password's wifi throught BLE
int ble_wifi_pw_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
        return BLE_ATT_ERR_UNLIKELY;

    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    ble_hs_mbuf_to_flat(ctxt->om, wifi_pass, sizeof(wifi_pass) - 1, NULL);
    wifi_pass[len] = '\0';

    ESP_LOGI(TAG, "Password received: \"%s\"", wifi_pass);
    nvs_manager_save_password(wifi_pass);
    return 0;
}

typedef enum {
    AHT20_CONNECT,
    BH1750_CONNECT,
    BMP280_CONNECT,
    BNO055_CONNECT,
    INA226_CONNECT,

    AHT20_READ,
    BH1750_READ,
    BMP280_READ,
    BNO055_READ,
    INA226_READ,
    SENSOR_MAX_READ, // Total number of read cmd

    AHT20_NOTIFY,
    BH1750_NOTIFY,
    BMP280_NOTIFY,
    BNO055_NOTIFY,
    INA226_NOTIFY,
    SENSOR_MAX_NOTIFY, // Total number of notify cmd

    ALL_CONNECT = 128,
    ALL_DISCONNECT = 129,
} ble_cmd_recv_t;

typedef struct {
    uint8_t sensor_id;
    bool is_connected;
} ble_sensor_state;

ble_sensor_state sensor_state_ble[SENSOR_MAX] = {
    {SENSOR_AHT20, false},
    {SENSOR_BH1750, false},
    {SENSOR_BMP280, false},
    {SENSOR_BNO055, false},
    {SENSOR_INA226, false},
};

// Implement for cmd from ble app
void ble_sensor_cmd_conn(uint16_t conn_handle, uint16_t attr_handle, ble_cmd_recv_t sensor_id) {
    esp_err_t ret;
    if (sensor_state_ble[sensor_id].is_connected == false) {
        i2c_init();
        ret = sensor_init_table[sensor_id]();
        sensor_state_ble[sensor_id].is_connected = true;
    }
    else {
        ret = sensor_deinit_table[sensor_id]();
        sensor_state_ble[sensor_id].is_connected = false;
    }
    uint8_t notify_data[2];
    notify_data[0] = sensor_id;
    notify_data[1] = (ret == ESP_OK && sensor_state_ble[sensor_id].is_connected == true) ? 0x00 : 0xFF;

    // update status
    struct os_mbuf *om = ble_hs_mbuf_from_flat(notify_data, sizeof(notify_data));
    if (om) {
        ble_gatts_notify_custom(conn_handle, attr_handle, om);
    }
}

// Implement for connect all cmd from ble app 
void ble_sensor_cmd_conn_all(uint16_t conn_handle, uint16_t attr_handle, bool connect_all) {
    esp_err_t ret = ESP_OK;
    if (connect_all) {
        i2c_init();
    }

    for (int sensor_id = 0; sensor_id < SENSOR_MAX; sensor_id++) {
        // If it's Connect all command, init everything
        if (connect_all) {
            if (sensor_state_ble[sensor_id].is_connected == false) {
                ret = sensor_init_table[sensor_id]();
                sensor_state_ble[sensor_id].is_connected = true;
            }
        }
        // If it's disconnect all command, deinit everything
        else {
            if (sensor_state_ble[sensor_id].is_connected == true) {
            ret = sensor_deinit_table[sensor_id]();
            sensor_state_ble[sensor_id].is_connected = false;
            }
        }

        uint8_t notify_data[2];
        notify_data[0] = sensor_id;
        notify_data[1] = (ret == ESP_OK && sensor_state_ble[sensor_id].is_connected == true) ? 0x00 : 0xFF;

        // update status
        struct os_mbuf *om = ble_hs_mbuf_from_flat(notify_data, sizeof(notify_data));
        if (om) {
            ble_gatts_notify_custom(conn_handle, attr_handle, om);
        }  

        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

// Task names table
char task_name[SENSOR_MAX][20] = {
    "aht20_read_task",
    "bh1750_read_task",
    "bmp280_read_task",
    "bno055_read_task",
    "ina226_read_task"
};

typedef struct {
    uint16_t conn_handle;
    uint16_t attr_handle;
} task_param_t;

#define SEN_DELAY 3000
void aht20_read(void *pvParameter) {
    task_param_t *param = (task_param_t *)pvParameter;

    esp_err_t ret = ESP_OK;
    uint8_t humidity;
    while (1) {
        if((ret = aht20_app_read_hum(&humidity)) == ESP_OK) {
            ESP_LOGI("GUI", "Humidity = %u%%", humidity);
        }
        else {
            ESP_LOGI("GUI", "Humidity = none");
        }
        vTaskDelay(pdMS_TO_TICKS(SEN_DELAY));

        // Packet to notify
        uint8_t notify_data[6];
        notify_data[0] = AHT20_NOTIFY;
        notify_data[1] = AHT20_NOTIFY;
        memcpy(&notify_data[2], &humidity, sizeof(uint8_t));

        // update status
        struct os_mbuf *om = ble_hs_mbuf_from_flat(notify_data, sizeof(notify_data));
        if (om) {
            ble_gatts_notify_custom(param->conn_handle, param->attr_handle, om);
        }  
    }
}

void bh1750_read(void *pvParameter) {
    task_param_t *param = (task_param_t *)pvParameter;

    esp_err_t ret = ESP_OK;
    float lux;
    while (1) {
        if ((ret = bh1750fvi_basic_read(&lux)) == ESP_OK) {
            ESP_LOGI("GUI", "Lux = %.2f", lux);
        }
        else {
            ESP_LOGI("GUI", "Lux = none");
        }
        vTaskDelay(pdMS_TO_TICKS(SEN_DELAY));

        // Packet to notify
        uint8_t notify_data[6];
        notify_data[0] = BH1750_NOTIFY;
        notify_data[1] = BH1750_NOTIFY;
        memcpy(&notify_data[2], &lux, sizeof(float));

        // update status
        struct os_mbuf *om = ble_hs_mbuf_from_flat(notify_data, sizeof(notify_data));
        if (om) {
            ble_gatts_notify_custom(param->conn_handle, param->attr_handle, om);
        }  
    }
}

void bmp280_read(void *pvParameter) {
    task_param_t *param = (task_param_t *)pvParameter;

    esp_err_t ret = ESP_OK;
    float temperature;
    float pressure;
    while (1) {
        if ((ret = bmp280_app_read(&temperature, &pressure)) == ESP_OK) {
            ESP_LOGI("GUI", "Temperature = %.2f", temperature);
            ESP_LOGI("GUI", "Pressure = %.2f", pressure);
        }
        else {
            ESP_LOGI("GUI", "Temperature = none");
            ESP_LOGI("GUI", "Pressure = none");
        }
        vTaskDelay(pdMS_TO_TICKS(SEN_DELAY));

        // Packet to notify
        uint8_t notify_data[6];
        notify_data[0] = BMP280_NOTIFY;
        notify_data[1] = BMP280_NOTIFY;
        memcpy(&notify_data[2], &pressure, sizeof(float));

        // update status
        struct os_mbuf *om = ble_hs_mbuf_from_flat(notify_data, sizeof(notify_data));
        if (om) {
            ble_gatts_notify_custom(param->conn_handle, param->attr_handle, om);
        }  
    }
}

void bno055_read(void *pvParameter) {
    task_param_t *param = (task_param_t *)pvParameter;

    esp_err_t ret = ESP_OK;
    float accel;
    while (1) {
        if ((ret = bno055_accel_read(&accel)) == ESP_OK) {
            ESP_LOGI("GUI", "Accel = %.2f", accel);
        }
        else {
            ESP_LOGI("GUI", "Accel = none");

        }
        vTaskDelay(pdMS_TO_TICKS(SEN_DELAY));

        // Packet to notify
        uint8_t notify_data[6];
        notify_data[0] = BNO055_NOTIFY;
        notify_data[1] = BNO055_NOTIFY;
        memcpy(&notify_data[2], &accel, sizeof(float));

        // update status
        struct os_mbuf *om = ble_hs_mbuf_from_flat(notify_data, sizeof(notify_data));
        if (om) {
            ble_gatts_notify_custom(param->conn_handle, param->attr_handle, om);
        }  
    }
}

void ina226_read(void *pvParameter) {
    task_param_t *param = (task_param_t *)pvParameter;

    esp_err_t ret = ESP_OK;
    float battery;
    while (1) {
        if ((ret = ina226_read_get_battery(&battery)) == ESP_OK) {
            ESP_LOGI("GUI", "Battery = %.2f%%", battery);
        }
        else {
            ESP_LOGI("GUI", "Battery = none");
        }
        vTaskDelay(pdMS_TO_TICKS(SEN_DELAY));

        // Packet to notify
        uint8_t notify_data[6];
        notify_data[0] = INA226_NOTIFY;
        notify_data[1] = INA226_NOTIFY;
        memcpy(&notify_data[2], &battery, sizeof(float));

        // update status
        struct os_mbuf *om = ble_hs_mbuf_from_flat(notify_data, sizeof(notify_data));
        if (om) {
            ble_gatts_notify_custom(param->conn_handle, param->attr_handle, om);
        }  
    }
}

// Read sensor functions table
typedef void (*ble_sensor_read_fn_t)(void *);
ble_sensor_read_fn_t ble_sensor_read_fn_table[SENSOR_MAX] = {
    aht20_read,
    bh1750_read,  
    bmp280_read,  
    bno055_read,  
    ina226_read,
};

TaskHandle_t xSensorTaskHandle[SENSOR_MAX] = {NULL};

bool is_reading[SENSOR_MAX] = {false};
// Enable sensor continous read
void ble_sensor_read(uint16_t conn_handle, uint16_t attr_handle, ble_cmd_recv_t data) {
    // Sensor ID
    uint8_t sensor_id = data - SENSOR_MAX;
    task_param_t *param = malloc(sizeof(task_param_t));
    param->conn_handle = conn_handle;
    param->attr_handle = attr_handle;

    if (data >= AHT20_READ && data < SENSOR_MAX_READ) {
        // Read
        if (!is_reading[data - SENSOR_MAX]) {
            // Task for read
            xTaskCreate(
                ble_sensor_read_fn_table[data - SENSOR_MAX], 
                task_name[data - SENSOR_MAX] , 
                2048, 
                param, 
                5, 
                &xSensorTaskHandle[data - SENSOR_MAX]
            );
            is_reading[sensor_id] = true;            
        }
        // Stop
        else {
            vTaskDelete(xSensorTaskHandle[data - SENSOR_MAX]);
            is_reading[sensor_id] = false;
        }

        // Packet to notify
        uint8_t notify_data[2];
        notify_data[0] = sensor_id;
        notify_data[1] = (is_reading[sensor_id] == true) ? 0x01 : 0x02;

        // update status
        struct os_mbuf *om = ble_hs_mbuf_from_flat(notify_data, sizeof(notify_data));
        if (om) {
            ble_gatts_notify_custom(conn_handle, attr_handle, om);
        }  
    }
}

// Callback for control sensor through BLE
int ble_sensor_ctrl_cb(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
        return BLE_ATT_ERR_UNLIKELY;
    
    uint8_t data = ctxt->om->om_data[0];
    ESP_LOGI(TAG, "Command received: \"%d\"", data);

    // Init Sensor, sensor index = data if data less than total number of sensors
    if (data < SENSOR_MAX) {
        ble_sensor_cmd_conn(conn_handle, attr_handle, data);
    }
    
    // Enable sensor continous read    
    ble_sensor_read(conn_handle, attr_handle, data);

    switch(data) {
        case ALL_CONNECT:
            ble_sensor_cmd_conn_all(conn_handle, attr_handle, true);
            break;
        case ALL_DISCONNECT:
            ble_sensor_cmd_conn_all(conn_handle, attr_handle, false);
            break;
        default:
            break;
    }

    return 0;
}

