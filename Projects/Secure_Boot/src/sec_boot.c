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
    CRC_Constructor(&hsb->hcrc);
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
 * SEC_BOOT_Boot — the main boot-decision loop.
 *
 * For each slot (active, then fallback):
 *   1. Read the stored CRC-32 tag from the tag sector.
 *   2. Compute CRC-32 over the image region.
 *   3. If they match, validate the MSP value at the image base.
 *   4. If MSP is a valid SRAM address → jump (never returns).
 *
 * Returns SEC_BOOT_ERR_NO_IMAGE when no slot can boot (caller halts).
 */
SEC_BOOT_Status SEC_BOOT_Boot(SEC_BOOT_HandleTypeDef *hsb,
                               USART_HandleType       *huart)
{
    char     buf[80];
    uint8_t  crc_out[4];
    uint32_t computed, stored, msp;

    /* ── Slot 1: Active image (sectors 8-29, 0x08010000, 176 KB) ─── */
    USART_WriteString(huart,
        "[SLOT 1] Active  0x08010000  sectors 8-29  176 KB\r\n");

    stored = *(volatile uint32_t *)BOOT_ACTIVE_TAG_ADDR;
    snprintf(buf, sizeof(buf), "  CRC tag @ 0x%08lX : %08lX\r\n",
             (unsigned long)BOOT_ACTIVE_TAG_ADDR, (unsigned long)stored);
    USART_WriteString(huart, buf);

    if (stored == 0xFFFFFFFFU) {
        USART_WriteString(huart, "  Tag : BLANK (slot not sealed)\r\n");
    } else {
        CRC_Calculate(&hsb->hcrc,
                      (const uint8_t *)BOOT_ACTIVE_ADDR, BOOT_ACTIVE_SIZE,
                      crc_out);
        computed = ((uint32_t)crc_out[0] << 24) | ((uint32_t)crc_out[1] << 16) |
                   ((uint32_t)crc_out[2] <<  8) |  (uint32_t)crc_out[3];
        snprintf(buf, sizeof(buf), "  Computed CRC      : %08lX\r\n",
                 (unsigned long)computed);
        USART_WriteString(huart, buf);

        if (computed == stored) {
            USART_WriteString(huart, "  CRC  : OK\r\n");
            msp = *(volatile uint32_t *)BOOT_ACTIVE_ADDR;
            snprintf(buf, sizeof(buf), "  App MSP           : 0x%08lX\r\n",
                     (unsigned long)msp);
            USART_WriteString(huart, buf);
            if ((msp >= 0x20000000U) && (msp <= 0x20020000U)) {
                USART_WriteString(huart, "  MSP  : valid — jumping to application\r\n");
                SEC_BOOT_Jump(BOOT_ACTIVE_ADDR);
                /* Reaches here only if jump somehow fails */
            }
            USART_WriteString(huart, "  MSP  : INVALID — no application flashed\r\n");
        } else {
            USART_WriteString(huart, "  CRC  : MISMATCH\r\n");
        }
    }

    /* ── Slot 2: Fallback image (sectors 32-61, 0x08040000, 240 KB) ─ */
    USART_WriteString(huart,
        "[SLOT 2] Fallback 0x08040000  sectors 32-61  240 KB\r\n");

    stored = *(volatile uint32_t *)BOOT_FALLBACK_TAG_ADDR;
    snprintf(buf, sizeof(buf), "  CRC tag @ 0x%08lX : %08lX\r\n",
             (unsigned long)BOOT_FALLBACK_TAG_ADDR, (unsigned long)stored);
    USART_WriteString(huart, buf);

    if (stored == 0xFFFFFFFFU) {
        USART_WriteString(huart, "  Tag : BLANK (no fallback)\r\n");
    } else {
        CRC_Calculate(&hsb->hcrc,
                      (const uint8_t *)BOOT_FALLBACK_ADDR, BOOT_FALLBACK_SIZE,
                      crc_out);
        computed = ((uint32_t)crc_out[0] << 24) | ((uint32_t)crc_out[1] << 16) |
                   ((uint32_t)crc_out[2] <<  8) |  (uint32_t)crc_out[3];
        snprintf(buf, sizeof(buf), "  Computed CRC      : %08lX\r\n",
                 (unsigned long)computed);
        USART_WriteString(huart, buf);

        if (computed == stored) {
            USART_WriteString(huart, "  CRC  : OK\r\n");
            msp = *(volatile uint32_t *)BOOT_FALLBACK_ADDR;
            snprintf(buf, sizeof(buf), "  Fallback MSP      : 0x%08lX\r\n",
                     (unsigned long)msp);
            USART_WriteString(huart, buf);
            if ((msp >= 0x20000000U) && (msp <= 0x20020000U)) {
                USART_WriteString(huart, "  MSP  : valid — jumping to fallback\r\n");
                SEC_BOOT_Jump(BOOT_FALLBACK_ADDR);
            }
            USART_WriteString(huart, "  MSP  : INVALID — no fallback application flashed\r\n");
        } else {
            USART_WriteString(huart, "  CRC  : MISMATCH\r\n");
        }
    }

    USART_WriteString(huart, "\r\n[BOOT FAILED] No trusted image in any slot\r\n");
    return SEC_BOOT_ERR_NO_IMAGE;
}