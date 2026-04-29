#include <string.h>
#include "../inc/sha256.h"
#include "../../UART/inc/uart.h"

static USART_HandleType g_uart;

static void print_hex(const uint8_t *buf, size_t len)
{
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        USART_WriteChar(&g_uart, hex[(buf[i] >> 4) & 0xF]);
        USART_WriteChar(&g_uart, hex[ buf[i]       & 0xF]);
    }
}

static void run_test(const char *label, const char *msg, const char *expected)
{
    HASH_HandleTypeDef hhash;
    uint8_t digest[32];

    HASH_Init(&hhash);
    HASH_SHA256_Start(&hhash);
    HASH_SHA256_Update(&hhash, (const uint8_t *)msg, strlen(msg));
    HASH_SHA256_Final(&hhash, digest);

    USART_WriteString(&g_uart, label);
    print_hex(digest, 32);
    USART_WriteString(&g_uart, "\r\nExpected Output = ");
    USART_WriteString(&g_uart, expected);
    USART_WriteString(&g_uart, "\r\n\n");
}

int main(void)
{
    USART_constructor(&g_uart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&g_uart, "\r\n=== SHA-256 Tests ===\r\n\n");

    run_test("SHA-256(\"abc\")  = ",
             "abc",
             "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    run_test("SHA-256(\"abcd\") = ",
             "abcd",
             "88d4266fd4e6338d13b845fcf289579d209c897823b9217da3e161936f031589");


    while (1) {}
}