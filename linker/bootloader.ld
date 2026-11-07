/* Boot region: sectors 0-7 (64 KB) from 0x08000000 (LAYOUT = bootloader).
 * Projects/Bootloader and Projects/Secure_Boot. The link fails if the image
 * would grow into the active slot at 0x08010000.
 */
INCLUDE memory_map.ld

MEMORY
{
  FLASH (rx)  : ORIGIN = __boot_base, LENGTH = __boot_size
  RAM   (xrw) : ORIGIN = __ram_base,  LENGTH = __ram_size
}

INCLUDE sections.ld
