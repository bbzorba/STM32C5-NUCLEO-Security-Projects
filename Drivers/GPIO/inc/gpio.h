// GPIO header file for STM32C5 (register-level, CMSIS only)
#include <stdint.h>

#ifndef __GPIO_H
#define __GPIO_H

///////////////////////////////////////GPIO ADDRESS DEFINITIONS//////////////////////////////////////////////////
#define __IO volatile

// STM32C5 base addresses (override from build flags if your variant differs)
#define RCC_BASE        0x46020C00U
#define GPIOA_BASE      0x42020000U
#define GPIOB_BASE      0x42020400U
#define GPIOC_BASE      0x42020800U
#define GPIOD_BASE      0x42020C00U
#define GPIOE_BASE      0x42021000U
#define GPIOF_BASE      0x42021400U
#define GPIOG_BASE      0x42021800U
#define GPIOH_BASE      0x42021C00U
#define GPIOI_BASE      0x42022000U

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RCC peripheral declarations and direct port access use RCC_TypeDef struct below (RCC->AHB2ENR etc.)



//////////////////////////////////////REGISTER BIT DEFINITIONS/////////////////////////////////////////////////
// NUCLEO-U575ZI-Q onboard LED pin output mode helpers (MODER field = 01b)
#define MODER_PC7_OUT ((uint32_t)(1U << (7 * 2)))  // PC7 LD1 Green: output mode
#define MODER_PB7_OUT ((uint32_t)(1U << (7 * 2)))  // PB7 LD2 Blue:  output mode
#define MODER_PG2_OUT ((uint32_t)(1U << (2 * 2)))  // PG2 LD3 Red:   output mode

// GPIO clock enable bits in RCC AHB2ENR
#define RCC_AHB2ENR_GPIOAEN  ((uint32_t)(1UL << 0))
#define RCC_AHB2ENR_GPIOBEN  ((uint32_t)(1UL << 1))
#define RCC_AHB2ENR_GPIOCEN  ((uint32_t)(1UL << 2))
#define RCC_AHB2ENR_GPIODEN  ((uint32_t)(1UL << 3))
#define RCC_AHB2ENR_GPIOEEN  ((uint32_t)(1UL << 4))
#define RCC_AHB2ENR_GPIOFEN  ((uint32_t)(1UL << 5))
#define RCC_AHB2ENR_GPIOGEN  ((uint32_t)(1UL << 6))
#define RCC_AHB2ENR_GPIOHEN  ((uint32_t)(1UL << 7))
#define RCC_AHB2ENR_GPIOIEN  ((uint32_t)(1UL << 8))

// GPIO pin definitions for ODR and IDR registers
#define GPIO_PIN_0  (1 << 0)  // Pin 0
#define GPIO_PIN_1  (1 << 1)  // Pin 1
#define GPIO_PIN_2  (1 << 2)  // Pin 2
#define GPIO_PIN_3  (1 << 3)  // Pin 3
#define GPIO_PIN_4  (1 << 4)  // Pin 4
#define GPIO_PIN_5  (1 << 5)  // Pin 5
#define GPIO_PIN_6  (1 << 6)  // Pin 6
#define GPIO_PIN_7  (1 << 7)  // Pin 7
#define GPIO_PIN_8  (1 << 8)  // Pin 8
#define GPIO_PIN_9  (1 << 9)  // Pin 9
#define GPIO_PIN_10 (1 << 10) // Pin 10
#define GPIO_PIN_11 (1 << 11) // Pin 11
#define GPIO_PIN_12 (1 << 12) // Pin 12
#define GPIO_PIN_13 (1 << 13) // Pin 13
#define GPIO_PIN_14 (1 << 14) // Pin 14
#define GPIO_PIN_15 (1 << 15) // Pin 15
#define GPIO_PIN_ALL (0xFFFF) // All pins



