idf_component_register(SRCS "main.c" "bme280.c" "driver_bme280_interface_template.c" "driver_bme280.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        esp_driver_i2c
                        nvs_flash)
