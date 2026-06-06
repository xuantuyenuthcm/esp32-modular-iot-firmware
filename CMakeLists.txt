# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS
    components/01_hal/i2c_bus
    components/01_hal/sensor_drivers/bmp280
    components/01_hal/sensor_drivers/bme280
    components/01_hal/sensor_drivers/bh1750
    components/01_hal/sensor_drivers/ina226
    components/01_hal/sensor_drivers/aht20
    components/01_hal/sensor_drivers/bno055
    components/01_hal/sensor_drivers/sensors_manager
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp32-iot-firmware)
