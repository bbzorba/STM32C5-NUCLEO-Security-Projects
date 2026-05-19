#include "../inc/flash.h"

#define KEY_STORAGE_ADDR 0x0803F000U

int main(void)
{
    FLASH_HandleTypeDef hflash;
    FLASH_Init(&hflash);

    USART_HandleType huart;
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&huart, "\r\nFLASH TEST\r\n");

    USART_WriteString(&huart, "Unlocking FLASH...\r\n");
    FLASH_Unlock(&hflash);
    USART_WriteString(&huart, "\r\nErasing FLASH...\r\n");
    FLASH_ErasePage(&hflash, KEY_STORAGE_ADDR);
    USART_WriteString(&huart, "Programming FLASH...\r\n");
    FLASH_ProgramWord(&hflash, KEY_STORAGE_ADDR, 0x12345678);
    USART_WriteString(&huart, "Locking FLASH...\r\n");
    FLASH_Lock(&hflash);

    uint32_t value = *(volatile uint32_t*)KEY_STORAGE_ADDR;

    if (value == 0x12345678)
        USART_WriteString(&huart, "FLASH WRITE OK\r\n");
    else
        USART_WriteString(&huart, "FLASH WRITE FAILED\r\n");

    while (1){ }
}