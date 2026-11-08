#include "bl_request.h"

/*
 * Register addresses from the STM32C562 device header (stm32c562xx.h). The
 * device header itself is not included: its RCC/GPIO definitions clash with
 * this repository's register-level headers.
 */
#define RCC_APB3ENR         (*(volatile uint32_t *)0x44020CA8U)   /* RCC_BASE + 0x0A8  */
#define RCC_APB3ENR_RTCAPBEN (1UL << 21)                          /* RTC + TAMP APB clock */
#define PWR_RTCCR           (*(volatile uint32_t *)0x44020824U)   /* PWR_BASE + 0x024  */
#define PWR_RTCCR_DRTCP     (1UL << 0)                            /* RTC domain write access */
#define TAMP_BKP0R          (*(volatile uint32_t *)0x44007D00U)   /* TAMP_BASE + 0x100 */
#define SCB_AIRCR           (*(volatile uint32_t *)0xE000ED0CU)
#define AIRCR_SYSRESETREQ   0x05FA0004U                           /* VECTKEY + SYSRESETREQ */

/* Clock the TAMP registers and open the RTC domain for writing */
static void bkp_access(void)
{
    RCC_APB3ENR |= RCC_APB3ENR_RTCAPBEN;
    (void)RCC_APB3ENR;                       /* clock enable takes effect */
    PWR_RTCCR |= PWR_RTCCR_DRTCP;
}

void BL_RequestBootloader(void)
{
    bkp_access();
    TAMP_BKP0R = BL_REQUEST_MAGIC;
    (void)TAMP_BKP0R;                        /* write reaches the backup domain */

    __asm volatile ("dsb" ::: "memory");
    SCB_AIRCR = AIRCR_SYSRESETREQ;
    __asm volatile ("dsb" ::: "memory");
    while (1) {}
}

uint8_t BL_TakeRequest(void)
{
    bkp_access();
    if (TAMP_BKP0R != BL_REQUEST_MAGIC)
        return 0U;
    TAMP_BKP0R = 0U;                         /* one request = one stay */
    return 1U;
}
