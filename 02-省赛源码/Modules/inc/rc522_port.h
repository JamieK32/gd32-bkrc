#ifndef RC522_PORT_H__
#define RC522_PORT_H__

int rc522_i2c_wr_cb(void *user, uint8_t dev_addr, uint8_t reg, uint8_t value);
int rc522_i2c_rd_cb(void *user, uint8_t dev_addr, uint8_t reg, uint8_t *value);

#endif 
