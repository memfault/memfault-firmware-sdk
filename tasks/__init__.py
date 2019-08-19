#
# Copyright (c) 2019-Present Memfault, Inc.
# See License.txt for details
#
from invoke import Collection, task

from . import wiced
from . import nrf

ns = Collection()
ns.add_collection(wiced)
ns.add_collection(nrf)
# When adding esp32 here, remove it from /tasks/sdk.py


@task(pre=[wiced.wiced_clean, wiced.wiced_build, nrf.nrf_clean, nrf.nrf_build])
def build_all_demos(ctx):
    """ Builds all demo apps (for CI purposes) """
    pass


ci = Collection("~ci")
ci.add_task(build_all_demos, name="build-all-demos")
ns.add_collection(ci)
