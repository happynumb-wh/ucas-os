#ifndef SPINLOCK_H
#define SPINLOCK_H
#include <type.h>
// spinlock 
typedef struct spinlock{
    uint64_t status;
    uint64_t cpu;
}spinlock_t;

// spinlock status
typedef enum {
    UNLOCKED,
    LOCKED,    
} spinlock_status_t;

#define spinlock_init(name) struct spinlock name = {UNLOCKED, 0};
#define initlock(name) {(name)->status = UNLOCKED; (name)->cpu = 0;}
#define release(name) ((name)->status = UNLOCKED);


// acquire 
extern void acquire(spinlock_t * spin);
// extern void release(spinlock_t * spin);


#endif 
