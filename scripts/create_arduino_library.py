#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

#!/usr/bin/env python

"""
A script which transforms the memfault-firmware-sdk into a project suitable for inclusion as an arduino library

For more details about the arduino library see:
  https://arduino.github.io/arduino-cli/0.20/library-specification/

The main transformations this script makes:
 - Removes .c & .cpp files that should not be included in an arduino build. (The arduino build system
   will by default recursively search and compile all .c/.cpp files in the library folder)
 - Patches all include paths to be relative to the root of the library. (There is no way to add
   additional include paths in an arduino library)

Example Usage:
  $ python create_arduino_library.py --tag 0.28.2 --output build
"""

import argparse
import glob
import logging
import os
import re
import shutil
import tarfile
import urllib.request
from urllib.error import HTTPError

ROOT_DIRECTORY = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
BUILD_DIRECTORY = os.path.join(ROOT_DIRECTORY, "build")

MEMFAULT_RELEASE_PATH = "https://github.com/memfault/memfault-firmware-sdk/archive/refs/tags"


def download_memfault_library(working_dir: str, tag: str):
    release_artifact_filename = f"{tag}.tar.gz"
    release_artifact_filepath = os.path.join(working_dir, release_artifact_filename)
    logging.debug("Downloading %s ...", release_artifact_filename)
    try:
        urllib.request.urlretrieve(  # noqa: S310
            f"{MEMFAULT_RELEASE_PATH}/{release_artifact_filename}", release_artifact_filepath
        )
    except HTTPError as err:
        if err.code != 404:
            raise
        logging.error("No memfault-firmware-sdk with tag %s exists", tag)
        return None

    return release_artifact_filepath


def extract_memfault_library(working_dir: str, release_artifact_filepath: str):
    logging.debug("Extracting %s", release_artifact_filepath)
    download = tarfile.open(release_artifact_filepath)
    download.extractall(working_dir)  # noqa: S202
    root_folder_name = download.getnames()[0]
    download.close()
    extract_dir = os.path.realpath(os.path.join(working_dir, root_folder_name))
    logging.debug("Data extracted to %s", extract_dir)
    return extract_dir


def arduinoify_memfault_sdk(sdk_root_dir: str, result_dir: str, port: str):
    if not os.path.exists(sdk_root_dir):
        raise FileNotFoundError(f"Memfault SDK directory does not exist: {sdk_root_dir}")

    # We will be re-packag'ing the SDK into a library with the following structure:
    #
    # |-- LICENSE
    # |-- README.md (sourced from port_dir if provided)
    # |-- examples (sourced from port_dir if provided)
    # |
    # |-- library.properties (sourced from port_dir if provided)
    # |-- src
    #    |-- memfault-firmware-sdk
    #    |--
    library_dir = os.path.join(result_dir)

    if port:
        port_dir = os.path.join(sdk_root_dir, "ports", port)
        if not os.path.exists(port_dir):
            logging.error("No port found for '%s' at %s", port, port_dir)
        else:
            logging.debug("Copying port from %s into %s", port_dir, library_dir)
            shutil.copytree(port_dir, library_dir)

    # After directory copy because it gets upset if file exists
    os.makedirs(library_dir, exist_ok=True)

    # Keep list for "ports" directory
    keep_list = ("panics",)
    port_root = os.path.join(sdk_root_dir, "ports")
    for f in os.listdir(port_root):
        if f in keep_list:
            continue
        path_to_rm = os.path.join(port_root, f)
        if not os.path.isdir(path_to_rm):
            continue
        logging.debug("Removing %s from memfault-firmware-sdk", path_to_rm)
        shutil.rmtree(path_to_rm)

    # Remove directories that aren't needed / contain .c / .cpp files we
    # do not want to compile
    remove_list = (".circleci", "examples", "tests")
    for directory in remove_list:
        path_to_rm = os.path.join(sdk_root_dir, directory)
        logging.debug("Removing %s from memfault-firmware-sdk", path_to_rm)
        shutil.rmtree(path_to_rm)

    files_to_patch = glob.glob(f"{sdk_root_dir}/**/*.c", recursive=True)
    files_to_patch.extend(glob.glob(f"{sdk_root_dir}/**/*.h", recursive=True))
    # also the memfault/metrics/heartbeat_config.def file
    files_to_patch.extend(
        glob.glob(f"{sdk_root_dir}/**/include/memfault/metrics/heartbeat_config.def")
    )

    logging.debug("Patching headers ...")

    # We need to swap out include paths to be relative to root of repo
    # since there's no way to add additional include paths in an arduino lib
    header_regex = re.compile(r'#include\s"memfault/')
    for f in files_to_patch:
        with open(f) as r:
            contents = r.read()
            contents = header_regex.sub(
                '#include "memfault-firmware-sdk/components/include/memfault/', contents
            )

        with open(f, "w") as w:
            w.write(contents)

    # Up level a few files to the root of the repo
    shutil.move(os.path.join(sdk_root_dir, "License.txt"), os.path.join(library_dir, "LICENSE.txt"))
    shutil.move(os.path.join(sdk_root_dir, "VERSION"), os.path.join(library_dir, "VERSION"))
    shutil.move(
        os.path.join(sdk_root_dir, "CHANGELOG.md"), os.path.join(library_dir, "CHANGELOG.md")
    )
    shutil.move(sdk_root_dir, os.path.join(library_dir, "src", "memfault-firmware-sdk"))

    logging.debug("Patched SDK Generation Success! %s", library_dir)
    return library_dir


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="""
Transforms the memfault-firmware-sdk into a project suitable for inclusion in an arduino library.

Example Usage:

# cd into directory with .project file
$ python create_arduino_library.py --tag 0.28.2 --output build
""",
    )
    parser.add_argument(
        "-t",
        "--tag",
        required=False,
        help='The release from https://github.com/memfault/memfault-firmware-sdk/releases to convert to an arduino library (i.e "0.28.2")',
    )
    parser.add_argument(
        "-m",
        "--memfault-sdk",
        required=False,
        help="Path to .tar.gz of the memfault-firmware-sdk to convert to an arduino library",
    )
    parser.add_argument(
        "-o",
        "--output",
        help="The directory to output result to. By default a build/ directory relative to script location",
        default=os.path.join(BUILD_DIRECTORY, "arduino-library"),
    )

    parser.add_argument(
        "-p",
        "--port",
        help="Additional porting layer to include in the arduino library",
        default=None,
    )

    parser.add_argument(
        "-v",
        "--verbose",
        default=False,
        action="store_true",
        help="enable verbose logging for debug",
    )

    args = parser.parse_args()

    if args.tag and args.memfault_sdk:
        raise ValueError("--tag and --memfault-sdk can not be used together")

    if not args.tag and not args.memfault_sdk:
        raise ValueError("Either --tag or --memfault-sdk must be specified")

    # Create output directory if it does not exist
    os.makedirs(BUILD_DIRECTORY, exist_ok=True)

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    if args.tag:
        release_artifact_filepath = download_memfault_library(BUILD_DIRECTORY, args.tag)
        extraction_working_dir = BUILD_DIRECTORY
    else:
        release_artifact_filepath = args.memfault_sdk
        extraction_working_dir = os.path.join(BUILD_DIRECTORY, "memfault-firmware-sdk")

    assert release_artifact_filepath
    release_artifact_dir = extract_memfault_library(
        extraction_working_dir, release_artifact_filepath
    )

    result_dir = arduinoify_memfault_sdk(release_artifact_dir, args.output, args.port)

    logging.info("Hurray, library created at %s", result_dir)
