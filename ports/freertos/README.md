# FreeRTOS Specific Port

## Overview

This directory contains an implementation of Memfault dependency functions when
a platform is built on top of [FreeRTOS](https://www.freertos.org/).

## Usage

1. Add `$MEMFAULT_FIRMWARE_SDK/ports/freertos/include` to your projects include
   path
2. Prior to using the Memfault SDK in your code, initialize the FreeRTOS port.

```c
#include "memfault/ports/freertos.h"

void main(void) {

  memfault_freertos_port_boot();
}
```
