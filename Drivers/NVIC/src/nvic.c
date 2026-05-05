/*
    Author: Baris Berk Zorba
    NVIC dispatch driver for STM32C562RE.

    Provides a centralized interrupt dispatch table. Peripheral drivers
    register a callback via NVIC_RegisterHandler() (or the higher-level
    NVIC_EXTI_Enable() for GPIO pins) instead of defining their own IRQ
    handler functions.

    HOW IT WORKS (dispatch table):
      Each peripheral IRQ handler in this file (e.g. EXTI13_IRQHandler)
      simply looks up the registered callback in s_table[] and calls it.
      Application code registers a callback via NVIC_RegisterHandler() —
      the hardware fires the IRQ, this file dispatches to the callback.
      Application code never needs to define its own IRQHandler functions.
*/

#include "../inc/nvic.h"
#include <stddef.h>

/* ── EXTI register addresses (STM32C562RE, new-style EXTI) ──────────── */
#define EXTI_BASE_ADDR  0x44022000UL
#define EXTI_RTSR1   (*(volatile uint32_t *)(EXTI_BASE_ADDR + 0x000U))
#define EXTI_FTSR1   (*(volatile uint32_t *)(EXTI_BASE_ADDR + 0x004U))
#define EXTI_RPR1    (*(volatile uint32_t *)(EXTI_BASE_ADDR + 0x00CU))
#define EXTI_FPR1    (*(volatile uint32_t *)(EXTI_BASE_ADDR + 0x010U))
#define EXTI_IMR1    (*(volatile uint32_t *)(EXTI_BASE_ADDR + 0x080U))
/* EXTICR[0-3] are at +0x060, +0x064, +0x068, +0x06C (8 bits per line) */
static volatile uint32_t * const EXTICR =
    (volatile uint32_t *)(EXTI_BASE_ADDR + 0x060U);

typedef struct {
    NVIC_HandlerFunc_t fn;
    void              *arg;
} NVIC_Entry_t;

static NVIC_Entry_t s_table[NVIC_IRQ_COUNT];

NVIC_StatusType NVIC_RegisterHandler(IRQn_Type irq, NVIC_HandlerFunc_t handler,
                                     void *arg, uint8_t priority)
{
    uint32_t idx = (uint32_t)(int32_t)irq;
    if (idx >= NVIC_IRQ_COUNT || handler == NULL) {
        return NVIC_ERROR;
    }
    s_table[idx].fn  = handler;
    s_table[idx].arg = arg;
    NVIC_SetPriority(irq, priority);
    NVIC_EnableIRQ(irq);
    return NVIC_OK;
}

NVIC_StatusType NVIC_UnregisterHandler(IRQn_Type irq)
{
    uint32_t idx = (uint32_t)(int32_t)irq;
    if (idx >= NVIC_IRQ_COUNT) {
        return NVIC_ERROR;
    }
    NVIC_DisableIRQ(irq);
    s_table[idx].fn  = NULL;
    s_table[idx].arg = NULL;
    return NVIC_OK;
}

/* ── EXTI helper API ────────────────────────────────────────────────────── */

NVIC_StatusType NVIC_EXTI_Enable(uint8_t pin, uint8_t port, NVIC_EdgeType edge,
                                 NVIC_HandlerFunc_t handler, void *arg,
                                 uint8_t priority)
{
    if (pin > 15U) return NVIC_ERROR;

    uint32_t mask = 1UL << pin;

    /* Route EXTI line to the selected port (8-bit field per pin in EXTICR[0-3]) */
    uint32_t cr_idx  = pin / 4U;
    uint32_t bit_pos = (pin % 4U) * 8U;
    EXTICR[cr_idx] = (EXTICR[cr_idx] & ~(0xFFUL << bit_pos))
                   | ((uint32_t)port   <<  bit_pos);

    /* Configure trigger edge */
    if (edge == NVIC_EDGE_RISING  || edge == NVIC_EDGE_BOTH) EXTI_RTSR1 |=  mask;
    else                                                      EXTI_RTSR1 &= ~mask;
    if (edge == NVIC_EDGE_FALLING || edge == NVIC_EDGE_BOTH) EXTI_FTSR1 |=  mask;
    else                                                      EXTI_FTSR1 &= ~mask;

    /* Clear any stale pending flag before unmasking */
    EXTI_RPR1 = mask;
    EXTI_FPR1 = mask;

    /* Unmask the line in the CPU interrupt mask register */
    EXTI_IMR1 |= mask;

    /* Register callback and enable the IRQ in the NVIC */
    return NVIC_RegisterHandler((IRQn_Type)(EXTI0_IRQn + (int)pin),
                                handler, arg, priority);
}

void NVIC_EXTI_Disable(uint8_t pin)
{
    if (pin > 15U) return;
    EXTI_IMR1 &= ~(1UL << pin);
    NVIC_UnregisterHandler((IRQn_Type)(EXTI0_IRQn + (int)pin));
}

