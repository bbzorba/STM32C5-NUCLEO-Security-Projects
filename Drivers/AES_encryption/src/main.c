#include <string.h>
#include "../inc/aes.h"
#include "../../UART/inc/uart.h"

static USART_HandleType g_uart;

static void print_hex(const char *label, const uint8_t *b, uint32_t n)
{
    static const char h[] = "0123456789ABCDEF";
    char tmp[3] = {0, 0, 0};
    USART_WriteString(&g_uart, label);
    USART_WriteString(&g_uart, ": ");
    for (uint32_t i = 0; i < n; i++) {
        tmp[0] = h[b[i] >> 4]; tmp[1] = h[b[i] & 0xF];
        USART_WriteString(&g_uart, tmp);
    }
    USART_WriteString(&g_uart, "\r\n");
}

static void pass_fail(const char *name, int ok)
{
    USART_WriteString(&g_uart, name);
    USART_WriteString(&g_uart, ok ? ": PASS\r\n" : ": FAIL\r\n");
}

/* Test 1 â€” AES-128-CBC  (NIST SP 800-38A F.2.1) */
static void test_aes_cbc(void)
{
    USART_WriteString(&g_uart, "\r\n=== Test 1: AES-128-CBC (NIST F.2.1) ===\r\n");

    static const uint8_t key[16] = {
        0x2b,0x7e,0x15,0x16, 0x28,0xae,0xd2,0xa6,
        0xab,0xf7,0x15,0x88, 0x09,0xcf,0x4f,0x3c };
    static const uint8_t iv[16] = {
        0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f };
    static const uint8_t pt[16] = {
        0x6b,0xc1,0xbe,0xe2, 0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11, 0x73,0x93,0x17,0x2a };
    static const uint8_t exp[16] = {
        0x76,0x49,0xab,0xac, 0x81,0x19,0xb2,0x46,
        0xce,0xe9,0x8e,0x9b, 0x12,0xe9,0x19,0x7d };

    AES_HandleTypeDef h = { .mode = AES_MODE_CBC, .keysize = AES_KEYSIZE_128 };
    AES_constructor(&h);
    uint8_t ct[16], dt[16];

    AES_Encrypt_Init(&h, key, iv);
    AES_Process(&h, pt, ct, 16);
    print_hex("  PT ", pt, 16);
    print_hex("  CT ", ct, 16);
    pass_fail("  Encrypt vs NIST", memcmp(ct, exp, 16) == 0);

    AES_Decrypt_Init(&h, key, iv);
    AES_Process(&h, ct, dt, 16);
    print_hex("  DT ", dt, 16);
    pass_fail("  Decrypt roundtrip", memcmp(dt, pt, 16) == 0);
}

/* Test 2 â€” SAES-256-ECB  (NIST SP 800-38A F.1.5) */
static void test_saes_ecb(void)
{
    USART_WriteString(&g_uart, "\r\n=== Test 2: SAES-256-ECB (NIST F.1.5) ===\r\n");

    static const uint8_t key[32] = {
        0x60,0x3d,0xeb,0x10, 0x15,0xca,0x71,0xbe,
        0x2b,0x73,0xae,0xf0, 0x85,0x7d,0x77,0x81,
        0x1f,0x35,0x2c,0x07, 0x3b,0x61,0x08,0xd7,
        0x2d,0x98,0x10,0xa3, 0x09,0x14,0xdf,0xf4 };
    static const uint8_t pt[16] = {
        0x6b,0xc1,0xbe,0xe2, 0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11, 0x73,0x93,0x17,0x2a };
    static const uint8_t exp[16] = {
        0xf3,0xee,0xd1,0xbd, 0xb5,0xd2,0xa0,0x3c,
        0x06,0x4b,0x5a,0x7e, 0x3d,0xb1,0x81,0xf8 };

    AES_HandleTypeDef h = { .mode = AES_MODE_ECB };
    SAES_constructor(&h);   /* sets keysize = AES_KEYSIZE_256 */
    uint8_t ct[16], dt[16];

    AES_Encrypt_Init(&h, key, NULL);
    if (AES_Process(&h, pt, ct, 16) != AES_SUCCESS) { USART_WriteString(&g_uart, "  Encrypt error\r\n"); return; }
    print_hex("  PT ", pt, 16);
    print_hex("  CT ", ct, 16);
    pass_fail("  Encrypt vs NIST", memcmp(ct, exp, 16) == 0);

    AES_Decrypt_Init(&h, key, NULL);
    if (AES_Process(&h, ct, dt, 16) != AES_SUCCESS) { USART_WriteString(&g_uart, "  Decrypt error\r\n"); return; }
    print_hex("  DT ", dt, 16);
    pass_fail("  Decrypt roundtrip", memcmp(dt, pt, 16) == 0);
}

