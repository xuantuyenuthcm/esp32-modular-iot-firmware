idf_component_register(SRCS "gatt_svcs.c" 
                    INCLUDE_DIRS "include"
                    REQUIRES bt
                    PRIV_REQUIRES nvs_manager
                    sensor_manager
                    )
