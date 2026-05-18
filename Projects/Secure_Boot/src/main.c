#include "../inc/sec_boot.h"

/*
 * Trusted P-256 public key (base-point X/Y coordinates).
 * In a real system this is burned into OTP or a protected flash page;
 * only the bootloader can read it.
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
 * Simulated firmware image.
 * In a real bootloader this would be the application binary read from flash.
 */
static const uint8_t FIRMWARE[] = "firmware:v1.0:hello-secure-world";
#define FIRMWARE_LEN (sizeof(FIRMWARE) - 1)   /* exclude null terminator */

static USART_HandleType huart;

/* Print one labelled test result */
static void print_result(const char *label, SEC_BOOT_Status st)
{
    USART_WriteString(&huart, label);
    USART_WriteString(&huart, st == SEC_BOOT_OK
                              ? "PASS\r\n"
                              : "FAIL  *** tamper detected ***\r\n");
}

int main(void)
{
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&huart, "\r\n=== Secure Boot Test ===\r\n\n");

    /* ── Step 1: Init secure-boot context with the trusted public key ── */
    SEC_BOOT_HandleTypeDef hsb;
    SEC_BOOT_Init(&hsb, TRUSTED_PUB_X, TRUSTED_PUB_Y);

    /*
     * ── Step 2: Compute and display the firmware image hash ────────────
     * In production: the signer computes SHA-256(image), signs it with the
     * private key, and stores (sig_r, sig_s) in flash next to the image.
     * We derive the hash at runtime here to keep the demo self-contained.
     */
    uint8_t firmware_hash[32];
    HASH_SHA256_Start(&hsb.hhash);
    HASH_SHA256_Update(&hsb.hhash, FIRMWARE, FIRMWARE_LEN);
    HASH_SHA256_Final(&hsb.hhash, firmware_hash);

    USART_WriteString(&huart, "Firmware SHA-256 : ");
    print_hex(&huart, firmware_hash, 32);
    USART_WriteString(&huart, "\r\n\n");

    /*
     * ── Step 3: Build the demo signature ───────────────────────────────
     * Our dummy ECDSA verifier passes when sig_r == SHA-256(image).
     * This mirrors real ECDSA: sig_r is mathematically tied to the hash.
     * In production (sig_r, sig_s) would be read from flash.
     */
    uint8_t       sig_r[32];
    const uint8_t sig_s[32] = {0x01};
    memcpy(sig_r, firmware_hash, 32);   /* sig_r = SHA-256(firmware) */

    /* ── Step 4: Verify the legitimate firmware image ───────────────── */
    SEC_BOOT_Status st;
    st = SEC_BOOT_VerifyImage(&hsb, FIRMWARE, FIRMWARE_LEN, sig_r, sig_s);
    print_result("Legitimate firmware : ", st);

    /* ── Step 5: Tamper one byte and re-verify (expect FAIL) ─────────── */
    uint8_t tampered[FIRMWARE_LEN];
    memcpy(tampered, FIRMWARE, FIRMWARE_LEN);
    tampered[0] ^= 0xFF;                /* flip the first byte */

    st = SEC_BOOT_VerifyImage(&hsb, tampered, FIRMWARE_LEN, sig_r, sig_s);
    print_result("Tampered  firmware  : ", st);

    /* ── Step 6: Final boot decision ────────────────────────────────── */
    USART_WriteString(&huart, "\r\nBoot decision: ");
    st = SEC_BOOT_VerifyImage(&hsb, FIRMWARE, FIRMWARE_LEN, sig_r, sig_s);
    if (st == SEC_BOOT_OK) {
        USART_WriteString(&huart, "Firmware TRUSTED -> launching application.\r\n");
        /*
         * In a real bootloader, jump to the application reset handler:
         *   uint32_t app_sp = *(uint32_t *)(APP_START_ADDR);
         *   uint32_t app_pc = *(uint32_t *)(APP_START_ADDR + 4);
         *   __set_MSP(app_sp);
         *   ((void (*)(void))app_pc)();
         */
    } else {
        USART_WriteString(&huart, "Firmware UNTRUSTED -> boot halted.\r\n");
        while (1) {}
    }

    while (1) {}
}