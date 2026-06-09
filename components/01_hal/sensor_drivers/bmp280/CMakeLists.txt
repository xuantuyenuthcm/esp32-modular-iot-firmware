idf_component_register(SRCS "driver_bmp280_interface.c" "driver_bmp280.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                    PRIV_REQUIRES
                        i2c_bus
)
