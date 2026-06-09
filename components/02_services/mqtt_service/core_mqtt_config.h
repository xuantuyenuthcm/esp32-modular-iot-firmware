#ifndef CORE_MQTT_CONFIG_H_
#define CORE_MQTT_CONFIG_H_

#define LogError( message )
#define LogWarn( message )
#define LogInfo( message )
#define LogDebug( message )

#define MQTT_NETWORK_BUF_SIZE   2048
#define MQTT_KEEP_ALIVE_SEC     60
#define MQTT_CONNECT_TIMEOUT_MS 10000
#define MQTT_RECONNECT_DELAY_MS 5000
#define MQTT_PROCESS_LOOP_MS    100
#define MQTT_PUBLISH_QUEUE_SIZE 10
#define MQTT_SUBSCRIBE_QUEUE_SIZE 10

#define AWS_ENDPOINT  "adiub9wc49ljw-ats.iot.ap-southeast-1.amazonaws.com"
#define AWS_PORT      "8883"
#define AWS_DEVICE_ID "esp32-001"
#define TOPIC_CMD     "devices/" AWS_DEVICE_ID "/ota/command"
#define TOPIC_OTA_CMD "devices/" AWS_DEVICE_ID "/ota/status"

#endif