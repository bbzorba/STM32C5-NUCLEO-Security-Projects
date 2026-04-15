#include "../inc/aes.h"

void AES_constructor(AES_HandleTypeDef *handle)
{
    handle->regs = AES;
    RCC->AHB2ENR |= RCC_AHB2ENR_AESEN;
}

int AES_Encrypt_Init(AES_HandleTypeDef *handle, const uint8_t *key, const uint8_t *iv)
{
    int nw      = (handle->keysize == AES_KEYSIZE_256) ? 8 : 4;
    uint32_t kr = (handle->keysize == AES_KEYSIZE_256) ? AES_CR_KEYSIZE_256 : 0;
    uint32_t ch = (uint32_t)handle->mode << 5;

    handle->regs->CR = AES_CR_IPRST; handle->regs->CR = 0;
    aes_load_key(handle->regs, key, nw);
    if (iv) aes_load_iv(handle->regs, iv);
    handle->regs->CR = ch | AES_CR_MODE_ENCRYPT | kr | AES_CR_EN;
    return AES_SUCCESS;
}

int AES_Decrypt_Init(AES_HandleTypeDef *handle, const uint8_t *key, const uint8_t *iv)
{
    static const uint8_t z[AES_BLOCK_SIZE] = {0};
    int nw      = (handle->keysize == AES_KEYSIZE_256) ? 8 : 4;
    uint32_t kr = (handle->keysize == AES_KEYSIZE_256) ? AES_CR_KEYSIZE_256 : 0;
    uint32_t ch = (uint32_t)handle->mode << 5;

    if (handle->mode == AES_MODE_CTR) {
        /* CTR decrypt = same as encrypt (symmetric keystream) */
        handle->regs->CR = AES_CR_IPRST; handle->regs->CR = 0;
        aes_load_key(handle->regs, key, nw);
        if (iv) aes_load_iv(handle->regs, iv);
        handle->regs->CR = ch | AES_CR_MODE_ENCRYPT | kr | AES_CR_EN;
        return AES_SUCCESS;
    }

    /* ECB/CBC: compute inverse key schedule first */
    handle->regs->CR = AES_CR_IPRST; handle->regs->CR = 0;
    aes_load_key(handle->regs, key, nw);
    handle->regs->CR = AES_CR_MODE_KEYDER | kr | AES_CR_EN;  /* CHMOD=0 (ECB) for key deriv */
    aes_write_block(handle->regs, z);
    if (aes_wait_ccf(handle->regs)) return AES_ERROR;

    handle->regs->CR = 0;
    if (iv) aes_load_iv(handle->regs, iv);
    handle->regs->CR = ch | AES_CR_MODE_DECRYPT | kr | AES_CR_EN;
    return AES_SUCCESS;
}

int AES_Process(AES_HandleTypeDef *handle, const uint8_t *in, uint8_t *out, uint32_t len)
{
    if (len % AES_BLOCK_SIZE) return AES_INVALID_DATA_SIZE;
    for (uint32_t i = 0; i < len; i += AES_BLOCK_SIZE) {
        aes_write_block(handle->regs, in + i);
        if (aes_wait_ccf(handle->regs)) return AES_ERROR;
        aes_read_block(handle->regs, out + i);
    }
    return AES_SUCCESS;
}

