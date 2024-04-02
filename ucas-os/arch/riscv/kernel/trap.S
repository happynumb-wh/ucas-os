#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)
  /* save exception_handler_entry into STVEC
   * enable global exceptions */
  la t0, exception_handler_entry
  csrw CSR_STVEC, t0

  li t0, SR_SIE
  csrs CSR_SSTATUS, t0
  jr ra
ENDPROC(setup_exception)
