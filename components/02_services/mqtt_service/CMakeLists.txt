idf_component_register(SRCS "mqtt_task.c"
                        "source/core_mqtt_serializer.c"
                        "source/core_mqtt_state.c"
                        "source/core_mqtt.c"
                    INCLUDE_DIRS "." "source/include" "source/interface"
                    REQUIRES
                        esp_timer
                        esp-tls   
                        esp_netif
                        esp_driver_gpio
                    PRIV_REQUIRES
                        app_types
                    EMBED_TXTFILES
                        "certs/AmazonRootCA1.pem"
                        "certs/device.crt"
                        "certs/device.key"
)
