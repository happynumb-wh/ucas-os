#ifndef UNAME_H
#define UNAME_H
#include <type.h>

typedef struct utsname {
    uint8_t sysname[65];
    uint8_t nodename[65];
    uint8_t release[65];
    uint8_t version[65];
    uint8_t machine[65];
    uint8_t domainname[65];
}utsname_t;

extern uint8 do_uname(struct utsname *uts);

#endif