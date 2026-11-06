# ── AES — hardware AES driver ─────────────────────────────────────────────
# Library: linked into any module that lists AES in its _DEPS
# (modes_of_op.c is only part of the standalone demo)
AES_SRC_C := Drivers/AES/src/aes.c
AES_INC   := Drivers/AES/inc

# Standalone build (PROJECT_DIR = Drivers/AES): needs GPIO and UART drivers
AES_DEPS  := GPIO UART
