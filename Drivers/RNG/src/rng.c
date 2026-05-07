#include "../inc/rng.h"

void RNG_Constructor(RNG_HandleTypeDef *hrng)
{
    /* Select CK48 clock source for RNG: psi_div_3_ck = MSIS/3 = 16 MHz.
     * RNG entropy clock must be > fHCLK/32 (= 1.5 MHz at 48 MHz system clock).
     * Without this, CK48 defaults to 0 Hz and CECS=1 blocks any output. */
    RCC->CCIPR2 = (RCC->CCIPR2 & ~RCC_CCIPR2_CK48SEL_MASK) | RCC_CCIPR2_CK48SEL_HSI_DIV3;

    /* Enable RNG clock, then reset the peripheral to a clean state */
    RCC->AHB2ENR  |=  RCC_AHB2ENR_RNGEN;
    RCC->AHB2RSTR |=  RCC_AHB2RSTR_RNGEN;
    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_RNGEN;

    hrng->Instance = TRNG;
    hrng->status   = RNG_OK;
    hrng->it_buf   = 0;
    hrng->it_len   = 0;
    hrng->it_idx   = 0;
}

void RNG_Enable(RNG_HandleTypeDef *hrng)
{
    hrng->Instance->CR |= RNG_CR_RNGEN;
}

void RNG_Disable(RNG_HandleTypeDef *hrng)
{
    hrng->Instance->CR &= ~RNG_CR_RNGEN;
}

RNG_StatusTypeDef RNG_Generate(RNG_HandleTypeDef *hrng, uint8_t *buffer, size_t length)
{
    if (!hrng || !buffer || length == 0) { return RNG_ERROR; }

    for (size_t i = 0; i < length; i++) {
        /* Wait for data ready */
        while ((hrng->Instance->SR & RNG_SR_DRDY) == 0) {
            /* Check for errors while waiting */
            if ((hrng->Instance->SR & (RNG_SR_CEIS | RNG_SR_SEIS)) != 0) {
                hrng->Instance->SR &= ~(RNG_SR_CEIS | RNG_SR_SEIS);
                hrng->Instance->CR &= ~RNG_CR_RNGEN;
                return RNG_ERROR;
            }
        }
        buffer[i] = (uint8_t)(hrng->Instance->DR & 0xFFU);
    }

    return RNG_OK;
}

/* -----------------------------------------------------------------------
 * Interrupt-based generation
 * ----------------------------------------------------------------------- */

static void rng_irq_handler(void *arg)
{
    RNG_HandleTypeDef *hrng = (RNG_HandleTypeDef *)arg;

    /* Error: clear flags, disable interrupt, abort */
    if ((hrng->Instance->SR & (RNG_SR_CEIS | RNG_SR_SEIS)) != 0) {
        hrng->Instance->SR &= ~(RNG_SR_CEIS | RNG_SR_SEIS);
        hrng->Instance->CR &= ~RNG_CR_IE;
        NVIC_UnregisterHandler(RNG_IRQn);
        hrng->status = RNG_ERROR;
        return;
    }

    /* Data ready: collect one byte */
    if ((hrng->Instance->SR & RNG_SR_DRDY) != 0) {
        hrng->it_buf[hrng->it_idx++] = (uint8_t)(hrng->Instance->DR & 0xFFU);
        if (hrng->it_idx >= hrng->it_len) {
            hrng->Instance->CR &= ~RNG_CR_IE;
            NVIC_UnregisterHandler(RNG_IRQn);
            hrng->status = RNG_OK;
        }
    }
}

RNG_StatusTypeDef RNG_Generate_IT(RNG_HandleTypeDef *hrng, uint8_t *buffer, size_t length)
{
    if (!hrng || !buffer || length == 0) { return RNG_ERROR; }

    hrng->it_buf = buffer;
    hrng->it_len = length;
    hrng->it_idx = 0;
    hrng->status = RNG_BUSY;

    /* Clear any stale error flags before enabling interrupt */
    hrng->Instance->SR &= ~(RNG_SR_CEIS | RNG_SR_SEIS);

    /* Register IRQ handler and enable RNG interrupt */
    NVIC_RegisterHandler(RNG_IRQn, rng_irq_handler, hrng, 0);
    hrng->Instance->CR |= RNG_CR_RNGEN | RNG_CR_IE;

    /* Wait for the interrupt handler to finish (busy-wait on status) */
    uint32_t timeout = 2000000U;
    while (hrng->status == RNG_BUSY && --timeout) {}

    if (timeout == 0) {
        hrng->Instance->CR &= ~RNG_CR_IE;
        NVIC_UnregisterHandler(RNG_IRQn);
        return RNG_ERROR;
    }

    return hrng->status;
}
