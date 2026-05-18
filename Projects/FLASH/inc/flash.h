#ifndef __FLASH_H
#define __FLASH_H

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "../../../Drivers/NVIC/inc/nvic.h"
#include "../../../Drivers/UART/inc/uart.h"

#define __IO volatile

#define FLASH_BASE_ADDR 0x40022000U

/* ==== FLASH REGISTERS ==== */
typedef struct
{
    __IO uint32_t ACR;          /* 0x000 - Access control register   */
    __IO uint32_t KEYR;         /* 0x004 - Flash key register        */
         uint32_t RESERVED0;   /* 0x008                              */
    __IO uint32_t OPTKEYR;      /* 0x00C - Option key register       */
         uint32_t RESERVED1[2];/* 0x010-0x014                        */
    __IO uint32_t OPSR;         /* 0x018 - Operation status register */
    __IO uint32_t OPTCR;        /* 0x01C - Option control register   */
    __IO uint32_t SR;           /* 0x020 - Status register           */
         uint32_t RESERVED2;   /* 0x024                              */
    __IO uint32_t CR;           /* 0x028 - Control register          */
} FLASH_TypeDef;

#define FLASH ((FLASH_TypeDef*)FLASH_BASE_ADDR)

/* ==== FLASH CONTROL BITS ==== */
#define FLASH_CR_LOCK       (1U << 0)   /* Configuration lock             */
#define FLASH_CR_PG         (1U << 1)   /* Programming enable             */
#define FLASH_CR_SER        (1U << 2)   /* Sector erase                   */
#define FLASH_CR_BER        (1U << 3)   /* Bank erase                     */
#define FLASH_CR_FW         (1U << 4)   /* Force write (flush buffer)     */
#define FLASH_CR_STRT       (1U << 5)   /* Start erase                    */
#define FLASH_CR_SNB_SHIFT  6
#define FLASH_CR_SNB_MASK   (0x3FU << FLASH_CR_SNB_SHIFT)  /* Sector number [11:6] */
#define FLASH_CR_BKSEL      (1U << 31)  /* Bank select (0=bank1, 1=bank2) */

/* ==== FLASH STATUS BITS ==== */
#define FLASH_SR_BSY        (1U << 0)   /* Busy flag                      */
#define FLASH_SR_WBNE       (1U << 1)   /* Write buffer not empty         */

/* ==== FLASH GEOMETRY ==== */
#define FLASH_BANK1_BASE    0x08000000U
#define FLASH_BANK2_BASE    0x08040000U
#define FLASH_SECTOR_SIZE   0x00002000U  /* 8 KB per sector */

/* ==== FLASH KEYS ==== */
#define FLASH_KEY1 0x45670123U
#define FLASH_KEY2 0xCDEF89ABU

/* ==== STATUS ==== */
typedef enum {
    FLASH_OK = 0,
    FLASH_ERROR
} FLASH_StatusTypeDef;

/* ==== HANDLE ==== */
typedef struct {
    FLASH_TypeDef *Instance;
    uint32_t last_error;
} FLASH_HandleTypeDef;


/* ==== API ==== */
void FLASH_Init(FLASH_HandleTypeDef *hflash);
FLASH_StatusTypeDef FLASH_Unlock(FLASH_HandleTypeDef *hflash);
FLASH_StatusTypeDef FLASH_Lock(FLASH_HandleTypeDef *hflash);
FLASH_StatusTypeDef FLASH_ProgramWord(FLASH_HandleTypeDef *hflash, uint32_t address, uint32_t data);
FLASH_StatusTypeDef FLASH_ErasePage(FLASH_HandleTypeDef *hflash, uint32_t page_address);
    
#endif /* __FLASH_H */