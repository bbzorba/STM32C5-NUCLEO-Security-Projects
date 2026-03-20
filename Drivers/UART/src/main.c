#include "../inc/gpio.h"

/*
 * NUCLEO-C562RE onboard user LED:
 *   LD1 Green = PA5 (the only user LED on this board)
 */
#define DELAY_CYCLES 400000

void delay_fn(volatile int count) {
    for (volatile int i = 0; i < count; i++);
}

int main(void) {
    GPIO_InitTypeDef led_init;
    led_init.Mode      = GPIO_MODE_OUTPUT_PP;
    led_init.Pull      = GPIO_NOPULL;
    led_init.Speed     = GPIO_SPEED_MEDIUM;
    led_init.Alternate = 0;

    /* LD1 Green — PA5 (the only user LED on NUCLEO-C562RE) */
    GPIO_HandleTypeDef ld1_green;
    led_init.Pin = GPIO_PIN_5;
    GPIO_constructor(&ld1_green, GPIO_A, &led_init);

    while (1) {
        GPIO_TogglePin(&ld1_green, GPIO_PIN_5);
        delay_fn(DELAY_CYCLES);
    }
}
