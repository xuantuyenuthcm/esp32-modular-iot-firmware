#ifndef SENSOR_ERROR_H
#define SENSOR_ERROR_H

typedef enum {
    SENSOR_OK,
    SENSOR_FAIL,
    SENSOR_HANDLE_NULL,
    SENSOR_NOT_INITIALIZE,
    SENSOR_FUNC_FAIL,       // read data fail, mode invalid, power on/down fail, compensate param fail, id error, math error
    SENSOR_SYS_ERR,         // reset fail, crc fail, read timeout, get nvm calibration failed
    SENSOR_READ_CALI_FAIL   // read calibration failed
}sensor_error_t;

#endif