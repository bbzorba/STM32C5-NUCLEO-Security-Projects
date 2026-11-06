# ── Secure_Boot — SHA-256 + ECDSA verified boot at 0x08000000 ─────────────
# Needs GPIO, UART, HASH, ECDSA, NVIC.
ifeq ($(MODULE),Secure_Boot)
DEPS := GPIO UART HASH ECDSA NVIC
endif
