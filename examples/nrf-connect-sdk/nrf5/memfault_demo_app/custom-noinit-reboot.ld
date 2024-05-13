SECTION_PROLOGUE(.memfaultnoinit, (NOLOAD),)
{
    _memfaultnoinit_start = .;
    KEEP(*(*.mflt_reboot_info));
    _memfaultnoinit_end = .;
} > MEMFAULTNOINIT
_memfaultnoinit_size = _memfaultnoinit_end - _memfaultnoinit_start;
