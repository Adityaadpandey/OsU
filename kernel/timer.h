/*
 * timer.h - Programmable Interval Timer (PIT) driver
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* PIT frequency (1193182 Hz base frequency) */
#define PIT_BASE_FREQ 1193182

/* Default tick rate: 100 Hz (10ms per tick) */
#define TIMER_FREQ 100

/* Initialize timer with given frequency */
void timer_init(uint32_t frequency);

/* Get tick count since boot */
uint32_t timer_get_ticks(void);

/* Sleep for approximately ms milliseconds */
void timer_sleep(uint32_t ms);

#endif /* TIMER_H */
