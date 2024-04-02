#include <os/uname.h>
#include <os/string.h>
#include <stdio.h>

static utsname_t utsname = {
    .sysname = "Linux",
    .nodename = "(none)",
    .release = "4.18.0-g53f52bdb-dirty",
    .version = "#17 SMP Wed Feb 15 09:53:34 CST 2023",
    .machine = "riscv64",
    .domainname = "(none)"
};

/**
 * @brief return the system message
 * 
 */
uint8 do_uname(struct utsname *uts)
{
    // if (sizeof(uts) != sizeof(utsname_t))
    //     return -1;
    strcpy(uts->sysname, utsname.sysname); 
    strcpy(uts->nodename, utsname.nodename); 
    strcpy(uts->release, utsname.release);
    strcpy(uts->version, utsname.version);
    strcpy(uts->machine, utsname.machine);
    strcpy(uts->domainname, utsname.domainname);
    return 0;
}