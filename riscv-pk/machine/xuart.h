// See LICENSE for license details.

#ifndef _RISCV_XUART_H
#define _RISCV_XUART_H

#include <stdint.h>

extern volatile uint32_t* xuart;

#define XUARTPS_FIFO_OFFSET 0x0000000c
#define UART0_BASE_ADDR 0xe0000000
#define XUARTPS_SR_OFFSET 0x0000000b

#define CDNS_UART_SR_RXEMPTY   0x00000002 /* RX FIFO empty */
#define CDNS_UART_SR_TXEMPTY   0x00000008 /* TX FIFO empty */
#define CDNS_UART_SR_TXFULL    0x00000010 /* TX FIFO full */
#define CDNS_UART_SR_RXTRIG    0x00000001 /* Rx Trigger */
#define CDNS_UART_SR_TACTIVE   0x00000800 /* TX state machine active */

void xuart_putchar(uint8_t ch);
int xuart_getchar();
void query_xuart(uintptr_t dtb);

#endif
