#include "../inc/hash.h"
#include <string.h>

void Hash_Constructor(HASH_HandleTypeDef *hash, HASH_AlgorithmTypeDef algorithm) {
    hash->regs      = HASH_regs;
    hash->algorithm = algorithm;
    hash->state     = HASH_OK;
    hash->msg_len   = 0;
    /* Enable HASH peripheral clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_HASHEN;
    Hash_Init(hash);
}

void Hash_Init(HASH_HandleTypeDef *hash) {
    /* ALGO is only latched when INIT=1 is set simultaneously (SVD: "This selection
     * is only taken into account when the INIT bit is set").  Build the full CR
     * value with INIT + ALGO in a single write so the hardware samples the
     * correct algorithm at the moment the engine resets.              */
    uint32_t cr = HASH_CR_INIT;   /* start with INIT=1, all other bits = 0 */
    if (hash->algorithm == HASH_SHA1)
        cr |= HASH_CR_ALGO_SHA1;
    else if (hash->algorithm == HASH_SHA224)
        cr |= HASH_CR_ALGO_SHA224;
    else
        cr |= HASH_CR_ALGO_SHA256;
    hash->regs->CR = cr;
    /* INIT resets the engine; wait DINIS=1 then confirm DCIS=0 before returning. */
    while (!(hash->regs->SR & HASH_SR_DINIS));
    while (  hash->regs->SR & HASH_SR_DCIS);
    hash->state   = HASH_OK;
    hash->msg_len = 0;
    memset(hash->hash_output, 0, sizeof(hash->hash_output));
}

void Hash_Reset(HASH_HandleTypeDef *hash) {
    /* Same INIT+ALGO-simultaneous rule applies here */
    uint32_t cr = HASH_CR_INIT;
    if (hash->algorithm == HASH_SHA1)
        cr |= HASH_CR_ALGO_SHA1;
    else if (hash->algorithm == HASH_SHA224)
        cr |= HASH_CR_ALGO_SHA224;
    else
        cr |= HASH_CR_ALGO_SHA256;
    hash->regs->CR = cr;
    /* Wait for reset to complete before returning */
    while (!(hash->regs->SR & HASH_SR_DINIS));
    while (  hash->regs->SR & HASH_SR_DCIS);
    hash->state   = HASH_OK;
    hash->msg_len = 0;
    memset(hash->hash_output, 0, sizeof(hash->hash_output));
}

HASH_StatusTypeDef Hash_Process_Data
    if (!data || len == 0)
        return HASH_ERROR;

    /* DATATYPE=0 (no byte swap): we pack bytes into big-endian 32-bit words
     * ourselves.  Full words are fed via LPDMA1; the last partial word (if
     * any) is written by the CPU after waiting for DINIS.               */
    size_t full_words = len / 4U;
    size_t rem        = len % 4U;

    /* Feed full 32-bit words via CPU (big-endian packing, poll DINIS each word) */
    for (size_t i = 0; i < full_words; i++) {
        while (!(hash->regs->SR & HASH_SR_DINIS));
        uint32_t w = ((uint32_t)data[i*4+0] << 24) |
                     ((uint32_t)data[i*4+1] << 16) |
                     ((uint32_t)data[i*4+2] <<  8) |
                     ((uint32_t)data[i*4+3]);
        hash->regs->DIN = w;
    }

    /* Write last partial word via CPU (left-justified, big-endian).         *
     * CRITICAL: NBLW must be written to STR BEFORE the DIN write, not after. *
     * The peripheral samples NBLW at the moment DIN is written; writing NBLW  *
     * afterwards (before DCAL) has no effect — the hardware already latched   *
     * NBLW=0 (all 32 bits valid) at DIN-write time.                           */
    if (rem > 0) {
        while (!(hash->regs->SR & HASH_SR_DINIS));
        /* Step 1: set NBLW=valid-bits BEFORE writing the partial last word */
        uint32_t nblw = (uint32_t)(rem * 8U) & HASH_STR_NBLW_MASK;
        hash->regs->STR = nblw;
        /* Step 2: write the left-justified last word */
        uint32_t last = 0;
        for (size_t j = 0; j < rem; j++)
            last |= ((uint32_t)data[full_words*4 + j] << (24U - 8U*j));
        hash->regs->DIN = last;
    }

    hash->msg_len += len;
    return HASH_OK;
}

HASH_StatusTypeDef Hash_Final(HASH_HandleTypeDef *hash, unsigned char *hash_output)
{
    if (!hash_output)
        return HASH_ERROR;

    /* Ensure DMA is disabled before triggering digest calculation */
    hash->regs->CR &= ~HASH_CR_DMAE;

    /* Write NBLW (valid bits in last word) together with DCAL so the
     * hardware sees the correct padding length when it starts the final
     * computation.  NBLW = (msg_len % 4) * 8; 0 means all 32 bits valid
     * (exact multiple of 4 bytes).                                       */
    uint32_t nblw = (uint32_t)((hash->msg_len % 4U) * 8U) & HASH_STR_NBLW_MASK;
    /* Guard against stale DCIS before firing DCAL */
    while (  hash->regs->SR & HASH_SR_DCIS);
    hash->regs->STR = nblw | HASH_STR_DCAL;
    while (!(hash->regs->SR & HASH_SR_DCIS));
    while (  hash->regs->SR & HASH_SR_BUSY );
    __asm volatile("dsb" : : : "memory");

    /* SHA-1=5 words, SHA-224=7 words, SHA-256=8 words */
    int words = (hash->algorithm == HASH_SHA1)   ? 5 :
                (hash->algorithm == HASH_SHA224)  ? 7 : 8;

    /* Read digest from HASH_HR0-HR7 at HASH_BASE + 0x310 (verified from SVD) */
    volatile uint32_t *hr = HASH_HR_OUT;
    for (int i = 0; i < words; i++) {
        uint32_t w = hr[i];
        hash_output[i*4+0] = (uint8_t)(w >> 24);
        hash_output[i*4+1] = (uint8_t)(w >> 16);
        hash_output[i*4+2] = (uint8_t)(w >>  8);
        hash_output[i*4+3] = (uint8_t)(w);
    }

    hash->state = HASH_OK;
    return HASH_OK;
}
