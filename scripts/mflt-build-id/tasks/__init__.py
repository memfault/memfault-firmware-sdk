#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

# flake8: noqa: M900

# We don't have invoke in the requirements because this is used by *other* packages,
# not by this package itself.

import invoke


@invoke.task
def test(ctx):
    """Run tests."""
    ctx.run("pytest", pty=True)
