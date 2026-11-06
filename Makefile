# Makefile for STM32 NUCLEO C562RE development board

#DONE
#PROJECT_DIR = Drivers/DMA
#PROJECT_DIR = Drivers/NVIC
#PROJECT_DIR = Drivers/GPIO
#PROJECT_DIR = Drivers/UART
#PROJECT_DIR = Drivers/FDCAN
#PROJECT_DIR = Drivers/HASH
#PROJECT_DIR = Drivers/CRC
#PROJECT_DIR = Drivers/AES
#PROJECT_DIR = Drivers/RNG
#PROJECT_DIR = Projects/ECDSA
#PROJECT_DIR = Projects/FLASH
#PROJECT_DIR = Drivers/SysTick
#PROJECT_DIR = Projects/App_Demo
#PROJECT_DIR = Projects/RSA
#PROJECT_DIR = Projects/Root_of_Trust
#PROJECT_DIR = Projects/Bootloader

#TBD
PROJECT_DIR = Projects/Secure_Boot
#PROJECT_DIR = Projects/Secure_FW_Update

CXX=arm-none-eabi-g++
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
MCU := cortex-m33
BOARD := NUCLEO-C562RE

# ── Per-board debug configuration ─────────────────────────
# These values are written into .vscode/settings.json by `make gen-debug-config`
# so that launch.json stays board-independent via ${config:stm32.*} variables.
ifeq ($(strip $(BOARD)),NUCLEO-C562RE)
DEBUG_DEVICE  := STM32C562RE
DEBUG_SVD     := STM32C562.svd
DEBUG_CFG     := stm32c562re.cfg
DEBUG_TAPID   := 0x6ba02477
endif
# Platform-specific shell & command selection.
# On Windows we prefer a POSIX shell (sh.exe from Git-for-Windows, MSYS2,
# Cygwin …) because cmd.exe can misinterpret forward-slash include paths
# (e.g. -IDrivers/compat_inc) as its own /C switch, causing:
#   "'ompat_inc' is not recognized as an internal or external command"
# If no POSIX shell is found we fall back to cmd.exe.
# On POSIX (Linux / macOS / WSL): use the default POSIX shell.
ifeq ($(OS),Windows_NT)
# Check whether Make already auto-detected a POSIX shell
ifneq (,$(filter %sh %sh.exe %bash %bash.exe,$(SHELL)))
_POSIX_SH := 1
RM        := rm -f
DEVNULL   := /dev/null
else
# Make defaulted to cmd.exe — look for sh.exe in PATH
_SH_CHECK := $(shell where sh.exe 2>nul)
ifneq ($(_SH_CHECK),)
SHELL     := sh.exe
_POSIX_SH := 1
RM        := rm -f
DEVNULL   := /dev/null
else
# No POSIX shell available — use cmd.exe as last resort
SHELL       := cmd.exe
.SHELLFLAGS := /C
_POSIX_SH   := 0
RM          := del /Q /F
DEVNULL     := nul
endif
endif
else
_POSIX_SH := 1
RM        := rm -f
DEVNULL   := /dev/null
endif

# ── Module wiring ─────────────────────────────────────────
# Every driver/project folder carries a module.mk. Its variables are prefixed
# with the folder name (e.g. UART_*, Secure_Boot_*):
#   <Name>_SRC_C       library source(s) linked into modules that depend on it
#   <Name>_INC         library include directory
#   <Name>_DEPS        modules linked when this folder is built as PROJECT_DIR
#   <Name>_INCS        extra include-only directories for that build
#   <Name>_FLASH_ADDR  load address when it is not 0x08000000
# A new driver/project only needs its own module.mk — nothing to edit here.
include $(wildcard Drivers/*/module.mk Projects/*/module.mk)

MODULE := $(notdir $(strip $(PROJECT_DIR)))
ifeq ($(wildcard $(PROJECT_DIR)/module.mk),)
$(warning $(PROJECT_DIR) has no module.mk — building its own sources only)
endif

# nvic.c provides strong USART+EXTI IRQ handler definitions for ALL projects.
# Startup files reference these names in their vector tables, so NVIC is
# always a dependency.
DEPS := $($(MODULE)_DEPS) $(filter-out $($(MODULE)_DEPS),NVIC)

