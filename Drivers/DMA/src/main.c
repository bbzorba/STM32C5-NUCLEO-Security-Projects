#include "../inc/dma.h"
#include "../../UART/inc/uart.h"

#define XFER_BYTES 256U

static uint8_t srcBuffer[XFER_BYTES];
static uint8_t dstBuffer[XFER_BYTES];

/* Print detailed DMA register dump on failure */
static void print_dma_state(USART_HandleType *huart, DMA_HandleType *dmaHandle,
                             uint8_t addressOk, uint8_t transferOk) {
    USART_WriteString(huart, "  addr=");
    uart_write_hex8(huart, addressOk);
    USART_WriteString(huart, " xfer=");
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
    USART_WriteString(huart, "\n");
}

/* Write decimal integer over UART */
static void uart_write_dec(USART_HandleType *huart, uint32_t n) {
    char buf[10];
    uint8_t i = 0U;
    if (n == 0U) { USART_WriteChar(huart, '0'); return; }
    while (n > 0U) { buf[i++] = (char)('0' + (n % 10U)); n /= 10U; }
    while (i > 0U) { USART_WriteChar(huart, buf[--i]); }
}

static void delay(volatile uint32_t ticks) {
    while (ticks--) {
        __asm volatile ("nop");
    }
}

int main(void) {
    uint32_t i;
    uint8_t addressOk = 1U;
    uint32_t xfer_count = 0U;

    /* Initialise source buffer: 0x00..0xFF */
    for (i = 0U; i < XFER_BYTES; i++) {
        srcBuffer[i] = (uint8_t)i;
    }

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

    /* One-time address sanity checks */
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

    /* One-time DMA init: enables clock, resets channel, clears flags */
    if (DMA_Init(&dmaHandle) != LPDMA_OK) {
        addressOk = 0U;
    }

    /* Second channel for UART DMA TX demo */
    DMA_HandleType dmaTx;
    DMA_Constructor(&dmaTx, LPDMA1_Channel0, LPDMA_MEMORY_TO_PERIPH);
    if (DMA_Init(&dmaTx) != LPDMA_OK) {
        addressOk = 0U;
    }

    while (1) {
        uint8_t transferOk = 1U;

        /* Clear destination so we can verify a fresh transfer */
        for (i = 0U; i < XFER_BYTES; i++) {
            dstBuffer[i] = 0U;
        }

        /* Configure: SW-triggered mem-to-mem, byte widths, SINC+DINC */
        if (DMA_ConfigTransfer(&dmaHandle,
                               (uintptr_t)srcBuffer,
                               (uintptr_t)dstBuffer,
                               (uint16_t)XFER_BYTES,
                               (LPDMA_TR1_SDW_LOG2_8 | LPDMA_TR1_DDW_LOG2_8 | LPDMA_TR1_DINC),
                               LPDMA_CTR2_SWREQ) != LPDMA_OK) {
            transferOk = 0U;
        }

        if (transferOk && DMA_Start(&dmaHandle) != LPDMA_OK) {
            transferOk = 0U;
        }

        /* Poll for transfer complete (timeout ~200 k iterations) */
        for (i = 0U; i < 200000U; i++) {
            if (DMA_IsTransferComplete(&dmaHandle)) {
                break;
            }
        }

        /* Capture SR before Stop clears flags */
        uint32_t sr_snap = dmaHandle.channel->SR;

        /* Verify data integrity */
        if (transferOk) {
            for (i = 0U; i < XFER_BYTES; i++) {
                if (srcBuffer[i] != dstBuffer[i]) {
                    transferOk = 0U;
                    break;
                }
            }
        }

        DMA_Stop(&dmaHandle);

        xfer_count++;

        /* Per-transfer stats: [#NNN] bytes=256 sr=0xXXXXXXXX first=XX last=FF PASS/FAIL */
        USART_WriteString(&huart, "[#");
        uart_write_dec(&huart, xfer_count);
        USART_WriteString(&huart, "] bytes=");
        uart_write_dec(&huart, XFER_BYTES);
        USART_WriteString(&huart, " sr=0x");
        uart_write_hex32(&huart, sr_snap);
        USART_WriteString(&huart, " first=");
        uart_write_hex8(&huart, dstBuffer[0]);
        USART_WriteString(&huart, " last=");
        uart_write_hex8(&huart, dstBuffer[XFER_BYTES - 1U]);

        if (addressOk && transferOk) {
            GPIO_TogglePin(&ld1_green, GPIO_PIN_5);
            USART_WriteString(&huart, " PASS\n");
        } else {
            USART_WriteString(&huart, " FAIL\n");
            print_dma_state(&huart, &dmaHandle, addressOk, transferOk);
        }

        /* Minimal UART DMA TX: wait for polling output to drain, then send via DMA */
        while (!(huart.regs->ISR & USART_ISR_TC)) { }
        huart.regs->ICR = USART_ICR_TCCF;
        static const uint8_t dma_str[] = "uart-dma ok\r\n";
        USART_WriteDMA(&huart, &dmaTx, dma_str, (uint16_t)(sizeof(dma_str) - 1U));
        while (!USART_IsTXDMAComplete(&dmaTx)) { }
        DMA_Stop(&dmaTx);
        USART_DisableTXDMA(&huart);

        /* ~1 s busy-wait at 48 MHz (volatile loop ~5 cycles/iter) */
        delay(10000000U);
    }
}