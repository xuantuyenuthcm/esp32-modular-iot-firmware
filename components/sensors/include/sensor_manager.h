#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t sensor_manager_init(void);

esp_err_t sensor_manager_deinit(void);

esp_err_t sensor_manager_get_raw_data(float *temp, uint16_t *year, uint8_t *month, uint8_t *date, uint8_t *hour, uint8_t *minute, uint8_t *second);

esp_err_t sensor_manager_set_time(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second);

#endif