#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/syscall.h>
#include <os/uname.h>
#include <os/lock.h>
#include <os/system.h>
#include <os/signal.h>
#include <os/futex.h>
#include <os/socket.h>
#include <os/smp.h>
#include <os/io.h>
#include <fs/poll.h>
#include <fs/pipe.h>
// #include <csr.h>
#include <pgtable.h>
#include <sdcard.h>
#include <sbi.h>
#include <stdio.h>
#include <screen.h>
#include <common.h>
#include <assert.h>
#include <os/elf.h>
#include <user_programs.h>

spinlock_init(cpu);

// init available pcb
static void init_pcb_queue()
{
    init_list(&available_queue.free_source_list);
    init_list(&available_queue.wait_list);
    // init all pcb available queue
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        pcb[i].used = 0;
        pcb[i].status = TASK_EXITED;
        list_add_tail(&pcb[i].list, &available_queue.free_source_list);
    }
    available_queue.source_num = NUM_MAX_TASK;
}

// init the first process - shell
static pid_t init_shell()
{
    /* init other pcb */
    init_pcb_queue();
    init_pre_load();

    char *shell_argv[] = {
        "shell",
        NULL,
    };

    pid_t shell_pid = do_exec("shell", 1, shell_argv, AUTO_CLEANUP_ON_EXIT);

    // Init pcb0 master
    init_list(&pid0_pcb_master.list);
    init_list(&pid0_pcb_slave.list);
    pid0_pcb_master.end_time = 0;
    pid0_pcb_master.utime = 0;
    pid0_pcb_master.stime = 0;
    pid0_pcb_master.pgdir = pa2kva(PGDIR_PA);

    pid0_pcb_slave.end_time = 0;
    pid0_pcb_slave.utime = 0;
    pid0_pcb_slave.stime = 0;
    pid0_pcb_slave.pgdir = pa2kva(PGDIR_PA);
    sys_time_master = sys_time_slave = get_ticks();
    current_running_master = &pid0_pcb_master;
    current_running_slave  = &pid0_pcb_slave;


    return shell_pid; 
}

