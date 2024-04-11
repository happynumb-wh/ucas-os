#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <os/spinlock.h>
#include <os/string.h>
#include <os/elf.h>
#include <os/kdasics.h>
#include <fs/file.h>
#include <fs/fat32.h>
#include <os/futex.h>
#include <compile.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>
#include <csr.h>
#include <stdio.h>
#include <user_programs.h>
/**
 * @brief init the kernel stack and the user stack
 *  the switch context and the save context 
 * @param kernel_stack kernel stack
 * @param user_stack No preprocessing user stack
 * @param entry_point the entry
 * @param pcb the pcb todo
 * @param argc the argc
 * @param argv the argv
 * @param envp the environment
 * no return 
 * 
 */
void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb)
{
    // trap pointer
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    
    memset(pt_regs, 0, sizeof(regs_context_t));
    
    pcb->kernel_sp -= (sizeof(regs_context_t) + sizeof(switchto_context_t));
    
    // switch pointer
    switchto_context_t *switch_reg = (switchto_context_t *)( kernel_stack - 
                                                             sizeof(regs_context_t) -
                                                             sizeof(switchto_context_t)
                                                            );
    memset(switch_reg, 0, sizeof(switchto_context_t));     
    pcb->save_context = pt_regs;
    pcb->switch_context = switch_reg;

    // save all the things on the stack
    // pcb->user_sp = user_stack;

    // set context regs
    if(pcb->type == USER_PROCESS || pcb->type == USER_THREAD){
        pt_regs->sp = (reg_t) pcb->user_sp;         //SP  
        pt_regs->gp = (reg_t)__global_pointer$;  //GP
        pt_regs->tp = (reg_t) pcb;               //TP
        // use sscratch to placed tp
        pt_regs->sscratch = (reg_t) pcb;
        pt_regs->sepc = entry_point;                  //sepc
        pt_regs->sstatus = SR_SUM | SR_FS | SR_SPIE;        //sstatus

    }

    // set switch regs
    switch_reg->ra = (pcb->type == USER_PROCESS || pcb->type == USER_THREAD) ?
                                             (reg_t)&ret_from_exception : entry_point;                                                       //ra

    switch_reg->sp = pcb->kernel_sp; 

    pcb->pge_num = 0;

}

/**
 * @brief init the clone process pcb
 * @param pcb the todo pcb
 * @return no return
 */
void init_clone_stack(pcb_t *pcb, void * tls)
{   
    

    /* trap pointer */
    pcb->save_context = \
        (regs_context_t *)(pcb->kernel_stack_top - sizeof(regs_context_t));
    /* switch regs */
    pcb->switch_context = (switchto_context_t *)( pcb->kernel_stack_top - 
                                                    sizeof(regs_context_t) -
                                                    sizeof(switchto_context_t)
                                                );
    /* copy all kernel stack */
    memcpy((void *)pcb->kernel_stack_base, (const void *)current_running->kernel_stack_base, PAGE_SIZE);
    pcb->kernel_sp = (uintptr_t) (pcb->switch_context);
    
    // some change trap reg
    pcb->switch_context->ra = (reg_t)&ret_from_exception ;
    pcb->switch_context->sp = pcb->kernel_sp             ;

    pcb->save_context->sp   = pcb->user_stack_top == USER_STACK_ADDR ?  current_running->save_context->sp \
                                        : pcb->user_stack_top;
    // the tp may not be the pcb
    if (pcb->flag & CLONE_SETTLS)
        pcb->save_context->tp = (reg_t)tls               ;
    pcb->save_context->a0  = 0                          ; // child return zero
    pcb->save_context->sscratch  = (reg_t)(pcb)               ; // sscratch save the pcb address

    init_list(&pcb->wait_list);
}


extern char * dll_linker;
/**
 * @brief load process into memory
 * @name the process name
 */
