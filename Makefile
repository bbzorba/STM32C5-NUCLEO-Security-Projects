# Makefile for STM32 NUCLEO C562RE development board

#DONE
#PROJECT_DIR = Drivers/UART
#PROJECT_DIR = Drivers/GPIO
#PROJECT_DIR = Drivers/bxCAN
#PROJECT_DIR = Drivers/AES
PROJECT_DIR = Drivers/RSA

#TBD
#PROJECT_DIR = Projects/Memory_Protection
#PROJECT_DIR = Projects/Secure_Boot
#PROJECT_DIR = Projects/Secure_Firmware_Update
#PROJECT_DIR = Projects/TrustZone
#PROJECT_DIR = Projects/Crypto
#PROJECT_DIR = Drivers/bxCAN_cpp

CXX=arm-none-eabi-g++
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
MCU := cortex-m33
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

# Flashing configuration
FLASH_ADDR=0x08000000
FLASH_TOOL?=cubeprog  # options: cubeprog | openocd | stlink
# Sanitize FLASH_TOOL in case of stray spaces from environment or edits
_FLASH_TOOL:=$(strip $(FLASH_TOOL))
CUBE_PROG?=C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe
OPENOCD?=openocd
STFLASH?=st-flash
OPENOCD_SCRIPTS?=
OPENOCD_IF?=interface/stlink.cfg
OPENOCD_TARGET?=target/stm32f4x.cfg
OPENOCD_EXTRA?=

CFLAGS=-mcpu=$(MCU) -mthumb -Wall -O2 -g -DSTM32F407xx -DUSE_HAL_DRIVER \
	-IDrivers/compat_inc \
	-IDrivers/CMSIS \
	-IDrivers/STM32F4xx_HAL_Driver \
	-IDrivers/STM32F4xx_HAL_Driver/Legacy \
	-I$(PROJECT_DIR)/inc \
	-ffunction-sections -fdata-sections

# DEBUG=1 enables debugger-friendly codegen. This improves breakpoint reliability.
ifeq ($(strip $(DEBUG)),1)
CFLAGS := $(filter-out -O2,$(CFLAGS)) -Og -g3 -fno-inline
endif

# UART driver has a dedicated STM32C5 register-level port.
ifeq ($(strip $(PROJECT_DIR)),Drivers/UART)
MCU := cortex-m33
CFLAGS := $(filter-out -DSTM32F407xx -DUSE_HAL_DRIVER,$(CFLAGS)) -DSTM32C5xx
OPENOCD_IF := interface/stlink-dap.cfg
OPENOCD_TARGET := target/stm32u5x.cfg
OPENOCD_EXTRA := -c "transport select dapdirect_swd" -c "set CPUTAPID 0x6ba02477"
endif

# GPIO driver has a dedicated STM32C5 register-level port.
ifeq ($(strip $(PROJECT_DIR)),Drivers/GPIO)
MCU := cortex-m33
CFLAGS := $(filter-out -DSTM32F407xx -DUSE_HAL_DRIVER,$(CFLAGS)) -DSTM32C5xx
OPENOCD_IF := interface/stlink-dap.cfg
OPENOCD_TARGET := target/stm32u5x.cfg
OPENOCD_EXTRA := -c "transport select dapdirect_swd" -c "set CPUTAPID 0x6ba02477"
endif

# bxCAN (FDCAN) driver has a dedicated STM32C5 register-level port.
ifeq ($(strip $(PROJECT_DIR)),Drivers/bxCAN)
MCU := cortex-m33
CFLAGS := $(filter-out -DSTM32F407xx -DUSE_HAL_DRIVER,$(CFLAGS)) -DSTM32C5xx
OPENOCD_IF := interface/stlink-dap.cfg
OPENOCD_TARGET := target/stm32u5x.cfg
OPENOCD_EXTRA := -c "transport select dapdirect_swd" -c "set CPUTAPID 0x6ba02477"
endif

# AES driver has a dedicated STM32C5 register-level port.
ifeq ($(strip $(PROJECT_DIR)),Drivers/AES)
MCU := cortex-m33
CFLAGS := $(filter-out -DSTM32F407xx -DUSE_HAL_DRIVER,$(CFLAGS)) -DSTM32C5xx
OPENOCD_IF := interface/stlink-dap.cfg
OPENOCD_TARGET := target/stm32u5x.cfg
OPENOCD_EXTRA := -c "transport select dapdirect_swd" -c "set CPUTAPID 0x6ba02477"
endif

