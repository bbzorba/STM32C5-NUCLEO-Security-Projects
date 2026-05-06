#include "../inc/rng.h"

void RNG_Constructor(RNG_HandleTypeDef *hrng)
{
    /* Reset peripheral to clear any locked state from previous sessions */
    RCC->AHB2ENR  |=  RCC_AHB2ENR_RNGEN;   /* clock on before reset */
    RCC->AHB2RSTR |=  RCC_AHB2RSTR_RNGEN;  /* assert reset */
    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_RNGEN;  /* release reset */

    hrng->Instance = TRNG;
    hrng->status   = RNG_OK;
    hrng->it_buf   = 0;
    hrng->it_len   = 0;
    hrng->it_idx   = 0;
}

void RNG_Enable(RNG_HandleTypeDef *hrng)
{
    /* Config bits (CED etc.) are writable only while CONDRST=1.
     * On C562, keep CONDRST=1 during generation — DRDY will assert when ready. */
    hrng->Instance->CR |= RNG_CR_RNGEN | RNG_CR_CED | RNG_CR_CONDRST;
    hrng->Instance->HTCR0 = RNG_HTCR0_MAGIC;  /* re-arm health tests */
}

void RNG_Disable(RNG_HandleTypeDef *hrng)
{
    hrng->Instance->CR &= ~RNG_CR_RNGEN;
}

RNG_StatusTypeDef RNG_Generate(RNG_HandleTypeDef *hrng, uint8_t *buffer, size_t length)
{
    if (!hrng || !buffer || length == 0) { return RNG_ERROR; }

    for (size_t i = 0; i < length; ) {
        /* Clear any seed/clock error before waiting */
        if (hrng->Instance->SR & (RNG_SR_SEIS | RNG_SR_CEIS)) {
            hrng->Instance->SR &= ~(RNG_SR_SEIS | RNG_SR_CEIS);
            return RNG_ERROR;
        }

        /* Wait for a 32-bit word to be ready */
        uint32_t timeout = 5000000U;  /* ~0.6 s at 48 MHz */
        while (!(hrng->Instance->SR & RNG_SR_DRDY)) {
            if (--timeout == 0) { return RNG_ERROR; }
        }

        /* Consume the 32-bit word byte by byte */
        uint32_t word = hrng->Instance->DR;
        for (uint8_t b = 0; b < 4U && i < length; b++, i++) {
            buffer[i] = (uint8_t)(word >> (b * 8U));
        }
    }

    return RNG_OK;
}

/* -----------------------------------------------------------------------
 * Interrupt-based generation
 * ----------------------------------------------------------------------- */

static void rng_irq_handler(void *arg)
{
    RNG_HandleTypeDef *hrng = (RNG_HandleTypeDef *)arg;

    /* Check for errors first — if an error occurred, clear it and abort the operation. */
    if (hrng->Instance->SR & (RNG_SR_SEIS | RNG_SR_CEIS)) {
        hrng->Instance->SR &= ~(RNG_SR_SEIS | RNG_SR_CEIS);
        hrng->Instance->CR &= ~RNG_CR_IE;
        NVIC_UnregisterHandler(RNG_IRQn);
        hrng->status = RNG_ERROR;
        return;
    }

    /* Check if data is ready. If not, just return and wait for the next interrupt. */
    if (!(hrng->Instance->SR & RNG_SR_DRDY)) { return; }

    /* Read the 32-bit word and copy it byte by byte into the user buffer. */
    uint32_t word = hrng->Instance->DR;
    for (uint8_t b = 0; b < 4U && hrng->it_idx < hrng->it_len; b++, hrng->it_idx++) {
        hrng->it_buf[hrng->it_idx] = (uint8_t)(word >> (b * 8U));
    }

    /* If we've filled the user buffer, disable interrupts and mark the operation as complete. */
    if (hrng->it_idx >= hrng->it_len) {
        hrng->Instance->CR &= ~RNG_CR_IE;
        NVIC_UnregisterHandler(RNG_IRQn);
        hrng->status = RNG_OK;
    }
}

RNG_StatusTypeDef RNG_Generate_IT(RNG_HandleTypeDef *hrng, uint8_t *buffer, size_t length)
{
    hrng->it_buf = buffer;
    hrng->it_len = length;
    hrng->it_idx = 0U;
    hrng->status = RNG_BUSY;

    NVIC_RegisterHandler(RNG_IRQn, rng_irq_handler, hrng, 0U);
    hrng->Instance->CR |= RNG_CR_IE;

    uint32_t timeout = 2000000U;
    while (hrng->status == RNG_BUSY) {
        if (--timeout == 0) {
            hrng->Instance->CR &= ~RNG_CR_IE;
            NVIC_UnregisterHandler(RNG_IRQn);
            return RNG_ERROR;
        }
    }
    return hrng->status;
}