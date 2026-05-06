#include <string.h>
#include "../inc/rng.h"
#include "../../UART/inc/uart.h"

static USART_HandleType huart;

int main(void)
{
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&huart, "\r\n=== RNG Test ===\r\n\n");

    RNG_HandleTypeDef hrng;
    RNG_Constructor(&hrng);
    RNG_Enable(&hrng);
    USART_WriteString(&huart, "CR=0x"); uart_write_hex32(&huart, hrng.Instance->CR);
    USART_WriteString(&huart, " SR=0x"); uart_write_hex32(&huart, hrng.Instance->SR);
    /* NSMR at base+0x30: should be 0x1FF (all oscillators unmasked) */
    volatile uint32_t nsmr = *(volatile uint32_t*)(0x420C0800U + 0x30U);
    USART_WriteString(&huart, " NSMR=0x"); uart_write_hex32(&huart, nsmr);
    USART_WriteString(&huart, "\r\n");

    uint8_t random_data[16];
    if (RNG_Generate(&hrng, random_data, sizeof(random_data)) == RNG_OK) {
        USART_WriteString(&huart, "Random Data: ");
        for (size_t i = 0; i < sizeof(random_data); i++) {
            char byte_str[3];
            snprintf(byte_str, sizeof(byte_str), "%02X", random_data[i]);
            USART_WriteString(&huart, byte_str);
        }
        USART_WriteString(&huart, "\r\n");
    } else {
        USART_WriteString(&huart, "RNG failed SR=0x");
        uart_write_hex32(&huart, hrng.Instance->SR);
        USART_WriteString(&huart, " CR=0x");
        uart_write_hex32(&huart, hrng.Instance->CR);
        USART_WriteString(&huart, "\r\n");
    }

    RNG_Disable(&hrng);

    /* --- Interrupt-based generation --- */
    RNG_Enable(&hrng);
    uint8_t random_it[16];
    if (RNG_Generate_IT(&hrng, random_it, sizeof(random_it)) == RNG_OK) {
        USART_WriteString(&huart, "Random IT:   ");
        for (size_t i = 0; i < sizeof(random_it); i++) {
            char byte_str[3];
            snprintf(byte_str, sizeof(byte_str), "%02X", random_it[i]);
            USART_WriteString(&huart, byte_str);
        }
        USART_WriteString(&huart, "\r\n");
    } else {
        USART_WriteString(&huart, "RNG_IT generation failed\r\n");
    }
    RNG_Disable(&hrng);

    while (1) {}
}