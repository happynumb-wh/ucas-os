#ifndef _RISCV_UART_LITE_H
#define _RISCV_UART_LITE_H

#include <stdint.h>

extern volatile uint8_t* uartlite;

void uartlite_putchar(uint8_t ch);
int uartlite_getchar();
void query_uartlite(uintptr_t dtb);

#endif
