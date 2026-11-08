#include "uart.h"
#include "usb_cdc.h"
#include "bl_request.h"
#include <stdio.h>

/*
 * App_Demo — minimal application for the active slot (sectors 8-29,
 * 0x08010000), started by the boot code at 0x08000000 (Projects/Bootloader or
 * Projects/Secure_Boot), which relocates VTOR here before jumping.
 *
 *   - prints a tick every second on USART2 (ST-LINK VCP) and, while a PC has
 *     it open, on its own USB CDC port (USB-C user connector, CAFE:B0C6)
 *   - 1200-baud touch on that USB port → BL_RequestBootloader(): the MCU
 *     resets into Projects/Bootloader, which stays and waits for an upload.
 *     `make bl-upload LINK=usb` (tools/bl_upload.py) does this by itself.
 */

#define APP_USB_PRODUCT  "C562 App CDC"

static USART_HandleType huart;

/* Log line on USART2, and on USB while its COM port is open */
static void app_log(const char *str)
{
    USART_WriteString(&huart, str);
    if (USB_CDC_IsOpen())
        USB_CDC_WriteString(str);
}

/* Let USB finish the current control transfer, detach, then reset into the bootloader */
static void restart_into_bootloader(void)
{
    app_log("[APP] 1200-baud touch on USB — restarting into the bootloader\r\n");

    uint32_t start = USB_CDC_Millis();
    while (USB_CDC_Millis() - start < 20U)
        USB_CDC_Task();
    USB_CDC_Deinit();                         /* host sees an unplug, then the bootloader */

    while (USB_CDC_Millis() - start < 25U) {} /* last UART character leaves the shifter */
    BL_RequestBootloader();                   /* never returns */
}

int main(void)
{
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    USB_CDC_SetIdentity(USB_CDC_APP_PID, APP_USB_PRODUCT);
    USB_CDC_Init();

    /* Read VTOR to confirm the boot code placed us at the right address */
    uint32_t vtor = *(volatile uint32_t *)0xE000ED08U;

    char buf[64];
    app_log("\r\n=== APPLICATION v1.1 ===\r\n");
    app_log("Slot     : Active (sectors 8-29)\r\n");
    snprintf(buf, sizeof(buf), "VTOR     : 0x%08lX\r\n", (unsigned long)vtor);
    app_log(buf);
    app_log("Boot     : started by the boot code at 0x08000000\r\n");
    app_log("USB      : " APP_USB_PRODUCT " (CAFE:B0C6) — 1200-baud touch = bootloader\r\n\r\n");

    uint32_t tick = 0;
    uint32_t last = USB_CDC_Millis();
    while (1) {
        USB_CDC_Task();

        if (USB_CDC_TakeTouchEvent())
            restart_into_bootloader();

        if (USB_CDC_Millis() - last >= 1000U) {
            last += 1000U;
            snprintf(buf, sizeof(buf), "tick %lu\r\n", (unsigned long)tick++);
            app_log(buf);
        }
    }
}
