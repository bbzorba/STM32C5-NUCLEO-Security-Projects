#ifndef BUTTON_LED_H
#define BUTTON_LED_H

/* All base types (LED_Type, LEDColor_Type, LEDState_Type, pin macros, etc.)
 * come from the LED_Blink library header -- no redefinitions needed here.  */
#include "../../Projects/LED_Blink/inc/led.h"
#include "../inc/mutex.h"
#include <stdio.h>

/* -----------------------------------------------------------------------
 * Minimal SYSCFG / EXTI register definitions (not in gpio.h / uart.h).
 * Base addresses from STM32F4 reference manual.
 * ----------------------------------------------------------------------- */
#define SYSCFG_BASE             (APB2PERIPH_BASE + 0x3800U)
#define EXTI_BASE               (APB2PERIPH_BASE + 0x3C00U)
#define RCC_APB2ENR_SYSCFGEN    (1U << 14)

typedef struct {
    volatile uint32_t MEMRMP;
    volatile uint32_t PMC;
    volatile uint32_t EXTICR[4];
    uint32_t          RESERVED[2];
    volatile uint32_t CMPCR;
} SYSCFG_TypeDef;

typedef struct {
    volatile uint32_t IMR;
    volatile uint32_t EMR;
    volatile uint32_t RTSR;
    volatile uint32_t FTSR;
    volatile uint32_t SWIER;
    volatile uint32_t PR;
} EXTI_TypeDef;

/* Button object: wraps a GPIO handle plus the EXTI configuration
 * computed at construction time so Button_Init is fully generic.   */
typedef struct {
    GPIO_HandleTypeDef gpio;       /* GPIO pin handle (regs + init ptr)  */
    GPIO_InitTypeDef   gpio_init;  /* Init struct storage; gpio.init → &this */
    uint8_t            exti_line;  /* 0-15, derived from pin mask          */
    uint8_t            port_code;  /* 0=A, 1=B, 2=C, 3=D …, from port addr */
    uint8_t            nvic_priority;
} Button_TypeDef;

#define SYSCFG  ((SYSCFG_TypeDef *)SYSCFG_BASE)
#define EXTI    ((EXTI_TypeDef  *)EXTI_BASE)

/* Initialises the GPIO pin and EXTI interrupt for the given port/pin.
 * pin_mask: one of GPIO_PIN_0 … GPIO_PIN_15
 * port    : GPIO_A … GPIO_I
 * nvic_priority: 0 (highest) … 15 (lowest) */
void Button_constructor(Button_TypeDef *button,
                        GPIO_ManualTypeDef *port, uint32_t pin_mask,
                        uint32_t mode, uint32_t pull, uint32_t speed,
                        uint8_t nvic_priority);

/* Configures EXTI and NVIC from the fields already set by Button_constructor.
 * Generic: works for any line 0-15 on any GPIO port.               */
void Button_Init(Button_TypeDef *button);

/* Public blink API -- pass a NULL-terminated array of LED pointers. */
void blink_LEDS_sync(USART_HandleType *usart, LED_Type * const leds[]);
void blink_LEDS_async(USART_HandleType *usart, LED_Type * const leds[]);

#endif // BUTTON_LED_H