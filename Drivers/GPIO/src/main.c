#include "../inc/gpio.h"
#define delay 200000

GPIO_InitTypeDef GPIO_InitStruct;

// Simple delay function
void delay_fn(volatile int count) {
    for (volatile int i = 0; i < count; i++);
}

int main(void) {
    // Configure GPIO_InitStruct for output pins
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // Push-pull output mode
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // No pull-up/pull-down
    GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;   // Medium speed
    
    GPIO_HandleTypeDef GPIO_LEDS;
    GPIO_constructor(&GPIO_LEDS, GPIO_D, &GPIO_InitStruct);

    while(1) {
        //GPIOD_TogglePins(delay);
        GPIO_TogglePin(&GPIO_LEDS, GPIO_PIN_15);
        delay_fn(delay);
        GPIO_TogglePin(&GPIO_LEDS, GPIO_PIN_14);
        delay_fn(delay);
        GPIO_TogglePin(&GPIO_LEDS, GPIO_PIN_13);
        delay_fn(delay);
        GPIO_TogglePin(&GPIO_LEDS, GPIO_PIN_12);
        delay_fn(delay);
    }
}

