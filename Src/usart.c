#include "usart.h"
#include "stm32f4xx.h"

/* USART2: TX=PA2 (AF7), 115200 baud, APB1=42 MHz */

void USART2_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2: AF7, no pull, push-pull, high speed */
    GPIOA->MODER   &= ~(3U << 4);
    GPIOA->MODER   |=  (2U << 4);
    GPIOA->OTYPER  &= ~(1U << 2);
    GPIOA->OSPEEDR |=  (3U << 4);
    GPIOA->PUPDR   &= ~(3U << 4);
    GPIOA->AFR[0]  &= ~(0xFU << 8);
    GPIOA->AFR[0]  |=  (7U   << 8);  /* AF7 */

    /* BRR = fPCLK / baud = 42 000 000 / 115200 ≈ 364.58
       Mantissa = 364, fraction = 0.58 * 16 ≈ 9  → 0x016D + 9 = 0x016D | 0x0009 */
    USART2->BRR = (364U << 4) | 9U;
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

void USART2_SendChar(char c)
{
    while (!(USART2->SR & USART_SR_TXE)) {}
    USART2->DR = (uint8_t)c;
}

void USART2_SendString(const char *s)
{
    while (*s) USART2_SendChar(*s++);
}

int __io_putchar(int ch)
{
    USART2_SendChar((char)ch);
    return ch;
}
