#include "root_of_trust.h"
#include <string.h>

#define UID_BASE  ((volatile uint32_t *)0x08FFF800U)

#define FLASH_OPTSR_CUR  (*(volatile uint32_t *)0x40022050U) //RDP level in bits [15:8] (AA=L0, BB=L0.5, CC=L2, else L1).

/* ── 1. Device identity ──────────────────────────────────────────────── */
void ROT_ReadUID(uint8_t uid[12])
{
    for (int i = 0; i < 3; i++) {
        uint32_t w = UID_BASE[i];
        uid[i*4 + 0] = (uint8_t)(w);
        uid[i*4 + 1] = (uint8_t)(w >>  8);
        uid[i*4 + 2] = (uint8_t)(w >> 16);
        uid[i*4 + 3] = (uint8_t)(w >> 24);
    }
}

/* ── 2. Security posture ─────────────────────────────────────────────── */
uint8_t ROT_ReadRDPLevel(void)
{
    /* RDP level is in bits [15:8] of FLASH_OPTSR_CUR (STM32C562RE RM Table 28) */
    return (uint8_t)((FLASH_OPTSR_CUR >> 8U) & 0xFFU);
}

/* ── SW CRC-32 (IEEE 802.3 / zlib — poly=0x04C11DB7 reflected,
 *    init=0xFFFFFFFF, RefIn=true, RefOut=true, XorOut=0xFFFFFFFF).
 *    Matches Python zlib.crc32() and tools/seal.py exactly. ─────────── */
static uint32_t rot_sw_crc32(const void *buf, size_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    const uint8_t *p = (const uint8_t *)buf;
    for (size_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1U));
    }
    return crc ^ 0xFFFFFFFFU;
}

/* ── 3. Firmware integrity ───────────────────────────────────────────── */
void ROT_MeasureBootloader(HASH_HandleTypeDef *hhash, CRC_HandleTypeDef *hcrc, uint8_t digest[32])
{
    /* NOTE: The STM32C562RE HASH peripheral is not accessible from the
     * application slot (non-secure context) due to TrustZone GTZC configuration.
     * We use a pure software CRC-32 over the active image (176 KB padded), which
     * is independent of any peripheral and matches tools/seal.py exactly. */
    (void)hhash;
    (void)hcrc;
    uint32_t crc = rot_sw_crc32((const void *)0x08010000U, 176U * 1024U);
    digest[0] = (uint8_t)(crc >> 24);
    digest[1] = (uint8_t)(crc >> 16);
    digest[2] = (uint8_t)(crc >>  8);
    digest[3] = (uint8_t)(crc);
}
