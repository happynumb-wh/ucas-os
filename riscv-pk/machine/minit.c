// See LICENSE for license details.

#include "mtrap.h"
#include "atomic.h"
#include "vm.h"
#include "fp_emulation.h"
#include "fdt.h"
#include "uart.h"
#include "xuart.h"
#include "uartlite.h"
#include "uart16550.h"
#include "finisher.h"
#include "disabled_hart_mask.h"
#include "htif.h"
#include <string.h>
#include <limits.h>

//#define RISCV_FLASH_START
#define QSPI_BASE_ADDR    (0x31000000)
#define REG32(addr)       (*(volatile uint32_t *)(uint32_t)(addr))
pte_t* root_page_table;
uintptr_t mem_size;
volatile uint64_t* mtime;
volatile uint32_t* plic_priorities;
size_t plic_ndevs;
void* kernel_start;
void* kernel_end;


#ifdef RISCV_FLASH_START
/* trigger flash to exit xip mode*/
static void qspi_trigger_flash()
{
    uint32_t status = 0x1;
    REG32(QSPI_BASE_ADDR + 0x90) = 0xb5b00001; // read novolatile register
    while(status){
        status =  REG32(QSPI_BASE_ADDR + 0x90);
        status &= 0x1;
    }
    REG32(QSPI_BASE_ADDR + 0x90) = 0xb5b00001;
    while(status){
        status =  REG32(QSPI_BASE_ADDR + 0x90);
        status &= 0x1;
    }
}
/*
 *enable flash address writalbe permission
 *exit flash and qspi xip mode
 */
static void qspi_init()
{
    /* enable flash address writable permission */
    write_csr(0x7c0, 0x80b080f08000000UL);
    REG32(QSPI_BASE_ADDR) = 0x80180081;// disable xip mode
    REG32(QSPI_BASE_ADDR + 0x4) = 0x0; // read config register to 0x0
    qspi_trigger_flash();
    REG32(QSPI_BASE_ADDR + 0x4) = 0x0a0222ec; // for read config
    qspi_trigger_flash();
}
#endif

static void mstatus_init()
{
  // Enable FPU
  //if (supports_extension('D') || supports_extension('F'))
  //  write_csr(mstatus, MSTATUS_FS);

  // Enable user/supervisor use of perf counters
  if (supports_extension('S'))
    write_csr(scounteren, -1);
  if (supports_extension('U'))
    write_csr(mcounteren, -1);

  // Enable software interrupts && external interrupts
  write_csr(mie, MIP_MSIP | MIP_MEIP);

  // Disable paging
  if (supports_extension('S'))
    write_csr(sptbr, 0);

#ifdef RISCV_FLASH_START
    qspi_init();
#endif
}

// send S-mode interrupts and most exceptions straight to S-mode
static void delegate_traps()
{
  if (!supports_extension('S'))
    return;

  uintptr_t interrupts = MIP_SSIP | MIP_STIP | MIP_SEIP;
  uintptr_t exceptions =
    (1U << CAUSE_MISALIGNED_FETCH) |
    (1U << CAUSE_FETCH_PAGE_FAULT) |
    (1U << CAUSE_BREAKPOINT) |
    (1U << CAUSE_LOAD_PAGE_FAULT) |
    (1U << CAUSE_STORE_PAGE_FAULT) |
    (1U << CAUSE_USER_ECALL) |
     /* dasics exceptions */
    (1U << FDIUJumpFault) |
    (1U << FDIULoadAccessFault) |
    (1U << FDIUStoreAccessFault);

  write_csr(mideleg, interrupts);
  write_csr(medeleg, exceptions);
  assert(read_csr(mideleg) == interrupts);
  assert(read_csr(medeleg) == exceptions);



}

static void dump_misa(uint32_t misa) {
  int i;
  for (i = 0; i < 26; i ++) {
    if (misa & 0x1) printm("%c ", 'A' + i);
    misa >>= 1;
  }
  printm("\n");
}

static void fp_init()
{
  if (!supports_extension('D') && !supports_extension('F'))
    return;

  assert(read_csr(mstatus) & MSTATUS_FS);

#ifdef __riscv_flen
  for (int i = 0; i < 32; i++)
    init_fp_reg(i);
  write_csr(fcsr, 0);
#else
  uintptr_t fd_mask = (1 << ('F' - 'A')) | (1 << ('D' - 'A'));
  clear_csr(misa, fd_mask);
  dump_misa(read_csr(misa));
  //assert(!(read_csr(misa) & fd_mask));
#endif
}

hls_t* hls_init(uintptr_t id)
{
  hls_t* hls = OTHER_HLS(id);
  memset(hls, 0, sizeof(*hls));
  return hls;
}

static void memory_init()
{
  mem_size = mem_size / MEGAPAGE_SIZE * MEGAPAGE_SIZE;
}

