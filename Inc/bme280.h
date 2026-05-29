#ifndef BME280_H
#define BME280_H

#include <stdint.h>

/* SDO = GND  ->  0x76
   SDO = VCC  ->  0x77  */
#define BME280_ADDR   0x76

typedef struct {
    float temperature;   /* Celsius */
    float pressure;      /* hPa     */
    float humidity;      /* %RH     */
} BME280_Data;

uint8_t BME280_Init(void);
uint8_t BME280_Read(BME280_Data *out);

#endif /* BME280_H */
