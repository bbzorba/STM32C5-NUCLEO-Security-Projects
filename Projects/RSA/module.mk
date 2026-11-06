# ── RSA — standalone demo at 0x08000000 ───────────────────────────────────
# Needs UART + NVIC only (no peripheral drivers).
RSA_DEPS := UART NVIC
RSA_INCS := Drivers/GPIO/inc
