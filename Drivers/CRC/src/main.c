#include <string.h>
#include "../inc/crc.h"
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
    CRC_HandleTypeDef hcrc;
    CRC_Constructor(&hcrc);

    uint8_t crc_out[4];
    CRC_Calculate(&hcrc, (const uint8_t *)msg, strlen(msg), crc_out);

    USART_WriteString(&g_uart, label);
    print_hex(crc_out, 4);
    USART_WriteString(&g_uart, "\r\nExpected: ");
    USART_WriteString(&g_uart, expected);
    USART_WriteString(&g_uart, "\r\n\n");
}

int main(void)
{
    USART_constructor(&g_uart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&g_uart, "\r\n=== CRC Tests ===\r\n\n");

    run_test("CRC32(\"123456789\") = ", "123456789", "cbf43926");

    while (1) {}
}