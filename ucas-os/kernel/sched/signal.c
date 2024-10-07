#include <os/sched.h>
#include <os/signal.h>
#include <os/irq.h>
#include <assert.h>


regs_context_t signal_context_buf;
switchto_context_t signal_switch_buf;

sigaction_table_t sig_table[NUM_MAX_TASK];
/**
 * @brief used to fetch and/or change the signal mask of
 * the calling thread.  The signal mask is the set of signals whose
 * delivery is currently blocked for the caller.
 * @param how SIG_BLOCK: The set of blocked signals is the union of the current 
 * set and the set argument.
 * @param how SIG_UNBLOCK: The signals in set are removed from the current 
 * set of blocked signals.  It is permissible to attempt to 
 * unblock a signal which is not blocked.
 * @param how SIG_SETMASK: The set of blocked signals is set to the argument set.
 * @param set: is NULL, then the signal mask is unchanged
 * @param oldset: If oldset is non-NULL, the previous value of the 
 * signal mask is stored in oldset.
 * @return 
 */
int do_rt_sigprocmask(int32_t how, const sigset_t *restrict set, sigset_t *restrict oldset, size_t sigsetsize)
{
    // printk("sigmask entry\n");
    
    if (oldset)
        memcpy(&oldset->sig[0], &current_running->sig_mask, sizeof(sigset_t));
    // printk("sigmask return\n");
    if (!set)
        return oldset->sig[0];
    if (how == SIG_BLOCK)
        // add
        current_running->sig_mask |= set->sig[0];
    else if (how == SIG_UNBLOCK)
        // remove
        current_running->sig_mask &= ~(set->sig[0]);
    else if (how == SIG_SETMASK)
        // set
        current_running->sig_mask = set->sig[0];
    else
        assert(0);
    // printk("sigmask return\n");
    return 0;
}


// void set_sigaction_handler(pcb_t *setpcb, pcb_t *lastpcb, int32_t signum, struct sigaction *act)
// {

//     sigaction_t *this_sigaction = &setpcb->sigactions[signum - 1];
//     if (act) {
//         this_sigaction->handler = act->handler;
//         this_sigaction->flags = act->flags;
//         this_sigaction->restorer = act->restorer;
//         this_sigaction->sa_mask = act->sa_mask;
//     }
//     // handle shared handler process / thread
//     // when this function is first used (lastpcb == NULL), there are two directions for recursion.
//     // then, for the up direction, only when current pcb is sharable, it can continually go up.
//     // for the down direction, it can not go up due to lastpcb, so it will spread until the last son.

//     // go up on tree. if the last step is down, then it will not execute.
//     if ((setpcb->parent.parent != lastpcb) && (setpcb->parent.parent != &pcb[0]) && (setpcb->parent.parent->used) && (setpcb->flag & CLONE_SIGHAND)) 
//         set_sigaction_handler(setpcb->parent.parent,setpcb,signum,act); // don't need to copy to oldact

//     // go down on tree. in most of the condition, it can only go down
//     for (int i=0;i<NUM_MAX_TASK;i++)
//         if ((&pcb[i] != lastpcb) && (pcb[i].used) && (pcb[i].parent.parent == setpcb) && (pcb[i].flag & CLONE_SIGHAND)) 
//             set_sigaction_handler(&pcb[i],setpcb,signum,act); // don't need to copy to oldact
//     return;
// }



/**
 * @brief used to change the action taken by a process 
 * on receipt of a specific signal
 * @param signum the signal except SIGKILL and SIGSTOP.
 * @param act If not null, the action of the sign will be instll from the act
 * @param oldact If oldact is non-NULL, the previous action is saved in oldact.
 * @return return 0
 */
int do_rt_sigaction(int32_t signum, struct sigaction *act, 
                        struct sigaction *oldact, size_t sigsetsize)
{
    // printk("signum: %d, act handler:%lx\n", signum, act->handler);
    
