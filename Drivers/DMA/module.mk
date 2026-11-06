# ── DMA — LPDMA driver ────────────────────────────────────────────────────
# Library: linked into any module that lists DMA in its _DEPS
DMA_SRC_C := Drivers/DMA/src/dma.c
DMA_INC   := Drivers/DMA/inc

# Standalone build (PROJECT_DIR = Drivers/DMA): needs GPIO and UART drivers
DMA_DEPS  := GPIO UART
