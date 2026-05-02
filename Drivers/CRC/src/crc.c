#include "../inc/crc.h"

void CRC_Constructor(CRC_HandleTypeDef *hcrc)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    hcrc->Instance = CRC_PERIPH;
    hcrc->state    = CRC_OK;
}

/*
 * Computes standard CRC-32 (poly=0x04C11DB7, init=0xFFFFFFFF, RefIn=true,
 * RefOut=true, XorOut=0xFFFFFFFF). Expected: CRC32("123456789") = 0xCBF43926.
 * Result is written big-endian into crc_out[0..3].
 */
CRC_StatusTypeDef CRC_Calculate(CRC_HandleTypeDef *hcrc, const uint8_t *data, size_t len, uint8_t *crc_out)
{
    if (!hcrc || !data || !crc_out)
        return CRC_ERROR;

    /* Reset and configure: 32-bit poly, byte-wise input reversal, output reversal */
    hcrc->Instance->CR = CRC_CR_RESET | CRC_CR_POLYSIZE_32 | CRC_CR_REV_IN_BYTE | CRC_CR_REV_OUT;

    /* Feed data one byte at a time via 8-bit writes to DR */
    volatile uint8_t *dr8 = (volatile uint8_t *)&hcrc->Instance->DR;
    for (size_t i = 0; i < len; i++)
        *dr8 = data[i];

    /* Read result and apply final XOR (standard CRC-32 uses 0xFFFFFFFF) */
    uint32_t result = hcrc->Instance->DR ^ 0xFFFFFFFFU;

    crc_out[0] = (uint8_t)(result >> 24);
    crc_out[1] = (uint8_t)(result >> 16);
    crc_out[2] = (uint8_t)(result >>  8);
    crc_out[3] = (uint8_t)(result);

    return CRC_OK;
}
