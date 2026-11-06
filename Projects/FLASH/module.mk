# ── FLASH — flash erase/program/protect driver + demo ─────────────────────
# Library interface: linked into any module that lists FLASH in its DEPS
# (flash_protect.c is only part of the standalone demo)
FLASH_SRC_C := Projects/FLASH/src/flash.c
FLASH_INC   := Projects/FLASH/inc

# Standalone build (PROJECT_DIR = Projects/FLASH): needs AES, CRC, UART and SysTick drivers
ifeq ($(MODULE),FLASH)
DEPS       := UART SysTick CRC AES
EXTRA_INCS := Drivers/GPIO/inc
endif
