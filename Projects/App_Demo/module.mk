# ── App_Demo — minimal application for the Secure Boot handoff ────────────
# Needs UART + NVIC (for IRQ dispatch).
# Runs in the active slot (0x08010000) — NOT 0x08000000.
ifeq ($(MODULE),App_Demo)
DEPS       := UART NVIC
# linker/app_slot.ld, 0x08010000
LAYOUT     := app_slot
# seal-active / restore-bootloader + clear the stale CRC tag after `make flash`
include tools/mk/active_slot.mk
endif
