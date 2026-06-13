/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @file      driver_ds3231_interface_template.c
 * @brief     driver ds3231 interface template source file
 * @version   2.0.0
 * @author    Shifeng Li
 * @date      2021-03-15
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2021/03/15  <td>2.0      <td>Shifeng Li  <td>format the code
 * <tr><td>2020/11/30  <td>1.0      <td>Shifeng Li  <td>first upload
 * </table>
 */

#include <stdarg.h>
#include <string.h>

#include "driver_ds3231_interface.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "DS3231";
#define I2C_MASTER_SCL_IO 8
#define I2C_MASTER_SDA_IO 9
#define I2C_MASTER_FREQ_HZ 100000

static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t ds3231_dev_handle = NULL;
/**
 * @brief  interface iic bus init
 * @return status code
 *         - 0 success
 *         - 1 iic init failed
 * @note   none
 */
uint8_t ds3231_interface_iic_init(void)
{
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    if (i2c_new_master_bus(&i2c_mst_config, &bus_handle) != ESP_OK)
    {
        return 1;
    }
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x68,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    if (i2c_master_bus_add_device(bus_handle, &dev_cfg, &ds3231_dev_handle) != ESP_OK)
    {
        return 1;
    }
    return 0;
}

/**
 * @brief  interface iic bus deinit
 * @return status code
 *         - 0 success
 *         - 1 iic deinit failed
 * @note   none
 */
uint8_t ds3231_interface_iic_deinit(void)
{
    if (ds3231_dev_handle)
    {
        i2c_master_bus_rm_device(ds3231_dev_handle);
        ds3231_dev_handle = NULL;
    }
    if (bus_handle)
    {
        i2c_del_master_bus(bus_handle);
        bus_handle = NULL;
    }
    return 0;
}

/**
 * @brief      interface iic bus read
 * @param[in]  addr iic device write address
 * @param[in]  reg iic register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len length of the data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t ds3231_interface_iic_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    esp_err_t err = i2c_master_transmit_receive(ds3231_dev_handle, &reg, 1, buf, len, -1);
    return (err == ESP_OK) ? 0 : 1;
}

/**
 * @brief     interface iic bus write
 * @param[in] addr iic device write address
 * @param[in] reg iic register address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of the data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t ds3231_interface_iic_write(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t *write_buf = (uint8_t *)malloc(len + 1);
    if (write_buf == NULL)
        return 1;

    write_buf[0] = reg;
    memcpy(write_buf + 1, buf, len);

    esp_err_t err = i2c_master_transmit(ds3231_dev_handle, write_buf, len + 1, -1);
    free(write_buf);

    return (err == ESP_OK) ? 0 : 1;
}

/**
 * @brief     interface delay ms
 * @param[in] ms time
 * @note      none
 */
void ds3231_interface_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/**
 * @brief     interface print format data
 * @param[in] fmt format data
 * @note      none
 */
void ds3231_interface_debug_print(const char *const fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);

    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    ESP_LOGI(TAG, "%s", buf);
}

/**
 * @brief     interface receive callback
 * @param[in] type interrupt type
 * @note      none
 */
void ds3231_interface_receive_callback(uint8_t type)
{
    switch (type)
    {
    case DS3231_STATUS_ALARM_2:
    {
        ds3231_interface_debug_print("ds3231: irq alarm2.\n");

        break;
    }
    case DS3231_STATUS_ALARM_1:
    {
        ds3231_interface_debug_print("ds3231: irq alarm1.\n");

        break;
    }
    default:
    {
        break;
    }
    }
}
static void ds3231_rtc_task(void *pvParameters)
{
    ds3231_handle_t *handle = (ds3231_handle_t *)pvParameters;

    ds3231_time_t current_time;
    float temp_celsius;
    int16_t temp_raw;
    uint8_t last_second = 99;

    ESP_LOGI(TAG, "RTC Task started in background.");

    while (1)
    {
        if (ds3231_get_time(handle, &current_time) == 0)
        {
            if (current_time.second != last_second)
            {
                last_second = current_time.second;
                ds3231_get_temperature(handle, &temp_raw, &temp_celsius);

                ESP_LOGI(TAG, "Time: %02d:%02d:%02d - %02d/%02d/%04d    Temperature: %.2f °C",
                         current_time.hour, current_time.minute, current_time.second,
                         current_time.date, current_time.month, current_time.year, temp_celsius);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void ds3231_interface_start_task(ds3231_handle_t *handle)
{
    xTaskCreate(ds3231_rtc_task, "rtc_task", 4096, (void *)handle, 5, NULL);
}