# C++ flags largely mirror C; disable RTTI/exceptions to keep size small
CXXFLAGS=$(CFLAGS) -fno-exceptions -fno-rtti -fno-use-cxa-atexit

# Serial monitor settings (Windows PowerShell)
PORT ?=
BAUD ?= 115200
MONITOR_SECONDS ?=
LDFLAGS=-T$(PROJECT_DIR)/linker.ld --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections

# Optional external driver selection -------------------------------------------------
# Include UART driver automatically for the LED_Blink project so
# project code can use the shared UART API without adding local stubs.
ifeq ($(strip $(PROJECT_DIR)),Projects/LED_Blink)
DRIVERS := UART
endif
ifeq ($(strip $(PROJECT_DIR)),Drivers/bxCAN)
DRIVERS := UART GPIO
endif
ifeq ($(strip $(PROJECT_DIR)),Projects/LED_Blink_cpp)
DRIVERS := UART_cpp GPIO_cpp
endif
# By default (when DRIVERS is empty) we build ONLY the selected PROJECT sources.
# To pull in driver code under Drivers/<Name>, invoke:
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

# Project sources
SRC_C   := $(wildcard $(PROJECT_DIR)/src/*.c)
SRC_CPP := $(wildcard $(PROJECT_DIR)/src/*.cpp)
SRC := $(SRC_C) $(SRC_CPP)

# If the project uses HAL, add a minimal set of HAL sources
HAL_SRC := \
	Drivers/STM32F4xx_HAL_Driver/stm32f4xx_hal.c \
	Drivers/STM32F4xx_HAL_Driver/stm32f4xx_hal_rcc.c \
	Drivers/STM32F4xx_HAL_Driver/stm32f4xx_hal_gpio.c \
	Drivers/STM32F4xx_HAL_Driver/stm32f4xx_hal_cortex.c \
	Drivers/STM32F4xx_HAL_Driver/stm32f4xx_hal_pcd.c \
	Drivers/STM32F4xx_HAL_Driver/stm32f4xx_ll_usb.c

GPIO_SRC_C := Drivers/GPIO/src/gpio.c
GPIO_SRC_CPP := Drivers/GPIO_cpp/src/gpio.cpp

I2C_SRC_C := Drivers/I2C/src/i2c.c
I2C_SRC_CPP := Drivers/I2C_cpp/src/i2c.cpp

SPI_SRC_C := Drivers/SPI/src/spi.c
SPI_SRC_CPP := Drivers/SPI_cpp/src/spi.cpp

UART_SRC_C := Drivers/UART/src/uart.c
UART_SRC_CPP := Drivers/UART_cpp/src/uart.cpp

PWM_SRC_C := Drivers/PWM/src/pwm.c
PWM_SRC_CPP := Drivers/PWM_cpp/src/pwm.cpp

HC_06_SRC_C := Projects/HC06_Bluetooth/src/hc06.c
HC_06_SRC_CPP := Projects/HC06_Bluetooth_cpp/src/hc06.cpp

Servo_MOTOR_SRC_C := Projects/Servo_Motor/src/servo.c
Servo_MOTOR_SRC_CPP := Projects/Servo_Motor_cpp/src/servo.cpp

MLX90614_SRC_C := Projects/MLX90614_Temp/src/mlx90614_temp.c
MLX90614_SRC_CPP := Projects/MLX90614_Temp_cpp/src/mlx90614_temp.cpp

LED_SRC_C := Projects/LED_Blink/src/led.c
LED_SRC_CPP := Projects/LED_Blink_cpp/src/led.cpp

# BME68x environmental sensor sources
BME68X_SRC_C := Projects/BME68x_Env_Sensor/src/bme68x_env_sensor.c

# Automatically include GPIO library when project includes the following source files
ifneq (,$(filter systick.c hc06.c pwm.c servo.c,$(notdir $(SRC))))
# Important: OBJ is derived from SRC_C/SRC_CPP (not SRC). Only add if not already present.
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
CFLAGS += -IDrivers/GPIO/inc
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
CFLAGS += -IDrivers/UART/inc
SRC_C += $(filter-out $(SRC_C),$(PWM_SRC_C))
CFLAGS += -IDrivers/PWM/inc
endif

# Automatically include GPIO_cpp, UART_cpp, and PWM_cpp libraries when project includes the following source files
ifneq (,$(filter systick.cpp hc06.cpp pwm.cpp servo.cpp,$(notdir $(SRC))))
SRC_CPP += $(filter-out $(SRC_CPP),$(GPIO_SRC_CPP))
CFLAGS += -IDrivers/GPIO_cpp/inc
SRC_CPP += $(filter-out $(SRC_CPP),$(UART_SRC_CPP))
CFLAGS += -IDrivers/UART_cpp/inc
SRC_CPP += $(filter-out $(SRC_CPP),$(PWM_SRC_CPP))
CFLAGS += -IDrivers/PWM_cpp/inc
# Also link the C PWM driver to satisfy free-function usages like Timer_Init/Configure_PWM from C++ code
SRC_C += $(filter-out $(SRC_C),$(PWM_SRC_C))
CFLAGS += -IDrivers/PWM/inc
endif

# Project-specific wiring for C++ servo/controller projects to ensure PWM_cpp is compiled
ifeq ($(PROJECT_DIR),Projects/Servo_Motor_cpp)
SRC_CPP += $(filter-out $(SRC_CPP),$(GPIO_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(UART_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(PWM_SRC_CPP))
SRC_C   += $(filter-out $(SRC_C),$(PWM_SRC_C))
CFLAGS  += -IDrivers/GPIO_cpp/inc -IDrivers/UART_cpp/inc -IDrivers/PWM_cpp/inc -IDrivers/PWM/inc
endif

ifeq ($(PROJECT_DIR),Projects/HC06_Servo_Controller_cpp)
SRC_CPP += $(filter-out $(SRC_CPP),$(GPIO_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(UART_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(PWM_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(HC_06_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(Servo_MOTOR_SRC_CPP))
CFLAGS  += -IDrivers/GPIO_cpp/inc -IDrivers/UART_cpp/inc -IDrivers/PWM_cpp/inc -IDrivers/PWM/inc
endif

# Project-specific wiring for AES: needs GPIO and UART drivers
ifeq ($(PROJECT_DIR),Drivers/AES)
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
CFLAGS += -IDrivers/GPIO/inc -IDrivers/UART/inc
endif

OBJ_UNSORTED=$(SRC_C:.c=.o) $(SRC_CPP:.cpp=.o) $(EXTERNAL_SRC_C:.c=.o) $(EXTERNAL_SRC_CPP:.cpp=.o)
OBJ=$(sort $(OBJ_UNSORTED))
TARGET=$(PROJECT_DIR)/main

# Use C++ linker if there are any C++ sources (project or external driver), else C linker
ifneq (,$(strip $(SRC_CPP) $(EXTERNAL_SRC_CPP)))
LINKER := $(CXX)
else
LINKER := $(CC)
endif
TARGET=$(PROJECT_DIR)/main

# Pattern rules
%.o: %.s
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: all build run clean flash flash_openocd flash_stlink flash_cubeprog
.PHONY: help all build run clean flash flash_openocd flash_stlink flash_cubeprog

all: build

build: $(OBJ)
	$(LINKER) $(CFLAGS) $(OBJ) -Wl,-Map=$(TARGET).map -o $(TARGET).elf $(LDFLAGS) $(EXTRA_LDFLAGS)
	$(OBJCOPY) -O ihex $(TARGET).elf $(TARGET).hex
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).bin
ifeq ($(_POSIX_SH),1)
	-$(RM) $(OBJ) 2>/dev/null || true
else
	-$(RM) $(subst /,\,$(OBJ)) 2>nul || exit 0
endif

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

flash_cubeprog: FLASH_TOOL=cubeprog
flash_cubeprog: build
	"$(CUBE_PROG)" -c port=SWD -halt -d $(TARGET).bin $(FLASH_ADDR) -rst

flash_openocd: FLASH_TOOL=openocd
flash_openocd: build
	"$(OPENOCD)" $(if $(OPENOCD_SCRIPTS),-s "$(OPENOCD_SCRIPTS)") -f $(OPENOCD_IF) $(OPENOCD_EXTRA) -f $(OPENOCD_TARGET) -c "program $(TARGET).elf verify reset exit"


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

flash_stlink: FLASH_TOOL=stlink
flash_stlink: build
	"$(STFLASH)" --reset write $(TARGET).bin $(FLASH_ADDR)

clean:
ifeq ($(_POSIX_SH),1)
	-$(RM) $(OBJ) 2>/dev/null || true
	-$(RM) $(TARGET).elf $(TARGET).hex $(TARGET).bin $(TARGET).map 2>/dev/null || true
else
	-$(RM) $(subst /,\,$(OBJ)) 2>nul || exit 0
	-$(RM) $(subst /,\,$(TARGET)).elf 2>nul || exit 0
	-$(RM) $(subst /,\,$(TARGET)).hex 2>nul || exit 0
	-$(RM) $(subst /,\,$(TARGET)).bin 2>nul || exit 0
	-$(RM) $(subst /,\,$(TARGET)).map 2>nul || exit 0
endif

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
# End of Makefile

# Project-specific wiring for HC06_Servo_Controller: needs GPIO, UART, PWM, Servo, HC06
ifeq ($(PROJECT_DIR),Projects/HC06_Servo_Controller)
# Include required drivers and servo/hc06 helpers
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(PWM_SRC_C))
SRC_C += $(filter-out $(SRC_C),Projects/Servo_Motor/src/servo.c)
SRC_C += $(filter-out $(SRC_C),Projects/HC06_Bluetooth/src/hc06.c)
CFLAGS += -IDrivers/GPIO/inc -IDrivers/UART/inc -IDrivers/PWM/inc -IProjects/Servo_Motor/inc -IProjects/HC06_Bluetooth/inc
endif

# Project-specific wiring for MLX90614_Temp: needs I2C driver
ifeq ($(PROJECT_DIR),Projects/MLX90614_Temp)
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(I2C_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
CFLAGS += -IDrivers/GPIO/inc -IDrivers/I2C/inc -IDrivers/UART/inc
endif

# Project-specific wiring for HC06_MLX90614_Temp: needs MLX90614, HC06, GPIO, I2C
ifeq ($(PROJECT_DIR),Projects/HC06_MLX90614_Temp)
SRC_C += $(filter-out $(SRC_C),$(MLX90614_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(HC_06_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(I2C_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
CFLAGS += -IDrivers/GPIO/inc -IDrivers/I2C/inc -IDrivers/UART/inc -IProjects/MLX90614_Temp/inc -IProjects/HC06_Bluetooth/inc
endif

# Project-specific wiring for BME68x: needs GPIO, I2C, UART drivers
ifeq ($(PROJECT_DIR),Projects/BME68x_Env_Sensor)
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(I2C_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
CFLAGS += -IDrivers/GPIO/inc -IDrivers/I2C/inc -IDrivers/UART/inc -IProjects/BME68x_Env_Sensor/inc
endif

# Project-specific wiring for HC06_BME68x_Env_Sensor: needs BME68x, HC06, GPIO, I2C
ifeq ($(PROJECT_DIR),Projects/HC06_BME68x_Env_Sensor)
SRC_C += $(filter-out $(SRC_C),$(BME68X_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(HC_06_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(I2C_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
CFLAGS += -IDrivers/GPIO/inc -IDrivers/I2C/inc -IDrivers/UART/inc -IProjects/BME68x_Env_Sensor/inc -IProjects/HC06_Bluetooth/inc
endif

# Project-specific wiring for PWM: needs GPIO & UART drivers
ifeq ($(PROJECT_DIR),Drivers/PWM)
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
CFLAGS += -IDrivers/GPIO/inc -IDrivers/UART/inc
endif

# Project-specific wiring for SPI: needs GPIO driver
ifeq ($(PROJECT_DIR),Drivers/SPI)
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
CFLAGS += -IDrivers/GPIO/inc
endif

# Project-specific wiring for SPI_cpp: needs GPIO_cpp driver
ifeq ($(PROJECT_DIR),Drivers/SPI_cpp)
SRC_CPP += $(filter-out $(SRC_CPP),$(GPIO_SRC_CPP))
CFLAGS  += -IDrivers/GPIO_cpp/inc
endif

# Project-specific wiring for TSL2591_Light: needs I2C & UART drivers
ifeq ($(PROJECT_DIR),Projects/TSL2591_Light)
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
CLEAR_UNUSED :=
SRC_C += $(filter-out $(SRC_C),$(I2C_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
CFLAGS += -IDrivers/I2C/inc -IDrivers/UART/inc
endif

# Project-specific wiring for bxCAN: needs GPIO & UART drivers
ifeq ($(PROJECT_DIR),Drivers/bxCAN)
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
CFLAGS += -IDrivers/GPIO/inc -IDrivers/UART/inc
endif

# Project-specific wiring for LIS302DL_Accelerometer: needs GPIO, SPI & UART drivers
ifeq ($(PROJECT_DIR),Projects/LIS302DL_Accelerometer)
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(SPI_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
CFLAGS += -IDrivers/GPIO/inc -IDrivers/SPI/inc -IDrivers/UART/inc
endif

# Project-specific wiring for Button_LED_Blink_semaphore: same deps as mutex variant
ifeq ($(PROJECT_DIR),Projects/Button_LED_Blink_semaphore)
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(SPI_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(LED_SRC_C))
CFLAGS += -IDrivers/GPIO/inc -IDrivers/SPI/inc -IDrivers/UART/inc -IProjects/LED_Blink/inc
endif

# Project-specific wiring for Button_LED_Blink_semaphore_cpp: same deps as mutex variant
ifeq ($(PROJECT_DIR),Projects/Button_LED_Blink_semaphore_cpp)
SRC_CPP += $(filter-out $(SRC_CPP),$(GPIO_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(SPI_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(UART_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(LED_SRC_CPP))
CFLAGS += -IDrivers/GPIO_cpp/inc -IDrivers/SPI_cpp/inc -IDrivers/UART_cpp/inc -IProjects/LED_Blink_cpp/inc
endif

# Project-specific wiring for Button_LED_Blink_mutex: needs LED_Blink, GPIO, SPI & UART drivers
ifeq ($(PROJECT_DIR),Projects/Button_LED_Blink_mutex)
SRC_C += $(filter-out $(SRC_C),$(GPIO_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(SPI_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(UART_SRC_C))
SRC_C += $(filter-out $(SRC_C),$(LED_SRC_C))
CFLAGS += -IDrivers/GPIO/inc -IDrivers/SPI/inc -IDrivers/UART/inc -IProjects/LED_Blink/inc
endif

# Project-specific wiring for Button_LED_Blink_mutex_cpp: needs LED_Blink, GPIO, SPI & UART drivers
ifeq ($(PROJECT_DIR),Projects/Button_LED_Blink_mutex_cpp)
SRC_CPP += $(filter-out $(SRC_CPP),$(GPIO_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(SPI_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(UART_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(LED_SRC_CPP))
CFLAGS += -IDrivers/GPIO_cpp/inc -IDrivers/SPI_cpp/inc -IDrivers/UART_cpp/inc -IProjects/LED_Blink_cpp/inc
endif

# Project-specific wiring for Button_LED_Blink_semaphore_cpp: needs LED_Blink, GPIO, SPI & UART drivers
ifeq ($(PROJECT_DIR),Projects/Button_LED_Blink_semaphore_cpp)
SRC_CPP += $(filter-out $(SRC_CPP),$(GPIO_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(SPI_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(UART_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(LED_SRC_CPP))
CFLAGS += -IDrivers/GPIO_cpp/inc -IDrivers/SPI_cpp/inc -IDrivers/UART_cpp/inc -IProjects/LED_Blink_cpp/inc
endif

# Project-specific wiring for LIS302DL_Accelerometer_cpp: needs GPIO_cpp, SPI_cpp & UART_cpp drivers
ifeq ($(PROJECT_DIR),Projects/LIS302DL_Accelerometer_cpp)
SRC_CPP += $(filter-out $(SRC_CPP),$(GPIO_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(SPI_SRC_CPP))
SRC_CPP += $(filter-out $(SRC_CPP),$(UART_SRC_CPP))
CFLAGS  += -IDrivers/GPIO_cpp/inc -IDrivers/SPI_cpp/inc -IDrivers/UART_cpp/inc
endif

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
ifeq ($(_POSIX_SH),1)
	@mkdir -p "$(BUILD_OUT)"
	@cp -f "$(TARGET).elf" "$(BUILD_OUT)/main.elf"
	@cp -f "$(TARGET).bin" "$(BUILD_OUT)/main.bin"
else
	@if not exist "$(BUILD_OUT)" mkdir "$(BUILD_OUT)"
	@copy /Y "$(subst /,\,$(TARGET)).elf" "$(BUILD_OUT)\main.elf" >nul
	@copy /Y "$(subst /,\,$(TARGET)).bin" "$(BUILD_OUT)\main.bin" >nul
endif
	@echo --- Debug build ready: $(BUILD_OUT)/main.elf ---

# debug: standalone build + flash (without VS Code debugger)
debug: debug-build
	"$(CUBE_PROG)" -c port=SWD reset=HWrst -w "$(BUILD_OUT)/main.bin" $(FLASH_ADDR) -v -hardRst
	@echo ""
	@echo "--- GCC build + flash complete: $(BUILD_OUT)/main.elf ---"
	@echo "Press F5 in VS Code [select Cortex Debug: ST-LINK] to start debugging."