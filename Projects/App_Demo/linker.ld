/* Linker script for App_Demo — runs in the Secure Boot ACTIVE slot.
 * ORIGIN must match BOOT_ACTIVE_ADDR (0x08010000).
 * LENGTH must match BOOT_ACTIVE_SIZE (22 sectors × 8 KB = 176 KB).
 * The bootloader sets VTOR = 0x08010000 before jumping, so the
 * vector table must be placed at the very start of this region.
 */
ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08010000, LENGTH = 176K   /* sectors 8-29 */
  RAM   (xrw): ORIGIN = 0x20000000, LENGTH = 128K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector :
  {
    KEEP(*(.isr_vector))
  } >FLASH

  .text :
  {
    *(.text*)
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)
    *(.init)
    *(.fini)
  } >FLASH

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
  .ARM.exidx : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } > FLASH

  __etext = .;

  .data : AT (__etext)
  {
    __data_start__ = .;
    *(.data*)
    __data_end__ = .;
  } >RAM

  .bss :
  {
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    __bss_end__ = .;
    _end = .;
    end  = .;
  } >RAM
}
