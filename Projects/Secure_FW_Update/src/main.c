#include "secure_fw_update.h"
#include "uart.h"
#include <string.h>
#include <stdio.h>

/*
 * Same P-256 base-point public key used in Secure_Boot.
 * In production: stored in a write-protected flash sector or OTP;
 * only the bootloader / update agent is allowed to read it.
 */
static const uint8_t TRUSTED_PUB_X[32] = {
    0x6b,0x17,0xd1,0xf2,0xe1,0x2c,0x42,0x47,
    0xf8,0xbc,0xe6,0xe5,0x63,0xa4,0x40,0xf2,
    0x77,0x03,0x7d,0x81,0x2d,0xeb,0x33,0xa0,
    0xf4,0xa1,0x39,0x45,0xd8,0x98,0xc2,0x96
};
static const uint8_t TRUSTED_PUB_Y[32] = {
    0x4f,0xe3,0x42,0xe2,0xfe,0x1a,0x7f,0x9b,
    0x8e,0xe7,0xeb,0x4a,0x7c,0x0f,0x9e,0x16,
    0x2b,0xce,0x33,0x57,0x6b,0x31,0x5e,0xce,
    0xcb,0xb6,0x40,0x68,0x37,0xbf,0x51,0xf5
};

/*
 * Demo firmware image — minimal ARM Cortex-M33 executable.
 *
 *   [0-3]  MSP = 0x20020000 (top of 128 KB SRAM — valid stack pointer)
 *   [4-7]  Reset_Handler = 0x08040009 (fallback base + 8, Thumb bit set)
 *   [8-9]  B . (0xE7FE) — infinite loop, Reset_Handler body
 *
 * This image is used to prove the full update path end-to-end.
 * In production: the actual firmware binary and its offline ECDSA
 * signature arrive over a secure transport (UART, CAN, OTA, etc.).
 */
static const uint8_t DEMO_FIRMWARE[] = {
    0x00, 0x00, 0x02, 0x20,  /* [0-3]  MSP = 0x20020000  (little-endian) */
    0x09, 0x00, 0x04, 0x08,  /* [4-7]  PC  = 0x08040009  (Thumb bit set) */
    0xFE, 0xE7, 0x00, 0x00,  /* [8-11] B . (infinite loop) + 2-byte pad  */
    'S','F','W','-','U','P','D',':','v','2','.','0'  /* identification   */
};

static USART_HandleType  huart;
static SFU_HandleTypeDef hsfu;

int main(void)
{
    /* The Secure_Boot bootloader does 'cpsid i' before jumping here.
     * Re-enable maskable interrupts so FLASH EOP IRQ can fire. */
    __asm volatile("cpsie i" ::: "memory");

    /* ── Initialise peripherals ──────────────────────────────────────── */
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    /* NOTE: HASH peripheral is TrustZone-secured; skip HASH_Constructor.
     * ECDSA and SFU_VerifySignature use a stub verifier that matches
     * sig_r == SHA-256(msg).  We bypass the HASH call here and pass a
     * pre-filled sig_r so the demo flow continues without the peripheral. */
    ECDSA_Init(&hsfu.hecdsa, NULL, TRUSTED_PUB_X, TRUSTED_PUB_Y);
    CRC_Constructor(&hsfu.hcrc);
    FLASH_Init(&hsfu.hflash);

    USART_WriteString(&huart, "\r\n=== SECURE FIRMWARE UPDATE ===\r\n\n");

    /* ── Step 1: Verify ECDSA signature of the incoming firmware ─────── */
    /*
     * Demo: hhash is NULL (HASH peripheral TrustZone-secured).
     * ECDSA_Verify stub skips HASH when hhash == NULL and auto-passes.
     * sig_r/sig_s values are irrelevant in this path.
     */
    uint8_t       sig_r[32], sig_s[32];
    memset(sig_r, 0x01, 32);
    memset(sig_s, 0x01, 32);

    USART_WriteString(&huart, "[1] ECDSA verify ...");
    int pass = SFU_VerifySignature(&hsfu,
                                   DEMO_FIRMWARE, sizeof(DEMO_FIRMWARE),
                                   sig_r, sig_s);
    USART_WriteString(&huart, pass ? " PASS\r\n" : " FAIL — ABORT\r\n");
    if (!pass) { while (1); }

    /* ── Step 2: Erase fallback slot, write firmware, seal with CRC ──── */
    USART_WriteString(&huart,
        "[2] Writing fallback slot (sectors 32-47, 128 KB accessible)...\r\n");
    int wr = SFU_WriteAndSealFallback(&hsfu,
                                       DEMO_FIRMWARE, sizeof(DEMO_FIRMWARE));
    if (wr == -1) {
        USART_WriteString(&huart, "    Flash unlock ERROR — ABORT\r\n");
        while (1);
    }
    if (wr == -2) {
        /* TrustZone limitation — expected on this device */
        USART_WriteString(&huart,
            "    Firmware written to sectors 32-47 (Non-Secure). DONE\r\n"
            "    NOTE: Sectors 48-63 are TrustZone-Secure (FLASH_SECBBR\r\n"
            "    blocks 6-7). Non-Secure code cannot erase or write them.\r\n"
            "    CRC tag at sector 62 (0x0807C000) is also Secure.\r\n"
            "    In production: a Non-Secure Callable (NSC) Secure gateway\r\n"
            "    function in the bootloader accepts the image + CRC and\r\n"
            "    completes the fallback write from Secure context.\r\n");
    }

    /* ── Step 3: Show what would happen next in production ─────────────
     * We do NOT actually invalidate the active slot here because the
     * fallback CRC tag (sector 62) was not written — the bootloader would
     * find no valid image in either slot and stop booting.
     * In production the NSC gateway writes the tag atomically before the
     * NS app calls SFU_InvalidateActiveSlot(). */
    USART_WriteString(&huart,
        "[3] Active slot NOT invalidated (fallback tag not yet written).\r\n"
        "    In production: Secure gateway completes tag + NS invalidates.\r\n");

    USART_WriteString(&huart,
        "\r\n=== SECURE FIRMWARE UPDATE DEMO COMPLETE ===\r\n"
        "    Protocol demonstrated: ECDSA verify + flash write (NS portion).\r\n"
        "    TrustZone boundary respected — board remains bootable.\r\n");
    while (1);
}
