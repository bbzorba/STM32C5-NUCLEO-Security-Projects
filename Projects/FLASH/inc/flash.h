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
    __IO uint32_t ACR;
    __IO uint32_t PDKEYR;
    __IO uint32_t KEYR;
    __IO uint32_t OPTKEYR;
    __IO uint32_t SR;
    __IO uint32_t CR;
    __IO uint32_t ECCR;
    __IO uint32_t OPTR;
} FLASH_TypeDef;

#define FLASH ((FLASH_TypeDef*)FLASH_BASE_ADDR)

/* ==== FLASH CONTROL BITS ==== */
#define FLASH_CR_PG        (1U << 0)
#define FLASH_CR_PER       (1U << 1)
#define FLASH_CR_STRT      (1U << 16)
#define FLASH_CR_LOCK      (1U << 31)

/* ==== FLASH STATUS BITS ==== */
#define FLASH_SR_BSY       (1U << 16)

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