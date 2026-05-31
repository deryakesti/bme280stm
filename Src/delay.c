#include "delay.h"

/* Rough busy-wait. At 168 MHz core: ~42000 nop cycles ≈ 1 ms.
   Replace with SysTick for production accuracy.            */
void delay_ms(uint32_t ms)
{
    while (ms--) {
        volatile uint32_t c = 42000U;
        while (c--) { __asm("nop"); }
    }
}
