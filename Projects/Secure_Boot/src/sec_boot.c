#include "../inc/sec_boot.h"

/* ── Internal helpers ────────────────────────────────────────────── */

/*
 * Branch to an application image located at app_base.
 * Reads the initial MSP and Reset_Handler address from the vector table,
 * relocates VTOR, then hands over control. Never returns on success.
 */
static void SEC_BOOT_Jump(uint32_t app_base)
{
    uint32_t app_msp = *(volatile uint32_t *)(app_base + 0U);
    uint32_t app_pc  = *(volatile uint32_t *)(app_base + 4U);

    /* Mask all interrupts before handing over */
    __asm volatile ("cpsid i" ::: "memory");

    /* Relocate VTOR so the application's own handlers take over */
    *(volatile uint32_t *)0xE000ED08U = app_base;   /* SCB->VTOR */

    /* Set the stack pointer and jump to the application reset handler */
    __asm volatile (
        "msr msp, %0\n\t"
        "bx  %1\n\t"
        : : "r"(app_msp), "r"(app_pc)
        : "memory"
    );
}

/* ── Public API ──────────────────────────────────────────────────── */

void SEC_BOOT_Init(SEC_BOOT_HandleTypeDef *hsb,
                   const uint8_t *pub_x,
                   const uint8_t *pub_y)
{
    HASH_Constructor(&hsb->hhash);
    ECDSA_Init(&hsb->hecdsa, &hsb->hhash, pub_x, pub_y);
}

SEC_BOOT_Status SEC_BOOT_VerifyImage(SEC_BOOT_HandleTypeDef *hsb,
                                     const uint8_t *image,
                                     size_t         len,
                                     const uint8_t *sig_r,
                                     const uint8_t *sig_s)
{
    if (!ECDSA_Verify(&hsb->hecdsa, image, len, sig_r, sig_s))
        return SEC_BOOT_ERR_SIGNATURE;
    return SEC_BOOT_OK;
}

/*
 * SEC_BOOT_Boot — NIST SP 800-193 aligned secure boot sequence.
 *
 * Boot authentication chain:
 *   1. Sanity check: validate MSP at BOOT_ACTIVE_ADDR points into SRAM.
 *      An erased flash slot (0xFFFFFFFF) is rejected here before any
 *      expensive crypto operation is performed.
 *   2. Integrity + Authentication: hardware SHA-256 over the image bytes,
 *      then ECDSA_Verify compares the digest against the pre-embedded sig_r.
 *      sig_r is produced OFFLINE by the firmware signer; an attacker cannot
 *      forge a matching sig_r without the corresponding private key.
 *   3. Both checks pass → jump to application (never returns).
 *
 * Returns SEC_BOOT_ERR_NO_IMAGE when no bootable image is found so the
 * caller can fall through to a diagnostic or halt.
 */
SEC_BOOT_Status SEC_BOOT_Boot(SEC_BOOT_HandleTypeDef *hsb,
                               const uint8_t    *sig_r,
                               const uint8_t    *sig_s,
                               size_t            image_len,
                               USART_HandleType *huart)
{
    char     buf[80];
    uint32_t msp;

    USART_WriteString(huart, "[BOOT] Active slot \u2014 0x08010000\r\n");

    /* Step 1: validate MSP (rules out erased flash / no application) */
    msp = *(volatile uint32_t *)BOOT_ACTIVE_ADDR;
    snprintf(buf, sizeof(buf), "  MSP      : 0x%08lX\r\n", (unsigned long)msp);
    USART_WriteString(huart, buf);

    if (msp < 0x20000000U || msp > 0x20020000U) {
        USART_WriteString(huart,
            "  MSP      : INVALID \u2014 no application in slot\r\n"
            "[BOOT] No trusted image found\r\n");
        return SEC_BOOT_ERR_NO_IMAGE;
    }

    /* Step 2: SHA-256 + ECDSA signature verification */
    USART_WriteString(huart, "  Verifying SHA-256 + ECDSA signature ...\r\n");

    SEC_BOOT_Status st = SEC_BOOT_VerifyImage(
        hsb,
        (const uint8_t *)BOOT_ACTIVE_ADDR,
        image_len,
        sig_r,
        sig_s);

    if (st != SEC_BOOT_OK) {
        USART_WriteString(huart,
            "  Signature : INVALID \u2014 boot refused\r\n"
            "[BOOT] No trusted image found\r\n");
        return SEC_BOOT_ERR_NO_IMAGE;
    }

    /* Step 3: signature is valid — jump to application */
    USART_WriteString(huart,
        "  Signature : OK\r\n"
        "[BOOT] Jumping to application ...\r\n");
    SEC_BOOT_Jump(BOOT_ACTIVE_ADDR);

    /* Unreachable — SEC_BOOT_Jump does not return */
    return SEC_BOOT_ERR_NO_IMAGE;
}