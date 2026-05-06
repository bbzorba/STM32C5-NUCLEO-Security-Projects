#include <string.h>
#include "../inc/crc.h"
#include "../../UART/inc/uart.h"

USART_HandleType huart;

static void run_test(const char *label, const char *msg, const char *expected)
{
    CRC_HandleTypeDef hcrc;
    CRC_Constructor(&hcrc);

    uint8_t crc_out[4];
    CRC_Calculate(&hcrc, (const uint8_t *)msg, strlen(msg), crc_out);

    USART_WriteString(&huart, label);
    print_hex(&huart, crc_out, 4);
    USART_WriteString(&huart, "\r\nExpected: ");
    USART_WriteString(&huart, expected);
    USART_WriteString(&huart, "\r\n\n");
}

int main(void)
{
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&huart, "\r\n=== CRC Tests ===\r\n\n");

    run_test("CRC32(\"123456789\") = ", "123456789", "cbf43926");

    while (1) {}
}