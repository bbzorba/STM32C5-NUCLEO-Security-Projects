/* Output sections shared by every image. The layout script that includes
 * this file defines the FLASH and RAM regions.
 *
 * Symbols used by the startup code: _estack, __etext, __data_start__,
 * __data_end__, __bss_start__, __bss_end__.
 * Stack: __stack_size__ bytes are reserved after .bss so a RAM overflow fails
 * at link time. Default 1 KB; a module sets STACK_SIZE in its module.mk.
 */
ENTRY(Reset_Handler)

PROVIDE(__stack_size__ = 0x400);

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
    . += __stack_size__;
  } >RAM
}
