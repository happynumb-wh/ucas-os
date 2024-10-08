#include <os/list.h>
#include <screen.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <os/spinlock.h>
#include <os/string.h>
#include <os/futex.h>
#include <os/kdasics.h>
#include <fs/fat32.h>
#include <compile.h>
#include <stdio.h>
#include <assert.h>
#include <user_programs.h>

pcb_t pcb[NUM_MAX_TASK];
pcb_t * current_running_master;
pcb_t * current_running_slave;
const ptr_t pid0_stack_master = INIT_KERNEL_STACK_MSTER; 
const ptr_t pid0_stack_slave = INIT_KERNEL_STACK_SLAVE; 


pcb_t pid0_pcb_master = {
    .pid = 0,
    .kernel_sp = INIT_KERNEL_STACK_MSTER,
    .user_sp = INIT_KERNEL_STACK_MSTER,
    .pgdir = 0,
    .preempt_count = 0,
    .name = "pid0_master",
    .save_context = (regs_context_t *)(INIT_KERNEL_STACK_MSTER - sizeof(regs_context_t)),
    .switch_context = NULL, 
    .status   = TASK_READY,
    .type = KERNEL_PROCESS
};  //master
pcb_t pid0_pcb_slave = {
    .pid = 0,
    .kernel_sp = INIT_KERNEL_STACK_SLAVE,
    .user_sp = INIT_KERNEL_STACK_SLAVE,
    .pgdir = 0,    
    .preempt_count = 0,
    .name = "pid0_slave",
    .save_context = NULL,
    .switch_context = NULL,
    .status   = TASK_READY,
    .type = KERNEL_PROCESS
};  //slave

LIST_HEAD(ready_queue);
LIST_HEAD(zombie_queue);
LIST_HEAD(used_queue)
// available pcb queue
source_manager_t available_queue;

/* current running task PCB */
/* global process id */
pid_t process_id = 1;


void do_scheduler(void)
{
    uint64_t cpu_id = get_current_cpu_id();
    pcb_t * prev_running = current_running;  
    int is_execve = 0;
    // if (list_empty(&ready_queue)) goto end;
    if (current_running->status == TASK_RUNNING && current_running->pid) {
        current_running->status = TASK_READY;
        list_add_tail(&current_running->list, &ready_queue);
    }

    if (!list_empty(&ready_queue)) { 
        current_running = list_entry(ready_queue.next, pcb_t, list);
        list_del(ready_queue.next);
        if (cpu_id)
            current_running_slave  = current_running;
        else 
            current_running_master = current_running;
    }else{
        if (current_running->status != TASK_RUNNING) {
            current_running = (cpu_id == 0) ? &pid0_pcb_master : &pid0_pcb_slave;
            if(cpu_id == 0)
                current_running_master = current_running;
            else
                current_running_slave  = current_running;
        }
    }
    // release the lock
    if(get_pgdir() == kva2pa(current_running->pgdir)) ;
    else{
        set_satp(SATP_MODE_SV39, current_running->pid, \
                (uint64_t)kva2pa((current_running->pgdir)) >> 12);
        local_flush_tlb_all();
    }
    // save the id
    current_running->core_id = prev_running->core_id;
    current_running->status = TASK_RUNNING;

    if (current_running->execve)
    {
        current_running->execve = 0;
        is_execve = 1;
    }
    if (is_execve || current_running != prev_running)
        switch_to(prev_running, current_running, is_execve);  
}


void do_priori(int priori, int pcb_id){
    if(pcb_id){
        pcb[pcb_id].priority = priori;
        pcb[pcb_id].status = TASK_READY;
        pcb[pcb_id].pre_time = get_ticks() ;
        list_add(&pcb[pcb_id].list, &ready_queue);
    }      
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    list_add(pcb_node,queue);
    current_running->status = TASK_BLOCKED;
    do_scheduler();
}

void do_unblock(list_node_t *pcb_node)
{
    list_del(pcb_node);
    list_entry(pcb_node , pcb_t, list)->status=TASK_READY;
    list_add(pcb_node, &ready_queue);
}


int get_file_from_kernel(const char * elf_name){
    int file_id;
    for (file_id = 0; file_id < ELF_FILE_NUM; file_id++){
        if (!strcmp(elf_name, elf_files[file_id].file_name)){
            break;
        }       
    }    
    if (file_id == ELF_FILE_NUM)
        return -1;
    return file_id;
}

