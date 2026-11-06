# ── AES — hardware AES driver ─────────────────────────────────────────────
# Library interface: linked into any module that lists AES in its DEPS
# (modes_of_op.c is only part of the standalone demo)
AES_SRC_C := Drivers/AES/src/aes.c
AES_INC   := Drivers/AES/inc

# Standalone build (PROJECT_DIR = Drivers/AES): needs GPIO and UART drivers
ifeq ($(MODULE),AES)
DEPS := GPIO UART
endif
