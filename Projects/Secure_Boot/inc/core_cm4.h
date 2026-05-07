#ifndef CORE_CM4_H
#define CORE_CM4_H

#include <stdint.h>

/*
 * Minimal ARM Cortex-M NVIC driver — no CMSIS dependency.
 * NVIC register addresses are identical across Cortex-M0+/M3/M4/M33.
 * __NVIC_PRIO_BITS must be defined before including this header.
 */

#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS 4U
#endif

/* ARM core NVIC registers (constant across all Cortex-M variants) */
#define NVIC_ISER_BASE  ((volatile uint32_t *)0xE000E100UL)   /* Set Enable    */
#define NVIC_ICER_BASE  ((volatile uint32_t *)0xE000E180UL)   /* Clear Enable  */
#define NVIC_IPR_BASE   ((volatile uint8_t  *)0xE000E400UL)   /* Priority byte */

/* Enable a peripheral interrupt in the NVIC */
static inline void NVIC_EnableIRQ(int32_t IRQn)
{
    if (IRQn >= 0) {
        NVIC_ISER_BASE[(uint32_t)IRQn >> 5U] = (1UL << ((uint32_t)IRQn & 0x1FU));
    }
}

/* Disable a peripheral interrupt in the NVIC */
static inline void NVIC_DisableIRQ(int32_t IRQn)
{
    if (IRQn >= 0) {
        NVIC_ICER_BASE[(uint32_t)IRQn >> 5U] = (1UL << ((uint32_t)IRQn & 0x1FU));
    }
}

/* Set interrupt priority (lower value = higher priority) */
static inline void NVIC_SetPriority(int32_t IRQn, uint32_t priority)
{
    if (IRQn >= 0) {
        NVIC_IPR_BASE[(uint32_t)IRQn] =
            (uint8_t)((priority << (8U - __NVIC_PRIO_BITS)) & 0xFFU);
    }
}

#endif /* CORE_CM4_H */

