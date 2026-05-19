#include "../inc/systick.h"

/* Countdown used by the interrupt-based delay */
static volatile uint32_t s_tick_ms = 0;

void SysTick_Handler(void)
{
    s_tick_ms++;
}

void SysTick_constructor(SysTick_HandleTypeDef *handle, SysTick_ManualType *regs, SysTick_StatusTypeDef status){
    handle->regs = regs;
    handle->status = status;
    handle->SystemCoreClock = 48000000UL; // Assuming default HSI clock

    if(handle->status != SYSTICK_OK){
        // Handle error if needed
        return;
    }
}

void SysTick_delay(SysTick_HandleTypeDef *handle, volatile uint32_t sec){
    int i;

    handle->regs->LOAD = handle->SystemCoreClock * sec - 1; // Load the SysTick timer for 1ms
    handle->regs->VAL = 0; // Clear the current value
    handle->regs->CTRL = 5; // Enable SysTick with processor clock, no interrupt

    for(i=0; i<sec; i++){
        while (!(handle->regs->CTRL & SysTick_CTRL_COUNTFLAG_Msk)); // Wait for the COUNTFLAG
    }
    
    handle->regs->CTRL = 0; // Disable SysTick
}

void SysTick_delay_ms(SysTick_HandleTypeDef *handle, volatile uint32_t ms){
    int i;

    handle->regs->LOAD = handle->SystemCoreClock / 1000 - 1; // Load the SysTick timer for 1ms
    handle->regs->VAL = 0; // Clear the current value
    handle->regs->CTRL = 5; // Enable SysTick with processor clock, no interrupt

    for(i=0; i<ms; i++){
        while (!(handle->regs->CTRL & SysTick_CTRL_COUNTFLAG_Msk)); // Wait for the COUNTFLAG
    }
    
    handle->regs->CTRL = 0; // Disable SysTick
}

void SysTick_delay_ms_irq(SysTick_HandleTypeDef *handle, volatile uint32_t ms)
{
    uint32_t start = s_tick_ms;

    handle->regs->LOAD = handle->SystemCoreClock / 1000 - 1; // 1 ms per tick
    handle->regs->VAL  = 0;
    handle->regs->CTRL = 7; // CLKSOURCE=1, TICKINT=1, ENABLE=1

    while ((s_tick_ms - start) < ms); // wait for IRQ-driven delta

    handle->regs->CTRL = 0; // Disable SysTick
}

void SysTick_StartTimer(SysTick_HandleTypeDef *handle)
{
    handle->regs->LOAD = handle->SystemCoreClock / 1000 - 1; // 1 ms per tick
    handle->regs->VAL  = 0;
    handle->regs->CTRL = 7; // CLKSOURCE=1, TICKINT=1, ENABLE=1
    handle->start_tick = s_tick_ms; // snapshot after timer is running
}

float SysTick_GetElapsedTime_ms(SysTick_HandleTypeDef *handle)
{
    uint32_t val      = handle->regs->VAL;            // read VAL first to minimise race
    uint32_t full_ms  = s_tick_ms - handle->start_tick;
    uint32_t load     = handle->regs->LOAD;
    // Sub-ms: VAL counts down from LOAD. Ticks elapsed in current period = LOAD - VAL.
    float sub_ms = (float)(load - val) / (float)(load + 1U);
    return (float)full_ms + sub_ms;                   // total elapsed time in ms
}

uint32_t SysTick_GetElapsedTime_us(SysTick_HandleTypeDef *handle)
{
    uint32_t val     = handle->regs->VAL;            // read downcounter first
    uint32_t full_ms = s_tick_ms - handle->start_tick;
    uint32_t load    = handle->regs->LOAD;

    /* Ticks elapsed in the current 1 ms window, converted to µs */
    uint32_t sub_us  = (load - val) * 1000U / (load + 1U);

    return full_ms * 1000U + sub_us;
}