uintptr_t load_process_memory(const char * path, pcb_t * initpcb)
{
    // #ifdef k210
    int dynamic = 0;
    uint64_t length = 0;     
    int elf_id = get_file_from_kernel(path);

    ptr_t entry_point = 0;

    if (elf_id == -1)
    {
        int fd;
        if((fd = fat32_openat(AT_FDCWD, path, O_RDONLY, 0)) <= 0) {
            printk("failed to open %s\n", path);
            return -ENOENT;
        }



        entry_point = fat32_load_elf(         fd, 
                                        initpcb, 
                                        &length,
                                        &dynamic
                                    ); 
    } else {        
        ElfFile * elf = &elf_files[elf_id];

        entry_point = load_elf(
                                elf, \
                                initpcb,
                                &length,
                                &dynamic
                            );        
    }


    if(entry_point == 0)
        return -ENOEXEC;


    return entry_point;
}

/**
 * @brief init the pcb member
 * @param initpcb the pcb todo
 * @param type the task type for user or kernel
 * @param mode the mode to exit
 * no return
 */
void init_pcb_member(pcb_t * initpcb,  task_type_t type, spawn_mode_t mode)
{
    initpcb->pre_time = get_ticks();
    // id lock
    initpcb->pid = process_id++;

    initpcb->dasics_flag = 0;
    initpcb->used = 1;
    initpcb->status = TASK_READY;
    initpcb->type = type;
    initpcb->mode = mode;
    initpcb->flag = 0;
    initpcb->dasics = 0;
    initpcb->parent.parent = current_running;
    initpcb->user_sp = initpcb->user_stack_top;
    initpcb->kernel_sp = initpcb->kernel_stack_top;
    initpcb->kernel_stack_base = initpcb->kernel_stack_top - PAGE_SIZE;
    initpcb->user_stack_base = initpcb->user_stack_top - PAGE_SIZE;
    initpcb->execve = 0;
    clear_pcb_time(initpcb);
    initpcb->fd_limit.rlim_cur = MAX_FD_NUM;
    initpcb->fd_limit.rlim_max = MAX_FD_NUM;
    mylimit.rlim_cur = MAX_FD_NUM;
    mylimit.rlim_max = MAX_FD_NUM;
    init_list(&initpcb->wait_list);
    
    // fd table
    initpcb->pfd = alloc_fd_table();
    init_fd(initpcb->pfd);
    // set signal
    initpcb->sigactions = alloc_sig_table();
    memset(&initpcb->mm, 0, sizeof(mm_struct_t));
    initpcb->is_handle_signal = 0;
    initpcb->sig_recv = 0;
    initpcb->sig_pend = 0;
    initpcb->sig_mask = 0;    
    //itimer
    initpcb->itimer.add_tick = 0;

    // init excellent load
    #ifdef FAST
        if (!strcmp(initpcb->name, "shell"))
            return;
        int exe_load_id = find_name_elf(initpcb->name);
        if (exe_load_id == -1) 
            printk("[Error] init execve pcb not find the %s\n", initpcb->name);
        else initpcb->exe_load = &pre_elf[exe_load_id];
    #endif
}

/**
 * @brief init the clone pcb
 * @param initpcb the pcb todo
 * @param type the task type for user or kernel
 * @param mode the mode to exit
 */
