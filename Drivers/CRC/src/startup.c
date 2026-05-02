#include <stdint.h>

extern int main(void);
extern void SystemInit(void);
void Reset_Handler(void);
void Default_Handler(void);
void SecureFault_Handler(void);
void HardFault_Handler(void);

typedef void (*isr_handler_t)(void);

#ifndef STM32_IRQ_TABLE_SIZE
#define STM32_IRQ_TABLE_SIZE 80U
#endif

uint32_t SystemCoreClock = 48000000UL;

void SystemInit(void) {
    SystemCoreClock = 48000000UL;
}

/* ---------- IAR Compiler (iccarm) ---------- */
#if defined(__ICCARM__)

extern void *CSTACK$$Limit;

#pragma location = ".intvec"
const isr_handler_t __vector_table[16 + STM32_IRQ_TABLE_SIZE] = {
    [0]  = (isr_handler_t)&CSTACK$$Limit,
    [1]  = Reset_Handler,
    [2]  = Default_Handler,
    [3]  = HardFault_Handler,
    [4]  = Default_Handler,
    [5]  = Default_Handler,
    [6]  = Default_Handler,
    [7]  = SecureFault_Handler,
    [11] = Default_Handler,
    [12] = Default_Handler,
    [14] = Default_Handler,
    [15] = Default_Handler,
};

void Reset_Handler(void) {
    extern void __iar_data_init3(void);
    __iar_data_init3();

    SystemInit();
    main();
    while (1);
}

/* ---------- GCC / arm-none-eabi-gcc ---------- */
#else

extern uint32_t _estack;

/* Linker-provided symbols for .data and .bss initialization */
extern uint32_t __etext;        /* end of .text (LMA of .data in flash) */
extern uint32_t __data_start__;  /* start of .data in RAM */
extern uint32_t __data_end__;    /* end of .data in RAM */
extern uint32_t __bss_start__;   /* start of .bss */
extern uint32_t __bss_end__;     /* end of .bss */

__attribute__((section(".isr_vector")))
isr_handler_t vector_table[16 + STM32_IRQ_TABLE_SIZE] = {
    [0]  = (isr_handler_t)&_estack,
    [1]  = Reset_Handler,
    [2]  = Default_Handler,
    [3]  = HardFault_Handler,
    [4]  = Default_Handler,
    [5]  = Default_Handler,
    [6]  = Default_Handler,
    [7]  = SecureFault_Handler,
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

#endif /* __ICCARM__ */

void Default_Handler(void) {
    while (1);
}

void HardFault_Handler(void) {
    while (1);
}

void SecureFault_Handler(void) {
    /* SAES NS access triggers SecureFault (vector[7]).
     * Distinct symbol makes it identifiable in the debugger. */
    while (1);
}