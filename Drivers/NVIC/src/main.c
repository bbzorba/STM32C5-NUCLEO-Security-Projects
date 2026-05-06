/*
    NVIC driver test -- GPIO EXTI button (PC13) + LED (PA5)

    Hardware (NUCLEO-C562RE):
      - Blue User Button B1 : PC13  (active low, pulled-up internally)
      - Green LED LD1        : PA5   (active high)
      - USART2 VCP TX        : PA2   (ST-LINK virtual COM port)

    Test:
      - Each falling edge on PC13 (button press) fires EXTI13_IRQn = 20.
      - The handler (registered via NVIC driver) toggles PA5 LED and
        prints an incrementing press count to the serial terminal.
      - EXTI registers (STM32C562RE, base 0x44022000, from SVD):
          RTSR1   @ +0x000   rising trigger
          FTSR1   @ +0x004   falling trigger
          FPR1    @ +0x010   falling pending
          EXTICR4 @ +0x06C   port select lines 12-15 (8 bits each)
          IMR1    @ +0x080   interrupt mask (reset = 0xFFFFFFFF, unmasked)
      - EXTICR4 EXTI13 field at bits [15:8]; port C = 0x02.
*/

#include "../inc/nvic.h"
#include "../../GPIO/inc/gpio.h"
#include "../../UART/inc/uart.h"

static USART_HandleType huart;
static volatile uint32_t g_press_count = 0U;

/* EXTI13 callback: invoked by nvic.c dispatch on each button press */
static void button_callback(void *arg)
{
    (void)arg;
    NVIC_EXTI_ClearPending(13);    /* clear before any other work */
    GPIO_A->ODR ^= (1U << 5);     /* toggle green LED on PA5 */
    g_press_count++;
    USART_WriteString(&huart, "Button pressed! Count: ");
    print_dec(&huart, g_press_count);
    USART_WriteString(&huart, "\r\n");
}

int main(void)
{
    /* USART2 TX-only, PA2, ST-LINK VCP, 115200 baud */
    USART_constructor(&huart, USART_2, TX_ONLY, __115200);
    USART_WriteString(&huart, "\r\nNVIC driver test\r\n");
    USART_WriteString(&huart, "EXTI13 (PC13, B1) falling edge -> toggles LED + prints\r\n\r\n");

    /* LED: PA5 output push-pull, starts off */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIO_A->MODER  &= ~(3U << 10);
    GPIO_A->MODER  |=  (1U << 10);   /* output */
    GPIO_A->OTYPER &= ~(1U << 5);    /* push-pull */
    GPIO_A->ODR    &= ~(1U << 5);    /* LED off */

    /* Button: PC13 input with pull-up */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    GPIO_C->MODER &= ~(3U << 26);    /* input */
    GPIO_C->PUPDR &= ~(3U << 26);
    GPIO_C->PUPDR |=  (1U << 26);    /* pull-up */

    /* Configure EXTI13 on PC13 (port C), falling edge — one call does everything */
    NVIC_EXTI_Enable(13, NVIC_PORT_C, NVIC_EDGE_FALLING, button_callback, NULL, 0U);

    USART_WriteString(&huart, "Ready. Press B1 (PC13) to test.\r\n");

    while (1) {
        delay(2000000U);
    }
}