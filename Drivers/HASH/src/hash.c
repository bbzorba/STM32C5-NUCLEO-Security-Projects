#include "../inc/hash.h"
#include <string.h>

void Hash_Constructor(HASH_HandleTypeDef *hash, HASH_AlgorithmTypeDef algorithm) {
    hash->regs      = HASH_regs;
    hash->algorithm = algorithm;
    hash->state     = HASH_OK;
    hash->msg_len   = 0;
    /* Enable clocks before any register access */
    RCC->AHB1ENR |= RCC_AHB1ENR_LPDMA1EN;  /* LPDMA1 for DMA data feed  */
    RCC->AHB2ENR |= RCC_AHB2ENR_HASHEN;    /* HASH peripheral           */
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
    /* DATATYPE=0 (32-bit, no swap), DMAE=0, MDMAT=0, MODE=0 — all clear */
    hash->regs->CR = cr;
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
    hash->state   = HASH_OK;
    hash->msg_len = 0;
    memset(hash->hash_output, 0, sizeof(hash->hash_output));
}

HASH_StatusTypeDef Hash_Process_Data(HASH_HandleTypeDef *hash,
                                     const unsigned char *data, size_t len)
{
    if (!data || len == 0)
        return HASH_ERROR;

    /* DATATYPE=0 (no byte swap): we pack bytes into big-endian 32-bit words
     * ourselves.  Full words are fed via LPDMA1; the last partial word (if
     * any) is written by the CPU after waiting for DINIS.               */
    size_t full_words = len / 4U;
    size_t rem        = len % 4U;

    if (full_words > 0) {
        /* Build a byte-ordered DMA source buffer on the stack. For long messages this should be heap-allocated. */
        uint32_t dma_buf[full_words];
        for (size_t i = 0; i < full_words; i++) {
            dma_buf[i] = ((uint32_t)data[i*4+0] << 24) |
                         ((uint32_t)data[i*4+1] << 16) |
                         ((uint32_t)data[i*4+2] <<  8) |
                         ((uint32_t)data[i*4+3]);
        }

        /* Enable HASH DMA request generation */
        hash->regs->CR |= HASH_CR_DMAE;

        /* Reset LPDMA1 Channel 0 */
        LPDMA1_C0CR = LPDMA1_CCR_RESET;
        while (LPDMA1_C0CR & LPDMA1_CCR_RESET);

        LPDMA1_C0CFCR = 0x1FFU;  /* clear all flags */

        /* CTR1: src=word(2), SINC=1, dst=word(2), DINC=0 */
        LPDMA1_C0TR1 = LPDMA1_CTR1_MEM_TO_PERIPH_WORD;

        /* CTR2: REQSEL=63 (HASH_IN), hardware request */
        LPDMA1_C0TR2 = LPDMA1_REQUEST_HASH_IN;

        /* CBR1: byte count */
        LPDMA1_C0BR1 = (uint32_t)(full_words * 4U);

        LPDMA1_C0SAR = (uint32_t)dma_buf;
        LPDMA1_C0DAR = HASH_DIN_ADDR;
        LPDMA1_C0LLR = 0U;

        /* Enable channel */
        LPDMA1_C0CR = LPDMA1_CCR_EN;

        /* Wait for transfer complete */
        while (!(LPDMA1_C0SR & LPDMA1_C0SR_TCF));

        /* Disable HASH DMA requests */
        hash->regs->CR &= ~HASH_CR_DMAE;
    }

    /* Write last partial word via CPU (left-justified, big-endian) */
    if (rem > 0) {
        while (!(hash->regs->SR & HASH_SR_DINIS));
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

    /* NBLW = number of valid bits in the last word (0 = all 32 bits valid).
     * Step 1: write NBLW while DCAL=0 (constraint: can't change NBLW while
     *         a computation is active / DCAL=1).
     * Step 2: |= DCAL — read-modify-write preserves the NBLW just written.
     *         A plain write (= HASH_STR_DCAL) would zero NBLW back to 0,
     *         causing the hardware to treat all 32 bits as valid (wrong).  */
    uint32_t nblw = (uint32_t)((hash->msg_len % 4U) * 8U);
    hash->regs->STR = nblw & HASH_STR_NBLW_MASK;  /* step 1: set NBLW    */
    hash->regs->STR |= HASH_STR_DCAL;              /* step 2: RMW, keep NBLW */

    /* Wait for digest calculation complete (DCIS set in SR) */
    while (!(hash->regs->SR & HASH_SR_DCIS));

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
