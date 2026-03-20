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

void USART_constructor(USART_HandleType *handle, USART_ManualType *regs, UART_COMType _comtype, UART_BaudRateType _baudrate)
{
    handle->comType = _comtype;
    handle->baudRate = _baudrate;
    handle->regs = regs;

    USART_Init(handle);
}

// Static pointers used by IRQ wrappers so the IRQ handler can find the handle
static USART_HandleType *s_usart1_handle = NULL;
static USART_HandleType *s_usart2_handle = NULL;
static USART_HandleType *s_usart3_handle = NULL;
static USART_HandleType *s_uart4_handle  = NULL;
static USART_HandleType *s_uart5_handle  = NULL;

// High-level init: fill handle then configure hardware
void USART_Init(USART_HandleType *handle)
{
    // Disable USART before configuration
    handle->regs->CR1 = 0x0000;

    // Enable peripheral clock and configure GPIO pins
    if (handle->regs == USART_1) {
        // USART1 on APB2
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
        RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

        if (handle->comType == TX_ONLY || handle->comType == RX_AND_TX) {
            // PA9 TX: AF7
            GPIO_A->MODER &= ~MODER_PIN9_MASK;
            GPIO_A->MODER |=  MODER_PIN9_SET;
            GPIO_A->AFR[1] &= ~AFRH_PIN9_MASK;
            GPIO_A->AFR[1] |=  AFRH_PIN9_SET_AF7;
        }
        if (handle->comType == RX_ONLY || handle->comType == RX_AND_TX) {
            // PA10 RX: AF7
            GPIO_A->MODER &= ~MODER_PIN10_MASK;
            GPIO_A->MODER |=  MODER_PIN10_SET;
            GPIO_A->AFR[1] &= ~AFRH_PIN10_MASK;
            GPIO_A->AFR[1] |=  AFRH_PIN10_SET_AF7;
        }
    }
    else if (handle->regs == USART_2) {
        // USART2 on APB1 — ST-LINK VCP on NUCLEO-C562RE
        RCC->APB1LENR |= RCC_APB1LENR_USART2EN;
        RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

        if (handle->comType == TX_ONLY || handle->comType == RX_AND_TX) {
            // PA2 TX: AF7
            GPIO_A->MODER &= ~MODER_PIN2_MASK;
            GPIO_A->MODER |=  MODER_PIN2_SET;
            GPIO_A->AFR[0] &= ~AFRL_PIN2_MASK;
            GPIO_A->AFR[0] |=  AFRL_PIN2_SET_AF7;
        }
        if (handle->comType == RX_ONLY || handle->comType == RX_AND_TX) {
            // PA3 RX: AF7
            GPIO_A->MODER &= ~MODER_PIN3_MASK;
            GPIO_A->MODER |=  MODER_PIN3_SET;
            GPIO_A->AFR[0] &= ~AFRL_PIN3_MASK;
            GPIO_A->AFR[0] |=  AFRL_PIN3_SET_AF7;
        }
    }
    else if (handle->regs == USART_3) {
        // USART3 on APB1
        RCC->APB1LENR |= RCC_APB1LENR_USART3EN;
        RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOBEN;

        if (handle->comType == TX_ONLY || handle->comType == RX_AND_TX) {
            // PB10 TX: AF7
            GPIO_B->MODER &= ~MODER_PIN10_MASK;
            GPIO_B->MODER |=  MODER_PIN10_SET;
            GPIO_B->AFR[1] &= ~AFRH_PIN10_MASK;
            GPIO_B->AFR[1] |=  AFRH_PIN10_SET_AF7;
        }
        if (handle->comType == RX_ONLY || handle->comType == RX_AND_TX) {
            // PB11 RX: AF7
            GPIO_B->MODER &= ~MODER_PIN11_MASK;
            GPIO_B->MODER |=  MODER_PIN11_SET;
            GPIO_B->AFR[1] &= ~AFRH_PIN11_MASK;
            GPIO_B->AFR[1] |=  AFRH_PIN11_SET_AF7;
        }
    }
    else if (handle->regs == UART_4) {
        // UART4 on APB1
        RCC->APB1LENR |= RCC_APB1LENR_UART4EN;
        RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

        if (handle->comType == TX_ONLY || handle->comType == RX_AND_TX) {
            // PA0 TX: AF8
            GPIO_A->MODER &= ~MODER_PIN0_MASK;
            GPIO_A->MODER |=  MODER_PIN0_SET;
            GPIO_A->AFR[0] &= ~AFRL_PIN0_MASK;
            GPIO_A->AFR[0] |=  AFRL_PIN0_SET_AF8;
        }
        if (handle->comType == RX_ONLY || handle->comType == RX_AND_TX) {
            // PA1 RX: AF8
            GPIO_A->MODER &= ~MODER_PIN1_MASK;
            GPIO_A->MODER |=  MODER_PIN1_SET;
            GPIO_A->AFR[0] &= ~AFRL_PIN1_MASK;
            GPIO_A->AFR[0] |=  AFRL_PIN1_SET_AF8;
        }
    }
    else if (handle->regs == UART_5) {
        // UART5 on APB1 — TX on GPIOC, RX on GPIOD
        RCC->APB1LENR |= RCC_APB1LENR_UART5EN;

        if (handle->comType == TX_ONLY || handle->comType == RX_AND_TX) {
            // PC12 TX: AF8
            RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
            GPIO_C->MODER &= ~MODER_PIN12_MASK;
            GPIO_C->MODER |=  MODER_PIN12_SET;
            GPIO_C->AFR[1] &= ~AFRH_PIN12_MASK;
            GPIO_C->AFR[1] |=  AFRH_PIN12_SET_AF8;
        }
        if (handle->comType == RX_ONLY || handle->comType == RX_AND_TX) {
            // PD2 RX: AF8
            RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
            GPIO_D->MODER &= ~MODER_PIN2_MASK;
            GPIO_D->MODER |=  MODER_PIN2_SET;
            GPIO_D->AFR[0] &= ~AFRL_PIN2_MASK;
            GPIO_D->AFR[0] |=  AFRL_PIN2_SET_AF8;
        }
    }

    // Clear all pending status flags via ICR
    handle->regs->ICR = 0x00123BFFU;

    // Set baud rate (oversampling by 16)
    uint32_t baud_val = (handle->baudRate == __115200) ? 115200U : 9600U;
    uint32_t fck_hz = (handle->regs == USART_1) ? APB2_CLK_HZ : APB1_CLK_HZ;
    handle->regs->BRR = BRR_Oversample_by_16(fck_hz, baud_val);

    // 1 stop bit, no flow control
    handle->regs->CR2 = 0x0000;
    handle->regs->CR3 = 0x0000;

    // Enable TX/RX as requested, then UE last
    uint32_t cr1 = 0;
    switch (handle->comType) {
        case TX_ONLY:   cr1 = USART_CR1_TE; break;
        case RX_ONLY:   cr1 = USART_CR1_RE; break;
        case RX_AND_TX:
        default:        cr1 = (USART_CR1_TE | USART_CR1_RE); break;
    }

    // Enable USART (UE = bit 0) at the end of initialization
    cr1 |= USART_CR1_UE;
    handle->regs->CR1 = cr1;

    // Enable the interrupt in the NVIC, but DON'T enable it in the USART yet.
    IRQn_Type irq;
    if      (handle->regs == USART_1) irq = USART1_IRQn;
    else if (handle->regs == USART_2) irq = USART2_IRQn;
    else if (handle->regs == USART_3) irq = USART3_IRQn;
    else if (handle->regs == UART_4)  irq = UART4_IRQn;
    else if (handle->regs == UART_5)  irq = UART5_IRQn;
    else return;

    NVIC_SetPriority(irq, 0);
    NVIC_EnableIRQ(irq);

    // Register handle for IRQ wrappers
    if      (handle->regs == USART_1) s_usart1_handle = handle;
    else if (handle->regs == USART_2) s_usart2_handle = handle;
    else if (handle->regs == USART_3) s_usart3_handle = handle;
    else if (handle->regs == UART_4)  s_uart4_handle  = handle;
    else if (handle->regs == UART_5)  s_uart5_handle  = handle;
}

