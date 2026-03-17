
#include "powerLed.h"
#include <stdio.h>


void powerLED_constructor(powerLED_Type * const pled, 
                          LEDColor_Type _color, 
                          LEDState_Type _state,
                          LEDDiameter_Type _diameter,
                          LEDCurrent_Type _current,
                          LEDVoltage_Type _voltage){
                            
                            static struct led_vtable const vtable = {
                                (void (*)(USART_HandleType *, LED_Type const * const))PowerLED_runDiagnostics,
                                (uint32_t (*)(USART_HandleType *, LED_Type const * const))PowerLED_computeEfficiency
                            };
                            LED_constructor(&pled->super,_color,_state);
                            pled->super.vptr = &vtable; // Override vtable to PowerLED's
                            pled->diameter = _diameter;
                            pled->voltage = _voltage;
                            pled->current = _current;
                          }

void PowerLED_setDiameter(powerLED_Type * const pled, LEDDiameter_Type _diameter){
    pled->diameter = _diameter;
}

void PowerLED_setCurrent(powerLED_Type * const pled, LEDCurrent_Type _current){
    pled->current = _current;
}

void PowerLED_setVoltage(powerLED_Type * const pled, LEDVoltage_Type _voltage){
    pled->voltage = _voltage;
}

led_elec_type PowerLED_computePower(powerLED_Type * const pled){
    // Return power in milliwatts (mA * V = mW)
    return (pled->current * pled->voltage);
}

led_elec_type PowerLED_getDiameter(powerLED_Type * const pled){
    return pled->diameter;
}

led_elec_type PowerLED_getCurrent(powerLED_Type * const pled){
    return pled->current;
}

led_elec_type PowerLED_getVoltage(powerLED_Type * const pled){
    return pled->voltage;
}

uint32_t PowerLED_computeEfficiency(USART_HandleType *usart, const powerLED_Type* const pled) {
    const int8_t input = 120;
    const int8_t output = 86;
    int8_t eff = ((input/output)*100) - (pled->current)/2;

    const char *color_name = (pled->super.color == YELLOW) ? "YELLOW"
                           : (pled->super.color == BLUE)   ? "BLUE"
                                                           : "UNKNOWN";
    char buf[32];
    USART_WriteString(usart, "-- Power LED Efficiency (");
    USART_WriteString(usart, color_name);
    USART_WriteString(usart, ") --\r\n");
    int n = snprintf(buf, sizeof(buf), "Efficiency: %d%%\r\n\r\n", eff);
    if (n > 0) USART_WriteString(usart, buf);

    return (uint32_t)eff;
}

void PowerLED_runDiagnostics(USART_HandleType * usart, const powerLED_Type* const pled) {
    const char *color_name = (pled->super.color == YELLOW) ? "YELLOW"
                           : (pled->super.color == BLUE)   ? "BLUE"
                                                           : "UNKNOWN";

    USART_WriteString(usart, "-- Power LED Diagnostics (");
    USART_WriteString(usart, color_name);
    USART_WriteString(usart, ") --\r\n");

    char buf[48];
    // i < current: one step per 10 mA (range [0, current)
    for (int i = 0; i < (int)pled->current; i += 10) {
        led_elec_type pwr = PowerLED_computePower((powerLED_Type*)pled);
        int n = snprintf(buf, sizeof(buf), "Step %d: power = %u mW\r\n", i, (unsigned)pwr);
        if (n > 0) USART_WriteString(usart, buf);
    }

    USART_WriteString(usart, "-- End Diagnostics (");
    USART_WriteString(usart, color_name);
    USART_WriteString(usart, ") --\r\n\r\n");
}