// GPIO MODER register Mask bit definitions
#define MODER_PIN0_MASK          ((uint32_t)0x00000003)        // mask to clear MODER_PIN0 bits (bits 1:0)
#define MODER_PIN1_MASK          ((uint32_t)0x0000000C)        // mask to clear MODER_PIN1 bits (bits 3:2)
#define MODER_PIN2_MASK          ((uint32_t)0x00000030)        // mask to clear MODER_PIN2 bits (bits 5:4)
#define MODER_PIN3_MASK          ((uint32_t)0x000000C0)        // mask to clear MODER_PIN3 bits (bits 7:6)
#define MODER_PIN4_MASK          ((uint32_t)0x00000300)        // mask to clear MODER_PIN4 bits (bits 9:8)
#define MODER_PIN5_MASK          ((uint32_t)0x00000C00)        // mask to clear MODER_PIN5 bits (bits 11:10)
#define MODER_PIN6_MASK          ((uint32_t)0x00003000)        // mask to clear MODER_PIN6 bits (bits 13:12)
#define MODER_PIN7_MASK          ((uint32_t)0x0000C000)        // mask to clear MODER_PIN7 bits (bits 15:14)
#define MODER_PIN8_MASK          ((uint32_t)0x00030000)        // mask to clear MODER_PIN8 bits (bits 17:16)
#define MODER_PIN9_MASK          ((uint32_t)0x000C0000)        // mask to clear MODER_PIN9 bits (bits 19:18)
#define MODER_PIN10_MASK         ((uint32_t)0x00300000)        // mask to clear MODER_PIN10 bits (bits 21:20)
#define MODER_PIN11_MASK         ((uint32_t)0x00C00000)        // mask to clear MODER_PIN11 bits (bits 23:22)
#define MODER_PIN12_MASK         ((uint32_t)0x03000000)        // mask to clear MODER_PIN12 bits (bits 25:24)
#define MODER_PIN13_MASK         ((uint32_t)0x0C000000)        // mask to clear MODER_PIN13 bits (bits 27:26)
#define MODER_PIN14_MASK         ((uint32_t)0x30000000)        // mask to clear MODER_PIN14 bits (bits 29:28)
#define MODER_PIN15_MASK         ((uint32_t)0xC0000000)        // mask to clear MODER_PIN15 bits (bits 31:30)

// GPIO MODER register Set bit definitions for Alternate Function mode (10b)
#define MODER_PIN0_SET           ((uint32_t)0x00000002)        // set AF for PIN0 (bits 1:0)
#define MODER_PIN1_SET           ((uint32_t)0x00000008)        // set AF for PIN1 (bits 3:2)
#define MODER_PIN2_SET           ((uint32_t)0x00000020)        // set AF for PIN2 (bits 5:4)
#define MODER_PIN3_SET           ((uint32_t)0x00000080)        // set AF for PIN3 (bits 7:6)
#define MODER_PIN4_SET           ((uint32_t)0x00000200)        // set AF for PIN4 (bits 9:8)
#define MODER_PIN5_SET           ((uint32_t)0x00000800)        // set AF for PIN5 (bits 11:10)
#define MODER_PIN6_SET           ((uint32_t)0x00002000)        // set AF for PIN6 (bits 13:12)
#define MODER_PIN7_SET           ((uint32_t)0x00008000)        // set AF for PIN7 (bits 15:14)
#define MODER_PIN8_SET           ((uint32_t)0x00020000)        // set AF for PIN8 (bits 17:16)
#define MODER_PIN9_SET           ((uint32_t)0x00080000)        // set AF for PIN9 (bits 19:18)
#define MODER_PIN10_SET          ((uint32_t)0x00200000)        // set AF for PIN10 (bits 21:20)
#define MODER_PIN11_SET          ((uint32_t)0x00800000)        // set AF for PIN11 (bits 23:22)
#define MODER_PIN12_SET          ((uint32_t)0x02000000)        // set AF for PIN12 (bits 25:24)
#define MODER_PIN13_SET          ((uint32_t)0x08000000)        // set AF for PIN13 (bits 27:26)
#define MODER_PIN14_SET          ((uint32_t)0x20000000)        // set AF for PIN14 (bits 29:28)
#define MODER_PIN15_SET          ((uint32_t)0x80000000)        // set AF for PIN15 (bits 31:30)

