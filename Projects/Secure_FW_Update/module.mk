# ── Secure_FW_Update — signed update into the fallback slot ───────────────
# Needs UART + NVIC + HASH + RNG + ECDSA + CRC + the FLASH driver from
# Projects/FLASH. Runs in the active slot (0x08010000); writes the update
# to the fallback slot.
Secure_FW_Update_DEPS       := UART NVIC HASH RNG ECDSA CRC FLASH
Secure_FW_Update_INCS       := Drivers/GPIO/inc
Secure_FW_Update_FLASH_ADDR := 0x08010000
