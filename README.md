# BME280 STM32F407VGT6 — CMSIS Only (HAL yok)

Bosch BME280 sıcaklık/basınç/nem sensöründen I2C ile veri okuyan, HAL katmanı
kullanmayan saf CMSIS C uygulaması.

---

## Donanım Bağlantısı

| BME280 | STM32F407VGT6 |
|--------|---------------|
| VCC    | 3.3 V         |
| GND    | GND           |
| SCL    | PB6 (I2C1)    |
| SDA    | PB7 (I2C1)    |
| SDO    | GND → adres **0x76** |
| CSB    | 3.3 V (I2C modu) |

SCL ve SDA hatlarına 4.7 kΩ pull-up direnci (3.3 V - pin arası) ekleyin.

USART2 TX → PA2 (115200 8N1) üzerinden debug çıkışı alınır.

---

## Proje Yapısı

```
bme280stm/
├── Inc/
│   ├── i2c.h
│   ├── bme280.h
│   └── usart.h
├── Src/
│   ├── main.c
│   ├── i2c.c
│   ├── bme280.c
│   ├── usart.c
│   └── system_stm32f4xx.c
├── Startup/
│   └── startup_stm32f407vgtx.s
├── LD/
│   └── stm32f407vgtx.ld
├── CMSIS/            ← STM32CubeF4 başlıklarını buraya kopyalayın
└── Makefile
```

---

## Saat Ayarları

| Alan   | Değer   |
|--------|---------|
| HSE    | 8 MHz   |
| SYSCLK | 168 MHz |
| APB1   | 42 MHz  |
| APB2   | 84 MHz  |

---

## CMSIS Başlıklarını Temin Etme

`CMSIS/` klasörüne STM32CubeF4 paketinden şu dosyaları kopyalayın:

```
Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f407xx.h
Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h   (üzerine yazın)
Drivers/CMSIS/Include/core_cm4.h
Drivers/CMSIS/Include/cmsis_compiler.h
Drivers/CMSIS/Include/cmsis_gcc.h
```

---

## Derleme

```bash
make          # elf / hex / bin üretir
make flash    # st-flash ile karta yükler
make clean
```

---

## Çıktı Örneği (115200 baud)

```
=== BME280 CMSIS Driver (STM32F407VGT6) ===
BME280 baslatildi.

Sicaklik : 23.45 C
Basinc   : 1013.25 hPa
Nem      : 48.60 %
---
```
