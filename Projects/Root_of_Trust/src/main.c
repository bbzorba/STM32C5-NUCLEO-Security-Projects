#include "root_of_trust.h"
#include "uart.h"
#include <stdio.h>

static USART_HandleType    huart;
static HASH_HandleTypeDef  hhash;
static CRC_HandleTypeDef   hcrc;

int main(void)
{
    /* ── Initialise peripherals ──────────────────────────────────────── */
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    HASH_Constructor(&hhash);  /* needed for handle init even though SHA-256 is not used */
    CRC_Constructor(&hcrc);    /* needed for handle init even though CRC-32 is used */

    USART_WriteString(&huart, "\r\n=== ROOT OF TRUST MEASUREMENTS ===\r\n\n");

    /* ── 1. Device identity: 96-bit hardware unique ID ───────────────── */
    uint8_t uid[12];
    ROT_ReadUID(uid);
    USART_WriteString(&huart, "[1] Device UID (96-bit)  : ");
    print_hex(&huart, uid, 12);
    USART_WriteString(&huart, "\r\n");

    /* ── 2. Security posture: RDP level from option bytes ────────────── */
    uint8_t rdp = ROT_ReadRDPLevel();
    char buf[80];
    snprintf(buf, sizeof(buf),
        "[2] RDP Level            : 0x%02X (%s)\r\n",
        rdp,
        rdp == 0xAA ? "Level 0 - debug OPEN (no protection)" :
        rdp == 0xBB ? "Level 0.5 - RDP1 (debug disabled)"    :
        rdp == 0xCC ? "Level 2 - LOCKED (permanent)"         :
                      "Level 1 - protected (unknown sub-level)");
    USART_WriteString(&huart, buf);

    /* ── 3. Firmware integrity: CRC-32 of the active app slot (176 KB) ── */
    uint8_t digest[32];
    ROT_MeasureBootloader(&hhash, &hcrc, digest);
    USART_WriteString(&huart, "[3] Firmware CRC-32      : ");
    print_hex(&huart, digest, 4);
    USART_WriteString(&huart, "\r\n");

    USART_WriteString(&huart, "\r\n=== RoT COMPLETE ===\r\n");
    while (1);
}
