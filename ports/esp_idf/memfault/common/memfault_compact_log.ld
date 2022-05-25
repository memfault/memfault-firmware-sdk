/*
  Custom section to be included in the linker command file to support compact
  logs. Read more at https://mflt.io/compact-logs
*/

SECTIONS
{
  log_fmt 0xF0000000 (INFO) :
  {
      /*
      explicitly define this symbol. some versions of the xtensa toolchain
      (esp-2020r3-8.4.0 at least) incorrectly places it by default at 8 bytes
      into the log_fmt section, which causes decoding to fail.
      */
      __start_log_fmt = ABSOLUTE(.);
      KEEP(*(*.log_fmt_hdr))
      KEEP(*(log_fmt))
  }
}
