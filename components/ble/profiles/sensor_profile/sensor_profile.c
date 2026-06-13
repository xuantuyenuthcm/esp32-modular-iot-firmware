#include "sensor_profile.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "host/ble_hs.h"

static const char *TAG = "SENSOR_PROF";

// Sensor Service UUID: f3b890a2-f85f-4a00-b6f4-1ea15a9bc83d
static const ble_uuid128_t sensor_svc_uuid = BLE_UUID128_INIT(
    0x3d, 0xc8, 0x9b, 0x5a, 0xa1, 0x1e, 0xf4, 0xb6,
    0x00, 0x4a, 0x5f, 0xf8, 0xa2, 0x90, 0xb8, 0xf3);

// Sensor Data Characteristic UUID: 6a1b2c3d-4e5f-6a7b-8c9d-0e1f2a3b4c5d
static const ble_uuid128_t sensor_data_uuid = BLE_UUID128_INIT(
    0x5d, 0x4c, 0x3b, 0x2a, 0x1f, 0x0e, 0x9d, 0x8c,
    0x7b, 0x6a, 0x5f, 0x4e, 0x3d, 0x2c, 0x1b, 0x6a);

// Sensor Config Characteristic UUID: b1c2d3e4-f5a6-b7c8-d9e0-f1a2b3c4d5e6
static const ble_uuid128_t sensor_config_uuid = BLE_UUID128_INIT(
    0xe6, 0xd5, 0xc4, 0xb3, 0xa2, 0xf1, 0xe0, 0xd9,
    0xc8, 0xb7, 0xa6, 0xf5, 0xe4, 0xd3, 0xc2, 0xb1);

static uint16_t data_val_handle;
static uint16_t config_val_handle;

static sensor_profile_cb_t app_cb = NULL;

static int handle_sensor_data_read(uint16_t conn_handle, struct ble_gatt_access_ctxt *ctxt)
{
    if (app_cb)
    {
        uint8_t buf[128] = {0};
        sensor_profile_cb_param_t param = {
            .event = SENSOR_PROFILE_EVT_READ_DATA,
            .conn_handle = conn_handle,
            .data = buf,
            .len = 0,
            .max_len = sizeof(buf)};

        int rc = app_cb(&param);
        if (rc == 0 && param.len > 0)
        {
            return os_mbuf_append(ctxt->om, param.data, param.len);
        }
        return rc == 0 ? BLE_ATT_ERR_UNLIKELY : rc;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int handle_sensor_config_read(uint16_t conn_handle, struct ble_gatt_access_ctxt *ctxt)
{
    if (app_cb)
    {
        uint8_t buf[128] = {0};
        sensor_profile_cb_param_t param = {
            .event = SENSOR_PROFILE_EVT_READ_CONFIG,
            .conn_handle = conn_handle,
            .data = buf,
            .len = 0,
            .max_len = sizeof(buf)};

        int rc = app_cb(&param);
        if (rc == 0 && param.len > 0)
        {
            return os_mbuf_append(ctxt->om, param.data, param.len);
        }
        return rc == 0 ? BLE_ATT_ERR_UNLIKELY : rc;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int handle_sensor_config_write(uint16_t conn_handle, struct ble_gatt_access_ctxt *ctxt)
{
    if (app_cb)
    {
        uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
        if (om_len > 128)
        {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }

        uint8_t buf[128] = {0};
        int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &om_len);
        if (rc != 0)
        {
            return BLE_ATT_ERR_UNLIKELY;
        }

        sensor_profile_cb_param_t param = {
            .event = SENSOR_PROFILE_EVT_WRITE_CONFIG,
            .conn_handle = conn_handle,
            .data = buf,
            .len = om_len,
            .max_len = sizeof(buf)};

        return app_cb(&param);
    }
    return BLE_ATT_ERR_UNLIKELY;
}

// GATT access callback
static int sensor_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    ESP_LOGD(TAG, "Access CB: attr_handle=%d, op=%d", attr_handle, ctxt->op);

    if (attr_handle == data_val_handle)
    {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
        {
            return handle_sensor_data_read(conn_handle, ctxt);
        }
    }
    else if (attr_handle == config_val_handle)
    {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
        {
            return handle_sensor_config_read(conn_handle, ctxt);
        }
        else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
        {
            return handle_sensor_config_write(conn_handle, ctxt);
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}

// GATT service
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &sensor_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                // Sensor Data Characteristic
                .uuid = &sensor_data_uuid.u,
                .access_cb = sensor_access_cb,
                .val_handle = &data_val_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            {
                // Sensor Config Characteristic
                .uuid = &sensor_config_uuid.u,
                .access_cb = sensor_access_cb,
                .val_handle = &config_val_handle,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ,
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

esp_err_t sensor_profile_init(void)
{
    int rc;

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_gatts_count_cfg failed, rc=%d", rc);
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "ble_gatts_add_svcs failed, rc=%d", rc);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "sensor profile initialized");
    return ESP_OK;
}

esp_err_t sensor_profile_register_cb(sensor_profile_cb_t cb)
{
    app_cb = cb;
    return ESP_OK;
}

esp_err_t sensor_profile_notify_data(uint16_t conn_handle, const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate mbuf for notification");
        return ESP_ERR_NO_MEM;
    }

    int rc = ble_gatts_notify_custom(conn_handle, data_val_handle, om);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Failed to notify sensor data, rc=%d", rc);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Sensor data notified, len=%d", len);
    return ESP_OK;
}
