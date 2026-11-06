# ── UART — register-level USART driver (STM32C5 port, USART v2) ───────────
# Library: linked into any module that lists UART in its _DEPS
UART_SRC_C := Drivers/UART/src/uart.c
UART_INC   := Drivers/UART/inc

# Standalone build (PROJECT_DIR = Drivers/UART): needs GPIO and DMA drivers
UART_DEPS  := GPIO DMA
