#include "../inc/flash.h"

/* ==== INIT ==== */
void FLASH_Init(FLASH_HandleTypeDef *hflash)
{
    hflash->Instance = FLASH;
    hflash->last_error = 0;
}

/* ==== UNLOCK ==== */
FLASH_StatusTypeDef FLASH_Unlock(
    FLASH_HandleTypeDef *hflash)
{
    if (!(hflash->Instance->CR & FLASH_CR_LOCK))
        return FLASH_OK;

    hflash->Instance->KEYR = FLASH_KEY1;
    hflash->Instance->KEYR = FLASH_KEY2;

    if (hflash->Instance->CR & FLASH_CR_LOCK)
        return FLASH_ERROR;

    return FLASH_OK;
}

/* ==== LOCK ==== */
FLASH_StatusTypeDef FLASH_Lock(
    FLASH_HandleTypeDef *hflash)
{
    hflash->Instance->CR |= FLASH_CR_LOCK;

    return FLASH_OK;
}

/* ==== PROGRAM WORD ==== */
FLASH_StatusTypeDef FLASH_ProgramWord(
    FLASH_HandleTypeDef *hflash,
    uint32_t address,
    uint32_t data)
{
    while (hflash->Instance->SR & FLASH_SR_BSY);

    hflash->Instance->CR |= FLASH_CR_PG;

    *(volatile uint32_t*)address = data;

    while (hflash->Instance->SR & FLASH_SR_BSY);

    hflash->Instance->CR &= ~FLASH_CR_PG;

    return FLASH_OK;
}

/* ==== ERASE PAGE ==== */
FLASH_StatusTypeDef FLASH_ErasePage(
    FLASH_HandleTypeDef *hflash,
    uint32_t page_address)
{
    while (hflash->Instance->SR & FLASH_SR_BSY);

    hflash->Instance->CR |= FLASH_CR_PER;

    *(volatile uint32_t*)page_address = 0xFFFFFFFF;

    hflash->Instance->CR |= FLASH_CR_STRT;

    while (hflash->Instance->SR & FLASH_SR_BSY);

    hflash->Instance->CR &= ~FLASH_CR_PER;

    return FLASH_OK;
}