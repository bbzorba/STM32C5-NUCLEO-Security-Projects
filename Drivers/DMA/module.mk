# ── DMA — LPDMA driver ────────────────────────────────────────────────────
# Library interface: linked into any module that lists DMA in its DEPS
DMA_SRC_C := Drivers/DMA/src/dma.c
DMA_INC   := Drivers/DMA/inc

# Standalone build (PROJECT_DIR = Drivers/DMA): needs GPIO and UART drivers
ifeq ($(MODULE),DMA)
DEPS := GPIO UART
endif
