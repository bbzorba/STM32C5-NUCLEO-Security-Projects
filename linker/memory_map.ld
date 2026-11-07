/* STM32C562RE memory map — the one place the addresses are defined.
 * Included by the layout scripts (standalone.ld, bootloader.ld, app_slot.ld).
 * Must match Projects/Bootloader/inc/bootloader.h, Projects/Secure_Boot and
 * tools/seal.py.  Flash: 512 KB, single bank, 8 KB sectors.
 */
__flash_base = 0x08000000;
__flash_size = 512K;

__boot_base  = 0x08000000;     /* sectors 0-7    Bootloader / Secure_Boot            */
__boot_size  = 64K;

__slot_base  = 0x08010000;     /* sectors 8-29   active slot: apps started by them   */
__slot_size  = 176K;

__slot_tag   = 0x0803C000;     /* sector 30      CRC-32 tag of the active slot       */

__ram_base   = 0x20000000;
__ram_size   = 128K;
