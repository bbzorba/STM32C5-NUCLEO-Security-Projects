// AES driver for STM32C562RE
// NOTE: STM32C562RE has the AES peripheral (0x420C0000, AHB2).
#ifndef __AES_H
#define __AES_H

#include <stddef.h>
#include <stdint.h>
#include "../../GPIO/inc/gpio.h"  /* RCC_TypeDef, RCC */

#define __IO volatile

/* AES base address and RCC clock enable (verified: STM32C562.svd) */
#define AES_BASE          0x420C0000U
#define RCC_AHB2ENR_AESEN (1U << 16)

/* AES_CR bits */
#define AES_CR_EN           (1U <<  0)
#define AES_CR_MODE_ENCRYPT (0U <<  3)   /* MODE = 00 */
#define AES_CR_MODE_KEYDER  (1U <<  3)   /* MODE = 01, key derivation */
#define AES_CR_MODE_DECRYPT (2U <<  3)   /* MODE = 10 */
/* CHMOD[2:0] at bits 6:5; use (AES_ModeTypeDef << 5): ECB=0, CBC=1, CTR=2 */
#define AES_CR_KEYSIZE_256  (1U << 18)   /* 0 = 128-bit, 1 = 256-bit */
#define AES_CR_IPRST        (1U << 31)   /* software reset */
#define AES_ISR_CCF   (1U << 0)
#define AES_ISR_RWEIF (1U << 1)
#define AES_ISR_KEIF  (1U << 2)

/* AES_ICR bits (offset 0x308, write-1-to-clear) */
#define AES_ICR_CCF   (1U << 0)
#define AES_ICR_RWEIF (1U << 1)
#define AES_ICR_KEIF  (1U << 2)

/* Sizes and return codes */
#define AES_BLOCK_SIZE        16
#define AES_SUCCESS            0
#define AES_ERROR             -1
#define AES_INVALID_DATA_SIZE -4

/* AES register map (verified offsets: STM32C562.svd)
 * KEYR[0..3] = 128-bit key; KEYRH[0..3] = upper 128 bits for 256-bit key
 * CCF is in ISR (offset 0x304, bit 0) — cleared via ICR */
typedef struct {
    __IO uint32_t CR;        /* 0x000 */
    __IO uint32_t SR;        /* 0x004 â€” BUSY/KEYVALID, no CCF */
    __IO uint32_t DINR;      /* 0x008 */
    __IO uint32_t DOUTR;     /* 0x00C */
    __IO uint32_t KEYR[4];   /* 0x010-0x01C: 128-bit key (lower half / only half) */
    __IO uint32_t IVR[4];    /* 0x020-0x02C */
    __IO uint32_t KEYRH[4];  /* 0x030-0x03C: 256-bit key upper half */
    __IO uint32_t SUSP[8];   /* 0x040-0x05C */
    uint32_t      _pad[168]; /* 0x060-0x2FF */
    __IO uint32_t IER;       /* 0x300 */
    __IO uint32_t ISR;       /* 0x304 */
    __IO uint32_t ICR;       /* 0x308 */
} AES_ManualTypeDef;

#define AES ((AES_ManualTypeDef *)AES_BASE)

typedef enum { 
    AES_MODE_ECB = 0, 
    AES_MODE_CBC, 
    AES_MODE_CTR 
} AES_ModeTypeDef;

typedef enum { 
    AES_KEYSIZE_128 = 0, 
    AES_KEYSIZE_256 
} AES_KeySizeTypeDef;

typedef struct {
    AES_ManualTypeDef *regs;
    AES_ModeTypeDef    mode;
    AES_KeySizeTypeDef keysize;
} AES_HandleTypeDef;

/* Low-level streaming API — init once, call Process for each chunk */
void AES_constructor(AES_HandleTypeDef *h);
int  AES_Encrypt_Init(AES_HandleTypeDef *h, const uint8_t *key, const uint8_t *iv);
int  AES_Decrypt_Init(AES_HandleTypeDef *h, const uint8_t *key, const uint8_t *iv);
int  AES_Process(AES_HandleTypeDef *h, const uint8_t *in, uint8_t *out, uint32_t len);

/* One-shot mode functions — set up, process entire buffer, done.
 * len must be a multiple of AES_BLOCK_SIZE (16 bytes). */
int AES_ECB_Encrypt(const uint8_t *key, AES_KeySizeTypeDef ks, const uint8_t *in, uint8_t *out, uint32_t len);
int AES_ECB_Decrypt(const uint8_t *key, AES_KeySizeTypeDef ks, const uint8_t *in, uint8_t *out, uint32_t len);
int AES_CBC_Encrypt(const uint8_t *key, AES_KeySizeTypeDef ks, const uint8_t *iv,    const uint8_t *in, uint8_t *out, uint32_t len);
int AES_CBC_Decrypt(const uint8_t *key, AES_KeySizeTypeDef ks, const uint8_t *iv,    const uint8_t *in, uint8_t *out, uint32_t len);
int AES_CTR_Crypt  (const uint8_t *key, AES_KeySizeTypeDef ks, const uint8_t *nonce, const uint8_t *in, uint8_t *out, uint32_t len);

#endif /* __AES_H */
