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
PROJECT_DIR = Projects/Bootloader

#TBD
#PROJECT_DIR = Projects/Secure_Boot
#PROJECT_DIR = Projects/Secure_FW_Update

# Default goal: show help when running plain `make`
.DEFAULT_GOAL := help

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

# ── Shell selection ───────────────────────────────────────
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

# Running inside WSL: Windows tools (.exe) are callable directly
_WSL := $(if $(WSL_DISTRO_NAME),1,)

# Look a command up on PATH; expands to its location or to nothing
ifeq ($(OS),Windows_NT)
on_path = $(shell where $(1) 2>$(DEVNULL))
else
on_path = $(shell command -v $(1) 2>/dev/null)
endif

_space := $(subst ,, )
# $(wildcard) for a single path that may contain spaces
path_exists = $(wildcard $(subst $(_space),\$(_space),$(1)))

# ── Toolchain ─────────────────────────────────────────────
# arm-none-eabi-gcc is taken from PATH. When it is not on PATH, the toolchain
# bundled with STM32CubeIDE is used (C:/ST/STM32CubeIDE_* on Windows and WSL,
# /opt/st/stm32cubeide_* on Linux). Override with TOOLCHAIN_DIR=<.../bin>.
ifeq ($(origin TOOLCHAIN_DIR),undefined)
ifeq ($(call on_path,arm-none-eabi-gcc),)
TOOLCHAIN_DIR := $(lastword $(sort $(wildcard \
	C:/ST/STM32CubeIDE*/STM32CubeIDE/plugins/*gnu-tools-for-stm32*/tools/bin \
	/mnt/c/ST/STM32CubeIDE*/STM32CubeIDE/plugins/*gnu-tools-for-stm32*/tools/bin \
	/opt/st/stm32cubeide*/plugins/*gnu-tools-for-stm32*/tools/bin)))
endif
endif

ifneq ($(strip $(TOOLCHAIN_DIR)),)
TOOLCHAIN_DIR := $(patsubst %/,%,$(strip $(TOOLCHAIN_DIR)))
_EXE := $(if $(wildcard $(TOOLCHAIN_DIR)/arm-none-eabi-gcc.exe),.exe,)
CC      := $(TOOLCHAIN_DIR)/arm-none-eabi-gcc$(_EXE)
CXX     := $(TOOLCHAIN_DIR)/arm-none-eabi-g++$(_EXE)
OBJCOPY := $(TOOLCHAIN_DIR)/arm-none-eabi-objcopy$(_EXE)
# WSL + Windows toolchain: gcc.exe cannot locate cc1/as/ld from the argv[0]
# WSL hands it ("cannot execute 'cc1'"), so pass its install prefix explicitly.
ifeq ($(_WSL)$(_EXE),1.exe)
export GCC_EXEC_PREFIX := $(shell wslpath -m '$(patsubst %/bin,%,$(TOOLCHAIN_DIR))')/lib/gcc/
export WSLENV := $(if $(WSLENV),$(WSLENV):,)GCC_EXEC_PREFIX
endif
else
CC      := arm-none-eabi-gcc
CXX     := arm-none-eabi-g++
OBJCOPY := arm-none-eabi-objcopy
endif

# Python interpreter used by the helper scripts (python3 on Linux / WSL)
ifeq ($(origin PYTHON),undefined)
PYTHON := $(if $(call on_path,python3),python3,python)
endif

# ── Compiler flags ────────────────────────────────────────
CFLAGS=-mcpu=$(MCU) -mthumb -Wall -O2 -g -DSTM32C5xx \
	-IDrivers/compat_inc \
	-IDrivers/CMSIS \
	-I$(PROJECT_DIR)/inc \
	-ffunction-sections -fdata-sections

# DEBUG=1 enables debugger-friendly codegen. This improves breakpoint reliability.
ifeq ($(strip $(DEBUG)),1)
CFLAGS := $(filter-out -O2,$(CFLAGS)) -Og -g3 -fno-inline
endif

LDFLAGS=-T$(PROJECT_DIR)/linker.ld --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections

