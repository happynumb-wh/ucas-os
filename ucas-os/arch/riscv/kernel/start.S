/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

/* maybe you need another stack for booting? */

.section ".entry_function","ax"
ENTRY(_boot)

  bnez a0, loop

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop
  // Mask all interrupt
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  // Disable FPU in kernel space
  li t0, SR_FS
  csrc sstatus, t0

  // Find a place in the mem to be the stack of boot
  la sp, boot_stack

  mv a0, a0
  mv a1, a1
  mv a2, a2

  // this will never return
  call boot_kernel

loop:
  wfi
  j loop

END(_boot)
