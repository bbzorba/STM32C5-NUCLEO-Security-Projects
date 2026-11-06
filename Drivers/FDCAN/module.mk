# ── FDCAN — CAN FD driver (STM32C5 port) ──────────────────────────────────
# Library: linked into any module that lists FDCAN in its _DEPS
FDCAN_SRC_C := Drivers/FDCAN/src/fdcan.c
FDCAN_INC   := Drivers/FDCAN/inc

# Standalone build (PROJECT_DIR = Drivers/FDCAN): needs GPIO and UART drivers
FDCAN_DEPS  := GPIO UART
