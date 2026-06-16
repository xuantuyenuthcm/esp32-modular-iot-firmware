#include "gatt_svc_wifi.h"
#include "nvs_manager.h"
#include "sensor_manager.h"

#include <string.h>
#include "esp_log.h"

static char wifi_ssid[32 + 1] = {0};
static char wifi_pass[64 + 1] = {0};

static const char *TAG = "GATT_SVC_WIFI";

// Callback of SSID's wifi throught BLE
int ble_wifi_ssid_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
        return BLE_ATT_ERR_UNLIKELY;

    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    ble_hs_mbuf_to_flat(ctxt->om, wifi_ssid, sizeof(wifi_ssid) - 1, NULL);
    wifi_ssid[len] = '\0';

    ESP_LOGI(TAG, "SSID received: \"%s\"", wifi_ssid);
    nvs_manager_save_ssid(wifi_ssid);
    return 0;
}

// Callback of password's wifi throught BLE
int ble_wifi_pw_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
        return BLE_ATT_ERR_UNLIKELY;

    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    ble_hs_mbuf_to_flat(ctxt->om, wifi_pass, sizeof(wifi_pass) - 1, NULL);
    wifi_pass[len] = '\0';

    ESP_LOGI(TAG, "Password received: \"%s\"", wifi_pass);
    nvs_manager_save_password(wifi_pass);
    return 0;
}

typedef enum {
    AHT20_CONNECT,
    BH1750_CONNECT,
    BMP280_CONNECT,
    BNO055_CONNECT,
    INA226_CONNECT,

    AHT20_READ,
    BH1750_READ,
    BMP280_READ,
    BNO055_READ,
    INA226_READ,

    AHT20_DISCONNECT,
    BH1750_DISCONNECT,
    BMP280_DISCONNECT,
    BNO055_DISCONNECT,
    INA226_DISCONNECT,
} ble_cmd_recv_t;

// Callback for control sensor through BLE
int ble_sensor_ctrl_cb(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
        return BLE_ATT_ERR_UNLIKELY;

    uint8_t data = ctxt->om->om_data[0];

    switch(data) {
        case AHT20_CONNECT:
            i2c_init();
            esp_err_t ret = sensor_init_table[SENSOR_AHT20]();

            uint8_t notify_data[2];
            notify_data[0] = AHT20_CONNECT;
            notify_data[0] = (ret == ESP_OK) ? 0x00 : 0xFF;

            struct os_mbuf *om = ble_hs_mbuf_from_flat(notify_data, sizeof(notify_data));
            if (om) {
                ble_gatts_notify_custom(conn_handle, attr_handle, om);
            }
            break;

        default:
            break;
    }

    ESP_LOGI(TAG, "Command received: \"%d\"", data);
    return 0;
}