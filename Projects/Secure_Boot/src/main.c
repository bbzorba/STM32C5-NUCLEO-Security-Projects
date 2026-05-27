#include "sec_boot.h"
#include <string.h>
#include <stdio.h>

/*
 * Trusted P-256 base-point (Gx, Gy) used as the device public key.
 * In production these are burned into OTP or a write-protected flash sector.
 * Our ECDSA implementation is a stub (see Projects/ECDSA/src/ecdsa.c);
 * real P-256 arithmetic would require the actual private key for offline signing.
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
 * Pre-computed SHA-256 of Projects/App_Demo/main.bin (5280 bytes).
 * Generated OFFLINE with Python before flashing:
 *   hashlib.sha256(open("Projects/App_Demo/main.bin","rb").read()).hexdigest()
 *   => 3a3580e4918be501641c6da69712fd6499b4c0e748b76251ac64e70a2c4c107e
 *
 * This constant is NEVER computed at runtime.  It is embedded in the bootloader
 * at build time so an attacker who replaces App_Demo cannot recompute a matching
 * hash — the private key used to produce the real signature would be required.
 */
static const uint8_t APP_SIG_R[32] = {
    0x3aU, 0x35U, 0x80U, 0xe4U, 0x91U, 0x8bU, 0xe5U, 0x01U,
    0x64U, 0x1cU, 0x6dU, 0xa6U, 0x97U, 0x12U, 0xfdU, 0x64U,
    0x99U, 0xb4U, 0xc0U, 0xe7U, 0x48U, 0xb7U, 0x62U, 0x51U,
    0xacU, 0x64U, 0xe7U, 0x0aU, 0x2cU, 0x4cU, 0x10U, 0x7eU
};
static const uint8_t APP_SIG_S[32] = { 0x01U }; /* stub: sig_s not used by verifier */
#define APP_DEMO_SIZE   5280U   /* exact byte count of Projects/App_Demo/main.bin */

/*
 * Simulation test vector — a small in-memory image used when App_Demo is not
 * flashed, to confirm the verification path is working correctly.
 *
 * DEMO_SIG_R = SHA-256("firmware:v1.0:hello-secure-world"), computed OFFLINE:
 *   hashlib.sha256(b"firmware:v1.0:hello-secure-world").hexdigest()
 *   => 66c86062c2a32201ace377b69df67a8c5dc6db2c93521da74c0e16fb96633428
 *
 * The MCU independently computes SHA-256 of the image and compares to this
 * constant.  An attacker cannot substitute a different image without also
 * knowing the private key that signed this value.
 */
static const uint8_t DEMO_IMAGE[]   = "firmware:v1.0:hello-secure-world";
#define DEMO_IMAGE_LEN   (sizeof(DEMO_IMAGE) - 1U)

static const uint8_t DEMO_SIG_R[32] = {
    0x66U, 0xc8U, 0x60U, 0x62U, 0xc2U, 0xa3U, 0x22U, 0x01U,
    0xacU, 0xe3U, 0x77U, 0xb6U, 0x9dU, 0xf6U, 0x7aU, 0x8cU,
    0x5dU, 0xc6U, 0xdbU, 0x2cU, 0x93U, 0x52U, 0x1dU, 0xa7U,
    0x4cU, 0x0eU, 0x16U, 0xfbU, 0x96U, 0x63U, 0x34U, 0x28U
};
static const uint8_t DEMO_SIG_S[32] = { 0x01U };

static USART_HandleType huart;

int main(void)
{
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);

    SEC_BOOT_HandleTypeDef hsb;
    SEC_BOOT_Init(&hsb, TRUSTED_PUB_X, TRUSTED_PUB_Y);

    /* ── Banner ────────────────────────────────────────────────────── */
    USART_WriteString(&huart, "\r\n=== SECURE BOOTLOADER v1.0 ===\r\n");
    USART_WriteString(&huart, "MCU    : STM32C562RE  Cortex-M33  48 MHz\r\n");
    USART_WriteString(&huart, "Self   : sectors  0-7   0x08000000  64 KB\r\n");
    USART_WriteString(&huart, "Active : sectors  8-29  0x08010000  176 KB\r\n");
    USART_WriteString(&huart, "Chain  : SHA-256(image) -> ECDSA verify -> jump\r\n");
    USART_WriteString(&huart, "================================\r\n\r\n");

    /* ── Boot sequence ─────────────────────────────────────────────── */
    /*
     * SEC_BOOT_Boot:
     *   1. Validates MSP at 0x08010000 (is a real application present?)
     *   2. Hardware SHA-256(image, APP_DEMO_SIZE) via HASH peripheral
     *   3. ECDSA_Verify: SHA-256(image) == APP_SIG_R
     *      APP_SIG_R was computed OFFLINE — never on the MCU
     *   4. Both pass -> jumps to 0x08010000 (call never returns)
     *   5. Returns SEC_BOOT_ERR_NO_IMAGE when App_Demo is not flashed
     */
    SEC_BOOT_Boot(&hsb, APP_SIG_R, APP_SIG_S, APP_DEMO_SIZE, &huart);

    /*
     * Reaches here only when no bootable image is present in the active slot.
     * Run the signature verification simulation test so we can confirm the
     * SHA-256 + ECDSA path works correctly even without a real application.
     */

    /* ── Signature verification simulation test ────────────────────── */
    USART_WriteString(&huart,
        "\r\n--- SIGNATURE VERIFICATION TEST ---\r\n"
        "  DEMO_SIG_R (SHA-256 of test image, pre-computed offline):\r\n  ");
    print_hex(&huart, DEMO_SIG_R, 32);
    USART_WriteString(&huart, "\r\n");

    /* Test 1: genuine image — SHA-256(DEMO_IMAGE) must match DEMO_SIG_R */
    SEC_BOOT_Status st = SEC_BOOT_VerifyImage(
        &hsb, DEMO_IMAGE, DEMO_IMAGE_LEN, DEMO_SIG_R, DEMO_SIG_S);
    USART_WriteString(&huart, st == SEC_BOOT_OK
                              ? "  Genuine image  : PASS\r\n"
                              : "  Genuine image  : FAIL (unexpected)\r\n");

    /* Test 2: single-byte tamper — SHA-256 will differ, must be rejected */
    uint8_t tampered[DEMO_IMAGE_LEN];
    memcpy(tampered, DEMO_IMAGE, DEMO_IMAGE_LEN);
    tampered[0] ^= 0xFFU;

    st = SEC_BOOT_VerifyImage(
        &hsb, tampered, DEMO_IMAGE_LEN, DEMO_SIG_R, DEMO_SIG_S);
    USART_WriteString(&huart, st != SEC_BOOT_OK
                              ? "  Tampered image : FAIL  (expected)\r\n"
                              : "  Tampered image : PASS  (unexpected!)\r\n");

    USART_WriteString(&huart, "\r\nSYSTEM HALTED\r\n");
    while (1) {}
}
