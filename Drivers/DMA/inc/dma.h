#ifndef __DMA_H
#define __DMA_H

#include <stdint.h>
#include <stddef.h>

#define __IO volatile

#define LPDMA1_BASE 0x40020000U
#define LPDMA2_BASE 0x40021000U

#define RCC_AHB1ENR_LPDMA1EN ((uint32_t)(1UL << 0))
#define RCC_AHB1ENR_LPDMA2EN ((uint32_t)(1UL << 1))

#define LPDMA_INSTANCE_SIZE 0x1000U
#define LPDMA_CH_FIRST_OFFSET 0x50U
#define LPDMA_CH_STRIDE 0x80U

#define LPDMA_CH_REG_LBAR_OFFSET 0x00U
#define LPDMA_CH_REG_FCR_OFFSET  0x0CU
#define LPDMA_CH_REG_SR_OFFSET   0x10U
#define LPDMA_CH_REG_CR_OFFSET   0x14U
#define LPDMA_CH_REG_TR1_OFFSET  0x40U
#define LPDMA_CH_REG_TR2_OFFSET  0x44U
#define LPDMA_CH_REG_BR1_OFFSET  0x48U
#define LPDMA_CH_REG_SAR_OFFSET  0x4CU
#define LPDMA_CH_REG_DAR_OFFSET  0x50U
#define LPDMA_CH_REG_LLR_OFFSET  0x7CU

#define LPDMA_CHANNEL_ADDR(base, ch) ((uintptr_t)(base) + LPDMA_CH_FIRST_OFFSET + ((uintptr_t)(ch) * LPDMA_CH_STRIDE))

/* LPDMA CCR register bit definitions (STM32C562RE) */
#define LPDMA_CCR_EN           0x0001U     // Bit 0: Channel enable
#define LPDMA_CCR_RESET        0x0002U     // Bit 1: Channel reset (self-clearing)
#define LPDMA_CCR_SUSP         0x0004U     // Bit 2: Channel suspend
#define LPDMA_CCR_TCIE         0x0100U     // Bit 8: Transfer complete interrupt enable
#define LPDMA_CCR_HTIE         0x0200U     // Bit 9: Half transfer interrupt enable
#define LPDMA_CCR_DTEIE        0x0400U     // Bit 10: Data transfer error interrupt enable
#define LPDMA_CCR_ULEIE        0x0800U     // Bit 11: Update linked-list enable
#define LPDMA_CCR_USEIE        0x1000U     // Bit 12: User interrupt enable
#define LPDMA_CCR_SUSPIE       0x2000U     // Bit 13: Suspend request enable
#define LPDMA_CCR_TOIE         0x4000U     // Bit 14: Trigger overrun interrupt enable
#define LPDMA_CCR_LSM          0x8000U     // Bit 15: Linked-list mode
#define LPDMA_CCR_PL_LOW       0x0000U     // Priority level: Low
#define LPDMA_CCR_PL_MEDIUM    0x10000U    // Bit 22: Priority level: Medium
#define LPDMA_CCR_PL_HIGH      0x20000U    // Bit 23: Priority level: High
#define LPDMA_CCR_PL_VERY_HIGH 0x30000U    // Bit 22-23: Priority level: Very High

#define LPDMA_CTR2_SWREQ         0x00000200U // Bit 9: Software request

/* LPDMA TR1 register bit definitions (STM32C562RE) */
#define LPDMA_TR1_SDW_LOG2_8   0x00000000U // Source data width: 8-bit
#define LPDMA_TR1_SDW_LOG2_16  0x00000001U // Bit 0: Source data width: 16-bit
#define LPDMA_TR1_SDW_LOG2_32  0x00000002U // Bit 1: Source data width: 32-bit
#define LPDMA_TR1_SINC         0x00000008U // Bit 3: Source increment mode enable
#define LPDMA_TR1_DDW_LOG2_8   0x00000000U // Destination data width: 8-bit
#define LPDMA_TR1_DDW_LOG2_16  0x00010000U // Bit 16: Destination data width: 16-bit
#define LPDMA_TR1_DDW_LOG2_32  0x00020000U // Bit 17: Destination data width: 32-bit
#define LPDMA_TR1_DINC         0x00080000U // Bit 19: Destination increment mode enable

#define LPDMA_SR_TCF           0x00000100U // Bit 8: Transfer complete flag

/* LPDMA status return type */
typedef enum {
    LPDMA_OK = 0,
    LPDMA_ERROR,
    LPDMA_BUSY
} LPDMA_StatusType;

/* LPDMA transfer direction enum */
typedef enum {
    LPDMA_MEMORY_TO_PERIPH = 0,
    LPDMA_PERIPH_TO_MEMORY
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
    LPDMA_ChannelType *channel;
    uintptr_t controllerBase;
    uint8_t channelIndex;
    LPDMA_StatusType status;
    LPDMA_DirectionType direction;
} DMA_HandleType;

#define LPDMA1_Channel0 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA1_BASE, 0U))
#define LPDMA1_Channel1 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA1_BASE, 1U))
#define LPDMA2_Channel0 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA2_BASE, 0U))
#define LPDMA2_Channel1 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA2_BASE, 1U))
#define LPDMA2_Channel2 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA2_BASE, 2U))
#define LPDMA2_Channel3 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA2_BASE, 3U))
#define LPDMA2_Channel4 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA2_BASE, 4U))
#define LPDMA2_Channel5 ((LPDMA_ChannelType *)LPDMA_CHANNEL_ADDR(LPDMA2_BASE, 5U))

/* LPDMA hardware request numbers for USART2 (STM32C562RE).
 * Source: stm32c5xx_ll_dma.h — same values for both LPDMA1 and LPDMA2. */
#define LPDMA_REQSEL_USART2_TX   15U   /* LL_LPDMA1_REQUEST_USART2_TX */
#define LPDMA_REQSEL_USART2_RX   14U   /* LL_LPDMA1_REQUEST_USART2_RX */

extern volatile uint32_t g_dma_debug_point;

void DMA_Constructor(DMA_HandleType *handle, LPDMA_ChannelType *channel, LPDMA_DirectionType direction);
LPDMA_StatusType DMA_Init(DMA_HandleType *LPDMA_handle);
LPDMA_StatusType DMA_ConfigTransfer(DMA_HandleType *LPDMA_handle, uintptr_t src, uintptr_t dst, uint16_t bytes, uint32_t tr1Config, uint32_t tr2Config);
LPDMA_StatusType DMA_Start(DMA_HandleType *LPDMA_handle);
LPDMA_StatusType DMA_Stop(DMA_HandleType *LPDMA_handle);
uint8_t DMA_IsTransferComplete(DMA_HandleType *LPDMA_handle);
uintptr_t DMA_GetRegisterAddress(DMA_HandleType *LPDMA_handle, uint32_t regOffset);

#endif // __DMA_H
