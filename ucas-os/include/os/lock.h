#ifndef INCLUDE_LOCK_H_
#define INCLUDE_LOCK_H_
#define MAX_LOCK_NUM 16
#define MAX_WAIT_QUEUE 16
#define MAX_BARRIER_NUM 16
#define MAX_MAILBOX_NUM 16
#define MAX_MSG_LENGTH 64

#include <os/list.h>
#include <os/spinlock.h>

#define USER_LOCK 0
#define USER_NULOCK 1

typedef spinlock_status_t lock_status_t;

typedef enum {
    USED,
    UNSED,
} user_lock_status_t;

typedef enum {
    MBOX_OPEN,
    MBOX_CLOSE,
} mailbx_ststus_t;

/* sleep lock */
typedef struct mutex_lock
{
    uint64_t pid;
    uint64_t status;
    list_head block_queue;
} mutex_lock_t;

#define mutex_init(lock) struct mutex_lock lock = \
                            {  .pid = 0,\
                               .status = UNLOCKED,\
                               .block_queue = {&(lock.block_queue), &(lock.block_queue)} \
                            }
                         

void spin_lock_init(spinlock_t *lock);
int spin_lock_try_acquire(spinlock_t *lock);
void spin_lock_acquire(spinlock_t *lock);
void spin_lock_release(spinlock_t *lock);

/* mutex lock */
void do_mutex_lock_init(mutex_lock_t *lock);
void do_mutex_lock_acquire(mutex_lock_t *lock);
void do_mutex_lock_release(mutex_lock_t *lock);

#endif
