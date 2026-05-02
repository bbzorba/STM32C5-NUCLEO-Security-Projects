#ifndef __CRC_H
#define __CRC_H

#include <stdint.h>
#include <stddef.h>
#include "../../GPIO/inc/gpio.h"   /* RCC_TypeDef / RCC */

#define __IO volatile

/* RCC clock enable for CRC (AHB1ENR bit 12) */
#define RCC_AHB1ENR_CRCEN  (1U << 12)

/* CRC peripheral base address */
#define CRC_BASE  0x40023000UL

/* CRC CR register bit definitions */
#define CRC_CR_RESET        (1U << 0)  /* Reset CRC calculation unit (auto-cleared) */
#define CRC_CR_POLYSIZE_32  (0U << 3)  /* 32-bit polynomial */
#define CRC_CR_REV_IN_BYTE  (1U << 5)  /* Reverse input bits by byte */
#define CRC_CR_REV_OUT      (1U << 7)  /* Reverse output bits */

/* CRC register map */
typedef struct {
    __IO uint32_t DR;     /* 0x000: Data register (write input / read result) */
    __IO uint32_t IDR;    /* 0x004: Independent data register */
    __IO uint32_t CR;     /* 0x008: Control register */
         uint32_t RSVD;  /* 0x00C: Reserved */
    __IO uint32_t INIT;   /* 0x010: Initial CRC value */
    __IO uint32_t POL;    /* 0x014: Polynomial */
} CRC_TypeDef;

#define CRC_PERIPH  ((CRC_TypeDef *)CRC_BASE)

typedef enum {
    CRC_OK = 0,
    CRC_ERROR
} CRC_StatusTypeDef;

typedef struct {
    CRC_TypeDef       *Instance;
    CRC_StatusTypeDef  state;
} CRC_HandleTypeDef;

/* Public API */
void CRC_Constructor(CRC_HandleTypeDef *hcrc);
CRC_StatusTypeDef CRC_Calculate(CRC_HandleTypeDef *hcrc, const uint8_t *data, size_t len, uint8_t *crc_out);

#endif /* __CRC_H */