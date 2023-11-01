/*
  Custom section to be included in the linker command file to support compact
  logs. Read more at https://mflt.io/compact-logs
*/

SECTION_PROLOGUE(log_fmt,0xF0000000 (INFO),)
{
    /*
    explicitly define this symbol, in case the linker doesn't auto-define it
    */
    __start_log_fmt = ABSOLUTE(.);
    KEEP(*(*.log_fmt_hdr))
    KEEP(*(log_fmt))
}
