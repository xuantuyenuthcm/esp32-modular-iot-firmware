idf_component_register(SRCS "sensor_manager.c" "aht20.c" "bh1750.c" "bme280.c" "bmp280.c" "bno055.c" "ina226.c" 
                    INCLUDE_DIRS "include"
                    REQUIRES
                        i2c_bus
                        bh1750
                        bme280
                        bmp280 
                        aht20   
                        ina226
                        bno055 
                    PRIV_REQUIRES
                        app_types
)
