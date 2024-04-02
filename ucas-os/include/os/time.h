/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                  Timer
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_TIME_H_
#define INCLUDE_TIME_H_

#include <type.h>
#include <os/list.h>
#define MICRO 6
#define NANO 9



#define UTIME_NOW  0x3fffffff
#define UTIME_OMIT 0x3ffffffe

#define TIME_BASE 10000000

typedef void (*TimerCallFunc)(void *parameter);
static LIST_HEAD(timer_master);
static LIST_HEAD(timer_slave);
static uint64_t sys_time_master = 0;
static uint64_t sys_time_slave = 0;


/* for gettimes */
typedef struct tms              
{                     
    clock_t tms_utime;  
    clock_t tms_stime;  
    clock_t tms_cutime; 
    clock_t tms_cstime; 
}tm_t;


/* nanosecond precison timespec */
typedef struct timespec {
    time_t tv_sec; // seconds
    time_t tv_nsec; // and nanoseconds
}timespec_t;


/* microsecond precison timeval */
typedef struct timeval {
    time_t  tv_sec;   
    time_t  tv_usec;
}timeval_t;

/* itimer */
#define ITIMER_REAL 0 /* Timers run in real time */
#define ITIMER_VIRTUAL 1 /* Timers run only when the process is executing */
#define ITIMER_PROF 2 /* Timers run when the process is executing and 
                        when the system is executing on behanlf of the process */
struct itimerval {
    struct timeval it_interval; /* Interval for periodic timer */
    struct timeval it_value;    /* Time until next expiration */
};

extern uint32_t time_base;
extern uint64_t time_elapsed;
extern uint64_t MHZ;


uint64_t get_timer(void);
uint64_t get_ticks(void);

/* clear pcb time */
void clear_pcb_time(void * initpcb);
extern void update_utime();
extern void update_stime();



/* utlis.c */
void ticks_to_timespec(time_t time, timespec_t *ts);
void ticks_to_timeval(time_t time, timeval_t *tms);
uint64_t timespec_to_ticks(timespec_t *ts);
uint64_t timeval_to_ticks(timeval_t *tms);


extern uint64_t get_time_base();
extern uint32_t do_gettimeofday(timeval_t *ts);
extern clock_t do_gettimes(tm_t *tms);
int do_nanosleep(timespec_t *ts);

/* sleep */
void check_sleeping();

void latency(uint64_t time);

int32_t do_clock_gettime(uint64_t clock_id, struct timespec *tp);//就是gettimeofday
void tick_to_timeval(time_t time, struct timeval *tv);

extern list_head timers;
typedef struct timer{
    struct itimerval set_itime;
    int64_t add_tick; // is 0: not set
    int64_t last_send_tick; // the tick when sending last SIGALRM  
    list_node_t list;
    void *process;
    int value_valid;
} timer_t;
void itimer_check(void);
int do_setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value);
#endif
