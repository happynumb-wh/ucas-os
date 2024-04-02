#ifndef INCLUDE_SYSTEM_H
#define INCLUDE_SYSTEM_H

#include <os/time.h>
#include <type.h>
#include <os/errno.h>
#include <os/resource.h>

#define SYSCALL_SUCCESSED 0
#define SYSCALL_FAILED -1
#ifndef IGNORE
    #define IGNORE 0
#endif


struct sysinfo {
    long uptime;             /* Seconds since boot */
    unsigned long loads[3];  /* 1, 5, and 15 minute load averages */
    unsigned long totalram;  /* Total usable main memory size */
    unsigned long freeram;   /* Available memory size */
    unsigned long sharedram; /* Amount of shared memory */
    unsigned long bufferram; /* Memory used by buffers */
    unsigned long totalswap; /* Total swap space size */
    unsigned long freeswap;  /* Swap space still available */
    unsigned short procs;    /* Number of current processes */
    unsigned long totalhigh; /* Total high memory size */
    unsigned long freehigh;  /* Available high memory size */
    unsigned int mem_unit;   /* Memory unit size in bytes */
    char _f[20-2*sizeof(long)-sizeof(int)]; /* Padding to 64 bytes */
};

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN (-1)
#define RUSAGE_THREAD   1

struct rusage {
    struct timeval ru_utime; /* user CPU time used */
    struct timeval ru_stime; /* system CPU time used */
    long   ru_maxrss;        /* maximum resident set size */
    long   ru_ixrss;         /* integral shared memory size */
    long   ru_idrss;         /* integral unshared data size */
    long   ru_isrss;         /* integral unshared stack size */
    long   ru_minflt;        /* page reclaims (soft page faults) */
    long   ru_majflt;        /* page faults (hard page faults) */
    long   ru_nswap;         /* swaps */
    long   ru_inblock;       /* block input operations */
    long   ru_oublock;       /* block output operations */
    long   ru_msgsnd;        /* IPC messages sent */
    long   ru_msgrcv;        /* IPC messages received */
    long   ru_nsignals;      /* signals received */
    long   ru_nvcsw;         /* voluntary context switches */
    long   ru_nivcsw;        /* involuntary context switches */
};

extern struct rlimit mylimit;

int do_prlimit(pid_t pid,   int resource,  const struct rlimit *new_limit, 
    struct rlimit *old_limit);
int do_getrlimit(int resource,struct rlimit * rlim);
int do_setrlimit(int resource,const struct rlimit * rlim);
int32_t do_sysinfo(struct sysinfo *info);
void do_syslog(void);
int do_vm86(unsigned long fn, void *v86);
int32_t do_getrusage(int32_t who, struct rusage *usage);
#endif