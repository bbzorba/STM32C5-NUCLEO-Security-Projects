#include "../inc/flash.h"

/* ==== WRITE PROTECT ====
 * Programs WRP1R/WRP2R_PRG so the specified sectors are write-protected.
 * Each bit in sector_mask that is SET will be CLEARED in the PRG register
 * (0 = sector protected, 1 = sector writable — hardware polarity).
 * Changes are staged in the PRG register. Activate by calling OPTSTRT
 * and performing a system reset (not done here to keep the demo safe).
 */
FLASH_StatusTypeDef FLASH_WriteProtect(FLASH_HandleTypeDef *hflash,
                                         uint8_t bank, uint32_t sector_mask)
{
    /* Unlock option bytes if locked */
    if (hflash->Instance->OPTCR & FLASH_OPTCR_OPTLOCK) {
        hflash->Instance->OPTKEYR = FLASH_OPTKEY1;
        hflash->Instance->OPTKEYR = FLASH_OPTKEY2;
        if (hflash->Instance->OPTCR & FLASH_OPTCR_OPTLOCK)
            return FLASH_ERROR;
    }

    /* Clear the bits for the requested sectors to enable write protection */
    if (bank == 1U)
        FLASH_WRP1R_PRG_REG &= ~sector_mask;
    else
        FLASH_WRP2R_PRG_REG &= ~sector_mask;

    /* Re-lock option bytes without committing (safe for demo) */
    hflash->Instance->OPTCR |= FLASH_OPTCR_OPTLOCK;
    return FLASH_OK;
}

/* ==== READ PROTECT ====
 * Returns the active RDP level byte from OPTSR_CUR bits[15:8].
 * 0xAA = Level 0 (no protection), 0xBB = Level 1, 0xCC = Level 2 (irreversible).
 */
uint8_t FLASH_ReadProtect(FLASH_HandleTypeDef *hflash)
{
    (void)hflash;
    return (uint8_t)((FLASH_OPTSR_CUR_REG >> 8U) & 0xFFU);
}

FLASH_StatusTypeDef FLASH_IsWriteProtected(FLASH_HandleTypeDef *hflash, uint32_t address) {
    uint32_t sector_mask;
    if (address >= FLASH_BANK2_BASE) {
        sector_mask = 1U << ((address - FLASH_BANK2_BASE) / FLASH_SECTOR_SIZE);
        return (FLASH_WRP2R_CUR_REG & sector_mask) == 0U ? FLASH_OK : FLASH_ERROR; // 0 = protected
    } else {
        sector_mask = 1U << ((address - FLASH_BANK1_BASE) / FLASH_SECTOR_SIZE);
        return (FLASH_WRP1R_CUR_REG & sector_mask) == 0U ? FLASH_OK : FLASH_ERROR; // 0 = protected
    }
}

/* ==== FLASH CHECK DATA INTEGRITY ==== */
FLASH_StatusTypeDef FLASH_CheckIntegrity(CRC_HandleTypeDef *hcrc, uint32_t address, size_t length, const uint8_t expected_crc[4]) {
    uint8_t crc_out[4];
    CRC_Calculate(hcrc, (const uint8_t *)address, length, crc_out);
    return (memcmp(crc_out, expected_crc, 4) == 0) ? FLASH_OK : FLASH_ERROR;
}

/* ==== STORE INTEGRITY TAG ====
 * Computes CRC-32 of [region_addr .. region_addr+region_len) bytes,
 * erases the sector at tag_addr, and programs the 4-byte big-endian CRC
 * as a single 32-bit word there.
 * tag_addr MUST lie OUTSIDE the measured region.
 */
FLASH_StatusTypeDef FLASH_StoreIntegrityTag(FLASH_HandleTypeDef *hflash,
                                             CRC_HandleTypeDef  *hcrc,
                                             uint32_t region_addr,
                                             size_t   region_len,
                                             uint32_t tag_addr)
{
    uint8_t crc_out[4];

    /* Compute CRC of the measured flash region */
    if (CRC_Calculate(hcrc, (const uint8_t *)region_addr, region_len, crc_out) != CRC_OK)
        return FLASH_ERROR;

    /* Pack big-endian bytes into a single 32-bit word for FLASH_ProgramWord */
    uint32_t crc_word = ((uint32_t)crc_out[0] << 24) |
                        ((uint32_t)crc_out[1] << 16) |
                        ((uint32_t)crc_out[2] <<  8) |
                         (uint32_t)crc_out[3];

    if (FLASH_Unlock(hflash) != FLASH_OK)
        return FLASH_ERROR;

    if (FLASH_ErasePage(hflash, tag_addr) != FLASH_OK) {
        FLASH_Lock(hflash);
        return FLASH_ERROR;
    }

    if (FLASH_ProgramWord(hflash, tag_addr, crc_word) != FLASH_OK) {
        FLASH_Lock(hflash);
        return FLASH_ERROR;
    }

    FLASH_Lock(hflash);
    return FLASH_OK;
}