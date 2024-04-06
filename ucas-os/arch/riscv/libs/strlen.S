#include <asm.h>


/* int strlen(const char *s) */
ENTRY(strlen)

	/*
	 * Returns
	 *   a0 - string length
	 *
	 * Parameters
	 *   a0 - String to measure
	 *
	 * Clobbers:
	 *   t0, t1
	 */
	mv	t1, a0
1:
	lbu	t0, 0(t1)
	beqz	t0, 2f
	addi	t1, t1, 1
	j	1b
2:
	sub	a0, t1, a0
	ret

END(strlen)


