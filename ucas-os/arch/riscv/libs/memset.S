#include <asm.h>

/* void *memset(void *, int, size_t) */
ENTRY(memset)
	move t0, a0  /* Preserve return value */

	/* Defer to byte-oriented fill for small sizes */
	sltiu a3, a2, 16
	bnez a3, 4f

	/*
	 * Round to nearest XLEN-aligned address
	 * greater than or equal to start address
	 */
	addi a3, t0, RISCV_SZPTR-1
	andi a3, a3, ~(RISCV_SZPTR-1)
	beq a3, t0, 2f  /* Skip if already aligned */
	/* Handle initial misalignment */
	sub a4, a3, t0
1:
	sb a1, 0(t0)
	addi t0, t0, 1
	bltu t0, a3, 1b
	sub a2, a2, a4  /* Update count */

2: /* Duff's device with 32 XLEN stores per iteration */
	/* Broadcast value into all bytes */
	andi a1, a1, 0xff
	slli a3, a1, 8
	or a1, a3, a1
	slli a3, a1, 16
	or a1, a3, a1
	slli a3, a1, 32
	or a1, a3, a1

	/* Calculate end address */
	andi a4, a2, ~(RISCV_SZPTR-1)
	add a3, t0, a4

	andi a4, a4, 31*RISCV_SZPTR  /* Calculate remainder */
	beqz a4, 3f            /* Shortcut if no remainder */
	neg a4, a4
	addi a4, a4, 32*RISCV_SZPTR  /* Calculate initial offset */

	/* Adjust start address with offset */
	sub t0, t0, a4

	/* Jump into loop body */
	/* Assumes 32-bit instruction lengths */
	la a5, 3f
	srli a4, a4, 1
	add a5, a5, a4
	jr a5
3:
	sd a1,        0(t0)
	sd a1,    RISCV_SZPTR(t0)
	sd a1,  2*RISCV_SZPTR(t0)
	sd a1,  3*RISCV_SZPTR(t0)
	sd a1,  4*RISCV_SZPTR(t0)
	sd a1,  5*RISCV_SZPTR(t0)
	sd a1,  6*RISCV_SZPTR(t0)
	sd a1,  7*RISCV_SZPTR(t0)
	sd a1,  8*RISCV_SZPTR(t0)
	sd a1,  9*RISCV_SZPTR(t0)
	sd a1, 10*RISCV_SZPTR(t0)
	sd a1, 11*RISCV_SZPTR(t0)
	sd a1, 12*RISCV_SZPTR(t0)
	sd a1, 13*RISCV_SZPTR(t0)
	sd a1, 14*RISCV_SZPTR(t0)
	sd a1, 15*RISCV_SZPTR(t0)
	sd a1, 16*RISCV_SZPTR(t0)
	sd a1, 17*RISCV_SZPTR(t0)
	sd a1, 18*RISCV_SZPTR(t0)
	sd a1, 19*RISCV_SZPTR(t0)
	sd a1, 20*RISCV_SZPTR(t0)
	sd a1, 21*RISCV_SZPTR(t0)
	sd a1, 22*RISCV_SZPTR(t0)
	sd a1, 23*RISCV_SZPTR(t0)
	sd a1, 24*RISCV_SZPTR(t0)
	sd a1, 25*RISCV_SZPTR(t0)
	sd a1, 26*RISCV_SZPTR(t0)
	sd a1, 27*RISCV_SZPTR(t0)
	sd a1, 28*RISCV_SZPTR(t0)
	sd a1, 29*RISCV_SZPTR(t0)
	sd a1, 30*RISCV_SZPTR(t0)
	sd a1, 31*RISCV_SZPTR(t0)
	addi t0, t0, 32*RISCV_SZPTR
	bltu t0, a3, 3b
	andi a2, a2, RISCV_SZPTR-1  /* Update count */

4:
	/* Handle trailing misalignment */
	beqz a2, 6f
	add a3, t0, a2
5:
	sb a1, 0(t0)
	addi t0, t0, 1
	bltu t0, a3, 5b
6:
	ret
END(memset)