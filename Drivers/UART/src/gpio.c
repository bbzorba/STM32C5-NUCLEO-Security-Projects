#include "../inc/gpio.h"

#define GPIO_NUM 16
#define GPIO_MODER_MODE0 0x3U
#define GPIO_MODE 0x3U
#define GPIO_PUPDR_PUPD0 0x3U
#define GPIO_OSPEEDR_OSPEED0 0x3U
#define GPIO_OTYPER_OT0 0x1U
#define GPIO_OUTPUT_TYPE_MASK 0x10U

void GPIO_constructor(GPIO_HandleTypeDef *GPIOx, 
                      GPIO_ManualTypeDef *regs, 
                      GPIO_InitTypeDef *GPIO_InitStruct) 
{
    GPIOx->regs = regs;
    GPIOx->init = GPIO_InitStruct;

    GPIO_Init(GPIOx);
}

void GPIO_Init(GPIO_HandleTypeDef *GPIOx) {

    // Pass the handle (contains regs) to match function signature
    __RCC_GPIO_CLK_ENABLE(GPIOx); // Enable GPIO clock
    
    uint32_t position;
    uint32_t temp = 0x00U;
    uint32_t ioposition = 0x00U;
    uint32_t iocurrent = 0x00U;

    for(position = 0; position < GPIO_NUM; position++) {
        ioposition = (1U << position);
        iocurrent = (GPIOx->init->Pin & ioposition);

        if (iocurrent == ioposition) {
            
            // Alternate function selection 
            if (GPIOx->init->Mode == GPIO_MODE_AF_PP || GPIOx->init->Mode == GPIO_MODE_AF_OD) {
                temp = GPIOx->regs->AFR[position >> 3U];
                temp &= ~(0xFU << (((uint32_t)position & 0x07U) * 4U));
                temp |= ((GPIOx->init->Alternate & 0x0FU) << (((uint32_t)position & 0x07U) * 4U));
                GPIOx->regs->AFR[position >> 3U] = temp;
            }

            // Direction mode configuration
            temp = GPIOx->regs->MODER;
            temp &= ~(GPIO_MODER_MODE0 << (position * 2U));
            temp |= ((GPIOx->init->Mode & GPIO_MODE) << (position * 2U));
            GPIOx->regs->MODER = temp;

            // Alternate function selection 
            if ((GPIOx->init->Mode == GPIO_MODE_OUTPUT_PP) || (GPIOx->init->Mode == GPIO_MODE_OUTPUT_OD) ||
                (GPIOx->init->Mode == GPIO_MODE_AF_PP) || (GPIOx->init->Mode == GPIO_MODE_AF_OD)) {
                temp = GPIOx->regs->OSPEEDR;
                temp &= ~(GPIO_OSPEEDR_OSPEED0 << (position * 2U));
                temp |= (GPIOx->init->Speed  << (position * 2U));
                GPIOx->regs->OSPEEDR = temp;

                // Output type configuration
                temp = GPIOx->regs->OTYPER;
                temp &= ~(GPIO_OTYPER_OT0 << position);
                temp |= (((GPIOx->init->Mode & GPIO_OUTPUT_TYPE_MASK) >> 4U) << position);
                GPIOx->regs->OTYPER = temp;
            }

            // Pull-up Pull down resistor configuration
            temp = GPIOx->regs->PUPDR;
            temp &= ~(GPIO_PUPDR_PUPD0 << (position * 2U));
            temp |= ((GPIOx->init->Pull & GPIO_PUPDR_PUPD0) << (position * 2U));
            GPIOx->regs->PUPDR = temp;
        }
    }
}

// Read the specified input port pin state
GPIO_PinState GPIO_ReadPin(GPIO_HandleTypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIO_PinState bitstatus;

    if ((GPIOx->regs->IDR & GPIO_Pin) != (uint32_t)GPIO_PIN_RESET) {
        bitstatus = GPIO_PIN_SET;
    } else {
        bitstatus = GPIO_PIN_RESET;
    }
    return bitstatus;
}

// Write to the specified output port pin
void GPIO_WritePin(GPIO_HandleTypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    if (PinState != GPIO_PIN_RESET) {
        GPIOx->regs->BSRR = (uint32_t)GPIO_Pin; // Set the pin
    } else {
        GPIOx->regs->BSRR = (uint32_t)(GPIO_Pin << 16U); // Reset the pin (upper half of BSRR)
    }
}

void GPIO_TogglePin(GPIO_HandleTypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIOx->regs->ODR ^= GPIO_Pin; // Toggle the specified pin
}

void GPIO_SetPin(GPIO_HandleTypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIOx->regs->ODR |= GPIO_Pin; // Set the pin
}

void GPIO_ResetPin(GPIO_HandleTypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIOx->regs->ODR &= ~GPIO_Pin; // Reset the pin
}

// Use project GPIO struct (single 32-bit BSRR)
void GPIO_SetBit(GPIO_HandleTypeDef *GPIOx, uint32_t pin) {
	 GPIOx->regs->BSRR = (1u << pin); 
}
void GPIO_ResetBit(GPIO_HandleTypeDef *GPIOx, uint32_t pin) { 
	GPIOx->regs->BSRR = (1u << (pin + 16u)); 
}

void __RCC_GPIO_CLK_ENABLE(GPIO_HandleTypeDef *GPIOx) {
    if      (GPIOx->regs == GPIO_A) RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    else if (GPIOx->regs == GPIO_B) RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    else if (GPIOx->regs == GPIO_C) RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    else if (GPIOx->regs == GPIO_D) RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
    else if (GPIOx->regs == GPIO_E) RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
    else if (GPIOx->regs == GPIO_H) RCC->AHB2ENR |= RCC_AHB2ENR_GPIOHEN;

    /* Read-back to ensure the clock enable has propagated through the bus
     * before any GPIO register access (required per STM32 errata). */
    __IO uint32_t tmpreg = RCC->AHB2ENR;
    (void)tmpreg;
}