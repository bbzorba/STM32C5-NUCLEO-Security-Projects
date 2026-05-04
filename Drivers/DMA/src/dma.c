#include "../inc/dma.h"

volatile uint32_t g_dma_debug_point = 0U;

#define DMA_DEBUG_POINT(id) do { g_dma_debug_point = (id); } while (0)

static uint8_t DMA_DecodeChannel(DMA_HandleType *handle) {
    uintptr_t addr;
    uintptr_t base;
    uintptr_t rel;

    if (handle == NULL || handle->channel == NULL) {
        return 0U;
    }

    addr = (uintptr_t)handle->channel;
    if (addr >= LPDMA1_BASE && addr < (LPDMA1_BASE + LPDMA_INSTANCE_SIZE)) {
        base = LPDMA1_BASE;
    } else if (addr >= LPDMA2_BASE && addr < (LPDMA2_BASE + LPDMA_INSTANCE_SIZE)) {
        base = LPDMA2_BASE;
    } else {
        return 0U;
    }

    if (addr < (base + LPDMA_CH_FIRST_OFFSET)) {
        return 0U;
    }

    rel = addr - (base + LPDMA_CH_FIRST_OFFSET);
    if ((rel % LPDMA_CH_STRIDE) != 0U) {
        return 0U;
    }

    handle->channelIndex = (uint8_t)(rel / LPDMA_CH_STRIDE);
    handle->controllerBase = base;
    return 1U;
}

void DMA_Constructor(DMA_HandleType *handle, LPDMA_ChannelType *channel, LPDMA_DirectionType direction) {
    DMA_DEBUG_POINT(0x10U);
    if (handle == NULL) {
        DMA_DEBUG_POINT(0x1FU);
        return;
    }

    handle->channel = channel;
    handle->controllerBase = 0U;
    handle->channelIndex = 0U;
    handle->status = LPDMA_OK;
    handle->direction = direction;
    DMA_DEBUG_POINT(0x11U);
}

LPDMA_StatusType DMA_Init(DMA_HandleType *LPDMA_handle) {
    DMA_DEBUG_POINT(0x20U);
    if(LPDMA_handle == NULL || LPDMA_handle->channel == NULL) {
        DMA_DEBUG_POINT(0x2FU);
        return LPDMA_ERROR;
    }

    if (!DMA_DecodeChannel(LPDMA_handle)) {
        LPDMA_handle->status = LPDMA_ERROR;
        DMA_DEBUG_POINT(0x2EU);
        return LPDMA_ERROR;
    }

    if (LPDMA_handle->controllerBase == LPDMA1_BASE) {
        RCC->AHB1ENR |= RCC_AHB1ENR_LPDMA1EN;
    } else {
        RCC->AHB1ENR |= RCC_AHB1ENR_LPDMA2EN;
    }

    LPDMA_handle->channel->CR = 0U;
    LPDMA_handle->channel->CR = LPDMA_CCR_RESET;
    while ((LPDMA_handle->channel->CR & LPDMA_CCR_RESET) != 0U) {
    }
    LPDMA_handle->channel->FCR = (LPDMA_CCR_TCIE | LPDMA_CCR_HTIE | LPDMA_CCR_DTEIE |
                                  LPDMA_CCR_ULEIE | LPDMA_CCR_USEIE | LPDMA_CCR_SUSPIE |
                                  LPDMA_CCR_TOIE);
    LPDMA_handle->channel->LBAR = 0U;
    LPDMA_handle->channel->TR1 = 0U;
    LPDMA_handle->channel->TR2 = 0U;
    LPDMA_handle->channel->BR1 = 0U;
    LPDMA_handle->channel->SAR = 0U;
    LPDMA_handle->channel->DAR = 0U;
    LPDMA_handle->channel->LLR = 0U;

    LPDMA_handle->status = LPDMA_OK;
    DMA_DEBUG_POINT(0x21U);
    return LPDMA_OK;
}

LPDMA_StatusType DMA_ConfigTransfer(DMA_HandleType *LPDMA_handle, uintptr_t src, uintptr_t dst, uint16_t bytes, uint32_t tr1Config, uint32_t tr2Config) {
    DMA_DEBUG_POINT(0x30U);
    if (LPDMA_handle == NULL || LPDMA_handle->channel == NULL || bytes == 0U) {
        DMA_DEBUG_POINT(0x3FU);
        return LPDMA_ERROR;
    }

    LPDMA_handle->channel->CR &= ~LPDMA_CCR_EN;

    if (LPDMA_handle->direction == LPDMA_MEMORY_TO_PERIPH) {
        tr1Config |= LPDMA_TR1_SINC;
    } else {
        tr1Config |= LPDMA_TR1_DINC;
    }

    LPDMA_handle->channel->SAR = (uint32_t)src;
    LPDMA_handle->channel->DAR = (uint32_t)dst;
    LPDMA_handle->channel->BR1 = (uint32_t)bytes;
    LPDMA_handle->channel->TR1 = tr1Config;
    LPDMA_handle->channel->TR2 = tr2Config;
    DMA_DEBUG_POINT(0x31U);
    return LPDMA_OK;
}

LPDMA_StatusType DMA_Start(DMA_HandleType *LPDMA_handle) {
    DMA_DEBUG_POINT(0x40U);
    if (LPDMA_handle == NULL || LPDMA_handle->channel == NULL) {
        DMA_DEBUG_POINT(0x4FU);
        return LPDMA_ERROR;
    }

    LPDMA_handle->channel->FCR = (LPDMA_CCR_TCIE | LPDMA_CCR_HTIE | LPDMA_CCR_DTEIE |
                                  LPDMA_CCR_ULEIE | LPDMA_CCR_USEIE | LPDMA_CCR_SUSPIE |
                                  LPDMA_CCR_TOIE);

    /* TR2 (including SWREQ) must be fully configured before enabling the channel.
     * LPDMA configuration registers are locked once EN=1 and cannot be written. */
    LPDMA_handle->channel->CR |= LPDMA_CCR_EN;

    LPDMA_handle->status = LPDMA_BUSY;
    DMA_DEBUG_POINT(0x41U);
    return LPDMA_OK;
}

LPDMA_StatusType DMA_Stop(DMA_HandleType *LPDMA_handle) {
    DMA_DEBUG_POINT(0x50U);
    if (LPDMA_handle == NULL || LPDMA_handle->channel == NULL) {
        DMA_DEBUG_POINT(0x5FU);
        return LPDMA_ERROR;
    }

    LPDMA_handle->channel->CR &= ~LPDMA_CCR_EN;
    LPDMA_handle->status = LPDMA_OK;
    DMA_DEBUG_POINT(0x51U);
    return LPDMA_OK;
}

uint8_t DMA_IsTransferComplete(DMA_HandleType *LPDMA_handle) {
    DMA_DEBUG_POINT(0x60U);
    if (LPDMA_handle == NULL || LPDMA_handle->channel == NULL) {
        DMA_DEBUG_POINT(0x6FU);
        return 0U;
    }

    if ((LPDMA_handle->channel->SR & LPDMA_SR_TCF) != 0U) {
        LPDMA_handle->status = LPDMA_OK;
        DMA_DEBUG_POINT(0x61U);
        return 1U;
    }

    DMA_DEBUG_POINT(0x62U);
    return 0U;
}

uintptr_t DMA_GetRegisterAddress(DMA_HandleType *LPDMA_handle, uint32_t regOffset) {
    if (LPDMA_handle == NULL || LPDMA_handle->channel == NULL) {
        return 0U;
    }

    return ((uintptr_t)LPDMA_handle->channel + regOffset);
}