#include <os/sched.h>
#include <os/kdasics.h>
#include <os/auxvec.h>
#include <os/mm.h>
#include <stdio.h>
#include <assert.h>

/**
 * @brief copy the argv, envp, and aux on the user stack.
 * @param pcb the pcb structure 
 * @param user_stack init user stack
 * @param argc num of argc
 * @param argv argv
 * @param envp environment
 * @param filename the filename
 */
static uintptr_t copy_thing_user_stack(pcb_t *init_pcb, ptr_t user_stack, int argc, \
            const char *argv[], const char *envp[], const char *filename)
{
    // the kva of the user_top
    uintptr_t kustack_top = get_kva_of(init_pcb->user_stack_top - PAGE_SIZE, init_pcb->pgdir) + PAGE_SIZE;
    // caculate the space needed
    int envp_num = (int)get_num_from_parm(envp);

    // printk("argc: %d, envp_num: %d\n", argc, envp_num);
    // for (int i = 0; i < argc; i++)
    // {
    //     /* code */
    //     printk("%s\n", argv[i]);
    // }
    // for (int i = 0; i < envp_num; i++)
    // {
    //     /* code */
    //     printk("%s\n", envp[i]);
    // }
    
    int tot_length = (argc + envp_num + 3) * sizeof(uintptr_t *) + sizeof(aux_elem_t) * (AUX_CNT + 1) + SIZE_RESTORE;
    
    int point_length = tot_length;
    // get the space of the argv
    for (int i = 0; i < argc; i++)
    {
        tot_length += (strlen(argv[i]) + 1);
    }
    // get the space of the envp
    for (int i = 0; i < envp_num; i++)
    {
        tot_length += (strlen(envp[i]) + 1);
    }
    
    tot_length += (strlen(filename) + 1);
    // for random
    tot_length += (0x10);
    tot_length = ROUND(tot_length, 0x100);

    // printk("tot_length: 0x%lx\n", tot_length);

    uintptr_t kargv_pointer = kustack_top - tot_length;

    intptr_t kargv = kargv_pointer + point_length;
    
    // printk("point length: 0x%lx, kargv_pointer: 0x%lx, kargv: 0x%lx\n",point_length, kargv_pointer, kargv);
    /* 1. save argc */
    *((uintptr_t *)kargv_pointer) = argc; kargv_pointer += sizeof(uintptr_t*);
    /* 2. save argv */
    uintptr_t new_argv = init_pcb->user_stack_top - tot_length + point_length;
    for (int j = 0; j < argc; j++){ 
        *((uintptr_t *)kargv_pointer + j) = (uint64_t)new_argv;
        strcpy((char *)kargv , argv[j]);
        new_argv = new_argv + strlen(argv[j]) + 1;
        kargv = kargv + strlen(argv[j]) + 1;
    }
    // kargv_pointer += sizeof(uintptr_t*) * argc;
    *((uintptr_t *)kargv_pointer + argc) = 0;

    kargv_pointer += (argc + 1) * sizeof(uintptr_t *);

    /* 3. save envp */
    for (int j = 0; j < envp_num; j++){ 
        *((uintptr_t *)kargv_pointer + j) = (uint64_t)new_argv;
        strcpy((char *)kargv , envp[j]);
        new_argv = new_argv + strlen(envp[j]) + 1;
        kargv = kargv + strlen(envp[j]) + 1;
    }
    *((uintptr_t *)kargv_pointer + envp_num) = 0;

    kargv_pointer += (envp_num + 1) * sizeof(uintptr_t *);

    /* 4. save the file name */
    strcpy((char *)kargv , filename);

    uintptr_t file_pointer = new_argv;
    new_argv = new_argv + strlen(filename) + 1;
    kargv = kargv + strlen(filename) + 1;

    /* 5. set aux */
    // aux_elem_t aux_vec[AUX_CNT];
    set_aux_vec((aux_elem_t *)kargv_pointer, &init_pcb->elf, file_pointer, new_argv);

    // printk("aux_vec: %lx \n", kargv_pointer);
    /* 6. copy aux on the user_stack */
    // memcpy(kargv_pointer, aux_vec, sizeof(aux_elem_t) * (AUX_CNT + 1));
    *((uintptr_t *)kargv_pointer + AUX_CNT + 1) = 0;

    /* 7. copy the restore on the user stack */
    memcpy((char *)(kustack_top - SIZE_RESTORE), __restore, SIZE_RESTORE);
    /* 8. return user_stack */
    return init_pcb->user_stack_top - tot_length;
    
}