// GPIO AFRL register Mask bit definitions
#define AFRL_PIN0_MASK           ((uint32_t)0x0000000F)        // mask to clear AFRL_PIN0 bits (bits 3:0)
#define AFRL_PIN1_MASK           ((uint32_t)0x000000F0)        // mask to clear AFRL_PIN1 bits (bits 7:4)
#define AFRL_PIN2_MASK           ((uint32_t)0x00000F00)        // mask to clear AFRL_PIN2 bits (bits 11:8)
#define AFRL_PIN3_MASK           ((uint32_t)0x0000F000)        // mask to clear AFRL_PIN3 bits (bits 15:12)
#define AFRL_PIN4_MASK           ((uint32_t)0x000F0000)        // mask to clear AFRL_PIN4 bits (bits 19:16)
#define AFRL_PIN5_MASK           ((uint32_t)0x00F00000)        // mask to clear AFRL_PIN5 bits (bits 23:20)
#define AFRL_PIN6_MASK           ((uint32_t)0x0F000000)        // mask to clear AFRL_PIN6 bits (bits 27:24)
#define AFRL_PIN7_MASK           ((uint32_t)0xF0000000)        // mask to clear AFRL_PIN7 bits (bits 31:28)
#define AFRH_PIN8_MASK           ((uint32_t)0x0000000F)        // mask to clear AFRH_PIN8 bits (bits 3:0)
#define AFRH_PIN9_MASK           ((uint32_t)0x000000F0)        // mask to clear AFRH_PIN9 bits (bits 7:4)
#define AFRH_PIN10_MASK          ((uint32_t)0x00000F00)        // mask to clear AFRH_PIN10 bits (bits 11:8)
#define AFRH_PIN11_MASK          ((uint32_t)0x0000F000)        // mask to clear AFRH_PIN11 bits (bits 15:12)
#define AFRH_PIN12_MASK          ((uint32_t)0x000F0000)        // mask to clear AFRH_PIN12 bits (bits 19:16)
#define AFRH_PIN13_MASK          ((uint32_t)0x00F00000)        // mask to clear AFRH_PIN13 bits (bits 23:20)
#define AFRH_PIN14_MASK          ((uint32_t)0x0F000000)        // mask to clear AFRH_PIN14 bits (bits 27:24)
#define AFRH_PIN15_MASK          ((uint32_t)0xF0000000)        // mask to clear AFRH_PIN15 bits (bits 31:28)

//GPIO AFR register Set bit definitions for AF4 (0100b) [I2C1, I2C2, I2C3]
#define AFRL_PIN6_SET_AF4        ((uint32_t)0x04000000)        // set AF4 for PB6 (I2C1 SCL)
#define AFRL_PIN7_SET_AF4        ((uint32_t)0x40000000)        // set AF4 for PB7 (I2C1 SDA)
#define AFRH_PIN8_SET_AF4        ((uint32_t)0x00000004)        // set AF4 for PB8 (I2C1 SCL alt)
#define AFRH_PIN9_SET_AF4        ((uint32_t)0x00000040)        // set AF4 for PB9 (I2C1 SDA alt)
#define AFRH_PIN10_SET_AF4       ((uint32_t)0x00000400)        // set AF4 for PB10 (I2C2 SCL)
#define AFRH_PIN11_SET_AF4       ((uint32_t)0x00004000)        // set AF4 for PB11 (I2C2 SDA)

//GPIO AFR register Set bit definitions for AF5 (0101b) [SPI1, SPI2]
#define AFRL_PIN0_SET_AF5        ((uint32_t)0x00000005)        // set AF5 for PIN0 (bits 3:0)
#define AFRL_PIN1_SET_AF5        ((uint32_t)0x00000050)        // set AF5 for PIN1 (bits 7:4)
#define AFRL_PIN2_SET_AF5        ((uint32_t)0x00000500)        // set AF5 for PIN2 (bits 11:8)
#define AFRL_PIN3_SET_AF5        ((uint32_t)0x00005000)        // set AF5 for PIN3 (bits 15:12)
#define AFRL_PIN4_SET_AF5        ((uint32_t)0x00050000)        // set AF5 for PIN4 (bits 19:16)
#define AFRL_PIN5_SET_AF5        ((uint32_t)0x00500000)        // set AF5 for PIN5 (bits 23:20)
#define AFRL_PIN6_SET_AF5        ((uint32_t)0x05000000)        // set AF5 for PIN6 (bits 27:24)
#define AFRL_PIN7_SET_AF5        ((uint32_t)0x50000000)        // set AF5 for PIN7 (bits 31:28)
#define AFRH_PIN8_SET_AF5        ((uint32_t)0x00000005)        // set AF5 for PIN8 (bits 3:0)
#define AFRH_PIN9_SET_AF5        ((uint32_t)0x00000050)        // set AF5 for PIN9 (bits 7:4)
#define AFRH_PIN10_SET_AF5       ((uint32_t)0x00000500)        // set AF5 for PIN10 (bits 11:8)
#define AFRH_PIN11_SET_AF5       ((uint32_t)0x00005000)        // set AF5 for PIN11 (bits 15:12)
#define AFRH_PIN12_SET_AF5       ((uint32_t)0x00050000)        // set AF5 for PIN12 (bits 19:16)
#define AFRH_PIN13_SET_AF5       ((uint32_t)0x00500000)        // set AF5 for PIN13 (bits 23:20)
#define AFRH_PIN14_SET_AF5       ((uint32_t)0x05000000)        // set AF5 for PIN14 (bits 27:24)
#define AFRH_PIN15_SET_AF5       ((uint32_t)0x50000000)        // set AF5 for PIN15 (bits 31:28)

