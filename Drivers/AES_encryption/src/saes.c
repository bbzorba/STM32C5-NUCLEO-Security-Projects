#include "../inc/aes.h"

/* SAES always requires 256-bit keys (hardware enforced per STM32C5A3.svd CCB constraints).
 * ECB/CBC/CTR modes share AES_Encrypt_Init / AES_Decrypt_Init / AES_Process â€”
 * the same functions work because SAES has the same register layout as AES. */

void SAES_constructor(AES_HandleTypeDef *handle)
{
    handle->regs    = SAES;
    handle->keysize = AES_KEYSIZE_256;     /* SAES requires 256-bit key */
    RCC->AHB2ENR |= RCC_AHB2ENR_SAESEN;  /* bit 20 â€” verified from STM32C5A3.svd */
}

/* DHUK: hardware-provisioned key, never software-readable.
 * After SAES_DHUK_*_Init, call AES_Process() to encrypt/decrypt. */

int SAES_DHUK_Encrypt_Init(AES_HandleTypeDef *handle, const uint8_t *iv)
{
    uint32_t ch = (uint32_t)handle->mode << 5;

    handle->regs->CR = AES_CR_IPRST; handle->regs->CR = 0;
    handle->regs->CR = SAES_CR_KEYSEL_DHUK | AES_CR_EN;
    uint32_t t = 100000U;
    while (!(handle->regs->SR & AES_SR_KEYVALID)) if (!--t) return AES_ERROR;

    handle->regs->CR = 0;
    if (iv) aes_load_iv(handle->regs, iv);
    handle->regs->CR = SAES_CR_KEYSEL_DHUK | ch | AES_CR_MODE_ENCRYPT | AES_CR_KEYSIZE_256 | AES_CR_EN;
    return AES_SUCCESS;
}

int SAES_DHUK_Decrypt_Init(AES_HandleTypeDef *handle, const uint8_t *iv)
{
    static const uint8_t z[AES_BLOCK_SIZE] = {0};
    uint32_t ch = (uint32_t)handle->mode << 5;

    handle->regs->CR = AES_CR_IPRST; handle->regs->CR = 0;
    handle->regs->CR = SAES_CR_KEYSEL_DHUK | AES_CR_EN;
    uint32_t t = 100000U;
    while (!(handle->regs->SR & AES_SR_KEYVALID)) if (!--t) return AES_ERROR;

    if (handle->mode != AES_MODE_CTR) {
        handle->regs->CR = 0;
        handle->regs->CR = SAES_CR_KEYSEL_DHUK | AES_CR_MODE_KEYDER | AES_CR_KEYSIZE_256 | AES_CR_EN;
        aes_write_block(handle->regs, z);
        if (aes_wait_ccf(handle->regs)) return AES_ERROR;
    }

    handle->regs->CR = 0;
    if (iv) aes_load_iv(handle->regs, iv);
    uint32_t md = (handle->mode == AES_MODE_CTR) ? AES_CR_MODE_ENCRYPT : AES_CR_MODE_DECRYPT;
    handle->regs->CR = SAES_CR_KEYSEL_DHUK | ch | md | AES_CR_KEYSIZE_256 | AES_CR_EN;
    return AES_SUCCESS;
}
