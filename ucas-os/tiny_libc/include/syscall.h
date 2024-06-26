#ifndef __SYSCALL_H__
#define __SYSCALL_H__
#define SHELL_SYSCALL_BASE 300

#define SYSCALL_EXEC            (0 + SHELL_SYSCALL_BASE)
#define SYSCALL_SLEEP           (2 + SHELL_SYSCALL_BASE)
#define SYSCALL_KILL            (3 + SHELL_SYSCALL_BASE)
#define SYSCALL_WAITPID         (4 + SHELL_SYSCALL_BASE)
#define SYSCALL_PS              (5 + SHELL_SYSCALL_BASE)
#define SYSCALL_GETPID          (6 + SHELL_SYSCALL_BASE)
#define SYSCALL_YIELD           (7 + SHELL_SYSCALL_BASE)
#define SYSCALL_WRITE           (20 + SHELL_SYSCALL_BASE)
#define SYSCALL_READCH          (21 + SHELL_SYSCALL_BASE)
#define SYSCALL_REFLUSH         (23 + SHELL_SYSCALL_BASE)
#define SYSCALL_GET_TIMEBASE    (30 + SHELL_SYSCALL_BASE)
#define SYSCALL_GET_TICK        (31 + SHELL_SYSCALL_BASE)


#endif