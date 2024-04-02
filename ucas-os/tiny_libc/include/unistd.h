#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <stdint.h>
typedef int32_t pid_t;
typedef pid_t pthread_t;

pid_t sys_exec(char *name, int argc, char **argv);
void sys_exit(void);
void sys_sleep(uint32_t time);
int  sys_kill(pid_t pid);
void sys_ps(void);
int  sys_waitpid(pid_t pid);
pid_t sys_getpid(void);
void sys_yield(void);
void sys_write(char *buff);
int  sys_getchar(void);
void sys_move_cursor(int x, int y);
void sys_reflush(void);
long sys_get_timebase(void);
long sys_get_tick(void);
#endif