void init_clone_pcb(pcb_t * initpcb, task_type_t type, spawn_mode_t mode)
{
    

    initpcb->pre_time = get_ticks();
    // id lock
    initpcb->pid = process_id;
    initpcb->parent.parent = current_running;
    initpcb->ppid = current_running->pid;
    initpcb->tid = process_id++;
    initpcb->execve = 0;
    initpcb->dasics_flag = current_running->dasics_flag;

    initpcb->used = 1;
    initpcb->status = TASK_READY;
    initpcb->type = type;
    initpcb->mode = mode;
    // initpcb->flag = 0;
    initpcb->user_sp = initpcb->user_stack_top;
    initpcb->kernel_sp = initpcb->kernel_stack_top;
    initpcb->kernel_stack_base = initpcb->kernel_stack_top - PAGE_SIZE;
    initpcb->user_stack_base = initpcb->user_stack_top - PAGE_SIZE;
    clear_pcb_time(initpcb);
    init_list(&initpcb->wait_list);
    memcpy(&initpcb->fd_limit,&current_running->fd_limit,sizeof(struct rlimit));
    memcpy(&initpcb->mm, &current_running->mm, sizeof(mm_struct_t));
    //itimer
    initpcb->itimer.add_tick = 0;
    // clone fd init
    if (initpcb->flag & CLONE_FILES) {
        initpcb->pfd = current_running->pfd;
        pfd_table_t* pt =  list_entry(current_running->pfd, pfd_table_t, pfd);
        pt->num++;
    }else{
        initpcb->pfd = alloc_fd_table();
        // init_fd(initpcb->pfd);
        for (int i = 0; i < MAX_FD_NUM; i++)
        {
            /* code */
            copy_fd(&initpcb->pfd[i], &current_running->pfd[i]);            
        }
    }    

    // copy elf
    memcpy(&initpcb->elf, &current_running->elf, sizeof(ELF_info_t));
    initpcb->edata = current_running->edata;
    // set signal
    if (initpcb->flag & CLONE_SIGHAND) {
        initpcb->sigactions = current_running->sigactions;
        sigaction_table_t* st =  list_entry(current_running->sigactions, sigaction_table_t, sigactions);
        st->num++;
    }else{
        initpcb->sigactions = alloc_sig_table();
        memcpy(initpcb->sigactions,current_running->sigactions,NUM_SIG * sizeof(sigaction_t));
    } 
    //initpcb->sigactions = alloc_sig_table();
    initpcb->is_handle_signal = 0;
    initpcb->sig_recv = 0;
    initpcb->sig_pend = 0;
    initpcb->sig_mask = 0;   
    // init excellent load
    #ifdef FAST
        initpcb->exe_load = current_running->exe_load;
    #endif
    
}

/**
 * @brief init the execve pcb
 * @param initpcb the pcb todo
 * @param type the task type for user or kernel
 * @param mode the mode to exit
 */
void init_execve_pcb(pcb_t * initpcb, task_type_t type, spawn_mode_t mode)
{
    initpcb->status = TASK_READY;
    initpcb->dasics_flag = 0;
    initpcb->type = type;
    if (initpcb->mode == AUTO_CLEANUP_ON_EXIT)
        initpcb->mode = mode;
    /* stack message */
    initpcb->user_sp = initpcb->user_stack_top;
    initpcb->kernel_sp = initpcb->kernel_stack_top;
    initpcb->kernel_stack_base = initpcb->kernel_stack_top - PAGE_SIZE;
    initpcb->user_stack_base = initpcb->user_stack_top - PAGE_SIZE;
    clear_pcb_time(initpcb);   
    memset(&initpcb->mm, 0, sizeof(mm_struct_t));    
    // set signal
    //memset(initpcb->sigactions, 0, sizeof(sigaction_t) * NUM_SIG);
    initpcb->execve = 1;
    initpcb->is_handle_signal = 0;
    initpcb->sig_recv = 0;
    initpcb->sig_pend = 0;
    initpcb->sig_mask = 0;   
    // set signal lock
    // init fd
    //init_fd(initpcb->pfd);
    // init excellent load
    #ifdef FAST
        int exe_load_id = find_name_elf(initpcb->name);
        if (exe_load_id == -1) 
            printk("[Error] init execve pcb not find the %s\n", initpcb->name);
        else initpcb->exe_load = &pre_elf[exe_load_id];
    #endif
}

/**
 * @brief handle pipe and redirect in exec
 * @param initpcb the init pcb
 * @param argc the argc
 * @param argv the argv
 * @return return the true argc num
 */
