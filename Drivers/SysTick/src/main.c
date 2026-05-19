#include "../inc/systick.h"
#include "../../../Drivers/UART/inc/uart.h"
#include <stdio.h>

GPIO_InitTypeDef GPIO_InitStruct;
GPIO_HandleTypeDef LED_Handle;

//main function
int main(void) {
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT; // Set as output
    GPIO_InitStruct.Pin = GPIO_PIN_5; // Pin 5 (PA5)
    GPIO_InitStruct.Pull = GPIO_NOPULL; // No pull-up or pull-down
    GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;   // Medium speed
    
    GPIO_constructor(&LED_Handle, GPIO_A, &GPIO_InitStruct);

    USART_HandleType UART_Handle;
    USART_constructor(&UART_Handle, USART_2, TX_ONLY, __115200);

    SysTick_HandleTypeDef SysTickHandle;
    SysTick_constructor(&SysTickHandle, SysTick, SYSTICK_OK);

    GPIO_TogglePin(&LED_Handle, GPIO_PIN_5); // Toggle PA5

    SysTick_StartTimer(&SysTickHandle);
    SysTick_delay_ms_irq(&SysTickHandle, 1000); // 1 second via interrupt (s_tick_ms increments)
    
    while (1) {
        GPIO_TogglePin(&LED_Handle, GPIO_PIN_5); // Toggle PA5
        SysTick_delay_ms_irq(&SysTickHandle, 500); // Delay 500 ms via SysTick interrupt

        uint32_t elapsed_time_us = SysTick_GetElapsedTime_us(&SysTickHandle);
        USART_WriteString(&UART_Handle, "Elapsed (us): ");
        char buffer[12];
        sprintf(buffer, "%lu", elapsed_time_us);
        USART_WriteString(&UART_Handle, buffer);
        USART_WriteString(&UART_Handle, "\r\n");
    }
}
