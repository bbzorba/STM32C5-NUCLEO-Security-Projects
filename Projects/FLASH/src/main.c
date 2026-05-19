#include "../inc/flash.h"
#include "../../../Drivers/SysTick/inc/systick.h"
#include "../../../Drivers/UART/inc/uart.h"

#define KEY_STORAGE_ADDR 0x0803F000U

static void Test_FlashElapsedTime(FLASH_HandleTypeDef *hflash, USART_HandleType *huart, SysTick_HandleTypeDef *htick, char buffer[64]) {
    
    USART_WriteString(huart, "\r\nUnlocking FLASH...\r\n");
    SysTick_StartTimer(htick);
    FLASH_Unlock(hflash);
    uint32_t elapsed_time_us = SysTick_GetElapsedTime_us(htick);
    USART_WriteString(huart, "Elapsed (us): ");
    sprintf(buffer, "%lu", elapsed_time_us);
    USART_WriteString(huart, buffer);
    USART_WriteString(huart, "\r\n");

    USART_WriteString(huart, "\r\nErasing FLASH...\r\n");
    SysTick_StartTimer(htick);
    FLASH_ErasePage(hflash, KEY_STORAGE_ADDR);
    elapsed_time_us = SysTick_GetElapsedTime_us(htick);
    USART_WriteString(huart, "Elapsed (us): ");
    sprintf(buffer, "%lu", elapsed_time_us);
    USART_WriteString(huart, buffer);
    USART_WriteString(huart, "\r\n");

    USART_WriteString(huart, "\r\nProgramming FLASH...\r\n");
    SysTick_StartTimer(htick);
    FLASH_ProgramWord(hflash, KEY_STORAGE_ADDR, 0x12345678);
    elapsed_time_us = SysTick_GetElapsedTime_us(htick);
    USART_WriteString(huart, "Elapsed (us): ");
    sprintf(buffer, "%lu", elapsed_time_us);
    USART_WriteString(huart, buffer);
    USART_WriteString(huart, "\r\n");

    USART_WriteString(huart, "\r\nLocking FLASH...\r\n");
    SysTick_StartTimer(htick);
    FLASH_Lock(hflash);
    elapsed_time_us = SysTick_GetElapsedTime_us(htick);
    USART_WriteString(huart, "Elapsed (us): ");
    sprintf(buffer, "%lu", elapsed_time_us);
    USART_WriteString(huart, buffer);
    USART_WriteString(huart, "\r\n");

    USART_WriteString(huart, "\r\nReading a Byte from FLASH...\r\n");
    SysTick_StartTimer(htick);
    uint8_t byte_value = FLASH_ReadByte(KEY_STORAGE_ADDR);
    elapsed_time_us = SysTick_GetElapsedTime_us(htick);
    USART_WriteString(huart, "Read Byte: 0x");
    sprintf(buffer, "%02X", byte_value);
    USART_WriteString(huart, buffer);
    USART_WriteString(huart, "\r\nElapsed (us): ");
    sprintf(buffer, "%lu", elapsed_time_us);
    USART_WriteString(huart, buffer);
    USART_WriteString(huart, "\r\n");

    USART_WriteString(huart, "\r\nReading a Word from FLASH...\r\n");
    SysTick_StartTimer(htick);
    uint32_t word_value = FLASH_ReadWord(KEY_STORAGE_ADDR);
    elapsed_time_us = SysTick_GetElapsedTime_us(htick);
    USART_WriteString(huart, "Elapsed (us): ");
    sprintf(buffer, "%lu", elapsed_time_us);
    USART_WriteString(huart, buffer);
    USART_WriteString(huart, "\r\n");

    if (word_value == 0x12345678)
        USART_WriteString(huart, "FLASH WRITE & READ OK\r\n");
    else
        USART_WriteString(huart, "FLASH WRITE / READ FAILED\r\n");
}

