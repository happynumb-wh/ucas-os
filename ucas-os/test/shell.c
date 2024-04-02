/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define SHELL_BEGIN 20
#define SHELL_BUF_SIZE 64
#define SHELL_MAX_ARGS 64

void _halt(int code)
{
    __asm__ volatile ("mv a0, %0\n\r.word 0x0005006b"::"r"(code));

    // should not reach here during simulation
    printf("Exit with code = %d\n", code);

    // should not reach here on FPGA
    while(1);
}


typedef int pid_t;
int main(void)
{
    int shell_tail = 0;
    char shell_buff[SHELL_BUF_SIZE];

    // sys_move_cursor(0, SHELL_BEGIN);
    // printf("------------------- COMMAND -------------------\n");
    printf("[Shell]: DASICS_TEST:\n");

    // while (1)
    // {
    //     int command = 0;

    //     // call syscall to read UART port
    //     char ch = sys_getchar();

    //     // parse input
    //     // note: backspace maybe 8('\b') or 127(delete)
    //     if (ch == '\b' || ch == '\177')
    //     {
    //         if (shell_tail > 0)
    //         {
    //             shell_tail--;
    //             printf("%c", ch);
    //         }
    //     }
    //     else if (ch == '\n' || ch == '\r')
    //     {
    //         shell_buff[shell_tail] = '\0';
    //         command = 1;
    //         printf("%c", ch);
    //         printf("\n");
    //     }
    //     else if (ch != 0)
    //     {
    //         shell_buff[shell_tail++] = ch;
    //         printf("%c", ch);
    //     }

    //     // execute shell command according to the input
    //     if (command == 1)
    //     {
    //         int len = strlen(shell_buff);

    //         if (len == 0)
    //         {
    //             /* user enters a single '\n', do nothing */
    //         }
    //         else if (strncmp(shell_buff, "ps", 2) == 0)
    //         {
    //             sys_ps();
    //         }
    //         else if (strncmp(shell_buff, "exec", 4) == 0)
    //         {
    //             int wait_flag = 1;
    //             int argc = 0;
    //             char *argv[SHELL_MAX_ARGS];

    //             for (int i = 4; i < len; ++i)
    //             {
    //                 if (!isspace(shell_buff[i]) && isspace(shell_buff[i-1]))
    //                 {
    //                     if (shell_buff[i] == '&')
    //                     {
    //                         wait_flag = 0;
    //                         shell_buff[i - 1] = '\0';
    //                         break;
    //                     }
    //                     argv[argc++] = shell_buff + i;
    //                     shell_buff[i-1] = '\0';
    //                 }
    //             }
    //             pid_t pid = sys_exec(argv[0], argc, argv);
    //             if (pid == -1)
    //             {
    //                 // printf("Info: failed to open %s\n", argv[0]);
    //             } else
    //             {
    //                 // printf("Info: execute %s successfully, pid = %d ...\n", argv[0],
    //                         // pid); 
    //                 // if (wait_flag)
    //                 //     sys_waitpid(pid);
    //             }

    //         }
    //         else if (strncmp(shell_buff, "kill", 4) == 0)
    //         {
    //             pid_t pid = 0;

    //             for (int i = 4; i < len; ++i)
    //             {
    //                 if (!isspace(shell_buff[i]))
    //                 {
    //                     int j = i;
    //                     while (j < len && !isspace(shell_buff[j]))
    //                     {
    //                         pid = pid * 10 + shell_buff[j++] - '0';
    //                     }
    //                     break;
    //                 }
    //             }

    //             if (pid == 0 || pid == 1)
    //             {
    //                 printf("Error: process with pid %d cannot be killed!\n", pid);
    //             }
    //             else
    //             {
    //                 printf("Info: kill process with pid %d ...\n", pid);
    //                 sys_kill(pid);
    //             }
    //         } 
    //         else
    //         {
    //             printf("Error: Unknown Command %s!\n", shell_buff);
    //         }

    //         printf("> root@wh: ");
    //         shell_tail = 0;
    //     }
    

        #define NULL ((void *) 0)

        printf("[Shell]: Do test : %s\n", "dasics_test_rwx");
        int rwx_argc = 2;
        char *rwx_argv[] = {
            "dasics_test_rwx",
            "-dasics",
            NULL,
        };

        pid_t rwx_pid = sys_exec(rwx_argv[0], rwx_argc, rwx_argv);

        sys_waitpid(rwx_pid);


        printf("[Shell]: Do test : %s\n", "dasics_test_jump");

        int dasics_jump_argc = 2;
        char *dasics_jump_argv[] = {
            "dasics_test_jump",
            "-dasics",
            NULL,
        };

        pid_t jump_pid = sys_exec(dasics_jump_argv[0], dasics_jump_argc, dasics_jump_argv);   
        
        sys_waitpid(jump_pid);



        printf("[Shell]: Do test : %s\n", "dasics_test_ofb");

        int dasics_ofb_argc = 2;
        char *dasics_ofb_argv[] = {
            "dasics_test_ofb",
            "-dasics",
            NULL,
        };

        pid_t ofb_pid = sys_exec(dasics_ofb_argv[0], dasics_ofb_argc, dasics_ofb_argv);   

        
        sys_waitpid(ofb_pid);


        printf("[Shell]: Do test : %s\n", "dasics_test_free");
        int dasics_free_argc = 2;
        char *dasics_free_argv[] = {
            "dasics_test_free",
            "-dasics",
            NULL,
        };

        pid_t free_pid = sys_exec(dasics_free_argv[0], dasics_free_argc, dasics_free_argv);   
        
        sys_waitpid(free_pid);

        printf("[Shell]: Do test : %s\n", "case-study");
        int dasics_case_study_argc = 2;
        char *dasics_case_study_argv[] = {
            "attack_case",
            "-dasics",
            NULL,
        };

        pid_t case_study_pid = sys_exec(dasics_case_study_argv[0], dasics_case_study_argc, dasics_case_study_argv);   
        
        sys_waitpid(case_study_pid);


        _halt(0);
    //    sys_sleep(1);

    // }
    

    // while(1)
    // {
        // printf("[Shell]: Over, go to sleep\n");
        // sys_sleep(10000);
    // }



    return 0;
}
