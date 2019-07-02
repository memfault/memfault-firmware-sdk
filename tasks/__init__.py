#
# Copyright (c) 2019-Present Memfault, Inc.
# See License.txt for details
#
from invoke import Collection, task

from .wiced import WICED, wiced_build, wiced_clean
from .nrf import NRF5, nrf_clean, nrf_build

COLLECTIONS = (WICED, NRF5)

ns = Collection()
for collection in COLLECTIONS:
    ns.add_collection(collection)


@task(pre=[wiced_clean, wiced_build, nrf_clean, nrf_build])
def build_all_demos(ctx):
    """ Builds all demo apps (for CI purposes) """
    pass


ci = Collection("~ci")
ci.add_task(build_all_demos, name="build-all-demos")
ns.add_collection(ci)
