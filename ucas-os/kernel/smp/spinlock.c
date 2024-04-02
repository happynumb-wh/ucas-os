#include <atomic.h>
#include <os/spinlock.h>
#include <os/lock.h>

// acquire the spin lock
void acquire(spinlock_t * spin){
    spin_lock_acquire(spin);
    // the cpu handle the spinlock
    // spin->cpu = get_current_cpu_id();
}




// release the spin lock
// void release(spinlock_t * spin){
//     // set the lock status
//     spin->status = UNLOCKED;
// }

