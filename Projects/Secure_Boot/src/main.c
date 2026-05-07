#include "../inc/sec_boot.h"

static USART_HandleType huart;

int main(void)
{
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&huart, "\r\n=== Secure Boot Test ===\r\n\n");

    /* TBD: add Secure Boot tests here */

    while (1) {}
}