// Use some macro
#define STACK_ADD(sp, items) ((elf_addr_t __user *)(sp) - (items))
#define STACK_ROUND(sp, items) \
	(((unsigned long) (sp - items)) &~ 15UL)
#define STACK_ALLOC(sp, len) ({ sp -= len ; sp; })

static uintptr_t
create_elf_tables(pcb_t * initpcb, int argc, const char * argv[], const char * envp[])
{

    ELF_info_t *elf = &initpcb->elf;
    // the kva of the user_top
    uintptr_t kustack_top = get_kva_of(elf->ustack - PAGE_SIZE, elf->pgdir) \
                                 + PAGE_SIZE;

    uintptr_t ustack_top = elf->ustack;

    unsigned char k_rand_bytes[16];

    elf_addr_t * u_rand_bytes = NULL;

    ustack_top = STACK_ROUND(ustack_top, 0);
    kustack_top = STACK_ROUND(kustack_top, 0);

    int envp_num = get_num_from_parm(envp);

    // COPY envp str
    char *kenvp[MAX_PARAM_NUM] = {NULL};

    for (int i = envp_num - 1; i >= 0; i--)
    {
        int param_length = strlen(envp[i]);
        STACK_ALLOC(kustack_top, param_length + 1);
        STACK_ALLOC(ustack_top, param_length + 1);
        strcpy((void *)kustack_top, envp[i]);
        kenvp[i] = (void *)ustack_top;
    }


    // COPY envp str
    char *kargv[MAX_PARAM_NUM] = {NULL};

    for (int i = argc - 1; i >= 0; i--)
    {
        int param_length = strlen(argv[i]);
        STACK_ALLOC(kustack_top, param_length + 1);
        STACK_ALLOC(ustack_top, param_length + 1);
        strcpy((void *)kustack_top, argv[i]);
        kargv[i] = (void *)ustack_top;
    }   

    //random byte
    for (int i = 0; i < sizeof(k_rand_bytes); i++)
    {
        /* code */
        k_rand_bytes[i] = (unsigned char)krand();
    }

    // RANDOM
    STACK_ALLOC(kustack_top, sizeof(k_rand_bytes));
    STACK_ALLOC(ustack_top, sizeof(k_rand_bytes));    

    u_rand_bytes = (elf_addr_t *)ustack_top;
    memcpy((void *)kustack_top, k_rand_bytes, sizeof(k_rand_bytes));


    kustack_top = STACK_ROUND(kustack_top, 0);
    ustack_top = STACK_ROUND(ustack_top, 0);

    elf_addr_t *elf_info = (elf_addr_t *)kustack_top;

    // COPY aux
 #define NEW_AUX_ENT(id, val) \
	do { \
        STACK_ALLOC(kustack_top, sizeof(aux_elem_t)); \
        STACK_ALLOC(ustack_top, sizeof(aux_elem_t)); \
        elf_info = (elf_addr_t *)kustack_top; \
		elf_info[0] = id; \
		elf_info[1] = val; \
	} while (0)   

    NEW_AUX_ENT(0, 0);     
    NEW_AUX_ENT(0x28, 0);
    NEW_AUX_ENT(0x29, 0);
    NEW_AUX_ENT(0x2a, 0);
    NEW_AUX_ENT(0x2b, 0);
    NEW_AUX_ENT(0x2c, 0);
    NEW_AUX_ENT(0x2d, 0);

    // NEW_AUX_ENT(AT_SYSINFO_EHDR, 0x3fc2dee000);     // 0x21
    NEW_AUX_ENT(AT_HWCAP, 0x112d);                  // 0x10
    NEW_AUX_ENT(AT_PAGESZ, PAGE_SIZE);              // 0x06
    NEW_AUX_ENT(AT_CLKTCK, 0x64);                   // 0x11
    NEW_AUX_ENT(AT_PHDR, elf->text_begin + elf->phoff);               // 0x03
    NEW_AUX_ENT(AT_PHENT, elf->phent);              // 0x04
    NEW_AUX_ENT(AT_PHNUM, elf->phnum);              // 0x05
    NEW_AUX_ENT(AT_BASE, DYNAMIC_VADDR_PFFSET);     // 0x07                       // 0x07
    NEW_AUX_ENT(AT_FLAGS, 0);                       // 0x08
    NEW_AUX_ENT(AT_ENTRY, elf->entry);              // 0x09
    NEW_AUX_ENT(AT_UID, 0);                         // 0x0b
    NEW_AUX_ENT(AT_EUID, 0);                        // 0x0c
    NEW_AUX_ENT(AT_GID, 0);                         // 0x0d
    NEW_AUX_ENT(AT_EGID, 0);                        // 0x0e
    NEW_AUX_ENT(AT_SECURE, 0);                      // 0x17
    NEW_AUX_ENT(AT_RANDOM, (uint64_t)u_rand_bytes);             // 0x19
    // NEW_AUX_ENT(AT_EXECFN, 3);           // 0x1f
   

// #ifdef DASICS
    NEW_AUX_ENT(AT_LINKER, elf->interp_load_entry);
    NEW_AUX_ENT(AT_FIXUP, 0);
    if (initpcb->dasics_flag)
    {
        NEW_AUX_ENT(AT_DASICS, 1);
    } else 
    {
        NEW_AUX_ENT(AT_DASICS, 0);
    }
    NEW_AUX_ENT(AT_TRUST_BASE, TRUST_LIB_BASE);
    NEW_AUX_ENT(AT_LINKER_COPY, elf->copy_interp_entry);
// #endif


    // ALLOC envp[]
    assert(envp_num + 1 <= MAX_PARAM_NUM);
    // ALLOC envp[]
    STACK_ALLOC(kustack_top, sizeof(void *) * (envp_num + 1));
    STACK_ALLOC(ustack_top, sizeof(void *) * (envp_num + 1));

    memcpy((void *)kustack_top, (const void *)kenvp, sizeof(void *) * (envp_num + 1));


    // ALLOC argv[]
    assert(argc + 1 <= MAX_PARAM_NUM);
    STACK_ALLOC(kustack_top, sizeof(void *) * (argc + 1));
    STACK_ALLOC(ustack_top, sizeof(void *) * (argc + 1));

    memcpy((void *)kustack_top, (const void *)kargv, sizeof(void *) * (argc + 1));

    // SAVE argc
    STACK_ALLOC(kustack_top, sizeof(void *));
    STACK_ALLOC(ustack_top, sizeof(void *));

    *((uint64_t * )kustack_top) = argc;

    return ustack_top;
}


