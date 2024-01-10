#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

"""
A small wrapper around the Make-based unit test driver, to provide a pytest
front-end for running the tests.
"""

import subprocess
from pathlib import Path

import pytest

TEST_DIR = Path(__file__).parent

# Run this special Make target to print out the set of tests that are available.
# This will honor the TEST_MAKEFILE_FILTER environment variable, if set, so the
# same filtering behavior is supported.
output = subprocess.check_output([
    "make",
    "--no-print-directory",
    "-C",
    TEST_DIR,
    "print-makefiles",
])
test_makefiles = output.decode().split()


# Pass the test_makefiles list as a parameter to the test function, so that one
# test runs for each makefile target. NOTE: this does bypass the Make dependency
# graph, especially if pytest is run with the pytest-xdist parallel runner, so
# this WILL FAIL in weird ways if we add a shared prerequisite to the test
# targets! Right now each test target is independent, so this is OK.
@pytest.mark.parametrize(
    "target_makefile",
    test_makefiles,
)
def test_cpputest(target_makefile):
    subprocess.check_call(["make", "-C", TEST_DIR, target_makefile])
