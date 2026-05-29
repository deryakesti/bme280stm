#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/* I2C1: SCL=PB6, SDA=PB7 (AF4) */

void     I2C1_Init(void);
uint8_t  I2C1_Write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint32_t len);
uint8_t  I2C1_Read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint32_t len);

#endif /* I2C_H */
