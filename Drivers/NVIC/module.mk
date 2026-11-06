# ── NVIC — interrupt dispatch driver ──────────────────────────────────────
# Library interface: always linked by the top-level Makefile — nvic.c provides
# the strong USART + EXTI IRQ handlers every startup.c vector table refers to.
NVIC_SRC_C := Drivers/NVIC/src/nvic.c
NVIC_INC   := Drivers/NVIC/inc

# Standalone build (PROJECT_DIR = Drivers/NVIC): needs GPIO + UART + DMA drivers
ifeq ($(MODULE),NVIC)
DEPS := GPIO UART DMA
endif
