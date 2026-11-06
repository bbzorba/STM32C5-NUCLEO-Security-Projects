# ── Secure Boot active slot (0x08010000) — shared upload targets ──────────
# Included from the module.mk of the applications that run in the active slot
# (App_Demo, Root_of_Trust, Secure_FW_Update), inside their ifeq ($(MODULE),…)
# block, so these targets only exist when one of them is PROJECT_DIR.

# `make flash` writes the slot without sealing it, so the CRC tag left by an
# earlier seal no longer matches — erase it so Secure_Boot skips the slot
# instead of halting with "BOOT FAILED".
define POST_FLASH
@echo "  Active slot written without sealing — clearing stale CRC tag (sector 30)."
@echo "  Run 'make seal-active' to properly seal."
$(PYTHON) tools/clear_tag.py
endef

# Seal the active application slot:
#   1. Build the application binary
#   2. Pad to 176 KB, compute CRC-32, flash app + write CRC tag, reset board
.PHONY: seal-active
seal-active: build
	$(PYTHON) tools/seal.py $(TARGET).bin

# Restore Secure_Boot to 0x08000000 after testing Flash_Encrypt.
# Flash_Encrypt overwrites the bootloader; run this target before trying
# to test any active-slot project.
.PHONY: restore-bootloader
restore-bootloader:
	$(MAKE) flash PROJECT_DIR=Projects/Secure_Boot

# Listed in `make help` (section "Active-slot apps"); MODULE_HELP marks them available
MODULE_HELP := active-slot
