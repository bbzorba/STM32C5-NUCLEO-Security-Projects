# ── ECDSA — signature verification (used by Secure_Boot / Secure_FW_Update) ───
# Library interface: linked into any module that lists ECDSA in its DEPS
ECDSA_SRC_C := Projects/ECDSA/src/ecdsa.c
ECDSA_INC   := Projects/ECDSA/inc

# Standalone build (PROJECT_DIR = Projects/ECDSA): needs GPIO, UART and HASH drivers
ifeq ($(MODULE),ECDSA)
DEPS := GPIO UART HASH
endif
