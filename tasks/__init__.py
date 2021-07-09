#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import os

from invoke import Collection, task

from . import esp32, mbed, nrf, nrfconnect, wiced, zephyr
from .macos_ftdi import is_macos

SDK_FW_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SDK_FW_TESTS_ROOT = os.path.join(SDK_FW_ROOT, "tests")


@task
def fw_sdk_unit_test(ctx, coverage=False, rule="", test_filter=None, test_dir=SDK_FW_TESTS_ROOT):
    """Runs unit tests"""
    env_dict = {}
    if is_macos():
        # Search to see if CPPUTEST_HOME is already on the path (i.e in a conda environment)
        # Otherwise, fallback to the default install location used with brew
        env_dict["CPPUTEST_HOME"] = os.environ.get(
            "CPPUTEST_HOME", "/usr/local/Cellar/cpputest/4.0"
        )

    if "CPPUTEST_HOME" in os.environ:
        # override target platform so the test build system can locate the
        # conda-installed cpputest libraries. the conda-forge linux installation
        # puts the library under lib64 for some reason 🙄
        env_dict["TARGET_PLATFORM"] = "lib" if is_macos() else "lib64"

    if coverage:
        rule += " lcov"

    if test_filter:
        env_dict["TEST_MAKEFILE_FILTER"] = test_filter

    with ctx.cd(test_dir):
        ctx.run("make {}".format(rule), env=env_dict, pty=True)


ns = Collection()
ns.add_collection(mbed)
ns.add_collection(nrf)
ns.add_collection(wiced)
ns.add_collection(esp32)
ns.add_task(fw_sdk_unit_test, name="test")


@task(
    pre=[
        mbed.mbed_clean,
        mbed.mbed_build,
        nrf.nrf_clean,
        nrf.nrf_build,
        wiced.wiced_clean,
        wiced.wiced_build,
        esp32.esp32_app_clean,
        esp32.esp32_app_build,
    ]
)
def build_all_demos(ctx):
    """ Builds all demo apps (for CI purposes) """
    pass


ci = Collection("~ci")
ci.add_task(build_all_demos, name="build-all-demos")
ci.add_task(zephyr.zephyr_project_ci_setup)
ci.add_task(nrfconnect.nrfconnect_project_ci_setup)
ns.add_collection(ci)
