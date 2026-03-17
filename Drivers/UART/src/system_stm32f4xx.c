#include <stdint.h>

uint32_t SystemCoreClock = 16000000UL; // Default HSI frequency

void SystemInit(void) {
    // Default: HSI 16MHz, no PLL
    SystemCoreClock = 16000000UL;
}
