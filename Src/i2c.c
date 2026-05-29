#include "i2c.h"
#include "stm32f4xx.h"

/* Timeout in simple loop counts */
#define I2C_TIMEOUT  100000U

static uint8_t wait_flag(volatile uint32_t *reg, uint32_t flag, uint32_t set)
{
    uint32_t t = I2C_TIMEOUT;
    if (set) {
        while (!(*reg & flag)) { if (--t == 0) return 1; }
    } else {
        while (  *reg & flag)  { if (--t == 0) return 1; }
    }
    return 0;
}

void I2C1_Init(void)
{
    /* 1. Clocks: GPIOB + I2C1 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* 2. PB6 (SCL), PB7 (SDA): AF4, open-drain, no pull (external resistors) */
    GPIOB->MODER   &= ~((3U << 12) | (3U << 14));
    GPIOB->MODER   |=  ((2U << 12) | (2U << 14));  /* Alternate function */
    GPIOB->OTYPER  |=  (1U << 6) | (1U << 7);      /* Open-drain */
    GPIOB->OSPEEDR |=  (3U << 12) | (3U << 14);    /* High speed */
    GPIOB->PUPDR   &= ~((3U << 12) | (3U << 14));  /* No pull */
    GPIOB->AFR[0]  &= ~((0xFU << 24) | (0xFU << 28));
    GPIOB->AFR[0]  |=  ((4U  << 24) | (4U  << 28)); /* AF4 = I2C1 */

    /* 3. Reset & configure I2C1
       APB1 = 42 MHz (assuming HSE 8 MHz, PLL: M=8, N=336, P=2, Q=7)
       Standard mode 100 kHz:
         FREQ = 42
         CCR  = 42 MHz / (2 * 100 kHz) = 210
         TRISE = 42 + 1 = 43             */
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    I2C1->CR2   = 42U;           /* FREQ = 42 MHz */
    I2C1->CCR   = 210U;          /* Standard mode, Thigh = Tlow = CCR * Tpclk1 */
    I2C1->TRISE = 43U;
    I2C1->CR1  |= I2C_CR1_PE;   /* Enable peripheral */
}

/* Returns 0 on success, non-zero on error */
uint8_t I2C1_Write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint32_t len)
{
    /* Wait until bus is free */
    if (wait_flag((volatile uint32_t *)&I2C1->SR2, I2C_SR2_BUSY, 0)) return 1;

    /* START */
    I2C1->CR1 |= I2C_CR1_START;
    if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_SB, 1)) return 2;

    /* Address + WRITE */
    I2C1->DR = (uint8_t)(dev_addr << 1);
    if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_ADDR, 1)) return 3;
    (void)I2C1->SR2; /* Clear ADDR */

    /* Register address */
    if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_TXE, 1)) return 4;
    I2C1->DR = reg;

    /* Data bytes */
    for (uint32_t i = 0; i < len; i++) {
        if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_TXE, 1)) return 5;
        I2C1->DR = data[i];
    }

    /* Wait until BTF then STOP */
    if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_BTF, 1)) return 6;
    I2C1->CR1 |= I2C_CR1_STOP;
    return 0;
}

/* Returns 0 on success, non-zero on error */
uint8_t I2C1_Read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint32_t len)
{
    /* ---- Write phase: send register address ---- */
    if (wait_flag((volatile uint32_t *)&I2C1->SR2, I2C_SR2_BUSY, 0)) return 1;

    I2C1->CR1 |= I2C_CR1_START;
    if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_SB, 1)) return 2;

    I2C1->DR = (uint8_t)(dev_addr << 1);
    if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_ADDR, 1)) return 3;
    (void)I2C1->SR2;

    if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_TXE, 1)) return 4;
    I2C1->DR = reg;
    if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_BTF, 1)) return 5;

    /* ---- Read phase: repeated START ---- */
    I2C1->CR1 |= I2C_CR1_START;
    if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_SB, 1)) return 6;

    I2C1->CR1 |= I2C_CR1_ACK;  /* Enable ACK before sending address */
    I2C1->DR   = (uint8_t)((dev_addr << 1) | 1U);
    if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_ADDR, 1)) return 7;

    if (len == 1) {
        I2C1->CR1 &= ~I2C_CR1_ACK; /* NACK the only byte */
        (void)I2C1->SR2;            /* Clear ADDR */
        I2C1->CR1 |= I2C_CR1_STOP;
        if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_RXNE, 1)) return 8;
        data[0] = (uint8_t)I2C1->DR;
    } else {
        (void)I2C1->SR2; /* Clear ADDR */
        for (uint32_t i = 0; i < len; i++) {
            if (i == len - 1) {
                /* Before reading last byte: NACK + STOP */
                I2C1->CR1 &= ~I2C_CR1_ACK;
                I2C1->CR1 |=  I2C_CR1_STOP;
            }
            if (wait_flag((volatile uint32_t *)&I2C1->SR1, I2C_SR1_RXNE, 1)) return 9;
            data[i] = (uint8_t)I2C1->DR;
        }
    }

    /* Re-enable ACK for next transaction */
    I2C1->CR1 |= I2C_CR1_ACK;
    return 0;
}
