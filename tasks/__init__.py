#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import os
import pathlib
from shutil import which

from invoke import Collection, task

from . import esp32, mbed, nrf, nrfconnect, wiced, zephyr
from .macos_ftdi import is_macos

SDK_FW_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SDK_FW_TASKS_DIR = pathlib.Path(os.path.join(SDK_FW_ROOT, "tasks"))
SDK_FW_TESTS_ROOT = os.path.join(SDK_FW_ROOT, "tests")


@task(
    help={
        "coverage": "Generate coverage report",
        "rule": "Override default make rule to run",
        "test_filter": "Test filter expression, eg '*metrics*'",
        "test_dir": "Test directory (typically tests/)",
        "extra_make_options": "Extra make options",
        "verbose": "Verbose output",
        "no_pytest": "Don't run pytest, instead run the Make-based test runner",
    }
)
def fw_sdk_unit_test(
    ctx,
    coverage=False,
    rule="",
    test_filter=None,
    test_dir=SDK_FW_TESTS_ROOT,
    extra_make_options="",
    verbose=False,
    no_pytest=False,
):
    """Runs unit tests"""

    if rule == "clean":
        builddir = os.path.abspath(os.path.join(test_dir, "build"))
        ctx.run(f"rm -rf {builddir}")
        return

    # Check if it's necessary to set the CPPUTEST_HOME variable; Macos (brew)
    # and conda environment requires this
    env_dict = {}
    if is_macos():
        # best effort- if brew exists, try to use it to source the cpputest
        # install location
        brew = which("brew")
        if brew:
            result = ctx.run("brew --prefix cpputest")
            cpputest_home = result.stdout.strip()

            env_dict["CPPUTEST_HOME"] = cpputest_home

    # if this is already set in the host environment, use it- it's probably a
    # conda environment
    if "CPPUTEST_HOME" in os.environ:
        env_dict["CPPUTEST_HOME"] = os.environ["CPPUTEST_HOME"]

    if test_filter:
        env_dict["TEST_MAKEFILE_FILTER"] = test_filter

    make_options = []

    # set output-sync option only if make supports it. macos uses a 12+ year old
    # copy of make by default that doesn't have this option.
    result = ctx.run("make --help", hide=True)
    if "--output-sync" in result.stdout:
        make_options.append("--output-sync=recurse")

    if extra_make_options:
        make_options.append(extra_make_options)

    cpus = 1
    if os.getenv("CIRCLECI"):
        # getting the number of cpus available to the circleci executor from
        # within the docker container is a hassle, so bail and use 2 cpus
        cpus = 2
    else:
        try:
            # Only available on Linux, but it's better
            cpus = len(os.sched_getaffinity(0))  # pyright: ignore[reportGeneralTypeIssues]
        except AttributeError:
            # Available on Mac
            cpus = int((os.cpu_count() or 4) / 2)

    make_options.extend(["-j", str(cpus)])

    # force colorized cpputest output
    env_dict["CPPUTEST_EXE_FLAGS"] = "-c " + os.environ.get("CPPUTEST_EXE_FLAGS", "")

    # force compiler colored output; it detects as running in a non-tty but the
    # color output is useful
    env_dict["COMPILER_SPECIFIC_WARNINGS"] = "-fdiagnostics-color=always " + os.environ.get(
        "COMPILER_SPECIFIC_WARNINGS", ""
    )

    with ctx.cd(test_dir):
        # The main unit tests (in the 'tests' directory) use a test.py file to
        # drive the test run. The internal tests, in the 'internal/tests' directory,
        # use a Make-based test runner, so support both.
        if os.path.exists(os.path.join(test_dir, "test.py")) and not no_pytest:
            # run the tests with pytest
            test_cmd = "pytest --numprocesses=auto {verbose} test.py".format(
                verbose="-v" if verbose else ""
            )
        else:
            # run normal make-based tests
            test_cmd = "make {} {}".format(" ".join(make_options), rule)

        ctx.run(test_cmd, env=env_dict, pty=True)

        if coverage:
            # run lcov to generate coverage report
            ctx.run(
                "make --no-print-directory SILENCE=@ {} lcov".format(" ".join(make_options)),
                env=env_dict,
                pty=True,
            )


ns = Collection()
ns.add_task(fw_sdk_unit_test, name="test")
ns.add_collection(mbed)
ns.add_collection(nrf)
ns.add_collection(wiced)
ns.add_collection(esp32)
if os.environ.get("STM32_ENABLED"):
    from . import stm32

    ns.add_collection(stm32)

# These tasks are only included if they exist in the SDK
if (SDK_FW_TASKS_DIR / "internal.py").exists():
    from . import internal

    ns.add_collection(internal)

if (SDK_FW_TASKS_DIR / "modus.py").exists():
    from . import modus

    ns.add_collection(modus)


@task(
    pre=[
        esp32.esp32_app_clean,
        esp32.esp32_app_build,
        esp32.esp32_app_clean,
        esp32.esp32s2_app_build,
        esp32.esp32_app_clean,
        esp32.esp32s3_app_build,
    ]
)
def build_all_demos(ctx):
    """Builds all demo apps (for CI purposes)"""


ci = Collection("~ci")
ci.add_task(build_all_demos, name="build-all-demos")
ci.add_task(zephyr.zephyr_project_ci_setup)
ci.add_task(nrfconnect.nrfconnect_project_ci_setup)
ns.add_collection(ci)