    if (signum > NUM_SIG){
        printk("process %d: cannot set signum %d, too large\n", current_running->pid, signum);
        return 1;
    }
    sigaction_t *this_sigaction = &current_running->sigactions[signum - 1];
    if (oldact) memcpy(oldact, this_sigaction, sizeof(sigaction_t));
    // handle shared handler process / thread in a recursive way
    if (act) memcpy(this_sigaction, act, sizeof(sigaction_t));
    return 0;
}


/**
 * @brief return form the signal handle 
 * @return no return
 */
void do_rt_sigreturn()
{
    
    // u_cxt
    ucontext_t __maybe_unused *u_cxt = (ucontext_t *)(SIGNAL_HANDLER_ADDR - sizeof(ucontext_t));
    // recover form the user cancel
    change_signal_mask(current_running, current_running->prev_mask);
    // finished one signal
    current_running->is_handle_signal = 0;
    // offset
    uint64_t offset = (sizeof(regs_context_t) + sizeof(switchto_context_t));
    // resize kernel_sp
    uint64_t core_id = current_running->core_id;
    current_running->kernel_sp += offset;
    current_running->save_context = (regs_context_t *)(current_running->save_context) + offset;
    current_running->switch_context = (switchto_context_t *)(current_running->switch_context) + offset;
    current_running->core_id = core_id;    
    // jump to next
    // current_running->save_context->sepc = current_running->save_context->sepc;
    // u_cxt->uc_mcontext.__gregs[0] ? u_cxt->uc_mcontext.__gregs[0] :                     
    // ret 
    exit_signal_trampoline();

}

/**
 * @brief 
 * @param set 
 * 
 */
int do_rt_sigtimedwait(const sigset_t *restrict set,
                       siginfo_t *restrict info,
                       const struct timespec *restrict timeout)
{
    // // // assert(info);
    // // // printk("do_rt_sigtimedwait not avalib\n");
    // 
    // int64_t sig_value;
    // if ((set->sig[0] & current_running->sig_pend) || (set->sig[0] & current_running->sig_recv))
    // {   
    //     return get_sig_num(current_running, set); // there is already a signal
    // }

    // int first = 1;
    // while (!(set->sig[0] & current_running->sig_pend) && !(set->sig[0] & current_running->sig_recv))
    // {
    //     // printk("signal_recv: %d\n", current_running->sig_recv);
    //     // one sleep
    //     if (first)
    //     {
    //         do_nanosleep(timeout);
    //         first = 0;
    //     } else 
    //     {
    //         time_t now = get_ticks();
    //         if (now >= current_running->end_time)
    //         {
    //             printk("Timeout!\n");
    //                 if ((sig_value = get_sig_num(current_running, set)) == -1) return -EAGAIN;
    //                 // printk("sig_value: %d\n", sig_value);
    //                 else return sig_value;
    //         } else
    //         {
    //             timeval_t new;
    //             ticks_to_timeval(current_running->end_time - now, &new);
    //             do_nanosleep(&new);
    //         }            
    //     }
    // }
    // trans_pended_signal_to_recv(current_running);

    // if ((sig_value = get_sig_num(current_running, set)) == -1) return -EAGAIN;
    // // printk("sig_value: %d\n", sig_value);
    // else return sig_value;
    return 1;
}

/**
 * @brief the function deal the signal
 */
