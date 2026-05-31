#include <stdio.h>
#include "stm32f4xx.h"
#include "i2c.h"
#include "bme280.h"
#include "usart.h"
#include "delay.h"

int main(void)
{
    USART2_Init();
    I2C1_Init();

    printf("\r\n=== BME280 CMSIS Driver (STM32F407VGT6) ===\r\n");

    if (BME280_Init() != 0) {
        printf("BME280 baslatma hatasi!\r\n");
        while (1) {}
    }

    printf("BME280 baslatildi.\r\n\r\n");

    BME280_Data data;

    while (1) {
        if (BME280_Read(&data) == 0) {
            /* Floating-point printf: -u _printf_float gerektirir (linker flag) */
            printf("Sicaklik : %.2f C\r\n",  (double)data.temperature);
            printf("Basinc   : %.2f hPa\r\n", (double)data.pressure);
            printf("Nem      : %.2f %%\r\n",  (double)data.humidity);
            printf("---\r\n");
        } else {
            printf("Okuma hatasi!\r\n");
        }

        delay_ms(2000);
    }
}