void NVIC_EXTI_ClearPending(uint8_t pin)
{
    uint32_t mask = 1UL << pin;
    EXTI_RPR1 = mask;   /* write-1-to-clear rising  pending */
    EXTI_FPR1 = mask;   /* write-1-to-clear falling pending */
}

/* ── Internal dispatch ──────────────────────────────────────────────────── */
static void dispatch(IRQn_Type irq)
{
    uint32_t idx = (uint32_t)(int32_t)irq;
    if (idx < NVIC_IRQ_COUNT && s_table[idx].fn != NULL) {
        s_table[idx].fn(s_table[idx].arg);
    }
}

/* ── EXTI IRQ handlers ────────────────────────────────────────────────── */
void EXTI0_IRQHandler(void)  { dispatch(EXTI0_IRQn);  }
void EXTI1_IRQHandler(void)  { dispatch(EXTI1_IRQn);  }
void EXTI2_IRQHandler(void)  { dispatch(EXTI2_IRQn);  }
void EXTI3_IRQHandler(void)  { dispatch(EXTI3_IRQn);  }
void EXTI4_IRQHandler(void)  { dispatch(EXTI4_IRQn);  }
void EXTI5_IRQHandler(void)  { dispatch(EXTI5_IRQn);  }
void EXTI6_IRQHandler(void)  { dispatch(EXTI6_IRQn);  }
void EXTI7_IRQHandler(void)  { dispatch(EXTI7_IRQn);  }
void EXTI8_IRQHandler(void)  { dispatch(EXTI8_IRQn);  }
void EXTI9_IRQHandler(void)  { dispatch(EXTI9_IRQn);  }
void EXTI10_IRQHandler(void) { dispatch(EXTI10_IRQn); }
void EXTI11_IRQHandler(void) { dispatch(EXTI11_IRQn); }
void EXTI12_IRQHandler(void) { dispatch(EXTI12_IRQn); }
void EXTI13_IRQHandler(void) { dispatch(EXTI13_IRQn); }
void EXTI14_IRQHandler(void) { dispatch(EXTI14_IRQn); }
void EXTI15_IRQHandler(void) { dispatch(EXTI15_IRQn); }

/* ── LPDMA1 channel IRQ handlers ─────────────────────────────────────── */
void LPDMA1_Channel0_IRQHandler(void) { dispatch(LPDMA1_Channel0_IRQn); }
void LPDMA1_Channel1_IRQHandler(void) { dispatch(LPDMA1_Channel1_IRQn); }
void LPDMA1_Channel2_IRQHandler(void) { dispatch(LPDMA1_Channel2_IRQn); }
void LPDMA1_Channel3_IRQHandler(void) { dispatch(LPDMA1_Channel3_IRQn); }
void LPDMA1_Channel4_IRQHandler(void) { dispatch(LPDMA1_Channel4_IRQn); }
void LPDMA1_Channel5_IRQHandler(void) { dispatch(LPDMA1_Channel5_IRQn); }
void LPDMA1_Channel6_IRQHandler(void) { dispatch(LPDMA1_Channel6_IRQn); }
void LPDMA1_Channel7_IRQHandler(void) { dispatch(LPDMA1_Channel7_IRQn); }

/* ── Watchdog IRQ handler ─────────────────────────────────────────────── */
void IWDG1_IRQHandler(void) { dispatch(IWDG1_IRQn); }

/* ── ADC IRQ handlers ─────────────────────────────────────────────────── */
void ADC1_IRQHandler(void) { dispatch(ADC1_IRQn); }
void ADC2_IRQHandler(void) { dispatch(ADC2_IRQn); }
void ADC3_IRQHandler(void) { dispatch(ADC3_IRQn); }

/* ── FDCAN1 IRQ handlers ──────────────────────────────────────────────── */
void FDCAN1_IT0_IRQHandler(void) { dispatch(FDCAN1_IT0_IRQn); }
void FDCAN1_IT1_IRQHandler(void) { dispatch(FDCAN1_IT1_IRQn); }

/* ── SPI IRQ handlers ────────────────────────────────────────────────── */
void SPI1_IRQHandler(void) { dispatch(SPI1_IRQn); }
void SPI2_IRQHandler(void) { dispatch(SPI2_IRQn); }
void SPI3_IRQHandler(void) { dispatch(SPI3_IRQn); }

/* ── USART/UART IRQ handlers ─────────────────────────────────────────── */
void USART1_IRQHandler(void) { dispatch(USART1_IRQn); }
void USART2_IRQHandler(void) { dispatch(USART2_IRQn); }
void USART3_IRQHandler(void) { dispatch(USART3_IRQn); }
void UART4_IRQHandler(void)  { dispatch(UART4_IRQn);  }
void UART5_IRQHandler(void)  { dispatch(UART5_IRQn);  }
void USART6_IRQHandler(void) { dispatch(USART6_IRQn); }
void UART7_IRQHandler(void)  { dispatch(UART7_IRQn);  }

/* ── HASH IRQ handler ────────────────────────────────────────────────── */
void HASH_IRQHandler(void) { dispatch(HASH_IRQn); }
