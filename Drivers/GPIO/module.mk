# ── GPIO — register-level GPIO driver (STM32C5 port) ──────────────────────
# Library: linked into any module that lists GPIO in its _DEPS
GPIO_SRC_C := Drivers/GPIO/src/gpio.c
GPIO_INC   := Drivers/GPIO/inc

# Standalone build (PROJECT_DIR = Drivers/GPIO): own sources only
GPIO_DEPS  :=
