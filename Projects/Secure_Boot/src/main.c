#include "sec_boot.h"
#include <string.h>
#include <stdio.h>

/*
 * Trusted P-256 base-point coordinates used as the device public key.
 * In production these are burned into OTP or a write-protected flash sector;
 * only the bootloader can read them.
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
 * Demo image blob for the ECDSA simulation test.
 * In production this would be the real application binary in flash;
 * the signer computes SHA-256(image), signs it offline, and embeds
 * (sig_r, sig_s) in the image trailer before programming the device.
 */
static const uint8_t DEMO_IMAGE[] = "firmware:v1.0:hello-secure-world";
#define DEMO_IMAGE_LEN  (sizeof(DEMO_IMAGE) - 1U)

static USART_HandleType huart;

/*
 * ECDSA simulation test — exercises the SHA-256 + ECDSA verification path
 * without a real signed binary.  Our stub verifier passes when
 * sig_r == SHA-256(image), which mimics how real ECDSA ties sig_r to the
 * image hash.  In production (sig_r, sig_s) are read from the image trailer.
 */
static void run_ecdsa_sim_test(SEC_BOOT_HandleTypeDef *hsb)
{
    uint8_t       img_hash[32];
    uint8_t       sig_r[32];
    const uint8_t sig_s[32] = {0x01};
    SEC_BOOT_Status st;

    USART_WriteString(&huart,
        "\r\n--- ECDSA SIGNATURE VERIFICATION (simulation) ---\r\n");
    USART_WriteString(&huart,
        "  Note: stub verifier; production uses real P-256 signing.\r\n");

    /* Compute SHA-256 of the demo image */
    HASH_SHA256_Start(&hsb->hhash);
    HASH_SHA256_Update(&hsb->hhash, DEMO_IMAGE, DEMO_IMAGE_LEN);
    HASH_SHA256_Final(&hsb->hhash, img_hash);
    USART_WriteString(&huart, "  SHA-256 : ");
    print_hex(&huart, img_hash, 32);
    USART_WriteString(&huart, "\r\n");

    /* sig_r = SHA-256(image) makes the stub verifier accept the image */
    memcpy(sig_r, img_hash, 32);

    st = SEC_BOOT_VerifyImage(hsb, DEMO_IMAGE, DEMO_IMAGE_LEN, sig_r, sig_s);
    USART_WriteString(&huart, st == SEC_BOOT_OK
                              ? "  Authentic image : PASS\r\n"
                              : "  Authentic image : FAIL\r\n");

    /* Flip one byte — must be rejected */
    uint8_t tampered[DEMO_IMAGE_LEN];
    memcpy(tampered, DEMO_IMAGE, DEMO_IMAGE_LEN);
    tampered[0] ^= 0xFFU;

    st = SEC_BOOT_VerifyImage(hsb, tampered, DEMO_IMAGE_LEN, sig_r, sig_s);
    USART_WriteString(&huart, st != SEC_BOOT_OK
                              ? "  Tampered image  : FAIL  (expected)\r\n"
                              : "  Tampered image  : PASS  (unexpected!)\r\n");
}

int main(void)
{
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);

    SEC_BOOT_HandleTypeDef hsb;
    SEC_BOOT_Init(&hsb, TRUSTED_PUB_X, TRUSTED_PUB_Y);

    /* -- Banner --------------------------------------------------- */
    USART_WriteString(&huart, "\r\n=== SECURE BOOTLOADER v1.0 ===\r\n");
    USART_WriteString(&huart, "MCU      : STM32C562RE  Cortex-M33  48 MHz\r\n");
    USART_WriteString(&huart, "Flash    : 512 KB  @ 0x08000000\r\n");
    USART_WriteString(&huart, "SysROM   : ST ROM bootloader @ 0x0BF80000\r\n");
    USART_WriteString(&huart, "Self     : sectors  0-7   0x08000000  64 KB\r\n");
    USART_WriteString(&huart, "Active   : sectors  8-29  0x08010000  176 KB\r\n");
    USART_WriteString(&huart, "Fallback : sectors 32-61  0x08040000  240 KB\r\n");
    USART_WriteString(&huart, "================================\r\n\r\n");

    /* -- Boot sequence --------------------------------------------- */
    /*
     * SEC_BOOT_Boot checks active then fallback.
     * It jumps (never returns) if a valid sealed image is found.
     * It returns SEC_BOOT_ERR_NO_IMAGE when nothing can boot — which is
     * the expected result here since no application is flashed yet.
     */
    SEC_BOOT_Boot(&hsb, &huart);

    /*
     * Execution only reaches here if no bootable image was found.
     * Run the ECDSA simulation test to verify the signature path works,
     * then halt.
     */
    run_ecdsa_sim_test(&hsb);

    USART_WriteString(&huart, "\r\nSYSTEM HALTED\r\n");
    while (1) {}
}
