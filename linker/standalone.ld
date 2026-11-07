/* Standalone image: the whole flash from 0x08000000, no bootloader.
 * Driver demos and projects that start straight from reset (LAYOUT = standalone).
 */
INCLUDE memory_map.ld

MEMORY
{
  FLASH (rx)  : ORIGIN = __flash_base, LENGTH = __flash_size
  RAM   (xrw) : ORIGIN = __ram_base,   LENGTH = __ram_size
}

INCLUDE sections.ld