/* Test 3 â€” SAES-256-CTR  (NIST SP 800-38A F.5.5) */
static void test_saes_ctr(void)
{
    USART_WriteString(&g_uart, "\r\n=== Test 3: SAES-256-CTR (NIST F.5.5) ===\r\n");

    static const uint8_t key[32] = {
        0x60,0x3d,0xeb,0x10, 0x15,0xca,0x71,0xbe,
        0x2b,0x73,0xae,0xf0, 0x85,0x7d,0x77,0x81,
        0x1f,0x35,0x2c,0x07, 0x3b,0x61,0x08,0xd7,
        0x2d,0x98,0x10,0xa3, 0x09,0x14,0xdf,0xf4 };
    static const uint8_t nonce[16] = {
        0xf0,0xf1,0xf2,0xf3, 0xf4,0xf5,0xf6,0xf7,
        0xf8,0xf9,0xfa,0xfb, 0xfc,0xfd,0xfe,0xff };
    static const uint8_t pt[16] = {
        0x6b,0xc1,0xbe,0xe2, 0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11, 0x73,0x93,0x17,0x2a };
    static const uint8_t exp[16] = {
        0x60,0x1e,0xc3,0x13, 0x77,0x57,0x89,0xa5,
        0xb7,0xa7,0xf5,0x04, 0xbb,0xf3,0xd2,0x28 };

    AES_HandleTypeDef h = { .mode = AES_MODE_CTR };
    SAES_constructor(&h);
    uint8_t ct[16], dt[16];

    AES_Encrypt_Init(&h, key, nonce);
    if (AES_Process(&h, pt, ct, 16) != AES_SUCCESS) { USART_WriteString(&g_uart, "  CTR error\r\n"); return; }
    print_hex("  PT ", pt, 16);
    print_hex("  CT ", ct, 16);
    pass_fail("  Encrypt vs NIST", memcmp(ct, exp, 16) == 0);

    AES_Decrypt_Init(&h, key, nonce);
    AES_Process(&h, ct, dt, 16);
    print_hex("  DT ", dt, 16);
    pass_fail("  Decrypt roundtrip", memcmp(dt, pt, 16) == 0);
}

/* Test 4 â€” SAES-CBC DHUK  (hardware key, roundtrip only) */
static void test_saes_dhuk(void)
{
    USART_WriteString(&g_uart, "\r\n=== Test 4: SAES-CBC DHUK (HW key) ===\r\n");

    static const uint8_t iv[16] = {
        0x00,0x11,0x22,0x33, 0x44,0x55,0x66,0x77,
        0x88,0x99,0xaa,0xbb, 0xcc,0xdd,0xee,0xff };
    static const uint8_t pt[16] = {
        0xDE,0xAD,0xBE,0xEF, 0xCA,0xFE,0xBA,0xBE,
        0x01,0x23,0x45,0x67, 0x89,0xAB,0xCD,0xEF };

    AES_HandleTypeDef h = { .mode = AES_MODE_CBC };
    SAES_constructor(&h);
    uint8_t ct[16], dt[16];

    if (SAES_DHUK_Encrypt_Init(&h, iv) != AES_SUCCESS) {
        USART_WriteString(&g_uart, "  DHUK not available (KEYVALID=0) - SKIP\r\n"); return;
    }
    AES_Process(&h, pt, ct, 16);
    print_hex("  PT ", pt, 16);
    print_hex("  CT ", ct, 16);

    if (SAES_DHUK_Decrypt_Init(&h, iv) != AES_SUCCESS) {
        USART_WriteString(&g_uart, "  DHUK decrypt init error\r\n"); return;
    }
    AES_Process(&h, ct, dt, 16);
    print_hex("  DT ", dt, 16);
    pass_fail("  DHUK roundtrip", memcmp(dt, pt, 16) == 0);
}

int main(void)
{
    USART_constructor(&g_uart, USART_2, TX_ONLY, __115200);

    USART_WriteString(&g_uart, "\r\n====================================\r\n");
    USART_WriteString(&g_uart,   " STM32C562RE AES/SAES Driver Tests  \r\n");
    USART_WriteString(&g_uart,   "====================================\r\n");

    test_aes_cbc();
    test_saes_ecb();
    test_saes_ctr();
    test_saes_dhuk();

    USART_WriteString(&g_uart, "\r\n--- All tests done ---\r\n");
    while (1) {}
}


