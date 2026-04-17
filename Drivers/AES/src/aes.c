#include "../inc/aes.h"

void AES_constructor(AES_HandleTypeDef *handle)
{
    handle->regs = AES;
    RCC->AHB2ENR |= RCC_AHB2ENR_AESEN;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Static helper functions for register access and CCF wait (not part of the public API)
static inline uint32_t pack_big_endian(const uint8_t *p) {
    return ((uint32_t)p[0]<<24) | ((uint32_t)p[1]<<16) | ((uint32_t)p[2]<<8) | (uint32_t)p[3];
}

/* KEYR[0]=LSW, KEYR[3]=MSW; for 256-bit (nw=8) also writes KEYRH */
static inline void load_key(AES_ManualTypeDef *regs, const uint8_t *k, int key_words) {
    for (int i = 0; i < key_words; i++) {
        uint32_t word = pack_big_endian(k + (key_words-1-i)*4);
        if (i < 4) 
            regs->KEYR[i] = word; 
        else 
            regs->KEYRH[i-4] = word;
    }
}

/* IVR[0]=iv[12..15] (LSW), IVR[3]=iv[0..3] (MSW) */
static inline void load_iv(AES_ManualTypeDef *regs, const uint8_t *iv) {
    for (int i = 0; i < 4; i++) 
        regs->IVR[i] = pack_big_endian(iv + (3-i)*4);
}

static inline void write_block(AES_ManualTypeDef *regs, const uint8_t *in) {
    for (int i = 0; i < 4; i++) 
        regs->DINR = pack_big_endian(in + i*4);
}

static inline void read_block(AES_ManualTypeDef *regs, uint8_t *out) {
    for (int i = 0; i < 4; i++) {
        uint32_t word = regs->DOUTR;
        out[i*4]=word>>24; 
        out[i*4+1]=word>>16; 
        out[i*4+2]=word>>8; 
        out[i*4+3]=(uint8_t)word;
    }
}

static inline int wait_ccf(AES_ManualTypeDef *regs) {
    uint32_t isr;
    do {
        isr = regs->ISR;
        if (isr & (AES_ISR_RWEIF|AES_ISR_KEIF)) { 
            regs->ICR = AES_ICR_RWEIF|AES_ICR_KEIF; 
            return AES_ERROR; 
        }
    } while (!(isr & AES_ISR_CCF));
    regs->ICR = AES_ICR_CCF;

    return AES_SUCCESS;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////



int AES_Encrypt_Init(AES_HandleTypeDef *handle, const uint8_t *key, const uint8_t *iv)
{
    int key_words       = (handle->keysize == AES_KEYSIZE_256) ? 8 : 4;
    uint32_t keysize_bit = (handle->keysize == AES_KEYSIZE_256) ? AES_CR_KEYSIZE_256 : 0;
    uint32_t chmod_bits  = (uint32_t)handle->mode << 5;    /* CR bits [6:5]: ECB=0, CBC=1, CTR=2 */

    handle->regs->CR = AES_CR_IPRST; 
    handle->regs->CR = 0;
    handle->regs->CR = keysize_bit;       /* set KEYSIZE before key load */
    load_key(handle->regs, key, key_words);
    if (iv) 
        load_iv(handle->regs, iv);
    handle->regs->CR = chmod_bits | AES_CR_MODE_ENCRYPT | keysize_bit | AES_CR_EN;

    return AES_SUCCESS;
}

int AES_Decrypt_Init(AES_HandleTypeDef *handle, const uint8_t *key, const uint8_t *iv)
{
    static const uint8_t z[AES_BLOCK_SIZE] = {0};
    int key_words        = (handle->keysize == AES_KEYSIZE_256) ? 8 : 4;
    uint32_t keysize_bit = (handle->keysize == AES_KEYSIZE_256) ? AES_CR_KEYSIZE_256 : 0;
    uint32_t chmod_bits  = (uint32_t)handle->mode << 5;    /* CR bits [6:5]: ECB=0, CBC=1, CTR=2 */

    if (handle->mode == AES_MODE_CTR) {
        /* CTR decrypt = same as encrypt (symmetric keystream) */
        handle->regs->CR = AES_CR_IPRST; 
        handle->regs->CR = 0;
        handle->regs->CR = keysize_bit;       /* set KEYSIZE before key load */
        load_key(handle->regs, key, key_words);
        if (iv) 
            load_iv(handle->regs, iv);
        handle->regs->CR = chmod_bits | AES_CR_MODE_ENCRYPT | keysize_bit | AES_CR_EN;

        return AES_SUCCESS;
    }

    /* ECB/CBC: compute inverse key schedule first */
    handle->regs->CR = AES_CR_IPRST; 
    handle->regs->CR = 0;
    handle->regs->CR = keysize_bit;       /* set KEYSIZE before key load */
    load_key(handle->regs, key, key_words);
    handle->regs->CR = AES_CR_MODE_KEYDER | keysize_bit | AES_CR_EN;  /* CHMOD=0 (ECB) for key deriv */
    write_block(handle->regs, z);
    if (wait_ccf(handle->regs)) 
        return AES_ERROR;

    handle->regs->CR = keysize_bit;       /* keep KEYSIZE set, clear EN */
    if (iv) 
        load_iv(handle->regs, iv);
    handle->regs->CR = chmod_bits | AES_CR_MODE_DECRYPT | keysize_bit | AES_CR_EN;
    
    return AES_SUCCESS;
}

int AES_Process(AES_HandleTypeDef *handle, const uint8_t *in, uint8_t *out, uint32_t len)
{
    if (len % AES_BLOCK_SIZE) 
        return AES_INVALID_DATA_SIZE;
    for (uint32_t i = 0; i < len; i += AES_BLOCK_SIZE) {
        write_block(handle->regs, in + i);
        if (wait_ccf(handle->regs)) 
            return AES_ERROR;
        read_block(handle->regs, out + i);
    }
    return AES_SUCCESS;
}