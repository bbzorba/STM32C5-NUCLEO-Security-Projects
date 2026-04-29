#include "../inc/hash.h"

void HASH_Init(HASH_HandleTypeDef *hhash)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_HASHEN;
    hhash->Instance = HASH_PERIPH;
    hhash->state    = HASH_OK;
    hhash->msg_len  = 0;
}

/* ── Static helpers shared by all algorithm Start/Final variants ─────── */

static HASH_StatusTypeDef hash_start(HASH_HandleTypeDef *hhash, uint32_t algo)
{
    hhash->Instance->CR  = HASH_CR_INIT | algo;
    while (!(hhash->Instance->SR & HASH_SR_DINIS));
    while (  hhash->Instance->SR & HASH_SR_DCIS);
    hhash->Instance->STR = 0U;   /* clear stale NBLW from any previous run */
    hhash->msg_len = 0;
    return HASH_OK;
}

static HASH_StatusTypeDef hash_final(HASH_HandleTypeDef *hhash, uint8_t *digest, int words)
{
    if (!digest) return HASH_ERROR;
    uint32_t nblw = (uint32_t)((hhash->msg_len % 4U) * 8U) & HASH_STR_NBLW_MASK;
    while (  hhash->Instance->SR & HASH_SR_DCIS);
    hhash->Instance->STR = nblw | HASH_STR_DCAL;
    while (!(hhash->Instance->SR & HASH_SR_DCIS));
    while (  hhash->Instance->SR & HASH_SR_BUSY);
    __asm volatile("dsb" : : : "memory");
    volatile uint32_t *hr = HASH_HR_OUT;
    for (int i = 0; i < words; i++) {
        uint32_t val  = hr[i];
        digest[i*4+0] = (uint8_t)(val >> 24);
        digest[i*4+1] = (uint8_t)(val >> 16);
        digest[i*4+2] = (uint8_t)(val >>  8);
        digest[i*4+3] = (uint8_t)(val);
    }
    return HASH_OK;
}

/* ── SHA-1 (160-bit / 5 words) ─────────────────────────────────────── */

HASH_StatusTypeDef HASH_SHA1_Start(HASH_HandleTypeDef *hhash)
{
    return hash_start(hhash, HASH_CR_ALGO_SHA1);
}

HASH_StatusTypeDef HASH_SHA1_Final(HASH_HandleTypeDef *hhash, uint8_t *digest)
{
    return hash_final(hhash, digest, 5);
}

/* ── SHA-224 (224-bit / 7 words) ────────────────────────────────────── */

HASH_StatusTypeDef HASH_SHA224_Start(HASH_HandleTypeDef *hhash)
{
    return hash_start(hhash, HASH_CR_ALGO_SHA224);
}

HASH_StatusTypeDef HASH_SHA224_Final(HASH_HandleTypeDef *hhash, uint8_t *digest)
{
    return hash_final(hhash, digest, 7);
}

/* ── SHA-256 (256-bit / 8 words) — unchanged ────────────────────────── */

HASH_StatusTypeDef HASH_SHA256_Start(HASH_HandleTypeDef *hhash)
{
    /* Write INIT + ALGO in a single write — ALGO is only sampled when INIT=1 */
    hhash->Instance->CR = HASH_CR_INIT | HASH_CR_ALGO_SHA256;
    /* INIT resets the engine and clears DCIS; wait for DINIS=1 (engine ready)
     * then confirm DCIS=0 before returning — DINIS can precede DCIS clearance. */
    while (!(hhash->Instance->SR & HASH_SR_DINIS));
    while (  hhash->Instance->SR & HASH_SR_DCIS);
    /* CRITICAL: INIT does NOT reset the STR register.  If the previous run left
     * a non-zero NBLW (e.g. a 3-byte message leaves NBLW=24), that value persists
     * in STR across INIT.  For messages whose length is a multiple of 4 bytes,
     * HASH_SHA256_Update never writes STR, so DIN words are fed while STR still
     * holds the stale NBLW — the hardware silently truncates the last word to the
     * old bit-count, reproducing the previous digest instead of the correct one.
     * Zeroing STR here guarantees NBLW=0 (all 32 bits valid) for every fresh run. */
    hhash->Instance->STR = 0U;
    hhash->msg_len = 0;
    return HASH_OK;
}

HASH_StatusTypeDef HASH_SHA256_Update(HASH_HandleTypeDef *hhash, const uint8_t *data, size_t len)
{
    if (!data || len == 0)
        return HASH_ERROR;

    size_t full_words = len / 4U;
    size_t rem        = len % 4U;

    /* Feed full 32-bit words; poll DINIS before each write */
    for (size_t i = 0; i < full_words; i++) {
        while (!(hhash->Instance->SR & HASH_SR_DINIS));
        uint32_t w = ((uint32_t)data[i*4+0] << 24) |
                     ((uint32_t)data[i*4+1] << 16) |
                     ((uint32_t)data[i*4+2] <<  8) |
                     ((uint32_t)data[i*4+3]);
        hhash->Instance->DIN = w;
    }

    /* Partial last word: set NBLW in STR BEFORE writing DIN */
    if (rem > 0) {
        while (!(hhash->Instance->SR & HASH_SR_DINIS));
        hhash->Instance->STR = (uint32_t)(rem * 8U) & HASH_STR_NBLW_MASK;
        uint32_t last = 0;
        for (size_t j = 0; j < rem; j++)
            last |= ((uint32_t)data[full_words*4 + j] << (24U - 8U*j));
        hhash->Instance->DIN = last;
    }

    hhash->msg_len += len;
    return HASH_OK;
}

HASH_StatusTypeDef HASH_SHA256_Final(HASH_HandleTypeDef *hhash, uint8_t *digest)
{
    if (!digest)
        return HASH_ERROR;

    /* NBLW = valid bits in last word (0 = all 32 bits valid, i.e. full-word message) */
    uint32_t nblw = (uint32_t)((hhash->msg_len % 4U) * 8U) & HASH_STR_NBLW_MASK;
    /* Guard against stale DCIS from a previous run (cleared by INIT in Start,
     * but confirm it is 0 before firing DCAL so we don't return old HR values). */
    while (  hhash->Instance->SR & HASH_SR_DCIS);
    hhash->Instance->STR = nblw | HASH_STR_DCAL;
    while (!(hhash->Instance->SR & HASH_SR_DCIS));
    while (  hhash->Instance->SR & HASH_SR_BUSY);
    __asm volatile("dsb" : : : "memory");

    volatile uint32_t *hr = HASH_HR_OUT;
    for (int i = 0; i < 8; i++) {
        uint32_t val  = hr[i];
        digest[i*4+0] = (uint8_t)(val >> 24);
        digest[i*4+1] = (uint8_t)(val >> 16);
        digest[i*4+2] = (uint8_t)(val >>  8);
        digest[i*4+3] = (uint8_t)(val);
    }

    return HASH_OK;
}