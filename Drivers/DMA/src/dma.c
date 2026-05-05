#include "../inc/dma.h"
#include "../../GPIO/inc/gpio.h"   /* RCC_TypeDef / RCC */
#include "../../NVIC/inc/nvic.h"   /* NVIC_RegisterHandler / IRQn_Type */

/* -----------------------------------------------------------------------
 * Channel index decode helper
 * ----------------------------------------------------------------------- */
static uint8_t decode_channel(DMA_HandleType *hdma)
{
    uintptr_t addr = (uintptr_t)hdma->channel;
    uintptr_t base;

    if (addr >= LPDMA1_BASE && addr < (LPDMA1_BASE + LPDMA_INSTANCE_SIZE)) {
        base = LPDMA1_BASE;
    } else if (addr >= LPDMA2_BASE && addr < (LPDMA2_BASE + LPDMA_INSTANCE_SIZE)) {
        base = LPDMA2_BASE;
    } else {
        return 0U;
    }

    if (addr < (base + LPDMA_CH_FIRST_OFFSET)) { return 0U; }

    uintptr_t rel = addr - (base + LPDMA_CH_FIRST_OFFSET);
    if ((rel % LPDMA_CH_STRIDE) != 0U) { return 0U; }

    hdma->channelIndex   = (uint8_t)(rel / LPDMA_CH_STRIDE);
    hdma->controllerBase = base;
    return 1U;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */
void DMA_Constructor(DMA_HandleType *hdma, LPDMA_ChannelType *channel,
                     LPDMA_DirectionType direction)
{
    hdma->channel       = channel;
    hdma->controllerBase = 0U;
    hdma->channelIndex  = 0U;
    hdma->status        = LPDMA_OK;
    hdma->direction     = direction;
    hdma->tc_flag       = 0U;
}

LPDMA_StatusType DMA_Init(DMA_HandleType *hdma)
{
    if (hdma == NULL || hdma->channel == NULL) { return LPDMA_ERROR; }
    if (!decode_channel(hdma)) { hdma->status = LPDMA_ERROR; return LPDMA_ERROR; }

    if (hdma->controllerBase == LPDMA1_BASE) {
        RCC->AHB1ENR |= RCC_AHB1ENR_LPDMA1EN;
    } else {
        RCC->AHB1ENR |= RCC_AHB1ENR_LPDMA2EN;
    }

    /* Reset channel and wait for self-clear */
    hdma->channel->CR = 0U;
    hdma->channel->CR = LPDMA_CCR_RESET;
    while (hdma->channel->CR & LPDMA_CCR_RESET) { }

    /* Clear all flags */
    hdma->channel->FCR = (LPDMA_CCR_TCIE | LPDMA_CCR_HTIE | LPDMA_CCR_DTEIE |
                          LPDMA_CCR_ULEIE | LPDMA_CCR_USEIE | LPDMA_CCR_SUSPIE |
                          LPDMA_CCR_TOIE);
    hdma->channel->LBAR = 0U;
    hdma->channel->TR1  = 0U;
    hdma->channel->TR2  = 0U;
    hdma->channel->BR1  = 0U;
    hdma->channel->SAR  = 0U;
    hdma->channel->DAR  = 0U;
    hdma->channel->LLR  = 0U;
    hdma->tc_flag       = 0U;
    hdma->status        = LPDMA_OK;
    return LPDMA_OK;
}

LPDMA_StatusType DMA_ConfigTransfer(DMA_HandleType *hdma, uintptr_t src, uintptr_t dst,
                                    uint16_t bytes, uint32_t tr1, uint32_t tr2)
{
    if (hdma == NULL || hdma->channel == NULL || bytes == 0U) { return LPDMA_ERROR; }

    hdma->channel->CR &= ~LPDMA_CCR_EN;

    /* Auto-apply increment for the moving side */
    if (hdma->direction == LPDMA_MEMORY_TO_PERIPH || hdma->direction == LPDMA_MEMORY_TO_MEMORY) {
        tr1 |= LPDMA_TR1_SINC;
    }
    if (hdma->direction == LPDMA_PERIPH_TO_MEMORY || hdma->direction == LPDMA_MEMORY_TO_MEMORY) {
        tr1 |= LPDMA_TR1_DINC;
    }

    hdma->channel->SAR = (uint32_t)src;
    hdma->channel->DAR = (uint32_t)dst;
    hdma->channel->BR1 = (uint32_t)bytes;
    hdma->channel->TR1 = tr1;
    hdma->channel->TR2 = tr2;
    return LPDMA_OK;
}

LPDMA_StatusType DMA_Start(DMA_HandleType *hdma)
{
    if (hdma == NULL || hdma->channel == NULL) { return LPDMA_ERROR; }

    /* Clear flags and tc_flag before launch */
    hdma->channel->FCR = (LPDMA_CCR_TCIE | LPDMA_CCR_HTIE | LPDMA_CCR_DTEIE |
                       LPDMA_CCR_ULEIE | LPDMA_CCR_USEIE | LPDMA_CCR_SUSPIE |
                       LPDMA_CCR_TOIE);
    hdma->tc_flag = 0U;
    hdma->channel->CR |= LPDMA_CCR_EN;
    hdma->status = LPDMA_BUSY;
    return LPDMA_OK;
}

LPDMA_StatusType DMA_Stop(DMA_HandleType *hdma)
{
    if (hdma == NULL || hdma->channel == NULL) { return LPDMA_ERROR; }
    hdma->channel->CR &= ~LPDMA_CCR_EN;
    hdma->status = LPDMA_OK;
    return LPDMA_OK;
}

uint8_t DMA_IsTransferComplete(DMA_HandleType *hdma)
{
    if (hdma == NULL || hdma->channel == NULL) { return 0U; }
    if (hdma->channel->SR & LPDMA_SR_TCF) {
        hdma->status = LPDMA_OK;
        return 1U;
    }
    return 0U;
}

/* -----------------------------------------------------------------------
 * Interrupt support — internal callback dispatched via nvic.c dispatch table
 * ----------------------------------------------------------------------- */
static void dma_tc_irq_callback(void *arg)
{
    DMA_HandleType *hdma = (DMA_HandleType *)arg;
    /* Clear TCF */
    hdma->channel->FCR = LPDMA_CCR_TCIE;
    hdma->tc_flag = 1U;
}

void DMA_EnableTCInterrupt(DMA_HandleType *hdma, uint8_t irqn_channel_offset, uint8_t priority)
{
    /* irqn_channel_offset: LPDMA1_Channel0_IRQn = 23 .. Channel7 = 30 */
    hdma->channel->CR |= LPDMA_CCR_TCIE;
    NVIC_RegisterHandler((IRQn_Type)irqn_channel_offset, dma_tc_irq_callback, hdma, priority);
}

void DMA_DisableTCInterrupt(DMA_HandleType *hdma, uint8_t irqn_channel_offset)
{
    hdma->channel->CR &= ~LPDMA_CCR_TCIE;
    NVIC_UnregisterHandler((IRQn_Type)irqn_channel_offset);
}