int handle_exec_pipe_redirect(pcb_t *initpcb, pid_t *pipe_pid, int argc, const char* argv[])
{
    
    int mid_argc;
    for (mid_argc = 0; mid_argc < argc && strcmp(argv[mid_argc],"|"); mid_argc++);
    int redirect1 = !strcmp(argv[mid_argc - 2], ">");
    int redirect2 = !strcmp(argv[mid_argc - 2], ">>");
    int true_argc = (redirect1 | redirect2) ? mid_argc - 2 : mid_argc;
    // argv[true_argc] = NULL;
    if (mid_argc != argc) {
        assert(argv[mid_argc+1][0] == '.' && argv[mid_argc+1][1] == '/');
        argv[mid_argc+1] += 2;
        // the pipe_pid
        *pipe_pid = do_exec(argv[mid_argc+1],argc-(mid_argc+1),&argv[mid_argc+1],AUTO_CLEANUP_ON_EXIT);
        pcb_t *targetpcb;
        // ============ could be deleted because of the get_free_pcb will get one free always
        int i;
        for (i=0;i<NUM_MAX_TASK;i++){
            targetpcb = &pcb[i];
            if (targetpcb->pid == *pipe_pid) break;
        }
        if (i==NUM_MAX_TASK) return -ENOMEM;
        // ===============
        // alloc one pipe
        uint32_t pipenum = alloc_one_pipe();
        if ((int)pipenum == -ENFILE) return -ENFILE;
        // init pipe
        targetpcb->pfd[0].dev = DEV_PIPE;
        targetpcb->pfd[0].pip_num = pipenum;
        targetpcb->pfd[0].is_pipe_read = 1;
        targetpcb->pfd[0].is_pipe_write = 0;        
        initpcb->pfd[1].dev = DEV_PIPE;
        initpcb->pfd[1].pip_num = pipenum;
        initpcb->pfd[1].is_pipe_read = 0;
        initpcb->pfd[1].is_pipe_write = 1;        
    }
    // TODO
    if (redirect1 | redirect2)
    {
        // the redirect file name
        const char *redirect_file = argv[mid_argc - 1];
        int fd = 0;
        fat32_close(1); //close STDOUT
        if(redirect1 && ((fd = fat32_openat(AT_FDCWD,redirect_file,O_WRONLY,0)) != -ENOENT)){
            fat32_close(fd);
            fat32_unlink(AT_FDCWD,redirect_file,0);
        }
        fd = fat32_openat(AT_FDCWD,redirect_file,O_WRONLY | O_CREATE,0);
        copy_fd(&initpcb->pfd[1],&current_running->pfd[get_fd_index(fd,current_running)]);
        initpcb->pfd[1].fd_num = 1;
        fat32_close(fd);
        if (redirect2) { // lseek
            fd_t *nfd = &initpcb->pfd[1];
            // if(nfd->share_fd != 0 && nfd->share_fd->version > nfd->version){
            //     memcpy(nfd, nfd->share_fd, sizeof(fd_t));
            // }   
            nfd->pos = nfd->length;
            // nfd->version++;
            // memcpy(nfd->share_fd, nfd, sizeof(fd_t));
        }
    }    
    return true_argc;
}


/**
 * @brief set the argc and argv in the user stack 
 * @param argc the num of argv
 * @param argv the argv array
 * @param init_pcb the pcb 
 */
void set_argc_argv(int argc, char * argv[], pcb_t *init_pcb){
    /* get the kernel addr */
    uintptr_t kustack_top = get_kva_of(init_pcb->user_stack_top - PAGE_SIZE, init_pcb->pgdir) + PAGE_SIZE;
    uintptr_t new_argv = init_pcb->user_stack_top - 0xc0;
    // uintptr_t new_argv_pointer = init_pcb->user_stack_top - 0x100;
    uintptr_t kargv = kustack_top - 0xc0;//get_kva_of(new_argv, pcb[pid].pgdir);
    uintptr_t kargv_pointer = kustack_top - 0x100;//get_kva_of(new_argv_pointer, pcb[pid].pgdir);
    *((uintptr_t *)kargv_pointer) = argc;
    for (int j = 1; j < argc; j++){ 
        *((uintptr_t *)kargv_pointer + j) = (uint64_t)new_argv;
        strcpy((char *)kargv , argv[j]);
        new_argv = new_argv + strlen(argv[j]) + 1;
        kargv = kargv + strlen(argv[j]) + 1;
    }     
}

/**
 * @brief set the std argc argv for std riscv
 * @param argc the num of argv
 * @param argv the argv array
 * @param init_pcb the pcb 
 */
