#ifndef UNAME_H
#define UNAME_H
#include <type.h>

typedef struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
}utsname_t;

extern uint8 do_uname(struct utsname *uts);

#endif