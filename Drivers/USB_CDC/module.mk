# ── USB_CDC — USB CDC ACM virtual COM port on the USB-C user connector ────
# Bare-metal TinyUSB (submodule Drivers/TinyUSB) on the USB_DRD_FS device:
# PA11/PA12 → NUCLEO-C562RE USB Type-C user connector, no wiring needed.
# TinyUSB's ST port needs the CMSIS device pack (Drivers/stm32c5xx-dfp) and
# the Arm CMSIS core headers (Drivers/CMSIS_6) — all three are submodules:
#   git submodule update --init --depth 1
ifeq ($(wildcard Drivers/TinyUSB/src/tusb.c Drivers/stm32c5xx-dfp/Include/stm32c5xx.h Drivers/CMSIS_6/CMSIS/Core/Include/core_cm33.h),)
$(error USB_CDC: submodules missing — run: git submodule update --init --depth 1)
endif

TUSB_SRC := Drivers/TinyUSB/src
USB_CDC_TUSB_SRC_C := \
	$(TUSB_SRC)/tusb.c \
	$(TUSB_SRC)/common/tusb_fifo.c \
	$(TUSB_SRC)/device/usbd.c \
	$(TUSB_SRC)/class/cdc/cdc_device.c \
	$(TUSB_SRC)/portable/st/stm32_fsdev/dcd_stm32_fsdev.c \
	$(TUSB_SRC)/portable/st/stm32_fsdev/fsdev_common.c

# Library interface: linked into any module that lists USB_CDC in its DEPS.
# Only usb_cdc.h is public — it has no ST/CMSIS includes.
USB_CDC_SRC_C := Drivers/USB_CDC/src/usb_cdc.c Drivers/USB_CDC/src/usb_descriptors.c $(USB_CDC_TUSB_SRC_C)
USB_CDC_INC   := Drivers/USB_CDC/inc

# The CMSIS device header defines RCC, GPIOA, FLASH … which clash with this
# repository's register-level headers, so its include paths are given only
# to the USB_CDC / TinyUSB objects (target-specific variable).
USB_CDC_PRIVATE_CFLAGS := -DSTM32C562xx \
	-IDrivers/USB_CDC/inc \
	-I$(TUSB_SRC) \
	-IDrivers/stm32c5xx-dfp/Include \
	-IDrivers/CMSIS_6/CMSIS/Core/Include
$(patsubst %.c,%.o,$(USB_CDC_SRC_C)): CFLAGS += $(USB_CDC_PRIVATE_CFLAGS)

# Standalone build (PROJECT_DIR = Drivers/USB_CDC): echo demo, log on USART2
ifeq ($(MODULE),USB_CDC)
DEPS        := GPIO UART
EXTRA_SRC_C := $(USB_CDC_TUSB_SRC_C)
endif
