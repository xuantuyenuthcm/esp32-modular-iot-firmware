#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <stdint.h>
#include "app_types.h"

#define AHT20_ADDRESS       0x38
#define BH1750FVI_ADDRESS   0x23
#define BMP280_ADDRESS      0x76
#define BNO055_ADDRESS      0x29
#define INA226_ADDRESS      0x40

typedef enum {
    SENSOR_AHT20,
    SENSOR_BH1750,
    SENSOR_BMP280,
    SENSOR_BNO055,
    SENSOR_INA226,
    SENSOR_MAX
} sensor_id_t;

extern sensor_state_t sensor_state[SENSOR_MAX];

uint8_t sensor_id_to_address(sensor_id_t sensor_id);
const char *sensor_id_to_str(sensor_id_t sensor_id);

#endif