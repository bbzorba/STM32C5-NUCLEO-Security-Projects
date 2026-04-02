#include "stm32f4xx.h"

extern int main(void);
void Reset_Handler(void);
void Default_Handler(void);

extern unsigned long __etext;
extern unsigned long __data_start__;
extern unsigned long __data_end__;
extern unsigned long __bss_start__;
extern unsigned long __bss_end__;

/* ---------- IAR Compiler (iccarm) ---------- */
#if defined(__ICCARM__)

#pragma weak EXTI0_IRQHandler     = Default_Handler
#pragma weak EXTI1_IRQHandler     = Default_Handler
#pragma weak EXTI2_IRQHandler     = Default_Handler
#pragma weak EXTI3_IRQHandler     = Default_Handler
#pragma weak EXTI4_IRQHandler     = Default_Handler
#pragma weak EXTI9_5_IRQHandler   = Default_Handler
#pragma weak EXTI15_10_IRQHandler = Default_Handler
#pragma weak TIM2_IRQHandler      = Default_Handler
#pragma weak TIM3_IRQHandler      = Default_Handler
#pragma weak TIM4_IRQHandler      = Default_Handler
#pragma weak USART1_IRQHandler    = Default_Handler
#pragma weak USART2_IRQHandler    = Default_Handler
#pragma weak USART3_IRQHandler    = Default_Handler

extern void EXTI0_IRQHandler(void);
extern void EXTI1_IRQHandler(void);
extern void EXTI2_IRQHandler(void);
extern void EXTI3_IRQHandler(void);
extern void EXTI4_IRQHandler(void);
extern void EXTI9_5_IRQHandler(void);
extern void EXTI15_10_IRQHandler(void);
extern void TIM2_IRQHandler(void);
extern void TIM3_IRQHandler(void);
extern void TIM4_IRQHandler(void);
extern void USART1_IRQHandler(void);
extern void USART2_IRQHandler(void);
extern void USART3_IRQHandler(void);

extern void *CSTACK$$Limit;

#pragma location = ".intvec"
void (* const __vector_table[])(void) = {
    (void (*)(void))&CSTACK$$Limit,
    Reset_Handler,              // Reset handler
    Default_Handler,            // NMI
    Default_Handler,            // HardFault
    Default_Handler,            // MemManage
    Default_Handler,            // BusFault
    Default_Handler,            // UsageFault
    0, 0, 0, 0,                 // Reserved
    Default_Handler,            // SVCall
    Default_Handler,            // DebugMonitor
    0,                          // Reserved
    Default_Handler,            // PendSV
    Default_Handler,            // SysTick
    /* Peripheral IRQs — must match the STM32F407 vector table exactly. */
    Default_Handler,            // IRQ0:  WWDG
    Default_Handler,            // IRQ1:  PVD
    Default_Handler,            // IRQ2:  TAMP_STAMP
    Default_Handler,            // IRQ3:  RTC_WKUP
    Default_Handler,            // IRQ4:  FLASH
    Default_Handler,            // IRQ5:  RCC
    EXTI0_IRQHandler,           // IRQ6:  EXTI0
    EXTI1_IRQHandler,           // IRQ7:  EXTI1
    EXTI2_IRQHandler,           // IRQ8:  EXTI2
    EXTI3_IRQHandler,           // IRQ9:  EXTI3
    EXTI4_IRQHandler,           // IRQ10: EXTI4
    Default_Handler,            // IRQ11: DMA1_Stream0
    Default_Handler,            // IRQ12: DMA1_Stream1
    Default_Handler,            // IRQ13: DMA1_Stream2
    Default_Handler,            // IRQ14: DMA1_Stream3
    Default_Handler,            // IRQ15: DMA1_Stream4
    Default_Handler,            // IRQ16: DMA1_Stream5
    Default_Handler,            // IRQ17: DMA1_Stream6
    Default_Handler,            // IRQ18: ADC
    Default_Handler,            // IRQ19: CAN1_TX
    Default_Handler,            // IRQ20: CAN1_RX0
    Default_Handler,            // IRQ21: CAN1_RX1
    Default_Handler,            // IRQ22: CAN1_SCE
    EXTI9_5_IRQHandler,         // IRQ23: EXTI9_5
    Default_Handler,            // IRQ24: TIM1_BRK / TIM9
    Default_Handler,            // IRQ25: TIM1_UP  / TIM10
    Default_Handler,            // IRQ26: TIM1_TRG / TIM11
    Default_Handler,            // IRQ27: TIM1_CC
    TIM2_IRQHandler,            // IRQ28: TIM2
    TIM3_IRQHandler,            // IRQ29: TIM3
    TIM4_IRQHandler,            // IRQ30: TIM4
    Default_Handler,            // IRQ31: I2C1_EV
    Default_Handler,            // IRQ32: I2C1_ER
    Default_Handler,            // IRQ33: I2C2_EV
    Default_Handler,            // IRQ34: I2C2_ER
    Default_Handler,            // IRQ35: SPI1
    Default_Handler,            // IRQ36: SPI2
    USART1_IRQHandler,          // IRQ37: USART1
    USART2_IRQHandler,          // IRQ38: USART2
    USART3_IRQHandler,          // IRQ39: USART3
    EXTI15_10_IRQHandler,       // IRQ40: EXTI15_10
};

