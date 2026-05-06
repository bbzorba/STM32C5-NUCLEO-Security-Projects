#include <stdint.h>

extern int main(void);
extern void SystemInit(void);
void Reset_Handler(void);
void Default_Handler(void);

typedef void (*isr_handler_t)(void);

/* Highest peripheral IRQ index used: ADC3=98 -> table index 16+98=114.
   128 entries -> size 144, covers all current and future peripherals. */
#ifndef STM32_IRQ_TABLE_SIZE
#define STM32_IRQ_TABLE_SIZE 128U
#endif

uint32_t SystemCoreClock = 48000000UL;

void SystemInit(void) {
    SystemCoreClock = 48000000UL;
}

/* ── IRQ handler forward declarations (identical for IAR and GCC) ─────── */
/* nvic.c provides the strong definitions for all of these. */
extern void EXTI0_IRQHandler(void);
extern void EXTI1_IRQHandler(void);
extern void EXTI2_IRQHandler(void);
extern void EXTI3_IRQHandler(void);
extern void EXTI4_IRQHandler(void);
extern void EXTI5_IRQHandler(void);
extern void EXTI6_IRQHandler(void);
extern void EXTI7_IRQHandler(void);
extern void EXTI8_IRQHandler(void);
extern void EXTI9_IRQHandler(void);
extern void EXTI10_IRQHandler(void);
extern void EXTI11_IRQHandler(void);
extern void EXTI12_IRQHandler(void);
extern void EXTI13_IRQHandler(void);
extern void EXTI14_IRQHandler(void);
extern void EXTI15_IRQHandler(void);
extern void LPDMA1_Channel0_IRQHandler(void);
extern void LPDMA1_Channel1_IRQHandler(void);
extern void LPDMA1_Channel2_IRQHandler(void);
extern void LPDMA1_Channel3_IRQHandler(void);
extern void LPDMA1_Channel4_IRQHandler(void);
extern void LPDMA1_Channel5_IRQHandler(void);
extern void LPDMA1_Channel6_IRQHandler(void);
extern void LPDMA1_Channel7_IRQHandler(void);
extern void IWDG1_IRQHandler(void);
extern void ADC1_IRQHandler(void);
extern void ADC2_IRQHandler(void);
extern void FDCAN1_IT0_IRQHandler(void);
extern void FDCAN1_IT1_IRQHandler(void);
extern void SPI1_IRQHandler(void);
extern void SPI2_IRQHandler(void);
extern void SPI3_IRQHandler(void);
extern void USART1_IRQHandler(void);
extern void USART2_IRQHandler(void);
extern void USART3_IRQHandler(void);
extern void UART4_IRQHandler(void);
extern void UART5_IRQHandler(void);
extern void HASH_IRQHandler(void);
extern void USART6_IRQHandler(void);
extern void UART7_IRQHandler(void);
extern void ADC3_IRQHandler(void);
extern void RNG_IRQHandler(void);

/*
 * VECTOR_TABLE_CONTENT — shared entries for both IAR and GCC tables.
 *
 * _VT_STACK_TOP is defined per-compiler before this macro is expanded:
 *   IAR : (isr_handler_t)&CSTACK$$Limit
 *   GCC : (isr_handler_t)&_estack
 */
