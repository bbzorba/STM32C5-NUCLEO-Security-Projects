# ── HASH — hardware SHA-256 driver ────────────────────────────────────────
# Library: linked into any module that lists HASH in its _DEPS
HASH_SRC_C := Drivers/HASH/src/hash.c
HASH_INC   := Drivers/HASH/inc

# Standalone build (PROJECT_DIR = Drivers/HASH): needs GPIO and UART drivers
HASH_DEPS  := GPIO UART
