#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define SHELL_BEGIN 20
#define SHELL_BUF_SIZE 128
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

    printf("------------------- COMMAND -------------------\n");
    printf("> root@wh: ");

    // char *ripe_argv[] = {
    //     "ripe_attack_generator",
    //     "-t",
    //     "direct",
    //     "-i",
    //     "shellcode",
    //     "-c",
    //     "ret",
    //     "-l",
    //     "stack",
    //     "-f",
    //     "memcpy",
    //     "-dasics",
    //     (void *)0
    // };


    char *cactusADM_argv[] = {
        "cactusADM",
        "benchADM.par",
        (void *)0
    };

    // pid_t pid =  sys_exec(ripe_argv[0], 12, ripe_argv);
    pid_t pid =  sys_exec(cactusADM_argv[0], 2, cactusADM_argv);


    sys_waitpid(pid);

    while (1)
    {
        int command = 0;

        // call syscall to read UART port
        char ch = sys_getchar();

        // parse input
        // note: backspace maybe 8('\b') or 127(delete)
        if (ch == '\b' || ch == '\177')
        {
            if (shell_tail > 0)
            {
                shell_tail--;
                printf("%c", ch);
            }
        }
        else if (ch == '\n' || ch == '\r')
        {
            shell_buff[shell_tail] = '\0';
            command = 1;
            printf("%c", ch);
            printf("\n");
        }
        else if (ch != 0)
        {
            shell_buff[shell_tail++] = ch;
            printf("%c", ch);
        }

        // execute shell command according to the input
        if (command == 1)
        {
            int len = strlen(shell_buff);

            if (len == 0)
            {
                /* user enters a single '\n', do nothing */
            }
            else if (strncmp(shell_buff, "ps", 2) == 0)
            {
                sys_ps();
            }
            else if (strncmp(shell_buff, "exec", 4) == 0)
            {
                int wait_flag = 1;
                int argc = 0;
                char *argv[SHELL_MAX_ARGS];

                for (int i = 4; i < len; ++i)
                {
                    if (!isspace(shell_buff[i]) && isspace(shell_buff[i-1]))
                    {
                        if (shell_buff[i] == '&')
                        {
                            wait_flag = 0;
                            shell_buff[i - 1] = '\0';
                            break;
                        }
                        argv[argc++] = shell_buff + i;
                        shell_buff[i-1] = '\0';
                    }
                }
                pid_t pid = sys_exec(argv[0], argc, argv);
                if (pid == -1)
                {
                    // printf("Info: failed to open %s\n", argv[0]);
                } else
                {
                    // printf("Info: execute %s successfully, pid = %d ...\n", argv[0],
                            // pid); 
                    if (wait_flag)
                        sys_waitpid(pid);
                }

            }
            else if (strncmp(shell_buff, "kill", 4) == 0)
            {
                pid_t pid = 0;

                for (int i = 4; i < len; ++i)
                {
                    if (!isspace(shell_buff[i]))
                    {
                        int j = i;
                        while (j < len && !isspace(shell_buff[j]))
                        {
                            pid = pid * 10 + shell_buff[j++] - '0';
                        }
                        break;
                    }
                }

                if (pid == 0 || pid == 1)
                {
                    printf("Error: process with pid %d cannot be killed!\n", pid);
                }
                else
                {
                    printf("Info: kill process with pid %d ...\n", pid);
                    sys_kill(pid);
                }
            } 
            else
            {
                printf("Error: Unknown Command %s!\n", shell_buff);
            }

            printf("> root@wh: ");
            shell_tail = 0;
        }
    }
    
    return 0;
}
