#include "../inc/ecdsa.h"

int main(void)
{
    USART_HandleType huart;
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&huart, "\r\n=== ECDSA Test ===\r\n\n");

    HASH_HandleTypeDef hhash;
    HASH_Constructor(&hhash);

    /* P-256 base-point X/Y (public key, fixed demo values) */
    uint8_t pub_x[32] = {
        0x6b,0x17,0xd1,0xf2,0xe1,0x2c,0x42,0x47,
        0xf8,0xbc,0xe6,0xe5,0x63,0xa4,0x40,0xf2,
        0x77,0x03,0x7d,0x81,0x2d,0xeb,0x33,0xa0,
        0xf4,0xa1,0x39,0x45,0xd8,0x98,0xc2,0x96
    };
    uint8_t pub_y[32] = {
        0x4f,0xe3,0x42,0xe2,0xfe,0x1a,0x7f,0x9b,
        0x8e,0xe7,0xeb,0x4a,0x7c,0x0f,0x9e,0x16,
        0x2b,0xce,0x33,0x57,0x6b,0x31,0x5e,0xce,
        0xcb,0xb6,0x40,0x68,0x37,0xbf,0x51,0xf5
    };

    /* Dummy signature values */
    uint8_t r[32] = {1};
    uint8_t s[32] = {2};

    ECDSA_HandleTypeDef hecdsa;
    ECDSA_Init(&hecdsa, &hhash, pub_x, pub_y);

    const char *msg = "hello secure world";

    /* First: print the SHA-256 hash of the message to show HASH driver works */
    uint8_t hash[32];
    HASH_SHA256_Start(&hhash);
    HASH_SHA256_Update(&hhash, (const uint8_t *)msg, strlen(msg));
    HASH_SHA256_Final(&hhash, hash);
    USART_WriteString(&huart, "SHA-256: ");
    for (size_t i = 0; i < 32; i++) {
        char byte_str[3];
        snprintf(byte_str, sizeof(byte_str), "%02X", hash[i]);
        USART_WriteString(&huart, byte_str);
    }
    USART_WriteString(&huart, "\r\n");

    /* Then run ECDSA verify (dummy math — expected INVALID with arbitrary r/s) */
    int valid = ECDSA_Verify(&hecdsa, (const uint8_t *)msg, strlen(msg), r, s);

    if (valid)
        USART_WriteString(&huart, "Signature VALID\r\n");
    else
        USART_WriteString(&huart, "Signature INVALID (expected with dummy r/s)\r\n");

    while (1) {}
}