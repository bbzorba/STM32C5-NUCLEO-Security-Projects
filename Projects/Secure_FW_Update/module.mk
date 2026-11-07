# ── Secure_FW_Update — signed update into the fallback slot ───────────────
# Needs UART + NVIC + HASH + RNG + ECDSA + CRC + the FLASH driver from
# Projects/FLASH. Runs in the active slot (0x08010000); writes the update
# to the fallback slot.
ifeq ($(MODULE),Secure_FW_Update)
DEPS       := UART NVIC HASH RNG ECDSA CRC FLASH
EXTRA_INCS := Drivers/GPIO/inc
# linker/app_slot.ld, 0x08010000
LAYOUT     := app_slot
# seal-active / restore-bootloader + clear the stale CRC tag after `make flash`
include tools/mk/active_slot.mk
endif
