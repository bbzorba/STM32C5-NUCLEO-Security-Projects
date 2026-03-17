#ifndef __UART_H
#define __UART_H

#include <stddef.h>
#include "../../GPIO/inc/gpio.h"

#define __IO volatile
#define __NVIC_PRIO_BITS 4

/*
 * STM32C5 defaults used by this register-level UART driver.
 * Override these from build flags if your exact STM32C5 variant differs.
 */
#ifndef STM32C5_IRQ_USART1
#define STM32C5_IRQ_USART1 27
#endif
#ifndef STM32C5_IRQ_USART2
#define STM32C5_IRQ_USART2 28
#endif
#ifndef STM32C5_IRQ_USART3
#define STM32C5_IRQ_USART3 29
#endif
#ifndef STM32C5_IRQ_UART4
#define STM32C5_IRQ_UART4 52
#endif
#ifndef STM32C5_IRQ_UART5
#define STM32C5_IRQ_UART5 53
#endif
#ifndef STM32C5_IRQ_USART6
#define STM32C5_IRQ_USART6 71
#endif

/* Minimal IRQn_Type covering the USART/UART IRQs used by this driver. */
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

    USART1_IRQn                 = STM32C5_IRQ_USART1,
    USART2_IRQn                 = STM32C5_IRQ_USART2,
    USART3_IRQn                 = STM32C5_IRQ_USART3,
    UART4_IRQn                  = STM32C5_IRQ_UART4,
    UART5_IRQn                  = STM32C5_IRQ_UART5,
    USART6_IRQn                 = STM32C5_IRQ_USART6
} IRQn_Type;
#endif

#include "core_cm4.h"

// Base addresses
/*
 * USART bases are kept compatible with common STM32 mappings.
 * If your STM32C5 part differs, override these at compile time.
 */
#ifndef USART_1_BASE
#define USART_1_BASE 0x40013800U
#endif
#ifndef USART_2_BASE
#define USART_2_BASE 0x40004400U
#endif
#ifndef USART_3_BASE
#define USART_3_BASE 0x40004800U
#endif
#ifndef UART_4_BASE
#define UART_4_BASE 0x40004C00U
#endif
#ifndef UART_5_BASE
#define UART_5_BASE 0x40005000U
#endif
#ifndef USART_6_BASE
#define USART_6_BASE 0x40011400U
#endif

/* RCC APB clock register offsets for USART peripherals (RCC_BASE comes from gpio.h). */
#ifndef RCC_APB1ENR1_OFFSET
#define RCC_APB1ENR1_OFFSET 0x58U
#endif
#ifndef RCC_APB2ENR_OFFSET
#define RCC_APB2ENR_OFFSET 0x60U
#endif

#define RCC_APB1ENR1_REG  (*((__IO uint32_t *)(RCC_BASE + RCC_APB1ENR1_OFFSET)))
#define RCC_APB2ENR_REG   (*((__IO uint32_t *)(RCC_BASE + RCC_APB2ENR_OFFSET)))

// USART clock enable bits
#define RCC_APB2ENR_USART_1EN  ((uint32_t)(1UL << 14))
#define RCC_APB1ENR1_USART_2EN ((uint32_t)(1UL << 17))
#define RCC_APB1ENR1_USART_3EN ((uint32_t)(1UL << 18))
#define RCC_APB1ENR1_UART_4EN  ((uint32_t)(1UL << 19))
#define RCC_APB1ENR1_UART_5EN  ((uint32_t)(1UL << 20))
#define RCC_APB2ENR_USART_6EN  ((uint32_t)(1UL <<  5))

// Default peripheral clocks (Hz) used for baud calculation when system clock
// is not explicitly configured elsewhere. On STM32F4 Discovery, using HSI by
// default gives APB1=APB2=16 MHz.
#ifndef APB1_CLK_HZ
#define APB1_CLK_HZ 16000000U
#endif
#ifndef APB2_CLK_HZ
#define APB2_CLK_HZ 16000000U
#endif

// USART BRR register bit definitions
#define BRR_CNF1_115200 0x08B                               // 115200 @ 16 MHz, OVER8=0
#define BRR_CNF2_9600  0x683                                // 9600 @ 16 MHz, OVER8=0

//USART CR1, CR2 & CR3 register bit definitions
#define CR2_CNF1 0x0000                                     // 1 stop bit, no-op
#define CR3_CNF1 0x0000                                     // No flow control, no-op
#define USART_2_CR1_DIS 0x0000                              // Disable USART_2
#define USART_CR1_EN 0x0001                                 // UE bit
#define USART_CR1_RX_EN 0x0004                              // RE bit
#define USART_CR1_TX_EN 0x0008                              // TE bit
#define USART_CR1_RXNEIE  0x0020                            // RXNEIE bit

// USART ISR/ICR register bit definitions
#define USART_ISR_RX_NOT_EMP 0x0020                         // RXNE_RXFNE
#define USART_ISR_TX_EMP 0x0080                             // TXE_TXFNF
#define USART_ISR_PE  0x0001
#define USART_ISR_FE  0x0002
#define USART_ISR_NE  0x0004
#define USART_ISR_ORE 0x0008
#define USART_ICR_PECF 0x0001
#define USART_ICR_FECF 0x0002
#define USART_ICR_NECF 0x0004
#define USART_ICR_ORECF 0x0008

/* USART registers structure */
typedef struct
{
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t CR3;
    __IO uint32_t BRR;
    __IO uint32_t GTPR;
    __IO uint32_t RTOR;
    __IO uint32_t RQR;
    __IO uint32_t ISR;
    __IO uint32_t ICR;
    __IO uint32_t RDR;
    __IO uint32_t TDR;
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
#define USART_6 ((USART_ManualType *)USART_6_BASE)

// GPIO types, port macros, MODER/AFR bit definitions come from gpio.h (included above)

// Low-level (register) API
void USART_x_Write(USART_HandleType *handle, int ch);
char USART_x_Read(USART_HandleType *handle);
uint16_t BRR_Oversample_by_16(uint32_t fck_hz, uint32_t baud);
const char* GetPortName(USART_HandleType *handle);

// Interrupt API
void USART_EnableRXInterrupt(USART_HandleType *handle, USART_Callback_t callback);
void USART_DisableRXInterrupt(USART_HandleType *handle);
void USART_IRQHandler(USART_HandleType *handle);
// High-level (object-style) API
void USART_constructor(USART_HandleType *handle, USART_ManualType *regs, UART_COMType _comtype, UART_BaudRateType _baudrate);
void USART_Init(USART_HandleType *handle);
void USART_WriteChar(USART_HandleType *handle, int ch);
char USART_ReadChar(USART_HandleType *handle);
void USART_WriteString(USART_HandleType *handle, const char *str);
void USART_ReadString(USART_HandleType *handle, char *buffer, size_t maxLength);

#endif // __UART_H