static void init_syscall(void)
{
    // initialize system call table.
    int i;
	for(i = 0; i < NUM_SYSCALLS; i++)
		syscall[i] = (long int (*)()) &handle_miss;
// ============ for the shell===================
    /* tmpt */ 
    syscall[SYS_screen_write]           = (long int (*)())&screen_write;   
    syscall[SYS_screen_reflush]         = (long int (*)())&screen_reflush;
    syscall[SYS_screen_clear]           = (long int (*)())&screen_clear;
    syscall[SYS_get_timer]              = (long int (*)())&get_timer;         
    syscall[SYS_get_tick]               = (long int (*)())&get_ticks;
    syscall[SYS_exec]                   = (long int (*)())&do_exec;
    
    syscall[SYSCALL_EXEC]               = (long int (*)())&do_exec;
    syscall[SYSCALL_SLEEP]              = (long int (*)())&do_nanosleep;
    syscall[SYSCALL_PS]                 = (long int (*)())&do_ps;
    syscall[SYSCALL_GETPID]             = (long int (*)())&do_getpid;
    syscall[SYSCALL_KILL]               = (long int (*)())&do_kill;
    syscall[SYSCALL_WAITPID]            = (long int (*)())&do_wait4;
    syscall[SYSCALL_READCH]             = (long int (*)())&sbi_console_getchar;
    syscall[SYSCALL_WRITE]              = (long int (*)())&screen_write; 
    syscall[SYSCALL_REFLUSH]            = (long int (*)())&screen_reflush;
// =========== for the k210===================
    /* id */
    syscall[SYS_getpid]              = (long int (*)())&do_getpid; 
    syscall[SYS_getppid]             = (long int (*)())&do_getppid;

    /* time */
    syscall[SYS_gettimeofday]        = (long int (*)())&do_gettimeofday;
    syscall[SYS_times]               = (long int (*)())&do_gettimes;

    /* process */
    syscall[SYS_exit]                = (long int (*)())&do_exit;
    syscall[SYS_clone]               = (long int (*)())&do_clone;
    syscall[SYS_wait4]               = (long int (*)())&do_wait4;
    syscall[SYS_nanosleep]           = (long int (*)())&do_nanosleep;
    syscall[SYS_execve]              = (long int (*)())&do_execve;
    syscall[SYS_sched_yield]         = (long int (*)())&do_scheduler;
    /* id */
    /* sys_msg */
    syscall[SYS_uname]               = (long int (*)())&do_uname;

    /* mm */
    syscall[SYS_brk]                 = (long int (*)())&do_brk;

    /* fat32 */
    /* open close */
    syscall[SYS_openat]              = (long int (*)())&fat32_openat;
    syscall[SYS_close]               = (long int (*)())&fat32_close;

    /* read */
    syscall[SYS_read]                = (long int (*)())&fat32_read;
    // /* write */
    syscall[SYS_write]               = (long int (*)())&fat32_write;

    syscall[SYS_pipe2]               = (long int (*)())&fat32_pipe2;
    syscall[SYS_dup]                 = (long int (*)())&fat32_dup;
    syscall[SYS_dup3]                = (long int (*)())&fat32_dup3;

    syscall[SYS_getcwd]              = (long int (*)())&fat32_getcwd;
    syscall[SYS_chdir]               = (long int (*)())&fat32_chdir;
    syscall[SYS_mkdirat]             = (long int (*)())&fat32_mkdirat;
    syscall[SYS_getdents64]          = (long int (*)())&fat32_getdents;
    syscall[SYS_linkat]              = (long int (*)())&fat32_link;
    syscall[SYS_unlinkat]            = (long int (*)())&fat32_unlink;
    syscall[SYS_mount]               = (long int (*)())&fat32_mount;
    syscall[SYS_umount2]             = (long int (*)())&fat32_umount;
    syscall[SYS_fstat]               = (long int (*)())&fat32_fstat;
    syscall[SYS_mmap]                = (long int (*)())&fat32_mmap;
    syscall[SYS_munmap]              = (long int (*)())&fat32_munmap;
/* == final comp == */
// stage 1
    syscall[SYS_fcntl]               = (long int (*)())&fat32_fcntl;
    syscall[SYS_ioctl]               = (long int (*)())&do_ioctl;
    syscall[SYS_readv]               = (long int (*)())&fat32_readv;
    syscall[SYS_writev]              = (long int (*)())&fat32_writev;
    syscall[SYS_pread]               = (long int (*)())&fat32_pread;
    syscall[SYS_pwrite]              = (long int (*)())&fat32_pwrite;
    syscall[SYS_lseek]               = (long int (*)())&fat32_lseek;
    syscall[SYS_fstatat]             = (long int (*)())&fat32_fstatat;
    syscall[SYS_utimensat]           = (long int (*)())&fat32_utimensat;
    syscall[SYS_exit_group]          = (long int (*)())&do_exit_group;
    syscall[SYS_set_tid_address]     = (long int (*)())&do_set_tid_address;
    syscall[SYS_clock_gettime]       = (long int (*)())&do_clock_gettime;
    // id
    syscall[SYS_geteuid]             = (long int (*)())&do_geteuid;
    syscall[SYS_getegid]             = (long int (*)())&do_getegid;
    syscall[SYS_gettid]              = (long int (*)())&do_gettid;
    syscall[SYS_setsid]              = (long int (*)())&do_setsid;
    
    syscall[SYS_sysinfo]             = (long int (*)())&do_sysinfo;
    syscall[SYS_statfs]              = (long int (*)())&fat32_statfs;
    syscall[SYS_fstatfs]             = (long int (*)())&fat32_fstatfs;
    syscall[SYS_getrlimit]           = (long int (*)())&do_getrlimit;
    syscall[SYS_setrlimit]           = (long int (*)())&do_setrlimit;
    syscall[SYS_prlimit]             = (long int (*)())&do_prlimit;
    syscall[SYS_tkill]               = (long int (*)())&do_tkill;
    syscall[SYS_sched_setscheduler]  = (long int (*)())&do_sched_setscheduler;
    syscall[SYS_sched_getaffinity]   = (long int (*)())&do_sched_getaffinity;
    syscall[SYS_ppoll]               = (long int (*)())&do_ppoll;  
    syscall[SYS_kill]                = (long int (*)())&do_kill;
    syscall[SYS_rt_sigaction]        = (long int (*)())&do_rt_sigaction;
    syscall[SYS_rt_sigprocmask]      = (long int (*)())&do_rt_sigprocmask;
    syscall[SYS_rt_sigreturn]        = (long int (*)())&do_rt_sigreturn;
    syscall[SYS_rt_sigtimedwait]     = (long int (*)())&do_rt_sigtimedwait;
    syscall[SYS_mprotect]            = (long int (*)())&do_mprotect;
    syscall[SYS_futex]               = (long int (*)())&do_futex;
    syscall[SYS_set_robust_list]     = (long int (*)())&do_set_robust_list;
    syscall[SYS_get_robust_list]     = (long int (*)())&do_get_robust_list;
    syscall[SYS_socket]              = (long int (*)())&do_socket;
    syscall[SYS_bind]                = (long int (*)())&do_bind;
    syscall[SYS_listen]              = (long int (*)())&do_listen;
    syscall[SYS_connect]             = (long int (*)())&do_connect;
    syscall[SYS_accept]              = (long int (*)())&do_accept;
    syscall[SYS_getsockname]         = (long int (*)())&do_getsockname;
    syscall[SYS_setsockopt]          = (long int (*)())&do_setsockopt;
    syscall[SYS_sendto]              = (long int (*)())&do_sendto;
    syscall[SYS_recvfrom]            = (long int (*)())&do_recvfrom;
    syscall[SYS_mremap]              = (long int (*)())&fat32_mremap;
    syscall[SYS_madvise]             = (long int (*)())&do_madvise;
    syscall[SYS_membarrier]          = (long int (*)())&do_membarrier;
//stage 2
    syscall[SYS_tgkill]              = (long int (*)())&do_tgkill;
    syscall[SYS_getuid]              = (long int (*)())&do_getuid;
    syscall[SYS_getgid]              = (long int (*)())&do_getgid;
    syscall[SYS_faccessat]           = (long int (*)())&fat32_faccessat;
    syscall[SYS_fsync]               = (long int (*)())&fat32_fsync;
    syscall[SYS_syslog]              = (long int (*)())&do_syslog;
    syscall[SYS_vm86]                = (long int (*)())&do_vm86;
    syscall[SYS_readlinkat]          = (long int (*)())&fat32_readlinkat; // specialized
    // below TODO
    syscall[SYS_getrusage]           = (long int (*)())&do_getrusage; 
    syscall[SYS_sendfile]            = (long int (*)())&fat32_sendfile;
    syscall[SYS_pselect6]            = (long int (*)())&do_pselect6;
    syscall[SYS_setitimer]           = (long int (*)())&do_setitimer;
    syscall[SYS_msync]               = (long int (*)())&fat32_msync;
    syscall[SYS_renameat2]           = (long int (*)())&fat32_renameat2;
}