void set_argc_argv_std(int argc, char * argv[], pcb_t *init_pcb)
{
    /* get the kernel addr */
    uintptr_t kustack_top = get_kva_of(init_pcb->user_stack_top - PAGE_SIZE, init_pcb->pgdir) + PAGE_SIZE;
    uintptr_t new_argv = init_pcb->user_stack_top - 0xc0;
    // uintptr_t new_argv_pointer = init_pcb->user_stack_top - 0x100;
    uintptr_t kargv = kustack_top - 0xc0;//get_kva_of(new_argv, pcb[pid].pgdir);
    uintptr_t kargv_pointer = kustack_top - 0x100;//get_kva_of(new_argv_pointer, pcb[pid].pgdir);
    for (int j = 0; j < argc; j++){ 
        *((uintptr_t *)kargv_pointer + j) = (uint64_t)new_argv;
        strcpy((char *)kargv , argv[j]);
        new_argv = new_argv + strlen(argv[j]) + 1;
        kargv = kargv + strlen(argv[j]) + 1;
    }            
}

/**
 * @brief get argc num from argv
 * @param argv the argv array
 * @return return the argc num
 */
uint64_t get_num_from_parm(const char *parm[])
{
    uint64_t num = 0;
    if (parm)
        while (parm[num]) num++;
    return num;
}
/**
 * @brief copy on write for clone 
 * 
 */
void copy_on_write(PTE src_pgdir, PTE dst_pgdir)
{
    PTE *kernelBase = (PTE *)pa2kva(PGDIR_PA);    
    PTE *src_base = (PTE *)src_pgdir;
    PTE *dst_base = (PTE *)dst_pgdir;
    for (int vpn2 = 0; vpn2 < PTE_NUM; vpn2++)
    {
        /* kernel space or null*/
        if (kernelBase[vpn2] || !src_base[vpn2]) 
            continue;
        PTE *second_page_src = (PTE *)pa2kva((get_pfn(src_base[vpn2]) << NORMAL_PAGE_SHIFT));
        PTE *second_page_dst = (PTE *)check_page_set_flag(dst_base , vpn2, _PAGE_PRESENT, 1);
        /* we will let the user be three levels page */ 
        for (int vpn1 = 0; vpn1 < PTE_NUM; vpn1++)
        {
            if (!second_page_src[vpn1]) 
            {
                // printk(" [Copy Error], second pgdir is null ?\n");
                continue;
            }
               
            PTE *third_page_src = (PTE *)pa2kva((get_pfn(second_page_src[vpn1]) << NORMAL_PAGE_SHIFT));
            PTE *third_page_dst = (PTE *)check_page_set_flag(second_page_dst , vpn1, _PAGE_PRESENT, 1);
            for (int vpn0 = 0; vpn0 < PTE_NUM; vpn0++)
            {
                if (!third_page_src[vpn0] || third_page_dst[vpn0]) 
                {
                    // printk(" [Copy Error], third pgdir is null ?\n");
                    continue;                    
                }
                // ================= debug ====================
                // uint64_t vaddr = ((uint64_t)vpn0 << 12) | 
                //                  ((uint64_t)vpn1 << 21) |
                //                  ((uint64_t)vpn2 << 30);     
                // printk("fork vaddr: %lx\n", vaddr); 
                // ================= debug ====================                           
                /* page base */
                uint64_t share_base = pa2kva((get_pfn(third_page_src[vpn0]) << NORMAL_PAGE_SHIFT));
                /* close the W */
                uint64_t pte_flags = get_attribute(third_page_src[vpn0]) & (~_PAGE_WRITE);
                set_attribute(&third_page_src[vpn0], pte_flags);
                /* child */
                third_page_dst[vpn0] = third_page_src[vpn0];

                /* one more share the page */
                share_add(share_base);     
            }
        }        
    } 
}

/**
 * @brief handle the eixt pcb
 * @param exitPCB the exit pcb
 * @return no return
 * 
 */
void handle_exit_pcb(pcb_t * exitPcb)
{
    release_wait_deal_son(exitPcb);

    #ifdef FAST
        int recycle_page_num;
        if (!(exitPcb->flag & CLONE_VM)) 
            recycle_page_num = recycle_page_part(exitPcb->pgdir, exitPcb->exe_load);
    #else
        int __maybe_unused recycle_page_num;
        if (!(exitPcb->flag & CLONE_VM)) 
            recycle_page_num = recycle_page_default(exitPcb->pgdir);
    #endif
    recycle_pcb_default(exitPcb);
    freePage(exitPcb->kernel_stack_base);    
}