# ── Module wiring ─────────────────────────────────────────
# Only the selected PROJECT_DIR/module.mk is read as the project. It declares,
# inside an `ifeq ($(MODULE),<Name>)` block, everything specific to building
# and uploading that folder:
#   DEPS        modules (driver/project folder names) linked into this build
#   EXTRA_INCS  extra include-only directories
#   EXTRA_SRC_C extra source files built into this project only
#   FLASH_ADDR  load address when it is not 0x08000000
#   POST_FLASH  commands run after `make flash` (optional)
#   MODULE_HELP non-empty when the module adds targets (shown in `make help`)
#   plus any project-specific targets (bl-upload, seal-active, …) and
#   additions to CFLAGS / LDFLAGS.
# Outside that block every module.mk exports its library interface, prefixed
# with the folder name, which is read for each module listed in DEPS:
#   <Name>_SRC_C   library source(s) linked into modules that depend on it
#   <Name>_INC     library include directory
override PROJECT_DIR := $(patsubst ./%,%,$(patsubst %/,%,$(strip $(PROJECT_DIR))))
MODULE := $(notdir $(PROJECT_DIR))

ifeq ($(wildcard $(PROJECT_DIR)/module.mk),)
$(error PROJECT_DIR=$(PROJECT_DIR) has no module.mk — pick one of: $(patsubst %/module.mk,%,$(wildcard Drivers/*/module.mk Projects/*/module.mk)))
endif
include $(PROJECT_DIR)/module.mk

# nvic.c provides strong USART+EXTI IRQ handler definitions for ALL projects.
# Startup files reference these names in their vector tables, so NVIC is
# always a dependency.
DEPS := $(filter-out $(MODULE),$(DEPS) $(filter-out $(DEPS),NVIC))

# Find the module.mk of a dependency by folder name
module_mk = $(firstword $(wildcard Drivers/$(1)/module.mk Projects/$(1)/module.mk))
_MISSING_DEPS := $(strip $(foreach d,$(DEPS),$(if $(call module_mk,$(d)),,$(d))))
ifneq ($(_MISSING_DEPS),)
$(error $(PROJECT_DIR)/module.mk: unknown DEPS '$(_MISSING_DEPS)' (no Drivers/<Name>/module.mk or Projects/<Name>/module.mk))
endif
include $(foreach d,$(DEPS),$(call module_mk,$(d)))

FLASH_ADDR ?= 0x08000000

# ── Flashing configuration ────────────────────────────────
FLASH_TOOL?=cubeprog  # options: cubeprog | openocd | stlink
# Sanitize FLASH_TOOL in case of stray spaces from environment or edits
_FLASH_TOOL:=$(strip $(FLASH_TOOL))

# STM32_Programmer_CLI: PATH, then the standalone STM32CubeProgrammer install,
# then the copy bundled with STM32CubeIDE.
ifeq ($(origin CUBE_PROG),undefined)
_CUBE_PROG_STD := $(if $(_WSL),/mnt/c,C:)/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe
CUBE_PROG := $(or \
	$(firstword $(call on_path,STM32_Programmer_CLI)), \
	$(if $(call path_exists,$(_CUBE_PROG_STD)),$(_CUBE_PROG_STD)), \
	$(lastword $(sort $(wildcard \
		C:/ST/STM32CubeIDE*/STM32CubeIDE/plugins/*cubeprogrammer*/tools/bin/STM32_Programmer_CLI.exe \
		/mnt/c/ST/STM32CubeIDE*/STM32CubeIDE/plugins/*cubeprogrammer*/tools/bin/STM32_Programmer_CLI.exe \
		/opt/st/stm32cubeide*/plugins/*cubeprogrammer*/tools/bin/STM32_Programmer_CLI))), \
	$(_CUBE_PROG_STD))
endif
# tools/seal.py, clear_tag.py and bl_upload.py read it from the environment
export CUBE_PROG
OPENOCD?=openocd
STFLASH?=st-flash
OPENOCD_SCRIPTS?=
OPENOCD_IF?=interface/stlink-dap.cfg
OPENOCD_TARGET?=target/stm32u5x.cfg
OPENOCD_EXTRA?=-c "transport select dapdirect_swd" -c "set CPUTAPID 0x6ba02477"

# Serial port settings (monitor, bl-upload)
#   PORT          forces a port (e.g. make bl-upload PORT=COM4)
#   DEFAULT_PORT  used when the ST-LINK virtual COM port is not auto-detected.
#                 Set it to your board's port — see `make com-list`, or the
#                 "STMicroelectronics STLink Virtual COM Port" in Device Manager.
PORT ?=
ifneq ($(OS)$(_WSL),)
DEFAULT_PORT ?= COM5
else
DEFAULT_PORT ?= /dev/ttyACM0
endif
BAUD ?= 115200
MONITOR_SECONDS ?=
POWERSHELL := $(if $(_WSL),powershell.exe,powershell)

