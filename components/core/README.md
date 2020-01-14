# core

Common code that is used by all other components.

## Application/platform-specific customizations

### Device information

The file `include/memfault/platform/device_info.h` contains the functions that
the Memfault SDK will call to get the unique device identifier (device serial),
software type, software version, hardware version, etc. The provided platform
reference implementation usually uses something like the WiFi MAC or chip ID as
base for the unique device identifier and uses preprocessor variables to get the
firmware and hardware versions (with fallbacks to hard-coded values, i.e.
`"xyz-main"` as software type `"1.0.0"` as software version and `"xyz-proto"` as
hardware version).

[To learn more about the concepts software type, software version, hardware version, etc. take a look at this documentation](https://mflt.io/36NGGgi).

Please change `memfault_platform_device_info.c` in ways that makes sense for
your project.