void handle_signal()
{   
    //return;
    // return;
    // get current running
    

    // judge if there are any unfinished signal
    if (current_running->sig_recv == 0 || current_running->is_handle_signal)
    {
        if (current_running->sig_recv == 0)
            ;// printk("There is no signal %s\n", current_running->name);
        else
            ;// printk("There are signal unfinished %s\n", current_running->name);
        return;
    }
    // need run
    if (!current_running->utime)
        return;
    current_running->is_handle_signal = 1;

    for (uint8_t i = 0; i < NUM_SIG; i++)
    {
        /* i == signum - 1 */
        if (ISSET_SIG(i + 1, current_running)){
            // 1. clear this signal
            CLEAR_SIG(i + 1, current_running);
            // printk("[handle signal] pid %d: handle signal %d\n",current_running->pid,i+1);
            // 2. change mask
            current_running->prev_mask = current_running->sig_mask;
            change_signal_mask(current_running, current_running->sigactions[i].sa_mask.sig[0]);
            // printk("[handle_signal] handling signum %d, handler = 0x%x\n",i+1,current_running->sigactions[i].handler);
            // 3. goto handler
            if (current_running->sigactions[i].handler == SIG_IGN)
                ;
            else if (current_running->sigactions[i].handler == SIG_DFL){
                /* kernel handle signal, no need to set trampoline */
                switch (i+1){
                    case SIGKILL:
                    case SIGSEGV:
                    case SIGALRM:
                        current_running->is_handle_signal = 0;
                        do_exit(1);
                        assert(0); /* never return */
                    case SIGCHLD:
                        handle_signal_sigchld();
                    case SIGCANCEL:
                        current_running->is_handle_signal = 0;
                        break;
                    default:
                        printk("unknown signal = %d\n",i+1);
                        assert(0);
                }
                // recover mask
                change_signal_mask(current_running, current_running->prev_mask);

            }
            else{
                /* set a trampoline */
                memcpy(&signal_context_buf, current_running->save_context, sizeof(regs_context_t));
                memcpy(&signal_switch_buf, current_running->switch_context, sizeof(switchto_context_t));
                /* set ucontext */
                // uintptr_t new_sp = set_ucontext();
                signal_context_buf.a0 = (reg_t)(i + 1);                                                  /* a0 = signum */
                // signal_context_buf.a1 = (reg_t)NULL;                                                     /* a1 is unuseful */
                // signal_context_buf.a2 = (reg_t)(SIGNAL_HANDLER_ADDR - sizeof(ucontext_t));               /* a2 is uncontext */
                signal_context_buf.ra  = (reg_t)(USER_STACK_ADDR - SIZE_RESTORE);                         /* should do sigreturn when handler return */
                                                                                                               
                // signal_context_buf.regs[2]  = new_sp;
                signal_context_buf.sepc = (reg_t)current_running->sigactions[i].handler;                              /* sepc = entry */
                // for switch
                uint64_t offset = (sizeof(regs_context_t) + sizeof(switchto_context_t));
                // new kernel_stack 
                signal_switch_buf.ra = pcb->kernel_sp - offset;
                // another one sp
                current_running->kernel_sp -= offset;
                current_running->save_context = (regs_context_t *)(current_running->save_context) - offset;
                current_running->switch_context = (switchto_context_t *)(current_running->switch_context) - offset;
                set_signal_trampoline(&signal_context_buf, &signal_switch_buf);

            }
            break;
        }        
    }
}


/**
 * @brief trans pending sign to unpending
 * @param pcb the pcb
 * @return no return
 */
void trans_pended_signal_to_recv(pcb_t *pcb)
{
    if (pcb->sig_pend != 0){
        uint64_t prev_recv = pcb->sig_recv;
        pcb->sig_recv |= (pcb->sig_pend & ~(pcb->sig_mask));
        pcb->sig_pend &= ~(prev_recv ^ pcb->sig_recv);
    }
}

/**
 * @brief change the pcb mask and trans pended signal to unpended
 * @param pcb the pcb
 * @param newmask the mask
 * @return no return
 */
void change_signal_mask(pcb_t *pcb, uint64_t newmask)
{
    pcb->sig_mask = newmask;
    // tmp no set
    trans_pended_signal_to_recv(pcb);
}

/**
 * @brief send signal to appointed process
 * @param signum the sig id
 * @param pcb the target pcb
 */
