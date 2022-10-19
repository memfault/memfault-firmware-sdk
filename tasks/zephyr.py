#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import json
import os

from invoke import task

SDK_FW_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ZEPHYR_ROOT = os.path.join(SDK_FW_ROOT, "examples", "zephyr", "stm32l4_disco")
ZEPHYR_UPSTREAM_REPO = "git@github.com:zephyrproject-rtos/zephyr.git"


def _shallow_clone_and_checkout(ctx, repo_uri, branch, dest_dir, commit):
    ctx.run(f"git clone {repo_uri} --single-branch --branch {branch} {dest_dir}")
    with ctx.cd(dest_dir):
        ctx.run(f"git checkout {commit}")


@task()
def zephyr_project_ci_setup(ctx):
    """Prepare Zephyr Projects for CI Tests

    Clones the repos specified in .ci-project-setup.json if and only
    if they don't already exist. This makes it easy to use pre-cached
    artifacts when running the job in CI
    """
    zephyr_ci_cfg_json = os.path.join(ZEPHYR_ROOT, ".ci-project-setup.json")
    with open(zephyr_ci_cfg_json, "rb") as json_file:
        data = json.load(json_file)
        for project, info in data.items():
            dest_dir = os.path.join(ZEPHYR_ROOT, "build", project)
            clone_dir = os.path.join(dest_dir, "zephyr")
            if not os.path.exists(clone_dir):
                _shallow_clone_and_checkout(
                    ctx, ZEPHYR_UPSTREAM_REPO, info["branch"], clone_dir, info["commit"]
                )
                with ctx.cd(dest_dir):
                    ctx.run("west init -l zephyr")
                    ctx.run("west update")
