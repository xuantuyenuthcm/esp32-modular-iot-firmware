idf_component_register(SRCS "driver_bno055_interface.c" "driver_bno055.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        sensor_driver_interface_manager 
                    PRIV_REQUIRES
                        i2c_bus
)

