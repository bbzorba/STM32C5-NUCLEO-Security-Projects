# ── SysTick — system timer driver ─────────────────────────────────────────
# Library: linked into any module that lists SysTick in its _DEPS
SysTick_SRC_C := Drivers/SysTick/src/systick.c
SysTick_INC   := Drivers/SysTick/inc

# Standalone build (PROJECT_DIR = Drivers/SysTick): needs GPIO and UART drivers
SysTick_DEPS  := GPIO UART