/**
 * @brief exec one process, used by the shell
 * @return succeed return the new pid failed return -1
 */
pid_t do_exec(const char *file_name, int argc, const char* argv[], spawn_mode_t mode){
    pcb_t *initpcb = get_free_pcb();
    
    // init the pcb 
    initpcb->pgdir = allocPage() - PAGE_SIZE;   
    initpcb->kernel_stack_top = allocPage();         //a kernel virtual addr, has been mapped
    initpcb->kernel_sp = initpcb->kernel_stack_top;
    initpcb->user_stack_top = USER_STACK_ADDR;       //a user virtual addr, not mapped
    initpcb->elf.ustack = USER_STACK_ADDR;
    initpcb->elf.pgdir = initpcb->pgdir;
    share_pgtable(initpcb->pgdir, PGDIR_PA);
    /* map the user_stack, for NUM_MAX_USTACK*/
    for (int i = 0; i < NUM_MAX_USTACK; i++)
    {
        alloc_page_helper(initpcb->user_stack_top - (i + 1) * PAGE_SIZE, \
                                                         initpcb->pgdir, \
                                                         MAP_USER, \
                                                         _PAGE_READ | _PAGE_WRITE | _PAGE_ACCESSED | _PAGE_DIRTY
                         );
    }
    
    /* copy the name */
    strcpy(initpcb->name, file_name); 
    /* init pcb mem */
    init_pcb_member(initpcb, USER_PROCESS, AUTO_CLEANUP_ON_EXIT);

    if (!strcmp(argv[argc - 1], DASICS_COMMAND))
    {
        argc-=1;
        initpcb->dasics = 1;
        initpcb->dasics_flag = DASICS_STATIC;
    }

    initpcb->parent.parent = current_running;
    initpcb->ppid = initpcb->parent.parent->pid;
    initpcb->mask = 3;
    initpcb->priority = 3;
    // load process into memory
    ptr_t entry_point = load_process_memory(file_name, initpcb);


    if ((int64_t)entry_point <= 0)
    {
        printk("[Error] load %s to memory false\n", file_name);
        recycle_pcb_default(initpcb);
        return -1;
    }

    pid_t pipe_pid = 0;
    // handle for pipe and redirect
    // int true_argc = handle_exec_pipe_redirect(initpcb, &pipe_pid, argc, argv);
    int true_argc = argc;
    // copy all things on the user stack
    // initpcb->user_sp = copy_thing_user_stack(initpcb, initpcb->user_stack_top, true_argc,
    //                     argv, fixed_envp, file_name);

    initpcb->user_sp = create_elf_tables(initpcb, true_argc, argv, fixed_envp);
    // init all stack
    init_pcb_stack(initpcb->kernel_sp, initpcb->user_sp, entry_point, initpcb);
    // add to ready queue
    list_add(&initpcb->list, &ready_queue);

    init_dasics_reg(initpcb);

    if (strcmp("shell", file_name))
        do_scheduler();
        
    return pipe_pid ? pipe_pid : initpcb->pid;
}

