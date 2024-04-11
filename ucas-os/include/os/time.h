#ifndef INCLUDE_TIME_H_
#define INCLUDE_TIME_H_

#include <type.h>
#include <compile.h>
#include <os/list.h>

#define MICRO 6
#define NANO 9



#define UTIME_NOW  0x3fffffff
#define UTIME_OMIT 0x3ffffffe

#define TIME_BASE 50000000

typedef void (*TimerCallFunc)(void *parameter);
static LIST_HEAD(timer_master);
static LIST_HEAD(timer_slave);
extern clock_t sys_time_master;
extern clock_t sys_time_slave;

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
uint64_t timespec_to_ticks(const timespec_t *ts);
uint64_t timeval_to_ticks(const timeval_t *tms);


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


/*
 * Flags for getrandom(2)
 *
 * GRND_NONBLOCK	Don't block and return EAGAIN instead
 * GRND_RANDOM		Use the /dev/random pool instead of /dev/urandom
 */
#define GRND_NONBLOCK	0x0001
#define GRND_RANDOM	0x0002
int64_t do_getrandom(char *buf, size_t count, uint32_t flags);

#endif
