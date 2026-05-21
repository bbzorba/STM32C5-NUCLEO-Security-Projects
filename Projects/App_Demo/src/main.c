#include "uart.h"
#include <stdio.h>

/*
 * App_Demo — minimal application to demonstrate the Secure Boot handoff.
 *
 * The bootloader at 0x08000000 verifies the CRC tag in sector 30,
 * validates the MSP, relocates VTOR to 0x08010000, then jumps here.
 * This application runs in the ACTIVE slot (sectors 8-29, 0x08010000).
 */

static USART_HandleType huart;

/* Simple busy-wait delay (~1 second at 48 MHz with -O2) */
static void delay_1s(void)
{
    volatile uint32_t i = 4800000UL;
    while (i--) __asm volatile ("nop");
}

int main(void)
{
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);

    /* Read VTOR to confirm the bootloader placed us at the right address */
    uint32_t vtor = *(volatile uint32_t *)0xE000ED08U;

    char buf[64];
    USART_WriteString(&huart, "\r\n=== APPLICATION v1.0 ===\r\n");
    snprintf(buf, sizeof(buf), "Slot     : Active (sectors 8-29)\r\n");
    USART_WriteString(&huart, buf);
    snprintf(buf, sizeof(buf), "VTOR     : 0x%08lX\r\n", (unsigned long)vtor);
    USART_WriteString(&huart, buf);
    USART_WriteString(&huart, "Boot     : Secure Boot handoff OK\r\n\r\n");

    uint32_t tick = 0;
    while (1) {
        snprintf(buf, sizeof(buf), "tick %lu\r\n", (unsigned long)tick++);
        USART_WriteString(&huart, buf);
        delay_1s();
    }
}
