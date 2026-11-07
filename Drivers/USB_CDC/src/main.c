/*
 * USB_CDC demo — NUCLEO-C562RE
 *
 * Plug a USB cable into the USB Type-C user connector (not the ST-LINK one).
 * The board enumerates as "C562 Bootloader CDC" (VID 0xCAFE / PID 0xB0C5):
 *   Windows : USB Serial Device (COMx)       Linux / WSL+usbipd : /dev/ttyACM*
 * Open it with any terminal — every character is echoed back in upper case.
 * Status is logged on USART2 (ST-LINK VCP, 115200 8N1).
 */
#include "uart.h"
#include "usb_cdc.h"

static USART_HandleType huart;

#define GREETING_DELAY_MS  200U

int main(void)
{
    USART_constructor(&huart, USART_2, RX_AND_TX, __115200);
    USART_WriteString(&huart, "\r\n=== USB CDC demo ===\r\n");

    USB_CDC_Init();
    USART_WriteString(&huart, "USB started — connect the USB-C user port\r\n");

    uint8_t was_mounted = 0U;
    uint8_t  greet = 0U;
    uint32_t greet_at = 0U;
    while (1) {
        USB_CDC_Task();

        uint8_t mounted = USB_CDC_IsMounted();
        if (mounted != was_mounted) {
            USART_WriteString(&huart, mounted ? "USB: enumerated by host\r\n"
                                              : "USB: detached\r\n");
            was_mounted = mounted;
        }
        if (USB_CDC_TakeOpenEvent()) {
            USART_WriteString(&huart, "USB: COM port opened\r\n");
            greet_at = USB_CDC_Millis() + GREETING_DELAY_MS;
            greet    = 1U;
        }
        /* Greet a little after the open: Windows serial libraries (pyserial,
           PuTTY …) purge their RX buffer right after asserting DTR */
        if (greet && (int32_t)(USB_CDC_Millis() - greet_at) >= 0) {
            greet = 0U;
            USB_CDC_WriteString("\r\nC562 USB CDC echo — type something\r\n");
        }

        uint8_t c;
        while (USB_CDC_ReadByte(&c)) {
            if (c >= 'a' && c <= 'z')
                c = (uint8_t)(c - 'a' + 'A');
            USB_CDC_Write(&c, 1U);
        }
    }
}
