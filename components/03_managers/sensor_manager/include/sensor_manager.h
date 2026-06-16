#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "i2c_manager.h"
#include "sensor_error.h"
#include "sensor_types.h"

#define SENSOR_PROCESS_LOOP_MS    5000

typedef esp_err_t (*sensor_init_fn_t)(void);
extern sensor_init_fn_t sensor_init_table[SENSOR_MAX];

typedef uint8_t (*sensor_deinit_fn_t)(void);
extern sensor_deinit_fn_t sensor_deinit_table[SENSOR_MAX];

void sensor_init();
void sensor_get_data_json_packet(char *buffer, int buffer_size);
void sensor_task(void *pvParameter);

void aht20_read(void *pvParameter);
void bh1750_read(void *pvParameter);
void bmp280_read(void *pvParameter);
void bno055_read(void *pvParameter);
void ina226_read(void *pvParameter);

#endif