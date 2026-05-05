/*
    Author: Baris Berk Zorba
    Date: June 2024 (ported to STM32C562RE, March 2026)
    UART driver for STM32C562RE (Cortex-M33, new-style USART with ISR/ICR/RDR/TDR)

    NUCLEO-C562RE: USART2 is connected to the ST-LINK Virtual COM Port.

    USART pins for STM32C562RE:
    USART1 -> PA9  (TX), PA10 (RX)  [AF7]
    USART2 -> PA2  (TX), PA3  (RX)  [AF7]  (ST-LINK VCP on NUCLEO board)
    USART3 -> PB10 (TX), PB11 (RX)  [AF7]
    UART4  -> PA0  (TX), PA1  (RX)  [AF8]
    UART5  -> PC12 (TX), PD2  (RX)  [AF8]
*/

#include "../inc/uart.h"

// Compute BRR for oversampling by 16
uint16_t BRR_Oversample_by_16(uint32_t fck_hz, uint32_t baud) {
    return (uint16_t)((fck_hz + (baud / 2U)) / baud);
}

void USART_constructor(USART_HandleType *huart, USART_ManualType *regs, UART_COMType _comtype, UART_BaudRateType _baudrate)
{
    huart->comType = _comtype;
    huart->baudRate = _baudrate;
    huart->regs = regs;

    USART_Init(huart);
}

// Static pointers used by IRQ wrappers so the IRQ handler can find the handle
static USART_HandleType *s_usart1_handle = NULL;
static USART_HandleType *s_usart2_handle = NULL;
static USART_HandleType *s_usart3_handle = NULL;
static USART_HandleType *s_uart4_handle  = NULL;
static USART_HandleType *s_uart5_handle  = NULL;

