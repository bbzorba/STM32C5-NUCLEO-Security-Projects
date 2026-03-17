#include "../inc/powerLed.h"
#include "../inc/medicalLED.h"

// Function prototypes
void delay(volatile uint32_t count);

USART_HandleType usart;

LED_Type greenLED;
MedicalLED_Type redLED;

powerLED_Type yellowLED;
powerLED_Type blueLED;

led_elec_type yellow_led_power;
led_elec_type blue_led_power;

//main function
int main(void) {
    USART_constructor(&usart, USART_2, RX_AND_TX, __115200);
    USART_WriteString(&usart, "\r\n Power LED & Medical LED Efficiency Computing & Diagnostics Application\r\n");

    LED_constructor(&greenLED, GREEN, LED_OFF);
    // Use a valid LEDColor_Type (e.g., RED) instead of casting wavelength enum
    MedicalLED_constructor(&redLED, INFRARED, RED, LED_OFF);
    powerLED_constructor(&yellowLED, YELLOW, LED_OFF, DIAM_5MM,CURRENT_NORMAL, VOL_NORMAL);
    powerLED_constructor(&blueLED, BLUE, LED_OFF, DIAM_7MM,CURRENT_HIGH, VOL_NORMAL);

    yellow_led_power = PowerLED_computePower(&yellowLED);
    blue_led_power = PowerLED_computePower(&blueLED);

    // Print initial readable status for all LEDs and measurements
    LED_getState(&usart, &redLED.super);
    LED_getState(&usart, &greenLED);
    LED_getState(&usart, (LED_Type*)&yellowLED);
    LED_getState(&usart, (LED_Type*)&blueLED);

    char buf[128];
    int n = snprintf(buf, sizeof(buf), "Medical LED wavelength is currently: %d nm\r\n"
                     "Power LED voltage: %u V\r\n"
                     "Power LED current: %u mA\r\n\r\n",
                     MedicalLED_getWavelength(&redLED),
                     PowerLED_getVoltage(&yellowLED),
                     PowerLED_getCurrent(&yellowLED));
    if (n > 0) {
        USART_WriteString(&usart, buf);
    }

    while (1) {

        delay(8000000);

        LED_setState(&redLED.super, LED_TOGGLE);
        LED_setState(&greenLED, LED_TOGGLE);
        LED_setState((LED_Type*)&yellowLED, LED_TOGGLE);
        LED_setState((LED_Type*)&blueLED, LED_TOGGLE);

        delay(8000000);

        LED_getState(&usart, &redLED.super);
        delay(800000);
        LED_getState(&usart, &greenLED);
        delay(800000);
        LED_getState(&usart, (LED_Type*)&yellowLED);
        delay(800000);
        LED_getState(&usart, (LED_Type*)&blueLED);

        delay(8000000);

        PowerLED_computeEfficiency(&usart, &yellowLED);
        PowerLED_runDiagnostics(&usart, &yellowLED);

        delay(8000000);

        PowerLED_computeEfficiency(&usart, &blueLED);
        PowerLED_runDiagnostics(&usart, &blueLED);

        delay(8000000);

        MedicalLED_computeEfficiency(&usart, &redLED);
        MedicalLED_runDiagnostics(&usart, &redLED);

    }
}

void delay(volatile uint32_t count) {
    while(count--) {
        __asm__("nop");
    }
}
