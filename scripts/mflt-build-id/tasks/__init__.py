#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import invoke


@invoke.task
def test(ctx):
    """Run tests."""
    ctx.run("pytest", pty=True)