// High-level init: fill handle then configure hardware
void USART_Init(USART_HandleType *huart)
{
    // Disable USART before configuration
    huart->regs->CR1 = 0x0000;

    // Enable peripheral clock and configure GPIO pins
    if (huart->regs == USART_1) {
        // USART1 on APB2
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
        RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

        if (huart->comType == TX_ONLY || huart->comType == RX_AND_TX) {
            // PA9 TX: AF7
            GPIO_A->MODER &= ~MODER_PIN9_MASK;
            GPIO_A->MODER |=  MODER_PIN9_SET;
            GPIO_A->AFR[1] &= ~AFRH_PIN9_MASK;
            GPIO_A->AFR[1] |=  AFRH_PIN9_SET_AF7;
        }
        if (huart->comType == RX_ONLY || huart->comType == RX_AND_TX) {
            // PA10 RX: AF7
            GPIO_A->MODER &= ~MODER_PIN10_MASK;
            GPIO_A->MODER |=  MODER_PIN10_SET;
            GPIO_A->AFR[1] &= ~AFRH_PIN10_MASK;
            GPIO_A->AFR[1] |=  AFRH_PIN10_SET_AF7;
        }
    }
    else if (huart->regs == USART_2) {
        // USART2 on APB1 — ST-LINK VCP on NUCLEO-C562RE
        RCC->APB1LENR |= RCC_APB1LENR_USART2EN;
        RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

        if (huart->comType == TX_ONLY || huart->comType == RX_AND_TX) {
            // PA2 TX: AF7
            GPIO_A->MODER &= ~MODER_PIN2_MASK;
            GPIO_A->MODER |=  MODER_PIN2_SET;
            GPIO_A->AFR[0] &= ~AFRL_PIN2_MASK;
            GPIO_A->AFR[0] |=  AFRL_PIN2_SET_AF7;
        }
        if (huart->comType == RX_ONLY || huart->comType == RX_AND_TX) {
            // PA3 RX: AF7
            GPIO_A->MODER &= ~MODER_PIN3_MASK;
            GPIO_A->MODER |=  MODER_PIN3_SET;
            GPIO_A->AFR[0] &= ~AFRL_PIN3_MASK;
            GPIO_A->AFR[0] |=  AFRL_PIN3_SET_AF7;
        }
    }
    else if (huart->regs == USART_3) {
        // USART3 on APB1
        RCC->APB1LENR |= RCC_APB1LENR_USART3EN;
        RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOBEN;

        if (huart->comType == TX_ONLY || huart->comType == RX_AND_TX) {
            // PB10 TX: AF7
            GPIO_B->MODER &= ~MODER_PIN10_MASK;
            GPIO_B->MODER |=  MODER_PIN10_SET;
            GPIO_B->AFR[1] &= ~AFRH_PIN10_MASK;
            GPIO_B->AFR[1] |=  AFRH_PIN10_SET_AF7;
        }
        if (huart->comType == RX_ONLY || huart->comType == RX_AND_TX) {
            // PB11 RX: AF7
            GPIO_B->MODER &= ~MODER_PIN11_MASK;
            GPIO_B->MODER |=  MODER_PIN11_SET;
            GPIO_B->AFR[1] &= ~AFRH_PIN11_MASK;
            GPIO_B->AFR[1] |=  AFRH_PIN11_SET_AF7;
        }
    }
    else if (huart->regs == UART_4) {
        // UART4 on APB1
        RCC->APB1LENR |= RCC_APB1LENR_UART4EN;
        RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

        if (huart->comType == TX_ONLY || huart->comType == RX_AND_TX) {
            // PA0 TX: AF8
            GPIO_A->MODER &= ~MODER_PIN0_MASK;
            GPIO_A->MODER |=  MODER_PIN0_SET;
            GPIO_A->AFR[0] &= ~AFRL_PIN0_MASK;
            GPIO_A->AFR[0] |=  AFRL_PIN0_SET_AF8;
        }
        if (huart->comType == RX_ONLY || huart->comType == RX_AND_TX) {
            // PA1 RX: AF8
            GPIO_A->MODER &= ~MODER_PIN1_MASK;
            GPIO_A->MODER |=  MODER_PIN1_SET;
            GPIO_A->AFR[0] &= ~AFRL_PIN1_MASK;
            GPIO_A->AFR[0] |=  AFRL_PIN1_SET_AF8;
        }
    }
    else if (huart->regs == UART_5) {
        // UART5 on APB1 — TX on GPIOC, RX on GPIOD
        RCC->APB1LENR |= RCC_APB1LENR_UART5EN;

        if (huart->comType == TX_ONLY || huart->comType == RX_AND_TX) {
            // PC12 TX: AF8
            RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
            GPIO_C->MODER &= ~MODER_PIN12_MASK;
            GPIO_C->MODER |=  MODER_PIN12_SET;
            GPIO_C->AFR[1] &= ~AFRH_PIN12_MASK;
            GPIO_C->AFR[1] |=  AFRH_PIN12_SET_AF8;
        }
        if (huart->comType == RX_ONLY || huart->comType == RX_AND_TX) {
            // PD2 RX: AF8
            RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
            GPIO_D->MODER &= ~MODER_PIN2_MASK;
            GPIO_D->MODER |=  MODER_PIN2_SET;
            GPIO_D->AFR[0] &= ~AFRL_PIN2_MASK;
            GPIO_D->AFR[0] |=  AFRL_PIN2_SET_AF8;
        }
    }

    // Clear all pending status flags via ICR
    huart->regs->ICR = 0x00123BFFU;

    // Set baud rate (oversampling by 16)
    uint32_t baud_val = (huart->baudRate == __115200) ? 115200U : 9600U;
    uint32_t fck_hz = (huart->regs == USART_1) ? APB2_CLK_HZ : APB1_CLK_HZ;
    huart->regs->BRR = BRR_Oversample_by_16(fck_hz, baud_val);

    // 1 stop bit, no flow control
    huart->regs->CR2 = 0x0000;
    huart->regs->CR3 = 0x0000;

    // Enable TX/RX as requested, then UE last
    uint32_t cr1 = 0;
    switch (huart->comType) {
        case TX_ONLY:   cr1 = USART_CR1_TE; break;
        case RX_ONLY:   cr1 = USART_CR1_RE; break;
        case RX_AND_TX:
        default:        cr1 = (USART_CR1_TE | USART_CR1_RE); break;
    }

    // Enable USART (UE = bit 0) at the end of initialization
    cr1 |= USART_CR1_UE;
    huart->regs->CR1 = cr1;

    // Enable the interrupt in the NVIC, but DON'T enable it in the USART yet.
    IRQn_Type irq;
    if      (huart->regs == USART_1) irq = USART1_IRQn;
    else if (huart->regs == USART_2) irq = USART2_IRQn;
    else if (huart->regs == USART_3) irq = USART3_IRQn;
    else if (huart->regs == UART_4)  irq = UART4_IRQn;
    else if (huart->regs == UART_5)  irq = UART5_IRQn;
    else return;

    NVIC_SetPriority(irq, 0);
    NVIC_EnableIRQ(irq);

    // Register handle for IRQ wrappers
    if      (huart->regs == USART_1) s_usart1_handle = huart;
    else if (huart->regs == USART_2) s_usart2_handle = huart;
    else if (huart->regs == USART_3) s_usart3_handle = huart;
    else if (huart->regs == UART_4)  s_uart4_handle  = huart;
    else if (huart->regs == UART_5)  s_uart5_handle  = huart;
}

