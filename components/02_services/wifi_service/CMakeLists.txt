idf_component_register(SRCS "wifi_task.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        esp_wifi
                        esp_timer
                    PRIV_REQUIRES
                        app_types
                        nvs_manager
)
