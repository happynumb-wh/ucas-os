// See LICENSE for license details.

#ifndef _RISCV_XUART_H
#define _RISCV_XUART_H

#include <stdint.h>

extern volatile uint32_t* xuart;

void xuart_putchar(uint8_t ch);
int xuart_getchar();
void query_xuart(uintptr_t dtb);

#endif
