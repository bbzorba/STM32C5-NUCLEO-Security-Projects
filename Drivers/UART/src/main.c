#include "../inc/uart.h"

static char buffer[64];

// Global handle so the RX callback (called from IRQ context) can reach it
static USART_HandleType usart;

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

    // -------- Phase 1: Polling TX test --------
    USART_WriteString(&usart, "=== Polling TX test ===\r\n");
    USART_WriteString(&usart, "If you can read this, TX works!\r\n");

    // -------- Phase 2: Polling RX test --------
    USART_WriteString(&usart, "\r\n=== Polling RX test ===\r\n");
    USART_WriteString(&usart, "Type something and press Enter:\r\n");

    USART_ReadString(&usart, buffer, sizeof(buffer));

    USART_WriteString(&usart, "You typed: ");
    USART_WriteString(&usart, buffer);
    USART_WriteString(&usart, "\r\n");
    USART_WriteString(&usart, "Polling RX works!\r\n");

    // -------- Phase 3: Interrupt-based RX echo --------
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