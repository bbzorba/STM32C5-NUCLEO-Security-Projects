#include <stdint.h>
#include "../inc/uart.h"

extern int main(void);
extern void SystemInit(void);
void Reset_Handler(void);
void Default_Handler(void);
extern uint32_t _estack;

/* USART/UART IRQ handlers defined in uart.c — declared weak so the linker
   falls back to Default_Handler if uart.c is not linked. */
void USART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART4_IRQHandler(void)  __attribute__((weak, alias("Default_Handler")));
void UART5_IRQHandler(void)  __attribute__((weak, alias("Default_Handler")));
void USART6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

typedef void (*isr_handler_t)(void);

#ifndef STM32_IRQ_TABLE_SIZE
#define STM32_IRQ_TABLE_SIZE 128U
#endif

__attribute__((section(".isr_vector")))
isr_handler_t vector_table[16 + STM32_IRQ_TABLE_SIZE] = {
    [0] = (isr_handler_t)&_estack,
    [1] = Reset_Handler,
    [2] = Default_Handler,
    [3] = Default_Handler,
    [4] = Default_Handler,
    [5] = Default_Handler,
    [6] = Default_Handler,
    [11] = Default_Handler,
    [12] = Default_Handler,
    [14] = Default_Handler,
    [15] = Default_Handler,
    [16 + USART1_IRQn] = USART1_IRQHandler,
    [16 + USART2_IRQn] = USART2_IRQHandler,
    [16 + USART3_IRQn] = USART3_IRQHandler,
    [16 + UART4_IRQn] = UART4_IRQHandler,
    [16 + UART5_IRQn] = UART5_IRQHandler,
    [16 + USART6_IRQn] = USART6_IRQHandler,
};

void Reset_Handler(void) {
    SystemInit();
    main();
    while (1);
}

void Default_Handler(void) {
    while (1);
}