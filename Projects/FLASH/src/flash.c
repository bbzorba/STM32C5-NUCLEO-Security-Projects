#include "../inc/flash.h"

/* ==== INIT ==== */
void FLASH_Init(FLASH_HandleTypeDef *hflash) {
    hflash->Instance = FLASH;
    hflash->last_error = 0;
}

/* ==== UNLOCK ==== */
FLASH_StatusTypeDef FLASH_Unlock(FLASH_HandleTypeDef *hflash) {
    if (!(hflash->Instance->CR & FLASH_CR_LOCK))
        return FLASH_OK;

    hflash->Instance->KEYR = FLASH_KEY1;
    hflash->Instance->KEYR = FLASH_KEY2;

    if (hflash->Instance->CR & FLASH_CR_LOCK)
        return FLASH_ERROR;

    return FLASH_OK;
}

/* ==== LOCK ==== */
FLASH_StatusTypeDef FLASH_Lock(FLASH_HandleTypeDef *hflash) {
    hflash->Instance->CR |= FLASH_CR_LOCK;

    return FLASH_OK;
}

/* ==== PROGRAM WORD ==== */
FLASH_StatusTypeDef FLASH_ProgramWord(FLASH_HandleTypeDef *hflash, uint32_t address, uint32_t data) {
    while (hflash->Instance->SR & FLASH_SR_BSY);

    hflash->Instance->CR |= FLASH_CR_PG;

    *(volatile uint32_t*)address = data;

    /* Force write: flush the 32-bit word from the write buffer without
       waiting for a full 128-bit quad-word. */
    hflash->Instance->CR |= FLASH_CR_FW;

    /* Wait until the write buffer is empty and the operation is done. */
    while (hflash->Instance->SR & (FLASH_SR_BSY | FLASH_SR_WBNE));

    hflash->Instance->CR &= ~FLASH_CR_PG;

    return FLASH_OK;
}

/* ==== ERASE SECTOR ==== */
FLASH_StatusTypeDef FLASH_ErasePage(FLASH_HandleTypeDef *hflash, uint32_t page_address) {
    uint32_t bksel, sector_num, cr;

    while (hflash->Instance->SR & FLASH_SR_BSY);

    /* Compute sector number and bank from address (8 KB sectors). */
    if (page_address >= FLASH_BANK2_BASE) {
        bksel = 1U;
        sector_num = (page_address - FLASH_BANK2_BASE) / FLASH_SECTOR_SIZE;
    } else {
        bksel = 0U;
        sector_num = (page_address - FLASH_BANK1_BASE) / FLASH_SECTOR_SIZE;
    }

    /* Configure sector erase: set SER, SNB, BKSEL. */
    cr = hflash->Instance->CR;
    cr &= ~(FLASH_CR_BER | FLASH_CR_SER | FLASH_CR_SNB_MASK | FLASH_CR_BKSEL);
    cr |= FLASH_CR_SER | (sector_num << FLASH_CR_SNB_SHIFT) | (bksel ? FLASH_CR_BKSEL : 0U);
    hflash->Instance->CR = cr;

    /* Start erase. */
    hflash->Instance->CR |= FLASH_CR_STRT;

    while (hflash->Instance->SR & FLASH_SR_BSY);

    /* Clear erase configuration. */
    hflash->Instance->CR &= ~(FLASH_CR_SER | FLASH_CR_SNB_MASK | FLASH_CR_BKSEL);

    return FLASH_OK;
}