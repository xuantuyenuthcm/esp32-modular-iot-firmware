idf_component_register(SRCS "driver_aht20_read_test.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                    PRIV_REQUIRES
                        i2c_bus
                        aht20
)
