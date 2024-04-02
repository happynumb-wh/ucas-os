/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

ENTRY(_start)
  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  fence
  fence.i
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  bnez a0, secondary

  /* Clear BSS for flat non-ELF images */
  la a3, __bss_start
  la a4, __BSS_END__
  ble a4, a3, clear_bss_done
clear_bss:
  sd zero, (a3)
  addi a3, a3, RISCV_SZPTR
  blt a3, a4, clear_bss
clear_bss_done:

  /* setup C environment */
  la tp, pid0_pcb_master
  ld sp, pid0_stack_master
  csrw CSR_SSCRATCH, x0
  call main

secondary:
  la tp, pid0_pcb_slave
  ld sp, pid0_stack_slave
  csrw CSR_SSCRATCH, x0


  call main

loop:
  wfi
  j loop

END(_start)

// .section .data
// .align 3
// .global _filesystem_start, _filesystem_end
// 
// _filesystem_start:
// .incbin "../../../img/rootfs.img"
// _filesystem_end:

