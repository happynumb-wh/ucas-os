#include <os/irq.h>
#include <os/time.h>
#include <fs/fat32.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/mm.h>
#include <os/kdasics.h>
#include <os/futex.h>
#include <asm/regs.h>
#include <stdio.h>
#include <assert.h>
#include <sbi.h>
#include <screen.h>
#include <csr.h>
#include <sdcard.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;
//debug
char*syscall_name[512];

void reset_irq_timer()
{
    screen_reflush();
    check_sleeping();
    check_futex_timeout();
    itimer_check();
    sbi_set_timer(get_ticks() + time_base);
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // TODO interrupt handler.
    // call corresponding handler by the value of `cause`
    regs_context_t * prev_context = current->save_context;
    // reg_t prev_sp =  current->kernel_sp;
    current->save_context = regs;
    // current->kernel_sp = (uint64_t)regs;
    if ((regs->sstatus & SR_SPP) == 0)
        update_utime();
    else 
        update_stime();

    if ((cause & SCAUSE_IRQ_FLAG) == SCAUSE_IRQ_FLAG) 
    {
        irq_table[regs->scause & ~(SCAUSE_IRQ_FLAG)](regs, stval, cause);
    } else
    {
        exc_table[regs->scause](regs, stval, cause);
    }
    // No nested exception
    if ((regs->sstatus & SR_SPP) != 0)
    {
        regs->sscratch = 0;
        current->save_context = prev_context;
        
    }

    update_stime();
}

void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    reset_irq_timer();
}

void handle_page_fault_inst(regs_context_t *regs, uint64_t stval, uint64_t cause){
    // printk("inst page fault: %lx\n",stval);
    uint64_t fault_addr = stval;
    // PTE * pte = get_PTE_of(stval, current->pgdir);
    // if (pte)
    //     printk("store sepc: 0x%lx, PTE: 0x%lx, stval: 0x%lx\n",regs->sepc, *pte, stval);   
    switch(check_W_SD_and_set_AD(fault_addr, current_running->pgdir, LOAD))
    {
    case ALLOC_NO_AD:
        // local_flush_tlb_page(stval);
        break;
    case DASICS_NO_V:
        PTE * pte = get_PTE_of(fault_addr, current->pgdir);
        assert(pte != NULL);
        *pte = *pte | _PAGE_PRESENT;
        break;
    default: 
        printk(">[Error] can't resolve the inst page fault with 0x%x!\n",stval);
        handle_other(regs,stval,cause);        
        break;
    };

    local_flush_tlb_all();

}


void handle_page_fault_load(regs_context_t *regs, uint64_t stval, uint64_t cause){
    // printk("load page fault: %lx\n",stval);
    uint64_t fault_addr = stval;   
    // PTE * pte = get_PTE_of(stval, current->pgdir);
    // if (pte)
    //     printk("load PTE: 0x%lx, stval: 0x%lx\n", *pte, stval);   
    switch (check_W_SD_and_set_AD(fault_addr, current_running->pgdir, LOAD))
    {
    case ALLOC_NO_AD:
        // local_flush_tlb_page(stval);
        break;
    case IN_SD:
        printk("> Attention: the 0x%lx addr was swapped to SD! the origin physical addr is 0x%lx\n", 
        fault_addr, kva2pa(get_pfn_of(fault_addr, current_running->pgdir)));
        break; 
    case NO_ALLOC:
        if (!is_legal_addr(stval))
            handle_other(regs, stval, cause);
        alloc_page_helper(fault_addr, current_running->pgdir, MAP_USER, \
                        _PAGE_READ | _PAGE_WRITE | _PAGE_ACCESSED | _PAGE_ACCESSED);
        current_running->pge_num++;
        break; 
    case DASICS_NO_V:
        PTE * pte = get_PTE_of(fault_addr, current->pgdir);
        assert(pte != NULL);
        *pte = *pte | _PAGE_PRESENT;
        break;             
    default:
        break;
    };

    local_flush_tlb_all();

    // printk("load page end\n");
}

void handle_page_fault_store(regs_context_t *regs, uint64_t stval, uint64_t cause){
    // printk("store page fault: %lx\n",stval);
    // if (is_kva(stval))
    //     return;
    uint64_t fault_addr = stval;
    // PTE * pte = get_PTE_of(stval, current->pgdir);
    // if (pte)
    //     printk("store sepc: 0x%lx, PTE: 0x%lx, stval: 0x%lx\n",regs->sepc, *pte, stval);   
    switch (check_W_SD_and_set_AD(fault_addr, current_running->pgdir, STORE))
    {
    case ALLOC_NO_AD:
        // local_flush_tlb_page(stval);
        break;
    case IN_SD:
        break; 
    case IN_SD_NO_W:
        break; 
    case NO_W:
        deal_no_W(fault_addr);
        // local_flush_tlb_page(stval);
        break;
    case NO_ALLOC:
        alloc_page_helper(fault_addr, current_running->pgdir, MAP_USER, \
                        _PAGE_READ | _PAGE_WRITE | _PAGE_ACCESSED | _PAGE_DIRTY);
        // load_lazy_mmap(fault_addr);
        current_running->pge_num++;  
        break;      
    case DASICS_NO_V:
        PTE * pte = get_PTE_of(fault_addr, current->pgdir);
        assert(pte != NULL);
        *pte = *pte | _PAGE_PRESENT;
        break;
    default:
        break;
    }
    local_flush_tlb_all();

    // printk("store page end\n");
}


