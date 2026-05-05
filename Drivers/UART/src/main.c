#include "../inc/uart.h"

static char buffer[64];
static uint8_t dma_rx_buf[8];
static const uint8_t dma_tx_buf[] = "Hello via DMA TX!\r\n";

// Global handles so callbacks and DMA ops can reach them from any scope
static USART_HandleType usart;
static DMA_HandleType   dmaTx;
static DMA_HandleType   dmaRx;

// RX interrupt callback — must be at file scope (not nested inside main).
static void uart_rx_cb(char c) {
    if (c == '\r' || c == '\n') {
        // Enter pressed: move to next line (CR+LF)
        USART_WriteChar(&usart, '\r');
        USART_WriteChar(&usart, '\n');
    } else {
        USART_WriteChar(&usart, c);
    }
}

// Function prototypes
void delay(volatile uint32_t count);

int main(void) {
    // Initialize USART2 for RX and TX at 115200
    USART_constructor(&usart, USART_2, RX_AND_TX, __115200);

    // Initialize DMA channels
    DMA_Constructor(&dmaTx, LPDMA1_Channel0, LPDMA_MEMORY_TO_PERIPH);
    DMA_Init(&dmaTx);
    DMA_Constructor(&dmaRx, LPDMA1_Channel1, LPDMA_PERIPH_TO_MEMORY);
    DMA_Init(&dmaRx);

    // -------- Test 1: Polling TX test --------
    USART_WriteString(&usart, "=== Polling TX test ===\r\n");
    USART_WriteString(&usart, "If you can read this, TX works!\r\n");

    // -------- Test 2: Polling RX test --------
    USART_WriteString(&usart, "\r\n=== Polling RX test ===\r\n");
    USART_WriteString(&usart, "Type something and press Enter:\r\n");

    USART_ReadString(&usart, buffer, sizeof(buffer));
    USART_WriteString(&usart, "You typed: ");
    USART_WriteString(&usart, buffer);
    USART_WriteString(&usart, "\r\n");
    USART_WriteString(&usart, "Polling RX works!\r\n");

    // -------- Test 3: DMA-based TX test --------
    USART_WriteString(&usart, "\r\n=== DMA TX test ===\r\n");
    USART_WriteDMA(&usart, &dmaTx, dma_tx_buf, (uint16_t)(sizeof(dma_tx_buf) - 1U));
    DMA_Stop(&dmaTx);  // Ensure DMA is stopped before disabling
    USART_DisableTXDMA(&usart);
    USART_WriteString(&usart, "DMA TX complete!\r\n");

    // -------- Test 4: DMA-based RX test --------
    USART_WriteString(&usart, "\r\n=== DMA RX test ===\r\n");
    USART_WriteString(&usart, "Send exactly 8 bytes...\r\n");
    USART_ReadDMA(&usart, &dmaRx, dma_rx_buf, (uint16_t)sizeof(dma_rx_buf));
    DMA_Stop(&dmaRx);  // Ensure DMA is stopped before disabling
    USART_DisableRXDMA(&usart);

    USART_WriteString(&usart, "Received: ");
    for (uint8_t i = 0; i < (uint8_t)sizeof(dma_rx_buf); i++) {
        USART_WriteChar(&usart, (char)dma_rx_buf[i]);
    }
    USART_WriteString(&usart, "\r\nDMA RX complete!\r\n");

    // -------- Test 5: Interrupt-based RX echo --------
    USART_WriteString(&usart, "\r\n=== Interrupt RX echo test ===\r\n");
    USART_WriteString(&usart, "Type characters to see them echoed (interrupt-driven).\r\n");
    USART_EnableRXInterrupt(&usart, uart_rx_cb);

    while (1) {
        // main loop can do other work; RX is handled by interrupt callback
        delay(1000000);
    }
}

void delay(volatile uint32_t count) {
    while(count--) {
        __asm__("nop");
    }
}