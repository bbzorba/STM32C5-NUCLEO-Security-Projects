#ifndef TUSB_CONFIG_H_
#define TUSB_CONFIG_H_

/* TinyUSB configuration for Drivers/USB_CDC (one CDC ACM interface) */

/* Selects the CMSIS device header pulled in by the stm32_fsdev port */
#ifndef STM32C562xx
#define STM32C562xx
#endif

#define CFG_TUSB_MCU              OPT_MCU_STM32C5
#define CFG_TUSB_OS               OPT_OS_NONE      /* bare metal, USB_CDC_Task() polls */
#define CFG_TUSB_DEBUG            0

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN        __attribute__ ((aligned(4)))

/* Device role only, full speed (USB_DRD_FS) */
#define CFG_TUD_ENABLED           1
#define CFG_TUD_MAX_SPEED         OPT_MODE_FULL_SPEED
#define CFG_TUD_ENDPOINT0_SIZE    64

/* Classes */
#define CFG_TUD_CDC               1
#define CFG_TUD_MSC               0
#define CFG_TUD_HID               0
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            0

/* CDC FIFOs: RX holds a full 256-byte bootloader chunk + the next header */
#define CFG_TUD_CDC_NOTIFY        1
#define CFG_TUD_CDC_RX_BUFSIZE    512
#define CFG_TUD_CDC_TX_BUFSIZE    256
#define CFG_TUD_CDC_RX_EPSIZE     64
#define CFG_TUD_CDC_TX_EPSIZE     64

#endif /* TUSB_CONFIG_H_ */
