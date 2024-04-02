#ifndef INCLUDE_RESOURCE_H
#define INCLUDE_RESOURCE_H

#include <type.h>

/* Kinds of resource limit.  */
enum __rlimit_resource
{
    /* Per-process CPU limit, in seconds.  */
    RLIMIT_CPU = 0,
#define RLIMIT_CPU RLIMIT_CPU
    /* Largest file that can be created, in bytes.  */
    RLIMIT_FSIZE = 1,
#define        RLIMIT_FSIZE RLIMIT_FSIZE
    /* Maximum size of data segment, in bytes.  */
    RLIMIT_DATA = 2,
#define        RLIMIT_DATA RLIMIT_DATA
    /* Maximum size of stack segment, in bytes.  */
    RLIMIT_STACK = 3,
#define        RLIMIT_STACK RLIMIT_STACK
    /* Largest core file that can be created, in bytes.  */
    RLIMIT_CORE = 4,
#define        RLIMIT_CORE RLIMIT_CORE
    /* Largest resident set size, in bytes.
        This affects swapping; processes that are exceeding their
        resident set size will be more likely to have physical memory
        taken from them.  */
    __RLIMIT_RSS = 5,
#define        RLIMIT_RSS __RLIMIT_RSS
    /* Number of open files.  */
    RLIMIT_NOFILE = 7,
    __RLIMIT_OFILE = RLIMIT_NOFILE, /* BSD name for same.  */
#define RLIMIT_NOFILE RLIMIT_NOFILE
#define RLIMIT_OFILE __RLIMIT_OFILE
    /* Address space limit.  */
    RLIMIT_AS = 9,
#define RLIMIT_AS RLIMIT_AS
    /* Number of processes.  */
    __RLIMIT_NPROC = 6,
#define RLIMIT_NPROC __RLIMIT_NPROC
    /* Locked-in-memory address space.  */
    __RLIMIT_MEMLOCK = 8,
#define RLIMIT_MEMLOCK __RLIMIT_MEMLOCK
    /* Maximum number of file locks.  */
    __RLIMIT_LOCKS = 10,
#define RLIMIT_LOCKS __RLIMIT_LOCKS
    /* Maximum number of pending signals.  */
    __RLIMIT_SIGPENDING = 11,
#define RLIMIT_SIGPENDING __RLIMIT_SIGPENDING
    /* Maximum bytes in POSIX message queues.  */
    __RLIMIT_MSGQUEUE = 12,
#define RLIMIT_MSGQUEUE __RLIMIT_MSGQUEUE
    /* Maximum nice priority allowed to raise to.
        Nice levels 19 .. -20 correspond to 0 .. 39
        values of this resource limit.  */
    __RLIMIT_NICE = 13,
#define RLIMIT_NICE __RLIMIT_NICE
    /* Maximum realtime priority allowed for non-priviledged
        processes.  */
    __RLIMIT_RTPRIO = 14,
#define RLIMIT_RTPRIO __RLIMIT_RTPRIO
    /* Maximum CPU time in µs that a process scheduled under a real-time
        scheduling policy may consume without making a blocking system
        call before being forcibly descheduled.  */
    __RLIMIT_RTTIME = 15,
#define RLIMIT_RTTIME __RLIMIT_RTTIME
    __RLIMIT_NLIMITS = 16,
    __RLIM_NLIMITS = __RLIMIT_NLIMITS
#define RLIMIT_NLIMITS __RLIMIT_NLIMITS
#define RLIM_NLIMITS __RLIM_NLIMITS
};

struct rlimit {
    rlim_t rlim_cur;  /* Soft limit */
    rlim_t rlim_max;  /* Hard limit (ceiling for rlim_cur) */
};


#endif