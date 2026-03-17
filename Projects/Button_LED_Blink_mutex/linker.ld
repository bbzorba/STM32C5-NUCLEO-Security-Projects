ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
  .isr_vector :
  {
    KEEP(*(.isr_vector))
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text*)
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)
    KEEP(*(.init))
    KEEP(*(.fini))
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    KEEP(*(.eh_frame))
    __etext = .;
  } >FLASH

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
    . = ALIGN(4);
    __bss_end__ = .;
    end = .;
    _end = .;
  } >RAM

  .stack :
  {
    . = ALIGN(8);
    _stack = .;
    . += 0x400;
  } >RAM
}
