#ifndef __MEMLAYOUT_H
#define __MEMLAYOUT_H
#include <pgtable.h>
// Physical memory layout

// k210 peripherals
// (0x0200_0000, 0x1000),      /* CLINT     */
// // we only need claim/complete for target0 after initializing
// (0x0C20_0000, 0x1000),      /* PLIC      */
// (0x3800_0000, 0x1000),      /* UARTHS    */
// (0x3800_1000, 0x1000),      /* GPIOHS    */
// (0x5020_0000, 0x1000),      /* GPIO      */
// (0x5024_0000, 0x1000),      /* SPI_SLAVE */
// (0x502B_0000, 0x1000),      /* FPIOA     */
// (0x502D_0000, 0x1000),      /* TIMER0    */
// (0x502E_0000, 0x1000),      /* TIMER1    */
// (0x502F_0000, 0x1000),      /* TIMER2    */
// (0x5044_0000, 0x1000),      /* SYSCTL    */
// (0x5200_0000, 0x1000),      /* SPI0      */
// (0x5300_0000, 0x1000),      /* SPI1      */
// (0x5400_0000, 0x1000),      /* SPI2      */
// (0x8000_0000, 0x600000),    /* Memory    */

// qemu -machine virt is set up like this,
// based on qemu's hw/riscv/virt.c:
//
// 00001000 -- boot ROM, provided by qemu
// 02000000 -- CLINT
// 0C000000 -- PLIC
// 10000000 -- uart0 
// 10001000 -- virtio disk 
// 80000000 -- boot ROM jumps here in machine mode
//             -kernel loads the kernel here
// unused RAM after 80000000.


// qemu puts UART registers here in physical memory.

#define UART0 0xffffffc010000000L
#define UART_V                  (UART0)
#define UART_IRQ 10
 
#define DISK_IRQ    1

#define UARTHS 0xffffffc038000000L
#define UARTHS_IRQ 33



// virtio mmio interface
#define VIRTIO0 0xffffffc010001000
#define VIRTIO0_V               (VIRTIO0)
#define VIRTIO0_IRQ 1


// local interrupt controller, which contains the timer.
#define CLINT 0xffffffc002000000L
#define CLINT_V                 (CLINT)
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.

#ifdef k210
    #define PLIC 0xffffffc00c200000L
#else
    #define PLIC 0xffffffc00c000000L
#endif
#define PLIC_V                  (PLIC)
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)

#ifdef k210
    #define GPIOHS 0xffffffc038001000

    #define GPIO 0xffffffc050200000

    #define SPI_SLAVE 0xffffffc050240000

    #define FPIOA 0xffffffc0502B0000

    #define SPI0 0xffffffc052000000

    #define SPI1 0xffffffc053000000

    #define SPI2 0xffffffc054000000

    #define FPIOA 0xffffffc0502B0000U

    #define FPIOA_BASE_ADDR  (0xffffffc0502B0000U)
#endif



// the physical address of rustsbi
#define RUSTSBI_BASE 0xffffffc080000000

// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80200000 to PHYSTOP.
#define KERNBASE 0xffffffc080020000
// #define PHYSTOP (KERNBASE + 128*1024*1024)
#define PHYSTOP KERNEL_END

// map the trampoline page to the highest address,
// in both user and kernel space.
#define TRAMPOLINE (MAXVA - PGSIZE)

// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
#define KSTACK(p) (TRAMPOLINE - ((p)+1)* 2*PGSIZE)

// User memory layout.
// Address zero first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
//   ...
//   TRAPFRAME (p->trapframe, used by the trampoline)
//   TRAMPOLINE (the same page as in the kernel)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)

#endif