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
    strcpy((char *)uts->sysname, utsname.sysname); 
    strcpy((char *)uts->nodename, utsname.nodename); 
    strcpy((char *)uts->release, utsname.release);
    strcpy((char *)uts->version, utsname.version);
    strcpy((char *)uts->machine, utsname.machine);
    strcpy((char *)uts->domainname, utsname.domainname);
    return 0;
}