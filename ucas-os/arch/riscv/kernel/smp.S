#include <asm.h>
#include <csr.h>


ENTRY(get_current_running)
  mv a0, tp
  jr ra
ENDPROC(get_current_running)