// GPIO AFR register Set bit definitions for AF6 (0110b) [SPI3, I2S3]
#define AFRL_PIN0_SET_AF6        ((uint32_t)0x00000006)        // set AF6 for PIN0 (bits 3:0)
#define AFRL_PIN1_SET_AF6        ((uint32_t)0x00000060)        // set AF6 for PIN1 (bits 7:4)
#define AFRL_PIN2_SET_AF6        ((uint32_t)0x00000600)        // set AF6 for PIN2 (bits 11:8)
#define AFRL_PIN3_SET_AF6        ((uint32_t)0x00006000)        // set AF6 for PIN3 (bits 15:12)
#define AFRL_PIN4_SET_AF6        ((uint32_t)0x00060000)        // set AF6 for PIN4 (bits 19:16)
#define AFRL_PIN5_SET_AF6        ((uint32_t)0x00600000)        // set AF6 for PIN5 (bits 23:20)
#define AFRL_PIN6_SET_AF6        ((uint32_t)0x06000000)        // set AF6 for PIN6 (bits 27:24)
#define AFRL_PIN7_SET_AF6        ((uint32_t)0x60000000)        // set AF6 for PIN7 (bits 31:28)
#define AFRH_PIN8_SET_AF6        ((uint32_t)0x00000006)        // set AF6 for PIN8 (bits 3:0)
#define AFRH_PIN9_SET_AF6        ((uint32_t)0x00000060)        // set AF6 for PIN9 (bits 7:4)
#define AFRH_PIN10_SET_AF6       ((uint32_t)0x00000600)        // set AF6 for PIN10 (bits 11:8)
#define AFRH_PIN11_SET_AF6       ((uint32_t)0x00006000)        // set AF6 for PIN11 (bits 15:12)
#define AFRH_PIN12_SET_AF6       ((uint32_t)0x00060000)        // set AF6 for PIN12 (bits 19:16)
#define AFRH_PIN13_SET_AF6       ((uint32_t)0x00600000)        // set AF6 for PIN13 (bits 23:20)
#define AFRH_PIN14_SET_AF6       ((uint32_t)0x06000000)        // set AF6 for PIN14 (bits 27:24)
#define AFRH_PIN15_SET_AF6       ((uint32_t)0x60000000)        // set AF6 for PIN15 (bits 31:28)

// GPIO AFR register Set bit definitions for AF7 (0111b) [USART1, USART2, USART3]
#define AFRL_PIN0_SET_AF7            ((uint32_t)0x00000007)        // set AF7 for PIN0 (bits 3:0)
#define AFRL_PIN1_SET_AF7            ((uint32_t)0x00000070)        // set AF7 for PIN1 (bits 7:4)
#define AFRL_PIN2_SET_AF7            ((uint32_t)0x00000700)        // set AF7 for PIN2 (bits 11:8)
#define AFRL_PIN3_SET_AF7            ((uint32_t)0x00007000)        // set AF7 for PIN3 (bits 15:12)
#define AFRL_PIN4_SET_AF7            ((uint32_t)0x00070000)        // set AF7 for PIN4 (bits 19:16)
#define AFRL_PIN5_SET_AF7            ((uint32_t)0x00700000)        // set AF7 for PIN5 (bits 23:20)
#define AFRL_PIN6_SET_AF7            ((uint32_t)0x07000000)        // set AF7 for PIN6 (bits 27:24)
#define AFRL_PIN7_SET_AF7            ((uint32_t)0x70000000)        // set AF7 for PIN7 (bits 31:28)
#define AFRH_PIN8_SET_AF7            ((uint32_t)0x00000007)        // set AF7 for PIN8 (bits 3:0)
#define AFRH_PIN9_SET_AF7            ((uint32_t)0x00000070)        // set AF7 for PIN9 (bits 7:4)
#define AFRH_PIN10_SET_AF7           ((uint32_t)0x00000700)        // set AF7 for PIN10 (bits 11:8)
#define AFRH_PIN11_SET_AF7           ((uint32_t)0x00007000)        // set AF7 for PIN11 (bits 15:12)
#define AFRH_PIN12_SET_AF7           ((uint32_t)0x00070000)        // set AF7 for PIN12 (bits 19:16)
#define AFRH_PIN13_SET_AF7           ((uint32_t)0x00700000)        // set AF7 for PIN13 (bits 23:20)
#define AFRH_PIN14_SET_AF7           ((uint32_t)0x07000000)        // set AF7 for PIN14 (bits 27:24)
#define AFRH_PIN15_SET_AF7           ((uint32_t)0x70000000)        // set AF7 for PIN15 (bits 31:28)

