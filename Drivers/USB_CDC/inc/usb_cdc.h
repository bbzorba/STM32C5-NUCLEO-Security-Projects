#ifndef __USB_CDC_H
#define __USB_CDC_H

/*
 * USB CDC ACM (virtual COM port) on the USB_DRD_FS device of the STM32C562
 * — NUCLEO-C562RE USB Type-C user connector (PA11 = USB_DM, PA12 = USB_DP,
 * routed to the connector on the board, no wiring and no GPIO set-up needed).
 *
 * Bare-metal wrapper around TinyUSB (Drivers/TinyUSB submodule):
 *   - 48 MHz USB kernel clock = HSI 144 MHz / 3 (HSIDIV3), trimmed by the
 *     CRS from the host's USB start-of-frame packets — no crystal needed
 *   - USB_DRD_FS_IRQHandler forwards to TinyUSB; everything else is polled
 *     through USB_CDC_Task()
 *
 * This header is deliberately free of ST / CMSIS includes so it can be used
 * next to the register-level drivers of this repository.
 */

#include <stdint.h>

/* USB identity (TinyUSB test VID — fine for development, not for products) */
#define USB_CDC_VID             0xCAFEU
#define USB_CDC_PID             0xB0C5U
#define USB_CDC_MANUFACTURER    "STM32C5-NUCLEO-Security-Projects"
#define USB_CDC_PRODUCT         "C562 Bootloader CDC"

#define USB_CDC_WRITE_TIMEOUT_MS  100U   /* give up when the host stops reading */

/* Clocks (HSIDIV3 + CRS), TinyUSB device stack, D+ pull-up and USB IRQ */
void     USB_CDC_Init(void);

/* Remove the D+ pull-up (host sees an unplug) and stop USB + CRS completely */
void     USB_CDC_Deinit(void);

/* Attach / detach the D+ pull-up without stopping the stack */
void     USB_CDC_Connect(void);
void     USB_CDC_Disconnect(void);

/* Run the TinyUSB device task — call often (every few ms at least) */
void     USB_CDC_Task(void);

/* 1 once the host has enumerated and configured the device */
uint8_t  USB_CDC_IsMounted(void);

/* 1 while a host program has the COM port open (DTR set) */
uint8_t  USB_CDC_IsOpen(void);

/* 1 once per closed→open transition of the COM port (consumes the event) */
uint8_t  USB_CDC_TakeOpenEvent(void);

/* Non-blocking: 1 and *byte filled when a byte was received, else 0 */
uint8_t  USB_CDC_ReadByte(uint8_t *byte);

/* Discard everything received so far */
void     USB_CDC_FlushRx(void);

/* Queue + send (blocking up to USB_CDC_WRITE_TIMEOUT_MS); dropped when not open */
void     USB_CDC_Write(const uint8_t *data, uint32_t len);
void     USB_CDC_WriteString(const char *str);

/* Milliseconds since USB_CDC_Init (DWT cycle counter based) */
uint32_t USB_CDC_Millis(void);

#endif /* __USB_CDC_H */
