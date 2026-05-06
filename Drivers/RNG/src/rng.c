#include "../inc/rng.h"

void RNG_Constructor(RNG_HandleTypeDef *hrng)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
    hrng->Instance = RNG_PERIPH;
    hrng->state    = RNG_OK;
    hrng->msg_len  = 0;
}
