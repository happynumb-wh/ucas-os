#include <syscall.h>
#include <stdint.h>
#include <unistd.h>

static const long IGNORE = 0L;

static long invoke_syscall(long sysno, long arg0, long arg1, long arg2,
                           long arg3, long arg4)
{
    register long a7 asm("a7") = sysno;
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a3 asm("a3") = arg3;
    register long a4 asm("a4") = arg4;
    asm volatile("ecall"                      \
                 : "+r"(a0)                   \
                 : "r"(a1), "r"(a2), "r"(a3), \
                   "r"(a4), "r"(a7)           \
                 : "memory");

    return a0;
}

void sys_yield(void)
{
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}


void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, (long)buff, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_getchar(void)
{
    int ch = -1;
    while (ch == -1)
    {
        ch = invoke_syscall(SYSCALL_READCH, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
    }

    return ch;
}

void sys_reflush(void)
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}


long sys_get_tick(void)
{
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

typedef uint64_t clock_t;

clock_t clock();

typedef struct
{
    clock_t sec; 
    clock_t usec;
} TimeVal;

void sys_sleep(uint32_t time)
{
    TimeVal tv = {.sec = time, .usec = 0};
    if (invoke_syscall(SYSCALL_SLEEP, (long)&tv, (long)&tv, IGNORE, IGNORE, IGNORE)) return;
    return;
}

void sys_ps(void)
{
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_exit(void)
{
    while (1)
    {
        /* code */
    }
}

pid_t sys_getpid(void)
{
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_exec(char *name, int argc, char **argv)
{
    return invoke_syscall(SYSCALL_EXEC, (long)name, (long)argc, (long)argv, (long)1, IGNORE);
}


int sys_kill(pid_t pid)
{
    return invoke_syscall(SYSCALL_KILL, (long)pid, (long)9, IGNORE, IGNORE, IGNORE);
}


int sys_waitpid(pid_t pid)
{
    uint16_t status;
    return invoke_syscall(SYSCALL_WAITPID, (long)pid, (long)&status, (long)0, IGNORE, IGNORE);
}

