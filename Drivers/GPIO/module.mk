# ── GPIO — register-level GPIO driver (STM32C5 port) ──────────────────────
# Library interface: linked into any module that lists GPIO in its DEPS
GPIO_SRC_C := Drivers/GPIO/src/gpio.c
GPIO_INC   := Drivers/GPIO/inc

# Standalone build (PROJECT_DIR = Drivers/GPIO): own sources only (+ NVIC, always linked)
ifeq ($(MODULE),GPIO)
DEPS := 
endif
