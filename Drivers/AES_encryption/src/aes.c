#include "../inc/aes.h"

void AES_constructor(AES_HandleTypeDef *handle)
{
    handle->regs = AES;
    RCC->AHB2ENR |= RCC_AHB2ENR_AESEN;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static inline uint32_t pack_big_endian(const uint8_t *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}

/* KEYR[0]=LSW, KEYR[3]=MSW; for 256-bit (nw=8) also writes KEYRH */
static inline void load_key(AES_ManualTypeDef *r, const uint8_t *k, int nw) {
    for (int i = 0; i < nw; i++) {
        uint32_t w = pack_big_endian(k + (nw-1-i)*4);
        if (i < 4) r->KEYR[i] = w; else r->KEYRH[i-4] = w;
    }
}

/* IVR[0]=iv[12..15] (LSW), IVR[3]=iv[0..3] (MSW) */
static inline void load_iv(AES_ManualTypeDef *r, const uint8_t *iv) {
    for (int i = 0; i < 4; i++) r->IVR[i] = pack_big_endian(iv + (3-i)*4);
}

static inline void write_block(AES_ManualTypeDef *r, const uint8_t *in) {
    for (int i = 0; i < 4; i++) r->DINR = pack_big_endian(in + i*4);
}

static inline void read_block(AES_ManualTypeDef *r, uint8_t *out) {
    for (int i = 0; i < 4; i++) {
        uint32_t w = r->DOUTR;
        out[i*4]=w>>24; out[i*4+1]=w>>16; out[i*4+2]=w>>8; out[i*4+3]=(uint8_t)w;
    }
}

static inline int wait_ccf(AES_ManualTypeDef *r) {
    uint32_t isr;
    do {
        isr = r->ISR;
        if (isr & (AES_ISR_RWEIF|AES_ISR_KEIF)) { r->ICR = AES_ICR_RWEIF|AES_ICR_KEIF; return AES_ERROR; }
    } while (!(isr & AES_ISR_CCF));
    r->ICR = AES_ICR_CCF;
    return AES_SUCCESS;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////



int AES_Encrypt_Init(AES_HandleTypeDef *handle, const uint8_t *key, const uint8_t *iv)
{
    int nw      = (handle->keysize == AES_KEYSIZE_256) ? 8 : 4;
    uint32_t kr = (handle->keysize == AES_KEYSIZE_256) ? AES_CR_KEYSIZE_256 : 0;
    uint32_t ch = (uint32_t)handle->mode << 5;

    handle->regs->CR = AES_CR_IPRST; 
    handle->regs->CR = 0;
    handle->regs->CR = kr;                /* set KEYSIZE before key load */
    load_key(handle->regs, key, nw);
    if (iv) load_iv(handle->regs, iv);
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
        handle->regs->CR = AES_CR_IPRST; 
        handle->regs->CR = 0;
        handle->regs->CR = kr;                /* set KEYSIZE before key load */
        load_key(handle->regs, key, nw);
        if (iv) load_iv(handle->regs, iv);
        handle->regs->CR = ch | AES_CR_MODE_ENCRYPT | kr | AES_CR_EN;
        return AES_SUCCESS;
    }

    /* ECB/CBC: compute inverse key schedule first */
    handle->regs->CR = AES_CR_IPRST; 
    handle->regs->CR = 0;
    handle->regs->CR = kr;                /* set KEYSIZE before key load */
    load_key(handle->regs, key, nw);
    handle->regs->CR = AES_CR_MODE_KEYDER | kr | AES_CR_EN;  /* CHMOD=0 (ECB) for key deriv */
    write_block(handle->regs, z);
    if (wait_ccf(handle->regs)) return AES_ERROR;

    handle->regs->CR = kr;                /* keep KEYSIZE set, clear EN */
    if (iv) load_iv(handle->regs, iv);
    handle->regs->CR = ch | AES_CR_MODE_DECRYPT | kr | AES_CR_EN;
    return AES_SUCCESS;
}

int AES_Process(AES_HandleTypeDef *handle, const uint8_t *in, uint8_t *out, uint32_t len)
{
    if (len % AES_BLOCK_SIZE) return AES_INVALID_DATA_SIZE;
    for (uint32_t i = 0; i < len; i += AES_BLOCK_SIZE) {
        write_block(handle->regs, in + i);
        if (wait_ccf(handle->regs)) return AES_ERROR;
        read_block(handle->regs, out + i);
    }
    return AES_SUCCESS;
}