/**
 * @brief release all son and find new father
 * @param todoPcb the pcb be dealed
 * @return no return;
 */
void release_wait_deal_son(pcb_t * exitPcb)
{
    pcb_t *wait_entry = NULL, *wait_q;
    list_for_each_entry_safe(wait_entry, wait_q, &exitPcb->wait_list, list)
    {
        if (wait_entry->status == TASK_BLOCKED) 
        {
            do_unblock(&wait_entry->list);
        }
    }
    /* me will be the son's father */
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].parent.parent == exitPcb && pcb[i].used) 
        {
        if (pcb[i].status == TASK_ZOMBIE)
        {
            handle_exit_pcb(&pcb[i]);    
            return;
        }
            // find father
            pcb[i].parent.parent = exitPcb->parent.parent;
        }
    }                              
}

/**
 * @brief recycle the PCB
 * @return Succeed return 1;
 * Failed return -1;
 */
int recycle_pcb_default(pcb_t * recyclePCB){
    // free the pcb
    free_pcb(recyclePCB);

    /* do */
    recyclePCB->pge_num = 0;

    // free fd
    free_fd_table(recyclePCB->pfd);
    // free sig
    free_sig_table(recyclePCB->sigactions);
    // itimer
    if(recyclePCB->itimer.add_tick != 0){
        recyclePCB->itimer.add_tick = 0;
        list_del(&recyclePCB->itimer.list);
        memset(&recyclePCB->itimer, 0, sizeof(timer_t));
    }
    
    for (int i = 0; i < MAX_FUTEX_NUM; i++)
        if (futex_node_used[i] == recyclePCB->pid) {
            list_del(&(futex_node[i].list));
            futex_node_used[i] = 0;
        }
    // freePage(recyclePCB->kernel_stack_top - NORMAL_PAGE_SIZE);
    return 0;
}

/**
 * @brief find one free pcb for the handler 
 * @return Success return the pcb sub failed return -1
 * 
 */
pcb_t* get_free_pcb(){
    list_head *freePcbList = &available_queue.free_source_list;
    
try:    if (!list_empty(freePcbList)){
        list_node_t* new_pcb = freePcbList->next;
        pcb_t *new = list_entry(new_pcb, pcb_t, list);
        /* one use */
        list_del(new_pcb);
        // list_add_tail(new_pcb, &used_queue);

        // printk("alloc mem kva 0x%lx\n", new->kva_begin);
        return new;        
    }else{
        // memCurr += PAGE_SIZE;    
        printk("Failed to allocPcb, I am pid: %d\n", current_running->pid);
        do_block(&current_running->list, &available_queue.wait_list);
        goto try;
    }
    return NULL;
}

/**
 * @brief add the pcb to the available_queue
 * @param RecyclePcb the recycle pcb
 * @return no return
 */
void free_pcb(pcb_t *RecyclePcb)
{
    RecyclePcb->used = 0;
    RecyclePcb->status = TASK_EXITED;
    RecyclePcb->tid = 0;
    RecyclePcb->clear_ctid = 0;
    // get the freePcbList
    list_head *freePcbList = &available_queue.free_source_list;
    if (list_empty(&RecyclePcb->list))    
        list_del(&RecyclePcb->list);
    list_add(&RecyclePcb->list, freePcbList);
    // we only free one
    if (!list_empty(&available_queue.wait_list))
        do_unblock(available_queue.wait_list.next);
    return;
}

pcb_t * get_pcb_by_pid(pid_t pid)
{
    pcb_t *target =  NULL;
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].pid == pid && pcb[i].status != TASK_EXITED)
        {
            target = &pcb[i];
            break;
        }
    }
    
    return target;
}

