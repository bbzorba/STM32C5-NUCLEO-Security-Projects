#include <stdint.h>

extern int main(void);
extern void SystemInit(void);
void Reset_Handler(void);
void Default_Handler(void);

typedef void (*isr_handler_t)(void);

/* Need at least 56 entries for UART5_IRQn = 55 */
#ifndef STM32_IRQ_TABLE_SIZE
#define STM32_IRQ_TABLE_SIZE 64U
#endif

/* STM32C562RE boots from HSI = 48 MHz (144 MHz oscillator / 3) */
uint32_t SystemCoreClock = 48000000UL;

void SystemInit(void) {
    SystemCoreClock = 48000000UL;
}

/* ---------- IAR Compiler (iccarm) ---------- */
#if defined(__ICCARM__)

#pragma weak USART1_IRQHandler = Default_Handler
#pragma weak USART2_IRQHandler = Default_Handler
#pragma weak USART3_IRQHandler = Default_Handler
#pragma weak UART4_IRQHandler  = Default_Handler
#pragma weak UART5_IRQHandler  = Default_Handler

extern void USART1_IRQHandler(void);
extern void USART2_IRQHandler(void);
extern void USART3_IRQHandler(void);
extern void UART4_IRQHandler(void);
extern void UART5_IRQHandler(void);

extern void *CSTACK$$Limit;

#pragma location = ".intvec"
const isr_handler_t __vector_table[16 + STM32_IRQ_TABLE_SIZE] = {
    [0]  = (isr_handler_t)&CSTACK$$Limit,
    [1]  = Reset_Handler,
    [2]  = Default_Handler,   /* NMI */
    [3]  = Default_Handler,   /* HardFault */
    [4]  = Default_Handler,   /* MemManage */
    [5]  = Default_Handler,   /* BusFault */
    [6]  = Default_Handler,   /* UsageFault */
    [11] = Default_Handler,   /* SVCall */
    [12] = Default_Handler,   /* DebugMon */
    [14] = Default_Handler,   /* PendSV */
    [15] = Default_Handler,   /* SysTick */
    /* STM32C562RE USART/UART IRQs */
    [16 + 51] = USART1_IRQHandler,
    [16 + 52] = USART2_IRQHandler,
    [16 + 53] = USART3_IRQHandler,
    [16 + 54] = UART4_IRQHandler,
    [16 + 55] = UART5_IRQHandler,
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

/* Weak aliases so that these handlers resolve to Default_Handler
   unless overridden by strong definitions (e.g. in uart.c) */
__attribute__((weak, alias("Default_Handler"))) void USART1_IRQHandler(void);
__attribute__((weak, alias("Default_Handler"))) void USART2_IRQHandler(void);
__attribute__((weak, alias("Default_Handler"))) void USART3_IRQHandler(void);
__attribute__((weak, alias("Default_Handler"))) void UART4_IRQHandler(void);
__attribute__((weak, alias("Default_Handler"))) void UART5_IRQHandler(void);

__attribute__((section(".isr_vector")))
isr_handler_t vector_table[16 + STM32_IRQ_TABLE_SIZE] = {
    [0]  = (isr_handler_t)&_estack,
    [1]  = Reset_Handler,
    [2]  = Default_Handler,   /* NMI */
    [3]  = Default_Handler,   /* HardFault */
    [4]  = Default_Handler,   /* MemManage */
    [5]  = Default_Handler,   /* BusFault */
    [6]  = Default_Handler,   /* UsageFault */
    [11] = Default_Handler,   /* SVCall */
    [12] = Default_Handler,   /* DebugMon */
    [14] = Default_Handler,   /* PendSV */
    [15] = Default_Handler,   /* SysTick */
    /* STM32C562RE USART/UART IRQs */
    [16 + 51] = USART1_IRQHandler,
    [16 + 52] = USART2_IRQHandler,
    [16 + 53] = USART3_IRQHandler,
    [16 + 54] = UART4_IRQHandler,
    [16 + 55] = UART5_IRQHandler,
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