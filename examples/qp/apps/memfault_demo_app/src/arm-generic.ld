STACK_SIZE = 8192;
HEAP_SIZE = 0;
ROM_SIZE = 1024K;
RAM_SIZE = 64K;

MEMORY {
    ROM (rx)  : ORIGIN = 0x08000000, LENGTH = ROM_SIZE
    RAM (xrw) : ORIGIN = 0x20000000, LENGTH = RAM_SIZE
}

PROVIDE(__sram_start = ORIGIN(RAM));
PROVIDE(__sram_end = ORIGIN(RAM) + LENGTH(RAM));

SECTIONS {
    .isr_vector : {
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >ROM

    .text : {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)

        KEEP (*(.init))
        KEEP (*(.fini))

        . = ALIGN(4);
    } >ROM

    .gnu_build_id :
    {
      PROVIDE(g_gnu_build_id = .);
      *(.note.gnu.build-id)
    } > ROM

    _etext = .;

    .stack : {
        __stack_start__ = .;
        . = . + STACK_SIZE;
        . = ALIGN(4);
        __stack_end__ = .;
    } >RAM

    /* See memfault_platform_ram_backed_coredump.c */
    .coredump_noinit (NOLOAD) : {
        KEEP(*(*.mflt_coredump))
    } >RAM

    .data :  AT (_etext) {
        __data_load = LOADADDR (.data);
        __data_start = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        __data_end__ = .;
        _edata = __data_end__;
    } >RAM

    .bss : {
        __bss_start__ = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = .;
    } >RAM

    __exidx_start = .;
    .ARM.exidx   : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } >RAM
    __exidx_end = .;

    PROVIDE ( end = _ebss );
    PROVIDE ( _end = _ebss );
    PROVIDE ( __end__ = _ebss );

    .heap : {
        __heap_start__ = .;
        . = . + HEAP_SIZE;
        . = ALIGN(4);
        __heap_end__ = .;
    } >RAM
}
