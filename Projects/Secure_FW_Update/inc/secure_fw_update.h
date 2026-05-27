#ifndef __SFU_H
#define __SFU_H

#include <stdint.h>
#include <stddef.h>
#include "hash.h"
#include "ecdsa.h"
#include "crc.h"
#include "flash.h"   /* found via -IProjects/FLASH/inc in Makefile */

/*
 * Memory map — must match Secure_Boot's constants exactly.
 * Sectors 32-61 (240 KB) are the fallback slot.
 * Sector 62 holds the CRC tag for the fallback image.
 * Sector 30 holds the CRC tag for the active image.
 */
#define SFU_FALLBACK_ADDR   0x08040000U          /* Fallback slot base  */
#define SFU_FALLBACK_SIZE   (240U * 1024U)        /* Fallback slot size  */
#define SFU_FALLBACK_TAG    0x0807C000U           /* Fallback CRC tag    */
#define SFU_ACTIVE_TAG      0x0803C000U           /* Active  CRC tag     */
#define SFU_SECTOR_SIZE     0x00002000U           /* 8 KB per sector     */

typedef struct {
    HASH_HandleTypeDef  hhash;
    ECDSA_HandleTypeDef hecdsa;
    CRC_HandleTypeDef   hcrc;
    FLASH_HandleTypeDef hflash;
} SFU_HandleTypeDef;

/*
 * Verify ECDSA-P256 signature of the firmware image.
 * Stub verifier: passes when sig_r == SHA-256(msg).
 * Returns 1 on PASS, 0 on FAIL.
 */
int SFU_VerifySignature(SFU_HandleTypeDef *h,
                        const uint8_t *msg, size_t len,
                        const uint8_t *sig_r, const uint8_t *sig_s);

/*
 * Erase the fallback flash slot (sectors 32-61), write the image,
 * compute CRC over the programmed flash region, and write the CRC tag
 * to sector 62.  Returns 0 on success, -1 on flash error.
 */
int SFU_WriteAndSealFallback(SFU_HandleTypeDef *h,
                              const uint8_t *img, size_t len);

/*
 * Erase the active slot's CRC tag (sector 30).
 * On next reset the bootloader will find no valid active image and
 * fall through to the freshly-updated fallback slot.
 */
void SFU_InvalidateActiveSlot(SFU_HandleTypeDef *h);

/* Trigger an immediate soft reset via the ARM SCB AIRCR register. */
void SFU_SoftReset(void);

#endif /* __SFU_H */
