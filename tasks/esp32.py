import os

from glob import glob
from invoke import task
from invoke import Collection
from sys import executable as PYTHON, exit
from .gdb import gdb_build_cmd
from .macos_ftdi import apple_ftdi_driver_disable

TASKS_DIR = os.path.dirname(__file__)
MEMFAULT_SDK_ROOT = os.path.join(TASKS_DIR, "..")
ESP32_PLATFORM_ROOT = os.path.join(MEMFAULT_SDK_ROOT, "platforms", "esp32")
ESP32_IDF_ROOT = os.path.join(ESP32_PLATFORM_ROOT, "esp-idf")
ESP32_IDF_SCRIPT = os.path.join(ESP32_IDF_ROOT, "tools", "idf.py")
ESP32_COREDUMP_SCRIPT = os.path.join(ESP32_IDF_ROOT, "components", "espcoredump", "espcoredump.py")
ESP32_TEST_APP_ROOT = os.path.join(ESP32_PLATFORM_ROOT, "apps", "memfault_demo_app")
ESP32_TEST_APP_ELF = os.path.join(ESP32_TEST_APP_ROOT, "build", "memfault-esp32-test-app.elf")
ESP32_TEST_APP_COREDUMP_OFFSET = 0x210000  # FIXME: pull from partitions.bin/csv ?

OPENOCD_GDB_PORT_DEFAULT = 3333


def _run_idf_script(ctx, *args, **kwargs):
    with ctx.cd(ESP32_TEST_APP_ROOT):
        ctx.run(
            "{idf} {args}".format(idf=ESP32_IDF_SCRIPT, args=" ".join(args)),
            env={"IDF_PATH": ESP32_IDF_ROOT, "PYTHON": PYTHON},
            **kwargs,
        )


def _esp32_guess_console_port():
    def _esp32_find_console_port():
        from pyftdi.usbtools import UsbTools

        # Try pyftdi first:
        devs = UsbTools.find_all([(0x0403, 0x6010)])
        if devs:
            return "ftdi://ftdi:2232/2"

        # Fall back to /dev/cu... device:
        usb_paths = glob("/dev/cu.usbserial-*1")
        if usb_paths:
            return usb_paths[0]

        print(
            "Cannot find ESP32 console /dev/... nor ftdi:// path, please specify it manually using --port"
        )
        exit(1)

    port = _esp32_find_console_port()
    print("No --port specified, using console port {port}".format(port=port))
    return port


@task
def esp32_app_build(ctx):
    """Build the ESP32 test app"""
    _run_idf_script(ctx, "build")


@task
def esp32_app_clean(ctx):
    """Clean the ESP32 test app"""
    _run_idf_script(ctx, "fullclean")


@task
def esp32_app_flash(ctx, port=None):
    """Flash the ESP32 test app"""
    if port is None:
        port = _esp32_guess_console_port()
    _run_idf_script(ctx, "-p {port}".format(port=port), "flash")


@task
def esp32_console(ctx, port=None):
    """Flash the ESP32 test app"""
    if port is None:
        port = _esp32_guess_console_port()
    # For now, just use miniterm, idf_monitor.py doesn't play nice with pyftdi for some reason :(
    # _run_idf_script(ctx, '-p {port}'.format(port=port), 'monitor', pty=True)
    with apple_ftdi_driver_disable(ctx):
        ctx.run("miniterm.py --raw {port} 115200".format(port=port), pty=True)


@task
def esp32_app_menuconfig(ctx):
    """Run menuconfig for the ESP32 test app"""
    _run_idf_script(ctx, "menuconfig", pty=True)


@task
def esp32_read_coredump(
    ctx, port=None, info=False, elf=ESP32_TEST_APP_ELF, flash_offset=ESP32_TEST_APP_COREDUMP_OFFSET
):
    """Reads coredump from the device using espcoredump.py tool"""
    if port is None:
        port = _esp32_guess_console_port()
    operation = "info_corefile" if info else "dbg_corefile"
    with ctx.cd(ESP32_TEST_APP_ROOT):
        ctx.run(
            "{python} {espcoredump} --port {port} {operation} --off {flash_offset} {elf}".format(
                python=PYTHON,
                espcoredump=ESP32_COREDUMP_SCRIPT,
                port=port,
                operation=operation,
                elf=elf,
                flash_offset=flash_offset,
            ),
            env={"IDF_PATH": ESP32_IDF_ROOT, "PYTHON": PYTHON},
            pty=True,
        )


@task
def esp32_openocd(ctx):
    """Launch openocd"""
    if "ESP32_OPENOCD" not in os.environ:
        print("Set ESP32_OPENOCD environment variable to point to openocd-esp32 root directory!")
        print(
            "Download the openocd-esp32 binaries here: https://github.com/espressif/openocd-esp32/releases"
        )
        exit(-1)
    with apple_ftdi_driver_disable(ctx):
        with ctx.cd(os.environ["ESP32_OPENOCD"]):
            ctx.run(
                "bin/openocd -s share/openocd/scripts "
                "-f interface/ftdi/esp32_devkitj_v1.cfg -f board/esp-wroom-32.cfg",
                pty=True,
            )


@task
def esp32_app_gdb(ctx, gdb=None, reset=False):
    """Launches xtensa-gdb with app elf and connects to openocd gdb server"""
    if gdb is None:
        gdb = OPENOCD_GDB_PORT_DEFAULT
    with ctx.cd(ESP32_TEST_APP_ROOT):
        gdb_cmd = gdb_build_cmd(
            "", ESP32_TEST_APP_ELF, gdb, gdb_prefix="xtensa-esp32-elf-", reset=reset
        )
        ctx.run(gdb_cmd, pty=True)


ESP32 = Collection("esp32")
ESP32.add_task(esp32_console, name="console")
ESP32.add_task(esp32_openocd, name="openocd")
ESP32.add_task(esp32_read_coredump, name="read-coredump")
ESP32.add_task(esp32_app_build, name="app-build")
ESP32.add_task(esp32_app_clean, name="app-clean")
ESP32.add_task(esp32_app_flash, name="app-flash")
ESP32.add_task(esp32_app_gdb, name="app-gdb")
ESP32.add_task(esp32_app_menuconfig, name="app-menuconfig")
