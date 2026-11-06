# ── Root_of_Trust — measures the bootloader at 0x08000000 ─────────────────
# Needs UART + NVIC + HASH + CRC. Runs in the active slot (0x08010000).
Root_of_Trust_DEPS       := UART NVIC HASH CRC
Root_of_Trust_FLASH_ADDR := 0x08010000