/* init dasics */
void init_dasics_reg(pcb_t *initpcb)
{
    if (!initpcb->dasics) return;

    initpcb->bounds_table = NULL;
    initpcb->available_handle = 0;
    for (int i = 0; i < DASICS_LIBCFG_WIDTH; i++)
    {
        initpcb->dlibcfg_handle_map[i] = -1;        
    }
    


    #define align8up(addr) 		 ((addr+0x7) & ~(0x7)) 
    #define align8down(addr) 	 (addr & ~(0x7))

    int dasics_jmpidx = 0;
    int dasics_libidx = 0;

    /* clear dasics csrs */
    pr_regs(initpcb)->dasicsUmainCfg = 0;
    pr_regs(initpcb)->dasicsLibCfg0 = 0;
    pr_regs(initpcb)->dasicsJmpCfg = 0;

#ifdef DASICS_DEBUG
    printk("[kernel]: start_code: 0x%lx, end_code: 0x%lx\n", pr_mm(initpcb).start_code, pr_mm(initpcb).end_code);
    printk("[kernel]: start_data: 0x%lx, end_data: 0x%lx\n", pr_mm(initpcb).start_data, pr_mm(initpcb).end_data);
    printk("[kernel]: elf_bss: 0x%lx, elf_brk: 0x%lx\n", pr_mm(initpcb).elf_bss, pr_mm(initpcb).elf_brk);
    printk("[kernel]: text start: 0x%lx, end: 0x%lx\n", pr_mm(initpcb).text_start, pr_mm(initpcb).text_end);
#endif

    /* if .ulibtext exists, set dasics user main boundary registers. */
    /* lib function text */
    if (pr_mm(initpcb).free_sec_exit)
    {
    #ifdef DASICS_DEBUG
        printk("[kernel]: ufreezonetext start: 0x%lx, end: 0x%lx\n", pr_mm(initpcb).ulibfreetext_start, pr_mm(initpcb).ulibfreetext_end);
    #endif
        pr_regs(initpcb)->dasicsJmpBounds[dasics_jmpidx * 2] = align8down(pr_mm(initpcb).ulibfreetext_start);
        pr_regs(initpcb)->dasicsJmpBounds[dasics_jmpidx * 2 + 1] = align8up(pr_mm(initpcb).ulibfreetext_end);

        pr_regs(initpcb)->dasicsJmpCfg |= (DASICS_JUMPCFG_V << (dasics_jmpidx++)*16);        
    }

    if (pr_mm(initpcb).ulib_sec_exist)
    {
    #ifdef DASICS_DEBUG
        printk("[kernel]: ulibtext start: 0x%lx, end: 0x%lx\n", pr_mm(initpcb).ulibtext_start, pr_mm(initpcb).ulibtext_end);
    #endif
        /* get read-only datas. */
        /* This area contains some other codes, however, lib text should not execute them. */
        pr_regs(initpcb)->dasicsLibBounds[dasics_libidx * 2] = align8down(pr_mm(initpcb).start_brk);
        pr_regs(initpcb)->dasicsLibBounds[dasics_libidx * 2 + 1] = align8up(pr_mm(initpcb).start_stack);    

        pr_regs(initpcb)->dasicsLibCfg0 |= ((DASICS_LIBCFG_V | DASICS_LIBCFG_R | DASICS_LIBCFG_W) << (4 * (dasics_libidx++)));

        /* protect data */
        /* currently protact heap\mmap\stack together */
        pr_regs(initpcb)->dasicsLibBounds[dasics_libidx * 2] = align8down(pr_mm(initpcb).ulibtext_end);  
        pr_regs(initpcb)->dasicsLibBounds[dasics_libidx * 2 + 1] = align8up(pr_mm(initpcb).start_data);

        pr_regs(initpcb)->dasicsLibCfg0 |= ((DASICS_LIBCFG_V | DASICS_LIBCFG_R) << (4 * (dasics_libidx++)));        
    }


    /* get main text */
    pr_regs(initpcb)->dasicsUmainCfg = DASICS_UCFG_ENA;
	pr_regs(initpcb)->dasicsUmainBoundLO = initpcb->dasics_flag == DASICS_DYNAMIC ?  \
                                            DASICS_LINKER_BASE : 
                                                align8down(pr_mm(initpcb).text_start);
	pr_regs(initpcb)->dasicsUmainBoundHI = align8up(pr_mm(initpcb).text_end);    

}
