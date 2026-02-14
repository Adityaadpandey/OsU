/*
 * syscall.h - System Call Interface
 * Ring 3 to Ring 0 transition via int 0x80
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>

/* System call numbers */
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_GETPID  3
#define SYS_SLEEP   4
#define SYS_YIELD   5

/* Initialize syscall handler */
void syscall_init(void);

/* Syscall implementations */
void sys_exit(int status);
int sys_write(int fd, const char *buf, size_t count);
int sys_read(int fd, char *buf, size_t count);
int sys_getpid(void);
void sys_sleep(uint32_t ms);
void sys_yield(void);

#endif /* SYSCALL_H */
