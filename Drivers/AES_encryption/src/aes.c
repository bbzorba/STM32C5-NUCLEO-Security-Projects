#include <string.h>
#include "../inc/aes.h"

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

/* Load key words: key_bytes is big-endian byte array, hardware wants MSW first */
static void load_key_words(volatile uint32_t *reg_base, const uint8_t *key_bytes, int num_words)
{
    for (int i = 0; i < num_words; i++) {
        int b = i * 4;
        reg_base[i] = ((uint32_t)key_bytes[b]     << 24) |
                      ((uint32_t)key_bytes[b + 1]  << 16) |
                      ((uint32_t)key_bytes[b + 2]  <<  8) |
                       (uint32_t)key_bytes[b + 3];
    }
}

static void load_iv_words(AES_ManualTypeDef *regs, const uint8_t *iv)
{
    for (int i = 0; i < 4; i++) {
        int b = i * 4;
        regs->IVR[i] = ((uint32_t)iv[b]     << 24) |
                       ((uint32_t)iv[b + 1]  << 16) |
                       ((uint32_t)iv[b + 2]  <<  8) |
                        (uint32_t)iv[b + 3];
    }
}

static void write_block(AES_ManualTypeDef *regs, const uint8_t *in)
{
    for (int i = 0; i < 4; i++) {
        int b = i * 4;
        regs->DINR = ((uint32_t)in[b]     << 24) |
                     ((uint32_t)in[b + 1]  << 16) |
                     ((uint32_t)in[b + 2]  <<  8) |
                      (uint32_t)in[b + 3];
    }
}

static void read_block(AES_ManualTypeDef *regs, uint8_t *out)
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

static int wait_ccf(AES_ManualTypeDef *regs)
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

/* RCC clock enable bits for AES/SAES on STM32C562RE (AHB2ENR) */
#define RCC_AHB2ENR_AESEN   (1U << 16)
#define RCC_AHB2ENR_SAESEN  (1U << 19)

/* -----------------------------------------------------------------------
 * AES (Non-Secure)
 * ----------------------------------------------------------------------- */

void AES_constructor(AES_HandleTypeDef *AESx)
{
    AESx->regs = AES;
    RCC->AHB2ENR |= RCC_AHB2ENR_AESEN;
}

int AES_Encrypt_Init(AES_HandleTypeDef *AESx, uint8_t *key, uint8_t *iv)
{
    AES_ManualTypeDef *r = AESx->regs;
    r->CR = 0;
    load_key_words(r->KEYR, key, 4);
    load_iv_words(r, iv);
    r->CR = AES_CR_CHMOD_CBC | AES_CR_MODE_ENCRYPT | AES_CR_EN;
    return AES_SUCCESS;
}

int AES_Encrypt(AES_HandleTypeDef *AESx, uint8_t *input, uint8_t *output, uint32_t length)
{
    AES_ManualTypeDef *r = AESx->regs;
    int ret;

    if (length % AES_BLOCK_SIZE != 0)
        return AES_INVALID_DATA_SIZE;

    for (uint32_t i = 0; i < length; i += AES_BLOCK_SIZE) {
        write_block(r, input + i);
        ret = wait_ccf(r);
        if (ret != AES_SUCCESS)
            return ret;
        read_block(r, output + i);
    }
    return AES_SUCCESS;
}

int AES_Decrypt_Init(AES_HandleTypeDef *AESx, uint8_t *key, uint8_t *iv)
{
    AES_ManualTypeDef *r = AESx->regs;
    r->CR = 0;
    load_key_words(r->KEYR, key, 4);
    load_iv_words(r, iv);
    r->CR = AES_CR_CHMOD_CBC | AES_CR_MODE_DECRYPT | AES_CR_EN;
    return AES_SUCCESS;
}

int AES_Decrypt(AES_HandleTypeDef *AESx, uint8_t *input, uint8_t *output, uint32_t length)
{
    AES_ManualTypeDef *r = AESx->regs;
    int ret;

    if (length % AES_BLOCK_SIZE != 0)
        return AES_INVALID_DATA_SIZE;

    for (uint32_t i = 0; i < length; i += AES_BLOCK_SIZE) {
        write_block(r, input + i);
        ret = wait_ccf(r);
        if (ret != AES_SUCCESS)
            return ret;
        read_block(r, output + i);
    }
    return AES_SUCCESS;
}

void Init_Sample_Test_Vector(uint8_t *key, uint8_t *iv, uint8_t *plaintext)
{
    /* NIST AES-128-CBC F.2.1 test vector */
    static const uint8_t sample_key[AES_KEY_SIZE] = {
        0x2b, 0x7e, 0x15, 0x16,
        0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88,
        0x09, 0xcf, 0x4f, 0x3c
    };
    static const uint8_t sample_iv[AES_IV_SIZE] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b,
        0x0c, 0x0d, 0x0e, 0x0f
    };
    static const uint8_t sample_plaintext[AES_BLOCK_SIZE] = {
        0x6b, 0xc1, 0xbe, 0xe2,
        0x2e, 0x40, 0x9f, 0xf9,
        0x92, 0xcb, 0xab, 0x1e,
        0xc7, 0xe3, 0xea, 0xd9
    };
    memcpy(key,       sample_key,       AES_KEY_SIZE);
    memcpy(iv,        sample_iv,        AES_IV_SIZE);
    memcpy(plaintext, sample_plaintext, AES_BLOCK_SIZE);
}