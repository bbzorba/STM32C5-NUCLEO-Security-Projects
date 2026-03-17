#include "../inc/mutex.h"
#include "../inc/button_led.h"  /* provides __disable_irq() & __enable_irq() via CMSIS chain */

volatile uint8_t g_mutex          = 0;  /* 0 = free, 1 = held */
volatile uint8_t g_async_requested = 0;  /* set by EXTI0 ISR  */

/* Non-blocking try: returns 1 if acquired, 0 if busy.
 * Uses a critical section so the read-modify-write is atomic. */
int mutex_try_acquire(void) {
    int acquired;
    __disable_irq();
    acquired = (g_mutex == 0);
    if (acquired) g_mutex = 1;
    __enable_irq();
    return acquired;
}

/* Blocking acquire: spins until the mutex is free. */
void mutex_acquire(void) {
    while (!mutex_try_acquire()) {}
}

void mutex_release(void) {
    __disable_irq();
    g_mutex = 0;
    __enable_irq();
}