# C++ flags largely mirror C; disable RTTI/exceptions to keep size small
CXXFLAGS=$(CFLAGS) -fno-exceptions -fno-rtti -fno-use-cxa-atexit

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
SRC_C   += $(filter-out $(SRC_C),$(EXTRA_SRC_C))
CFLAGS  += $(addprefix -I,$(foreach d,$(DEPS),$($(d)_INC)) $(EXTRA_INCS))

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

.PHONY: help all build run clean flash flash_openocd flash_stlink flash_cubeprog check-toolchain

all: build

# Fail early with a clear message instead of "arm-none-eabi-gcc: not found"
check-toolchain:
ifeq ($(_POSIX_SH),1)
	@command -v "$(CC)" >/dev/null 2>&1 || { \
		echo "ERROR: $(CC) not found. Install the Arm GNU toolchain (arm-none-eabi-gcc) and add it to PATH,"; \
		echo "       install STM32CubeIDE, or pass TOOLCHAIN_DIR=<path to the toolchain bin folder>."; \
		exit 1; }
endif
	@echo "=== Building PROJECT_DIR=$(PROJECT_DIR) -> $(TARGET).elf (load address $(FLASH_ADDR))"
	@echo "    project sources : $(wildcard $(PROJECT_DIR)/src/*.c $(PROJECT_DIR)/src/*.cpp)"
	@echo "    library modules : $(DEPS)  (driver code only - their main.c/startup.c are not built)"
	@echo "    library sources : $(strip $(filter-out $(PROJECT_DIR)/%,$(SRC_C)) $(EXTERNAL_SRC_C) $(EXTERNAL_SRC_CPP))"

# Module wiring is resolved above (module.mk files are included before any
# rule), so OBJ is complete when this prerequisite list is read.
$(OBJ): | check-toolchain

build: $(OBJ)
	$(LINKER) $(CFLAGS) $(OBJ) -Wl,-Map=$(TARGET).map -o $(TARGET).elf $(LDFLAGS) $(EXTRA_LDFLAGS)
	$(OBJCOPY) -O ihex $(TARGET).elf $(TARGET).hex
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).bin
	-$(PYTHON) -c "import os; [os.remove(f) for f in '$(OBJ)'.split() if os.path.isfile(f)]"
	@echo "--- Built $(PROJECT_DIR): $(TARGET).elf / .hex / .bin (load address $(FLASH_ADDR)) ---"

run: build

flash: build
ifeq ($(_FLASH_TOOL),cubeprog)
	"$(CUBE_PROG)" -c port=SWD -halt -d $(TARGET).bin $(FLASH_ADDR) -rst
else ifeq ($(_FLASH_TOOL),openocd)
	"$(OPENOCD)" $(if $(OPENOCD_SCRIPTS),-s "$(OPENOCD_SCRIPTS)") -f $(OPENOCD_IF) $(OPENOCD_EXTRA) -f $(OPENOCD_TARGET) -c "program $(TARGET).elf verify reset exit"
else ifeq ($(_FLASH_TOOL),stlink)
	"$(STFLASH)" --reset write $(TARGET).bin $(FLASH_ADDR)
else
	@echo Unknown FLASH_TOOL=$(_FLASH_TOOL). Use cubeprog, openocd, or stlink.
	@exit 1
endif
	$(POST_FLASH)

flash_cubeprog: FLASH_TOOL=cubeprog
flash_cubeprog: build
	"$(CUBE_PROG)" -c port=SWD -halt -d $(TARGET).bin $(FLASH_ADDR) -rst
	$(POST_FLASH)

flash_openocd: FLASH_TOOL=openocd
flash_openocd: build
	"$(OPENOCD)" $(if $(OPENOCD_SCRIPTS),-s "$(OPENOCD_SCRIPTS)") -f $(OPENOCD_IF) $(OPENOCD_EXTRA) -f $(OPENOCD_TARGET) -c "program $(TARGET).elf verify reset exit"
	$(POST_FLASH)

flash_stlink: FLASH_TOOL=stlink
flash_stlink: build
	"$(STFLASH)" --reset write $(TARGET).bin $(FLASH_ADDR)
	$(POST_FLASH)

