# ── ECDSA — signature verification (used by Secure_Boot / Secure_FW_Update)
# Library: linked into any module that lists ECDSA in its _DEPS
ECDSA_SRC_C := Projects/ECDSA/src/ecdsa.c
ECDSA_INC   := Projects/ECDSA/inc

# Standalone build (PROJECT_DIR = Projects/ECDSA): needs GPIO, UART and HASH drivers
ECDSA_DEPS  := GPIO UART HASH
