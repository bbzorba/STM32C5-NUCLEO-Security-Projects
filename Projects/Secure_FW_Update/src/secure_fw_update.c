#include "secure_fw_update.h"
#include <string.h>

/* ARM SCB Application Interrupt and Reset Control Register. */
#define SCB_AIRCR           (*(volatile uint32_t *)0xE000ED0CU)
#define AIRCR_VECTKEY_RESET  0x05FA0004U   /* VECTKEY=0x05FA, SYSRESETREQ=1 */

/* Software CRC-32 (IEEE 802.3, poly=0x04C11DB7, reflected).
 * Used instead of hardware CRC peripheral to avoid reading back Secure-attributed
 * flash regions (0x08060000+) which fault from Non-Secure context on this device. */
static uint32_t sw_crc32_update(uint32_t crc, uint8_t byte)
{
    crc ^= byte;
    for (int i = 0; i < 8; i++)
        crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1));
    return crc;
}

/* Compute CRC-32 over 'img_len' bytes of image data followed by
 * (total_len - img_len) bytes of 0xFF padding, matching the layout
 * written into the flash slot by seal.py and this driver. */
static uint32_t sw_crc32_padded(const uint8_t *img, size_t img_len, size_t total_len)
{
    uint32_t crc = 0xFFFFFFFFU;
    size_t i;
    for (i = 0; i < img_len; i++)
        crc = sw_crc32_update(crc, img[i]);
    for (; i < total_len; i++)
        crc = sw_crc32_update(crc, 0xFFU);
    return crc ^ 0xFFFFFFFFU;
}

/* ── 1. Signature verification ──────────────────────────────────────── */
int SFU_VerifySignature(SFU_HandleTypeDef *h,
                        const uint8_t *msg, size_t len,
                        const uint8_t *sig_r, const uint8_t *sig_s)
{
    return ECDSA_Verify(&h->hecdsa, msg, len, sig_r, sig_s);
}

/* ── 2. Write image and seal the fallback slot ───────────────────────── */
int SFU_WriteAndSealFallback(SFU_HandleTypeDef *h,
                              const uint8_t *img, size_t len)
{
    uint32_t addr, end_addr, i;
    uint32_t crc_tag;

    /* TrustZone restriction on STM32C562RE:
     * FLASH_SECBBR blocks 6-7 (sectors 48-63, 0x08060000-0x0807FFFF) are
     * Secure-attributed.  Attempting to erase or write those sectors from
     * Non-Secure context triggers a Secure fault and resets the MCU.
     * The accessible NS portion of the fallback slot is sectors 32-47 (128 KB).
     * Return SFU_ERR_TZ_LIMIT (-2) to let the caller explain the limitation. */
#define SFU_NS_FALLBACK_END  0x08060000U   /* start of Secure block 6 */

    if (FLASH_Unlock(&h->hflash) != FLASH_OK) return -1;

    /* Erase only the Non-Secure portion of the fallback slot (sectors 32-47) */
    end_addr = SFU_NS_FALLBACK_END;
    for (addr = SFU_FALLBACK_ADDR; addr < end_addr; addr += SFU_SECTOR_SIZE)
        FLASH_ErasePage(&h->hflash, addr);

    /* Write image word by word into the non-Secure region */
    for (i = 0; i + 4 <= len && (SFU_FALLBACK_ADDR + i) < SFU_NS_FALLBACK_END; i += 4) {
        uint32_t word;
        memcpy(&word, img + i, 4);
        FLASH_ProgramWord(&h->hflash, SFU_FALLBACK_ADDR + i, word);
    }

    /* Compute the full-slot CRC-32 in software for reporting purposes.
     * We cannot write the tag to sector 62 (0x0807C000) — it is Secure. */
    crc_tag = sw_crc32_padded(img, len, SFU_FALLBACK_SIZE);
    (void)crc_tag;  /* would be written to SFU_FALLBACK_TAG in production */

    FLASH_Lock(&h->hflash);
    return -2;   /* SFU_ERR_TZ_LIMIT: partial write, Secure sectors 48-63 skipped */
}

/* ── 3. Invalidate active slot so bootloader falls to fallback ───────── */
void SFU_InvalidateActiveSlot(SFU_HandleTypeDef *h)
{
    FLASH_Unlock(&h->hflash);
    FLASH_ErasePage(&h->hflash, SFU_ACTIVE_TAG);   /* erase sector 30 */
    FLASH_Lock(&h->hflash);
}

/* ── 4. Soft reset ───────────────────────────────────────────────────── */
void SFU_SoftReset(void)
{
    __asm volatile ("dsb 0xF" ::: "memory");  /* data synchronisation barrier */
    SCB_AIRCR = AIRCR_VECTKEY_RESET;
    while (1);  /* wait for hardware reset */
}
