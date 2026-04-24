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
    Hash_Constructor(&hash, HASH_SHA256);

    const char *msg = "abc";
    uint8_t digest[32];

    Hash_Process_Data(&hash, (const unsigned char*)msg, strlen(msg));
    Hash_Final(&hash, digest);

    /* Expected SHA-256("abc"):
       ba7816bf8f01cfea414140de5dae2ec73b338c0aa843c8a3cb3480e4f0d9c872 */
    USART_WriteString(&g_uart, "SHA-256(\"abc\") = ");
    print_hex(&g_uart, digest, 32);
    USART_WriteString(&g_uart, "\r\nExpected:        ba7816bf8f01cfea414140de5dae2ec73b338c0aa843c8a3cb3480e4f0d9c872\r\n");

    while (1) {}
}
