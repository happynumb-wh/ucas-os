#ifndef SMP_H
#define SMP_H

#include <type.h>

typedef struct pcb pcb_t;

#define current current_running

extern pcb_t * get_current_running();
extern void wakeup_other_hart();
extern uint64_t get_current_cpu_id();
extern void lock_kernel();
extern void unlock_kernel();
extern void init_kernel_lock();
//extern 
#endif /* SMP_H */
