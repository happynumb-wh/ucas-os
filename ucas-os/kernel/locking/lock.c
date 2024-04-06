#include <os/lock.h>
#include <os/sched.h>
#include <os/mm.h>
#include <atomic.h>
#include <screen.h> 
#include <os/string.h>
// mutex_init(lock);

void spin_lock_init(spinlock_t *lock)
{
    /* TODO */
    lock->status = UNLOCKED;
}

int spinlock_try_acquire(spinlock_t *lock)
{
    /* TODO */
    return atomic_swap_d(LOCKED, (ptr_t)&lock->status);
}

void spin_lock_acquire(spinlock_t *lock)
{
    /* TODO */
    while (UNLOCKED != spinlock_try_acquire(lock))
    {
        ;
    }
}

void spin_lock_release(spinlock_t *lock)
{
    /* TODO */
    atomic_swap_d(UNLOCKED, (ptr_t)&lock->status);
}




/* init one mutex lock */
void do_mutex_lock_init(mutex_lock_t *lock)
{
    init_list(&lock->block_queue);
    lock->status = UNLOCKED;
    lock->pid = 0;
}
/* acquire a mutex lock */
void do_mutex_lock_acquire(mutex_lock_t *lock)
{

    while(lock->status == LOCKED){
        do_block(&current_running->list, &lock->block_queue);
    }

    lock->status = LOCKED;
    lock->pid = current_running->pid;
}

/* release one progress */
void do_mutex_lock_release(mutex_lock_t *lock)
{
    if(!list_empty(&lock->block_queue)){
        do_unblock(lock->block_queue.prev);//释放一个锁进程        
    }
    else{
        lock->status = UNLOCKED;
        lock->pid = 0;
    }
}