// GPIO AFR register Set bit definitions for AF8 (1000b) [UART4, UART5, USART6]
#define AFRL_PIN0_SET_AF8        ((uint32_t)0x00000008)        // set AF8 for PIN0 (bits 3:0)
#define AFRL_PIN1_SET_AF8        ((uint32_t)0x00000080)        // set AF8 for PIN1 (bits 7:4)
#define AFRL_PIN2_SET_AF8        ((uint32_t)0x00000800)        // set AF8 for PIN2 (bits 11:8)
#define AFRL_PIN3_SET_AF8        ((uint32_t)0x00008000)        // set AF8 for PIN3 (bits 15:12)
#define AFRL_PIN4_SET_AF8        ((uint32_t)0x00080000)        // set AF8 for PIN4 (bits 19:16)
#define AFRL_PIN5_SET_AF8        ((uint32_t)0x00800000)        // set AF8 for PIN5 (bits 23:20)
#define AFRL_PIN6_SET_AF8        ((uint32_t)0x08000000)        // set AF8 for PIN6 (bits 27:24)
#define AFRL_PIN7_SET_AF8        ((uint32_t)0x80000000)        // set AF8 for PIN7 (bits 31:28)
#define AFRH_PIN8_SET_AF8        ((uint32_t)0x00000008)        // set AF8 for PIN8 (bits 3:0)
#define AFRH_PIN9_SET_AF8        ((uint32_t)0x00000080)        // set AF8 for PIN9 (bits 7:4)
#define AFRH_PIN10_SET_AF8       ((uint32_t)0x00000800)        // set AF8 for PIN10 (bits 11:8)
#define AFRH_PIN11_SET_AF8       ((uint32_t)0x00008000)        // set AF8 for PIN11 (bits 15:12)
#define AFRH_PIN12_SET_AF8       ((uint32_t)0x00080000)        // set AF8 for PIN12 (bits 19:16)
#define AFRH_PIN13_SET_AF8       ((uint32_t)0x00800000)        // set AF8 for PIN13 (bits 23:20)
#define AFRH_PIN14_SET_AF8       ((uint32_t)0x08000000)        // set AF8 for PIN14 (bits 27:24)
#define AFRH_PIN15_SET_AF8       ((uint32_t)0x80000000)        // set AF8 for PIN15 (bits 31:28)

// OTYPER register bit definitions
#define OTYPER_PIN0_OPEN_DRAIN ((uint32_t)0x00000001)        // set open-drain for PIN0
#define OTYPER_PIN1_OPEN_DRAIN  ((uint32_t)0x00000002)        // set open-drain for PIN1
#define OTYPER_PIN2_OPEN_DRAIN  ((uint32_t)0x00000004)        // set open-drain for PIN2
#define OTYPER_PIN3_OPEN_DRAIN  ((uint32_t)0x00000008)        // set open-drain for PIN3
#define OTYPER_PIN4_OPEN_DRAIN  ((uint32_t)0x00000010)        // set open-drain for PIN4
#define OTYPER_PIN5_OPEN_DRAIN  ((uint32_t)0x00000020)        // set open-drain for PIN5
#define OTYPER_PIN6_OPEN_DRAIN  ((uint32_t)0x00000040)        // set open-drain for PIN6
#define OTYPER_PIN7_OPEN_DRAIN  ((uint32_t)0x00000080)        // set open-drain for PIN7
#define OTYPER_PIN8_OPEN_DRAIN  ((uint32_t)0x00000100)        // set open-drain for PIN8
#define OTYPER_PIN9_OPEN_DRAIN  ((uint32_t)0x00000200)        // set open-drain for PIN9
#define OTYPER_PIN10_OPEN_DRAIN ((uint32_t)0x00000400)        // set open-drain for PIN10
#define OTYPER_PIN11_OPEN_DRAIN ((uint32_t)0x00000800)        // set open-drain for PIN11
#define OTYPER_PIN12_OPEN_DRAIN ((uint32_t)0x00001000)        // set open-drain for PIN12
#define OTYPER_PIN13_OPEN_DRAIN ((uint32_t)0x00002000)        // set open-drain for PIN13
#define OTYPER_PIN14_OPEN_DRAIN ((uint32_t)0x00004000)        // set open-drain for PIN14
#define OTYPER_PIN15_OPEN_DRAIN ((uint32_t)0x00008000)        // set open-drain for PIN15

