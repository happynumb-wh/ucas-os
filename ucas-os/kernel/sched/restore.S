#include <asm.h>
#include <csr.h>

ENTRY(__restore)
    li a7, 139
    ecall
END(__restore)