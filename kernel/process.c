/*
 * process.c - Process management implementation
 */

#include "process.h"
#include "memory.h"
#include "string.h"
#include "vga.h"

/* Process table */
static process_t process_table[MAX_PROCESSES];
static process_t *current_process = 0;
static uint32_t next_pid = 1;
static int scheduler_enabled = 0;

/* Initialize process management */
void process_init(void) {
    /* Clear all process slots */
    memset(process_table, 0, sizeof(process_table));
    current_process = 0;
    next_pid = 1;
    scheduler_enabled = 0;
}

/* Find a free process slot */
static process_t *find_free_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_UNUSED) {
            return &process_table[i];
        }
    }
    return 0;
}

/* Find process by PID */
static process_t *find_process(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid &&
            process_table[i].state != PROCESS_UNUSED) {
            return &process_table[i];
        }
    }
    return 0;
}

/* Wrapper to handle process exit when entry function returns */
static void process_wrapper(void (*entry)(void)) {
    entry();
    process_exit(0);
}

/* Create a new process */
int process_create(const char *name, void (*entry)(void)) {
    process_t *p = find_free_slot();
    if (!p) {
        return -1;  /* No free slots */
    }

    /* Initialize process */
    memset(p, 0, sizeof(process_t));
    p->pid = next_pid++;
    p->state = PROCESS_READY;
    p->priority = 1;
    p->time_slice = 10;  /* Default time slice */

    /* Copy name */
    size_t len = strlen(name);
    if (len > 31) len = 31;
    memcpy(p->name, name, len);
    p->name[len] = '\0';

    /* Set up initial stack
     * Stack grows downward, so we start at the top
     * The stack needs to look like context_switch will restore from it
     */
    uint32_t *sp = &p->stack[PROCESS_STACK_SIZE / 4 - 1];

    /* Push entry point address for process_wrapper */
    *sp-- = (uint32_t)entry;        /* Argument to wrapper */
    *sp-- = 0;                       /* Fake return address */
    *sp-- = (uint32_t)process_wrapper; /* EIP - entry point */
    *sp-- = 0x202;                   /* EFLAGS - interrupts enabled */
    *sp-- = 0;                       /* EAX */
    *sp-- = 0;                       /* ECX */
    *sp-- = 0;                       /* EDX */
    *sp-- = 0;                       /* EBX */
    *sp-- = 0;                       /* ESP (placeholder) */
    *sp-- = 0;                       /* EBP */
    *sp-- = 0;                       /* ESI */
    *sp = 0;                         /* EDI */

    p->stack_ptr = sp;

    return (int)p->pid;
}

/* Exit current process */
void process_exit(int exit_code) {
    if (!current_process) {
        return;
    }

    current_process->state = PROCESS_ZOMBIE;
    current_process->exit_code = exit_code;

    /* Yield to let scheduler pick another process */
    schedule();

    /* Should never reach here */
    while(1) {
        __asm__ volatile("hlt");
    }
}

/* Voluntarily yield CPU */
void process_yield(void) {
    schedule();
}

/* Get current process */
process_t *process_current(void) {
    return current_process;
}

/* Kill a process by PID */
int process_kill(uint32_t pid) {
    process_t *p = find_process(pid);
    if (!p) {
        return -1;
    }

    p->state = PROCESS_ZOMBIE;
    p->exit_code = -1;

    /* If killing current process, schedule another */
    if (p == current_process) {
        schedule();
    }

    return 0;
}

/* Clean up zombie processes */
static void cleanup_zombies(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_ZOMBIE) {
            process_table[i].state = PROCESS_UNUSED;
            process_table[i].pid = 0;
        }
    }
}

/* Simple round-robin scheduler */
void schedule(void) {
    if (!scheduler_enabled) {
        return;
    }

    cleanup_zombies();

    /* Find next runnable process */
    process_t *next = 0;
    int start = 0;

    if (current_process) {
        /* Start searching from after current process */
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (&process_table[i] == current_process) {
                start = i + 1;
                break;
            }
        }
    }

    /* Search for next ready process */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        int idx = (start + i) % MAX_PROCESSES;
        if (process_table[idx].state == PROCESS_READY) {
            next = &process_table[idx];
            break;
        }
    }

    /* If no process found, try to run current if it's still ready */
    if (!next && current_process && current_process->state == PROCESS_RUNNING) {
        return;  /* Keep running current */
    }

    if (!next) {
        /* No runnable processes - idle */
        return;
    }

    /* Perform context switch */
    process_t *old = current_process;

    if (old && old->state == PROCESS_RUNNING) {
        old->state = PROCESS_READY;
    }

    next->state = PROCESS_RUNNING;
    current_process = next;

    if (old && old != next) {
        context_switch(&old->stack_ptr, next->stack_ptr);
    } else if (!old) {
        /* First process to run - just switch to it */
        context_switch((uint32_t **)&old, next->stack_ptr);
    }
}

/* Initialize scheduler */
void scheduler_init(void) {
    scheduler_enabled = 1;
}

/* Called from timer interrupt */
void scheduler_tick(void) {
    if (!scheduler_enabled || !current_process) {
        return;
    }

    current_process->total_ticks++;

    if (current_process->time_slice > 0) {
        current_process->time_slice--;
    }

    if (current_process->time_slice == 0) {
        current_process->time_slice = 10;  /* Reset time slice */
        schedule();
    }
}

/* List all processes (for debugging/ps command) */
void process_list(void) {
    vga_puts("PID  STATE    NAME\n");
    vga_puts("---- -------- ----------------\n");

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROCESS_UNUSED) {
            vga_print_dec(process_table[i].pid);
            vga_puts("    ");

            switch (process_table[i].state) {
                case PROCESS_READY:   vga_puts("READY   "); break;
                case PROCESS_RUNNING: vga_puts("RUNNING "); break;
                case PROCESS_BLOCKED: vga_puts("BLOCKED "); break;
                case PROCESS_ZOMBIE:  vga_puts("ZOMBIE  "); break;
                default:              vga_puts("?       "); break;
            }

            vga_puts(" ");
            vga_puts(process_table[i].name);
            vga_puts("\n");
        }
    }
}
