/*
 * usb_cdc.c — bare-metal glue between this repository and TinyUSB.
 *
 * Register sequence follows ST's STM32CubeC5 NUCLEO-C562RE USBX CDC example
 * (mx_rcc / mx_crs / mx_usb_drd_fs) and TinyUSB's hw/bsp/stm32c5:
 *   1. HSIDIV3 on                → 48 MHz from the 144 MHz HSI
 *   2. CK48 kernel clock = HSIDIV3
 *   3. USB + CRS bus clocks on
 *   4. CRS: SYNC = USB SOF, auto-trim on — keeps the HSI within USB tolerance
 *   5. tusb_init() → TinyUSB enables the D+ pull-up and the USB IRQ
 * PA11/PA12 need no GPIO configuration: the USB PHY takes them over.
 */
#include "tusb.h"
#include "stm32c5xx.h"
#include "usb_cdc.h"

#define USB_RHPORT          0U

/* DWT cycle counter (also used by the bootloader for its timeouts) */
#define DEMCR_REG           (*(volatile uint32_t *)0xE000EDFCU)
#define DEMCR_TRCENA        (1UL << 24)
#define DWT_CTRL_REG        (*(volatile uint32_t *)0xE0001000U)
#define DWT_CTRL_CYCCNTENA  (1UL << 0)
#define DWT_CYCCNT_REG      (*(volatile uint32_t *)0xE0001004U)

static volatile uint8_t  s_open_event;
static volatile uint8_t  s_touch_event;
static volatile uint32_t s_bitrate;

/* ── Time base for TinyUSB (required with CFG_TUSB_OS = OPT_OS_NONE) ── */
uint32_t tusb_time_millis_api(void)
{
    /* Accumulate so the result keeps counting past the 32-bit CYCCNT wrap */
    static uint32_t last_cyc, ms, rem;
    uint32_t cyc_per_ms = SystemCoreClock / 1000U;
    uint32_t now = DWT_CYCCNT_REG;

    rem      += now - last_cyc;
    last_cyc  = now;
    ms       += rem / cyc_per_ms;
    rem      %= cyc_per_ms;
    return ms;
}

uint32_t USB_CDC_Millis(void)
{
    return tusb_time_millis_api();
}

/* ── Interrupt: USB_DRD_FS (IRQn 62) ───────────────────────────────── */
void USB_DRD_FS_IRQHandler(void)
{
    tusb_int_handler(USB_RHPORT, true);
}

/* ── TinyUSB callbacks ─────────────────────────────────────────────── */
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)itf;
    (void)rts;
    if (dtr)
        s_open_event = 1U;   /* host program opened the COM port */
    else if (s_bitrate == USB_CDC_TOUCH_BAUD)
        s_touch_event = 1U;  /* ... and closed it again at 1200 baud */
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_line_coding)
{
    (void)itf;
    s_bitrate = p_line_coding->bit_rate;
}

/* ── Clocks ────────────────────────────────────────────────────────── */
static void usb_clock_init(void)
{
    RCC->CR1 |= RCC_CR1_HSIDIV3ON;
    while (!(RCC->CR1 & RCC_CR1_HSIDIV3RDY)) {}

    RCC->CCIPR2 = (RCC->CCIPR2 & ~RCC_CCIPR2_CK48SEL) | RCC_CCIPR2_CK48SEL_1;   /* CK48 = HSIDIV3 */

    RCC->APB2ENR  |= RCC_APB2ENR_USBEN;
    RCC->APB1LENR |= RCC_APB1LENR_CRSEN;
    (void)RCC->APB1LENR;                       /* clock enable takes effect */

    /* Trim the HSI from the 1 kHz USB SOF (reset reload/limit values fit 48 MHz) */
    CRS->CFGR = (CRS->CFGR & ~CRS_CFGR_SYNCSRC) | CRS_CFGR_SYNCSRC_1;
    CRS->CR  |= CRS_CR_AUTOTRIMEN | CRS_CR_CEN;
}

/* ── Public API ────────────────────────────────────────────────────── */
void USB_CDC_Init(void)
{
    DEMCR_REG    |= DEMCR_TRCENA;
    DWT_CTRL_REG |= DWT_CTRL_CYCCNTENA;

    usb_clock_init();

    const tusb_rhport_init_t dev_init = {
        .role  = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO,
    };
    tusb_init(USB_RHPORT, &dev_init);
}

void USB_CDC_Deinit(void)
{
    tud_disconnect();                          /* D+ pull-up off: host sees an unplug */
    NVIC_DisableIRQ(USB_DRD_FS_IRQn);
    NVIC_ClearPendingIRQ(USB_DRD_FS_IRQn);

    /* Reset + gate USB and CRS so the application starts from reset state */
    RCC->APB2RSTR  |=  RCC_APB2RSTR_USBRST;
    RCC->APB2RSTR  &= ~RCC_APB2RSTR_USBRST;
    RCC->APB1LRSTR |=  RCC_APB1LRSTR_CRSRST;
    RCC->APB1LRSTR &= ~RCC_APB1LRSTR_CRSRST;
    RCC->APB2ENR   &= ~RCC_APB2ENR_USBEN;
    RCC->APB1LENR  &= ~RCC_APB1LENR_CRSEN;
}

void USB_CDC_Connect(void)    { tud_connect(); }
void USB_CDC_Disconnect(void) { tud_disconnect(); }

void USB_CDC_Task(void)
{
    tud_task();
}

uint8_t USB_CDC_IsMounted(void)
{
    return tud_mounted() ? 1U : 0U;
}

uint8_t USB_CDC_IsOpen(void)
{
    return tud_cdc_connected() ? 1U : 0U;
}

uint8_t USB_CDC_TakeOpenEvent(void)
{
    uint8_t ev = s_open_event;
    s_open_event = 0U;
    return ev;
}

uint8_t USB_CDC_TakeTouchEvent(void)
{
    uint8_t ev = s_touch_event;
    s_touch_event = 0U;
    return ev;
}

uint8_t USB_CDC_ReadByte(uint8_t *byte)
{
    if (tud_cdc_available() == 0U)
        return 0U;
    int32_t c = tud_cdc_read_char();
    if (c < 0)
        return 0U;
    *byte = (uint8_t)c;
    return 1U;
}

void USB_CDC_FlushRx(void)
{
    tud_cdc_read_flush();
}

void USB_CDC_Write(const uint8_t *data, uint32_t len)
{
    uint32_t start = tusb_time_millis_api();

    while (len > 0U && tud_cdc_connected()) {
        uint32_t n = tud_cdc_write(data, len);
        data += n;
        len  -= n;
        tud_cdc_write_flush();
        tud_task();
        if ((tusb_time_millis_api() - start) > USB_CDC_WRITE_TIMEOUT_MS)
            return;                            /* host stopped reading */
    }
    tud_cdc_write_flush();
    tud_task();
}

void USB_CDC_WriteString(const char *str)
{
    uint32_t len = 0U;
    while (str[len] != '\0')
        len++;
    USB_CDC_Write((const uint8_t *)str, len);
}
