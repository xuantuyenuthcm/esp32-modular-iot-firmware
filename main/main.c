#include <stdio.h>
#include "esp_log.h"
#include "bme280.h"
#include "nvs_flash.h"

void app_main(void)
{
    bme280_I2C_init();

    float temperature;
    float pressure;
    float humidity;

    bme280_app_read(&temperature, &pressure, &humidity);
    
    printf("Temperature: %f.2\n", temperature);
    printf("Pressure: %f.2\n", pressure);
    printf("humidity: %f.2%%\n", humidity);
}
