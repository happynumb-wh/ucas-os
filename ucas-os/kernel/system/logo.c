#include <logo.h>
#include <stdio.h>

void print_logo(){
#ifdef PRINT_LOG
    extern char _logo_start[];

    printk(_logo_start);
#endif
}