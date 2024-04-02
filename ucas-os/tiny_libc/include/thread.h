#ifndef PTHREAD_H_
#define PTHREAD_H_
#include "unistd.h"

/* pthread_create */
static inline void pthread_create(pthread_t *thread,
                   void (*start_routine)(void*),
                   void *arg)
{
    *thread =  sys_clone(CLONE_VM, start_routine, arg);

}

/* pthread wait */
static inline int pthread_join(pthread_t thread)
{
    return sys_waitpid(thread);
}



#endif