# ── Bootloader — simple UART bootloader at 0x08000000 ─────────────────────
# Needs UART + NVIC + CRC + the FLASH driver from Projects/FLASH. Boots the
# active slot (0x08010000) — same memory map as Secure_Boot / tools/seal.py.
ifeq ($(MODULE),Bootloader)
DEPS       := UART NVIC CRC FLASH
EXTRA_INCS := Drivers/GPIO/inc

# Upload an application through Projects/Bootloader over the ST-LINK VCP:
#   1. Resets the board over SWD so the bootloader opens its 3 s update window
#   2. Streams the binary, bootloader programs the active slot + writes the CRC tag
# Usage: make bl-upload APP=Projects/App_Demo/main.bin [PORT=COM5]
APP ?= Projects/App_Demo/main.bin
.PHONY: bl-upload
bl-upload:
	$(PYTHON) tools/bl_upload.py $(APP) $(if $(PORT),--port $(PORT),) --reset

define MODULE_HELP
@echo "  bl-upload         Upload APP=<bin> through the bootloader over UART (APP=$(APP))"
endef
endif
