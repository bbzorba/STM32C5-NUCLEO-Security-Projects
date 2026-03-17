#include "medicalLED.h"
#include <stdio.h>

static const char* LEDColorToString_med(LEDColor_Type color) {
    switch (color) {
        case GREEN: return "GREEN";
        case YELLOW: return "YELLOW";
        case RED: return "RED";
        case BLUE: return "BLUE";
        default: return "UNKNOWN";
    }
}

void MedicalLED_constructor(MedicalLED_Type* const mled,
                            LEDWavelength_Type _wavelength,
                            LEDColor_Type _color, 
                            LEDState_Type _state) {
    static struct led_vtable const vtable = {
        (void (*)(USART_HandleType *, LED_Type const * const))MedicalLED_runDiagnostics,
        (uint32_t (*)(USART_HandleType *, LED_Type const * const))MedicalLED_computeEfficiency
    };
    
    // Initialize base LED part
    LED_constructor(&mled->super, _color, _state);
    // Initialize MedicalLED specific part
    mled->wavelength = _wavelength;
    mled->super.vptr = &vtable; // Override vtable to MedicalLED's
}

LEDWavelength_Type MedicalLED_getWavelength(const MedicalLED_Type* const mled) {
    return mled->wavelength;
}

void MedicalLED_setWavelength(MedicalLED_Type* const mled, LEDWavelength_Type _wavelength) {
    mled->wavelength = _wavelength;
}

uint32_t MedicalLED_computeEfficiency(USART_HandleType * usart, const MedicalLED_Type* const mled) {
    const int8_t input = 120;
    const int8_t output = 86;
    const int8_t k1 = 4;
    const int8_t k2 = 6;

    int8_t K = (mled->wavelength == INFRARED) ? k1 : k2;
    int8_t efficiency = ((input/output)*100) - K;

    char buf[32];
    int n = snprintf(buf, sizeof(buf), "Medical LED Efficiency: %d%%\r\n\r\n", efficiency);
    if (n > 0) USART_WriteString(usart, buf);

    return (uint32_t)efficiency;
}

void MedicalLED_runDiagnostics(USART_HandleType * usart, const MedicalLED_Type* const mled) {
    const char *color_name = LEDColorToString_med(mled->super.color);

    USART_WriteString(usart, "-- Medical LED Diagnostics (");
    USART_WriteString(usart, color_name);
    USART_WriteString(usart, ") --\r\n");

    char buf[24];
    for (int i = 0; i < 10; i++) {
        int n = snprintf(buf, sizeof(buf), "Stage %d\r\n", i);
        if (n > 0) USART_WriteString(usart, buf);
    }

    USART_WriteString(usart, "-- End Diagnostics (");
    USART_WriteString(usart, color_name);
    USART_WriteString(usart, ") --\r\n\r\n");
}
