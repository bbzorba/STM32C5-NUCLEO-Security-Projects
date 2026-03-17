#include "../inc/button_led.h"

USART_HandleType usart;

Button_TypeDef userButton;

LED_Type greenLED;
LED_Type redLED;
LED_Type yellowLED;
LED_Type blueLED;

// NULL-terminated array passed to blink functions so each is called only once.
LED_Type *all_leds[] = {&greenLED, &yellowLED, &redLED, &blueLED, NULL};

//main function
int main(void) {
    Button_constructor(&userButton, GPIO_A, GPIO_PIN_0,
                       GPIO_MODE_INPUT, GPIO_PULLDOWN, GPIO_SPEED_LOW, 1);

    USART_constructor(&usart, USART_2, RX_AND_TX, __115200);
    USART_WriteString(&usart, "\r\n Button LED Blink Application with Synchronization\r\n");

    /* Button_LED_constructor calls LED_constructor internally and initialises
     * the USER button (PA0/EXTI0) on the first call -- no separate Button_Init needed. */
    LED_constructor(&greenLED, GREEN, LED_OFF);
    LED_constructor(&redLED, RED, LED_OFF);
    LED_constructor(&yellowLED, YELLOW, LED_OFF);
    LED_constructor(&blueLED, BLUE, LED_OFF);

    while (1) {
        /* Sync holds the mutex for 5 cycles (~2.5 s).  If the button is
         * pressed mid-blink, async is attempted inside sync and gets
         * "mutex busy".  It succeeds here, after the mutex is released.  */
        blink_LEDS_sync(&usart, all_leds);
        blink_LEDS_async(&usart, all_leds);
    }
}