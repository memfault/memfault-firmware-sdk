#!/usr/bin/env bash

# This script can be used (on a Linux host, at least) to set up a docker
# container to be able to write on a hosted volume, by using an overlay mount
# inside the container.
#
# It requires running docker with the --priviledged flag. An example:
#
# ❯docker run --privileged --rm -i -t -v"$PWD":/hostdir memfault/memfault-firmware-sdk-ci /hostdir/.circleci/runas.sh

set -euo pipefail

BASE_DIR=/hostdir
FINAL_DIR=/memfault-firmware-sdk
USER=${USER-$(whoami)}

mkdir -p /tmp/overlay/
sudo mount -t tmpfs tmpfs /tmp/overlay
mkdir /tmp/overlay/{up,work}
sudo mkdir "$FINAL_DIR"
sudo mount -t overlay overlay \
  -o lowerdir="$BASE_DIR,upperdir=/tmp/overlay/up/,workdir=/tmp/overlay/work/" \
  "$FINAL_DIR"
sudo chown -R "$USER:$USER $FINAL_DIR"

exec bash
