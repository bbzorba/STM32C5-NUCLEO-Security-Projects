#ifndef __HASH_H
#define __HASH_H

#include <stdint.h>
#include <stddef.h>
#include "../../GPIO/inc/gpio.h"   /* RCC_TypeDef / RCC */

#define __IO volatile

/* HASH peripheral base address (STM32C562RE, RM Table 2) */
#define HASH_BASE  0x420C0400U

/* HASH register map (offsets 0x000 - 0x024) */
typedef struct {
    __IO uint32_t CR;     /* 0x000  Control                */
    __IO uint32_t DIN;    /* 0x004  Data Input             */
    __IO uint32_t STR;    /* 0x008  Start (NBLW / DCAL)    */
    __IO uint32_t HRA[5]; /* 0x00C  Intermediate digest    */
    __IO uint32_t IMR;    /* 0x020  Interrupt Mask         */
    __IO uint32_t SR;     /* 0x024  Status                 */
} HASH_TypeDef;

#define HASH_PERIPH  ((HASH_TypeDef *)HASH_BASE)

/* Final SHA-256 digest output: 8 × 32-bit words at offset 0x310 */
#define HASH_HR_OUT  ((volatile uint32_t *)(HASH_BASE + 0x310U))

/* RCC clock enable for HASH */
#define RCC_AHB2ENR_HASHEN  (1U << 17)

/* CR bits */
#define HASH_CR_INIT        (1U << 2)   /* Reset engine (self-clearing)    */
#define HASH_CR_ALGO_SHA256 (3U << 17)  /* bits[18:17] = 11 → SHA-256      */

/* STR bits */
#define HASH_STR_NBLW_MASK  0x1FU
#define HASH_STR_DCAL       (1U << 8)   /* Fire padding + digest calc      */

/* SR bits */
#define HASH_SR_DINIS  (1U << 0)  /* Engine ready for input  */
#define HASH_SR_DCIS   (1U << 1)  /* Digest complete         */
#define HASH_SR_BUSY   (1U << 3)  /* Engine busy             */

typedef enum { HASH_OK = 0, HASH_ERROR } HASH_StatusTypeDef;

typedef struct {
    HASH_TypeDef       *Instance;
    HASH_StatusTypeDef  state;
    size_t              msg_len;  /* total bytes fed (used for NBLW) */
} HASH_HandleTypeDef;

void               HASH_Init        (HASH_HandleTypeDef *hhash);
HASH_StatusTypeDef HASH_SHA256_Start(HASH_HandleTypeDef *hhash);
HASH_StatusTypeDef HASH_SHA256_Update(HASH_HandleTypeDef *hhash, const uint8_t *data, size_t len);
HASH_StatusTypeDef HASH_SHA256_Final (HASH_HandleTypeDef *hhash, uint8_t *digest);

#endif /* __HASH_H */