#include "stm32f4xx.h"

extern int main(void);
void Reset_Handler(void);
void Default_Handler(void);

extern unsigned long __etext;       // end of code (flash)
extern unsigned long __data_start__;
extern unsigned long __data_end__;
extern unsigned long __bss_start__;
extern unsigned long __bss_end__;

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
    Default_Handler             // SysTick
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

void Default_Handler(void) {
    while (1) {}
}