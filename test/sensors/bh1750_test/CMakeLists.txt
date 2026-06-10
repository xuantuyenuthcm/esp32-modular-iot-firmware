idf_component_register(SRCS "driver_bh1750fvi_read_test.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                    PRIV_REQUIRES
                        i2c_bus
                        bh1750
)
