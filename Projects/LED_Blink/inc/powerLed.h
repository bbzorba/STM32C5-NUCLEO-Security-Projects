#ifndef _POWERLED_H
#define _POWERLED_H

#include "led.h"

typedef uint8_t led_elec_type;
typedef uint8_t led_dim_type;

typedef enum{

    CURRENT_LOW = 10, //10 mA
    CURRENT_NORMAL = 20,
    CURRENT_HIGH = 30,
    CURRENT_VERY_HIGH = 60
}LEDCurrent_Type;

typedef enum{
    DIAM_2MM = 2,
    DIAM_5MM = 5,
    DIAM_7MM = 7
}LEDDiameter_Type;

typedef enum{
    VOL_LOW = 3, //3V
    VOL_NORMAL = 5,
    VOL_HIGH = 9
}LEDVoltage_Type;


typedef struct{
    LED_Type super;
    LEDDiameter_Type diameter;
    LEDCurrent_Type current;
    LEDVoltage_Type voltage;
}powerLED_Type;

void powerLED_constructor(powerLED_Type * const pled, 
                          LEDColor_Type _color, 
                          LEDState_Type _state,
                          LEDDiameter_Type _diameter,
                          LEDCurrent_Type _current,
                          LEDVoltage_Type _voltage);

void PowerLED_setDiameter(powerLED_Type * const pled, LEDDiameter_Type _diameter);
void PowerLED_setCurrent(powerLED_Type * const pled, LEDCurrent_Type _current);
void PowerLED_setVoltage(powerLED_Type * const pled, LEDVoltage_Type _voltage);
led_elec_type PowerLED_computePower(powerLED_Type * const pled);
led_elec_type PowerLED_getDiameter(powerLED_Type * const pled);
led_elec_type PowerLED_getCurrent(powerLED_Type * const pled);
led_elec_type PowerLED_getVoltage(powerLED_Type * const pled);

uint32_t PowerLED_computeEfficiency(USART_HandleType *usart, powerLED_Type const * const pled);
void PowerLED_runDiagnostics(USART_HandleType *usart, powerLED_Type const * const pled);

#endif