void init_exception()
{
    /* note: handle_int, handle_syscall, handle_other, etc.*/
    for ( int i = 0; i < IRQC_COUNT; i++)
    {
        irq_table[i] = &handle_other;
    }
    for ( int i = 0; i < EXCC_COUNT; i++)
    {
        exc_table[i] = &handle_other;
    }
    irq_table[IRQC_S_TIMER]          = &handle_int;
    // irq_table[IRQC_S_SOFT]           = &disk_intr;
    exc_table[EXCC_SYSCALL]          = &handle_syscall;
    exc_table[EXCC_INST_PAGE_FAULT]  = &handle_page_fault_inst;
    exc_table[EXCC_LOAD_ACCESS]      = &handle_page_fault_load;    
    exc_table[EXCC_LOAD_PAGE_FAULT]  = &handle_page_fault_load;
    exc_table[EXCC_STORE_PAGE_FAULT] = &handle_page_fault_store;
    exc_table[EXCC_STORE_ACCESS]     = &handle_page_fault_store;
    // Dasics fault
    exc_table[FDIUJumpFault]         = &handle_dasics_fetch_fault;
    exc_table[FDIULoadAccessFault]   = &handle_dasics_load_fault;
    exc_table[FDIUStoreAccessFault]  = &handle_dasics_store_fault;


    // exc_table[EXCC_INST_PAGE_FAULT]  = &handle_page_fault_inst;
    setup_exception();
    //debug
    init_syscall_name();
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    printk("name: %s, pid: %ld\n", current->name, current->pid);
    printk("%s : %016lx ", "zero", regs->zero);
    printk("%s : %016lx ", "ra", regs->ra);
    printk("%s : %016lx ", "sp", regs->sp);
    printk("\n\r");
    printk("%s : %016lx ", "gp", regs->gp);
    printk("%s : %016lx ", "tp", regs->tp);
    printk("%s : %016lx ", "t0", regs->t0);
    printk("\n\r");
    printk("%s : %016lx ", "t1", regs->t1);
    printk("%s : %016lx ", "t2", regs->t2);
    printk("%s : %016lx ", "s0/fp", regs->s0);
    printk("\n\r");
    printk("%s : %016lx ", "s1", regs->s1);
    printk("%s : %016lx ", "a0", regs->a0);
    printk("%s : %016lx ", "a1", regs->a1);
    printk("\n\r");
    printk("%s : %016lx ", "a2", regs->a2);
    printk("%s : %016lx ", "a3", regs->a3);
    printk("%s : %016lx ", "a4", regs->a4);
    printk("\n\r");    
    printk("%s : %016lx ", "a5", regs->a5);
    printk("%s : %016lx ", "a6", regs->a6);
    printk("%s : %016lx ", "a7", regs->a7);
    printk("\n\r");
    printk("%s : %016lx ", "s2", regs->s2);
    printk("%s : %016lx ", "s3", regs->s3);
    printk("%s : %016lx ", "s4", regs->s4);
    printk("\n\r"); 
    printk("%s : %016lx ", "s5", regs->s5);
    printk("%s : %016lx ", "s6", regs->s6);
    printk("%s : %016lx ", "s7", regs->s7);
    printk("\n\r");    
    printk("%s : %016lx ", "s8", regs->s8);
    printk("%s : %016lx ", "s9", regs->s9);
    printk("%s : %016lx ", "s10", regs->s10);
    printk("\n\r");
    printk("%s : %016lx ", "s11", regs->s11);
    printk("%s : %016lx ", "t3", regs->t3);
    printk("%s : %016lx ", "t4", regs->t4);
    printk("\n\r");
    printk("%s : %016lx ", "t5", regs->t5);
    printk("%s : %016lx ", "t6", regs->t6);
    printk("\n\r");
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("stval: 0x%lx cause: %lx\n\r",
           stval, cause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
    printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    uintptr_t fp = regs->s0, sp = regs->sp;
    printk("[Backtrace]\n\r");
    printk("  addr: %lx sp: %lx fp: %lx\n\r", regs->ra - 4, sp, fp);


    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    // while (fp > 0x10000) {
    //     uintptr_t prev_ra = *(uintptr_t*)(fp-8);
    //     uintptr_t prev_fp = *(uintptr_t*)(fp-16);

    //     printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

    //     fp = prev_fp;
    // }
    // if (regs->scause == EXCC_LOAD_PAGE_FAULT)
    // {
    //     uint64_t kva =  get_kva_of(regs->sbadaddr, current->pgdir);
    //     printk("The kva: %lx, addr: %lx\n", kva, *(uint64_t *)kva);
    // }


    // screen_reflush();
    assert(0);
}

void handle_miss(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // the tp will be other thingss
    printk("name: %s, pid: %ld\n", current_running->name, current_running->pid);    
    printk("handle undefined syscall_number: %ld\n", regs->a7);
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("[Backtrace]\n\r");
    printk("  addr: 0x%lx sp: 0x%lx fp: 0x%lx\n\r", regs->ra - 4, regs->sp, regs->s0);
    screen_reflush();
    assert(0);
}

void deal_s_trap(pcb_t * tp, uint64_t stval, uint64_t cause){
    if(tp->pid ){
        if(cause == EXCC_STORE_PAGE_FAULT || \
           cause == EXCC_LOAD_PAGE_FAULT || \
           cause ==  EXCC_STORE_ACCESS ){
            if(tp->save_context->sstatus & SR_SPP){
                #ifdef QEMU
                    __asm__ volatile (
                            "la t0, 0x00040000\n"
                            "csrw sstatus, t0\n"
                            );
                #else
                    tp->save_context->sstatus = 0;
                    __asm__ volatile ("csrwi sstatus, 0\n");

                #endif                
            }
        }
    }
}

void debug_info(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // printk("==============debug-info start==============\n\r");
    if(regs->a7 == 165 || \
       regs->a7 == 321 || \
       regs->a7 == 323 || \
       regs->a7 == 320 || \
       regs->a7 == 64
    //    regs->a7 == 320 ||
    //    regs->a7 == 323
       ){
        // don't print getrusage and clock gettime
        return;
    }
    printk("core %d pid %d entering syscall: %s\n", \
                current->core_id, \
                current->pid, \
                syscall_name[regs->a7]);
    // pcb_t * pcb = current_running;//(pcb_t *)regs->regs[4];
    // printk("process name: %s, pid:%d\n", pcb->name, pcb->pid);
    //printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",regs->sstatus, regs->sbadaddr, regs->scause);
    // printk("stval: 0x%lx cause: %lx\n\r",stval, cause);
    // printk("sepc: 0x%lx\n\r", regs->sepc);
    //printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    // uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    // printk("[Backtrace]\n\r");
    // printk("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    // while (fp > USER_STACK_ADDR - PAGE_SIZE) {
    //     uintptr_t prev_ra = *(uintptr_t*)(fp-8);
    //     uintptr_t prev_fp = *(uintptr_t*)(fp-16);
    //     if(prev_ra > 0x100000){
    //         break;
    //     }
    //     if(prev_fp < 0x10000){
    //         break;
    //     }
    //     printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);
    //     fp = prev_fp;
        
    // }
    // printk("==============debug-info end================\n\r");
}

char *syscall_array[] = {
    "SYS_getcwd      ",     
    "SYS_dup         ",      
    "SYS_dup3        ",     
    "SYS_mkdirat     ",      
    "SYS_unlinkat    ",       
    "SYS_linkat      ",       
    "SYS_umount2     ",       
    "SYS_mount       ",       
    "SYS_chdir       ",       
    "SYS_openat      ",      
    "SYS_close       ",       
    "SYS_pipe2       ",         
    "SYS_getdents64  ",       
    "SYS_read        ",      
    "SYS_write       ",       
    "SYS_fstat       ",     
    "SYS_exit        ",      
    "SYS_nanosleep   ",      
    "SYS_sched_yield ",      
    "SYS_times       ",      
    "SYS_uname       ",      
    "SYS_gettimeofday",      
    "SYS_getpid      ",     
    "SYS_getppid     ",      
    "SYS_brk         ",      
    "SYS_munmap      ",      
    "SYS_clone       ",       
    "SYS_execve      ",      
    "SYS_mmap        ",       
    "SYS_wait4       ",      
    "SYS_exec          ",      
    "SYS_screen_write  ",      
    "SYS_screen_reflush",      
    "SYS_screen_clear  ",      
    "SYS_get_timer     ",      
    "SYS_get_tick      ",      
    "SYS_shm_pageget   ",      
    "SYS_shm_pagedt    ",      
    "SYS_disk_read     ",      
    "SYS_disk_write    ",      
    "SYS_fcntl             ",  
    "SYS_ioctl             ",  
    "SYS_statfs            ",  
    "SYS_fstatfs           ",  
    "SYS_lseek             ",  
    "SYS_readv             ",  
    "SYS_writev            ",  
    "SYS_pread             ",  
    "SYS_pwrite            ",  
    "SYS_ppoll             ",  
    "SYS_fstatat           ",  
    "SYS_utimensat         ",  
    "SYS_exit_group        ",  
    "SYS_set_tid_address   ",  
    "SYS_futex             ",  
    "SYS_set_robust_list   ",  
    "SYS_get_robust_list   ",  
    "SYS_clock_gettime     ",  
    "SYS_sched_setscheduler",  
    "SYS_sched_getaffinity ",  
    "SYS_kill              ",  
    "SYS_tkill             ",  
    "SYS_rt_sigaction      ",  
    "SYS_rt_sigprocmask    ",  
    "SYS_rt_sigtimedwait   ",  
    "SYS_rt_sigreturn      ",  
    "SYS_setsid            ",  
    "SYS_getrlimit         ",  
    "SYS_setrlimit         ",  
    "SYS_geteuid           ",  
    "SYS_getegid           ",  
    "SYS_gettid            ",  
    "SYS_sysinfo           ",  
    "SYS_socket            ",  
    "SYS_bind              ",  
    "SYS_listen            ",  
    "SYS_accept            ",  
    "SYS_connect           ",  
    "SYS_getsockname       ",  
    "SYS_sendto            ",  
    "SYS_recvfrom          ",  
    "SYS_setsockopt        ",  
    "SYS_mremap            ",  
    "SYS_mprotect          ",  
    "SYS_madvise           ",  
    "SYS_prlimit           ",
    "SYS_preload           ",
    "SYS_membarrier        ",
    "SYS_faccessat         ",
    "SYS_sendfile          ",
    "SYS_pselect6          ",
    "SYS_readlinkat        ",
    "SYS_fsync             ",
    "SYS_setitimer         ",
    "SYS_syslog            ",
    "SYS_tgkill            ",
    "SYS_getrusage         ",
    "SYS_vm86              ",
    "SYS_getuid            ",
    "SYS_getgid            ",
    "SYS_msync             ",
    "SYS_renameat2         ",
    "SYS_exec              ",
    "SYS_sleep             ",
    "SYS_kill              ",
    "SYS_watpid            ",
    "SYS_ps                ",
    "SYS_getpid            ",
    "SYS_yield             ",
    "SYS_write             ",
    "SYS_readch            ",
    "SYS_flush             ",
    "SYS_timebase          ",
    "SYS_gettick           ",
    "SYS_getrandm          ",
    "SYS_unkown            ",
};

void init_syscall_name(){
    for (int i = 0; i < 512; i++)
    {
      syscall_name[i] =  syscall_array[115];
    }
    
    syscall_name[ 17] = syscall_array[0];     
    syscall_name[ 23] = syscall_array[1];      
    syscall_name[ 24] = syscall_array[2];     
    syscall_name[ 34] = syscall_array[3];      
    syscall_name[ 35] = syscall_array[4];       
    syscall_name[ 37] = syscall_array[5];       
    syscall_name[ 39] = syscall_array[6];       
    syscall_name[ 40] = syscall_array[7];       
    syscall_name[ 49] = syscall_array[8];       
    syscall_name[ 56] = syscall_array[9];      
    syscall_name[ 57] = syscall_array[10];       
    syscall_name[ 59] = syscall_array[11];         
    syscall_name[ 61] = syscall_array[12];       
    syscall_name[ 63] = syscall_array[13];      
    syscall_name[ 64] = syscall_array[14];       
    syscall_name[ 80] = syscall_array[15];     
    syscall_name[ 93] = syscall_array[16];      
    syscall_name[101] = syscall_array[17];      
    syscall_name[124] = syscall_array[18];      
    syscall_name[153] = syscall_array[19];      
    syscall_name[160] = syscall_array[20];      
    syscall_name[169] = syscall_array[21];      
    syscall_name[172] = syscall_array[22];     
    syscall_name[173] = syscall_array[23];      
    syscall_name[214] = syscall_array[24];      
    syscall_name[215] = syscall_array[25];      
    syscall_name[220] = syscall_array[26];       
    syscall_name[221] = syscall_array[27];      
    syscall_name[222] = syscall_array[28];       
    syscall_name[260] = syscall_array[29];      
    syscall_name[243] = syscall_array[30];      
    syscall_name[244] = syscall_array[31];      
    syscall_name[245] = syscall_array[32];      
    syscall_name[246] = syscall_array[33];      
    syscall_name[247] = syscall_array[34];      
    syscall_name[248] = syscall_array[35];      
    syscall_name[249] = syscall_array[36];      
    syscall_name[250] = syscall_array[37];      
    syscall_name[251] = syscall_array[38];      
    syscall_name[252] = syscall_array[39];      
    syscall_name[ 25] = syscall_array[40];  
    syscall_name[ 29] = syscall_array[41];  
    syscall_name[ 43] = syscall_array[42];  
    syscall_name[ 44] = syscall_array[43];  
    syscall_name[ 62] = syscall_array[44];  
    syscall_name[ 65] = syscall_array[45];  
    syscall_name[ 66] = syscall_array[46];  
    syscall_name[ 67] = syscall_array[47];  
    syscall_name[ 68] = syscall_array[48];  
    syscall_name[ 73] = syscall_array[49];  
    syscall_name[ 79] = syscall_array[50];  
    syscall_name[ 88] = syscall_array[51];  
    syscall_name[ 94] = syscall_array[52];  
    syscall_name[ 96] = syscall_array[53];  
    syscall_name[ 98] = syscall_array[54];  
    syscall_name[ 99] = syscall_array[55];  
    syscall_name[100] = syscall_array[56];  
    syscall_name[113] = syscall_array[57];  
    syscall_name[119] = syscall_array[58];  
    syscall_name[123] = syscall_array[59];  
    syscall_name[129] = syscall_array[60];  
    syscall_name[130] = syscall_array[61];  
    syscall_name[134] = syscall_array[62];  
    syscall_name[135] = syscall_array[63];  
    syscall_name[137] = syscall_array[64];  
    syscall_name[139] = syscall_array[65];  
    syscall_name[157] = syscall_array[66];  
    syscall_name[163] = syscall_array[67];  
    syscall_name[164] = syscall_array[68]; 
    syscall_name[175] = syscall_array[69];  
    syscall_name[177] = syscall_array[70];  
    syscall_name[178] = syscall_array[71];  
    syscall_name[179] = syscall_array[72];  
    syscall_name[198] = syscall_array[73];  
    syscall_name[200] = syscall_array[74];  
    syscall_name[201] = syscall_array[75];  
    syscall_name[202] = syscall_array[76];  
    syscall_name[203] = syscall_array[77];  
    syscall_name[204] = syscall_array[78];  
    syscall_name[206] = syscall_array[79];  
    syscall_name[207] = syscall_array[80];  
    syscall_name[208] = syscall_array[81];  
    syscall_name[216] = syscall_array[82];  
    syscall_name[226] = syscall_array[83];  
    syscall_name[233] = syscall_array[84];  
    syscall_name[261] = syscall_array[85];
    syscall_name[253] = syscall_array[86];
    syscall_name[283] = syscall_array[87];
    syscall_name[ 48] = syscall_array[88];
    syscall_name[ 71] = syscall_array[89];
    syscall_name[ 72] = syscall_array[90];
    syscall_name[ 78] = syscall_array[91];
    syscall_name[ 82] = syscall_array[92];
    syscall_name[103] = syscall_array[93];
    syscall_name[116] = syscall_array[94];
    syscall_name[131] = syscall_array[95];
    syscall_name[165] = syscall_array[96];
    syscall_name[166] = syscall_array[97];
    syscall_name[174] = syscall_array[98];
    syscall_name[176] = syscall_array[99];
    syscall_name[227] = syscall_array[100];
    syscall_name[276] = syscall_array[101];
    syscall_name[300] = syscall_array[102];
    syscall_name[302] = syscall_array[103];
    syscall_name[303] = syscall_array[104];
    syscall_name[304] = syscall_array[105];
    syscall_name[305] = syscall_array[106];    
    syscall_name[306] = syscall_array[107];
    syscall_name[307] = syscall_array[108];
    syscall_name[320] = syscall_array[109];
    syscall_name[321] = syscall_array[110];
    syscall_name[323] = syscall_array[111];
    syscall_name[330] = syscall_array[112];
    syscall_name[331] = syscall_array[113];
    syscall_name[278] = syscall_array[114];
}