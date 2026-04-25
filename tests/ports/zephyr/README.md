# Zephyr Port Tests

This directory contains tests for the Zephyr port of the Memfault Embedded SDK.
These tests are run as part of the CI for the Zephyr port, and can be run
locally as well.

```bash
# initialize the west workspace
west init -l manifest
west update
# build and run the tests
west twister --testsuite-root . --test-config test_config.yaml --level smoke
```

For Linux hosts, you will likely need to install i386 build support. On
Debian-flavored Linux distributions, this should do it:

```bash
sudo apt install gcc-multilib
```
