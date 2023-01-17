#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import os
import shlex
import shutil
import sys

from invoke import Collection, task

from .print_chunk_watcher import PrintChunkWatcher

TASKS_DIR = os.path.dirname(__file__)
MEMFAULT_SDK_ROOT = os.path.dirname(TASKS_DIR)
MBED_TARGET = "DISCO_F429ZI"
MBED_TOOLCHAIN = "GCC_ARM"
MBED_BAUD_RATE = 115200
MBED_ROOT = os.path.join(MEMFAULT_SDK_ROOT, "examples", "mbed")
MBED_DEMO_APP_ROOT = os.path.join(MBED_ROOT, "apps", "memfault_demo_app")
MBED_DEMO_APP_BUILD_ROOT = os.path.join(
    MBED_DEMO_APP_ROOT, "BUILD", MBED_TARGET, "{}-DEBUG".format(MBED_TOOLCHAIN)
)
MBED_DEMO_APP_BUILD_PROFILE = os.path.join(
    MBED_DEMO_APP_ROOT, "mbed-os", "tools", "profiles", "debug.json"
)
MBED_DEMO_APP_ELF = os.path.join(MBED_DEMO_APP_BUILD_ROOT, "memfault_demo_app.elf")


@task
def mbed_clean(ctx):
    """Clean demo app that runs on Mbed OS 5"""
    print("Mbed CLI does not have a separate 'clean' action; approximating it...")
    if os.path.exists(MBED_DEMO_APP_BUILD_ROOT):
        shutil.rmtree(MBED_DEMO_APP_BUILD_ROOT)
    print("Clean finished.")


@task
def mbed_update(ctx):
    """Update or install the library dependencies for the Mbed demo app"""
    cmd = "mbed update"
    with ctx.cd(MBED_DEMO_APP_ROOT):
        ctx.run(cmd, pty=True)

        # 'mbed update' above installs some pretty out-of-date python packages
        # into the current python environment. Update them to more recent
        # versions that are compatible with python3.10.
        pip_packages = (
            "future==0.18.2",
            "intelhex==2.3.0",
            "pyelftools==0.29",
            # A hack to work around version pinning issue with mbed-os and
            # incompatibility with markupsafe 2.1.0 release
            "markupsafe==2.0.1",
        )
        # explicitly run the version of pip installed to the _active_ python
        # environment. in CI we auto load a specific venv in ~/.bashrc, which
        # confuses invoke ☹️ and causes the wrong pip to be run. pty=True should
        # fix this but doesn't seem to work. normally it's fine, but this
        # command runs in a separate manually created venv so things are more
        # delicate.
        # https://github.com/memfault/memfault/blob/299e7ee7ceec81449fc12d3658fc1ed8d99ebf81/docker-images/ci/Dockerfile#L98
        ctx.run("{} -m pip install {}".format(sys.executable, " ".join(pip_packages)), pty=True)

    # apply this patch to enable using ccache when building with the mbed cli.
    # some of these flags to 'patch' are probably not gonna work on mac osx. too
    # bad.
    with ctx.cd(MEMFAULT_SDK_ROOT):
        ctx.run("patch --forward -r- -p0 < tasks/mbed_ccache.patch", warn=True)

    print("Update finished.  Ignore warnings about source control above; do not run 'mbed new'.")


@task
def _mbed_update_required(ctx):
    """Most tasks require that the mbed_update task have been run at least once
    before to install mbed-os.  Do it now if it has not been done yet."""
    mbed_dir = os.path.join(MBED_DEMO_APP_ROOT, "mbed-os")
    if not os.path.isdir(mbed_dir):
        print("Mbed dependencies have not been installed yet; running 'mbed update'...")
        mbed_update(ctx)


@task(
    pre=[_mbed_update_required],
    help={
        "profile": "Mbed build profile file (def: {})".format(MBED_DEMO_APP_BUILD_PROFILE),
        "toolchain": "Mbed toolchain name (def: {})".format(MBED_TOOLCHAIN),
        "target": "Mbed target name (def: {})".format(MBED_TARGET),
        "flash": "Flash the target after building (def: false)",
    },
)
def mbed_build(
    ctx,
    profile=MBED_DEMO_APP_BUILD_PROFILE,
    toolchain=MBED_TOOLCHAIN,
    target=MBED_TARGET,
    flash=False,
):
    """Build the demo app that runs on Mbed OS 5"""
    cmd = "mbed compile --profile {} --toolchain {} --target {}".format(
        shlex.quote(profile), shlex.quote(toolchain), shlex.quote(target)
    )
    if flash:
        cmd += " --flash"

    with ctx.cd(MBED_DEMO_APP_ROOT):
        # to make ccache work, we need to set pretty aggressive sloppiness
        # (non-reproducible builds otherwise), and set and explicit
        # MBED_BUILD_TIMESTAMP value
        ctx.run(
            cmd,
            pty=True,
            env={
                "MBED_BUILD_TIMESTAMP": "0",
                "CCACHE_SLOPPINESS": "include_file_ctime,include_file_mtime,time_macros",
            },
        )
    print("Build finished.  Output: {}".format(MBED_DEMO_APP_ELF))


@task(
    pre=[_mbed_update_required], help={"target": "Mbed target name (def: {})".format(MBED_TARGET)}
)
def mbed_flash(ctx, target=MBED_TARGET):
    """Flash (and first build) the demo app that runs on Mbed OS 5"""
    print("Mbed CLI does not have a separate 'flash' action; approximating it...")
    mbed_build(ctx, flash=True, target=target)
    print("Flashing finished.")


@task(
    pre=[_mbed_update_required],
    help={
        "baudrate": "Baud rate (def: {})".format(MBED_BAUD_RATE),
        "target": "Mbed target name (def: {})".format(MBED_TARGET),
    },
)
def mbed_console(ctx, baudrate=MBED_BAUD_RATE, target=MBED_TARGET):
    """Start an Mbed serial console to interact with the demo app"""
    cmd = "mbed sterm --baudrate {:d} --echo off --target {}".format(baudrate, shlex.quote(target))
    with ctx.cd(MBED_DEMO_APP_ROOT):
        ctx.run(cmd, pty=True, watchers=[PrintChunkWatcher(ctx)])


ns = Collection("mbed")
ns.add_task(mbed_build, name="build")
ns.add_task(mbed_clean, name="clean")
ns.add_task(mbed_console, name="console")
ns.add_task(mbed_flash, name="flash")
ns.add_task(mbed_update, name="update")
