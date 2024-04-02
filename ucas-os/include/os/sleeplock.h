#ifndef __SLEEPLOCK_H
#define __SLEEPLOCK_H

#include <type.h>
#include <os/spinlock.h>
#include <os/sched.h>
// Long-term locks for processes
typedef struct sleeplock {
  uint32_t locked;          // Is the lock held?
  
  list_head sleep_queue;    // sleep queue

  // For debugging:
  char *name;        // Name of lock.
  int pid;           // Process holding lock
}sleeplock_t;

// sleep 
typedef struct sleeplist{
    list_head sleep_queue;
}sleeplist_t;

#define sleeplist_init(r)   { \
                            (r)->sleep_queue.prev = &(r)->sleep_queue;\
                            (r)->sleep_queue.next = &(r)->sleep_queue;\
                            }

// void initsleeplist(sleeplist_t *sleep_queue);
void sleep(sleeplist_t *sleep_queue);
void wakeup(sleeplist_t *sleep_queue);
void initsleeplock(struct sleeplock *lk);
void acquiresleep(struct sleeplock *lk);
void releasesleep(struct sleeplock *lk);
#endif
