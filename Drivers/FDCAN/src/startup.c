#include <stdint.h>

extern int main(void);
extern void SystemInit(void);
void Reset_Handler(void);
void Default_Handler(void);

typedef void (*isr_handler_t)(void);

#ifndef STM32_IRQ_TABLE_SIZE
#define STM32_IRQ_TABLE_SIZE 128U
#endif

uint32_t SystemCoreClock = 48000000UL;

void SystemInit(void) { SystemCoreClock = 48000000UL; }

/* IRQ handler declarations — strong definitions in Drivers/NVIC/src/nvic.c.
   bxCAN project: EXTI lines (GPIO interrupts) + FDCAN1 interrupt lines. */
extern void EXTI0_IRQHandler(void);  extern void EXTI1_IRQHandler(void);
extern void EXTI2_IRQHandler(void);  extern void EXTI3_IRQHandler(void);
extern void EXTI4_IRQHandler(void);  extern void EXTI5_IRQHandler(void);
extern void EXTI6_IRQHandler(void);  extern void EXTI7_IRQHandler(void);
extern void EXTI8_IRQHandler(void);  extern void EXTI9_IRQHandler(void);
extern void EXTI10_IRQHandler(void); extern void EXTI11_IRQHandler(void);
extern void EXTI12_IRQHandler(void); extern void EXTI13_IRQHandler(void);
extern void EXTI14_IRQHandler(void); extern void EXTI15_IRQHandler(void);
extern void FDCAN1_IT0_IRQHandler(void);
extern void FDCAN1_IT1_IRQHandler(void);

#define VECTOR_TABLE_CONTENT                                              \
    [0]  = _VT_STACK_TOP,                                                \
    [1]  = Reset_Handler,                                                \
    [2]  = Default_Handler, [3]  = Default_Handler,                      \
    [4]  = Default_Handler, [5]  = Default_Handler,                      \
    [6]  = Default_Handler, [11] = Default_Handler,                      \
    [12] = Default_Handler, [14] = Default_Handler,                      \
    [15] = Default_Handler,                                              \
    [16 +  7] = EXTI0_IRQHandler,  [16 +  8] = EXTI1_IRQHandler,        \
    [16 +  9] = EXTI2_IRQHandler,  [16 + 10] = EXTI3_IRQHandler,        \
    [16 + 11] = EXTI4_IRQHandler,  [16 + 12] = EXTI5_IRQHandler,        \
    [16 + 13] = EXTI6_IRQHandler,  [16 + 14] = EXTI7_IRQHandler,        \
    [16 + 15] = EXTI8_IRQHandler,  [16 + 16] = EXTI9_IRQHandler,        \
    [16 + 17] = EXTI10_IRQHandler, [16 + 18] = EXTI11_IRQHandler,       \
    [16 + 19] = EXTI12_IRQHandler, [16 + 20] = EXTI13_IRQHandler,       \
    [16 + 21] = EXTI14_IRQHandler, [16 + 22] = EXTI15_IRQHandler,       \
    [16 + 34] = FDCAN1_IT0_IRQHandler,                                   \
    [16 + 35] = FDCAN1_IT1_IRQHandler

#if defined(__ICCARM__)
extern void *CSTACK$$Limit;
#define _VT_STACK_TOP  ((isr_handler_t)&CSTACK$$Limit)
#pragma location = ".intvec"
const isr_handler_t vector_table[16 + STM32_IRQ_TABLE_SIZE] = { VECTOR_TABLE_CONTENT };
void Reset_Handler(void) {
    extern void __iar_data_init3(void); __iar_data_init3();
    SystemInit(); main(); while (1);
}
#else
extern uint32_t _estack, __etext, __data_start__, __data_end__, __bss_start__, __bss_end__;
#define _VT_STACK_TOP  ((isr_handler_t)&_estack)
__attribute__((section(".isr_vector")))
isr_handler_t vector_table[16 + STM32_IRQ_TABLE_SIZE] = { VECTOR_TABLE_CONTENT };
void Reset_Handler(void) {
    uint32_t *src = &__etext, *dst = &__data_start__;
    while (dst < &__data_end__)  { *dst++ = *src++; }
    dst = &__bss_start__;
    while (dst < &__bss_end__)   { *dst++ = 0; }
    SystemInit(); main(); while (1);
}
#endif

void Default_Handler(void) { while (1); }
