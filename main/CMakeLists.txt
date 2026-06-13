idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES openthread
                             mbedtls 
                             nvs_flash 
                             storage 
                             wifi 
                             ota 
                             sensors 
                             ble
                             app_core
)