/**
 * @brief execve one process by current pcb
 * @param file_name file name
 * @param argv argv
 * @param envp envp
 * @return succeed never return, failed retuen -1
 * 
 */
pid_t do_execve(const char* path, const char* argv[], const char * envp[]){
    // printk("enter exec\n");
    // printk("[execve] %s\n", path);
    pcb_t * initpcb =current_running;
    uintptr_t pre_pgdir = initpcb->pgdir;
    // a new pgdir
    initpcb->pgdir = allocPage() - PAGE_SIZE; 

    initpcb->user_stack_top = USER_STACK_ADDR;       //a user virtual addr, not mapped
    initpcb->elf.ustack = USER_STACK_ADDR;
    initpcb->kernel_sp =initpcb->kernel_stack_top;
    initpcb->elf.pgdir = initpcb->pgdir;

    share_pgtable(initpcb->pgdir, PGDIR_PA);
    
    /* map the user_stack */
    /* map the user_stack, for NUM_MAX_USTACK*/
    for (int i = 0; i < NUM_MAX_USTACK; i++)
    {
         alloc_page_helper(initpcb->user_stack_top - (i + 1) * PAGE_SIZE, \
                                                         initpcb->pgdir, \
                                                         MAP_USER, \
                                                         _PAGE_READ | _PAGE_WRITE | _PAGE_ACCESSED | _PAGE_DIRTY
                         );
    }
    // excellent_load_t *pre_exe_load = initpcb->exe_load;

    /* handle ./xxx */
    for (int i = 0; i < strlen(path); i++){
        if(path[i] == '/'){
            path = path + i + 1;
            break;
        }
    }
    
    /* copy the name */ 
    strcpy(initpcb->name, path); 

    /* init pcb mem */
    init_execve_pcb(initpcb, USER_PROCESS, AUTO_CLEANUP_ON_EXIT);
    
    initpcb->mask = 3;
    initpcb->priority = 3;
    // load process into memory
    ptr_t entry_point = load_process_memory(path, initpcb);

    if (entry_point <= 0)
    {
        printk("[Error] load %s to memory false\n", path);
        recycle_pcb_default(initpcb);
        return -1;
    }

    int argc = (int)get_num_from_parm(argv);

    if (!strcmp(argv[argc - 1], DASICS_COMMAND))
    {
        argc-=1;
        initpcb->dasics = 1;
        initpcb->dasics_flag = DASICS_STATIC;
    }

    // copya all things on the user stack
    initpcb->user_sp = copy_thing_user_stack(initpcb, initpcb->user_stack_top, argc, \
                        argv, envp, path);

    uint64_t save_core_id = get_current_cpu_id();
    // initialize process
    init_pcb_stack(initpcb->kernel_sp, initpcb->user_sp, entry_point, \
                    initpcb);
    initpcb->core_id = save_core_id;

    // add in the ready_queue
    list_add(&initpcb->list, &ready_queue);
    #ifdef FAST
        recycle_page_part(pre_pgdir, pre_exe_load);
    #else
        recycle_page_voilent(pre_pgdir);
    #endif

    // no signal
    do_scheduler();
    // this never return
    return initpcb->pid;    
}