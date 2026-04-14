#include "../inc/aes.h"

/* -----------------------------------------------------------------------
 * SAES — Secure AES peripheral (TrustZone, accessible from Secure world)
 *
 * The SAES peripheral is mapped at SAES_BASE (0x420C0C00) and has the
 * same register layout as AES.  It is enabled via RCC AHB2ENR bit 19
 * (SAESEN) and is only accessible from the Secure execution state.
 *
 * This driver provides the same API as the AES driver so callers can
 * swap between AES and SAES by choosing the constructor.
 * ----------------------------------------------------------------------- */

#define RCC_AHB2ENR_SAESEN  (1U << 19)

/* -----------------------------------------------------------------------
 * Internal helpers (duplicated from aes.c to keep each TU self-contained)
 * ----------------------------------------------------------------------- */

static void saes_load_key(volatile uint32_t *reg_base, const uint8_t *key, int num_words)
{
    for (int i = 0; i < num_words; i++) {
        int b = i * 4;
        reg_base[i] = ((uint32_t)key[b]     << 24) |
                      ((uint32_t)key[b + 1]  << 16) |
                      ((uint32_t)key[b + 2]  <<  8) |
                       (uint32_t)key[b + 3];
    }
}

static void saes_load_iv(AES_ManualTypeDef *regs, const uint8_t *iv)
{
    for (int i = 0; i < 4; i++) {
        int b = i * 4;
        regs->IVR[i] = ((uint32_t)iv[b]     << 24) |
                       ((uint32_t)iv[b + 1]  << 16) |
                       ((uint32_t)iv[b + 2]  <<  8) |
                        (uint32_t)iv[b + 3];
    }
}

static void saes_write_block(AES_ManualTypeDef *regs, const uint8_t *in)
{
    for (int i = 0; i < 4; i++) {
        int b = i * 4;
        regs->DINR = ((uint32_t)in[b]     << 24) |
                     ((uint32_t)in[b + 1]  << 16) |
                     ((uint32_t)in[b + 2]  <<  8) |
                      (uint32_t)in[b + 3];
    }
}

static void saes_read_block(AES_ManualTypeDef *regs, uint8_t *out)
{
    for (int i = 0; i < 4; i++) {
        uint32_t w = regs->DOUTR;
        int b = i * 4;
        out[b]     = (uint8_t)(w >> 24);
        out[b + 1] = (uint8_t)(w >> 16);
        out[b + 2] = (uint8_t)(w >>  8);
        out[b + 3] = (uint8_t)(w);
    }
}

static int saes_wait_ccf(AES_ManualTypeDef *regs)
{
    uint32_t sr;
    do {
        sr = regs->SR;
        if (sr & (AES_SR_RDERR | AES_SR_WRERR))
            return AES_ERROR;
    } while (!(sr & AES_SR_CCF));
    regs->CR |= AES_CR_CCFC;
    return AES_SUCCESS;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void SAES_constructor(AES_HandleTypeDef *AESx)
{
    AESx->regs = SAES;
    /* Enable SAES peripheral clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_SAESEN;
}

int SAES_Encrypt_Init(AES_HandleTypeDef *AESx, uint8_t *key, uint8_t *iv)
{
    AES_ManualTypeDef *r = AESx->regs;
    r->CR = 0;
    saes_load_key(r->KEYR, key, 4);
    saes_load_iv(r, iv);
    r->CR = AES_CR_CHMOD_CBC | AES_CR_MODE_ENCRYPT | AES_CR_EN;
    return AES_SUCCESS;
}

int SAES_Encrypt(AES_HandleTypeDef *AESx, uint8_t *input, uint8_t *output, uint32_t length)
{
    AES_ManualTypeDef *r = AESx->regs;
    int ret;

    if (length % AES_BLOCK_SIZE != 0)
        return AES_INVALID_DATA_SIZE;

    for (uint32_t i = 0; i < length; i += AES_BLOCK_SIZE) {
        saes_write_block(r, input + i);
        ret = saes_wait_ccf(r);
        if (ret != AES_SUCCESS)
            return ret;
        saes_read_block(r, output + i);
    }
    return AES_SUCCESS;
}

int SAES_Decrypt_Init(AES_HandleTypeDef *AESx, uint8_t *key, uint8_t *iv)
{
    AES_ManualTypeDef *r = AESx->regs;
    r->CR = 0;
    saes_load_key(r->KEYR, key, 4);
    saes_load_iv(r, iv);
    r->CR = AES_CR_CHMOD_CBC | AES_CR_MODE_DECRYPT | AES_CR_EN;
    return AES_SUCCESS;
}

int SAES_Decrypt(AES_HandleTypeDef *AESx, uint8_t *input, uint8_t *output, uint32_t length)
{
    AES_ManualTypeDef *r = AESx->regs;
    int ret;

    if (length % AES_BLOCK_SIZE != 0)
        return AES_INVALID_DATA_SIZE;

    for (uint32_t i = 0; i < length; i += AES_BLOCK_SIZE) {
        saes_write_block(r, input + i);
        ret = saes_wait_ccf(r);
        if (ret != AES_SUCCESS)
            return ret;
        saes_read_block(r, output + i);
    }
    return AES_SUCCESS;
}

