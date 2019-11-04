#
# Copyright (c) 2019-Present Memfault, Inc.
# See License.txt for details
#
import os

from invoke import Collection, task

from . import nrf, wiced
from .macos_ftdi import is_macos

SDK_FW_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SDK_FW_TESTS_ROOT = os.path.join(SDK_FW_ROOT, "tests")


@task
def fw_sdk_unit_test(ctx, coverage=False, rule="", test_filter=None, test_dir=SDK_FW_TESTS_ROOT):
    """Runs unit tests"""
    env_dict = {}
    if is_macos():
        env_dict["CPPUTEST_HOME"] = "/usr/local/Cellar/cpputest/3.8"
        env_dict["TARGET_PLATFORM"] = ""

    if coverage:
        rule += " lcov"

    if test_filter:
        env_dict["TEST_MAKEFILE_FILTER"] = test_filter

    with ctx.cd(test_dir):
        ctx.run("make {}".format(rule), env=env_dict, pty=True)


ns = Collection()
ns.add_collection(wiced)
ns.add_collection(nrf)
ns.add_task(fw_sdk_unit_test, name="test")
# When adding esp32 here, remove it from /tasks/sdk.py


@task(pre=[wiced.wiced_clean, wiced.wiced_build, nrf.nrf_clean, nrf.nrf_build])
def build_all_demos(ctx):
    """ Builds all demo apps (for CI purposes) """
    pass


ci = Collection("~ci")
ci.add_task(build_all_demos, name="build-all-demos")
ns.add_collection(ci)
