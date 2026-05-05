#ifndef __DMA_H
#define __DMA_H

#include <stdint.h>
#include <stddef.h>

#define __IO volatile

#define LPDMA1_BASE 0x40020000U
#define LPDMA2_BASE 0x40021000U

#define RCC_AHB1ENR_LPDMA1EN ((uint32_t)(1UL << 0))
#define RCC_AHB1ENR_LPDMA2EN ((uint32_t)(1UL << 1))

#define LPDMA_INSTANCE_SIZE   0x1000U
#define LPDMA_CH_FIRST_OFFSET 0x50U
#define LPDMA_CH_STRIDE       0x80U

#define LPDMA_CHANNEL_ADDR(base, ch) \
    ((uintptr_t)(base) + LPDMA_CH_FIRST_OFFSET + ((uintptr_t)(ch) * LPDMA_CH_STRIDE))

/* ---- CCR register bits ---- */
#define LPDMA_CCR_EN      0x0001U
#define LPDMA_CCR_RESET   0x0002U
#define LPDMA_CCR_SUSP    0x0004U
#define LPDMA_CCR_TCIE    0x0100U
#define LPDMA_CCR_HTIE    0x0200U
#define LPDMA_CCR_DTEIE   0x0400U
#define LPDMA_CCR_ULEIE   0x0800U
#define LPDMA_CCR_USEIE   0x1000U
#define LPDMA_CCR_SUSPIE  0x2000U
#define LPDMA_CCR_TOIE    0x4000U

/* ---- TR2 ---- */
#define LPDMA_CTR2_SWREQ  0x00000200U   /* bit 9: software request (mem-to-mem)  */
#define LPDMA_CTR2_DREQ   0x00000400U   /* bit 10: destination hardware request  */

/* USART2 hardware request IDs.
 * TX: USART2_TDR (destination) generates request -> DREQ bit set.
 * RX: USART2_RDR (source) generates request      -> DREQ=0. */
#define LPDMA_REQSEL_USART2_TX  (15U | LPDMA_CTR2_DREQ)
#define LPDMA_REQSEL_USART2_RX  14U

/* ---- TR1 data-width and increment bits ---- */
#define LPDMA_TR1_SDW_LOG2_8   0x00000000U
#define LPDMA_TR1_SDW_LOG2_16  0x00000001U
#define LPDMA_TR1_SDW_LOG2_32  0x00000002U
#define LPDMA_TR1_SINC         0x00000008U
#define LPDMA_TR1_DDW_LOG2_8   0x00000000U
#define LPDMA_TR1_DDW_LOG2_16  0x00010000U
#define LPDMA_TR1_DDW_LOG2_32  0x00020000U
#define LPDMA_TR1_DINC         0x00080000U

/* ---- SR flags ---- */
#define LPDMA_SR_TCF  0x00000100U   /* bit 8: transfer complete flag */

typedef enum {
    LPDMA_OK = 0,
    LPDMA_ERROR,
    LPDMA_BUSY
} LPDMA_StatusType;

typedef enum {
    LPDMA_MEMORY_TO_PERIPH  = 0,
    LPDMA_PERIPH_TO_MEMORY  = 1,
    LPDMA_MEMORY_TO_MEMORY  = 2
} LPDMA_DirectionType;

typedef struct {
    __IO uint32_t LBAR;
    __IO uint32_t RESERVED0[2];
    __IO uint32_t FCR;
    __IO uint32_t SR;
    __IO uint32_t CR;
    __IO uint32_t RESERVED1[10];
    __IO uint32_t TR1;
    __IO uint32_t TR2;
    __IO uint32_t BR1;
    __IO uint32_t SAR;
    __IO uint32_t DAR;
    __IO uint32_t RESERVED2[10];
    __IO uint32_t LLR;
} LPDMA_ChannelType;

typedef struct {
    LPDMA_ChannelType  *channel;
    uintptr_t           controllerBase;
    uint8_t             channelIndex;
    LPDMA_StatusType    status;
    LPDMA_DirectionType direction;
    volatile uint8_t    tc_flag;    /* set by IRQ callback, polled by transfer functions */
} DMA_HandleType;

#define LPDMA1_Channel0 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA1_BASE, 0U))
#define LPDMA1_Channel1 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA1_BASE, 1U))
#define LPDMA1_Channel2 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA1_BASE, 2U))
#define LPDMA1_Channel3 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA1_BASE, 3U))
#define LPDMA2_Channel0 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA2_BASE, 0U))
#define LPDMA2_Channel1 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA2_BASE, 1U))

void             DMA_Constructor(DMA_HandleType *hdma, LPDMA_ChannelType *channel, LPDMA_DirectionType direction);
LPDMA_StatusType DMA_Init(DMA_HandleType *hdma);
LPDMA_StatusType DMA_ConfigTransfer(DMA_HandleType *hdma, uintptr_t src, uintptr_t dst, uint16_t bytes, uint32_t tr1, uint32_t tr2);
LPDMA_StatusType DMA_Start(DMA_HandleType *hdma);
LPDMA_StatusType DMA_Stop(DMA_HandleType *hdma);
uint8_t          DMA_IsTransferComplete(DMA_HandleType *hdma);
void             DMA_EnableTCInterrupt(DMA_HandleType *hdma, uint8_t irqn_channel_offset, uint8_t priority);
void             DMA_DisableTCInterrupt(DMA_HandleType *hdma, uint8_t irqn_channel_offset);

#endif /* __DMA_H */
