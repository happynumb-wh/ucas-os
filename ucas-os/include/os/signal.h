#ifndef INCLUDE_SCHED_SIGNAL_H
#define INCLUDE_SCHED_SIGNAL_H
#include <os/sched.h>
#include <os/time.h>
#include <type.h>

/* for sigprocmask */
#define SIG_BLOCK 0x0
#define SIG_UNBLOCK 0x1
#define SIG_SETMASK 0x2

/* size of sigset_t */
#define _NSIG_WORDS 1

#define NUM_SIG 64


/* signal */
#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGALRM 14
#define SIGTERM 15
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19      
#define SIGTSTP 20

#define SIGTIMER 32
#define SIGCANCEL 33
#define SIGSYNCCALL 34

#define SA_RESTART    0x10000000
#define SA_RESTORER   0x04000000

#define SIG_DFL ((void(*)(int))0)
#define SIG_IGN ((void(*)(int))1)

typedef struct{
    unsigned long sig[_NSIG_WORDS];
}sigset_t;

typedef struct siginfo{
    int      si_signo;     /* Signal number */
    int      si_errno;     /* An errno value */
    int      si_code;      /* Signal code */
    int      si_trapno;    /* Trap number that caused
                             hardware-generated signal
                             (unused on most architectures) */
    pid_t    si_pid;       /* Sending process ID */
    uid_t    si_uid;       /* Real user ID of sending process */
    int      si_status;    /* Exit value or signal */
    clock_t  si_utime;     /* User time consumed */
    clock_t  si_stime;     /* System time consumed */
    sigval_t si_value;     /* Signal value */
    int      si_int;       /* POSIX.1b signal */
    void    *si_ptr;       /* POSIX.1b signal */
    int      si_overrun;   /* Timer overrun count;
                             POSIX.1b timers */
    int      si_timerid;   /* Timer ID; POSIX.1b timers */
    void    *si_addr;      /* Memory location which caused fault */
    long     si_band;      /* Band event (was int in glibc 2.3.2 and earlier) */
    int      si_fd;        /* File descriptor */
    short    si_addr_lsb;  /* Least significant bit of address
                             (since Linux 2.6.32) */
    void    *si_lower;     /* Lower bound when address violation
                             occurred (since Linux 3.19) */
    void    *si_upper;     /* Upper bound when address violation */
    int      si_pkey;      /* Protection key on PTE that caused
                             fault (since Linux 4.6) */
    void    *si_call_addr; /* Address of system call instruction
                             (since Linux 3.5) */
    int      si_syscall;   /* Number of attempted system call
                             (since Linux 3.5) */
    unsigned int si_arch;  /* Architecture of attempted system call
                             (since Linux 3.5) */
}siginfo_t;

// kernel sigaction
typedef struct sigaction{
	void (*handler)(int);
	unsigned long flags;
	void (*restorer)(void);
	sigset_t   sa_mask;
}sigaction_t;



#define ISSET_SIG(signum,pcb) \
    ((pcb)->sig_recv & ( 1lu << ((signum) - 1) ) )

#define CLEAR_SIG(signum,pcb) \
    ((pcb)->sig_recv &= ~( 1lu << ((signum) - 1) ) )

#define SET_SIG(signum,pcb) \
    ((pcb)->sig_recv |= ( 1lu << ((signum) - 1) ) )


typedef struct sigaltstack {
	void *ss_sp;
	int ss_flags;
	size_t ss_size;
}stack_t;

// for the user signal
typedef unsigned long __riscv_mc_gp_state[32];

struct __riscv_mc_f_ext_state {
	unsigned int __f[32];
	unsigned int __fcsr;
};

struct __riscv_mc_d_ext_state {
	unsigned long long __f[32];
	unsigned int __fcsr;
};

struct __riscv_mc_q_ext_state {
	unsigned long long __f[64] __attribute__((aligned(16)));
	unsigned int __fcsr;
	unsigned int __reserved[3];
};

union __riscv_mc_fp_state {
	struct __riscv_mc_f_ext_state __f;
	struct __riscv_mc_d_ext_state __d;
	struct __riscv_mc_q_ext_state __q;
};

typedef struct mcontext_t {
	__riscv_mc_gp_state __gregs;
	union __riscv_mc_fp_state __fpregs;
} mcontext_t;

typedef struct sigset_t 
{ 
    unsigned long __bits[128/sizeof(long)]; 
} usigset_t;

typedef struct ucontext
{
	unsigned long uc_flags;
	struct ucontext_t *uc_link;
	stack_t uc_stack;
	usigset_t uc_sigmask;
	mcontext_t uc_mcontext;
} ucontext_t;


extern void handle_signal();
int do_rt_sigprocmask(int32_t how, const sigset_t *restrict set, sigset_t *restrict oldset, size_t sigsetsize);
void do_rt_sigreturn();
int do_rt_sigaction(int32_t signum, struct sigaction *act, struct sigaction *oldact, size_t sigsetsize);
int do_rt_sigtimedwait(const sigset_t *restrict set, \
                       siginfo_t *restrict info, \
                       const struct timespec *restrict timeout);
uint64_t get_sig_num(void * pcb, sigset_t *set);
uintptr_t set_ucontext();

void init_sig_table();
sigaction_t *alloc_sig_table();
void free_sig_table(sigaction_t* sig_in);
#endif