# Flashing configuration
FLASH_ADDR = $(if $($(MODULE)_FLASH_ADDR),$($(MODULE)_FLASH_ADDR),0x08000000)
FLASH_TOOL?=cubeprog  # options: cubeprog | openocd | stlink
# Sanitize FLASH_TOOL in case of stray spaces from environment or edits
_FLASH_TOOL:=$(strip $(FLASH_TOOL))
CUBE_PROG?=C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe
OPENOCD?=openocd
STFLASH?=st-flash
OPENOCD_SCRIPTS?=
OPENOCD_IF?=interface/stlink-dap.cfg
OPENOCD_TARGET?=target/stm32u5x.cfg
OPENOCD_EXTRA?=-c "transport select dapdirect_swd" -c "set CPUTAPID 0x6ba02477"

CFLAGS=-mcpu=$(MCU) -mthumb -Wall -O2 -g -DSTM32C5xx \
	-IDrivers/compat_inc \
	-IDrivers/CMSIS \
	-I$(PROJECT_DIR)/inc \
	-ffunction-sections -fdata-sections

# DEBUG=1 enables debugger-friendly codegen. This improves breakpoint reliability.
ifeq ($(strip $(DEBUG)),1)
CFLAGS := $(filter-out -O2,$(CFLAGS)) -Og -g3 -fno-inline
endif

# All current projects target STM32C5 (Cortex-M33) via ST-LINK DAP.

# C++ flags largely mirror C; disable RTTI/exceptions to keep size small
CXXFLAGS=$(CFLAGS) -fno-exceptions -fno-rtti -fno-use-cxa-atexit

# Serial monitor settings (Windows PowerShell)
PORT ?=
BAUD ?= 115200
MONITOR_SECONDS ?=
LDFLAGS=-T$(PROJECT_DIR)/linker.ld --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections

# Optional external driver selection -------------------------------------------------
# By default (when DRIVERS is empty) we build ONLY the selected PROJECT sources
# plus its module.mk dependencies. To pull in all driver code under
# Drivers/<Name>, invoke:
#   make DRIVERS="UART I2C"   (names are directory basenames)
# We still exclude HAL, CMSIS, compat headers, and the active project if it lives
# under Drivers/.
ifeq ($(strip $(DRIVERS)),)
FILTERED_DRIVER_DIRS :=
else
DRIVER_DIRS_REQUESTED := $(addprefix Drivers/,$(DRIVERS))
# Filter out directories that do not exist to avoid warnings and dedupe
DRIVER_DIRS_EXISTING := $(sort $(wildcard $(DRIVER_DIRS_REQUESTED)))
FILTERED_DRIVER_DIRS := $(sort $(filter-out Drivers/STM32F4xx_HAL_Driver Drivers/CMSIS Drivers/compat_inc $(PROJECT_DIR),$(DRIVER_DIRS_EXISTING)))
endif