# ── Upload through Projects/Bootloader (UART, ST-LINK VCP) ───────────────
# Works with any PROJECT_DIR; Projects/Bootloader must already be on the board
# (make flash PROJECT_DIR=Projects/Bootloader).
#   make bl-upload                      build + upload BL_APP_DIR
#   make bl-upload APP=<file.bin>       upload a ready binary (linked at 0x08010000)
# BL_APP_DIR is the selected project when it runs in the active slot
# (FLASH_ADDR 0x08010000), otherwise Projects/App_Demo.
# Link: LINK=auto (default: USB CDC when the bootloader's USB-C port is
#       present, else UART), LINK=usb or LINK=uart.
# Port: PORT if given, else auto-detected (USB CDC CAFE:B0C5 / ST-LINK VCP),
#       else DEFAULT_PORT (UART).
# WSL : USBIPD=1 when the board's USB devices are attached to WSL with usbipd.
BL_APP_DIR ?= $(if $(filter 0x08010000,$(FLASH_ADDR)),$(PROJECT_DIR),Projects/App_Demo)
APP ?=
LINK ?= auto
USBIPD ?= 0
.PHONY: bl-upload
bl-upload:
ifeq ($(strip $(APP)),)
	@$(MAKE) --no-print-directory build PROJECT_DIR=$(BL_APP_DIR)
endif
	$(PYTHON) tools/bl_upload.py $(or $(APP),$(BL_APP_DIR)/main.bin) --link $(LINK) $(if $(PORT),--port $(PORT),--fallback-port $(DEFAULT_PORT)) $(if $(filter 1,$(USBIPD)),--wsl-usbipd) --reset

# Open a simple serial monitor with PowerShell (Windows or WSL)
.PHONY: monitor
monitor:
ifneq ($(OS)$(_WSL),)
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File "tools/monitor.ps1" $(if $(PORT),-ComPort $(PORT),) -Baud $(BAUD) $(if $(MONITOR_SECONDS),-DurationSec $(MONITOR_SECONDS),)
else
	@echo "This monitor target needs Windows PowerShell (Windows or WSL)."
endif

.PHONY: com-list
com-list:
ifneq ($(OS)$(_WSL),)
	$(POWERSHELL) -NoProfile -Command "Get-CimInstance Win32_SerialPort | Select-Object Name,DeviceID | Format-Table -AutoSize"
else
	@echo "Use: ls /dev/tty*"
endif

# Build, flash, then start monitor automatically
.PHONY: flashmonitor-auto
flashmonitor-auto: all flash monitor

clean:
	-$(PYTHON) -c "import os; [os.remove(f) for f in ('$(OBJ) $(TARGET).elf $(TARGET).hex $(TARGET).bin $(TARGET).map').split() if os.path.isfile(f)]"

# ── Git submodules: Drivers/TinyUSB, Drivers/stm32c5xx-dfp, Drivers/CMSIS_6 ──
# submodules         fetch the versions pinned by this repository (after git clone)
# submodules-update  move each one to the newest commit of its branch (.gitmodules);
#                    commit the result to pin the new versions for everyone
.PHONY: submodules submodules-update
submodules:
	git submodule update --init --depth 1
submodules-update:
	git submodule update --init --remote --depth 1
	git submodule status

# Print any Makefile variable, e.g. make print-PROJECT_DIR
.PHONY: print-%
print-%:
	@echo $($*)

# Every folder with a module.mk can be built as PROJECT_DIR
MODULE_DIRS := $(patsubst %/module.mk,%,$(wildcard Drivers/*/module.mk Projects/*/module.mk))

define _NEWLINE


endef

