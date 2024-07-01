MEMORY
{
    FLASH (xr) : ORIGIN = 0x00000000, LENGTH = 4M /* to 0x00003FFF = 0x007FFFFF*/
    RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 4M /* to 0x21FFFFFF = 0xFFFFFF */
}
ENTRY(Reset_Handler)

_Min_Heap_Size = 0x300000 ;        /* Required amount of heap. */
_Min_Stack_Size = 0x4000 ;       /* Required amount of stack. */
M_VECTOR_RAM_SIZE = (16 + 48) * 4;
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{

    .isr_vector :
    {
        __vector_table = .;
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text*)
        KEEP (*(.init))
        KEEP (*(.fini))
        KEEP(*(.eh_frame))
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .note.gnu.build-id :
    {
        __start_gnu_build_id_start = .;
        KEEP(*(.note.gnu.build-id))
    } > FLASH

    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } >FLASH

    .ARM :
    {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
        . = ALIGN(4);
    } >FLASH

    .interrupts_ram :
    {
        . = ALIGN(4);
        __VECTOR_RAM__ = .;
        __interrupts_ram_start__ = .;
        . += M_VECTOR_RAM_SIZE;
        . = ALIGN(4);
        __interrupts_ram_end = .;
    } > RAM

    _sidata = LOADADDR(.data);

    .data : /* AT ( _sidata ) */
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .uninitialized (NOLOAD):
    {
        . = ALIGN(32);
        __uninitialized_start = .;
        *(.uninitialized)
        KEEP(*(.keep.uninitialized))
        . = ALIGN(32);
        __uninitialized_end = .;
    } > RAM

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        __memfault_capture_bss_start = .;
        *tasks*.o(.bss COMMON .bss*)
        *timers*.o(.bss COMMON .bss*)
        __memfault_capture_bss_end = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } >RAM

    .heap :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        _heap_bottom = .;
        . = . + _Min_Heap_Size;
        _heap_top = .;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
       } >RAM

   /* Set stack top to end of RAM, and stack limit move down by
    * size of stack_dummy section */
   __StackTop = ORIGIN(RAM) + LENGTH(RAM);
   __StackLimit = __StackTop - _Min_Stack_Size;
   PROVIDE(__stack = __StackTop);

    log_fmt 0xF0000000 (INFO):
    {
       KEEP(*(*.log_fmt_hdr))
       KEEP(*(log_fmt))
    }

  /* Check if data + heap + stack exceeds RAM limit */
  ASSERT(__StackLimit >= _heap_top, "region RAM overflowed with stack")
}
