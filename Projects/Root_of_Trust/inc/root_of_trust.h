#ifndef __ROT_H
#define __ROT_H

#include <stdint.h>
#include "hash.h"
#include "crc.h"

/*
 * Root of Trust measurements for the STM32C562RE.
 *
 * Three concrete measures that give software visibility into device
 * identity, security posture, and code integrity:
 *
 *   1. Hardware UID96   -- 96-bit factory-programmed unique device identifier
 *   2. RDP level        -- option byte that controls debug access (read-only here)
 *   3. Firmware CRC-32  -- software CRC-32 of the active application image
 *
 */

/* Read the 96-bit hardware unique device ID into uid[12] (little-endian). */
void ROT_ReadUID(uint8_t uid[12]);

/* Return the RDP level byte from FLASH_OPTSR_CUR bits [15:8].
 *   0xAA = Level 0 (no protection, debug open)
 *   0xBB = Level 0.5 (no read-out protection but debug disabled)
 *   0xCC = Level 2 (permanent lock -- irreversible) */
uint8_t ROT_ReadRDPLevel(void);

/* Compute CRC-32 over the active application image and write
 * the 4-byte result into digest[0..3] (digest[4..31] = 0). */
void ROT_MeasureBootloader(HASH_HandleTypeDef *hhash, CRC_HandleTypeDef *hcrc, uint8_t digest[32]);

#endif /* __ROT_H */