DRIVER_INCLUDES := $(foreach d,$(FILTERED_DRIVER_DIRS),-I$(d)/inc)
EXTERNAL_SRC_C_RAW   := $(foreach d,$(FILTERED_DRIVER_DIRS),$(wildcard $(d)/src/*.c))
EXTERNAL_SRC_CPP_RAW := $(foreach d,$(FILTERED_DRIVER_DIRS),$(wildcard $(d)/src/*.cpp))
EXTERNAL_EXCLUDE_C   := %/main.c %/startup.c %/system_stm32f4xx.c %/system_%.c
EXTERNAL_EXCLUDE_CPP := %/main.cpp %/startup.cpp %/system_%.cpp
EXTERNAL_SRC_C   := $(sort $(filter-out $(EXTERNAL_EXCLUDE_C),$(EXTERNAL_SRC_C_RAW)))
EXTERNAL_SRC_CPP := $(sort $(filter-out $(EXTERNAL_EXCLUDE_CPP),$(EXTERNAL_SRC_CPP_RAW)))

CFLAGS += $(DRIVER_INCLUDES)

# Project sources + the library sources/includes of every dependency
SRC_C   := $(wildcard $(PROJECT_DIR)/src/*.c)
SRC_CPP := $(wildcard $(PROJECT_DIR)/src/*.cpp)
SRC_C   += $(filter-out $(SRC_C),$(foreach d,$(DEPS),$($(d)_SRC_C)))
CFLAGS  += $(addprefix -I,$(foreach d,$(DEPS),$($(d)_INC)) $($(MODULE)_INCS))

OBJ_UNSORTED=$(SRC_C:.c=.o) $(SRC_CPP:.cpp=.o) $(EXTERNAL_SRC_C:.c=.o) $(EXTERNAL_SRC_CPP:.cpp=.o)
OBJ=$(sort $(OBJ_UNSORTED))
TARGET=$(PROJECT_DIR)/main

# Use C++ linker if there are any C++ sources (project or external driver), else C linker
ifneq (,$(strip $(SRC_CPP) $(EXTERNAL_SRC_CPP)))
LINKER := $(CXX)
else
LINKER := $(CC)
endif

# Pattern rules
%.o: %.s
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: help all build run clean flash flash_openocd flash_stlink flash_cubeprog

all: build

# All module wiring is resolved above (module.mk files are included before
# any rule), so OBJ is complete when this prerequisite list is read.
build: $(OBJ)
	$(LINKER) $(CFLAGS) $(OBJ) -Wl,-Map=$(TARGET).map -o $(TARGET).elf $(LDFLAGS) $(EXTRA_LDFLAGS)
	$(OBJCOPY) -O ihex $(TARGET).elf $(TARGET).hex
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).bin
	-python -c "import os; [os.remove(f) for f in '$(OBJ)'.split() if os.path.isfile(f)]"

run: build

flash: build
ifeq ($(_FLASH_TOOL),cubeprog)
	"$(CUBE_PROG)" -c port=SWD -halt -d $(TARGET).bin $(FLASH_ADDR) -rst
ifeq ($(strip $(FLASH_ADDR)),0x08010000)
	@echo "  Active slot written without sealing — clearing stale CRC tag (sector 30)."
	@echo "  Run 'make seal-active PROJECT_DIR=$(PROJECT_DIR)' to properly seal."
	python tools/clear_tag.py
endif
else ifeq ($(_FLASH_TOOL),openocd)
	"$(OPENOCD)" $(if $(OPENOCD_SCRIPTS),-s "$(OPENOCD_SCRIPTS)") -f $(OPENOCD_IF) $(OPENOCD_EXTRA) -f $(OPENOCD_TARGET) -c "program $(TARGET).elf verify reset exit"
else ifeq ($(_FLASH_TOOL),stlink)
	"$(STFLASH)" --reset write $(TARGET).bin $(FLASH_ADDR)
else
	@echo Unknown FLASH_TOOL=$(_FLASH_TOOL). Use cubeprog, openocd, or stlink.
	@exit 1
endif

flash_cubeprog: FLASH_TOOL=cubeprog
flash_cubeprog: build
	"$(CUBE_PROG)" -c port=SWD -halt -d $(TARGET).bin $(FLASH_ADDR) -rst

flash_openocd: FLASH_TOOL=openocd
flash_openocd: build
	"$(OPENOCD)" $(if $(OPENOCD_SCRIPTS),-s "$(OPENOCD_SCRIPTS)") -f $(OPENOCD_IF) $(OPENOCD_EXTRA) -f $(OPENOCD_TARGET) -c "program $(TARGET).elf verify reset exit"

flash_stlink: FLASH_TOOL=stlink
flash_stlink: build
	"$(STFLASH)" --reset write $(TARGET).bin $(FLASH_ADDR)

# Seal the active application slot (Projects/App_Demo workflow):
#   1. Build the application binary
#   2. Pad to 176 KB, compute CRC-32, flash app + write CRC tag, reset board
# Usage: make seal-active   (with PROJECT_DIR = Projects/App_Demo)
.PHONY: seal-active
seal-active: build
	python tools/seal.py $(TARGET).bin

# Restore Secure_Boot to 0x08000000 after testing Flash_Encrypt.
# Flash_Encrypt overwrites the bootloader; run this target before trying
# to test any active-slot project (App_Demo / Root_of_Trust / Secure_FW_Update).
.PHONY: restore-bootloader
restore-bootloader:
	$(MAKE) flash PROJECT_DIR=Projects/Secure_Boot

# Open a simple serial monitor with PowerShell
.PHONY: monitor
monitor:
ifeq ($(OS),Windows_NT)
	powershell -NoProfile -ExecutionPolicy Bypass -File "tools/monitor.ps1" $(if $(PORT),-ComPort $(PORT),) -Baud $(BAUD) $(if $(MONITOR_SECONDS),-DurationSec $(MONITOR_SECONDS),)
else
	@echo "This monitor target is Windows-specific (PowerShell)."
endif

.PHONY: com-list
com-list:
ifeq ($(OS),Windows_NT)
	powershell -NoProfile -Command "Get-CimInstance Win32_SerialPort | Select-Object Name,DeviceID | Format-Table -AutoSize"
else
	@echo "Use: ls /dev/tty*"
endif

# Build, flash, then start monitor automatically
.PHONY: flashmonitor-auto
flashmonitor-auto: all flash monitor

clean:
	-python -c "import os; [os.remove(f) for f in ('$(OBJ) $(TARGET).elf $(TARGET).hex $(TARGET).bin $(TARGET).map').split() if os.path.isfile(f)]"

# Print any Makefile variable, e.g. make print-PROJECT_DIR
.PHONY: print-%
print-%:
	@echo $($*)

# Default goal: show help when running plain `make`
.DEFAULT_GOAL := help

# Help target: list common make targets and usage
help:
	@echo "Usage: make [target]"
	@echo "Common targets:"
	@echo "  build             Build the current PROJECT_DIR (same as 'make all')"
	@echo "  flash             Build and flash using FLASH_TOOL (cubeprog/openocd/stlink)"
	@echo "  flash_openocd     Flash with openocd"
	@echo "  flash_stlink      Flash with st-flash"
	@echo "  flash_cubeprog    Flash with STM32_Programmer_CLI"
	@echo "  monitor           Open serial monitor (Windows PowerShell)"
	@echo "                    Auto-detects USB-serial bridge when PORT is empty"
	@echo "                    Use MONITOR_SECONDS=N for timed CI verification"
	@echo "  com-list          List serial ports (Windows PowerShell)"
	@echo "  clean             Remove build artifacts"
	@echo "  run               Alias for build"
	@echo "  flashmonitor-auto Build, flash, and start monitor"
	@echo
	@echo "To build immediately, run: make build"

# --------------------
# IAR toolchain integration
# Builds the SAME source files as the GCC target using iccarm/ilinkarm directly.
# No .ewp project file required — all configuration lives here in the Makefile.
#
# Tool paths:  IAR_ROOT  = root of IAR EWARM install
#              ICCARM    = ARM C compiler
#              ILINKARM  = ARM linker
#              IELFTOOL  = converts .out → .hex / .bin  (like arm-none-eabi-objcopy)
#
# Overrideable from command line, e.g.:
#   make iar-build IAR_ROOT=C:/IAR/EWARM9
# --------------------

IAR_ROOT   ?= D:/iar/ewarm-9.70.4
ICCARM     := $(IAR_ROOT)/arm/bin/iccarm.exe
ILINKARM   := $(IAR_ROOT)/arm/bin/ilinkarm.exe
IELFTOOL   := $(IAR_ROOT)/arm/bin/ielftool.exe

# IAR output directory — fixed at workspace root so VS Code debug configs stay
# project-independent (launch.json always points to iar_out/main.out).
IAR_OUT    := iar_out

# IAR linker config — lives beside linker.ld in the project directory
IAR_ICF    ?= stm32c562re.icf

# All source files the IAR build should compile (same set as GCC, deduplicated)
IAR_ALL_SRC := $(sort $(SRC_C) $(EXTERNAL_SRC_C))

.PHONY: iar-build iar-clean iar-flash iar-debug
iar-build:
	@echo "----------------------------------------------------"
	@echo "  IAR build: $(PROJECT_DIR) [iccarm V9.70, Cortex-M33]"
	@echo "----------------------------------------------------"
ifeq ($(_POSIX_SH),1)
	@test -f "$(ICCARM)" || { echo "ERROR: iccarm not found at $(ICCARM)"; echo "Set IAR_ROOT, e.g.: make iar-build IAR_ROOT=C:/IAR/EWARM9"; exit 1; }
	@test -f "$(IAR_ICF)" || { echo "ERROR: IAR linker config not found at $(IAR_ICF)"; exit 1; }
	@mkdir -p "$(IAR_OUT)"
else
	@if not exist "$(ICCARM)" (echo ERROR: iccarm not found at $(ICCARM) & echo Set IAR_ROOT, e.g.: make iar-build IAR_ROOT=C:/IAR/EWARM9 & exit /b 1)
	@if not exist "$(IAR_ICF)" (echo ERROR: IAR linker config not found at $(IAR_ICF) & exit /b 1)
	@if not exist "$(IAR_OUT)" mkdir "$(IAR_OUT)"
endif
	python tools/iar_build.py \
	  --iccarm "$(ICCARM)" \
	  --ilinkarm "$(ILINKARM)" \
	  --ielftool "$(IELFTOOL)" \
	  --icf "$(IAR_ICF)" \
	  --opt-level $(if $(filter 1,$(DEBUG)),none,size) \
	  --outdir "$(IAR_OUT)" \
	  --inc "$(PROJECT_DIR)/inc" \
	  $(foreach d,$(FILTERED_DRIVER_DIRS),--inc "$(d)/inc") \
	  --define DSTM32C5xx \
	  $(IAR_ALL_SRC)

iar-flash: iar-build
	"$(CUBE_PROG)" -c port=SWD -halt -d "$(IAR_OUT)/main.bin" $(FLASH_ADDR) -rst

iar-clean:
	-python -c "import shutil,os; shutil.rmtree('$(IAR_OUT)', ignore_errors=True)"

iar-debug:
	@$(MAKE) iar-build DEBUG=1
	"$(CUBE_PROG)" -c port=SWD reset=HWrst -w "$(IAR_OUT)/main.bin" $(FLASH_ADDR) -v -hardRst
	@echo ""
	@echo "--- IAR build + flash complete: $(IAR_OUT)/main.out ---"
	@echo "Press F5 in VS Code [select IAR C-SPY: ST-LINK] to start debugging."

# TOOLCHAIN=iar redirects the standard targets
ifeq ($(strip $(TOOLCHAIN)),iar)
build: iar-build
flash: iar-flash
clean: iar-clean
endif

# GCC output mirror — keeps a project-independent copy so VS Code cortex-debug
# launch config always finds build_out/main.elf regardless of PROJECT_DIR.
BUILD_OUT := build_out

.PHONY: debug-build debug

# debug-build: compile with debug flags, copy ELF to build_out/.
# Does NOT flash — cortex-debug's GDB 'load' command flashes via OpenOCD
# so that GDB's breakpoint table stays in sync with the target.

debug-build:
	@$(MAKE) clean
	@$(MAKE) build DEBUG=1
	python -c "import os; os.makedirs('$(BUILD_OUT)', exist_ok=True)"
	python -c "import shutil; shutil.copy('$(TARGET).elf', '$(BUILD_OUT)/main.elf'); shutil.copy('$(TARGET).bin', '$(BUILD_OUT)/main.bin')"
	@echo --- Debug build ready: $(BUILD_OUT)/main.elf ---

# debug: standalone build + flash (without VS Code debugger)
debug: debug-build
	"$(CUBE_PROG)" -c port=SWD reset=HWrst -w "$(BUILD_OUT)/main.bin" $(FLASH_ADDR) -v -hardRst
	@echo ""
	@echo "--- GCC build + flash complete: $(BUILD_OUT)/main.elf ---"
	@echo "Press F5 in VS Code [select Cortex Debug: ST-LINK] to start debugging."

# gen-debug-config: writes board-specific stm32.* keys into .vscode/settings.json
# so that the generic launch.json (which uses ${config:stm32.*}) stays correct
# whenever BOARD changes.  Run once after changing the BOARD variable.
.PHONY: gen-debug-config
gen-debug-config:
	python -c "\
import json, sys; \
path = '.vscode/settings.json'; \
s = json.load(open(path)) if __import__('os').path.isfile(path) else {}; \
s.update({'stm32.board': '$(DEBUG_DEVICE and BOARD or BOARD)', 'stm32.device': '$(DEBUG_DEVICE)', 'stm32.svdFile': '$(DEBUG_SVD)', 'stm32.openocdCfg': '$(DEBUG_CFG)', 'stm32.cpuTapId': '$(DEBUG_TAPID)'}); \
json.dump(s, open(path, 'w'), indent=2); \
print('--- .vscode/settings.json updated for board: $(BOARD) ---')"
