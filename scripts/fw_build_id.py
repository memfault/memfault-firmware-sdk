#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

"""
Shim around mflt_build_id to keep the original fw_build_id.py file (this file) working as before.
See mflt-build-id/src/mflt_build_id/__init__.py for actual source code.
"""

import os
import sys

scripts_dir = os.path.dirname(os.path.realpath(__file__))
bundled_mflt_build_id_src_dir = os.path.join(scripts_dir, "mflt-build-id", "src")

if os.path.exists(bundled_mflt_build_id_src_dir):
    # Released SDK:
    sys.path.insert(0, bundled_mflt_build_id_src_dir)

from mflt_build_id import *  # noqa: E402, F403  # pyright: ignore[reportWildcardImportFromLibrary]

if __name__ == "__main__":
    from mflt_build_id import main

    main()
