# Silicon Labs emlib Port

This port adds implementations for coredump storage, CLI, reboot reasons, and
watchdog features using the Gecko SDK/emblib.

## Compatibility

The port has been built against v3.0 of the Gecko SDK. The port has been built
and tested against v4.3.0 using SimplicityStudio.

## Features

The port provides the following features:

- Coredump storage using a linker-defined region in internal Flash
- Reboot reason implementation for collecting reboot events
- An integration with the Watchdog peripheral to capture watchdog-triggered
  traces
- A Gecko SDK compatible CLI to demo Memfault features

## Adding To Your Project

The simplest way to add this to your project is to do the following:

1. Add the source files in `ports/emblib` to your build system
2. Add the include directory at `ports/include`

## CLI Component

The CLI component requires building with SimplicityStudio for full
functionality. After adding the file as source to your project, you will need to
add the following snippets to your .slcp file:

```yaml
# In component section
component:
  - instance: [example]
    id: cli
```

```yaml
# In template_contribution
template_contribution:
  - condition: [cli]
    name: cli_group
    priority: 0
    value: { name: mflt }
  - condition: [cli]
    name: cli_group
    priority: 0
    value: { name: test, id: mflt_test_root, group: mflt }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: export,
        group: mflt,
        handler: memfault_emlib_cli_export,
        help: Export data as base64 chunks,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: get_core,
        group: mflt,
        handler: memfault_emlib_cli_get_core,
        help: Get current coredump state,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: clear_core,
        group: mflt,
        handler: memfault_emlib_cli_clear_core,
        help: Clear coredump from storage,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: get_device_info,
        group: mflt,
        handler: memfault_emlib_cli_get_device_info,
        help: Read device info structure,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: assert,
        group: mflt_test_root,
        handler: memfault_emlib_cli_assert,
        help: Triggers assert to collect a coredump,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: busfault,
        group: mflt_test_root,
        handler: memfault_emlib_cli_busfault,
        help: Triggers busfault to collect a coredump,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: hang,
        group: mflt_test_root,
        handler: memfault_emlib_cli_hang,
        help: Triggers hang to collect a coredump,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: hardfault,
        group: mflt_test_root,
        handler: memfault_emlib_cli_hardfault,
        help: Triggers hardfault to collect a coredump,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: memmanage,
        group: mflt_test_root,
        handler: memfault_emlib_cli_memmanage,
        help: Triggers memory management fault to collect a coredump,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: usagefault,
        group: mflt_test_root,
        handler: memfault_emlib_cli_usagefault,
        help: Triggers usage fault to collect a coredump,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: reboot,
        group: mflt_test_root,
        handler: memfault_emlib_cli_reboot,
        help: Triggers reboot to test reboot reason tracking,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: heartbeat,
        group: mflt_test_root,
        handler: memfault_emlib_cli_heartbeat,
        help: Trigger capture of heartbeat metrics,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: logs,
        group: mflt_test_root,
        handler: memfault_emlib_cli_logs,
        help: Writes logs to internal buffers,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: log_capture,
        group: mflt_test_root,
        handler: memfault_emlib_cli_log_capture,
        help: Serializes current log buffer contents,
      }
  - condition: [cli]
    name: cli_command
    priority: 0
    value:
      {
        name: trace,
        group: mflt_test_root,
        handler: memfault_emlib_cli_trace,
        help: Captures a trace event,
      }
```
