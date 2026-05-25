idf_component_register(SRCS "main.c" 
                            "bmp280.c" "driver_bmp280_interface_template.c" "driver_bmp280.c"
                            "bme280.c" "driver_bme280_interface_template.c" "driver_bme280.c" 
                            "bh1750.c" "driver_bh1750fvi_interface_template.c" "driver_bh1750fvi.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        esp_driver_i2c)
