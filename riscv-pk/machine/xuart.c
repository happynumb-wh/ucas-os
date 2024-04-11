// See LICENSE for license details.

#include <string.h>
#include "xuart.h"
#include "fdt.h"

#define XUARTPS_FIFO_OFFSET 0x0000000c
#define XUARTPS_SR_OFFSET 0x0000000b
#define SR_TX_FULL  0x10
#define SR_RX_EMPTY 0x02

#define BIT(nr)         (1 << (nr))

#define RISCV_FENCE(p, s) \
        __asm__ __volatile__ ("fence " #p "," #s : : : "memory")

/* These barriers need to enforce ordering on both devices or memory */
#define mb()            RISCV_FENCE(iorw,iorw)
#define rmb()           RISCV_FENCE(ir,ir)
#define wmb()           RISCV_FENCE(ow,ow)

#define dmb()           mb()
#define __iormb()       rmb()
#define __iowmb()       wmb()

#define __arch_getl(a)        (*(unsigned int *)(a))
#define __arch_putl(v, a)     (*(unsigned int *)(a) = (v))

static inline void writel(uint32_t val, volatile void *addr)
{
  __iowmb();
  __arch_putl(val, addr);
}

static inline uint32_t readl(const volatile void *addr)
{
  uint32_t val;

  val = __arch_getl(addr);
  __iormb();
  return val;
}

#define ZYNQ_UART_SR_TXACTIVE   BIT(11) /* TX active */
#define ZYNQ_UART_SR_TXFULL     BIT(4)  /* TX FIFO full */
#define ZYNQ_UART_SR_RXEMPTY    BIT(1)  /* RX FIFO empty */

#define ZYNQ_UART_CR_TORST     BIT(6) /* RX logic reset */
#define ZYNQ_UART_CR_TX_EN     BIT(4) /* TX enabled */
#define ZYNQ_UART_CR_RX_EN     BIT(2) /* RX enabled */
#define ZYNQ_UART_CR_TXRST     BIT(1) /* TX logic reset */
#define ZYNQ_UART_CR_RXRST     BIT(0) /* RX logic reset */

#define ZYNQ_UART_MR_PARITY_NONE  0x00000020    /* No parity mode */
#define XUARTPS_MR_CHMODE_R_LOOP       0x00000300U /**< Remote loopback mode */
#define XUARTPS_MR_CHMODE_L_LOOP       0x00000200U /**< Local loopback mode */
#define XUARTPS_MR_CHMODE_ECHO         0x00000100U /**< Auto echo mode */
#define XUARTPS_MR_CHMODE_NORM         0x00000000U /**< Normal mode */

struct uart_zynq {
       uint32_t control; /* 0x0 - Control Register [8:0] */
       uint32_t mode; /* 0x4 - Mode Register [10:0] */
       uint32_t reserved1[4];
       uint32_t baud_rate_gen; /* 0x18 - Baud Rate Generator [15:0] */
       uint32_t reserved2[4];
       uint32_t channel_sts; /* 0x2c - Channel Status [11:0] */
       uint32_t tx_rx_fifo; /* 0x30 - FIFO [15:0] or [7:0] */
       uint32_t baud_rate_divider; /* 0x34 - Baud Rate Divider [7:0] */
};

/* Initialize the UART, with...some settings. */
static void _uart_zynq_serial_init(struct uart_zynq *regs)
{
  /* RX/TX enabled & reset */
  writel(ZYNQ_UART_CR_TX_EN | ZYNQ_UART_CR_RX_EN | ZYNQ_UART_CR_TXRST |
        ZYNQ_UART_CR_RXRST| ZYNQ_UART_CR_TORST, &regs->control);
  writel(ZYNQ_UART_MR_PARITY_NONE, &regs->mode); /* 8 bit, no parity */
}

volatile uint32_t* xuart = NULL; //(void *)0x40010000;

int xuart_getchar()
{
  struct uart_zynq *regs = (struct uart_zynq*)xuart;

  if (readl(&regs->channel_sts) & ZYNQ_UART_SR_RXEMPTY)
    return -1;
  
  return readl(&regs->tx_rx_fifo);
}

void xuart_putchar(uint8_t ch)
{
  struct uart_zynq *regs = (struct uart_zynq*)xuart;
  while (readl(&regs->channel_sts) & ZYNQ_UART_SR_TXFULL);
  writel(ch, &regs->tx_rx_fifo);
}

/*
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
*/

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

  _uart_zynq_serial_init((struct uart_zynq *)xuart);
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
