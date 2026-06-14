#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "i2c_manager.h"
#include "sensor_error.h"
#include "sensor_types.h"

#define SENSOR_PROCESS_LOOP_MS    5000

void sensor_init();
void sensor_get_data_json_packet(char *buffer, int buffer_size);
void sensor_task(void *pvParameter);

#endif