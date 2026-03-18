#include "../inc/gpio.h"

/*
 * NUCLEO-C562RE onboard user LEDs:
 *   LD1 Green = PB0
 *   LD2 Blue  = PB7
 *   LD3 Red   = PB14
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

    /* LD1 Green — PB0 */
    GPIO_HandleTypeDef ld1_green;
    led_init.Pin = GPIO_PIN_0;
    GPIO_constructor(&ld1_green, GPIO_B, &led_init);

    /* LD2 Blue — PB7 */
    GPIO_HandleTypeDef ld2_blue;
    led_init.Pin = GPIO_PIN_7;
    GPIO_constructor(&ld2_blue, GPIO_B, &led_init);

    /* LD3 Red — PB14 */
    GPIO_HandleTypeDef ld3_red;
    led_init.Pin = GPIO_PIN_14;
    GPIO_constructor(&ld3_red, GPIO_B, &led_init);

    while (1) {
        GPIO_TogglePin(&ld1_green, GPIO_PIN_0);
        delay_fn(DELAY_CYCLES);
        GPIO_TogglePin(&ld2_blue,  GPIO_PIN_7);
        delay_fn(DELAY_CYCLES);
        GPIO_TogglePin(&ld3_red,   GPIO_PIN_14);
        delay_fn(DELAY_CYCLES);
    }
}


