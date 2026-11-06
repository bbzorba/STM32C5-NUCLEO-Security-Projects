# ── RSA — standalone demo at 0x08000000 ───────────────────────────────────
# Needs UART + NVIC only (no peripheral drivers).
ifeq ($(MODULE),RSA)
DEPS       := UART NVIC
EXTRA_INCS := Drivers/GPIO/inc
endif
