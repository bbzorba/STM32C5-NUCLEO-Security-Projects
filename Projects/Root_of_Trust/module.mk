# ── Root_of_Trust — measures the bootloader at 0x08000000 ─────────────────
# Needs UART + NVIC + HASH + CRC. Runs in the active slot (0x08010000).
ifeq ($(MODULE),Root_of_Trust)
DEPS       := UART NVIC HASH CRC
FLASH_ADDR := 0x08010000
# seal-active / restore-bootloader + clear the stale CRC tag after `make flash`
include tools/mk/active_slot.mk
endif
