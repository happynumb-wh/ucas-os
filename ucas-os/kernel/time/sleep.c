#include <os/time.h>
#include <os/mm.h>
#include <os/irq.h>
#include <os/smp.h>
#include <stdio.h>
#include <screen.h>
#include <type.h>

/**
 * @brief sleep the pcb in the corelist, will sleep on the core id
 * @param sleep time
 */
int do_nanosleep(timespec_t *ts){
    /* sleep on the core queue */
    list_head * timer = get_current_cpu_id() == 0 ? &timer_master : &timer_slave;
    
    current_running->end_time = get_ticks() + timespec_to_ticks(ts);
    do_block(&current_running->list, timer);
    // return 0;
    if (current_running->sig_recv & 1ul << (SIGCANCEL - 1))
        return -EINTR;
    else return 0;
}


void check_sleeping(){
    /* for the k210 */
    list_head *k210_timer;
    pcb_t *k210_timer_qntry = NULL, *k210_timer_q;
    k210_timer = get_current_cpu_id() == 0 ? &timer_master : &timer_slave;
    list_for_each_entry_safe(k210_timer_qntry, k210_timer_q, k210_timer, list){
        if(get_ticks() >= k210_timer_qntry->end_time){
            do_unblock(&k210_timer_qntry->list);
        }      
    }        
}


void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();

    while (get_timer() - begin_time < time);
    return;
}