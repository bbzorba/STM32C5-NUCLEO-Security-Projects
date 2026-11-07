/* Active slot: sectors 8-29 (176 KB) from 0x08010000 (LAYOUT = app_slot).
 * Applications started by Projects/Bootloader or Projects/Secure_Boot. The boot
 * code sets VTOR = 0x08010000 before jumping, so the vector table must be the
 * first thing in this region (sections.ld puts .isr_vector first).
 */
INCLUDE memory_map.ld

MEMORY
{
  FLASH (rx)  : ORIGIN = __slot_base, LENGTH = __slot_size
  RAM   (xrw) : ORIGIN = __ram_base,  LENGTH = __ram_size
}

INCLUDE sections.ld
