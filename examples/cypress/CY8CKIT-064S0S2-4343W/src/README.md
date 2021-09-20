# CY8CKIT-064S0S2-4343W port

Port for the
[CY8CKIT-064S0S2-4343W](https://www.cypress.com/documentation/development-kitsboards/psoc-64-standard-secure-aws-wi-fi-bt-pioneer-kit-cy8ckit)
evaluation kit.

# Integration steps

Make sure that both `flash_obj` and `flash_obj_info` from
`memfault_platform_storage.h/c` are initialized before the actual Memfault
library.

`__MfltCoredumpsStart` and `__MfltCoredumpsEnd` define the memory region for
coredumps. It should be a 512-byte aligned flash memory region, at least 96kB
size is recommended. The demo application has an example implementation you can
use.

To uniquely identify your firmware type, version & device in your Memfault
dashboard, please also make sure to edit `memfault_platform_port.c` and fill all
the related fields in `memfault_platform_get_device_info`.

# Port details

Coredumps include data, bss & stack regions.

Port structure:

- `memfault_platform_port.c` - Device info, reboot, memory range, reboot reason,
  logging & memfault/freertos boot setup.
- `memfault_platform_storage.c/h` - Implements IO operations on coredump region
  in flash memory & device memory regions to dump.
- `memfault_test.c/h` - Functions for testing Memfault functionality - on demand
  heartbeat, trace and fault generation.
