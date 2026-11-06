# ── RNG — true random number generator driver ─────────────────────────────
# Library: linked into any module that lists RNG in its _DEPS
RNG_SRC_C := Drivers/RNG/src/rng.c
RNG_INC   := Drivers/RNG/inc

# Standalone build (PROJECT_DIR = Drivers/RNG): needs GPIO and UART drivers
RNG_DEPS  := GPIO UART
