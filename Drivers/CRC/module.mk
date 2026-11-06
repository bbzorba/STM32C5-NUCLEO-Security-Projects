# ── CRC — hardware CRC-32 driver ──────────────────────────────────────────
# Library interface: linked into any module that lists CRC in its DEPS
CRC_SRC_C := Drivers/CRC/src/crc.c
CRC_INC   := Drivers/CRC/inc

# Standalone build (PROJECT_DIR = Drivers/CRC): needs GPIO and UART drivers
ifeq ($(MODULE),CRC)
DEPS := GPIO UART
endif
