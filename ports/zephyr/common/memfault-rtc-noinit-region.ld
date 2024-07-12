/**
* For ESP32 variants other than ESP32-S3/S2, provide a region of noinit placed
in * RTC memory, for holding reboot reason information through OTA updates. *
The normal Zephyr noinit region is not suitable for this, because its offset *
can change through an OTA update, invalidating the reboot reason data.
 *
 * Unfortunately this doesn't place the section in the same order in RTC as in
 * the ESP32-S3/S2, but it puts it in a usable location.
 */
#if !defined(CONFIG_SOC_SERIES_ESP32S3) && !defined(CONFIG_SOC_SERIES_ESP32S2)
SECTION_PROLOGUE(.rtc_noinit,(NOLOAD),)
{
    . = ALIGN(4);
    _rtc_noinit_start = ABSOLUTE(.);
    *(.rtc_noinit .rtc_noinit.*)
    . = ALIGN(4) ;
    _rtc_noinit_end = ABSOLUTE(.);
} GROUP_LINK_IN(rtc_slow_seg)
#endif
