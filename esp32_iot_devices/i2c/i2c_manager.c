#include "i2c_manager.h"

const char *TAG_I2C = "I2C";


static i2c_master_bus_handle_t bus_handle = NULL;

void i2c_init() {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));
}

i2c_device_config_t i2c_get_dev_config(uint8_t dev_addr) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = dev_addr,
        .scl_speed_hz = 100000,                                      // 100k Hhz
    };

    return dev_cfg;
}

void i2c_add_device(uint8_t dev_addr, i2c_master_dev_handle_t *dev_handle) {
    i2c_device_config_t dev_cfg = i2c_get_dev_config(dev_addr);
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, dev_handle));
}
