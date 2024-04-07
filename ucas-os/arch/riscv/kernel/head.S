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

.section .data
.align 8
.global _filesystem_start, _filesystem_end

_filesystem_start:
.incbin "../../../img/rootfs.img"
_filesystem_end:

#ifdef PRINT_LOG

.section .data
.align 8
.global _logo_start, _log_end

_logo_start:
.incbin "../../../kernel/logo.txt"
_logo_end:

#endif