#ifndef __RNG_H
#define __RNG_H

#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "../../GPIO/inc/gpio.h"   /* RCC_TypeDef / RCC */
#include "../../NVIC/inc/nvic.h"
#include "../../UART/inc/uart.h"

#define __IO volatile

/* RNG peripheral base address */
#define RNG_BASE  0x420C0800UL

/* RCC clock enable for RNG (AHB2ENR bit 18) */
#define RCC_AHB2ENR_RNGEN   (1U << 18)
/* RCC peripheral reset for RNG (AHB2RSTR bit 18) */
#define RCC_AHB2RSTR_RNGEN  (1U << 18)

/* RCC_CCIPR2: CK48 clock source selection (bits 25:24)
 * CK48 feeds the RNG entropy clock (must be > fHCLK/32 to avoid CECS).
 * Select hsi_div_3_ck = HSIDIV3 = HSIS/3 = 16 MHz — on by default (HSIDIV3ON=1 at reset). */
#define RCC_CCIPR2_CK48SEL_MASK      (3U << 24)
#define RCC_CCIPR2_CK48SEL_HSI_DIV3  (2U << 24)

/* RNG CR register bits */
#define RNG_CR_RNGEN    (1U << 2)   /* Enable RNG */
#define RNG_CR_IE       (1U << 3)   /* Interrupt enable */
#define RNG_CR_CED      (1U << 5)   /* Clock Error Detection: 0=enabled, 1=DISABLED */
#define RNG_CR_CONDRST  (1U << 30)  /* Conditioning soft reset (HW-cleared when done) */

/* HTCR0 reset value — must be re-written during CONDRST to re-arm health tests */
#define RNG_HTCR0_MAGIC  0x000072ACU

/* RNG SR status bits */
#define RNG_SR_DRDY  (1U << 0)  /* Data Ready */
#define RNG_SR_CEIS  (1U << 5)  /* Clock Error Interrupt Status */
#define RNG_SR_SEIS  (1U << 6)  /* Seed Error Interrupt Status */

/* RNG register map */
typedef struct {
    __IO uint32_t CR;     /* 0x000: Control register */
    __IO uint32_t SR;     /* 0x004: Status register */
    __IO uint32_t DR;     /* 0x008: Data register (read random number) */
    __IO uint32_t NSCR;    /* 0x00C: Non-secure configuration register */
    __IO uint32_t HTCR0;  /* 0x010: Health test config register */
} RNG_ManualType;


#define TRNG ((RNG_ManualType *)RNG_BASE)

typedef enum {
    RNG_OK = 0,
    RNG_ERROR,
    RNG_BUSY
} RNG_StatusTypeDef;

typedef struct {
    RNG_ManualType            *Instance;
    volatile RNG_StatusTypeDef status;  /* volatile: written by ISR, polled by Generate_IT */
    /* State used by RNG_Generate_IT */
    uint8_t                   *it_buf;
    size_t                     it_len;
    size_t                     it_idx;
} RNG_HandleTypeDef;

/***************** Public API *****************/
void RNG_Constructor(RNG_HandleTypeDef *hrng);
void RNG_Enable(RNG_HandleTypeDef *hrng);
void RNG_Disable(RNG_HandleTypeDef *hrng);
RNG_StatusTypeDef RNG_Generate(RNG_HandleTypeDef *hrng, uint8_t *buffer, size_t length);
RNG_StatusTypeDef RNG_Generate_IT(RNG_HandleTypeDef *hrng, uint8_t *buffer, size_t length);

#endif /* __RNG_H */