#ifndef __SEC_BOOT_H
#define __SEC_BOOT_H

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "../../GPIO/inc/gpio.h"   /* RCC_TypeDef / RCC */
#include "../../NVIC/inc/nvic.h"
#include "../../UART/inc/uart.h"
#include "../../RNG/inc/rng.h"
#include "../../HASH/inc/hash.h"   /* SHA-256 image hashing              */
#include "../../ECDSA/inc/ecdsa.h" /* ECDSA signature verification       */

#define __IO volatile

/* ── Status codes ─────────────────────────────────────────────────── */
typedef enum {
    SEC_BOOT_OK            = 0,   /* image hash + signature verified    */
    SEC_BOOT_ERR_SIGNATURE = 1,   /* signature did not match image hash */
} SEC_BOOT_Status;

/* ── Handle ("object") ───────────────────────────────────────────── */
typedef struct {
    HASH_HandleTypeDef  hhash;   /* owns the HASH peripheral           */
    ECDSA_HandleTypeDef hecdsa;  /* signature-verify context           */
} SEC_BOOT_HandleTypeDef;

/* ── API ─────────────────────────────────────────────────────────── */

/*
 * Initialise the secure-boot context.
 * Resets the HASH peripheral and stores the trusted public key.
 */
void SEC_BOOT_Init(SEC_BOOT_HandleTypeDef *hsb,
                   const uint8_t *pub_x,
                   const uint8_t *pub_y);

/*
 * Hash the firmware image and verify its ECDSA signature.
 * Internally computes SHA-256(image) then calls ECDSA_Verify.
 * Returns SEC_BOOT_OK on success, SEC_BOOT_ERR_SIGNATURE otherwise.
 */
SEC_BOOT_Status SEC_BOOT_VerifyImage(SEC_BOOT_HandleTypeDef *hsb,
                                     const uint8_t *image,
                                     size_t         len,
                                     const uint8_t *sig_r,
                                     const uint8_t *sig_s);

#endif /* __SEC_BOOT_H */