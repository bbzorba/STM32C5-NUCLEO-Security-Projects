#include "../inc/systick.h"

GPIO_InitTypeDef GPIO_InitStruct;
GPIO_HandleTypeDef LED_Handle;

//main function
int main(void) {
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT; // Set as output
    GPIO_InitStruct.Pin = GPIO_PIN_5; // Pin 5 (PA5)
    GPIO_InitStruct.Pull = GPIO_NOPULL; // No pull-up or pull-down
    GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;   // Medium speed
    
    GPIO_constructor(&LED_Handle, GPIO_A, &GPIO_InitStruct);

    SysTick_HandleTypeDef SysTickHandle;
    SysTick_constructor(&SysTickHandle, SysTick, SYSTICK_OK);

    SysTick_delay(&SysTickHandle, 1); // Delay for 1 second (polling)
    
    while (1) {
        GPIO_TogglePin(&LED_Handle, GPIO_PIN_5); // Toggle PA5
        SysTick_delay_ms_irq(&SysTickHandle, 500); // Delay 500 ms via SysTick interrupt
    }
}
