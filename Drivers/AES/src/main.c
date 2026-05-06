#include <string.h>
#include "../inc/aes.h"
#include "../../UART/inc/uart.h"

USART_HandleType huart;

/* ── Shared NIST SP 800-38A test vectors ─────────────────────────────── */
static const uint8_t KEY128[16] = {
    0x2b,0x7e,0x15,0x16, 0x28,0xae,0xd2,0xa6,
    0xab,0xf7,0x15,0x88, 0x09,0xcf,0x4f,0x3c };

static const uint8_t KEY256[32] = {
    0x60,0x3d,0xeb,0x10, 0x15,0xca,0x71,0xbe,
    0x2b,0x73,0xae,0xf0, 0x85,0x7d,0x77,0x81,
    0x1f,0x35,0x2c,0x07, 0x3b,0x61,0x08,0xd7,
    0x2d,0x98,0x10,0xa3, 0x09,0x14,0xdf,0xf4 };

static const uint8_t plaintext[16] = {                  /* plaintext (all tests) */
    0x6b,0xc1,0xbe,0xe2, 0x2e,0x40,0x9f,0x96,
    0xe9,0x3d,0x7e,0x11, 0x73,0x93,0x17,0x2a };

static const uint8_t IV[16] = {                  /* CBC IV */
    0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f };

static const uint8_t NONCE[16] = {               /* CTR initial counter */
    0xf0,0xf1,0xf2,0xf3, 0xf4,0xf5,0xf6,0xf7,
    0xf8,0xf9,0xfa,0xfb, 0xfc,0xfd,0xfe,0xff };

static void pass_fail(const char *lbl, int ok)
{
    USART_WriteString(&huart, lbl);
    USART_WriteString(&huart, ok ? ": PASS\r\n" : ": FAIL\r\n");
}

/* ── ECB (F.1.1 / F.1.5) ─────────────────────────────────────────────── */
static void test_ecb(void)
{
    static const uint8_t exp128[16] = {
        0x3a,0xd7,0x7b,0xb4, 0x0d,0x7a,0x36,0x60,
        0xa8,0x9e,0xca,0xf3, 0x24,0x66,0xef,0x97 };
    static const uint8_t exp256[16] = {
        0xf3,0xee,0xd1,0xbd, 0xb5,0xd2,0xa0,0x3c,
        0x06,0x4b,0x5a,0x7e, 0x3d,0xb1,0x81,0xf8 };

    uint8_t ct[16], dt[16];

    USART_WriteString(&huart, "\r\n--- ECB ---\r\n");

    AES_ECB_Encrypt(KEY128, AES_KEYSIZE_128, plaintext, ct, 16);
    pass_fail("  ECB-128 enc", memcmp(ct, exp128, 16) == 0);
    AES_ECB_Decrypt(KEY128, AES_KEYSIZE_128, ct, dt, 16);
    pass_fail("  ECB-128 dec", memcmp(dt, plaintext, 16) == 0);

    AES_ECB_Encrypt(KEY256, AES_KEYSIZE_256, plaintext, ct, 16);
    pass_fail("  ECB-256 enc", memcmp(ct, exp256, 16) == 0);
    AES_ECB_Decrypt(KEY256, AES_KEYSIZE_256, ct, dt, 16);
    pass_fail("  ECB-256 dec", memcmp(dt, plaintext, 16) == 0);
}

/* ── CBC (F.2.1 / F.2.5) ─────────────────────────────────────────────── */
static void test_cbc(void)
{
    static const uint8_t exp128[16] = {
        0x76,0x49,0xab,0xac, 0x81,0x19,0xb2,0x46,
        0xce,0xe9,0x8e,0x9b, 0x12,0xe9,0x19,0x7d };
    static const uint8_t exp256[16] = {
        0xf5,0x8c,0x4c,0x04, 0xd6,0xe5,0xf1,0xba,
        0x77,0x9e,0xab,0xfb, 0x5f,0x7b,0xfb,0xd6 };

    uint8_t ct[16], dt[16];

    USART_WriteString(&huart, "\r\n--- CBC ---\r\n");

    AES_CBC_Encrypt(KEY128, AES_KEYSIZE_128, IV, plaintext, ct, 16);
    pass_fail("  CBC-128 enc", memcmp(ct, exp128, 16) == 0);
    AES_CBC_Decrypt(KEY128, AES_KEYSIZE_128, IV, ct, dt, 16);
    pass_fail("  CBC-128 dec", memcmp(dt, plaintext, 16) == 0);

    AES_CBC_Encrypt(KEY256, AES_KEYSIZE_256, IV, plaintext, ct, 16);
    pass_fail("  CBC-256 enc", memcmp(ct, exp256, 16) == 0);
    AES_CBC_Decrypt(KEY256, AES_KEYSIZE_256, IV, ct, dt, 16);
    pass_fail("  CBC-256 dec", memcmp(dt, plaintext, 16) == 0);
}

/* ── CTR (F.5.1 / F.5.5) ─────────────────────────────────────────────── */
static void test_ctr(void)
{
    static const uint8_t exp128[16] = {
        0x87,0x4d,0x61,0x91, 0xb6,0x20,0xe3,0x26,
        0x1b,0xef,0x68,0x64, 0x99,0x0d,0xb6,0xce };
    static const uint8_t exp256[16] = {
        0x60,0x1e,0xc3,0x13, 0x77,0x57,0x89,0xa5,
        0xb7,0xa7,0xf5,0x04, 0xbb,0xf3,0xd2,0x28 };

    uint8_t ct[16], dt[16];

    USART_WriteString(&huart, "\r\n--- CTR ---\r\n");

    AES_CTR_Crypt(KEY128, AES_KEYSIZE_128, NONCE, plaintext, ct, 16);
    pass_fail("  CTR-128 enc", memcmp(ct, exp128, 16) == 0);
    AES_CTR_Crypt(KEY128, AES_KEYSIZE_128, NONCE, ct, dt, 16);
    pass_fail("  CTR-128 dec", memcmp(dt, plaintext, 16) == 0);

    AES_CTR_Crypt(KEY256, AES_KEYSIZE_256, NONCE, plaintext, ct, 16);
    pass_fail("  CTR-256 enc", memcmp(ct, exp256, 16) == 0);
    AES_CTR_Crypt(KEY256, AES_KEYSIZE_256, NONCE, ct, dt, 16);
    pass_fail("  CTR-256 dec", memcmp(dt, plaintext, 16) == 0);
}


int main(void)
{
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&huart, "\r\n=== AES Tests (NIST SP 800-38A) ===\r\n");

    test_ecb();
    test_cbc();
    test_ctr();

    USART_WriteString(&huart, "\r\n--- Done ---\r\n");
    while (1) {}
}
