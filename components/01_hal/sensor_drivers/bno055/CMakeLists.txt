idf_component_register(SRCS "driver_bno055_interface.c" "driver_bno055.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                    PRIV_REQUIRES
                        i2c_bus 
)

