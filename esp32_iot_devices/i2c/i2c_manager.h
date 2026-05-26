#include "driver/i2c_master.h"

#define I2C_MASTER_SCL_IO   18
#define I2C_MASTER_SDA_IO   19
#define I2C_MASTER_NUM      I2C_NUM_1  

extern const char *TAG_I2C;

void i2c_init();
void i2c_add_device(uint8_t dev_addr, i2c_master_dev_handle_t *dev_handle);