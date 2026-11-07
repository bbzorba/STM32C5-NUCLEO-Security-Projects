#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "uart.h"     /* console + firmware update link (USART2), pulls in gpio.h */
#include "crc.h"      /* hardware CRC-32 (active slot integrity)   */
#include "flash.h"    /* sector erase / unlock / lock              */
#include "usb_cdc.h"  /* USB CDC ACM update link (USB-C user port) */

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
#define BL_FLASH_WORD_SIZE      16U                       /* 128-bit flash programming granularity */

/* RAM window used to sanity-check the application's initial MSP */
#define BL_RAM_START            0x20000000U
#define BL_RAM_END              0x20020000U               /* 128 KB SRAM                        */

/* ── Update protocol — same bytes on both links ─────────────────────────
 *   UART : USART2 / ST-LINK VCP, 115200 8N1
 *   USB  : CDC ACM on the USB-C user connector (PA11/PA12), any baud
 *   host -> 'u'                       board -> ACK
 *   host -> image size (u32 LE)       board -> erase slot + tag, ACK
 *   host -> data chunk (<=256 bytes)  board -> program, ACK   (repeat)
 *   host -> CRC-32 of padded slot     board -> verify, write tag, ACK
 * Any failure is answered with NACK and the update is aborted.           */
#define BL_CMD_UPDATE           'u'
#define BL_ACK                  0x79U                     /* same values as ST AN3155           */
#define BL_NACK                 0x1FU
#define BL_CHUNK_SIZE           256U                      /* multiple of the 16-byte flash word */
#define BL_BYTE_TIMEOUT_MS      2000U                     /* inter-byte timeout during update   */
#define BL_USART_CR1_FIFOEN     (1UL << 29)               /* USART CR1.FIFOEN: 8-byte RX FIFO   */

/* ── Entry into update mode ─────────────────────────────────────────────
 *   - every reset opens a BL_WAIT_MS window: 'u' on USART2 or USB CDC starts
 *     an update on that link (replies go back on the same link),
 *     pressing B1 cancels the auto-boot and stays in the bootloader;
 *     opening the USB COM port re-sends READY and grants BL_USB_OPEN_GRACE_MS
 *   - B1 held during reset: stay in the bootloader until an update arrives
 *   - no valid application: stay in the bootloader until an update arrives
 * LD1 is solid on during the window and blinks while the bootloader stays. */
#define BL_WAIT_MS              5000U                     /* update window after reset          */
#define BL_WAIT_FOREVER         0xFFFFFFFFU               /* stay until an update request       */
#define BL_POLL_MS              10U                       /* UART poll slice inside the window  */
#define BL_LED_BLINK_MS         250U                      /* LD1 half-period while staying      */
#define BL_READY_REPEAT_MS      1000U                     /* re-send "[BL] READY" while staying */
#define BL_USB_OPEN_GRACE_MS    3000U                     /* window left after the USB port opens */
#define BL_USB_READY_DELAY_MS   150U                      /* first READY after the USB port opens */
#define BL_USB_DRAIN_MS         50U                       /* let the host read the last USB bytes */
#define BL_BTN_SETTLE_MS        10U                       /* let the B1 pull-down / RC filter settle */

#define BL_BTN_PORT             GPIO_C                    /* B1 user button (blue)              */
#define BL_BTN_PIN              GPIO_PIN_13               /*   PC13, active HIGH: the MB2213     */
#define BL_BTN_PULL             GPIO_PULLDOWN             /*   board pulls it down, B1 drives   */
#define BL_BTN_ACTIVE           GPIO_PIN_SET              /*   it to VDD (measured over SWD)    */
#define BL_LED_PORT             GPIO_A                    /* LD1 user LED (green)               */
#define BL_LED_PIN              GPIO_PIN_5                /*   PA5, active high                 */

/* ── Optional USB VBUS sensing ─────────────────────────────────────────
 * The USB_DRD_FS device has no VBUS detector of its own; host presence is
 * seen through enumeration (USB_CDC_IsMounted) and the COM port being opened
 * (DTR) instead. If the board routes VBUS to a GPIO (through a divider), set
 * BL_USB_VBUS_SENSE to 1 and give that pin: the D+ pull-up is then only
 * enabled while VBUS is high (USB spec rule for self-powered devices).
 * Off by default — the NUCLEO-C562RE (MB2213) VBUS-sense pin is not confirmed. */
