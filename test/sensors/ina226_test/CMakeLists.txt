idf_component_register(SRCS "driver_ina226_alert_test.c" "driver_ina226_read_test.c" "driver_ina226_register_test.c"
                    INCLUDE_DIRS "include"
                    REQUIRES 
                    PRIV_REQUIRES
                        i2c_bus
                        ina226
)
