#include <os/time.h>
#include <os/mm.h>
#include <os/irq.h>
#include <type.h>
#include <assert.h>
#include <compile.h>
uint64_t time_elapsed = 0;
uint32_t time_base = 0;
clock_t sys_time_master;
clock_t sys_time_slave;

uint64_t get_ticks()
{
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed)
    );
    return time_elapsed;
}


uint64_t get_timer()
{
    return get_ticks() / time_base;
}

uint64_t get_time_base()
{
    return time_base;
}

/**
 * @brief syscall(SYS_gettimeofday, ts, 0) get the time;
 * @param ts the pointer from the user
 * @return succeed return 0, failed return -1
 */
uint32_t do_gettimeofday(timeval_t *ts) 
{
    // printk("enter get time of day\n");
    ticks_to_timeval(get_ticks(), ts);
    // printk("exit get time of day, ts->sec: %d, ts->tv_usec: %d\n", ts->tv_sec, ts->tv_usec);
    // ts->tv_sec += 1628129642;
    return 0;    
}


/**
 * @brief syscall(SYS_times, tms); get the time of the process
 * @param tms the pointer from the user
 */
clock_t do_gettimes(tm_t *tms) 
{
    
    tms->tms_stime = current_running->stime;
    tms->tms_utime = current_running->stime;
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].parent.parent == &pcb[i]) {
            tms->tms_cstime += pcb[i].stime;
            tms->tms_cutime += pcb[i].utime;
        }
    }
    return get_ticks();
}

int32_t do_clock_gettime(uint64_t clock_id, struct timespec *tp)
{
    // return do_gettimeofday(tp);
    ticks_to_timespec(get_ticks(), tp);
    return 0;
}

void tick_to_timeval(time_t time, struct timeval *tv){
    tv->tv_sec = time / time_base; /* compute second */
    uint64_t left = time % time_base; /* compute micro seconds */
    tv->tv_usec = 0;
    for (uint i = 0; i < MICRO; ++i){
        tv->tv_usec = 10 * tv->tv_usec + left * 10 / time_base;
        left = (left * 10) % time_base;
    }
}

uint64_t timeval_to_tick(const struct timeval *tv)
{
    uint64_t usec = tv->tv_usec, uticks = 0;
    for (uint8_t i = 0; i < MICRO; i++){
        uticks += time_base * (usec % 10);
        uticks /= 10;
        usec /= 10;
    }
    return uticks + tv->tv_sec * time_base;
}

LIST_HEAD(timers);

void itimer_check(void){
    timer_t *handling_timer, *next_timer;
    uint64_t nowtick = get_ticks();
    list_for_each_entry_safe(handling_timer, next_timer, &timers, list)
    {
        if (handling_timer->add_tick){
            time_t interval_tick = timeval_to_tick(&handling_timer->set_itime.it_interval);
            time_t value_tick = timeval_to_tick(&handling_timer->set_itime.it_value);
            //延时判断
            if(handling_timer->value_valid == 1){
                if(nowtick >= (value_tick + handling_timer->add_tick)){
                    handling_timer->value_valid = 0;
                    send_signal(SIGALRM, handling_timer->process);
                    handling_timer->last_send_tick = nowtick;
                }
            }else{
            //间隔
                if(interval_tick == 0){
                    continue;
                }
                if((nowtick - handling_timer->last_send_tick) >= interval_tick){
                    send_signal(SIGALRM, handling_timer->process);
                    handling_timer->last_send_tick = nowtick;
                }
            }
        }
        
    }
}

int do_setitimer(int which, const struct itimerval *new_value, 
                 struct itimerval *old_value){
    assert(which == ITIMER_REAL);
    
    time_t __maybe_unused  interval = timeval_to_tick(&new_value->it_interval);
    time_t value = timeval_to_tick(&new_value->it_value);
    if(value == 0){
        if (!list_empty(&current_running->itimer.list))
            list_del(&current_running->itimer.list);
        return 0;
    }
    if (!new_value || !old_value)
        return -EFAULT;
    /* old_value */
    if(current_running->itimer.add_tick){
       memcpy(old_value, &current_running->itimer.set_itime, sizeof(struct itimerval));
    }
    /* new_value */
    memcpy(&current_running->itimer.set_itime, new_value, sizeof(struct itimerval));
    current_running->itimer.add_tick = get_ticks();
    current_running->itimer.process = current_running;
    current_running->itimer.value_valid = 1;
    list_add_tail(&current_running->itimer.list, &timers);
    return 0;
}