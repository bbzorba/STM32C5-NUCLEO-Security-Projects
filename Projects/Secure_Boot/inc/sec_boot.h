#ifndef __SEC_BOOT_H
#define __SEC_BOOT_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "uart.h"     /* UART output                            */
#include "hash.h"     /* hardware SHA-256 (HASH peripheral)     */
#include "ecdsa.h"    /* ECDSA signature verification (stub)    */

#define __IO volatile

/* ── Flash memory map (STM32C562RE, 512 KB, single-bank, 8 KB sectors) ── */
#define BOOT_SECTOR_SIZE        0x00002000U               /* 8 KB per sector                   */
#define BOOT_SELF_ADDR          0x08000000U               /* Bootloader    sectors  0-7  64 KB  */
#define BOOT_ACTIVE_ADDR        0x08010000U               /* Active image  sectors  8-29 176 KB */
#define BOOT_ACTIVE_SIZE        (22U * BOOT_SECTOR_SIZE)  /* 176 KB                             */
#define BOOT_FALLBACK_ADDR      0x08040000U               /* Fallback image sectors 32-61 240KB */
#define BOOT_FALLBACK_SIZE      (30U * BOOT_SECTOR_SIZE)  /* 240 KB                             */

/* ── Status codes ─────────────────────────────────────────────────── */
typedef enum {
    SEC_BOOT_OK            = 0,   /* image verified and launched            */
    SEC_BOOT_ERR_SIGNATURE = 1,   /* ECDSA/hash signature mismatch          */
    SEC_BOOT_ERR_NO_IMAGE  = 2,   /* no valid image found in slot           */
} SEC_BOOT_Status;

/* ── Handle ───────────────────────────────────────────────────────── */
typedef struct {
    HASH_HandleTypeDef  hhash;    /* owns the hardware HASH peripheral      */
    ECDSA_HandleTypeDef hecdsa;   /* signature-verify context               */
} SEC_BOOT_HandleTypeDef;

/* ── API ─────────────────────────────────────────────────────────── */

/* Initialise hardware and bind the trusted public key */
void SEC_BOOT_Init(SEC_BOOT_HandleTypeDef *hsb,
                   const uint8_t *pub_x,
                   const uint8_t *pub_y);

/*
 * Verify a firmware image (NIST SP 800-193 style):
 *   1. Hardware SHA-256(image, len)           — measures the firmware
 *   2. ECDSA_Verify(sig_r, sig_s, hash, key) — authenticates the measurement
 * Returns SEC_BOOT_OK only when the signature is valid.
 */
SEC_BOOT_Status SEC_BOOT_VerifyImage(SEC_BOOT_HandleTypeDef *hsb,
                                     const uint8_t *image,
                                     size_t         len,
                                     const uint8_t *sig_r,
                                     const uint8_t *sig_s);

/*
 * Full secure-boot sequence:
 *   1. Validate MSP at BOOT_ACTIVE_ADDR (erased flash → rejected)
 *   2. Hardware SHA-256(image, image_len) + ECDSA_Verify against (sig_r, sig_s)
 *   3. Both pass → jump to application (never returns)
 *   4. Otherwise → return SEC_BOOT_ERR_NO_IMAGE so the caller can halt
 *
 * sig_r and sig_s are computed OFFLINE by the firmware signer and embedded
 * as constants in the bootloader — they are NEVER computed on the MCU.
 * image_len is the exact byte count of the application binary.
 */
SEC_BOOT_Status SEC_BOOT_Boot(SEC_BOOT_HandleTypeDef *hsb,
                               const uint8_t    *sig_r,
                               const uint8_t    *sig_s,
                               size_t            image_len,
                               USART_HandleType *huart);

#endif /* __SEC_BOOT_H */