static void hart_init()
{
  mstatus_init();
  //fp_init();
#ifndef BBL_BOOT_MACHINE
  delegate_traps();
#endif /* BBL_BOOT_MACHINE */
  setup_pmp();
}

static void plic_init()
{
  for (size_t i = 1; i <= plic_ndevs; i++)
    plic_priorities[i] = 1;
}

static void prci_test()
{
  assert(!(read_csr(mip) & MIP_MSIP));
  *HLS()->ipi = 1;
  assert(read_csr(mip) & MIP_MSIP);
  *HLS()->ipi = 0;

  assert(!(read_csr(mip) & MIP_MTIP));
  *HLS()->timecmp = 0;
  assert(read_csr(mip) & MIP_MTIP);
  *HLS()->timecmp = -1ULL;
}

static void hart_plic_init()
{
  // clear pending interrupts
  *HLS()->ipi = 0;
  *HLS()->timecmp = -1ULL;
  write_csr(mip, 0);

  if (!plic_ndevs)
    return;

  size_t ie_words = (plic_ndevs + 8 * sizeof(uintptr_t) - 1) /
		(8 * sizeof(uintptr_t));
  for (size_t i = 0; i < ie_words; i++) {
     if (HLS()->plic_s_ie) {
        // Supervisor not always present
        HLS()->plic_s_ie[i] = ULONG_MAX;
     }
  }
  *HLS()->plic_m_thresh = 0;
  if (HLS()->plic_s_thresh) {
      // Supervisor not always present
      *HLS()->plic_s_thresh = 0;
  }
}

static void wake_harts()
{
  printm("[DEBUG] harts num: %d hart mask %x disabled hart mask %x\n", MAX_HARTS, hart_mask, disabled_hart_mask);
  hart_proceed = 1;
  for (int hart = 0; hart < MAX_HARTS; ++hart)
    if ((((~disabled_hart_mask & hart_mask) >> hart) & 1))
      *OTHER_HLS(hart)->ipi = 1; // wakeup the hart
}

void init_first_hart(uintptr_t hartid, uintptr_t dtb)
{
#ifndef __QEMU__
  extern char dtb_start;
  dtb = (uintptr_t)&dtb_start;
#endif

  // Confirm console as early as possible
  hart_proceed = 0;
#ifndef S2C
  query_uart(dtb);
  query_xuart(dtb);
  query_uartlite(dtb);
  query_uart16550(dtb);
  query_htif(dtb);
#endif
  printm("bbl loader\r\n");

  hart_init();
  hls_init(0); // this might get called again from parse_config_string

  // Find the power button early as well so die() works
  query_finisher(dtb);

  query_mem(dtb);
  query_harts(dtb);
  query_clint(dtb);
  query_plic(dtb);
  query_chosen(dtb);

  wake_harts();
  printm("[DEBUG] wake harts done\n");

  plic_init();
  hart_plic_init();
  //prci_test();
  memory_init();
  boot_loader(dtb);
}

void init_other_hart(uintptr_t hartid, uintptr_t dtb)
{
  printm("[DEBUG] init other hart\n");
  hart_init();
  hart_plic_init();
  boot_other_hart(dtb);
}

void setup_pmp(void)
{
  // Set up a PMP to permit access to all of memory.
  // Ignore the illegal-instruction trap if PMPs aren't supported.
  uintptr_t pmpc = PMP_NAPOT | PMP_R | PMP_W | PMP_X;
  asm volatile ("la t0, 1f\n\t"
                "csrrw t0, mtvec, t0\n\t"
                "csrw pmpaddr0, %1\n\t"
                "csrw pmpcfg0, %0\n\t"
                ".align 2\n\t"
                "1: csrw mtvec, t0"
                : : "r" (pmpc), "r" (-1UL) : "t0");
}

void enter_supervisor_mode(void (*fn)(uintptr_t), uintptr_t arg0, uintptr_t arg1)
{
  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_S);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 0);
  write_csr(mstatus, mstatus);
  write_csr(mscratch, MACHINE_STACK_TOP() - MENTRY_FRAME_SIZE);
#ifndef __riscv_flen
  uintptr_t *p_fcsr = MACHINE_STACK_TOP() - MENTRY_FRAME_SIZE; // the x0's save slot
  *p_fcsr = 0;
#endif
  write_csr(mepc, fn);

  register uintptr_t a0 asm ("a0") = arg0;
  register uintptr_t a1 asm ("a1") = arg1;
  asm volatile ("mret" : : "r" (a0), "r" (a1));
  __builtin_unreachable();
}

void enter_machine_mode(void (*fn)(uintptr_t, uintptr_t), uintptr_t arg0, uintptr_t arg1)
{
  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 0);
  write_csr(mstatus, mstatus);
  write_csr(mscratch, MACHINE_STACK_TOP() - MENTRY_FRAME_SIZE);

  /* Jump to the payload's entry point */
  fn(arg0, arg1);

  __builtin_unreachable();
}
