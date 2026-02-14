/*
 * timer.c - PIT (Programmable Interval Timer) driver
 * Used for preemptive scheduling and time tracking
 */

#include "timer.h"
#include "idt.h"
#include "io.h"
#include "process.h"
#include "vga.h"

/* PIT ports */
#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND  0x43

/* Tick counter */
static volatile uint32_t tick_count = 0;
static uint32_t tick_frequency = TIMER_FREQ;

/* Timer interrupt handler */
static void timer_irq_handler(registers_t *r) {
    (void)r;
    tick_count++;

    /* Call scheduler tick for preemptive scheduling */
    scheduler_tick();
}

/* Initialize the PIT */
void timer_init(uint32_t frequency) {
    tick_frequency = frequency;

    /* Calculate divisor */
    uint32_t divisor = PIT_BASE_FREQ / frequency;

    /* Limit divisor to 16-bit value */
    if (divisor > 65535) {
        divisor = 65535;
    }
    if (divisor < 1) {
        divisor = 1;
    }

    /* Set PIT to mode 3 (square wave generator) */
    /* Channel 0, Access mode: lobyte/hibyte, Mode 3 */
    outb(PIT_COMMAND, 0x36);

    /* Send divisor */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));         /* Low byte */
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));  /* High byte */

    /* Register IRQ0 handler */
    idt_register_handler(32, timer_irq_handler);
}

/* Get tick count */
uint32_t timer_get_ticks(void) {
    return tick_count;
}

/* Sleep for approximately ms milliseconds */
void timer_sleep(uint32_t ms) {
    uint32_t ticks = (ms * tick_frequency) / 1000;
    if (ticks == 0) ticks = 1;

    uint32_t start = tick_count;
    while (tick_count - start < ticks) {
        __asm__ volatile("sti; hlt");
    }
}
