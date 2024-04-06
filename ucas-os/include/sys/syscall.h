/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

#include <os/syscall_number.h>
// #include <mthread.h>
#include <stdint.h>
#include <os.h>
#define O_RDONLY 0x00
#define O_WRONLY 0x01
#define O_RDWR 0x02 // 可读可写
//#define O_CREATE 0x200
#define O_CREATE 0x40
#define O_DIRECTORY 0x0200000

#define DEV_STDIN 0
#define DEV_STDOUT 1
#define DEV_STDERR 2


// #define DIR 0x040000
// #define FILE 0x100000

#define AT_FDCWD -100

#define SCREEN_HEIGHT 80
 
pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode);
pid_t sys_load(const char *path, char* argv[], char *env[]);
void sys_test(const char *file_name);


extern long invoke_syscall(long, long, long, long, long,long,long);
// extern int sys_fork(int); 
int sys_sleep(unsigned long long time);
#define BINSEM_OP_LOCK 0 // mutex acquire
#define BINSEM_OP_UNLOCK 1 // mutex release

pid_t sys_wait(pid_t pid, uint16_t *status, uint32_t options);

static inline pid_t waitpid(pid_t pid)
{
    uint16_t status;
    sys_wait(pid, &status, 0);
}

#define FREE 0
#define LOAD 1
int sys_pre_load(const char * file_name, int how);

void sys_write(char *);
void sys_reflush();


long sys_get_tick();

void sys_screen_clear(void);
pid_t getpid();

uint8_t sys_disk_read(void *data_buff, uint32_t sector, uint32_t count);
uint8_t sys_disk_write(void *data_buff, uint32_t sector, uint32_t count);


int open(const char *path, int flags);
int32_t close(uint32_t fd);
uint64_t sys_get_timer();
int thread_join(int thread);
#endif