// The beginning of everything >_< ~~~~~~~~~~~~~~
int main(unsigned long mhartid)
{   
    if (!mhartid) {
        printk("> [INIT] First core enter kernel\n\r");
        // init kernel lock
        // init_kernel_lock();

        // init mem
        uint32_t free_page_num = init_mem();
        printk("> [INIT] %d Mem initialization succeeded.\n\r", free_page_num); 
        
        // // init fd_table
        // init_fd_table();
        // init_sig_table();
        // printk("> [INIT] fd and sig table initialization succeeded.\n\r");
        // init shell
        pid_t shell_pid =  init_shell();
        printk("> [INIT] shell initialization succeeded, pid: %ld.\n\r", shell_pid);


        ((uintptr_t *)(pa2kva(PGDIR_PA)))[1] = NULL;
        ((uintptr_t *)(get_pcb_by_pid(shell_pid)))[1] = NULL;

        pid0_pcb_master.core_id = 0;

        // knock down
        time_base = TIME_BASE;//sbi_read_fdt(TIMEBASE);

        // binit();
        // printk("> [INIT] bio cache initialization succeeded.\n\r"); 

        // fat32_init();
        // printk("> [INIT] FAT32 init succeeded.\n\r");
        // init interrupt (^_^)
        init_exception();
        printk("> [INIT] Interrupt processing initialization succeeded.\n\r");   
        // init system call table (0_0)
        init_syscall();
        printk("> [INIT] System call initialized successfully.\n\r");
        // init screen (QAQ)
        init_screen();
        printk("> [INIT] SCREEN initialization succeeded.\n\r");
        // init futex
        // init_system_futex();
        // printk("> [INIT] System_futex initialization succeeded.\n\r");
        // init socket
        // init_socket();
        // printk("> [INIT] Socket initialization succeeded.\n\r");
        // while(1);
        // init_pre_load();
        // do_pre_load(dll_linker, LOAD);
        // printk("> [INIT] Init preload linker %s succeeded.\n\r", dll_linker);
        
    }else{
        printk("> [INIT] Second core enter kernel\n");
        // wait clean boot map
        pid0_pcb_slave.core_id = 1;
        setup_exception();  
        while(1);    
    }
    sbi_set_timer(get_ticks() + 200);
    // open float
    /* open interrupt */
    enable_interrupt();
 
    while (1) {    
        __asm__ __volatile__("wfi\n":::);
    }
    return 0;
}
