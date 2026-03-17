#ifndef MUTEX_H
#define MUTEX_H

#include <stdint.h>

/* Shared state: defined in mutex.c, visible to button_led.c (ISR & blink). */
extern volatile uint8_t g_mutex;
extern volatile uint8_t g_async_requested;

/* Non-blocking try: returns 1 if acquired, 0 if busy. */
int  mutex_try_acquire(void);

/* Blocking acquire: spins until the mutex is free. */
void mutex_acquire(void);

void mutex_release(void);

#endif // MUTEX_H
