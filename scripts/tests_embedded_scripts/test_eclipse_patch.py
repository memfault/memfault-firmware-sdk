#
# Copyright (c) Memfault, Inc.
# See LICENSE for details
#

import os
import subprocess
import sys
import tempfile
from pathlib import Path

MEMFAULT_SDK_ROOT_DIR = Path(__file__).resolve().parents[2]


def test_eclipse_project_patcher(snapshot):
    # create a temp directory for test output
    with tempfile.TemporaryDirectory() as temp_dir:
        # patch the eclipse project
        subprocess.run(
            [
                sys.executable,
                MEMFAULT_SDK_ROOT_DIR / "scripts" / "eclipse_patch.py",
                "--project-dir",
                Path(__file__).resolve().parent / "testinput",
                "--output",
                temp_dir,
            ],
            check=True,
            cwd=MEMFAULT_SDK_ROOT_DIR,
        )

        # read the patched project
        patched_project = {}
        for root, _, files in os.walk(temp_dir):
            for file in files:
                with open(os.path.join(root, file), "r") as f:
                    patched_project[file] = f.read()

        snapshot.assert_match(patched_project)
