#include <string.h>
#include "../inc/hash.h"
#include "../../UART/inc/uart.h"

static USART_HandleType g_uart;

static void print_hex(USART_HandleType *uart, const uint8_t *buf, size_t len)
{
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        USART_WriteChar(uart, hex[(buf[i] >> 4) & 0xF]);
        USART_WriteChar(uart, hex[ buf[i]       & 0xF]);
    }
}

int main(void)
{
    USART_constructor(&g_uart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&g_uart, "\r\n=== Hash Tests ===\r\n");

    HASH_HandleTypeDef hash;
    uint8_t digest[32];

    /* --- Test 1: SHA-256("abc") --- */
    Hash_Constructor(&hash, HASH_SHA256);
    Hash_Process_Data(&hash, (const unsigned char *)"abc", 3);
    Hash_Final(&hash, digest);

    USART_WriteString(&g_uart, "SHA-256(\"abc\")    = ");
    print_hex(&g_uart, digest, 32);
    USART_WriteString(&g_uart, "\r\nExpected:          ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\r\n");

    /* --- Test 2: SHA-256("abcd") — exact 4-byte word, NBLW=0 --- */
    Hash_Constructor(&hash, HASH_SHA256);
    Hash_Process_Data(&hash, (const unsigned char *)"abcd", 4);
    Hash_Final(&hash, digest);

    USART_WriteString(&g_uart, "SHA-256(\"abcd\")   = ");
    print_hex(&g_uart, digest, 32);
    USART_WriteString(&g_uart, "\r\nExpected:          88d4266fd4e6338d13b845fcf289579d209c897823b9217da3e161936f031589\r\n");

    while (1) {}
}