//////////////////////////////POLLING API (for both RX and TX)//////////////////////////////
void USART_x_Write(USART_HandleType *huart, int ch)
{
    // Wait until TXE (Transmit Data Register Empty) flag is set in ISR
    while (!(huart->regs->ISR & USART_ISR_TXE));
    // Write to TDR register
    huart->regs->TDR = (ch & 0xFF);
}

char USART_x_Read(USART_HandleType *huart)
{
    // Wait until RXNE (Read Data Register Not Empty) flag is set in ISR
    while (!(huart->regs->ISR & USART_ISR_RXNE));
    // Read from RDR register
    return (char)(huart->regs->RDR & 0xFF);
}
////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////INTERRUPT API (RX only)/////////////////////////////////////
// Call this to start using interrupts
void USART_EnableRXInterrupt(USART_HandleType *huart, USART_Callback_t callback) {
    huart->callback = callback;
    huart->regs->CR1 |= USART_CR1_RXNEIE;
}

// Call this to go back to polling mode
void USART_DisableRXInterrupt(USART_HandleType *huart) {
    huart->regs->CR1 &= ~USART_CR1_RXNEIE;
    huart->callback = NULL;
}

void USART_IRQHandler(USART_HandleType *huart) {
    uint32_t isr = huart->regs->ISR;

    // Error flags: ORE|NE|FE|PE — clear via ICR and discard any received byte.
    if (isr & USART_ISR_ERR_MASK) {
        huart->regs->ICR = (isr & USART_ISR_ERR_MASK);
        if (isr & USART_ISR_RXNE) {
            (void)huart->regs->RDR;
        }
        return;
    }

    // RXNE: read RDR to clear the flag, even without a callback.
    if (isr & USART_ISR_RXNE) {
        char c = (char)(huart->regs->RDR & 0xFF);
        if (huart->callback) {
            huart->callback(c);
        }
    }
}

// CMSIS IRQ wrappers: call the common handler with the registered handle
void USART1_IRQHandler(void) { if (s_usart1_handle) USART_IRQHandler(s_usart1_handle); }
void USART2_IRQHandler(void) { if (s_usart2_handle) USART_IRQHandler(s_usart2_handle); }
void USART3_IRQHandler(void) { if (s_usart3_handle) USART_IRQHandler(s_usart3_handle); }
void UART4_IRQHandler(void)  { if (s_uart4_handle)  USART_IRQHandler(s_uart4_handle); }
void UART5_IRQHandler(void)  { if (s_uart5_handle)  USART_IRQHandler(s_uart5_handle); }
////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////DMA API ////////////////////////////////////////////
void USART_EnableTXDMA(USART_HandleType *huart) {
    huart->regs->CR3 |= USART_CR3_DMAT;
}

void USART_DisableTXDMA(USART_HandleType *huart) {
    huart->regs->CR3 &= ~USART_CR3_DMAT;
}

void USART_EnableRXDMA(USART_HandleType *huart) {
    huart->regs->CR3 |= USART_CR3_DMAR;
}

void USART_DisableRXDMA(USART_HandleType *huart) {
    huart->regs->CR3 &= ~USART_CR3_DMAR;
}

DMA_StatusType USART_HandleTXDMA(USART_HandleType *huart) {
    // Placeholder for handling TX DMA completion or errors
    // In a real implementation, this would check DMA status flags and clear them
    return DMA_OK;
}

LPDMA_StatusType USART_WriteDMA(USART_HandleType *huart, DMA_HandleType *dma,
                                 const uint8_t *buf, uint16_t len) {
    LPDMA_StatusType st;

    if (huart == NULL || dma == NULL || buf == NULL || len == 0U) {
        return LPDMA_ERROR;
    }

    /* DMA_ConfigTransfer adds SINC automatically for MEMORY_TO_PERIPH direction.
     * dst is fixed at TDR (no DINC), byte width on both sides.
     * TR2 = REQSEL only — hardware-triggered by USART2 TXE, no SWREQ. */
    st = DMA_ConfigTransfer(dma,
                            (uintptr_t)buf,
                            (uintptr_t)&huart->regs->TDR,
                            len,
                            (LPDMA_TR1_SDW_LOG2_8 | LPDMA_TR1_DDW_LOG2_8),
                            LPDMA_REQSEL_USART2_TX);
    if (st != LPDMA_OK) {
        return st;
    }

    huart->regs->CR3 |= USART_CR3_DMAT;
    return DMA_Start(dma);
}

