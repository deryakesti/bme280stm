#include "stm32f4xx.h"

/*
 * PLL konfigürasyonu:
 *   HSE = 8 MHz
 *   SYSCLK = 168 MHz (PLL: M=8, N=336, P=2)
 *   AHB    = 168 MHz
 *   APB1   = 42  MHz (AHB/4)
 *   APB2   = 84  MHz (AHB/2)
 */

void SystemInit(void)
{
    /* FPU erişim izni (Cortex-M4) */
    SCB->CPACR |= (3UL << 20) | (3UL << 22);

    /* HSI açık, PLL kapalı varsayılır (reset sonrası) */

    /* Flash: 5 wait-state @ 168 MHz, önbellek + prefetch etkin */
    FLASH->ACR = FLASH_ACR_LATENCY_5WS |
                 FLASH_ACR_ICEN        |
                 FLASH_ACR_DCEN        |
                 FLASH_ACR_PRFTEN;

    /* HSE aç */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) {}

    /* PLL: kaynak=HSE, M=8, N=336, P=2, Q=7 */
    RCC->PLLCFGR = (8U  <<  0)  |   /* PLLM  */
                   (336U <<  6)  |   /* PLLN  */
                   (0U  << 16)  |   /* PLLP = /2 */
                   RCC_PLLCFGR_PLLSRC_HSE |
                   (7U  << 24);     /* PLLQ  */

    /* AHB=/1, APB1=/4, APB2=/2 */
    RCC->CFGR = RCC_CFGR_HPRE_DIV1  |
                RCC_CFGR_PPRE1_DIV4 |
                RCC_CFGR_PPRE2_DIV2;

    /* PLL aç, bekle */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}

    /* SYSCLK kaynağı = PLL */
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    SystemCoreClock = 168000000UL;
}
