#ifndef USART_H
#define USART_H

#include <stdint.h>

/* USART2: TX=PA2 (AF7), 115200 8N1 — for printf / debug output */

void USART2_Init(void);
void USART2_SendChar(char c);
void USART2_SendString(const char *s);

/* retargets printf → USART2 */
int __io_putchar(int ch);

#endif /* USART_H */
