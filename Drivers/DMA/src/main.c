#include "../inc/dma.h"
#include "../../UART/inc/uart.h"

static uint8_t srcBuffer[8] = {0x10U, 0x21U, 0x32U, 0x43U, 0x54U, 0x65U, 0x76U, 0x87U};
static uint8_t dstBuffer[8] = {0U};

static void uart_write_hex8(USART_HandleType *console, uint8_t value) {
    static const char hex[] = "0123456789ABCDEF";
    char text[3];

    text[0] = hex[(value >> 4) & 0x0FU];
    text[1] = hex[value & 0x0FU];
    text[2] = '\0';
    USART_WriteString(console, text);
}

static void uart_write_hex32(USART_HandleType *console, uint32_t value) {
    static const char hex[] = "0123456789ABCDEF";
    char text[11];
    uint32_t shift;

    text[0] = '0';
    text[1] = 'x';
    for (shift = 0U; shift < 8U; shift++) {
        text[2U + shift] = hex[(value >> ((7U - shift) * 4U)) & 0x0FU];
    }
    text[10] = '\0';
    USART_WriteString(console, text);
}

static void print_dma_state(USART_HandleType *console, DMA_HandleType *dmaHandle, uint8_t addressOk, uint8_t transferOk, uint8_t *dst, uint32_t size) {
    uint32_t index;

    USART_WriteString(console, "addr=");
    uart_write_hex8(console, addressOk);
    USART_WriteString(console, " transfer=");
    uart_write_hex8(console, transferOk);
    USART_WriteString(console, " dbg=");
    uart_write_hex32(console, g_dma_debug_point);
    USART_WriteString(console, " sr=");
    uart_write_hex32(console, dmaHandle->channel->SR);
    USART_WriteString(console, " cr=");
    uart_write_hex32(console, dmaHandle->channel->CR);
    USART_WriteString(console, " tr1=");
    uart_write_hex32(console, dmaHandle->channel->TR1);
    USART_WriteString(console, " tr2=");
    uart_write_hex32(console, dmaHandle->channel->TR2);
    USART_WriteString(console, " br1=");
    uart_write_hex32(console, dmaHandle->channel->BR1);
    USART_WriteString(console, " dst=");
    for (index = 0U; index < size; index++) {
        uart_write_hex8(console, dst[index]);
    }
    USART_WriteString(console, "\n");
}

static void delay(volatile uint32_t ticks) {
    while (ticks--) {
        __asm volatile ("nop");
    }
}

static void led_init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIO_A->MODER &= ~(0x3U << (5U * 2U));
    GPIO_A->MODER |=  (0x1U << (5U * 2U));
}

static void led_on(void) {
    GPIO_A->BSRR = (1U << 5U);
}

static void led_off(void) {
    GPIO_A->BSRR = (1U << (5U + 16U));
}

static void led_toggle(void) {
    GPIO_A->ODR ^= (1U << 5U);
}

int main(void) {
    uint8_t transferOk = 1U;
    uint8_t addressOk = 1U;
    uint32_t i;
    USART_HandleType console;

    led_init();
    USART_constructor(&console, USART_2, TX_ONLY, __115200);

    DMA_HandleType dmaHandle;
    DMA_Constructor(&dmaHandle, LPDMA1_Channel1, LPDMA_MEMORY_TO_PERIPH);

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
            led_toggle();
            USART_WriteString(&console, "DMA PASS\n");
            delay(300000U);
        } else {
            led_on();
            USART_WriteString(&console, "DMA FAIL\n");
            print_dma_state(&console, &dmaHandle, addressOk, transferOk, dstBuffer, (uint32_t)sizeof(dstBuffer));
            delay(80000U);
            led_off();
            delay(80000U);
            led_on();
            delay(80000U);
            led_off();
            delay(400000U);
        }
    }
}