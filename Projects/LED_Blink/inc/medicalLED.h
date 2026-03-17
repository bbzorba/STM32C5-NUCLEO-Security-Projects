#ifndef MEDICAL_LED_H
#define MEDICAL_LED_H

#include "led.h"
#include <stdint.h>

typedef enum {
    INFRARED = 910, // nm
    NEAR_INFRARED = 880,
} LEDWavelength_Type;

typedef struct
{
    LED_Type super;
    LEDWavelength_Type wavelength;
} MedicalLED_Type;

void MedicalLED_constructor(MedicalLED_Type* const mled,
                            LEDWavelength_Type _wavelength,
                            LEDColor_Type _color, 
                            LEDState_Type _state);

LEDWavelength_Type MedicalLED_getWavelength(const MedicalLED_Type* const mled);
void MedicalLED_setWavelength(MedicalLED_Type* const mled, LEDWavelength_Type _wavelength);

uint32_t MedicalLED_computeEfficiency(USART_HandleType * usart, const MedicalLED_Type* const mled);
void MedicalLED_runDiagnostics(USART_HandleType * usart, const MedicalLED_Type* const mled);

#endif // MEDICAL_LED_H