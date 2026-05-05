#ifndef __UART_H
#define __UART_H

#include <stdint.h>
#include <stdio.h>
#include "../../GPIO/inc/gpio.h"

#define __IO volatile
#define __NVIC_PRIO_BITS 4

/* IRQn_Type for STM32C562RE USART/UART peripherals.
   Verified against official stm32c5xx-dfp startup_stm32c562xx.c vector table. */
#ifndef __STM32C5xx_H
typedef enum IRQn
{
    NonMaskableInt_IRQn         = -14,
    MemoryManagement_IRQn       = -12,
    BusFault_IRQn               = -11,
    UsageFault_IRQn             = -10,
    SVCall_IRQn                 = -5,
    DebugMonitor_IRQn           = -4,
    PendSV_IRQn                 = -2,
    SysTick_IRQn                = -1,

    /* STM32C562RE peripheral IRQs (subset) */
    USART1_IRQn                 = 51,
    USART2_IRQn                 = 52,
    USART3_IRQn                 = 53,
    UART4_IRQn                  = 54,
    UART5_IRQn                  = 55,
} IRQn_Type;
#endif

#include "core_cm4.h"

// USART peripheral base addresses (STM32C562RE)
#define USART_1_BASE  0x40013800U                           // APB2
#define USART_2_BASE  0x40004400U                           // APB1
#define USART_3_BASE  0x40004800U                           // APB1
#define UART_4_BASE   0x40004C00U                           // APB1
#define UART_5_BASE   0x40005000U                           // APB1

// RCC clock enable bits — USART2/3, UART4/5 on APB1 (RCC->APB1LENR)
#define RCC_APB1LENR_USART2EN ((uint32_t)(1UL << 17))
#define RCC_APB1LENR_USART3EN ((uint32_t)(1UL << 18))
#define RCC_APB1LENR_UART4EN  ((uint32_t)(1UL << 19))
#define RCC_APB1LENR_UART5EN  ((uint32_t)(1UL << 20))
// USART1 on APB2 (RCC->APB2ENR)
#define RCC_APB2ENR_USART1EN  ((uint32_t)(1UL << 14))

// Default peripheral clocks (Hz) for baud rate calculation.
// STM32C562RE boots from HSI = 48 MHz, APB1/APB2 prescaler = /1 by default.
#ifndef APB1_CLK_HZ
#define APB1_CLK_HZ 48000000U
#endif
#ifndef APB2_CLK_HZ
#define APB2_CLK_HZ 48000000U
#endif

/* ---- USART CR1 register bit definitions (new-style USART, STM32C5) ---- */
#define USART_CR1_UE      0x0001U                          // Bit 0:  USART Enable
#define USART_CR1_RE      0x0004U                          // Bit 2:  Receiver Enable
#define USART_CR1_TE      0x0008U                          // Bit 3:  Transmitter Enable
#define USART_CR1_RXNEIE  0x0020U                          // Bit 5:  RXNE Interrupt Enable
#define USART_CR1_TXEIE   0x0080U                          // Bit 7:  TXE Interrupt Enable

/* ---- USART CR3 register bit definitions ---- */
#define USART_CR3_DMAR    0x0040U                          // Bit 6: DMA Enable Receiver
#define USART_CR3_DMAT    0x0080U                          // Bit 7: DMA Enable Transmitter

/* ---- USART ISR (Interrupt and Status Register) bit definitions ---- */
#define USART_ISR_PE       0x0001U                         // Bit 0:  Parity Error
#define USART_ISR_FE       0x0002U                         // Bit 1:  Framing Error
#define USART_ISR_NE       0x0004U                         // Bit 2:  Noise detected
#define USART_ISR_ORE      0x0008U                         // Bit 3:  Overrun Error
#define USART_ISR_RXNE     0x0020U                         // Bit 5:  Read Data Reg Not Empty
#define USART_ISR_TC       0x0040U                         // Bit 6:  Transmission Complete
#define USART_ISR_TXE      0x0080U                         // Bit 7:  Transmit Data Reg Empty
#define USART_ISR_ERR_MASK (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE) // Error flags mask

