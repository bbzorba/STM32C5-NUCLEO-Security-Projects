#include "bootloader.h"
#include "bl_request.h"

/*
 * Bootloader — simple UART bootloader for NUCLEO-C562RE.
 *
 * Lives at 0x08000000 (sectors 0-7) and owns the active slot at 0x08010000
 * (sectors 8-29) with its CRC-32 tag in sector 30 — the same map used by
 * Secure_Boot, App_Demo and tools/seal.py.
 *
 *   1. B1 held during reset, or a request from the application
 *      (bl_request.h, e.g. the 1200-baud USB touch) → stay in the bootloader
 *      (LD1 blinks)
 *   2. Otherwise open a BL_WAIT_MS (5 s) window, LD1 on — at least
 *      BL_USB_MOUNT_GRACE_MS (10 s) once a PC enumerates the USB port:
 *        'u' on USART2 (ST-LINK VCP)          → receive a new image over UART
 *        'u' on USB CDC (USB-C user connector) → receive a new image over USB
 *                                               (both: tools/bl_upload.py)
 *        B1 pressed                  → cancel auto-boot, stay in the bootloader
 *   3. Verify the active slot (tag + CRC-32 + vector table)
 *   4. Valid → jump to the application, otherwise stay here and wait
 */

static USART_HandleType huart;
static BL_HandleTypeDef hbl;

static void BL_RunUpdate(void)
{
    char buf[48];
    BL_Status st = BL_ReceiveImage(&hbl);
    snprintf(buf, sizeof(buf), "\r\n[BL] Update : %s\r\n", BL_StatusString(st));
    BL_Log(&hbl, buf);
}

int main(void)
{
    USART_constructor(&huart, USART_2, RX_AND_TX, __115200);
    BL_Init(&hbl, &huart);

    /* Sample B1 first, before the banner, so a press during reset is not missed */
    uint8_t held      = BL_ButtonPressed(&hbl);
    uint8_t requested = BL_TakeRequest();
    uint8_t stay      = held || requested;

    /* ── Banner ────────────────────────────────────────────────────── */
    USART_WriteString(&huart, "\r\n=== UART + USB BOOTLOADER v1.3 ===\r\n");
    USART_WriteString(&huart, "MCU    : STM32C562RE  Cortex-M33  48 MHz\r\n");
    USART_WriteString(&huart, "Self   : sectors  0-7   0x08000000  64 KB\r\n");
    USART_WriteString(&huart, "Active : sectors  8-29  0x08010000  176 KB\r\n");
    USART_WriteString(&huart, "Tag    : sector  30     0x0803C000  CRC-32\r\n");
    USART_WriteString(&huart, "Update : send 'u' within 5 s, 10 s once USB is enumerated (tools/bl_upload.py)\r\n");
    USART_WriteString(&huart, "Links  : USART2 (ST-LINK VCP) | USB CDC (USB-C user port)\r\n");
    USART_WriteString(&huart, "Stay   : hold B1 during reset, press it in the window, or 1200-baud touch on the app's USB port\r\n");
    USART_WriteString(&huart, "=============================\r\n");
    if (held)
        BL_Log(&hbl, "[BL] B1 held — staying in bootloader\r\n");
    if (requested)
        BL_Log(&hbl, "[BL] Requested by the application — staying in bootloader\r\n");

    while (1) {
        /* Host waits for this exact line before sending 'u' */
        BL_Log(&hbl, "[BL] READY\r\n");

        BL_Trigger trig = BL_WaitForUpdateRequest(&hbl, stay ? BL_WAIT_FOREVER : BL_WAIT_MS);

        if (trig == BL_TRIGGER_BUTTON) {
            BL_Log(&hbl, "[BL] B1 pressed — auto-boot cancelled, waiting for update\r\n");
            stay = 1U;
            continue;
        }
        if (trig == BL_TRIGGER_UART || trig == BL_TRIGGER_USB)
            BL_RunUpdate();

        /* Window expired or update finished — try to boot */
        if (BL_VerifyActiveSlot(&hbl) == BL_OK) {
            BL_Log(&hbl, "[BL] Jumping to application ...\r\n");
            BL_JumpToApplication(&hbl, BL_ACTIVE_ADDR);   /* never returns */
        }

        /* No bootable image — stay in the bootloader until one arrives */
        BL_Log(&hbl, "[BL] No valid application — waiting for update\r\n");
        stay = 1U;
    }
}
