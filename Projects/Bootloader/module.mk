# ── Bootloader — simple UART bootloader at 0x08000000 ─────────────────────
# Needs GPIO (B1 + LD1) + UART + USB_CDC (TinyUSB) + NVIC + CRC + the FLASH driver
# from Projects/FLASH. Boots the
# active slot (0x08010000) — same memory map as Secure_Boot / tools/seal.py.
# Library interface for applications (DEPS += Bootloader): bl_request.h —
# BL_RequestBootloader() resets into the bootloader and keeps it waiting.
Bootloader_SRC_C := Projects/Bootloader/src/bl_request.c
Bootloader_INC   := Projects/Bootloader/inc

ifeq ($(MODULE),Bootloader)
DEPS       := GPIO UART USB_CDC NVIC CRC FLASH
# linker/bootloader.ld, 64 KB at 0x08000000
LAYOUT     := bootloader

# Uploading an application through this bootloader is `make bl-upload`
# (top-level Makefile — available whatever PROJECT_DIR is selected).
endif
