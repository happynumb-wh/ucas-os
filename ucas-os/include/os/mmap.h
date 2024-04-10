#ifndef __INCLUDE_OS_MMAP_H
#define __INCLUDE_OS_MMAP_H

#include <pgtable.h>
#include <type.h>

// Map flag
#define MAP_SHARED 0x1
#define MAP_PRIVATE 0x2
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20




#define PROT_READ        0x1                /* Page can be read.  */
#define PROT_WRITE        0x2                /* Page can be written.  */
#define PROT_EXEC        0x4                /* Page can be executed.  */
#define PROT_NONE        0x0                /* Page can not be accessed.  */
#define PROT_GROWSDOWN        0x01000000        /* Extend change to start of
                                           growsdown vma (mprotect only).  */
#define PROT_GROWSUP        0x02000000        /* Extend change to start of
                                           growsup vma (mprotect only).  */


static inline uint64_t __prot_to_page_flag(uint64_t prot)
{
    uint64_t flag = 0;

    if (prot & PROT_EXEC)
        flag |= _PAGE_EXEC;
    if (prot & PROT_READ)
        flag |= _PAGE_READ;
    if (prot & PROT_WRITE)
        flag |= _PAGE_WRITE;

    return flag;
}




#endif