#include "sensor_manager.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver_ds3231_interface.h"
#include <string.h>

static const char *TAG = "SENSOR_MANAGER";

// #define DS3231_SCL_PIN 8
// #define DS3231_SDA_PIN 9

static float sensor_latest_temp = 0;

static ds3231_time_t sensor_latest_time = {0};
static ds3231_handle_t s_ds3231_handle;

static bool sensor_manager_is_initialized = false;
static SemaphoreHandle_t s_data_mutex = NULL; // Thêm ổ khóa Mutex

// Task
static void sensor_manager_task(void *arg)
{
    ESP_LOGI(TAG, "Sensor manager background task started");
    bool is_sensor_ready = false;

    while (1)
    {
        // try to init sensors if not ready or false => retry in 5s later
        if (!is_sensor_ready)
        {
            uint8_t err = ds3231_init(&s_ds3231_handle);
            if (err != 0)
            {
                ESP_LOGE(TAG, "DS3231 init failed, retrying in 5 seconds");
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
            ESP_LOGI(TAG, "DS3231 initialized");
            is_sensor_ready = true;
        }

        int16_t raw_temp = 0;
        if (ds3231_get_temperature(&s_ds3231_handle, &raw_temp, &sensor_latest_temp) == 0)
        {
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read DS3231 temperature");
            is_sensor_ready = false;
            ds3231_deinit(&s_ds3231_handle);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        ds3231_time_t temp_time;
        if (ds3231_get_time(&s_ds3231_handle, &temp_time) == 0)
        {
            if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE)
            {
                sensor_latest_time = temp_time;
                xSemaphoreGive(s_data_mutex);
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read DS3231 time");
            is_sensor_ready = false;
            ds3231_deinit(&s_ds3231_handle);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// init
esp_err_t sensor_manager_init(void)
{
    if (sensor_manager_is_initialized)
    {
        return ESP_OK;
    }

    if (s_data_mutex == NULL)
    {
        s_data_mutex = xSemaphoreCreateMutex();
        if (s_data_mutex == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    // link driver
    DRIVER_DS3231_LINK_INIT(&s_ds3231_handle, ds3231_handle_t);
    DRIVER_DS3231_LINK_IIC_INIT(&s_ds3231_handle, ds3231_interface_iic_init);
    DRIVER_DS3231_LINK_IIC_DEINIT(&s_ds3231_handle, ds3231_interface_iic_deinit);
    DRIVER_DS3231_LINK_IIC_READ(&s_ds3231_handle, ds3231_interface_iic_read);
    DRIVER_DS3231_LINK_IIC_WRITE(&s_ds3231_handle, ds3231_interface_iic_write);
    DRIVER_DS3231_LINK_DELAY_MS(&s_ds3231_handle, ds3231_interface_delay_ms);
    DRIVER_DS3231_LINK_DEBUG_PRINT(&s_ds3231_handle, ds3231_interface_debug_print);
    DRIVER_DS3231_LINK_RECEIVE_CALLBACK(&s_ds3231_handle, ds3231_interface_receive_callback);

    // task sensor
    if (xTaskCreate(sensor_manager_task, "sensor_task", 4096, NULL, 5, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create sensor task");
        return ESP_FAIL;
    }

    sensor_manager_is_initialized = true;
    ESP_LOGI(TAG, "sensor manager initialized ");
    return ESP_OK;
}

// deinit
esp_err_t sensor_manager_deinit(void)
{
    if (!sensor_manager_is_initialized)
    {
        return ESP_OK;
    }

    ds3231_deinit(&s_ds3231_handle);

    if (s_data_mutex)
    {
        vSemaphoreDelete(s_data_mutex);
        s_data_mutex = NULL;
    }

    sensor_manager_is_initialized = false;
    ESP_LOGI(TAG, "sensor manager deinitialized");
    return ESP_OK;
}

// get raw data
esp_err_t sensor_manager_get_raw_data(float *temp, uint16_t *year, uint8_t *month, uint8_t *date, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    if (s_data_mutex != NULL && xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE)
    {
        if (temp)
            *temp = sensor_latest_temp;
        if (year)
            *year = sensor_latest_time.year;
        if (month)
            *month = sensor_latest_time.month;
        if (date)
            *date = sensor_latest_time.date;
        if (hour)
            *hour = sensor_latest_time.hour;
        if (minute)
            *minute = sensor_latest_time.minute;
        if (second)
            *second = sensor_latest_time.second;

        xSemaphoreGive(s_data_mutex);
    }
    return ESP_OK;
}

// set time
esp_err_t sensor_manager_set_time(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second)
{
    if (!sensor_manager_is_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    ds3231_time_t t;
    memset(&t, 0, sizeof(t));
    t.year = year;
    t.month = month;
    t.date = date;
    t.hour = hour;
    t.minute = minute;
    t.second = second;
    t.format = DS3231_FORMAT_24H;
    t.am_pm = DS3231_AM;
    t.week = 1;

    if (ds3231_set_time(&s_ds3231_handle, &t) != 0)
    {
        ESP_LOGE(TAG, "Failed to set RTC time");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "RTC Time successfully updated to: %04d-%02d-%02d %02d:%02d:%02d", year, month, date, hour, minute, second);
    return ESP_OK;
}