/*
 * USB descriptors for Drivers/USB_CDC — one CDC ACM function (IAD).
 * Serial number string = the STM32's 96-bit unique ID, so every board gets
 * its own COM port identity on the host.
 */
#include "tusb.h"
#include "stm32c5xx.h"      /* UID_BASE */
#include "usb_cdc.h"

/* ── Device descriptor ─────────────────────────────────────────────── */
static tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    /* IAD: required for CDC so hosts bind both interfaces to one function */
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_CDC_VID,
    .idProduct          = USB_CDC_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

/* ── Configuration descriptor ──────────────────────────────────────── */
enum { ITF_NUM_CDC = 0, ITF_NUM_CDC_DATA, ITF_NUM_TOTAL };

#define EPNUM_CDC_NOTIF     0x81
#define EPNUM_CDC_OUT       0x02
#define EPNUM_CDC_IN        0x82
#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

static uint8_t const desc_configuration[] = {
    /* config number, interface count, string index, total length, attributes, power (mA) */
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    /* interface number, string index, notification EP + size, data EP out/in + size */
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

/* ── String descriptors ────────────────────────────────────────────── */
enum { STRID_LANGID = 0, STRID_MANUFACTURER, STRID_PRODUCT, STRID_SERIAL, STRID_CDC };

static char const *const string_desc[] = {
    [STRID_MANUFACTURER] = USB_CDC_MANUFACTURER,
    [STRID_PRODUCT]      = USB_CDC_PRODUCT,
    [STRID_CDC]          = "Bootloader port",
};

static uint16_t desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    uint8_t len = 0;

    if (index == STRID_LANGID) {
        desc_str[1] = 0x0409;                   /* English (US) */
        len = 1;
    } else if (index == STRID_SERIAL) {
        /* 96-bit unique ID as 24 hex characters */
        static const char hex[] = "0123456789ABCDEF";
        const uint8_t *uid = (const uint8_t *)UID_BASE;
        for (uint8_t i = 0; i < 12U; i++) {
            desc_str[1 + 2U * i]      = hex[uid[i] >> 4];
            desc_str[1 + 2U * i + 1U] = hex[uid[i] & 0x0FU];
        }
        len = 24;
    } else {
        if (index >= sizeof(string_desc) / sizeof(string_desc[0]) || string_desc[index] == NULL)
            return NULL;
        const char *str = string_desc[index];
        while (str[len] != '\0' && len < 32U) {
            desc_str[1 + len] = (uint16_t)str[len];
            len++;
        }
    }

    /* first word: length in bytes (incl. header) | descriptor type */
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2U * len + 2U));
    return desc_str;
}
