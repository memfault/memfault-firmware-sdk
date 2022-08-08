# Memfault Library for use with Particle Device OS

Ship Firmware with Confidence.

More details about the Memfault platform itself, how it works, and step-by-step
integration guides
[can be found here](https://mflt.io/particle-getting-started).

The library is compatible with all versions of
[Device OS](https://github.com/particle-iot/device-os) greater than or equal to
Device OS 3.0

:exclamation: Note: Use the
[particle-firmware-library](https://github.com/memfault/particle-firmware-library)
repository to add memfault support to an application. This repository is updated
as part of the release process for the
[memfault-firmware-sdk](https://github.com/memfault/memfault-firmware-sdk).

## Welcome to your library!

To get started, add the library to your particle application:

```bash
$ git submodule add https://github.com/memfault/particle-firmware-library lib/memfault
```

## Integration Steps

1. Add the following to your application

   ```c
   #include "memfault.h"
   Memfault memfault;

   void loop() {
     // ...
     memfault.process();
     // ...
   }
   ```

2. Create a Memfault Project Key at
   https://goto.memfault.com/create-key/particle and copy it to your clipboard.

3. Navigate to the Integrations tab in the Particle Cloud UI and create a new
   "Custom Template" webhook. Be sure to replace `MEMFAULT_PROJECT_KEY` below
   with the one copied in step 2.

   ```json
   {
     "event": "memfault-chunks",
     "responseTopic": "",
     "url": "https://chunks.memfault.com/api/v0/chunks/{{PARTICLE_DEVICE_ID}}",
     "requestType": "POST",
     "noDefaults": false,
     "rejectUnauthorized": true,
     "headers": {
       "Memfault-Project-Key": "MEMFAULT_PROJECT_KEY",
       "Content-Type": "application/octet-stream",
       "Content-Encoding": "base64"
     },
     "body": "{{{PARTICLE_EVENT_VALUE}}}"
   }
   ```

## Example Usage

See the [examples/memfault_test/](examples/memfault_test) folder for a complete
application demonstrating how to use Memfault.

## Questions

Don't hesitate to contact us for help! You can reach us through
[support@memfault.com](mailto:support@memfault.com).
