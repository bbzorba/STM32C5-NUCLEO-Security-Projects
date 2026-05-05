#include <stddef.h>
#include "../inc/gpio.h"
#include "../../NVIC/inc/nvic.h"

/*
 * NUCLEO-C562RE GPIO interrupt demo.
 *   LD1 Green = PA5 (active high)
 *   B1 Button = PC13 (active low, internal pull-up)
 *
 * Two test phases — select by defining TEST_PHASE before building:
 *
 *   Phase 1 — Polling  : main loop reads PC13; debounces; toggles LED.
 *   Phase 2 — Interrupt: NVIC_EXTI_Enable() does all EXTI setup in one call.
 *                        The callback toggles the LED on each button press.
 *                        The main loop sleeps in WFI between presses.
 */
#ifndef TEST_PHASE
#define TEST_PHASE 2          /* 1 = polling, 2 = interrupt */
#endif

void delay_fn(volatile int count) {
    for (volatile int i = 0; i < count; i++);
}

/* ── Phase 2 interrupt callback ────────────────────────────────────────────
 *
 * HOW TO USE NVIC_EXTI_Enable IN YOUR OWN CODE:
 *
 *   1. Write a callback: void my_cb(void *arg) { ... }
 *      - arg is whatever pointer you passed to NVIC_EXTI_Enable.
 *      - Always call NVIC_EXTI_ClearPending(pin) first to prevent re-fire.
 *
 *   2. Call NVIC_EXTI_Enable(pin, port, edge, my_cb, arg, priority).
 *      - pin    : GPIO pin number (0-15)
 *      - port   : NVIC_PORT_A / NVIC_PORT_B / NVIC_PORT_C …
 *      - edge   : NVIC_EDGE_FALLING / NVIC_EDGE_RISING / NVIC_EDGE_BOTH
 *      - my_cb  : your callback
 *      - arg    : pointer passed to callback (e.g. a handle, or NULL)
 *      - priority: NVIC priority (0 = highest)
 *
 *   3. To stop: NVIC_EXTI_Disable(pin).
 */
static GPIO_HandleTypeDef *s_led;   /* shared between main and callback */

static void btn_callback(void *arg) {
    (void)arg;
    NVIC_EXTI_ClearPending(13);        /* always clear pending flag first */
    GPIO_TogglePin(s_led, GPIO_PIN_5);
}

int main(void) {
    /* ── PA5: output push-pull (LED) ────────────────────────────────────── */
    GPIO_InitTypeDef led_cfg;
    led_cfg.Pin       = GPIO_PIN_5;
    led_cfg.Mode      = GPIO_MODE_OUTPUT_PP;
    led_cfg.Pull      = GPIO_NOPULL;
    led_cfg.Speed     = GPIO_SPEED_MEDIUM;
    led_cfg.Alternate = 0;

    GPIO_HandleTypeDef ld1;
    GPIO_constructor(&ld1, GPIO_A, &led_cfg);

    /* ── PC13: input with pull-up (button, active-low) ──────────────────── */
    GPIO_InitTypeDef btn_cfg;
    btn_cfg.Pin       = GPIO_PIN_13;
    btn_cfg.Mode      = GPIO_MODE_INPUT;
    btn_cfg.Pull      = GPIO_PULLUP;
    btn_cfg.Speed     = GPIO_SPEED_LOW;
    btn_cfg.Alternate = 0;

    GPIO_HandleTypeDef btn;
    GPIO_constructor(&btn, GPIO_C, &btn_cfg);

/* ════════════════════════════════════════════════════════════════════════
 * PHASE 1 — POLLING
 * ════════════════════════════════════════════════════════════════════════ */
#if TEST_PHASE == 1

    while (1) {
        /* PC13 is active-low: pressed → bit is 0 */
        if (!(GPIO_C->IDR & GPIO_PIN_13)) {
            GPIO_TogglePin(&ld1, GPIO_PIN_5);
            while (!(GPIO_C->IDR & GPIO_PIN_13));   /* wait for release */
            delay_fn(20000);                         /* debounce */
        }
    }

/* ════════════════════════════════════════════════════════════════════════
 * PHASE 2 — INTERRUPT via NVIC_EXTI_Enable
 * ════════════════════════════════════════════════════════════════════════ */
#else

    s_led = &ld1;

    /* One call sets up EXTICR, FTSR1, IMR1 and registers the callback. */
    NVIC_EXTI_Enable(13, NVIC_PORT_C, NVIC_EDGE_FALLING, btn_callback, NULL, 0);

    /* Blink once to signal "interrupt mode active" */
    GPIO_TogglePin(&ld1, GPIO_PIN_5);
    delay_fn(400000);
    GPIO_TogglePin(&ld1, GPIO_PIN_5);

    while (1) {
        __asm volatile ("wfi");   /* sleep until interrupt */
    }

#endif
}