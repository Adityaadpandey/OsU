/*
 * process.h - Process/Task management for OsU
 * Implements cooperative and preemptive multitasking
 */

#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>

/* Maximum number of processes */
#define MAX_PROCESSES 16

/* Process stack size (4KB per process) */
#define PROCESS_STACK_SIZE 4096

/* Process states */
typedef enum {
    PROCESS_UNUSED = 0,   /* Slot is free */
    PROCESS_READY,        /* Ready to run */
    PROCESS_RUNNING,      /* Currently executing */
    PROCESS_BLOCKED,      /* Waiting for event */
    PROCESS_ZOMBIE        /* Terminated, waiting cleanup */
} process_state_t;

/* CPU context saved during context switch */
typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eip;
    uint32_t eflags;
} cpu_context_t;

/* Task Control Block (TCB) */
typedef struct {
    uint32_t pid;                      /* Process ID */
    process_state_t state;             /* Current state */
    cpu_context_t context;             /* Saved CPU context */
    uint32_t stack[PROCESS_STACK_SIZE / 4]; /* Kernel stack */
    uint32_t *stack_ptr;               /* Current stack pointer */
    char name[32];                     /* Process name */
    uint32_t priority;                 /* Priority (0 = highest) */
    uint32_t time_slice;               /* Remaining time slice */
    uint32_t total_ticks;              /* Total CPU ticks used */
    int exit_code;                     /* Exit code when terminated */
} process_t;

/* Process management functions */
void process_init(void);
int process_create(const char *name, void (*entry)(void));
void process_exit(int exit_code);
void process_yield(void);
process_t *process_current(void);
int process_kill(uint32_t pid);

/* Scheduler functions */
void scheduler_init(void);
void schedule(void);
void scheduler_tick(void);           /* Called from timer interrupt */

/* Context switch (implemented in assembly) */
extern void context_switch(uint32_t **old_sp, uint32_t *new_sp);

/* List all processes (for ps command) */
void process_list(void);

#endif /* PROCESS_H */
