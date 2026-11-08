#ifndef __BL_REQUEST_H
#define __BL_REQUEST_H

/*
 * Application → bootloader handoff: "reset into the bootloader and stay there".
 *
 * The request is a magic word in TAMP backup register 0. Backup registers keep
 * their value across a system reset but are cleared when power is removed, so
 * a stale request cannot survive a power cycle. SRAM is not used: the
 * SRAM1_RST / SRAM2_RST option bytes may erase it on every system reset.
 *
 * Applications link this file by listing Bootloader in their module.mk DEPS.
 */

#include <stdint.h>

#define BL_REQUEST_MAGIC   0xB007AB1EU   /* TAMP->BKP0R value: stay in the bootloader */

/* Application: store the request and reset the MCU. Never returns. */
void    BL_RequestBootloader(void);

/* Bootloader: 1 if an application requested the bootloader. Clears the request. */
uint8_t BL_TakeRequest(void);

#endif /* __BL_REQUEST_H */
