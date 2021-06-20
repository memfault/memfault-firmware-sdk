#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

# -*- coding: utf-8 -*-
# snapshottest: v1 - https://goo.gl/zC4yUc
from __future__ import unicode_literals

from snapshottest import Snapshot

snapshots = Snapshot()

snapshots["test_build_id_dump 1"] = ["1", "16", "16e0fe39af176cfa4cf961321ccf51"]

snapshots["test_gnu_build_id_in_use 1"] = [
    "Found GNU Build ID: 3d6f95306bbc5fda4183728b3829f88b30f7aa1c"
]

snapshots["test_gnu_build_id_present_but_not_used 1"] = [
    "WARNING: Located a GNU build id but it's not being used by the Memfault SDK",
    "Added Memfault Generated Build ID to ELF: 028aa9800f0524c9b064711925df5ec403df6b16",
    "Found Memfault Build Id: 028aa9800f0524c9b064711925df5ec403df6b16",
]

snapshots["test_memfault_id_populated 1"] = [
    "Found Memfault Build Id: 16e0fe39af176cfa4cf961321ccf5193c2590451"
]

snapshots["test_memfault_id_unpopulated 1"] = [
    "Added Memfault Generated Build ID to ELF: 16e0fe39af176cfa4cf961321ccf5193c2590451",
    "Found Memfault Build Id: 16e0fe39af176cfa4cf961321ccf5193c2590451",
]
