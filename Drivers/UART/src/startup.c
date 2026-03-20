#include <stdint.h>

extern int main(void);
extern void SystemInit(void);
void Reset_Handler(void);
void Default_Handler(void);
extern uint32_t _estack;

/* Linker-provided symbols for .data and .bss initialization */
extern uint32_t __etext;        /* end of .text (LMA of .data in flash) */
extern uint32_t __data_start__;  /* start of .data in RAM */
extern uint32_t __data_end__;    /* end of .data in RAM */
extern uint32_t __bss_start__;   /* start of .bss */
extern uint32_t __bss_end__;     /* end of .bss */

typedef void (*isr_handler_t)(void);

#ifndef STM32_IRQ_TABLE_SIZE
#define STM32_IRQ_TABLE_SIZE 64U
#endif

uint32_t SystemCoreClock = 16000000UL; // Default HSI frequency

void SystemInit(void) {
    // Default: HSI, no PLL
    SystemCoreClock = 16000000UL;
}

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
    /* Copy .data section from flash (LMA) to RAM (VMA) */
    uint32_t *src = &__etext;
    uint32_t *dst = &__data_start__;
    while (dst < &__data_end__) {
        *dst++ = *src++;
    }

    /* Zero-fill .bss section */
    dst = &__bss_start__;
    while (dst < &__bss_end__) {
        *dst++ = 0;
    }

    SystemInit();
    main();
    while (1);
}

void Default_Handler(void) {
    while (1);
}