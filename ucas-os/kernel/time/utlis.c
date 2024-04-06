#include <os/time.h>
#include <os/sched.h>
#include <os/mm.h>
#include <os/irq.h>
#include <os/smp.h>
#include <type.h>


/**
 * @brief ticks to timespec base the HZ
 * @param time the ticks 
 * @param ts the pointer received from user
 * @return null
 */
void ticks_to_timespec(time_t time, timespec_t *ts) 
{
    // time /= 10;
    ts->tv_sec = time / time_base;
    uint64_t left = time % time_base;
    ts->tv_nsec = 0;
    for (uint i = 0; i < NANO; ++i)
    {
        ts->tv_nsec = 10*ts->tv_nsec + left * 10 / time_base;
        left = (left * 10) % time_base;
    }    
}

/**
 * @brief timespec to the ticks
 * @param ts the pointer to the timespec
 * @return ticks
 * 
 */
uint64_t timespec_to_ticks(const timespec_t *ts){
    uint64_t nsec = ts->tv_nsec, nticks = 0;
    for (uint8_t i = 0; i < NANO; i++){
        nticks += time_base * (nsec % 10);
        nticks /= 10;
        nsec /= 10;
    }
    return nticks + ts->tv_sec * time_base;
}


/**
 * @brief timedpec to the ticks
 * @param tms the pointer to the timeval
 * @return ticks
 * 
 */
uint64_t timeval_to_ticks(const timeval_t *tms){
    uint64_t usec = tms->tv_usec, uticks = 0;
    for (uint8_t i = 0; i < MICRO; i++){
        uticks += time_base * (usec % 10);
        uticks /= 10;
        usec /= 10;
    }
    return uticks + tms->tv_sec * time_base;  
}



/**
 * @brief ticks to timevaal
 * @param time the ticks
 * @param tms the pointer received from user
 */
void ticks_to_timeval(time_t time, timeval_t *tms)
{
    // time /= 10;
    tms->tv_sec = time / time_base; /* compute second */

    uint64_t left = time % time_base; /* compute micro seconds */
    tms->tv_usec = 0;
    for (uint i = 0; i < MICRO; ++i)
    {
        tms->tv_usec = 10*tms->tv_usec + left * 10 / time_base;
        left = (left * 10) % time_base;
    }    
}

/**
 * @brief init the pcb time, clear the pcb->end_time, stime, ustime
 * @param initpcb the pcb be processed
 * @return no return
 */
void clear_pcb_time(void * initpcb)
{
    pcb_t * pcb = (pcb_t *)initpcb;
    pcb->end_time = 0;
    pcb->utime = 0;
    pcb->stime = 0;
}

/**
 * @brief update the utime of the current_running
 */
void update_utime()
{
    uint64_t cpu_id = get_current_cpu_id();
    
    uint64_t *sys_time = cpu_id == 0 ? &sys_time_master : &sys_time_slave;
    current_running->utime += (get_ticks() - *sys_time);
    *sys_time = get_ticks();
}

/**
 * @brief update the stime of the current_running
 */
void update_stime()
{
    uint64_t cpu_id = get_current_cpu_id();
    
    uint64_t *sys_time = cpu_id == 0 ? &sys_time_master : &sys_time_slave;
    current_running->stime += (get_ticks() - *sys_time);
    *sys_time = get_ticks();   
}