void do_ps(void)
{
    printk("[Process Table]:");
    for (int i = 0; i < NUM_MAX_TASK; ++i)
    {
        if (pcb[i].status != TASK_EXITED)
        {
            printk("\n[%d] PID : %d  STATUS : %s", i, pcb[i].pid,
                        pcb[i].status == TASK_EXITED  ? "EXITED  " :
                        pcb[i].status == TASK_BLOCKED ? "BLOCKED " :
                        pcb[i].status == TASK_READY   ? "READY   " :
                        pcb[i].status == TASK_ZOMBIE  ? "ZOMBIE  " :
                        pcb[i].status == TASK_RUNNING ? "RUNNING " :
                                                        "UNKNOWN ");
            printk("mask: 0x%lx", pcb[i].mask);
            if (pcb[i].status==TASK_RUNNING)
                printk(" Running on core %d", &pcb[i] == current_running_master ? \
                        0 : 1);
        }
    }
    printk("\n");
}

/**
 * @brief wait the process if pid == -1, 
 * wait every child, else wait the pointed process
 * @param pid the wait pid
 * @param status the status when wnter exit
 * @param option not care now
 * @return true return the wait pid, else return -1
 */
uint64_t do_wait4(pid_t pid, uint16_t *status, int32_t options){
    
    pid_t ret = -1;
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].parent.parent == current_running && (pid == -1 || pid == pcb[i].pid)) {
            if (pcb[i].status != TASK_EXITED && pcb[i].status != TASK_ZOMBIE) {
                ret = pcb[i].pid;
                do_block(&current_running->list, &pcb[i].wait_list);
                /* set status */
                if(status){
                    (*((uint16_t *)status) = WEXITSTATUS(pcb[i].exit_status); 
                }
                // exit it 
                if (pcb[i].status == TASK_ZOMBIE)
                    handle_exit_pcb(&pcb[i]);
                // WEXITSTATUS(get_kva_of(status, current_running->pgdir), pcb[i].exit_status);
            } else if (pcb[i].status == TASK_ZOMBIE)
            {
                ret = pcb[i].pid;
                /* recycle pcb */
                // seat status
                if(status){
                    (*((uint16_t *)status) = WEXITSTATUS(pcb[i].exit_status);
                }
                // handle exit
                handle_exit_pcb(&pcb[i]);
            }  
        }
    }
    return ret;   
}

/**
 * @brief pcb exit myself we will recycle all the source now
 * for the moment
 * @param exit_status : exit status
 * @return no return 
 * 
 */
void do_exit(int32_t exit_status){
    
    pcb_t * exit_pcb;
    exit_pcb = current_running;
    // Change child's father
    pcb_t *child_qentry = NULL, *child_q;

    /* exit status */
    current_running->exit_status = exit_status;

    if (current_running->clear_ctid) {
        *(int *)(current_running->clear_ctid) = 0;
        do_futex(current_running->clear_ctid, FUTEX_WAKE, 1, NULL, NULL, 0);
    }
    
    if (current_running->pid == 1)
    {
        printk("[Error]: Init process exit\n");
        assert(0);
    }

    // Change father
    assert(exit_pcb->parent.parent != NULL);
    list_for_each_entry_safe(child_qentry, child_q, &exit_pcb->parent.head, parent.list)
    {
        list_del(&child_qentry->parent.list);
        list_add_tail(&child_qentry->parent.list, &exit_pcb->parent.parent->parent.head);
        child_qentry->parent.parent = exit_pcb->parent.parent;
    }
    

    if (current_running->mode == AUTO_CLEANUP_ON_EXIT) {
        /* free the wait */
        handle_exit_pcb(exit_pcb);
    } else if (current_running->type == USER_PROCESS && \
               current_running->mode == ENTER_ZOMBIE_ON_EXIT) {
        current_running->status = TASK_ZOMBIE;
        if (current_running->flag & SIGCHLD)
        {
            // current_running->exit_status = 0;
            send_signal(SIGCHLD, current_running->parent.parent);
        }        
    }
    /* scheduler */
    do_scheduler();  
}