/* ---- USART ICR (Interrupt Flag Clear Register) bit definitions ---- */
#define USART_ICR_PECF     0x0001U                         // Bit 0:  PE clear
#define USART_ICR_FECF     0x0002U                         // Bit 1:  FE clear
#define USART_ICR_NECF     0x0004U                         // Bit 2:  NE clear
#define USART_ICR_ORECF    0x0008U                         // Bit 3:  ORE clear
#define USART_ICR_TCCF     0x0040U                         // Bit 6:  TC clear

/* USART registers structure (new-style: ISR/ICR/RDR/TDR) */
typedef struct
{
    __IO uint32_t CR1;            /* 0x00  Control Register 1 */
    __IO uint32_t CR2;            /* 0x04  Control Register 2 */
    __IO uint32_t CR3;            /* 0x08  Control Register 3 */
    __IO uint32_t BRR;            /* 0x0C  Baud Rate Register */
    __IO uint32_t GTPR;           /* 0x10  Guard Time and Prescaler Register */
    __IO uint32_t RTOR;           /* 0x14  Receiver Timeout Register */
    __IO uint32_t RQR;            /* 0x18  Request Register */
    __IO uint32_t ISR;            /* 0x1C  Interrupt and Status Register */
    __IO uint32_t ICR;            /* 0x20  Interrupt Flag Clear Register */
    __IO uint32_t RDR;            /* 0x24  Receive Data Register */
    __IO uint32_t TDR;            /* 0x28  Transmit Data Register */
    __IO uint32_t PRESC;          /* 0x2C  Prescaler Register */
} USART_ManualType;

// UART configuration enums
typedef enum {
    RX_ONLY = 0,
    TX_ONLY,
    RX_AND_TX
} UART_COMType;

typedef enum {
    __115200 = 0,
    __9600,
} UART_BaudRateType;

typedef enum {
    DMA_OK = 0,
    DMA_ERROR,
    DMA_BUSY
} DMA_StatusType;

typedef void (*USART_Callback_t)(char c);

typedef struct {
    UART_COMType comType;                 // configuration (RX/TX)
    UART_BaudRateType baudRate;           // selected baud enum
    USART_ManualType *regs;               // pointer to hardware register block
    USART_Callback_t callback;            // callback function for RX interrupt
} USART_HandleType;                       // High-level handle ("object")

// USART peripheral declarations
#define USART_1 ((USART_ManualType *)USART_1_BASE)
#define USART_2 ((USART_ManualType *)USART_2_BASE)
#define USART_3 ((USART_ManualType *)USART_3_BASE)
#define UART_4  ((USART_ManualType *)UART_4_BASE)
#define UART_5  ((USART_ManualType *)UART_5_BASE)

// Low-level (register) API
void USART_x_Write(USART_HandleType *handle, int ch);
char USART_x_Read(USART_HandleType *handle);
uint16_t BRR_Oversample_by_16(uint32_t fck_hz, uint32_t baud);
const char* GetPortName(USART_HandleType *handle);

// Interrupt API
void USART_EnableRXInterrupt(USART_HandleType *handle, USART_Callback_t callback);
void USART_DisableRXInterrupt(USART_HandleType *handle);
void USART_IRQHandler(USART_HandleType *handle);

// DMA API
void USART_EnableTXDMA(USART_HandleType *handle);
void USART_DisableTXDMA(USART_HandleType *handle);
void USART_EnableRXDMA(USART_HandleType *handle);
void USART_DisableRXDMA(USART_HandleType *handle);
DMA_StatusType USART_HandleTXDMA(USART_HandleType *handle);
DMA_StatusType USART_HandleRXDMA(USART_HandleType *handle);

// High-level (object-style) API
void USART_constructor(USART_HandleType *handle, USART_ManualType *regs, UART_COMType _comtype, UART_BaudRateType _baudrate);
void USART_Init(USART_HandleType *handle);
void USART_WriteChar(USART_HandleType *handle, int ch);
char USART_ReadChar(USART_HandleType *handle);
void USART_WriteString(USART_HandleType *handle, const char *str);
void USART_ReadString(USART_HandleType *handle, char *buffer, size_t maxLength);
void uart_write_hex8(USART_HandleType *handle, uint8_t byte);
void uart_write_hex32(USART_HandleType *handle, uint32_t word);

#endif // __UART_H