// ODR register bit definitions
#define ODR_PIN0_HIGH           ((uint32_t)0x00000001)        // set PIN0 high
#define ODR_PIN1_HIGH           ((uint32_t)0x00000002)        // set PIN1 high
#define ODR_PIN2_HIGH           ((uint32_t)0x00000004)        // set PIN2 high
#define ODR_PIN3_HIGH           ((uint32_t)0x00000008)        // set PIN3 high
#define ODR_PIN4_HIGH           ((uint32_t)0x00000010)        // set PIN4 high
#define ODR_PIN5_HIGH           ((uint32_t)0x00000020)        // set PIN5 high
#define ODR_PIN6_HIGH           ((uint32_t)0x00000040)        // set PIN6 high
#define ODR_PIN7_HIGH           ((uint32_t)0x00000080)        // set PIN7 high
#define ODR_PIN8_HIGH           ((uint32_t)0x00000100)        // set PIN8 high
#define ODR_PIN9_HIGH           ((uint32_t)0x00000200)        // set PIN9 high
#define ODR_PIN10_HIGH          ((uint32_t)0x00000400)        // set PIN10 high
#define ODR_PIN11_HIGH          ((uint32_t)0x00000800)        // set PIN11 high
#define ODR_PIN12_HIGH          ((uint32_t)0x00001000)        // set PIN12 high
#define ODR_PIN13_HIGH          ((uint32_t)0x00002000)        // set PIN13 high
#define ODR_PIN14_HIGH          ((uint32_t)0x00004000)        // set PIN14 high
#define ODR_PIN15_HIGH          ((uint32_t)0x00008000)        // set PIN15 high

// IDR register bit definitions
#define IDR_PIN0_HIGH           ((uint32_t)0x00000001)        // read PIN0 high
#define IDR_PIN1_HIGH           ((uint32_t)0x00000002)        // read PIN1 high
#define IDR_PIN2_HIGH           ((uint32_t)0x00000004)        // read PIN2 high
#define IDR_PIN3_HIGH           ((uint32_t)0x00000008)        // read PIN3 high
#define IDR_PIN4_HIGH           ((uint32_t)0x00000010)        // read PIN4 high
#define IDR_PIN5_HIGH           ((uint32_t)0x00000020)        // read PIN5 high
#define IDR_PIN6_HIGH           ((uint32_t)0x00000040)        // read PIN6 high
#define IDR_PIN7_HIGH           ((uint32_t)0x00000080)        // read PIN7 high
#define IDR_PIN8_HIGH           ((uint32_t)0x00000100)        // read PIN8 high
#define IDR_PIN9_HIGH           ((uint32_t)0x00000200)        // read PIN9 high
#define IDR_PIN10_HIGH          ((uint32_t)0x00000400)        // read PIN10 high
#define IDR_PIN11_HIGH          ((uint32_t)0x00000800)        // read PIN11 high
#define IDR_PIN12_HIGH          ((uint32_t)0x00001000)        // read PIN12 high
#define IDR_PIN13_HIGH          ((uint32_t)0x00002000)        // read PIN13 high
#define IDR_PIN14_HIGH          ((uint32_t)0x00004000)        // read PIN14 high
#define IDR_PIN15_HIGH          ((uint32_t)0x00008000)        // read PIN15 high

// GPIO PUPDR register Mask bit definitions
#define PUPDR_PIN0_MASK         ((uint32_t)0x00000003)        // mask to clear PUPDR_PIN0 bits (bits 1:0)
#define PUPDR_PIN1_MASK         ((uint32_t)0x0000000C)        // mask to clear PUPDR_PIN1 bits (bits 3:2)
#define PUPDR_PIN2_MASK         ((uint32_t)0x00000030)        // mask to clear PUPDR_PIN2 bits (bits 5:4)
#define PUPDR_PIN3_MASK         ((uint32_t)0x000000C0)        // mask to clear PUPDR_PIN3 bits (bits 7:6)
#define PUPDR_PIN4_MASK         ((uint32_t)0x00000300)        // mask to clear PUPDR_PIN4 bits (bits 9:8)
#define PUPDR_PIN5_MASK         ((uint32_t)0x00000C00)        // mask to clear PUPDR_PIN5 bits (bits 11:10)
#define PUPDR_PIN6_MASK         ((uint32_t)0x00003000)        // mask to clear PUPDR_PIN6 bits (bits 13:12)
#define PUPDR_PIN7_MASK         ((uint32_t)0x0000C000)        // mask to clear PUPDR_PIN7 bits (bits 15:14)
#define PUPDR_PIN8_MASK         ((uint32_t)0x00030000)        // mask to clear PUPDR_PIN8 bits (bits 17:16)
#define PUPDR_PIN9_MASK         ((uint32_t)0x000C0000)        // mask to clear PUPDR_PIN9 bits (bits 19:18)
#define PUPDR_PIN10_MASK        ((uint32_t)0x00300000)        // mask to clear PUPDR_PIN10 bits (bits 21:20)
#define PUPDR_PIN11_MASK        ((uint32_t)0x00C00000)        // mask to clear PUPDR_PIN11 bits (bits 23:22)
#define PUPDR_PIN12_MASK        ((uint32_t)0x03000000)        // mask to clear PUPDR_PIN12 bits (bits 25:24)
#define PUPDR_PIN13_MASK        ((uint32_t)0x0C000000)        // mask to clear PUPDR_PIN13 bits (bits 27:26)
#define PUPDR_PIN14_MASK        ((uint32_t)0x30000000)        // mask to clear PUPDR_PIN14 bits (bits 29:28)
#define PUPDR_PIN15_MASK        ((uint32_t)0xC0000000)        // mask to clear PUPDR_PIN15 bits (bits 31:30)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////GPIO CONFIGURATION DEFINITIONS/////////////////////////////////////////////
/* GPIO mode definitions for MODER register */
#define GPIO_MODE_INPUT 0x00U        // Input mode
#define GPIO_MODE_OUTPUT 0x01U       // Output mode
#define GPIO_MODE_ALTERNATE 0x02U    // Alternate function mode
#define GPIO_MODE_ANALOG 0x03U       // Analog mode

