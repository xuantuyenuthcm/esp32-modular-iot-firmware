idf_component_register(SRCS "sensor_manager.c" 
                            "drivers/ds3231/driver_ds3231.c" 
                            "drivers/ds3231/driver_ds3231_interface.c"
                    INCLUDE_DIRS "include" "drivers/ds3231"
                    REQUIRES esp_driver_i2c
                             driver
)