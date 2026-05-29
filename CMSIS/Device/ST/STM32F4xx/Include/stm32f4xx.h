/* Minimal stm32f4xx.h — gerçek projede STM32CubeF4 paketindekiyle değiştirin */
#ifndef STM32F4XX_H
#define STM32F4XX_H

#if defined(STM32F407xx)
  #include "stm32f407xx.h"
#else
  #error "Hedef cihaz tanımlanmamış. -DSTM32F407xx ekleyin."
#endif

#include "system_stm32f4xx.h"

#endif /* STM32F4XX_H */
