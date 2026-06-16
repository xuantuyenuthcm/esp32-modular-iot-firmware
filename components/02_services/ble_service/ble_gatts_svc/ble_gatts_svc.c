#include "ble_gatts_svc.h"
#include <stdio.h>
#include "esp_log.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "gatt_svcs.h"

static const char *TAG = "GATT_SVC";

//  WIFI====================================
//  76c3e5d6-ee9e-4f48-bb4d-287d88f872d6
static const ble_uuid128_t wifi_info_uuid = BLE_UUID128_INIT(0xd6, 0x72, 0xf8, 0x88, 0x7d, 0x28, 0x4d, 0xbb,
                                                             0x48, 0x4f, 0x9e, 0xee, 0xd6, 0xe5, 0xc3, 0x76);
//  6fc254b7-6bc0-475b-b836-720d2287922d
static const ble_uuid128_t wifi_ssid_info_uuid = BLE_UUID128_INIT(0x2d, 0x92, 0x87, 0x22, 0x0d, 0x72, 0x36, 0xb8,
                                                                  0x5b, 0x47, 0xc0, 0x6b, 0xb7, 0x54, 0xc2, 0x6f);
//  cfedbbc2-0598-4581-b1fd-b7d54fec0396
static const ble_uuid128_t wifi_pw_uuid = BLE_UUID128_INIT(0x96, 0x03, 0xec, 0x4f, 0xd5, 0xb7, 0xfd, 0xb1,
                                                           0x81, 0x45, 0x98, 0x05, 0xc2, 0xbb, 0xed, 0xcf);
            
//  SENSOR =================================
//  2d1de7b5-0e15-425f-941d-6b5a1372ebea
static const ble_uuid128_t sensor_monitor_uuid = BLE_UUID128_INIT(0xea, 0xeb, 0x72, 0x13, 0x5a, 0x6b, 0x1d, 0x94,
                                                                  0x5f, 0x42, 0x15, 0x0e, 0xb5, 0xe7, 0x1d, 0x2d);
// 45fb1ea9-6f80-48a4-9337-4e8e4d455e2c
static const ble_uuid128_t sensor_ctrl_uuid = BLE_UUID128_INIT(0x2c, 0x5e, 0x45, 0x4d, 0x8e, 0x4e, 0x37, 0x93,
                                                                  0xa4, 0x48, 0x80, 0x6f, 0xa9, 0x1e, 0xfb, 0x45);
// GATT table
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        // Service: wifi info
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &wifi_info_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                // Characteristic: save ssid
                .uuid = &wifi_ssid_info_uuid.u,
                .access_cb = ble_wifi_ssid_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY, 
            },
            {
                // Characteristic: save password
                .uuid = &wifi_pw_uuid.u,
                .access_cb = ble_wifi_pw_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
            },
            {
                0, // No more characteristics
            },
        },
    },

    {
        // Service: monitoring sensors
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &sensor_monitor_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {                
                // Characteristic: sensors control
                .uuid = &sensor_ctrl_uuid.u,
                .access_cb = ble_sensor_ctrl_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY, 
            },
            {
                0, // No more characteristics
            },
        },
        
    },

    {
        0, // No more services
    },
};

int gatt_svc_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_gatts_count_cfg rc=%d", rc);
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_gatts_add_svcs rc=%d", rc);
        return rc;
    }

    return 0;
}
