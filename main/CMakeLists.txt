idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        esp_driver_i2c   
                    PRIV_REQUIRES
                        sensor_manager
                        mqtt_service
                        wifi_service
                        app_types

                        aht20_test
                        bh1750_test
                        bmp280_test
                        ina226_test
                        
                        nvs_manager
                        ble_gatts_svc
                        ble_manager
                        gatt_svc_wifi
)
