# panics

Fault handling, coredump, reboot tracking and reboot loop detection API. See
documentation in header files for details on the API itself.

## Application/platform-specific customizations

### Coredump memory regions & storage

The file `include/memfault/platform/coredump.h` contains two functions that
likely need to be customized to your application:

- `memfault_platform_coredump_get_regions()`: the platform reference
  implementation usually capture all of RAM (if possible on the reference
  platform). Capturing all of RAM provides the best debugging experience. But
  this may not feasible, for example if not enough (flash) storage space is
  available. Or it may not be desirable, for example if RAM contains sensitive
  information that is not allowed to leave the device.

- `memfault_platform_coredump_storage_get_info()` - the platform reference
  implementation usually allocates a large size for coredump storage. Depending
  on how you change `memfault_platform_coredump_get_regions()`, you may want to
  shrink the storage size as well.

- `memfault_platform_coredump_storage...()` - the platform reference
  implementation uses whatever storage the reference platform has handy for
  storing coredump data. This may or may not be the best choice for your
  application.

Please change the reference implementations in ways that makes sense for your
project.
