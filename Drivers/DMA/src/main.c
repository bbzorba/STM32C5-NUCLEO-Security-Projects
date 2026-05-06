#include "../inc/dma.h"
#include "../../UART/inc/uart.h"

/* -----------------------------------------------------------------------
 * Hardware handles (file-scope so test functions can reach them)
 * ----------------------------------------------------------------------- */
USART_HandleType huart;
DMA_HandleType   dma_mem;   /* channel 1 — mem-to-mem tests  */
DMA_HandleType   dma_uart;  /* channel 0 — UART DMA TX tests  */

/* -----------------------------------------------------------------------
 * Buffers for mem-to-mem tests
 * ----------------------------------------------------------------------- */
#define BIG_BUF_SIZE 1024U
uint8_t src_buf[BIG_BUF_SIZE];
uint8_t dst_buf[BIG_BUF_SIZE];

/* ----------------------------------------------------------------------- */
static void pass_fail(uint8_t ok)
{
    USART_WriteString(&huart, ok ? "PASS\r\n" : "FAIL\r\n");
}

/* ----------------------------------------------------------------------- */
/* Test 1 — Polling mem-to-mem: copy 1 kB, verify every byte              */
static uint8_t test_memcpy_polling(void)
{
    uint32_t i;

    for (i = 0U; i < BIG_BUF_SIZE; i++) { src_buf[i] = (uint8_t)(i & 0xFFU); }
    for (i = 0U; i < BIG_BUF_SIZE; i++) { dst_buf[i] = 0U; }

    DMA_Init(&dma_mem);
    DMA_ConfigTransfer(&dma_mem,
                       (uintptr_t)src_buf, (uintptr_t)dst_buf,
                       (uint16_t)BIG_BUF_SIZE,
                       (LPDMA_TR1_SDW_LOG2_8 | LPDMA_TR1_DDW_LOG2_8),
                       LPDMA_CTR2_SWREQ);
    DMA_Start(&dma_mem);

    /* Poll with timeout */
    for (i = 0U; i < 500000U; i++) {
        if (DMA_IsTransferComplete(&dma_mem)) { break; }
    }
    DMA_Stop(&dma_mem);

    for (i = 0U; i < BIG_BUF_SIZE; i++) {
        if (dst_buf[i] != src_buf[i]) { return 0U; }
    }
    return 1U;
}

/* ----------------------------------------------------------------------- */
/* Test 2 — Interrupt-driven mem-to-mem: same transfer, wait on tc_flag   */
/* IRQn for LPDMA1 Channel 1 is 24 (= LPDMA1_Channel0_IRQn + 1)          */
#define DMA_MEM_IRQn  24U   /* LPDMA1_Channel1_IRQn */

static uint8_t test_memcpy_interrupt(void)
{
    uint32_t i;

    for (i = 0U; i < BIG_BUF_SIZE; i++) { src_buf[i] = (uint8_t)((i + 0x55U) & 0xFFU); }
    for (i = 0U; i < BIG_BUF_SIZE; i++) { dst_buf[i] = 0U; }

    DMA_Init(&dma_mem);
    DMA_EnableTCInterrupt(&dma_mem, DMA_MEM_IRQn, 0U);
    DMA_ConfigTransfer(&dma_mem,
                       (uintptr_t)src_buf, (uintptr_t)dst_buf,
                       (uint16_t)BIG_BUF_SIZE,
                       (LPDMA_TR1_SDW_LOG2_8 | LPDMA_TR1_DDW_LOG2_8),
                       LPDMA_CTR2_SWREQ);
    DMA_Start(&dma_mem);

    /* Wait for IRQ to set tc_flag */
    for (i = 0U; i < 500000U; i++) {
        if (dma_mem.tc_flag) { break; }
    }
    DMA_DisableTCInterrupt(&dma_mem, DMA_MEM_IRQn);
    DMA_Stop(&dma_mem);

    if (!dma_mem.tc_flag) { return 0U; }
    for (i = 0U; i < BIG_BUF_SIZE; i++) {
        if (dst_buf[i] != src_buf[i]) { return 0U; }
    }
    return 1U;
}

/* ----------------------------------------------------------------------- */
/* Test 3 — Word-width (32-bit) copy of 1 kB                              */
static uint8_t test_memcpy_word_width(void)
{
    uint32_t i;
    uint32_t *s32 = (uint32_t *)(void *)src_buf;
    uint32_t *d32 = (uint32_t *)(void *)dst_buf;

    for (i = 0U; i < BIG_BUF_SIZE / 4U; i++) { s32[i] = 0xDEAD0000U | i; }
    for (i = 0U; i < BIG_BUF_SIZE / 4U; i++) { d32[i] = 0U; }

    DMA_Init(&dma_mem);
    DMA_ConfigTransfer(&dma_mem,
                       (uintptr_t)src_buf, (uintptr_t)dst_buf,
                       (uint16_t)BIG_BUF_SIZE,
                       (LPDMA_TR1_SDW_LOG2_32 | LPDMA_TR1_DDW_LOG2_32),
                       LPDMA_CTR2_SWREQ);
    DMA_Start(&dma_mem);

    for (i = 0U; i < 500000U; i++) {
        if (DMA_IsTransferComplete(&dma_mem)) { break; }
    }
    DMA_Stop(&dma_mem);

    for (i = 0U; i < BIG_BUF_SIZE / 4U; i++) {
        if (d32[i] != s32[i]) { return 0U; }
    }
    return 1U;
}

/* ----------------------------------------------------------------------- */
/* Test 4 — UART DMA TX: send a string via DMA                            */
static const uint8_t uart_dma_str[] = "[UART-DMA] Hello via DMA TX!\r\n";

static uint8_t test_uart_dma_tx(void)
{
    LPDMA_StatusType st = USART_WriteDMA(&huart, &dma_uart,
                                         uart_dma_str,
                                         (uint16_t)(sizeof(uart_dma_str) - 1U));
    USART_DisableTXDMA(&huart);
    return (st == LPDMA_OK) ? 1U : 0U;
}

/* -----------------------------------------------------------------------
 * Entry point
 * ----------------------------------------------------------------------- */
int main(void)
{
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    DMA_Constructor(&dma_mem,  LPDMA1_Channel1, LPDMA_MEMORY_TO_MEMORY);
    DMA_Constructor(&dma_uart, LPDMA1_Channel0, LPDMA_MEMORY_TO_PERIPH);

    USART_WriteString(&huart, "\r\n=== DMA Tests ===\r\n");

    USART_WriteString(&huart, "[1] mem-to-mem polling (1kB): ");
    pass_fail(test_memcpy_polling());

    USART_WriteString(&huart, "[2] mem-to-mem interrupt (1kB): ");
    pass_fail(test_memcpy_interrupt());

    USART_WriteString(&huart, "[3] word-width copy (1kB): ");
    pass_fail(test_memcpy_word_width());

    USART_WriteString(&huart, "[4] UART DMA TX: ");
    pass_fail(test_uart_dma_tx());

    USART_WriteString(&huart, "=== Done ===\r\n");
    while (1) { }
}
