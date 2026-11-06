# ── RNG — true random number generator driver ─────────────────────────────
# Library interface: linked into any module that lists RNG in its DEPS
RNG_SRC_C := Drivers/RNG/src/rng.c
RNG_INC   := Drivers/RNG/inc

# Standalone build (PROJECT_DIR = Drivers/RNG): needs GPIO and UART drivers
ifeq ($(MODULE),RNG)
DEPS := GPIO UART
endif
