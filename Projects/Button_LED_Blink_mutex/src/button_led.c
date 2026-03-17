#include "../inc/button_led.h"

/* Simple busy-wait delay. */
static void delay(volatile uint32_t count) {
    while (count--) __asm__("nop");
}

/* -----------------------------------------------------------------------
 * Button_constructor
 *   Populates Button_TypeDef from the supplied parameters, configures the
 *   GPIO pin, then calls Button_Init to wire up the EXTI interrupt.
 *   Works for any port (A-I) and any pin (0-15).
 * ----------------------------------------------------------------------- */
void Button_constructor(Button_TypeDef *button,
                        GPIO_ManualTypeDef *port, uint32_t pin_mask,
                        uint32_t mode, uint32_t pull, uint32_t speed,
                        uint8_t nvic_priority) {
    /* Derive EXTI line (0-15) from the single-bit pin mask. */
    uint8_t line = 0;
    uint32_t tmp = pin_mask;
    while ((tmp >>= 1)) line++;

    /* Derive port code (A=0, B=1, C=2, D=3…) from the port base address. */
    uint8_t port_code = (uint8_t)(((uint32_t)port - GPIOA_BASE) / 0x400U);

    /* Populate the embedded init struct and point gpio.init at it. */
    button->gpio_init.Pin   = pin_mask;
    button->gpio_init.Mode  = mode;
    button->gpio_init.Pull  = pull;
    button->gpio_init.Speed = speed;
    button->gpio.regs       = port;
    button->gpio.init       = &button->gpio_init;

    button->exti_line     = line;
    button->port_code     = port_code;
    button->nvic_priority = nvic_priority;

    /* Configure the GPIO pin via the shared GPIO driver. */
    GPIO_constructor(&button->gpio, port, &button->gpio_init);

    /* Configure EXTI and NVIC. */
    Button_Init(button);
}

/* -----------------------------------------------------------------------
 * Button_Init
 *   Generic EXTI setup: works for any line (0-15) on any GPIO port.
 *   All required information is read from the Button_TypeDef fields
 *   set by Button_constructor, so nothing is hardcoded here.
 * ----------------------------------------------------------------------- */
void Button_Init(Button_TypeDef *button) {
    /* 1. Enable the GPIO port clock. */
    __RCC_GPIO_CLK_ENABLE(&button->gpio);

    /* 2. Enable SYSCFG clock (needed to write EXTICR). */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* 3. Route the chosen GPIO port to this EXTI line via SYSCFG EXTICR.
     *    EXTICR[line/4] holds 4 lines × 4 bits each.                    */
    uint8_t line  = button->exti_line;
    uint8_t shift = (uint8_t)((line % 4U) * 4U);
    SYSCFG->EXTICR[line / 4U] &= ~(0xFU << shift);
    SYSCFG->EXTICR[line / 4U] |=  ((uint32_t)button->port_code << shift);

    /* 4. Unmask the line and select rising-edge trigger. */
    EXTI->IMR  |=  (1U << line);
    EXTI->RTSR |=  (1U << line);
    EXTI->FTSR &= ~(1U << line);

    /* 5. Enable the correct NVIC IRQ.
     *    STM32F4 groups: lines 0-4 have dedicated IRQs;
     *    5-9 share EXTI9_5_IRQn; 10-15 share EXTI15_10_IRQn.           */
    IRQn_Type irqn;
    if      (line == 0) irqn = EXTI0_IRQn;
    else if (line == 1) irqn = EXTI1_IRQn;
    else if (line == 2) irqn = EXTI2_IRQn;
    else if (line == 3) irqn = EXTI3_IRQn;
    else if (line == 4) irqn = EXTI4_IRQn;
    else if (line <= 9) irqn = EXTI9_5_IRQn;
    else                irqn = EXTI15_10_IRQn;

    NVIC_SetPriority(irqn, button->nvic_priority);
    NVIC_EnableIRQ(irqn);
}

/* -----------------------------------------------------------------------
 * EXTI0_IRQHandler
 *   Fires on USER button rising edge (press). Sets g_async_requested so
 *   that the next call to blink_LEDS_async in main performs the fast blink.
 * ----------------------------------------------------------------------- */
void EXTI0_IRQHandler(void) {
    /* Clear the pending bit immediately to re-arm the interrupt. */
    EXTI->PR = (1U << 0);
    g_async_requested = 1;
}

/* -----------------------------------------------------------------------
 * blink_LEDS_sync
 *   Acquires the mutex (blocks until free), then toggles ALL LEDs at the
 *   same time every ~500 ms for 10 cycles, then releases the mutex.
 * ----------------------------------------------------------------------- */
void blink_LEDS_sync(USART_HandleType *usart, LED_Type * const leds[]) {
    mutex_acquire();
    USART_WriteString(usart, "-- Sync blink start --\r\n");

    /* 5 cycles (~2.5 s total) while the mutex is held continuously.
     * This gives the button time to be pressed mid-blink.            */
    for (int cycle = 0; cycle < 5; ++cycle) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "-- cycle no: %d --\r\n\r\n", cycle + 1);
            USART_WriteString(usart, buffer);
        /* Toggle every LED in one pass so they all switch simultaneously. */
        for (int i = 0; leds[i] != NULL; ++i)
            LED_setState(leds[i], LED_TOGGLE);
        USART_WriteString(usart, (LED_PORT->ODR & LED_PIN_GREEN) ? "All LEDs ON\r\n" : "All LEDs OFF\r\n");
        delay(2000000);

        /* Mid-cycle async probe: async finds the mutex busy here and prints
         * "deferring". After sync releases, the main-loop call succeeds.  */
        if (g_async_requested)
            blink_LEDS_async(usart, leds);
    }

    USART_WriteString(usart, "-- Sync blink end --\r\n\r\n");
    mutex_release();
}

/* -----------------------------------------------------------------------
 * blink_LEDS_async
 *   Runs ONLY when the button has been pressed (g_async_requested flag set
 *   by EXTI0_IRQHandler). Tries a non-blocking mutex acquire so it never
 *   clobbers an ongoing sync blink that still holds the lock. If the lock
 *   is busy the request is simply deferred to the next call.
 *   Pattern: 3 rapid ON/OFF flashes (~100 ms each) to distinguish it from
 *   the slow sync blink.
 * ----------------------------------------------------------------------- */
void blink_LEDS_async(USART_HandleType *usart, LED_Type * const leds[]) {
    if (!g_async_requested)
        return;  /* nothing to do -- skip silently */

    if (!mutex_try_acquire()) {
        USART_WriteString(usart, "Async: mutex busy, deferring.\r\n\r\n");
        return;
    }

    USART_WriteString(usart, "-- Async blink (button pressed) --\r\n");

    /* Blink LEDs one after another (sequential), printing state for each.
     * This is the distinguishing behaviour from sync (all-at-once). */
    for (int cycle = 0; cycle < 5; ++cycle) { /* 5 ON/OFF cycles */
        /* Print cycle number at the start of each cycle. */
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "-- cycle no: %d --\r\n\r\n", cycle + 1);
        USART_WriteString(usart, buffer);

        for (int i = 0; leds[i] != NULL; ++i) {
            LED_setState(leds[i], LED_ON);
            delay(200000);  /* ~100 ms ON */
            LED_setState(leds[i], LED_OFF);
            delay(100000);  /* ~50 ms gap before next LED */
        }
    }
    /* Clear request flag BEFORE releasing the mutex. */
    g_async_requested = 0;
    USART_WriteString(usart, "-- Async blink end --\r\n\r\n");
    mutex_release();
}