uint8_t USART_IsTXDMAComplete(DMA_HandleType *dma) {
    return DMA_IsTransferComplete(dma);
}

LPDMA_StatusType USART_ReadDMA(USART_HandleType *huart, DMA_HandleType *dma,
                                uint8_t *buf, uint16_t len) {
    LPDMA_StatusType st;
    if (huart == NULL || dma == NULL || buf == NULL || len == 0U) { return LPDMA_ERROR; }
    /* DMA_ConfigTransfer adds DINC automatically for PERIPH_TO_MEMORY direction.
     * src is fixed at RDR (no SINC), byte width on both sides. */
    st = DMA_ConfigTransfer(dma,
                            (uintptr_t)&huart->regs->RDR,
                            (uintptr_t)buf,
                            len,
                            (LPDMA_TR1_SDW_LOG2_8 | LPDMA_TR1_DDW_LOG2_8),
                            LPDMA_REQSEL_USART2_RX);
    if (st != LPDMA_OK) { return st; }
    huart->regs->CR3 |= USART_CR3_DMAR;
    return DMA_Start(dma);
}

uint8_t USART_IsRXDMAComplete(DMA_HandleType *dma) {
    return DMA_IsTransferComplete(dma);
}


////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////Object style wrappers///////////////////////////////////////
void writeString(USART_HandleType *huart, const char *str) {
    // send CRLF then string
    USART_x_Write(huart, '\r');
    USART_x_Write(huart, '\n');
    while (*str) {
        USART_x_Write(huart, *str++);
    }
}

void readString(USART_HandleType *huart, char *buffer, size_t maxLength) {
    size_t index = 0;
    char c;
    while (index < (maxLength - 1)) { // Leave space for null terminator
        c = USART_x_Read(huart);
        if (c == '\n' || c == '\r') { // Stop on newline or carriage return
            break;
        }
        buffer[index++] = c;
    }
    buffer[index] = '\0'; // Null-terminate the string
}

const char* GetPortName(USART_HandleType *huart) {
    if (!huart->regs) return "USART?";
    if (huart->regs == USART_1) return "USART1";
    if (huart->regs == USART_2) return "USART2";
    if (huart->regs == USART_3) return "USART3";
    if (huart->regs == UART_4)  return "UART4";
    if (huart->regs == UART_5)  return "UART5";
    return "USART?";
}

void USART_WriteChar(USART_HandleType *huart, int ch) {
    USART_x_Write(huart, ch);
}

char USART_ReadChar(USART_HandleType *huart) {
    return USART_x_Read(huart);
}

void USART_WriteString(USART_HandleType *huart, const char *str) {
    while (*str) {
        char c = *str++;
        if (c == '\n') {
            USART_x_Write(huart, '\r');
        }
        USART_x_Write(huart, c);
    }
}

void USART_ReadString(USART_HandleType *huart, char *buffer, size_t maxLength) {
    size_t i = 0; char c;
    while (i < maxLength - 1) {
        c = USART_ReadChar(huart);
        if (c == '\n' || c == '\r') {
            // Echo CR+LF so cursor moves to next line
            USART_WriteChar(huart, '\r');
            USART_WriteChar(huart, '\n');
            break;
        }
        USART_WriteChar(huart, c);  // echo typed character
        buffer[i++] = c;
    }
    buffer[i] = '\0';
}

void uart_write_hex8(USART_HandleType *huart, uint8_t byte) {
    const char hexDigits[] = "0123456789ABCDEF";
    USART_WriteChar(huart, hexDigits[(byte >> 4) & 0x0F]);
    USART_WriteChar(huart, hexDigits[byte & 0x0F]);
}

void uart_write_hex32(USART_HandleType *huart, uint32_t word) {
    uart_write_hex8(huart, (word >> 24) & 0xFF);
    uart_write_hex8(huart, (word >> 16) & 0xFF);
    uart_write_hex8(huart, (word >> 8) & 0xFF);
    uart_write_hex8(huart, word & 0xFF);
}
////////////////////////////////////////////////////////////////////////////////////////////
