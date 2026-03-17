#include <stdint.h>

extern int main(void);
extern void SystemInit(void);
void Reset_Handler(void);
void Default_Handler(void);
extern uint32_t _estack;

typedef void (*isr_handler_t)(void);

#ifndef STM32_IRQ_TABLE_SIZE
#define STM32_IRQ_TABLE_SIZE 64U
#endif

__attribute__((section(".isr_vector")))
isr_handler_t vector_table[16 + STM32_IRQ_TABLE_SIZE] = {
    [0]  = (isr_handler_t)&_estack,
    [1]  = Reset_Handler,
    [2]  = Default_Handler,
    [3]  = Default_Handler,
    [4]  = Default_Handler,
    [5]  = Default_Handler,
    [6]  = Default_Handler,
    [11] = Default_Handler,
    [12] = Default_Handler,
    [14] = Default_Handler,
    [15] = Default_Handler,
};

void Reset_Handler(void) {
    SystemInit();
    main();
    while (1);
}

void Default_Handler(void) {
    while (1);
}