/* GPIO output type definitions for OTYPER register */
#define GPIO_OTYPE_PP 0x00U          // Push-pull
#define GPIO_OTYPE_OD 0x01U          // Open-drain

/* GPIO pull-up/pull-down definitions for PUPDR register */
#define GPIO_NOPULL 0x00U          // No pull-up, pull-down
#define GPIO_PULLUP 0x01U          // Pull-up
#define GPIO_PULLDOWN 0x02U        // Pull-down

/* GPIO speed definitions for OSPEEDR register */
#define GPIO_SPEED_LOW       0x00U   // Low speed
#define GPIO_SPEED_MEDIUM    0x01U   // Medium speed  
#define GPIO_SPEED_HIGH      0x02U   // High speed
#define GPIO_SPEED_VERY_HIGH 0x03U   // Very high speed

/* Composite modes (preferred) */
#define GPIO_MODE_OUTPUT_PP    0x00000001U  /* MODER=01, OTYPER=0 */
#define GPIO_MODE_OUTPUT_OD    0x00000011U  /* MODER=01, OTYPER=1 */
#define GPIO_MODE_AF_PP        0x00000002U  /* MODER=10, OTYPER=0 */
#define GPIO_MODE_AF_OD        0x00000012U  /* MODER=10, OTYPER=1 */
//////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////GPIO STRUCTURES AND ENUMERATIONS/////////////////////////////////////////////
/* RCC register definition structure — STM32C5 / STM32U5-compatible layout.
 * Field offsets from RCC_BASE:
 *   AHB2ENR @ 0x4C,  APB1ENR @ 0x58,  APB2ENR @ 0x60
 * Verified against STM32U5 Reference Manual (RM0456). */
typedef struct
{
    __IO uint32_t CR;           /* 0x00  Clock control register                        */
    __IO uint32_t ICSCR1;       /* 0x04  Internal clock sources calibration register 1 */
    __IO uint32_t ICSCR2;       /* 0x08  Internal clock sources calibration register 2 */
    __IO uint32_t ICSCR3;       /* 0x0C  Internal clock sources calibration register 3 */
    __IO uint32_t CRRCR;        /* 0x10  Clock recovery RC register                    */
         uint32_t RESERVED0[2];/* 0x14-0x18 Reserved                                  */
    __IO uint32_t CFGR1;        /* 0x1C  Clock configuration register 1                */
    __IO uint32_t CFGR2;        /* 0x20  Clock configuration register 2                */
    __IO uint32_t CFGR3;        /* 0x24  Clock configuration register 3                */
    __IO uint32_t PLL1CFGR;     /* 0x28  PLL1 configuration register                   */
    __IO uint32_t PLL2CFGR;     /* 0x2C  PLL2 configuration register                   */
    __IO uint32_t PLL3CFGR;     /* 0x30  PLL3 configuration register                   */
    __IO uint32_t PLL1DIVR;     /* 0x34  PLL1 dividers register                        */
    __IO uint32_t PLL1FRACR;    /* 0x38  PLL1 fractional divider register              */
    __IO uint32_t PLL2DIVR;     /* 0x3C  PLL2 dividers register                        */
    __IO uint32_t PLL2FRACR;    /* 0x40  PLL2 fractional divider register              */
    __IO uint32_t PLL3DIVR;     /* 0x44  PLL3 dividers register                        */
    __IO uint32_t PLL3FRACR;    /* 0x48  PLL3 fractional divider register              */
    __IO uint32_t AHB2ENR;      /* 0x4C  AHB2 peripheral clock enable register         */
    __IO uint32_t AHB2ENR2;     /* 0x50  AHB2 peripheral clock enable register 2       */
    __IO uint32_t AHB3ENR;      /* 0x54  AHB3 peripheral clock enable register         */
    __IO uint32_t APB1ENR;      /* 0x58  APB1 peripheral clock enable register         */
    __IO uint32_t APB1ENR2;     /* 0x5C  APB1 peripheral clock enable register 2       */
    __IO uint32_t APB2ENR;      /* 0x60  APB2 peripheral clock enable register         */
    __IO uint32_t APB3ENR;      /* 0x64  APB3 peripheral clock enable register         */
} RCC_TypeDef;

