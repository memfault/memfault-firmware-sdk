# Memfault Library for ModusToolbox:tm:

To get started with the Memfault ModusToolbox:tm: port see the integration guide
available [here](https://mflt.io/mtb-integration-guide).

## Port Configuration

The configuration files for this port contain parameters to modify the port and the SDK in general. The `configs/`
directory contains the default values.

### Metrics Configuration

The port comes with two sets of built-in metrics for WiFi and memory tracking. These metrics are controlled by these
two definitions:

- MEMFAULT_PORT_WIFI_TRACKING_ENABLED
- MEMFAULT_PORT_MEMORY_TRACKING_ENABLED

Setting these to 0 will disable the corresponding metrics.

### SDK Configuration

A set of typical defaults is provided in `configs/memfault_mtb_platform_config.h`. If you wish to override other SDK
values, simply create an application configuration file at `<path_to_app>/configs/memfault_platform_config.h`
