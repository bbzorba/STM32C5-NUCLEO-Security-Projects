# ── CRC — hardware CRC-32 driver ──────────────────────────────────────────
# Library: linked into any module that lists CRC in its _DEPS
CRC_SRC_C := Drivers/CRC/src/crc.c
CRC_INC   := Drivers/CRC/inc

# Standalone build (PROJECT_DIR = Drivers/CRC): needs GPIO and UART drivers
CRC_DEPS  := GPIO UART
