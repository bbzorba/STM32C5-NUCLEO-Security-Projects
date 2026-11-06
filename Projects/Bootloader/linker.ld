/* Linker script for Bootloader — sectors 0-7 only (64 KB @ 0x08000000).
 * Must never grow into the active slot at 0x08010000.
 */
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K   /* sectors 0–7 only */
    RAM   (rw) : ORIGIN = 0x20000000, LENGTH = 128K
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
    end = .;
    __end__ = .;
  } >RAM

  .stack :
  {
    . = ALIGN(8);
    _stack = .;
    . += 0x400;
  } >RAM
}
