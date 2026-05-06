#ifndef __NVIC_H
#define __NVIC_H

#include <stdint.h>

#ifndef __IO
#define __IO volatile
#endif

#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS 4
#endif

/*
 * IRQn_Type for STM32C562RE (Cortex-M33).
 * IRQ numbers verified against STM32C562.svd.
 */
#ifndef __STM32C5xx_H
typedef enum IRQn
{
    NonMaskableInt_IRQn    = -14,
    MemoryManagement_IRQn  = -12,
    BusFault_IRQn          = -11,
    UsageFault_IRQn        = -10,
    SVCall_IRQn            = -5,
    DebugMonitor_IRQn      = -4,
    PendSV_IRQn            = -2,
    SysTick_IRQn           = -1,
    /* EXTI line IRQs (GPIO external interrupt lines 0-15) */
    EXTI0_IRQn  = 7,
    EXTI1_IRQn  = 8,
    EXTI2_IRQn  = 9,
    EXTI3_IRQn  = 10,
    EXTI4_IRQn  = 11,
    EXTI5_IRQn  = 12,
    EXTI6_IRQn  = 13,
    EXTI7_IRQn  = 14,
    EXTI8_IRQn  = 15,
    EXTI9_IRQn  = 16,
    EXTI10_IRQn = 17,
    EXTI11_IRQn = 18,
    EXTI12_IRQn = 19,
    EXTI13_IRQn = 20,
    EXTI14_IRQn = 21,
    EXTI15_IRQn = 22,
    /* LPDMA1 channel IRQs */
    LPDMA1_Channel0_IRQn = 23,
    LPDMA1_Channel1_IRQn = 24,
    LPDMA1_Channel2_IRQn = 25,
    LPDMA1_Channel3_IRQn = 26,
    LPDMA1_Channel4_IRQn = 27,
    LPDMA1_Channel5_IRQn = 28,
    LPDMA1_Channel6_IRQn = 29,
    LPDMA1_Channel7_IRQn = 30,
    /* Watchdog */
    IWDG1_IRQn           = 31,
    /* ADC */
    ADC1_IRQn            = 32,
    ADC2_IRQn            = 33,
    /* FDCAN1 */
    FDCAN1_IT0_IRQn      = 34,
    FDCAN1_IT1_IRQn      = 35,
    /* SPI */
    SPI1_IRQn            = 48,
    SPI2_IRQn            = 49,
    SPI3_IRQn            = 50,
    /* USART/UART peripheral IRQs */
    USART1_IRQn = 51,
    USART2_IRQn = 52,
    USART3_IRQn = 53,
    UART4_IRQn  = 54,
    UART5_IRQn  = 55,
    /* RNG */
    RNG_IRQn            = 64,
    /* HASH */
    HASH_IRQn            = 69,
    /* Extended USART/UART */
    USART6_IRQn          = 96,
    UART7_IRQn           = 97,
    /* ADC3 */
    ADC3_IRQn            = 98,
} IRQn_Type;
#endif /* __STM32C5xx_H */

#include "core_cm4.h"  /* NVIC_SetPriority, NVIC_EnableIRQ, NVIC_DisableIRQ */

/* Callback invoked from the hardware IRQ handler */
typedef void (*NVIC_HandlerFunc_t)(void *arg);

typedef enum {
    NVIC_OK    = 0,
    NVIC_ERROR = 1,
} NVIC_StatusType;

/* Must exceed the largest IRQn value in use (ADC3 = 98). */
#define NVIC_IRQ_COUNT 128U

/*
 * Register a handler for a peripheral IRQ and enable it in the NVIC.
 *   irq      - peripheral IRQ number (must be >= 0)
 *   handler  - callback invoked when the IRQ fires
 *   arg      - opaque pointer passed to handler (e.g. a handle struct)
 *   priority - NVIC priority (0 = highest)
 */
NVIC_StatusType NVIC_RegisterHandler(IRQn_Type irq, NVIC_HandlerFunc_t handler,
                                     void *arg, uint8_t priority);

/* Disable an IRQ and remove its handler from the table. */
NVIC_StatusType NVIC_UnregisterHandler(IRQn_Type irq);

/* ── EXTI (GPIO external interrupt) helper API ───────────────────────────
 *
 * These three functions wrap all the low-level EXTI register writes so
 * application code only needs one call to enable a GPIO interrupt.
 *
 * NVIC_EXTI_Enable()      - configure edge, route pin→port, unmask, register
 * NVIC_EXTI_Disable()     - mask the line and remove the handler
 * NVIC_EXTI_ClearPending()- clear the hardware pending flag (call at the top
 *                           of your callback to prevent re-triggering)
 *
 * Example:
 *   static void btn_cb(void *arg) {
 *       NVIC_EXTI_ClearPending(13);
 *       GPIO_TogglePin((GPIO_HandleTypeDef *)arg, GPIO_PIN_5);
 *   }
 *   NVIC_EXTI_Enable(13, NVIC_PORT_C, NVIC_EDGE_FALLING, btn_cb, &led, 0);
 * ─────────────────────────────────────────────────────────────────────── */

/* Edge selection for NVIC_EXTI_Enable() */
typedef enum {
    NVIC_EDGE_FALLING = 0,   /* trigger on falling edge only  */
    NVIC_EDGE_RISING  = 1,   /* trigger on rising  edge only  */
    NVIC_EDGE_BOTH    = 2,   /* trigger on both edges         */
} NVIC_EdgeType;

/* Port constants for the EXTICR source-selection field (A=0, B=1, C=2 …) */
#define NVIC_PORT_A  0U
#define NVIC_PORT_B  1U
#define NVIC_PORT_C  2U
#define NVIC_PORT_D  3U
#define NVIC_PORT_E  4U
#define NVIC_PORT_H  7U

/*
 * NVIC_EXTI_Enable - configure one GPIO pin as an external interrupt source.
 *   pin      - GPIO pin number 0-15
 *   port     - NVIC_PORT_A … NVIC_PORT_H
 *   edge     - NVIC_EDGE_FALLING, NVIC_EDGE_RISING, or NVIC_EDGE_BOTH
 *   handler  - callback invoked when the interrupt fires
 *   arg      - opaque pointer passed to handler (pass NULL if unused)
 *   priority - NVIC priority level (0 = highest)
 */
NVIC_StatusType NVIC_EXTI_Enable(uint8_t pin, uint8_t port, NVIC_EdgeType edge,
                                 NVIC_HandlerFunc_t handler, void *arg,
                                 uint8_t priority);

/* Disable the EXTI line for the given pin and remove its handler. */
void NVIC_EXTI_Disable(uint8_t pin);

/* Clear the EXTI pending flag for 'pin'. Call at the start of your callback. */
void NVIC_EXTI_ClearPending(uint8_t pin);

#endif /* __NVIC_H */