#ifndef BL_USB_VBUS_SENSE
#define BL_USB_VBUS_SENSE       0
#endif
/* #define BL_USB_VBUS_PORT     GPIO_x */
/* #define BL_USB_VBUS_PIN      GPIO_PIN_n */

/* ── Cortex-M33 core registers ─────────────────────────────────────── */
#define DEMCR_REG               (*(volatile uint32_t *)0xE000EDFCU)
#define DEMCR_TRCENA            (1U << 24)
#define DWT_CTRL_REG            (*(volatile uint32_t *)0xE0001000U)
#define DWT_CTRL_CYCCNTENA      (1U << 0)
#define DWT_CYCCNT_REG          (*(volatile uint32_t *)0xE0001004U)
#define SCB_VTOR_REG            (*(volatile uint32_t *)0xE000ED08U)
#define SYST_CSR_REG            (*(volatile uint32_t *)0xE000E010U)
#define NVIC_ICPR_BASE          ((volatile uint32_t *)0xE000E280U)   /* Clear Pending (ICER/ISER in core_cm4.h) */
#define NVIC_REG_COUNT          (NVIC_IRQ_COUNT / 32U)

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

/* ── What ended an update window ──────────────────────────────────── */
typedef enum {
    BL_TRIGGER_NONE   = 0,  /* window expired, nothing requested          */
    BL_TRIGGER_UART   = 1,  /* host sent BL_CMD_UPDATE on USART2          */
    BL_TRIGGER_BUTTON = 2,  /* B1 pressed — stay in the bootloader        */
    BL_TRIGGER_USB    = 3,  /* host sent BL_CMD_UPDATE on USB CDC         */
} BL_Trigger;

/* ── Link the update runs on ──────────────────────────────────────── */
typedef enum {
    BL_LINK_UART = 0,       /* USART2 — ST-LINK virtual COM port          */
    BL_LINK_USB  = 1,       /* USB CDC ACM — USB-C user connector         */
} BL_Link;

/* ── Handle ───────────────────────────────────────────────────────── */
typedef struct {
    USART_HandleType    *huart;   /* console + UART update link             */
    BL_Link              link;    /* link of the running update             */
    FLASH_HandleTypeDef  hflash;  /* owns the FLASH controller              */
    CRC_HandleTypeDef    hcrc;    /* owns the hardware CRC peripheral       */
    GPIO_HandleTypeDef   hbtn;    /* B1 — bootloader-mode request           */
    GPIO_InitTypeDef     btn_cfg;
    GPIO_HandleTypeDef   hled;    /* LD1 — bootloader-mode indicator        */
    GPIO_InitTypeDef     led_cfg;
#if BL_USB_VBUS_SENSE
    GPIO_HandleTypeDef   hvbus;   /* USB VBUS sense input                   */
    GPIO_InitTypeDef     vbus_cfg;
    uint8_t              vbus;    /* last VBUS state                        */
#endif
} BL_HandleTypeDef;

/* ── API ─────────────────────────────────────────────────────────── */

/* Bind the UART, initialise FLASH + CRC + B1 + LD1 + USB CDC, start the DWT counter */
void BL_Init(BL_HandleTypeDef *hbl, USART_HandleType *huart);

/* 1 while B1 is held down */
uint8_t BL_ButtonPressed(BL_HandleTypeDef *hbl);

/*
 * Wait for an update request on USART2 and USB CDC.
 *   timeout_ms != BL_WAIT_FOREVER : LD1 on; returns UART / USB on 'u' (and sets
 *                                   hbl->link), BUTTON when B1 is pressed,
 *                                   NONE when the window ends
 *   timeout_ms == BL_WAIT_FOREVER : LD1 blinks, "[BL] READY" repeats every
 *                                   BL_READY_REPEAT_MS; returns only on 'u' (UART / USB)
 */
BL_Trigger BL_WaitForUpdateRequest(BL_HandleTypeDef *hbl, uint32_t timeout_ms);

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

/* Console line on USART2, and on USB CDC while its COM port is open */
void BL_Log(BL_HandleTypeDef *hbl, const char *str);

#endif /* __BOOTLOADER_H */
