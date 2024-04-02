// See LICENSE for license details.

#include <string.h>
#include "xuart.h"
#include "fdt.h"

#define XUARTPS_FIFO_OFFSET 0x0000000c
#define XUARTPS_SR_OFFSET 0x0000000b
#define SR_TX_FULL  0x10
#define SR_RX_EMPTY 0x02

volatile uint32_t* xuart = NULL; //(void *)0x40010000;

void xuart_putchar(uint8_t ch) {
  volatile uint32_t *tx = xuart + XUARTPS_FIFO_OFFSET;
  while((*((volatile unsigned char*)(xuart + XUARTPS_SR_OFFSET)) & SR_TX_FULL) != 0);
  *tx = ch;
}

int xuart_getchar() {
  if (*((volatile unsigned char*)(xuart + XUARTPS_SR_OFFSET)) & SR_RX_EMPTY) {
    return -1;
  }
  int32_t ch = xuart[XUARTPS_FIFO_OFFSET];
  return ch;
}

struct xuart_scan {
  int compat;
  uint64_t reg;
};

static void xuart_open(const struct fdt_scan_node *node, void *extra) {
  struct xuart_scan *scan = (struct xuart_scan *)extra;
  memset(scan, 0, sizeof(*scan));
}

static void xuart_prop(const struct fdt_scan_prop *prop, void *extra) {
  struct xuart_scan *scan = (struct xuart_scan *)extra;
  if (!strcmp(prop->name, "compatible") && !strcmp((const char*)prop->value, "xlnx,xuartps")) {
    scan->compat = 1;
  } else if (!strcmp(prop->name, "reg")) {
    fdt_get_address(prop->node->parent, prop->value, &scan->reg);
  }
}

static void xuart_done(const struct fdt_scan_node *node, void *extra) {
  struct xuart_scan *scan = (struct xuart_scan *)extra;
  if (!scan->compat || !scan->reg || xuart) return;

  // Enable Rx/Tx channels
  xuart = (void*)(uintptr_t)scan->reg;
  //revise here
  //uart[UART_REG_TXCTRL] = UART_TXEN;
  //uart[UART_REG_RXCTRL] = UART_RXEN;
}

void query_xuart(uintptr_t fdt) {
  struct fdt_cb cb;
  struct xuart_scan scan;

  memset(&cb, 0, sizeof(cb));
  cb.open = xuart_open;
  cb.prop = xuart_prop;
  cb.done = xuart_done;
  cb.extra = &scan;

  fdt_scan(fdt, &cb);
}
