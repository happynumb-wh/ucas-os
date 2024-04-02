#include <sbi.h>
#include <atomic.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lock.h>
#include <os/sync.h>
#include <os/irq.h>
spinlock_t kernel_lock;


void wakeup_other_hart()
{
    uint64_t id = 2;
    sbi_send_ipi(&id); 
    __asm__ __volatile__ (
        "csrw sip, zero"
    );
}

// get current 
uint64_t get_current_cpu_id()
{
    return current->core_id;
}

// lock kernel
void lock_kernel()
{
    spin_lock_acquire(&kernel_lock);
}

void unlock_kernel()
{
    // atomic_swap(UNLOCKED, (ptr_t)&kernel_lock.status);
    // kernel_lock.status = UNLOCKED;
    spin_lock_release(&kernel_lock);
}

void init_kernel_lock()
{
    kernel_lock.status = UNLOCKED;
}