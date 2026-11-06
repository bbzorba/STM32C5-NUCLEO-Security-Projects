#include "bootloader.h"

/*
 * Bootloader — simple UART bootloader for NUCLEO-C562RE.
 *
 * Lives at 0x08000000 (sectors 0-7) and owns the active slot at 0x08010000
 * (sectors 8-29) with its CRC-32 tag in sector 30 — the same map used by
 * Secure_Boot, App_Demo and tools/seal.py.
 *
 *   1. After reset, wait BL_WAIT_MS for 'u' on USART2 (ST-LINK VCP)
 *   2. 'u' received → receive a new image (tools/bl_upload.py)
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
    USART_WriteString(&huart, buf);
}

int main(void)
{
    USART_constructor(&huart, USART_2, RX_AND_TX, __115200);
    BL_Init(&hbl, &huart);

    /* ── Banner ────────────────────────────────────────────────────── */
    USART_WriteString(&huart, "\r\n=== UART BOOTLOADER v1.0 ===\r\n");
    USART_WriteString(&huart, "MCU    : STM32C562RE  Cortex-M33  48 MHz\r\n");
    USART_WriteString(&huart, "Self   : sectors  0-7   0x08000000  64 KB\r\n");
    USART_WriteString(&huart, "Active : sectors  8-29  0x08010000  176 KB\r\n");
    USART_WriteString(&huart, "Tag    : sector  30     0x0803C000  CRC-32\r\n");
    USART_WriteString(&huart, "Update : send 'u' within 3 s (tools/bl_upload.py)\r\n");
    USART_WriteString(&huart, "=============================\r\n");
    /* Host waits for this exact line before sending 'u' */
    USART_WriteString(&huart, "[BL] READY\r\n");

    /* ── Update window ─────────────────────────────────────────────── */
    if (BL_WaitForUpdateRequest(&hbl, BL_WAIT_MS))
        BL_RunUpdate();

    /* ── Boot ──────────────────────────────────────────────────────── */
    while (1) {
        if (BL_VerifyActiveSlot(&hbl) == BL_OK) {
            USART_WriteString(&huart, "[BL] Jumping to application ...\r\n");
            BL_JumpToApplication(&hbl, BL_ACTIVE_ADDR);   /* never returns */
        }

        /* No bootable image — stay in the bootloader until one arrives */
        USART_WriteString(&huart,
            "[BL] No valid application — waiting for update\r\n"
            "[BL] READY\r\n");
        while (!BL_WaitForUpdateRequest(&hbl, BL_WAIT_MS)) {}
        BL_RunUpdate();
    }
}
