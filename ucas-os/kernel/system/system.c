#include <os/system.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/time.h>
#include <assert.h>

struct rlimit mylimit;

int32_t do_sysinfo(struct sysinfo *info){
    info->uptime = get_timer();
    info->loads[0] = 0; info->loads[1] = 0; info->loads[2] = 0;
    info->totalram= KERNEL_END - FREEMEM;
    info->freeram = KERNEL_END - memCurr;
    // for (list_node_t *i = freePageList.next; i != &freePageList; i = i->next)
    //     info->freeram += NORMAL_PAGE_SIZE;
    info->freeram = freePageManager.source_num * NORMAL_PAGE_SIZE;
    info->sharedram = 0;
    info->bufferram = 0x10000; //?
    info->totalswap = 0;
    info->freeswap = 0;
    info->procs = 0;
    for (uint8_t i = 0; i < NUM_MAX_TASK; i++)
        if (pcb[i].status != TASK_ZOMBIE && pcb[i].status != TASK_EXITED)
            info->procs++;
    info->totalhigh = 0;
    info->freehigh = 0;
    info->mem_unit = 1;
    return 0;
}




int do_prlimit(pid_t pid, int resource, const struct rlimit *new_limit, 
    struct rlimit *old_limit){
    if (resource == RLIMIT_NOFILE) {
        if (old_limit) do_getrlimit(resource,old_limit);
        if (new_limit) do_setrlimit(resource,new_limit);
    }
    return 0;
}

int do_getrlimit(int resource,struct rlimit * rlim){
    
    memcpy(&mylimit,&current_running->fd_limit,sizeof(struct rlimit));
    memcpy(rlim, &mylimit, sizeof(struct rlimit));
    return 0;
}

int do_setrlimit(int resource,const struct rlimit * rlim){
    
    memcpy(&mylimit, rlim, sizeof(struct rlimit));
    memcpy(&current_running->fd_limit,&mylimit,sizeof(struct rlimit));
    return 0;
}
void do_syslog(){
    return;
}

int do_vm86(unsigned long fn, void *v86){
    return 0;
}

int32_t do_getrusage(int32_t who, struct rusage *usage){
    
    if (who == RUSAGE_SELF){
        tick_to_timeval(current_running->utime, &usage->ru_utime);
        tick_to_timeval(current_running->stime, &usage->ru_stime);
        usage->ru_maxrss = 752;
        usage->ru_ixrss = 0;
        usage->ru_idrss = 0;
        usage->ru_isrss = 0;
        usage->ru_minflt = 32;
        usage->ru_majflt = 7;
        usage->ru_nswap = 0;
        usage->ru_inblock = 1440;
        usage->ru_oublock = 0;
        usage->ru_msgsnd = 0;
        usage->ru_msgrcv = 0;
        usage->ru_nsignals = 0;
        usage->ru_nvcsw = 0;
        usage->ru_nivcsw = 0;
    }
    else
        assert(0);
    return 0;
}
