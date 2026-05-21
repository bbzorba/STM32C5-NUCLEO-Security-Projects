#ifndef __SEC_BOOT_H
#define __SEC_BOOT_H

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
/* All headers found via CFLAGS -I paths set in the Makefile */
#include "gpio.h"     /* — RCC_TypeDef / RCC           */
#include "nvic.h"     /*                                  */
#include "uart.h"     /*                                  */
#include "rng.h"      /*                                   */
#include "hash.h"     /* — SHA-256 image hashing         */
#include "ecdsa.h"    /* — ECDSA signature verification */
#include "crc.h"      /* — CRC-32 image integrity        */

#define __IO volatile

/* ── Flash memory map (STM32C562RE, 512 KB, single-bank, 8 KB sectors) ── */
#define BOOT_SECTOR_SIZE        0x00002000U               /* 8 KB per sector                   */
#define BOOT_SELF_ADDR          0x08000000U               /* Bootloader    sectors  0-7  64 KB  */
#define BOOT_ACTIVE_ADDR        0x08010000U               /* Active image  sectors  8-29 176 KB */
#define BOOT_ACTIVE_SIZE        (22U * BOOT_SECTOR_SIZE)  /* 176 KB                             */
#define BOOT_ACTIVE_TAG_ADDR    0x0803C000U               /* CRC tag — active  (sector 30)      */
#define BOOT_KEY_STORE_ADDR     0x0803E000U               /* Public key / flags (sector 31)     */
#define BOOT_FALLBACK_ADDR      0x08040000U               /* Fallback image sectors 32-61 240KB */
#define BOOT_FALLBACK_SIZE      (30U * BOOT_SECTOR_SIZE)  /* 240 KB                             */
#define BOOT_FALLBACK_TAG_ADDR  0x0807C000U               /* CRC tag — fallback (sector 62)     */
#define BOOT_FLAGS_ADDR         0x0807E000U               /* Update state flags (sector 63)     */
#define BOOT_SYSROM_ADDR        0x0BF80000U               /* ST ROM bootloader (informational)  */

/* ── Status codes ─────────────────────────────────────────────────── */
typedef enum {
    SEC_BOOT_OK            = 0,   /* image verified and launched             */
    SEC_BOOT_ERR_SIGNATURE = 1,   /* ECDSA signature mismatch                */
    SEC_BOOT_ERR_CRC       = 2,   /* CRC mismatch — image corrupted/tampered */
    SEC_BOOT_ERR_NO_IMAGE  = 3,   /* no valid image found in any slot        */
} SEC_BOOT_Status;

/* ── Handle ───────────────────────────────────────────────────────── */
typedef struct {
    HASH_HandleTypeDef  hhash;    /* owns the HASH peripheral               */
    ECDSA_HandleTypeDef hecdsa;   /* signature-verify context               */
    CRC_HandleTypeDef   hcrc;     /* CRC-32 peripheral                      */
} SEC_BOOT_HandleTypeDef;

/* ── API ─────────────────────────────────────────────────────────── */

/* Initialise peripherals and store the trusted public key */
void SEC_BOOT_Init(SEC_BOOT_HandleTypeDef *hsb,
                   const uint8_t *pub_x,
                   const uint8_t *pub_y);

/* Verify a firmware image via SHA-256 + ECDSA */
SEC_BOOT_Status SEC_BOOT_VerifyImage(SEC_BOOT_HandleTypeDef *hsb,
                                     const uint8_t *image,
                                     size_t         len,
                                     const uint8_t *sig_r,
                                     const uint8_t *sig_s);

/*
 * Run the full boot sequence:
 *   1. CRC-check active image (sectors 8-29)  → jump if valid app MSP found
 *   2. CRC-check fallback image (sectors 32-61) → jump if valid
 *   3. Return SEC_BOOT_ERR_NO_IMAGE if both fail (caller should halt)
 */
SEC_BOOT_Status SEC_BOOT_Boot(SEC_BOOT_HandleTypeDef *hsb,
                               USART_HandleType       *huart);

#endif /* __SEC_BOOT_H */