#include "bme280.h"
#include "i2c.h"
#include "delay.h"
#include <stdint.h>

/* ── Register addresses ────────────────────────────────────────────── */
#define REG_ID          0xD0
#define REG_RESET       0xE0
#define REG_CTRL_HUM    0xF2
#define REG_STATUS      0xF3
#define REG_CTRL_MEAS   0xF4
#define REG_CONFIG      0xF5
#define REG_PRESS_MSB   0xF7
#define REG_CALIB_00    0x88  /* temp + press trimming (T1..P9) */
#define REG_CALIB_26    0xE1  /* humidity trimming (H2..H6)     */
#define REG_DIG_H1      0xA1

#define BME280_CHIP_ID  0x60
#define BME280_RESET_VAL 0xB6

/* ── Compensation data ─────────────────────────────────────────────── */
static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t  dig_H1;
static int16_t  dig_H2;
static uint8_t  dig_H3;
static int16_t  dig_H4, dig_H5;
static int8_t   dig_H6;

/* Fine temperature used by pressure & humidity compensation */
static int32_t t_fine;

/* ── Helpers ───────────────────────────────────────────────────────── */
static uint8_t read_reg(uint8_t reg, uint8_t *val)
{
    return I2C1_Read(BME280_ADDR, reg, val, 1);
}

static uint8_t write_reg(uint8_t reg, uint8_t val)
{
    return I2C1_Write(BME280_ADDR, reg, &val, 1);
}

/* ── Trim register loading ─────────────────────────────────────────── */
static uint8_t load_calibration(void)
{
    uint8_t buf[26];

    /* 0x88..0x9F — T1..P9 (24 bytes) + 2 unused = 26 bytes total */
    if (I2C1_Read(BME280_ADDR, REG_CALIB_00, buf, 26)) return 1;

    dig_T1 = (uint16_t)(buf[1]  << 8 | buf[0]);
    dig_T2 = (int16_t) (buf[3]  << 8 | buf[2]);
    dig_T3 = (int16_t) (buf[5]  << 8 | buf[4]);
    dig_P1 = (uint16_t)(buf[7]  << 8 | buf[6]);
    dig_P2 = (int16_t) (buf[9]  << 8 | buf[8]);
    dig_P3 = (int16_t) (buf[11] << 8 | buf[10]);
    dig_P4 = (int16_t) (buf[13] << 8 | buf[12]);
    dig_P5 = (int16_t) (buf[15] << 8 | buf[14]);
    dig_P6 = (int16_t) (buf[17] << 8 | buf[16]);
    dig_P7 = (int16_t) (buf[19] << 8 | buf[18]);
    dig_P8 = (int16_t) (buf[21] << 8 | buf[20]);
    dig_P9 = (int16_t) (buf[23] << 8 | buf[22]);

    /* 0xA1 — H1 */
    if (read_reg(REG_DIG_H1, &dig_H1)) return 2;

    /* 0xE1..0xE7 — H2..H6 */
    uint8_t hbuf[7];
    if (I2C1_Read(BME280_ADDR, REG_CALIB_26, hbuf, 7)) return 3;

    dig_H2 = (int16_t)(hbuf[1] << 8 | hbuf[0]);
    dig_H3 = hbuf[2];
    dig_H4 = (int16_t)((int16_t)(hbuf[3] << 4) | (hbuf[4] & 0x0F));
    dig_H5 = (int16_t)((int16_t)(hbuf[5] << 4) | (hbuf[4] >> 4));
    dig_H6 = (int8_t)hbuf[6];

    return 0;
}

/* ── Compensation formulas (from Bosch datasheet) ─────────────────── */
static float compensate_temperature(int32_t adc_T)
{
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * (int32_t)dig_T2) >> 11;
    var2 = (((((adc_T >> 4) - (int32_t)dig_T1) *
              ((adc_T >> 4) - (int32_t)dig_T1)) >> 12) * (int32_t)dig_T3) >> 14;
    t_fine = var1 + var2;
    return (float)((t_fine * 5 + 128) >> 8) / 100.0f;
}

static float compensate_pressure(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * (int64_t)dig_P1 >> 33;
    if (var1 == 0) return 0.0f;
    p    = 1048576 - adc_P;
    p    = (((p << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)dig_P8 * p) >> 19;
    p    = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (float)(uint32_t)p / 25600.0f;  /* Pa→hPa */
}

static float compensate_humidity(int32_t adc_H)
{
    int32_t v;
    v = t_fine - 76800;
    v = (((((adc_H << 14) - ((int32_t)dig_H4 << 20) - ((int32_t)dig_H5 * v)) +
           16384) >> 15) *
         (((((((v * (int32_t)dig_H6) >> 10) *
              (((v * (int32_t)dig_H3) >> 11) + 32768)) >> 10) + 2097152) *
           (int32_t)dig_H2 + 8192) >> 14));
    v = v - (((((v >> 15) * (v >> 15)) >> 7) * (int32_t)dig_H1) >> 4);
    if (v < 0)        v = 0;
    if (v > 419430400) v = 419430400;
    return (float)(v >> 12) / 1024.0f;
}

/* ── Public API ────────────────────────────────────────────────────── */
uint8_t BME280_Init(void)
{
    uint8_t id;

    /* Soft reset */
    if (write_reg(REG_RESET, BME280_RESET_VAL)) return 1;
    delay_ms(10);

    /* Verify chip ID */
    if (read_reg(REG_ID, &id)) return 2;
    if (id != BME280_CHIP_ID) return 3;

    /* Load trimming */
    if (load_calibration()) return 4;

    /* Humidity oversampling x1 (must be written before ctrl_meas) */
    if (write_reg(REG_CTRL_HUM, 0x01)) return 5;

    /* t_os=x1, p_os=x1, forced mode trigger on each read */
    /* We use normal mode here: t_os=x2, p_os=x4, h_os=x1
       config: standby 500ms, filter off                         */
    if (write_reg(REG_CONFIG,    0xA0)) return 6;  /* t_sb=1000ms, filter=off */
    if (write_reg(REG_CTRL_MEAS, 0x57)) return 7;  /* osrs_t=x2, osrs_p=x16, normal mode */

    delay_ms(100);
    return 0;
}

uint8_t BME280_Read(BME280_Data *out)
{
    uint8_t buf[8];

    /* Burst-read press(3) + temp(3) + hum(2) from 0xF7 */
    if (I2C1_Read(BME280_ADDR, REG_PRESS_MSB, buf, 8)) return 1;

    int32_t adc_P = (int32_t)(((uint32_t)buf[0] << 12) |
                               ((uint32_t)buf[1] <<  4) |
                               ((uint32_t)buf[2] >>  4));
    int32_t adc_T = (int32_t)(((uint32_t)buf[3] << 12) |
                               ((uint32_t)buf[4] <<  4) |
                               ((uint32_t)buf[5] >>  4));
    int32_t adc_H = (int32_t)(((uint32_t)buf[6] << 8) | buf[7]);

    /* Temperature must be computed first — it fills t_fine */
    out->temperature = compensate_temperature(adc_T);
    out->pressure    = compensate_pressure(adc_P);
    out->humidity    = compensate_humidity(adc_H);

    return 0;
}