# Help target: list common make targets and usage
help:
	@echo "Usage: make [target] [PROJECT_DIR=<dir>] [VAR=value ...]"
	@echo "Current PROJECT_DIR: $(PROJECT_DIR)  (load address $(FLASH_ADDR))"
	@echo "Linked modules:      $(DEPS)"
	@echo
	@echo "Build:"
	@echo "  build             Build the current PROJECT_DIR (same as 'make all' / 'make run')"
	@echo "  clean             Remove build artifacts of the current PROJECT_DIR"
	@echo "  debug-build       Clean + build with DEBUG=1, copy ELF/BIN to build_out/"
	@echo "Flash over SWD (ST-LINK cable):"
	@echo "  flash             Build and flash PROJECT_DIR with FLASH_TOOL (cubeprog/openocd/stlink)"
	@echo "  flash_cubeprog    Flash with STM32_Programmer_CLI"
	@echo "  flash_openocd     Flash with openocd"
	@echo "  flash_stlink      Flash with st-flash"
	@echo "  debug             debug-build, then flash build_out/main.bin for VS Code debugging"
	@echo
	@echo "Upload through Projects/Bootloader (flash it once first: make flash PROJECT_DIR=Projects/Bootloader):"
	@echo "  bl-upload         Build + upload $(or $(APP),$(BL_APP_DIR)) to the active slot 0x08010000"
	@echo "      LINK=auto     USB CDC if the bootloader's USB port is present, else UART (default)"
	@echo "      LINK=usb      USB CDC, USB-C user connector (VID:PID CAFE:B0C5)"
	@echo "      LINK=uart     USART2 through the ST-LINK virtual COM port"
	@echo "      APP=<bin>     upload a ready binary instead of building one"
	@echo "      PORT=<port>   force a port (COM4, /dev/ttyACM0); DEFAULT_PORT=$(DEFAULT_PORT) is the UART fallback"
	@echo "      USBIPD=1      WSL2 with the board's USB devices attached through usbipd"
	@echo "    e.g. make bl-upload LINK=usb"
	@echo "         make bl-upload LINK=uart PORT=COM4 APP=Projects/Root_of_Trust/main.bin"
	@echo "  Details: Projects/Bootloader/README.md"
	@echo
	@echo "Active-slot apps (PROJECT_DIR = Projects/App_Demo | Root_of_Trust | Secure_FW_Update):"
	@echo "  seal-active       Build, flash over SWD + CRC-seal the active slot (tools/seal.py)"
	@echo "  restore-bootloader Re-flash Projects/Secure_Boot at 0x08000000"
ifneq ($(strip $(MODULE_HELP)),)
	@echo "  -> available now ($(PROJECT_DIR) is selected)"
else
	@echo "  -> not available with PROJECT_DIR=$(PROJECT_DIR)"
endif
	@echo
	@echo "Serial / misc:"
	@echo "  monitor           Open serial monitor (Windows PowerShell, also from WSL)"
	@echo "                    Auto-detects USB-serial bridge when PORT is empty"
	@echo "                    Use MONITOR_SECONDS=N for timed CI verification"
	@echo "  com-list          List serial ports (Windows PowerShell, also from WSL)"
	@echo "  flashmonitor-auto Build, flash, and start monitor"
	@echo "  gen-debug-config  Write board settings into .vscode/settings.json"
	@echo "  print-VAR         Print a Makefile variable, e.g. make print-SRC_C"
	@echo "  submodules        Fetch TinyUSB / stm32c5xx-dfp / CMSIS_6 (git submodules)"
	@echo "  submodules-update Update them to the newest upstream commits"
	@echo
	@echo "Variables: PROJECT_DIR, FLASH_TOOL=$(_FLASH_TOOL), DEBUG=1, PORT, BAUD=$(BAUD), DRIVERS, TOOLCHAIN_DIR"
	@echo "Compiler:  $(CC)"
	@echo
	@echo "Available PROJECT_DIR values:"
	$(foreach d,$(MODULE_DIRS),@echo "  $(d)"$(_NEWLINE))
	@echo
	@echo "To build immediately, run: make build"

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
	$(PYTHON) -c "import os; os.makedirs('$(BUILD_OUT)', exist_ok=True)"
	$(PYTHON) -c "import shutil; shutil.copy('$(TARGET).elf', '$(BUILD_OUT)/main.elf'); shutil.copy('$(TARGET).bin', '$(BUILD_OUT)/main.bin')"
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
	$(PYTHON) -c "\
import json, sys; \
path = '.vscode/settings.json'; \
s = json.load(open(path)) if __import__('os').path.isfile(path) else {}; \
s.update({'stm32.board': '$(DEBUG_DEVICE and BOARD or BOARD)', 'stm32.device': '$(DEBUG_DEVICE)', 'stm32.svdFile': '$(DEBUG_SVD)', 'stm32.openocdCfg': '$(DEBUG_CFG)', 'stm32.cpuTapId': '$(DEBUG_TAPID)'}); \
json.dump(s, open(path, 'w'), indent=2); \
print('--- .vscode/settings.json updated for board: $(BOARD) ---')"
