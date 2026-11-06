#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "uart.h"     /* console + firmware update link (USART2)   */
#include "crc.h"      /* hardware CRC-32 (active slot integrity)   */
#include "flash.h"    /* sector erase / unlock / lock              */

#define __IO volatile

/* ── Flash memory map (STM32C562RE, 512 KB, single-bank, 8 KB sectors) ──
 * Same layout as Secure_Boot and tools/seal.py, so an image sealed with
 * 'make seal-active' boots here too.                                      */
#define BL_SECTOR_SIZE          0x00002000U               /* 8 KB per sector                   */
#define BL_SELF_ADDR            0x08000000U               /* Bootloader    sectors  0-7  64 KB  */
#define BL_ACTIVE_ADDR          0x08010000U               /* Active image  sectors  8-29 176 KB */
#define BL_ACTIVE_SIZE          (22U * BL_SECTOR_SIZE)    /* 176 KB                             */
#define BL_TAG_ADDR             0x0803C000U               /* sector 30 — CRC-32 tag of slot     */
#define BL_TAG_BLANK            0xFFFFFFFFU               /* erased tag word = slot not sealed  */

/* RAM window used to sanity-check the application's initial MSP */
#define BL_RAM_START            0x20000000U
#define BL_RAM_END              0x20020000U               /* 128 KB SRAM                        */

/* ── Update protocol (USART2 / ST-LINK VCP, 115200 8N1) ──────────────────
 *   host -> 'u'                       board -> ACK
 *   host -> image size (u32 LE)       board -> erase slot + tag, ACK
 *   host -> data chunk (<=256 bytes)  board -> program, ACK   (repeat)
 *   host -> CRC-32 of padded slot     board -> verify, write tag, ACK
 * Any failure is answered with NACK and the update is aborted.           */
#define BL_CMD_UPDATE           'u'
#define BL_ACK                  0x79U                     /* same values as ST AN3155           */
#define BL_NACK                 0x1FU
#define BL_CHUNK_SIZE           256U                      /* multiple of the 16-byte flash word */
#define BL_WAIT_MS              3000U                     /* update window after reset          */
#define BL_BYTE_TIMEOUT_MS      2000U                     /* inter-byte timeout during update   */

/* ── Status codes ─────────────────────────────────────────────────── */
typedef enum {
    BL_OK            = 0,   /* operation completed                        */
    BL_ERR_TIMEOUT   = 1,   /* host stopped sending                       */
    BL_ERR_SIZE      = 2,   /* image size is 0 or larger than the slot    */
    BL_ERR_FLASH     = 3,   /* erase / program / read-back failure        */
    BL_ERR_CRC       = 4,   /* slot CRC does not match the tag            */
    BL_ERR_NO_TAG    = 5,   /* slot has never been sealed                 */
    BL_ERR_NO_IMAGE  = 6,   /* vector table does not look like an image   */
} BL_Status;

/* ── Handle ───────────────────────────────────────────────────────── */
typedef struct {
    USART_HandleType    *huart;   /* console + update link                  */
    FLASH_HandleTypeDef  hflash;  /* owns the FLASH controller              */
    CRC_HandleTypeDef    hcrc;    /* owns the hardware CRC peripheral       */
} BL_HandleTypeDef;

/* ── API ─────────────────────────────────────────────────────────── */

/* Bind the UART, initialise FLASH + CRC and start the DWT cycle counter */
void BL_Init(BL_HandleTypeDef *hbl, USART_HandleType *huart);

/* Returns 1 when the host sent BL_CMD_UPDATE within timeout_ms, else 0 */
uint8_t BL_WaitForUpdateRequest(BL_HandleTypeDef *hbl, uint32_t timeout_ms);

/*
 * Receive a new application into the active slot (protocol above):
 *   1. Read image size, erase sectors 8-30 (slot + tag)
 *   2. Program each chunk as 128-bit flash words, read back to verify
 *   3. Compare host CRC with hardware CRC over the full 176 KB slot
 *   4. Match → write the CRC tag to sector 30
 * Leaves the slot unsealed (no tag) on any failure, so it is never booted.
 */
BL_Status BL_ReceiveImage(BL_HandleTypeDef *hbl);

/*
 * Check the active slot before booting:
 *   1. Tag present at BL_TAG_ADDR
 *   2. CRC-32(slot) == tag
 *   3. Initial MSP points into SRAM, Reset_Handler points into the slot
 */
BL_Status BL_VerifyActiveSlot(BL_HandleTypeDef *hbl);

/* Release peripherals, relocate VTOR and branch to the image. Never returns. */
void BL_JumpToApplication(BL_HandleTypeDef *hbl, uint32_t app_base);

const char *BL_StatusString(BL_Status st);

#endif /* __BOOTLOADER_H */
