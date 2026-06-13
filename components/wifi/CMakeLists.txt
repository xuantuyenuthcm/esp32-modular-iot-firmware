idf_component_register(SRCS "wifi_service.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi 
                             esp_netif 
                             esp_event 
                             lwip 
                             storage
                             esp_http_client
)