#define VECTOR_TABLE_CONTENT                                          \
    [0]  = _VT_STACK_TOP,                                            \
    [1]  = Reset_Handler,                                            \
    [2]  = Default_Handler,   /* NMI               */               \
    [3]  = Default_Handler,   /* HardFault         */               \
    [4]  = Default_Handler,   /* MemManage         */               \
    [5]  = Default_Handler,   /* BusFault          */               \
    [6]  = Default_Handler,   /* UsageFault        */               \
    [11] = Default_Handler,   /* SVCall            */               \
    [12] = Default_Handler,   /* DebugMon          */               \
    [14] = Default_Handler,   /* PendSV            */               \
    [15] = Default_Handler,   /* SysTick           */               \
    /* EXTI line IRQs (lines 0-15, IRQn 7-22) */                    \
    [16 +  7] = EXTI0_IRQHandler,                                    \
    [16 +  8] = EXTI1_IRQHandler,                                    \
    [16 +  9] = EXTI2_IRQHandler,                                    \
    [16 + 10] = EXTI3_IRQHandler,                                    \
    [16 + 11] = EXTI4_IRQHandler,                                    \
    [16 + 12] = EXTI5_IRQHandler,                                    \
    [16 + 13] = EXTI6_IRQHandler,                                    \
    [16 + 14] = EXTI7_IRQHandler,                                    \
    [16 + 15] = EXTI8_IRQHandler,                                    \
    [16 + 16] = EXTI9_IRQHandler,                                    \
    [16 + 17] = EXTI10_IRQHandler,                                   \
    [16 + 18] = EXTI11_IRQHandler,                                   \
    [16 + 19] = EXTI12_IRQHandler,                                   \
    [16 + 20] = EXTI13_IRQHandler,                                   \
    [16 + 21] = EXTI14_IRQHandler,                                   \
    [16 + 22] = EXTI15_IRQHandler,                                   \
    /* LPDMA1 channel IRQs (IRQn 23-30) */                          \
    [16 + 23] = LPDMA1_Channel0_IRQHandler,                         \
    [16 + 24] = LPDMA1_Channel1_IRQHandler,                         \
    [16 + 25] = LPDMA1_Channel2_IRQHandler,                         \
    [16 + 26] = LPDMA1_Channel3_IRQHandler,                         \
    [16 + 27] = LPDMA1_Channel4_IRQHandler,                         \
    [16 + 28] = LPDMA1_Channel5_IRQHandler,                         \
    [16 + 29] = LPDMA1_Channel6_IRQHandler,                         \
    [16 + 30] = LPDMA1_Channel7_IRQHandler,                         \
    /* IWDG (IRQn 31) */                                             \
    [16 + 31] = IWDG1_IRQHandler,                                    \
    /* ADC (IRQn 32-33, 98) */                                       \
    [16 + 32] = ADC1_IRQHandler,                                     \
    [16 + 33] = ADC2_IRQHandler,                                     \
    [16 + 98] = ADC3_IRQHandler,                                     \
    /* FDCAN1 (IRQn 34-35) */                                        \
    [16 + 34] = FDCAN1_IT0_IRQHandler,                               \
    [16 + 35] = FDCAN1_IT1_IRQHandler,                               \
    /* SPI (IRQn 48-50) */                                           \
    [16 + 48] = SPI1_IRQHandler,                                     \
    [16 + 49] = SPI2_IRQHandler,                                     \
    [16 + 50] = SPI3_IRQHandler,                                     \
    /* USART/UART (IRQn 51-55, 96-97) */                            \
    [16 + 51] = USART1_IRQHandler,                                   \
    [16 + 52] = USART2_IRQHandler,                                   \
    [16 + 53] = USART3_IRQHandler,                                   \
    [16 + 54] = UART4_IRQHandler,                                    \
    [16 + 55] = UART5_IRQHandler,                                    \
    [16 + 96] = USART6_IRQHandler,                                   \
    [16 + 97] = UART7_IRQHandler,                                    \
    /* RNG (IRQn 70) */                                             \
    [16 + 64] = RNG_IRQHandler,                                     \
    /* HASH (IRQn 69) */                                             \
    [16 + 69] = HASH_IRQHandler,                                    \

/* ── IAR Compiler (iccarm) ─────────────────────────────────────────────── */
#if defined(__ICCARM__)

extern void *CSTACK$$Limit;
#define _VT_STACK_TOP  ((isr_handler_t)&CSTACK$$Limit)

#pragma location = ".intvec"
const isr_handler_t vector_table[16 + STM32_IRQ_TABLE_SIZE] = {
    VECTOR_TABLE_CONTENT
};

void Reset_Handler(void) {
    extern void __iar_data_init3(void);
    __iar_data_init3();
    SystemInit();
    main();
    while (1);
}

/* ── GCC / arm-none-eabi-gcc ────────────────────────────────────────────── */
#else

extern uint32_t _estack;
extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;

#define _VT_STACK_TOP  ((isr_handler_t)&_estack)

__attribute__((section(".isr_vector")))
isr_handler_t vector_table[16 + STM32_IRQ_TABLE_SIZE] = {
    VECTOR_TABLE_CONTENT
};

void Reset_Handler(void) {
    uint32_t *src = &__etext;
    uint32_t *dst = &__data_start__;
    while (dst < &__data_end__)  { *dst++ = *src++; }
    dst = &__bss_start__;
    while (dst < &__bss_end__)   { *dst++ = 0; }
    SystemInit();
    main();
    while (1);
}

#endif /* __ICCARM__ */

void Default_Handler(void) {
    while (1);
}