/* GPIO initialization structure */
typedef struct {
    uint32_t Pin;            // Specifies the GPIO pins to be configured.
    uint32_t Mode;           // Specifies the operating mode for the selected pins.
    uint32_t Pull;           // Specifies the Pull-up or Pull-down activation for the selected pins.
    uint32_t Speed;          // Specifies the speed for the selected pins.
    uint32_t Alternate;     // Specifies the alternate function for the selected pins.
} GPIO_InitTypeDef;

/* GPIO port mode enumeration */
typedef struct {
    __IO uint32_t MODER;    // GPIO port mode register
    __IO uint32_t OTYPER;   // GPIO port output type register
    __IO uint32_t OSPEEDR;  // GPIO port output speed register
    __IO uint32_t PUPDR;    // GPIO port pull-up/pull-down register
    __IO uint32_t IDR;      // GPIO port input data register
    __IO uint32_t ODR;      // GPIO port output data register
    __IO uint32_t BSRR;     // GPIO port bit set/reset register
     __IO uint32_t LCKR;     // GPIO port configuration lock register
     __IO uint32_t AFR[2];   // GPIO alternate function registers (AFR[0]: AFRL, AFR[1]: AFRH)
} GPIO_ManualTypeDef;

/* GPIO handle structure */
typedef struct {
    GPIO_ManualTypeDef *regs;  // Pointer to GPIO port base address
     GPIO_InitTypeDef *init;     // GPIO initialization structure
} GPIO_HandleTypeDef;

void GPIO_constructor(GPIO_HandleTypeDef *GPIOx, 
                      GPIO_ManualTypeDef *regs, 
                      GPIO_InitTypeDef *GPIO_InitStruct);

/* GPIO pin state enumeration */
typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
} GPIO_PinState;

// Peripheral declarations
#define RCC ((RCC_TypeDef *)RCC_BASE)
#define GPIO_A ((GPIO_ManualTypeDef *)GPIOA_BASE)
#define GPIO_B ((GPIO_ManualTypeDef *)GPIOB_BASE)
#define GPIO_C ((GPIO_ManualTypeDef *)GPIOC_BASE)
#define GPIO_D ((GPIO_ManualTypeDef *)GPIOD_BASE)
#define GPIO_E ((GPIO_ManualTypeDef *)GPIOE_BASE)
#define GPIO_F ((GPIO_ManualTypeDef *)GPIOF_BASE)
#define GPIO_G ((GPIO_ManualTypeDef *)GPIOG_BASE)
#define GPIO_H ((GPIO_ManualTypeDef *)GPIOH_BASE)
#define GPIO_I ((GPIO_ManualTypeDef *)GPIOI_BASE)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////GPIO FUNCTION PROTOTYPES/////////////////////////////////////////////////
/* Function prototypes */
void GPIO_Init(GPIO_HandleTypeDef *GPIOx);
void GPIO_TogglePin(GPIO_HandleTypeDef *GPIOx, uint16_t GPIO_Pin);
void GPIO_SetPin(GPIO_HandleTypeDef *GPIOx, uint16_t GPIO_Pin);
void GPIO_ResetPin(GPIO_HandleTypeDef *GPIOx, uint16_t GPIO_Pin);
void GPIO_WritePin(GPIO_HandleTypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
GPIO_PinState GPIO_ReadPin(GPIO_HandleTypeDef *GPIOx, uint16_t GPIO_Pin);
void __RCC_GPIO_CLK_ENABLE(GPIO_HandleTypeDef *GPIOx); // RCC peripheral clock enable function
// Use project GPIO struct (single 32-bit BSRR)
void GPIO_SetBit(GPIO_HandleTypeDef *GPIOx, uint32_t pin);
void GPIO_ResetBit(GPIO_HandleTypeDef *GPIOx, uint32_t pin);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif // __GPIO_H