void send_signal(int signum, pcb_t *pcb)
{

    if (signum == 0) 
    {
        // log(0, "test signal setup");
        return ;
    }
    uintptr_t sig_value = (1lu << (signum - 1));
    // printk("signal mask: 0x%lx, sig_value: 0x%lx\n", pcb->sig_mask, sig_value);
    if ((sig_value & pcb->sig_mask) != 0)
    {
        // log(0, "block this signal");
        // printk("signal 0x%lx was blocked\n", sig_value);
        pcb->sig_pend |= sig_value;
    }
    else
    {
        // log(0, "recv this signal");
        // printk("signal 0x%lx was recved\n", sig_value);
        pcb->sig_recv |= sig_value;
    }
    if (pcb->status == TASK_BLOCKED)
    {
        // log(0, "receiver is unblocked", pcb->status);
        // printk("%s was unblocked\n", pcb->name);
        do_unblock(&pcb->list);
    }    
}

/**
 * @brief get sig value from pcb for timewait
 * @param pcb the pcb
 * @param set the set
 * @return return sig value
 */
uint64_t get_sig_num(void * pcb, sigset_t *set)
{
    pcb_t * todo_pcb = (pcb_t *)pcb;
    if (set->sig[0] & todo_pcb->sig_pend)
    {
        uint64_t high_pend = set->sig[0] & todo_pcb->sig_pend;
        for (int i = NUM_SIG; i > 0; i--)
        {
            uint64_t mask = 1ul << (i - 1);
            if (high_pend & mask)
            {
                return i;
            }
        }
        
    }

    if (set->sig[0] & todo_pcb->sig_recv)
    {
        uint64_t high_recv = set->sig[0] & todo_pcb->sig_recv;
        for (int i = NUM_SIG; i > 0; i--)
        {
            uint64_t mask = 1ul << (i - 1);
            if (high_recv & mask)
            {
                return i;
            }
        }
    }
    return -1;

}

/**
 * @brief set the ucontext for the 
 * @return the signal handle stack top
 */
uintptr_t set_ucontext()
{
    
    // trap user sp, be aligned
    uintptr_t trap_sp = SIGNAL_HANDLER_ADDR;
    uintptr_t trap_sp_base = SIGNAL_HANDLER_ADDR - PAGE_SIZE;
    // check the user stack carefully
    int need_alloc_page = get_kva_of(trap_sp_base, current_running->pgdir);

    if (!need_alloc_page)
        alloc_page_helper(trap_sp_base, current_running->pgdir, MAP_USER, _PAGE_READ | _PAGE_WRITE);
    // remember fence
    if (!need_alloc_page)
        local_flush_tlb_all();

    ucontext_t *u_cxt = (ucontext_t *)(get_kva_of(trap_sp - sizeof(ucontext_t),current_running->pgdir));
    // all reserve
    // set value
    memset(u_cxt, 0, sizeof(ucontext_t));
    u_cxt->uc_link = (ucontext_t *)(trap_sp - sizeof(ucontext_t));
    u_cxt->uc_sigmask.__bits[0] = current_running->sig_mask;
    // // copy switch context, reserve temporarily
    u_cxt->uc_mcontext.__gregs[0] = current_running->save_context->sepc;
    return ROUNDDOWN(trap_sp - sizeof(ucontext_t), 0x10);
}

void init_sig_table(){
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        sig_table[i].num = 0;
        sig_table[i].used = 0;
        memset(sig_table[i].sigactions,0,NUM_SIG * sizeof(sigaction_t));
    }
}

sigaction_t *alloc_sig_table(){
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if(sig_table[i].used == 0){
            // printk("get: %d\n", i);
            sig_table[i].used = 1;
            sig_table[i].num = 1;
            return sig_table[i].sigactions;
        }
    }
    printk("sig_table is full\n");
    assert(0);
    return NULL;
}

void free_sig_table(sigaction_t* sig_in){
    sigaction_table_t* st =  list_entry(sig_in, sigaction_table_t, sigactions);  
    st->num --;
    if(st->num == 0){
        st->used = 0;
        memset(sig_in,0,NUM_SIG * sizeof(sigaction_t));
    }
}