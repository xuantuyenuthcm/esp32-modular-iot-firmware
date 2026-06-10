idf_component_register(SRCS "driver_bmp280_read_test.c" "driver_bmp280_register_test.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                    PRIV_REQUIRES
                        i2c_bus
                        bmp280
)