//////////////////////////////POLLING API (for both RX and TX)//////////////////////////////
void USART_x_Write(USART_HandleType *handle, int ch)
{
    // Wait until TXE (Transmit Data Register Empty) flag is set in ISR
    while (!(handle->regs->ISR & USART_ISR_TXE));
    // Write to TDR register
    handle->regs->TDR = (ch & 0xFF);
}

char USART_x_Read(USART_HandleType *handle)
{
    // Wait until RXNE (Read Data Register Not Empty) flag is set in ISR
    while (!(handle->regs->ISR & USART_ISR_RXNE));
    // Read from RDR register
    return (char)(handle->regs->RDR & 0xFF);
}
////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////INTERRUPT API (RX only)/////////////////////////////////////
// Call this to start using interrupts
void USART_EnableRXInterrupt(USART_HandleType *handle, USART_Callback_t callback) {
    handle->callback = callback;
    handle->regs->CR1 |= USART_CR1_RXNEIE;
}

// Call this to go back to polling mode
void USART_DisableRXInterrupt(USART_HandleType *handle) {
    handle->regs->CR1 &= ~USART_CR1_RXNEIE;
    handle->callback = NULL;
}

void USART_IRQHandler(USART_HandleType *handle) {
    uint32_t isr = handle->regs->ISR;

    // Error flags: ORE|NE|FE|PE — clear via ICR and discard any received byte.
    if (isr & USART_ISR_ERR_MASK) {
        handle->regs->ICR = (isr & USART_ISR_ERR_MASK);
        if (isr & USART_ISR_RXNE) {
            (void)handle->regs->RDR;
        }
        return;
    }

    // RXNE: read RDR to clear the flag, even without a callback.
    if (isr & USART_ISR_RXNE) {
        char c = (char)(handle->regs->RDR & 0xFF);
        if (handle->callback) {
            handle->callback(c);
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




////////////////////////////////Object style wrappers///////////////////////////////////////
void writeString(USART_HandleType *handle, const char *str) {
    // send CRLF then string
    USART_x_Write(handle, '\r');
    USART_x_Write(handle, '\n');
    while (*str) {
        USART_x_Write(handle, *str++);
    }
}

void readString(USART_HandleType *handle, char *buffer, size_t maxLength) {
    size_t index = 0;
    char c;
    while (index < (maxLength - 1)) { // Leave space for null terminator
        c = USART_x_Read(handle);
        if (c == '\n' || c == '\r') { // Stop on newline or carriage return
            break;
        }
        buffer[index++] = c;
    }
    buffer[index] = '\0'; // Null-terminate the string
}

const char* GetPortName(USART_HandleType *handle) {
    if (!handle->regs) return "USART?";
    if (handle->regs == USART_1) return "USART1";
    if (handle->regs == USART_2) return "USART2";
    if (handle->regs == USART_3) return "USART3";
    if (handle->regs == UART_4)  return "UART4";
    if (handle->regs == UART_5)  return "UART5";
    return "USART?";
}

void USART_WriteChar(USART_HandleType *handle, int ch) {
    USART_x_Write(handle, ch);
}

char USART_ReadChar(USART_HandleType *handle) {
    return USART_x_Read(handle);
}

void USART_WriteString(USART_HandleType *handle, const char *str) {
    while (*str) {
        char c = *str++;
        if (c == '\n') {
            USART_x_Write(handle, '\r');
        }
        USART_x_Write(handle, c);
    }
}

void USART_ReadString(USART_HandleType *handle, char *buffer, size_t maxLength) {
    size_t i = 0; char c;
    while (i < maxLength - 1) {
        c = USART_ReadChar(handle);
        if (c == '\n' || c == '\r') {
            // Echo CR+LF so cursor moves to next line
            USART_WriteChar(handle, '\r');
            USART_WriteChar(handle, '\n');
            break;
        }
        USART_WriteChar(handle, c);  // echo typed character
        buffer[i++] = c;
    }
    buffer[i] = '\0';
}
////////////////////////////////////////////////////////////////////////////////////////////