void Reset_Handler(void) {
    extern void __iar_data_init3(void);
    __iar_data_init3();

    SystemInit();
    main();
    while (1) {}
}

/* ---------- GCC / arm-none-eabi-gcc ---------- */
#else

/* Weak aliases: each handler falls back to Default_Handler unless the
 * application defines its own (e.g. EXTI0_IRQHandler in button_led.c).
 * Adding a new interrupt only requires defining the matching handler
 * function — no startup.c edit needed.                              */
void EXTI0_IRQHandler    (void) __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler    (void) __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler    (void) __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler    (void) __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler    (void) __attribute__((weak, alias("Default_Handler")));
void EXTI9_5_IRQHandler  (void) __attribute__((weak, alias("Default_Handler")));
void EXTI15_10_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler     (void) __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler     (void) __attribute__((weak, alias("Default_Handler")));
void TIM4_IRQHandler     (void) __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler   (void) __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler   (void) __attribute__((weak, alias("Default_Handler")));
void USART3_IRQHandler   (void) __attribute__((weak, alias("Default_Handler")));

__attribute__((section(".isr_vector")))
void (* const vector_table[])(void) = {
    (void (*)(void))0x20020000, // Initial stack pointer (128KB SRAM)
    Reset_Handler,              // Reset handler
    Default_Handler,            // NMI
    Default_Handler,            // HardFault
    Default_Handler,            // MemManage
    Default_Handler,            // BusFault
    Default_Handler,            // UsageFault
    0, 0, 0, 0,                 // Reserved
    Default_Handler,            // SVCall
    Default_Handler,            // DebugMonitor
    0,                          // Reserved
    Default_Handler,            // PendSV
    Default_Handler,            // SysTick
    /* Peripheral IRQs — must match the STM32F407 vector table exactly.
     * Every slot up to the highest IRQ used must be present.        */
    Default_Handler,            // IRQ0:  WWDG
    Default_Handler,            // IRQ1:  PVD
    Default_Handler,            // IRQ2:  TAMP_STAMP
    Default_Handler,            // IRQ3:  RTC_WKUP
    Default_Handler,            // IRQ4:  FLASH
    Default_Handler,            // IRQ5:  RCC
    EXTI0_IRQHandler,           // IRQ6:  EXTI0  (pins x0 of any port)
    EXTI1_IRQHandler,           // IRQ7:  EXTI1  (pins x1)
    EXTI2_IRQHandler,           // IRQ8:  EXTI2  (pins x2)
    EXTI3_IRQHandler,           // IRQ9:  EXTI3  (pins x3)
    EXTI4_IRQHandler,           // IRQ10: EXTI4  (pins x4)
    Default_Handler,            // IRQ11: DMA1_Stream0
    Default_Handler,            // IRQ12: DMA1_Stream1
    Default_Handler,            // IRQ13: DMA1_Stream2
    Default_Handler,            // IRQ14: DMA1_Stream3
    Default_Handler,            // IRQ15: DMA1_Stream4
    Default_Handler,            // IRQ16: DMA1_Stream5
    Default_Handler,            // IRQ17: DMA1_Stream6
    Default_Handler,            // IRQ18: ADC
    Default_Handler,            // IRQ19: CAN1_TX
    Default_Handler,            // IRQ20: CAN1_RX0
    Default_Handler,            // IRQ21: CAN1_RX1
    Default_Handler,            // IRQ22: CAN1_SCE
    EXTI9_5_IRQHandler,         // IRQ23: EXTI9_5 (pins x5-x9, shared)
    Default_Handler,            // IRQ24: TIM1_BRK / TIM9
    Default_Handler,            // IRQ25: TIM1_UP  / TIM10
    Default_Handler,            // IRQ26: TIM1_TRG / TIM11
    Default_Handler,            // IRQ27: TIM1_CC
    TIM2_IRQHandler,            // IRQ28: TIM2
    TIM3_IRQHandler,            // IRQ29: TIM3
    TIM4_IRQHandler,            // IRQ30: TIM4
    Default_Handler,            // IRQ31: I2C1_EV
    Default_Handler,            // IRQ32: I2C1_ER
    Default_Handler,            // IRQ33: I2C2_EV
    Default_Handler,            // IRQ34: I2C2_ER
    Default_Handler,            // IRQ35: SPI1
    Default_Handler,            // IRQ36: SPI2
    USART1_IRQHandler,          // IRQ37: USART1
    USART2_IRQHandler,          // IRQ38: USART2
    USART3_IRQHandler,          // IRQ39: USART3
    EXTI15_10_IRQHandler,       // IRQ40: EXTI15_10 (pins x10-x15, shared)
};

void Reset_Handler(void) {
    unsigned long *src, *dst;
    // Copy .data from flash to RAM
    src = &__etext;
    dst = &__data_start__;
    while (dst < &__data_end__) {
        *dst++ = *src++;
    }
    // Zero .bss
    dst = &__bss_start__;
    while (dst < &__bss_end__) {
        *dst++ = 0;
    }
    // System init
    SystemInit();
    // Jump to main
    main();
    while (1) {}
}

#endif /* __ICCARM__ */

void Default_Handler(void) {
    while (1) {}
}