void do_exit_group(int32_t exit_status){
    do_exit(exit_status);
}

/**
 * @brief clone one process,
 * succed return child's pid
 * failed return -1
 * ignore the flag, tls, ctid, pid
 */
pid_t do_clone(uint32_t flag, uint64_t stack, pid_t ptid, void *tls, pid_t ctid) 
{
    // printk("\t\tenter fork\n");
    
    pcb_t *initpcb = get_free_pcb();
    // printk("enter fork\n");
    /* alloc the pgdir */
    // for CONLE_VM, share the VM
    if (flag & CLONE_VM) initpcb->pgdir = current_running->pgdir;
    else
    {   
        initpcb->pgdir = allocPage() - PAGE_SIZE;
        /* copy kernel */
        share_pgtable(initpcb->pgdir, PGDIR_PA);         
    } 
    
    initpcb->kernel_stack_top = allocPage();         //a kernel virtual addr, has been mapped

    // if point usatck  
    initpcb->user_stack_top = stack ? stack : USER_STACK_ADDR;


    //init pfd_table
    initpcb->flag = flag;    

    // init pcb
    init_clone_pcb(initpcb, USER_PROCESS, ENTER_ZOMBIE_ON_EXIT);
    // don't need tmp
    // if (flag & CLONE_CHILD_SETTID) *(int *)ctid = initpcb->tid;
    // if (flag & CLONE_PARENT_SETTID) *(int *)ptid = initpcb->tid;
    // if (flag & CLONE_CHILD_CLEARTID) initpcb->clear_ctid = ctid;
    // for the qemu and k210, maybe we need to alloc the user stack first, 
    // don't care about the copy on write.
    // and we don't know whether it grows automatically
    // #ifndef k210
        uintptr_t ku_base;
        for (uintptr_t u_base = initpcb->user_stack_top - NORMAL_PAGE_SIZE; \
                (ku_base = get_kva_of(u_base, current_running->pgdir)) != 0; \
                u_base -= NORMAL_PAGE_SIZE)
        {
            /* code */
            uintptr_t cloneu_base = (uintptr_t)alloc_page_helper(u_base, \
                                                      initpcb->pgdir, \
                                                      MAP_USER, \
                                                      _PAGE_READ | _PAGE_WRITE
                                                     );
            memcpy((void *)cloneu_base, (const void *)ku_base, PAGE_SIZE);
        }
    // #endif    

    /* for COW clone */
    if ((flag & CLONE_VM) == 0)
        copy_on_write(current_running->pgdir, initpcb->pgdir);

    // the clone don't need new agv and envp, so we don't care
    // name
    strcpy(initpcb->name, current_running->name);
    strcat(initpcb->name, "_son");

    // init clone satck
    init_clone_stack(initpcb, tls);

    list_add(&initpcb->list, &ready_queue);
    list_add_tail(&initpcb->parent.list, &initpcb->parent.parent->parent.head);

    // printk("clone end\n");
    return initpcb->pid;
}

/* final competition */
int do_sched_setscheduler(pid_t pid, int policy, const struct sched_param *param){
    printk("do_sched_setscheduler to do.\n");
    return 0;
}

int do_sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask){
    printk("do_sched_getaffinity to do.\n");
    return 0;
}

int32_t do_kill(pid_t pid, int32_t sig){
    uint8_t ret = 0;
    
    for (uint32_t i = 0; i < NUM_MAX_TASK; i++){
        if ((pid > 0 && pcb[i].pid == pid) || (pid < -1 && pcb[i].pid == -pid)){
            send_signal(sig, &pcb[i]);
            ret++;
            break;
        }
        else if ((pid == 0) && !(current_running->status == TASK_EXITED)){
            send_signal(sig, current_running);
            ret++;
            break;
        }
        else if ((pid == -1) && !(pcb[i].status == TASK_EXITED)){
            send_signal(sig, &pcb[i]);
            ret++;
        }
    }
    return 0;
    if (ret > 0) return 0;
    else {
        printk("[kill] kill failed, no process found\n");
        return -ESRCH;
    }
}


int32_t do_tkill(int tid, int sig){
    return do_kill(tid, sig);
}


int32_t do_tgkill(int tgid, int tid, int sig){
    return do_kill(tid, sig);
}