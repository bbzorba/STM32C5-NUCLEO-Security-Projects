# ── RSA — standalone demo at 0x08000000 ───────────────────────────────────
# Needs UART + NVIC only (no peripheral drivers).
ifeq ($(MODULE),RSA)
DEPS       := UART NVIC
EXTRA_INCS := Drivers/GPIO/inc
# 8 KB: modular exponentiation uses ~3.2 KB of stack
STACK_SIZE := 0x2000
endif
