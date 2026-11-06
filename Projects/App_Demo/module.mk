# ── App_Demo — minimal application for the Secure Boot handoff ────────────
# Needs UART + NVIC (for IRQ dispatch).
# Flash address is 0x08010000 (Secure Boot active slot) — NOT 0x08000000.
App_Demo_DEPS       := UART NVIC
App_Demo_FLASH_ADDR := 0x08010000
