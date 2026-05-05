#include "../inc/dma.h"

static uint8_t srcBuffer[8] = {0x10U, 0x21U, 0x32U, 0x43U, 0x54U, 0x65U, 0x76U, 0x87U};
static uint8_t dstBuffer[8] = {0U};

static void print_dma_state(USART_HandleType *huart, DMA_HandleType *dmaHandle, uint8_t addressOk, uint8_t transferOk, uint8_t *dst, uint32_t size) {
    uint32_t index;

    USART_WriteString(huart, "addr=");
    uart_write_hex8(huart, addressOk);
    USART_WriteString(huart, " transfer=");
    uart_write_hex8(huart, transferOk);
    USART_WriteString(huart, " dbg=");
    uart_write_hex32(huart, g_dma_debug_point);
    USART_WriteString(huart, " sr=");
    uart_write_hex32(huart, dmaHandle->channel->SR);
    USART_WriteString(huart, " cr=");
    uart_write_hex32(huart, dmaHandle->channel->CR);
    USART_WriteString(huart, " tr1=");
    uart_write_hex32(huart, dmaHandle->channel->TR1);
    USART_WriteString(huart, " tr2=");
    uart_write_hex32(huart, dmaHandle->channel->TR2);
    USART_WriteString(huart, " br1=");
    uart_write_hex32(huart, dmaHandle->channel->BR1);
    USART_WriteString(huart, " dst=");
    for (index = 0U; index < size; index++) {
        uart_write_hex8(huart, dst[index]);
    }
    USART_WriteString(huart, "\n");
}

static void delay(volatile uint32_t ticks) {
    while (ticks--) {
        __asm volatile ("nop");
    }
}

int main(void) {
    uint8_t transferOk = 1U;
    uint8_t addressOk = 1U;
    uint32_t i;

    USART_HandleType huart;
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);

    DMA_HandleType dmaHandle;
    DMA_Constructor(&dmaHandle, LPDMA1_Channel1, LPDMA_MEMORY_TO_PERIPH);

    GPIO_InitTypeDef led_init;
    led_init.Mode = GPIO_MODE_OUTPUT_PP;
    led_init.Pin = GPIO_PIN_5;
    led_init.Pull = GPIO_NOPULL;
    led_init.Speed = GPIO_SPEED_MEDIUM;

    GPIO_HandleTypeDef ld1_green;
    GPIO_constructor(&ld1_green, GPIO_A, &led_init);

    if ((uintptr_t)&LPDMA1_Channel1->LBAR != (LPDMA1_BASE + 0x0D0U)) { addressOk = 0U; }
    if ((uintptr_t)&LPDMA1_Channel1->FCR  != (LPDMA1_BASE + 0x0DCU)) { addressOk = 0U; }
    if ((uintptr_t)&LPDMA1_Channel1->SR   != (LPDMA1_BASE + 0x0E0U)) { addressOk = 0U; }
    if ((uintptr_t)&LPDMA1_Channel1->CR   != (LPDMA1_BASE + 0x0E4U)) { addressOk = 0U; }
    if ((uintptr_t)&LPDMA1_Channel1->TR1  != (LPDMA1_BASE + 0x110U)) { addressOk = 0U; }
    if ((uintptr_t)&LPDMA1_Channel1->TR2  != (LPDMA1_BASE + 0x114U)) { addressOk = 0U; }
    if ((uintptr_t)&LPDMA1_Channel1->BR1  != (LPDMA1_BASE + 0x118U)) { addressOk = 0U; }
    if ((uintptr_t)&LPDMA1_Channel1->SAR  != (LPDMA1_BASE + 0x11CU)) { addressOk = 0U; }
    if ((uintptr_t)&LPDMA1_Channel1->DAR  != (LPDMA1_BASE + 0x120U)) { addressOk = 0U; }
    if ((uintptr_t)&LPDMA1_Channel1->LLR  != (LPDMA1_BASE + 0x14CU)) { addressOk = 0U; }

    if (DMA_Init(&dmaHandle) != LPDMA_OK) {
        transferOk = 0U;
    }

    if (transferOk && DMA_ConfigTransfer(&dmaHandle,
                                         (uintptr_t)srcBuffer,
                                         (uintptr_t)dstBuffer,
                                         (uint16_t)sizeof(srcBuffer),
                                         (LPDMA_TR1_SDW_LOG2_8 | LPDMA_TR1_DDW_LOG2_8 | LPDMA_TR1_SINC | LPDMA_TR1_DINC),
                                         LPDMA_CTR2_SWREQ) != LPDMA_OK) {
        transferOk = 0U;
    }

    if (transferOk && DMA_Start(&dmaHandle) != LPDMA_OK) {
        transferOk = 0U;
    }

    for (i = 0U; i < 200000U; i++) {
        if (DMA_IsTransferComplete(&dmaHandle)) {
            break;
        }
    }

    for (i = 0U; i < sizeof(srcBuffer); i++) {
        if (srcBuffer[i] != dstBuffer[i]) {
            transferOk = 0U;
            break;
        }
    }

    if (DMA_Stop(&dmaHandle) != LPDMA_OK) {
        transferOk = 0U;
    }

    while (1) {
        if (addressOk && transferOk) {
            GPIO_TogglePin(&ld1_green, GPIO_PIN_5);
            USART_WriteString(&huart, "DMA PASS\n");
            delay(300000U);
        } else {
            GPIO_WritePin(&ld1_green, GPIO_PIN_5, GPIO_PIN_SET);
            USART_WriteString(&huart, "DMA FAIL\n");
            print_dma_state(&huart, &dmaHandle, addressOk, transferOk, dstBuffer, (uint32_t)sizeof(dstBuffer));
            delay(80000U);
            GPIO_WritePin(&ld1_green, GPIO_PIN_5, GPIO_PIN_RESET);
            delay(80000U);
            GPIO_WritePin(&ld1_green, GPIO_PIN_5, GPIO_PIN_SET);
            delay(80000U);
            GPIO_WritePin(&ld1_green, GPIO_PIN_5, GPIO_PIN_RESET);
            delay(400000U);
        }
    }
}