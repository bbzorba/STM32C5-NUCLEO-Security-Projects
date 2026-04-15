// AES/SAES driver for STM32C562RE
#ifndef __AES_H
#define __AES_H

#include <stdint.h>
#include "../../GPIO/inc/gpio.h"  /* RCC_TypeDef, RCC */

#define __IO volatile

/* Peripheral base addresses (verified: STM32C562.svd, STM32C5A3.svd) */
#define AES_BASE   0x420C0000U
#define SAES_BASE  0x420C0C00U

/* RCC AHB2ENR clock-enable bits (verified: STM32C5A3.svd)
 * NOTE: bit 19 = PKAEN â€” SAESEN is bit 20, NOT 19 */
#define RCC_AHB2ENR_AESEN   (1U << 16)
#define RCC_AHB2ENR_SAESEN  (1U << 20)

/* AES_CR bits */
#define AES_CR_EN           (1U <<  0)
#define AES_CR_MODE_ENCRYPT (0U <<  3)   /* MODE = 00 */
#define AES_CR_MODE_KEYDER  (1U <<  3)   /* MODE = 01, key derivation */
#define AES_CR_MODE_DECRYPT (2U <<  3)   /* MODE = 10 */
/* CHMOD[2:0] at bits 6:5; use (AES_ModeTypeDef << 5): ECB=0, CBC=1, CTR=2 */
#define AES_CR_KEYSIZE_256  (1U << 18)   /* 0 = 128-bit, 1 = 256-bit */
#define AES_CR_IPRST        (1U << 31)   /* software reset */

/* SAES_CR: KEYSEL[2:0] at bits 28:26 (field value << 26) */
#define SAES_CR_KEYSEL_DHUK (1U << 26)   /* KEYSEL = 1, derived HW unique key */
#define SAES_CR_KEYSEL_BHK  (2U << 26)   /* KEYSEL = 2, boot HW key */

/* AES_SR bits */
#define AES_SR_KEYVALID (1U << 7)

/* AES_ISR bits */
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

/* Register map â€” identical for AES and SAES (SAES derivedFrom="AES" in SVD)
 * KEYR[0] = key bits[31:0]  (LSW), KEYR[3] = bits[127:96] (MSW)
 * IVR[0]  = IV  bits[31:0]  (LSW), IVR[3]  = bits[127:96] (MSW)
 * CCF is in ISR (offset 0x304, bit 0) â€” cleared by writing ICR */
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

#define AES  ((AES_ManualTypeDef *)AES_BASE)
#define SAES ((AES_ManualTypeDef *)SAES_BASE)

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

/* -----------------------------------------------------------------------
 * Shared inline helpers used by both aes.c and saes.c
 * ----------------------------------------------------------------------- */
static inline uint32_t aes_pack_be(const uint8_t *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}

/* KEYR[0]=LSW, KEYR[3]=MSW; for 256-bit (nw=8) also writes KEYRH */
static inline void aes_load_key(AES_ManualTypeDef *r, const uint8_t *k, int nw) {
    for (int i = 0; i < nw; i++) {
        uint32_t w = aes_pack_be(k + (nw-1-i)*4);
        if (i < 4) r->KEYR[i] = w; else r->KEYRH[i-4] = w;
    }
}

/* IVR[0]=iv[12..15] (LSW), IVR[3]=iv[0..3] (MSW) */
static inline void aes_load_iv(AES_ManualTypeDef *r, const uint8_t *iv) {
    for (int i = 0; i < 4; i++) r->IVR[i] = aes_pack_be(iv + (3-i)*4);
}

static inline void aes_write_block(AES_ManualTypeDef *r, const uint8_t *in) {
    for (int i = 0; i < 4; i++) r->DINR = aes_pack_be(in + i*4);
}

static inline void aes_read_block(AES_ManualTypeDef *r, uint8_t *out) {
    for (int i = 0; i < 4; i++) {
        uint32_t w = r->DOUTR;
        out[i*4]=w>>24; out[i*4+1]=w>>16; out[i*4+2]=w>>8; out[i*4+3]=(uint8_t)w;
    }
}

static inline int aes_wait_ccf(AES_ManualTypeDef *r) {
    uint32_t isr;
    do {
        isr = r->ISR;
        if (isr & (AES_ISR_RWEIF|AES_ISR_KEIF)) { r->ICR = AES_ICR_RWEIF|AES_ICR_KEIF; return AES_ERROR; }
    } while (!(isr & AES_ISR_CCF));
    r->ICR = AES_ICR_CCF;
    return AES_SUCCESS;
}

/* AES (non-secure peripheral) */
void AES_constructor(AES_HandleTypeDef *handle);
int  AES_Encrypt_Init(AES_HandleTypeDef *handle, const uint8_t *key, const uint8_t *iv);
int  AES_Decrypt_Init(AES_HandleTypeDef *handle, const uint8_t *key, const uint8_t *iv);
int  AES_Process(AES_HandleTypeDef *handle, const uint8_t *in, uint8_t *out, uint32_t len);

/* SAES (secure coprocessor) â€” always uses 256-bit key
 * AES_Encrypt_Init / AES_Decrypt_Init / AES_Process work for SAES too
 * (same register layout, different base address set by SAES_constructor) */
void SAES_constructor(AES_HandleTypeDef *handle);
int  SAES_DHUK_Encrypt_Init(AES_HandleTypeDef *handle, const uint8_t *iv);
int  SAES_DHUK_Decrypt_Init(AES_HandleTypeDef *handle, const uint8_t *iv);

#endif /* __AES_H */
