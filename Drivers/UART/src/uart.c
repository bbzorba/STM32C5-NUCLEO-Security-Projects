/*
    Author: Baris Berk Zorba
    Date: June 2024
    UART driver adapted for STM32C5-style register layout (USART v2).
    This driver provides an interface for initializing and using the UART peripherals
    on STM32 microcontrollers.

    USART pins for STM32F4xx series:
    USART1 -> PB6 (TX), PB7 (RX)
    USART2 -> PA2 (TX), PA3 (RX) or PD5 (TX), PD6 (RX)
    USART3 -> PB10 (TX), PB11 (RX) or PD8 (TX), PD9 (RX) or PC10 (TX), PC11(RX)
    UART4 -> PA0 (TX), PA1 (RX) or PC10 (TX), PC11 (RX)
    UART5 -> PC12 (TX), PD2 (RX)
    USART6 -> PC6 (TX), PC7 (RX)
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
static USART_HandleType *s_usart6_handle = NULL;

// High-level init: fill handle then configure hardware
void USART_Init(USART_HandleType *handle)
{
    // USART TX pin configuration
    if (handle->comType == TX_ONLY || handle->comType == RX_AND_TX) 
    {
        if (handle->regs == USART_1) {
            RCC_APB2ENR_REG |= RCC_APB2ENR_USART_1EN;                   // USART1 clock (APB2)

            // configuration for USART1 TX on PB6
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOBEN;                     // GPIOB clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            GPIO_B->MODER &= ~MODER_PIN6_MASK;                          // clear PB6
            GPIO_B->MODER |=  MODER_PIN6_SET;                           // Set AF for PB6 (bits 13:12)
            GPIO_B->AFR[0] &= ~AFRL_PIN6_MASK;                          // clear AFRL[27:24]
            GPIO_B->AFR[0] |=  AFRL_PIN6_SET_AF7;                       // AF7 for PB6

            // configuration for USART1 TX on PA9
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOAEN;                     // GPIOA clock
            GPIO_A->MODER &= ~MODER_PIN9_MASK;                          // clear PA9
            GPIO_A->MODER |=  MODER_PIN9_SET;                           // AF for PA9
            GPIO_A->AFR[1] &= ~AFRH_PIN9_MASK;                          // clear AFRH[11:8]
            GPIO_A->AFR[1] |=  AFRH_PIN9_SET_AF7;                       // AF7 for PA9
        }
        else if (handle->regs == USART_2) {
            RCC_APB1ENR1_REG |= RCC_APB1ENR1_USART_2EN;                 // USART2 clock (APB1)
            
            // configuration for USART2 TX on PA2
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOAEN;                     // GPIOA clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            GPIO_A->MODER &= ~MODER_PIN2_MASK;                          // clear PA2
            GPIO_A->MODER |=  MODER_PIN2_SET;                           // AF for PA2
            GPIO_A->AFR[0] &= ~AFRL_PIN2_MASK;                          // clear AFRL[11:8]
            GPIO_A->AFR[0] |=  AFRL_PIN2_SET_AF7;                       // AF7 for PA2

            // configuration for USART2 TX on PD5
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIODEN;                     // GPIOD clock
            GPIO_D->MODER &= ~MODER_PIN5_MASK;                          // clear PD5
            GPIO_D->MODER |=  MODER_PIN5_SET;                           // AF for PD5
            GPIO_D->AFR[0] &= ~AFRL_PIN5_MASK;                          // clear AFRL[23:20]
            GPIO_D->AFR[0] |=  AFRL_PIN5_SET_AF7;                       // AF7 for PD5
        }
        else if (handle->regs == USART_3) {
            RCC_APB1ENR1_REG |= RCC_APB1ENR1_USART_3EN;                 // USART3 clock (APB1)

            // configuration for USART3 TX on PB10
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOBEN;                     // GPIOB clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            GPIO_B->MODER &= ~MODER_PIN10_MASK;                         // clear PB10
            GPIO_B->MODER |=  MODER_PIN10_SET;                          // AF for PB10
            GPIO_B->AFR[1] &= ~AFRH_PIN10_MASK;                         // clear AFRH[11:8]
            GPIO_B->AFR[1] |=  AFRH_PIN10_SET_AF7;                      // AF7 for PB10

            // configuration for USART3 TX on PD8
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIODEN;                     // GPIOD clock
            GPIO_D->MODER &= ~MODER_PIN8_MASK;                          // clear PD8
            GPIO_D->MODER |=  MODER_PIN8_SET;                           // AF for PD8
            GPIO_D->AFR[1] &= ~AFRH_PIN8_MASK;                          // clear AFRH[3:0]
            GPIO_D->AFR[1] |=  AFRH_PIN8_SET_AF7;                       // AF7 for PD8

            // configuration for USART3 TX on PC10
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOCEN;                     // GPIOC clock
            GPIO_C->MODER &= ~MODER_PIN10_MASK;                         // clear PC10
            GPIO_C->MODER |=  MODER_PIN10_SET;                          // AF for PC10
            GPIO_C->AFR[1] &= ~AFRH_PIN10_MASK;                         // clear AFRH[11:8]
            GPIO_C->AFR[1] |=  AFRH_PIN10_SET_AF7;                      // AF7 for PC10
        }
        else if (handle->regs == UART_4) {
            RCC_APB1ENR1_REG |= RCC_APB1ENR1_UART_4EN;                  // UART4 clock (APB1)

            // configuration for UART4 TX on PA0
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOAEN;                     // GPIOA clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            GPIO_A->MODER &= ~MODER_PIN0_MASK;                          // clear PA0
            GPIO_A->MODER |=  MODER_PIN0_SET;                           // AF for PA0
            GPIO_A->AFR[0] &= ~AFRL_PIN0_MASK;                          // clear AFRL[3:0]
            GPIO_A->AFR[0] |=  AFRL_PIN0_SET_AF8;                       // AF8 for PA0

            // configuration for UART4 TX on PC10
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOCEN;                     // GPIOC clock
            GPIO_C->MODER &= ~MODER_PIN10_MASK;                         // clear PC10
            GPIO_C->MODER |=  MODER_PIN10_SET;                          // AF for PC10
            GPIO_C->AFR[1] &= ~AFRH_PIN10_MASK;                         // clear AFRH[11:8]
            GPIO_C->AFR[1] |=  AFRH_PIN10_SET_AF8;                      // AF8 for PC10
        }
        else if (handle->regs == UART_5) {
            RCC_APB1ENR1_REG |= RCC_APB1ENR1_UART_5EN;                  // UART5 clock (APB1)

            // configuration for UART5 TX on PC12
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOCEN;                     // GPIOC clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            GPIO_C->MODER &= ~MODER_PIN12_MASK;                         // clear PC12
            GPIO_C->MODER |=  MODER_PIN12_SET;                          // AF for PC12
            GPIO_C->AFR[1] &= ~AFRH_PIN12_MASK;                         // clear AFRH[15:12]
            GPIO_C->AFR[1] |=  AFRH_PIN12_SET_AF8;                      // AF8 for PC12
        }
        else if (handle->regs == USART_6) {
            RCC_APB2ENR_REG |= RCC_APB2ENR_USART_6EN;                   // USART6 clock (APB2)

            // configuration for USART6 TX on PC6
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOCEN;                     // GPIOC clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            GPIO_C->MODER &= ~MODER_PIN6_MASK;                          // clear PC6
            GPIO_C->MODER |=  MODER_PIN6_SET;                           // AF for PC6
            GPIO_C->AFR[0] &= ~AFRL_PIN6_MASK;                          // clear AFRL[27:24]
            GPIO_C->AFR[0] |=  AFRL_PIN6_SET_AF8;                       // AF8 for PC6
        }
    }

    // USART RX pin configuration
    if (handle->comType == RX_ONLY || handle->comType == RX_AND_TX) 
    {
        if (handle->regs == USART_1) {
            RCC_APB2ENR_REG |= RCC_APB2ENR_USART_1EN;                   // USART1 clock (APB2)

            // configuration for USART1 RX on PB7
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOBEN;                     // GPIOB clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            GPIO_B->MODER &= ~MODER_PIN7_MASK;                          // clear PB7
            GPIO_B->MODER |=  MODER_PIN7_SET;                           // AF for PB7
            GPIO_B->AFR[0] &= ~AFRL_PIN7_MASK;                          // clear AFRL[31:28]
            GPIO_B->AFR[0] |=  AFRL_PIN7_SET_AF7;                       // AF7 for PB7

            // configuration for USART1 RX on PA10
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOAEN;                     // GPIOA clock
            GPIO_A->MODER &= ~MODER_PIN10_MASK;                         // clear PA10
            GPIO_A->MODER |=  MODER_PIN10_SET;                          // AF for PA10
            GPIO_A->AFR[1] &= ~AFRH_PIN10_MASK;                         // clear AFRH[11:8]
            GPIO_A->AFR[1] |=  AFRH_PIN10_SET_AF7;                      // AF7 for PA10
        }
        else if (handle->regs == USART_2) {
            RCC_APB1ENR1_REG |= RCC_APB1ENR1_USART_2EN;                 // USART2 clock (APB1)
            
            // configuration for USART2 RX on PA3
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOAEN;                     // GPIOA clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            GPIO_A->MODER &= ~MODER_PIN3_MASK;                          // clear PA3
            GPIO_A->MODER |=  MODER_PIN3_SET;                           // AF for PA3
            GPIO_A->AFR[0] &= ~AFRL_PIN3_MASK;                          // clear AFRL[15:12]
            GPIO_A->AFR[0] |=  AFRL_PIN3_SET_AF7;                       // AF7 for PA3

            // configuration for USART2 RX on PD6
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIODEN;                     // GPIOD clock
            GPIO_D->MODER &= ~MODER_PIN6_MASK;                          // clear PD6
            GPIO_D->MODER |=  MODER_PIN6_SET;                           // AF for PD6
            GPIO_D->AFR[0] &= ~AFRL_PIN6_MASK;                          // clear AFRL[27:24]
            GPIO_D->AFR[0] |=  AFRL_PIN6_SET_AF7;                       // AF7 for PD6
        }
        else if (handle->regs == USART_3) {
            RCC_APB1ENR1_REG |= RCC_APB1ENR1_USART_3EN;                 // USART3 clock (APB1)

            // configuration for USART3 RX on PB11
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOBEN;                     // GPIOB clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            GPIO_B->MODER &= ~MODER_PIN11_MASK;                         // clear PB11
            GPIO_B->MODER |=  MODER_PIN11_SET;                          // AF for PB11
            GPIO_B->AFR[1] &= ~AFRH_PIN11_MASK;                         // clear AFRH[15:12]
            GPIO_B->AFR[1] |=  AFRH_PIN11_SET_AF7;                      // AF7 for PB11

            // configuration for USART3 RX on PD9
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIODEN;                     // GPIOD clock
            GPIO_D->MODER &= ~MODER_PIN9_MASK;                          // clear PD9
            GPIO_D->MODER |=  MODER_PIN9_SET;                           // AF for PD9
            GPIO_D->AFR[1] &= ~AFRH_PIN9_MASK;                          // clear AFRH[15:12]
            GPIO_D->AFR[1] |=  AFRH_PIN9_SET_AF7;                       // AF7 for PD9

            //configuration for USART3 RX on PC11
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOCEN;                     // GPIOC clock
            GPIO_C->MODER &= ~MODER_PIN11_MASK;                         // clear PC11
            GPIO_C->MODER |=  MODER_PIN11_SET;                          // AF for PC11
            GPIO_C->AFR[1] &= ~AFRH_PIN11_MASK;                         // clear AFRH[15:12]
            GPIO_C->AFR[1] |=  AFRH_PIN11_SET_AF7;                      // AF7 for PC11
        }
        else if (handle->regs == UART_4) {
            RCC_APB1ENR1_REG |= RCC_APB1ENR1_UART_4EN;                  // UART4 clock (APB1)

            // configuration for UART4 RX on PA1
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOAEN;                     // GPIOA clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            // Enable pull-up on RX to keep line idle-high and reduce noise
            GPIO_A->MODER &= ~MODER_PIN1_MASK;                          // clear PA1
            GPIO_A->MODER |=  MODER_PIN1_SET;                           // AF for PA1
            GPIO_A->AFR[0] &= ~AFRL_PIN1_MASK;                          // clear AFRL[7:4]
            GPIO_A->AFR[0] |=  AFRL_PIN1_SET_AF8;                       // AF8 for PA1

            // configuration for UART4 RX on PC11
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOCEN;                     // GPIOC clock
            GPIO_C->MODER &= ~MODER_PIN11_MASK;                         // clear PC11
            GPIO_C->MODER |=  MODER_PIN11_SET;                          // AF for PC11
            GPIO_C->AFR[1] &= ~AFRH_PIN11_MASK;                         // clear AFRH[15:12]
            GPIO_C->AFR[1] |=  AFRH_PIN11_SET_AF8;                      // AF8 for PC11
        }
        else if (handle->regs == UART_5) {
            RCC_APB1ENR1_REG |= RCC_APB1ENR1_UART_5EN;                  // UART5 clock (APB1)

            // configuration for UART5 RX on PD2
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIODEN;                     // GPIOD clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            GPIO_D->MODER &= ~MODER_PIN2_MASK;                          // clear PD2
            GPIO_D->MODER |=  MODER_PIN2_SET;                           // AF for PD2
            GPIO_D->AFR[0] &= ~AFRL_PIN2_MASK;                          // clear AFRL[11:8]
            GPIO_D->AFR[0] |=  AFRL_PIN2_SET_AF8;                       // AF8 for PD2
        }
        else if (handle->regs == USART_6) {
            RCC_APB2ENR_REG |= RCC_APB2ENR_USART_6EN;                   // USART6 clock (APB2)

            // configuration for USART6 RX on PC7
            RCC_AHB2ENR_REG |= RCC_AHB2ENR_GPIOCEN;                     // GPIOC clock
            handle->regs->CR1 = 0x0000;                                 // Disable USART before configuration
            GPIO_C->MODER &= ~MODER_PIN7_MASK;                          // clear PC7
            GPIO_C->MODER |=  MODER_PIN7_SET;                           // AF for PC7
            GPIO_C->AFR[0] &= ~AFRL_PIN7_MASK;                          // clear AFRL[31:28]
            GPIO_C->AFR[0] |=  AFRL_PIN7_SET_AF8;                       // AF8 for PC7
        }
    }

    // Clear any stale status by a dummy ISR/RDR read, then set baud
    (void)handle->regs->ISR;
    (void)handle->regs->RDR;
    // Baud rate before enabling UE; derive from bus clock.
    uint32_t baud_val = (handle->baudRate == __115200) ? 115200U : 9600U;
    uint32_t fck_hz = (handle->regs == USART_1 || handle->regs == USART_6) ? APB2_CLK_HZ : APB1_CLK_HZ;
    handle->regs->BRR = BRR_Oversample_by_16(fck_hz, baud_val);
    // 1 stop bit, no flow control
    handle->regs->CR2 = CR2_CNF1;
    handle->regs->CR3 = CR3_CNF1;

    // Enable TX/RX as requested, then UE last
    switch (handle->comType) {
        case TX_ONLY:
            handle->regs->CR1 = USART_CR1_TX_EN;
            break;
        case RX_ONLY:
            handle->regs->CR1 = USART_CR1_RX_EN;
            break;
        case RX_AND_TX:
        default:
            handle->regs->CR1 = (USART_CR1_TX_EN | USART_CR1_RX_EN);
            break;
    }

    // Enable USART at the end of initialization
    handle->regs->CR1 |= USART_CR1_EN;

    // Enable the interrupt in the NVIC, but DON'T enable it in the USART yet.
    IRQn_Type irq;
    if (handle->regs == USART_1)      irq = USART1_IRQn;
    else if (handle->regs == USART_2) irq = USART2_IRQn;
    else if (handle->regs == USART_3) irq = USART3_IRQn;
    else if (handle->regs == UART_4)  irq = UART4_IRQn;
    else if (handle->regs == UART_5)  irq = UART5_IRQn;
    else if (handle->regs == USART_6) irq = USART6_IRQn;
    else return;

    NVIC_SetPriority(irq, 0); // Set a priority
    NVIC_EnableIRQ(irq);      // Tell CPU to listen to this UART line

    // register handle for IRQ wrappers
    if (handle->regs == USART_1)      s_usart1_handle = handle;
    else if (handle->regs == USART_2) s_usart2_handle = handle;
    else if (handle->regs == USART_3) s_usart3_handle = handle;
    else if (handle->regs == UART_4)  s_uart4_handle  = handle;
    else if (handle->regs == UART_5)  s_uart5_handle  = handle;
    else if (handle->regs == USART_6) s_usart6_handle = handle;
}

//////////////////////////////POLLING API (for both RX and TX)//////////////////////////////
void USART_x_Write(USART_HandleType *handle, int ch)
{
    //1. wait until TX register is empty
    while(!(handle->regs->ISR & USART_ISR_TX_EMP));
    //2. write to TDR register
    handle->regs->TDR = (ch & 0xFF);
}

char USART_x_Read(USART_HandleType *handle)
{
    //1. wait until receiver not empty
    while(!(handle->regs->ISR & USART_ISR_RX_NOT_EMP));
    //2. read data from RDR register
    return (char)(handle->regs->RDR & 0xFF);
}
////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////INTERRUPT API (RX only)/////////////////////////////////////
// Call this to start using interrupts
void USART_EnableRXInterrupt(USART_HandleType *handle, USART_Callback_t callback) {
    handle->callback = callback;
    handle->regs->CR1 |= USART_CR1_RXNEIE; // Turn on the "faucet" at the peripheral
}

// Call this to go back to polling mode
void USART_DisableRXInterrupt(USART_HandleType *handle) {
    handle->regs->CR1 &= ~USART_CR1_RXNEIE; // Turn off the "faucet"
    handle->callback = NULL;
}

void USART_IRQHandler(USART_HandleType *handle) {
    uint32_t isr = handle->regs->ISR;

    // Error path: ORE/NE/FE/PE — read DR to clear all flags and discard the byte.
    // Must return after this so we don't try to read DR a second time below.
    if (isr & (USART_ISR_ORE | USART_ISR_NE | USART_ISR_FE | USART_ISR_PE)) {
        (void)handle->regs->RDR;
        handle->regs->ICR = (USART_ICR_ORECF | USART_ICR_NECF | USART_ICR_FECF | USART_ICR_PECF);
        return;
    }

    // RXNE: always read DR to clear the interrupt flag, even without a callback.
    // Not reading DR here would leave RXNE set and cause an infinite IRQ re-entry.
    if (isr & USART_ISR_RX_NOT_EMP) {
        char c = (char)(handle->regs->RDR & 0xFF); // clears RXNE
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
void USART6_IRQHandler(void) { if (s_usart6_handle) USART_IRQHandler(s_usart6_handle); }
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
    if (handle->regs == USART_6) return "USART6";
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