static void Test_FlashSecurityFeatures(FLASH_HandleTypeDef *hflash, USART_HandleType *huart, char buffer[64]){
    /* ==== Security Feature Demo ==== */
    USART_WriteString(huart, "\r\n--- Security Features ---\r\n");

    /* Read current RDP level */
    uint8_t rdp = FLASH_ReadProtect(hflash);
    const char *rdp_name = (rdp == FLASH_RDP_LEVEL_0) ? "Level 0 (no protection)" :
                           (rdp == FLASH_RDP_LEVEL_1) ? "Level 1 (read protected)" :
                           (rdp == 0xCCU)             ? "Level 2 (IRREVERSIBLE!)" :
                                                         "Level 1+ (non-0xAA = read protected)";
    sprintf(buffer, "RDP level:    0x%02X  %s\r\n", rdp, rdp_name);
    USART_WriteString(huart, buffer);

    /* Read current write-protection bitmap (0 = protected, 1 = writable) */
    sprintf(buffer, "WRP1R_CUR:    0x%08lX\r\n", FLASH_WRP1R_CUR_REG);
    USART_WriteString(huart, buffer);

    /* Protect sector 31 (KEY_STORAGE sector) in bank 1 */
    FLASH_StatusTypeDef wp_status = FLASH_WriteProtect(hflash, 1U, 1U << 31);
    if (wp_status == FLASH_OK) {
        sprintf(buffer, "WRP1R_PRG:    0x%08lX  (bit31=0 -> sector 31 protected)\r\n",
                FLASH_WRP1R_PRG_REG);
        USART_WriteString(huart, buffer);
        USART_WriteString(huart, "WRITE PROTECT CONFIG OK\r\n");
    } else {
        USART_WriteString(huart, "WRITE PROTECT CONFIG FAILED\r\n");
    }
    USART_WriteString(huart, "(Activate: OPTSTRT + system reset)\r\n");

}

static void Test_FlashIntegrity(FLASH_HandleTypeDef *hflash, CRC_HandleTypeDef *hcrc, SysTick_HandleTypeDef *htick, USART_HandleType *huart, char buffer[64]) {
    USART_WriteString(huart, "\r\n--- Data Integrity Check ---\r\n");

    /* CRC region:  all of bank 1 (256 KB) — covers app code + key storage */
    /* CRC tag:     bank 2 sector 0 (0x08040000) — outside the measured region */
    const uint32_t region_addr = FLASH_BANK1_BASE;
    const uint32_t region_size = 256U * 1024U;
    const uint32_t tag_addr    = FLASH_BANK2_BASE;

    /* Step 1: compute CRC of bank 1 and store it in bank 2 sector 0 */
    SysTick_StartTimer(htick);
    FLASH_StatusTypeDef store_status = FLASH_StoreIntegrityTag(hflash, hcrc,
                                            region_addr, region_size, tag_addr);
    uint32_t elapsed = SysTick_GetElapsedTime_us(htick);
    sprintf(buffer, "Store tag: %s  Elapsed (us): %lu\r\n",
            store_status == FLASH_OK ? "OK" : "FAIL", elapsed);
    USART_WriteString(huart, buffer);

    if (store_status != FLASH_OK) {
        USART_WriteString(huart, "FLASH DATA INTEGRITY FAILED\r\n");
        return;
    }

    /* Step 2: read back the stored CRC word and unpack to byte array */
    uint32_t stored_word = FLASH_ReadWord(tag_addr);
    uint8_t  stored_crc[4] = {
        (uint8_t)(stored_word >> 24),
        (uint8_t)(stored_word >> 16),
        (uint8_t)(stored_word >>  8),
        (uint8_t)(stored_word)
    };
    sprintf(buffer, "CRC tag:   %02X%02X%02X%02X  @0x%08lX\r\n",
            stored_crc[0], stored_crc[1], stored_crc[2], stored_crc[3],
            (unsigned long)tag_addr);
    USART_WriteString(huart, buffer);

    /* Step 3: verify — re-compute CRC of bank 1 and compare to stored tag */
    SysTick_StartTimer(htick);
    FLASH_StatusTypeDef check_status = FLASH_CheckIntegrity(hcrc,
                                            region_addr, region_size, stored_crc);
    elapsed = SysTick_GetElapsedTime_us(htick);
    sprintf(buffer, "Verify:    %s  Elapsed (us): %lu\r\n",
            check_status == FLASH_OK ? "OK" : "FAIL", elapsed);
    USART_WriteString(huart, buffer);

    if (check_status == FLASH_OK)
        USART_WriteString(huart, "FLASH DATA INTEGRITY OK\r\n");
    else
        USART_WriteString(huart, "FLASH DATA INTEGRITY FAILED\r\n");
}

int main(void)
{
    FLASH_HandleTypeDef hflash;
    FLASH_Init(&hflash);

    USART_HandleType huart;
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&huart, "\r\nFLASH TEST\r\n");

    SysTick_HandleTypeDef htick;
    SysTick_constructor(&htick, SysTick, SYSTICK_OK);

    CRC_HandleTypeDef hcrc;
    CRC_Constructor(&hcrc);

    char buffer[64];

    Test_FlashElapsedTime(&hflash, &huart, &htick, buffer);
    Test_FlashSecurityFeatures(&hflash, &huart, buffer);
    Test_FlashIntegrity(&hflash, &hcrc, &htick, &huart, buffer);
    while (1){ }
}