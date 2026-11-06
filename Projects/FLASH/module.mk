# ── FLASH — flash erase/program/protect driver + demo ─────────────────────
# Library: linked into any module that lists FLASH in its _DEPS
# (flash_protect.c is only part of the standalone demo)
FLASH_SRC_C := Projects/FLASH/src/flash.c
FLASH_INC   := Projects/FLASH/inc

# Standalone build (PROJECT_DIR = Projects/FLASH): needs AES, CRC, UART and SysTick drivers
FLASH_DEPS  := UART SysTick CRC AES
FLASH_INCS  := Drivers/GPIO/inc
