#include "uartlite.h"
#include <string.h>
#include "fdt.h"

volatile uint8_t* uartlite;

#define UART_LITE_RX_FIFO    0x0
#define UART_LITE_TX_FIFO    0x4
#define UART_LITE_STAT_REG   0x8
#define UART_LITE_CTRL_REG   0xc

#define UART_LITE_RST_FIFO   0x03
#define UART_LITE_INTR_EN    0x10
#define UART_LITE_TX_FULL    0x08
#define UART_LITE_TX_EMPTY   0x04
#define UART_LITE_RX_FULL    0x02
#define UART_LITE_RX_VALID   0x01

void uartlite_putchar(uint8_t ch) {
  if (ch == '\n') uartlite_putchar('\r');

  while (uartlite[UART_LITE_STAT_REG] & UART_LITE_TX_FULL);
  uartlite[UART_LITE_TX_FIFO] = ch;
}

int uartlite_getchar() {
  if (uartlite[UART_LITE_STAT_REG] & UART_LITE_RX_VALID)
    return (int8_t)uartlite[UART_LITE_RX_FIFO];
  return -1;
}

struct uart_scan
{
  int compat;
  uint64_t reg;
};

static void uart_open(const struct fdt_scan_node *node, void *extra)
{
  struct uart_scan *scan = (struct uart_scan *)extra;
  memset(scan, 0, sizeof(*scan));
}

static void uart_prop(const struct fdt_scan_prop *prop, void *extra)
{
  struct uart_scan *scan = (struct uart_scan *)extra;
  if (!strcmp(prop->name, "compatible") && !strcmp((const char*)prop->value, "xilinx,uartlite")) {
    scan->compat = 1;
  } else if (!strcmp(prop->name, "reg")) {
    fdt_get_address(prop->node->parent, prop->value, &scan->reg);
  }
}

static void uart_done(const struct fdt_scan_node *node, void *extra)
{
  struct uart_scan *scan = (struct uart_scan *)extra;
  if (!scan->compat || !scan->reg || uartlite) return;

  uartlite = (void*)(uintptr_t)scan->reg;
  uartlite[UART_LITE_CTRL_REG] = UART_LITE_RST_FIFO;
}

void query_uartlite(uintptr_t fdt)
{
  struct fdt_cb cb;
  struct uart_scan scan;

  memset(&cb, 0, sizeof(cb));
  cb.open = uart_open;
  cb.prop = uart_prop;
  cb.done = uart_done;
  cb.extra = &scan;

  fdt_scan(fdt, &cb);
}
