# ── App_Demo — minimal application for the Secure Boot handoff ────────────
# Needs UART + NVIC (for IRQ dispatch).
# Flash address is 0x08010000 (Secure Boot active slot) — NOT 0x08000000.
ifeq ($(MODULE),App_Demo)
DEPS       := UART NVIC
FLASH_ADDR := 0x08010000
# seal-active / restore-bootloader + clear the stale CRC tag after `make flash`
include tools/mk/active_slot.mk
endif
