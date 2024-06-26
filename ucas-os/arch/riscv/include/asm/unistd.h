#ifndef __ASM_UNISTD_H__
#define __ASM_UNISTD_H__

#define SHELL_SYSCALL_BASE 300

#define SYSCALL_EXEC            (0 + SHELL_SYSCALL_BASE)
#define SYSCALL_EXIT            (1 + SHELL_SYSCALL_BASE)
#define SYSCALL_SLEEP           (2 + SHELL_SYSCALL_BASE)
#define SYSCALL_KILL            (3 + SHELL_SYSCALL_BASE)
#define SYSCALL_WAITPID         (4 + SHELL_SYSCALL_BASE)
#define SYSCALL_PS              (5 + SHELL_SYSCALL_BASE)
#define SYSCALL_GETPID          (6 + SHELL_SYSCALL_BASE)
#define SYSCALL_YIELD           (7 + SHELL_SYSCALL_BASE)
#define SYSCALL_WRITE           (20 + SHELL_SYSCALL_BASE)
#define SYSCALL_READCH          (21 + SHELL_SYSCALL_BASE)
#define SYSCALL_CURSOR          (22 + SHELL_SYSCALL_BASE)
#define SYSCALL_REFLUSH         (23 + SHELL_SYSCALL_BASE)
#define SYSCALL_CLEAR           (24 + SHELL_SYSCALL_BASE)
#define SYSCALL_GET_TIMEBASE    (30 + SHELL_SYSCALL_BASE)
#define SYSCALL_GET_TICK        (31 + SHELL_SYSCALL_BASE)
#define SYSCALL_LOCK_INIT       (40 + SHELL_SYSCALL_BASE)
#define SYSCALL_LOCK_ACQ        (41 + SHELL_SYSCALL_BASE)
#define SYSCALL_LOCK_RELEASE    (42 + SHELL_SYSCALL_BASE)
#define SYSCALL_SHOW_TASK       (43 + SHELL_SYSCALL_BASE)
#define SYSCALL_BARR_INIT       (44 + SHELL_SYSCALL_BASE)
#define SYSCALL_BARR_WAIT       (45 + SHELL_SYSCALL_BASE)
#define SYSCALL_BARR_DESTROY    (46 + SHELL_SYSCALL_BASE)
#define SYSCALL_COND_INIT       (47 + SHELL_SYSCALL_BASE)
#define SYSCALL_COND_WAIT       (48 + SHELL_SYSCALL_BASE)
#define SYSCALL_COND_SIGNAL     (49 + SHELL_SYSCALL_BASE)
#define SYSCALL_COND_BROADCAST  (50 + SHELL_SYSCALL_BASE)
#define SYSCALL_COND_DESTROY    (51 + SHELL_SYSCALL_BASE)
#define SYSCALL_MBOX_OPEN       (52 + SHELL_SYSCALL_BASE)
#define SYSCALL_MBOX_CLOSE      (53 + SHELL_SYSCALL_BASE)
#define SYSCALL_MBOX_SEND       (54 + SHELL_SYSCALL_BASE)
#define SYSCALL_MBOX_RECV       (55 + SHELL_SYSCALL_BASE)
#define SYSCALL_TASKSET         (56 + SHELL_SYSCALL_BASE)
/* Project4 */
#define SYSCALL_SHM_GET         (57 + SHELL_SYSCALL_BASE)
#define SYSCALL_SHM_DT          (58 + SHELL_SYSCALL_BASE)
#define SYSCALL_CLONE           (59 + SHELL_SYSCALL_BASE)
#define SYSCALL_SWAP            (60 + SHELL_SYSCALL_BASE)
#define SYSCALL_SNAPSHOT        (61 + SHELL_SYSCALL_BASE)
#define SYSCALL_QUERY_PA        (62 + SHELL_SYSCALL_BASE)

#endif