#include "../inc/flash.h"
#include <stdint.h>

static volatile uint8_t s_flash_eop   = 0;
static volatile uint8_t s_flash_error = 0;

/* EOP interrupt: fired when an erase or program operation completes (end of operation). */
/* TODO: Write protection interrupt handling */
void FLASH_IRQHandler(void)
{
    uint32_t sr = FLASH->SR;
    /* Clear all pending flags via the write-only CCR register */
    FLASH_CCR_REG = FLASH_CCR_CLR_EOP | FLASH_CCR_CLR_WRPERR | FLASH_CCR_CLR_PGSERR;
    /* Disable EOP interrupt; re-armed per operation in FLASH_ErasePage */
    FLASH->CR &= ~FLASH_CR_EOPIE;
    if (sr & FLASH_SR_EOP)                             { s_flash_eop = 1; }
    if (sr & (FLASH_SR_WRPERR | FLASH_SR_PGSERR))      { s_flash_error = 1; s_flash_eop = 1; }
}

/* ==== INIT ==== */
void FLASH_Init(FLASH_HandleTypeDef *hflash) {
    hflash->Instance = FLASH;
    hflash->last_error = 0;
    /* Enable FLASH global interrupt in NVIC (IRQn = 5) */
    NVIC_SetPriority(FLASH_IRQn, 0);
    NVIC_EnableIRQ(FLASH_IRQn);
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

    /* Single-bank mode: BKSEL is always 0 (ignored by hardware anyway).
     * The physical write address is used directly. */
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

uint8_t FLASH_ReadByte(uint32_t address) {
    return *(volatile uint8_t*)address;
}

uint32_t FLASH_ReadWord(uint32_t address) {
    return *(volatile uint32_t*)address;
}

/* ==== ERASE SECTOR ==== */
FLASH_StatusTypeDef FLASH_ErasePage(FLASH_HandleTypeDef *hflash, uint32_t page_address) {
    uint32_t bksel, sector_num, cr;

    while (hflash->Instance->SR & FLASH_SR_BSY);

    /* STM32C562RE is always single-bank (DBANK=0 option byte).
     * BKSEL is ignored by hardware; SNB is the absolute sector number 0-63
     * computed from the start of flash.  Do NOT split on FLASH_BANK2_BASE —
     * that would give SNB=0 for sector 32+ and erase the wrong sector. */
    bksel = 0U;
    sector_num = (page_address - FLASH_BANK1_BASE) / FLASH_SECTOR_SIZE;

    /* Configure sector erase: set SER, SNB, BKSEL. */
    cr = hflash->Instance->CR;
    cr &= ~(FLASH_CR_BER | FLASH_CR_SER | FLASH_CR_SNB_MASK | FLASH_CR_BKSEL);
    cr |= FLASH_CR_SER | (sector_num << FLASH_CR_SNB_SHIFT) | (bksel ? FLASH_CR_BKSEL : 0U);
    hflash->Instance->CR = cr;

    /* Arm the EOP interrupt and start erase. */
    s_flash_eop   = 0;
    s_flash_error = 0;
    /* Clear any stale EOP/error flags before enabling the interrupt to avoid
       a spurious immediate trigger from a leftover flag set by a prior operation. */
    FLASH_CCR_REG = FLASH_CCR_CLR_EOP | FLASH_CCR_CLR_WRPERR | FLASH_CCR_CLR_PGSERR;
    hflash->Instance->CR |= FLASH_CR_EOPIE;  /* enable EOP interrupt */
    hflash->Instance->CR |= FLASH_CR_STRT;   /* kick off erase       */

    /* Wait for EOP via interrupt — CPU is free to do other work here. */
    while (!s_flash_eop);

    /* Clear erase configuration. */
    hflash->Instance->CR &= ~(FLASH_CR_SER | FLASH_CR_SNB_MASK | FLASH_CR_BKSEL);
    hflash->last_error = s_flash_error;

    return s_flash_error ? FLASH_ERROR : FLASH_OK;
}
