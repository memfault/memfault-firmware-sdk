#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

"""
A script which can be used to convert the memfault-firmware-sdk into a CMSIS-Pack file which
can then be imported and added to a Keil MDK or ARM Development Studio Based Project
"""

import argparse
import datetime
import fnmatch
import glob
import logging
import os
import pathlib
import re
import shutil
import sys
import tempfile
import xml.etree.ElementTree as ET  # noqa: N817

PDSC_TEMPLATE = """<?xml version="1.0" encoding="utf-8"?>
<package schemaVersion="1.4" xmlns:xs="http://www.w3.org/2001/XMLSchema-instance" xs:noNamespaceSchemaLocation="PACK.xsd">
  <vendor>Memfault</vendor>
  <name>FirmwareSDK</name>
  <description>Memfault SDK</description>
  <url>https://github.com/memfault/memfault-firmware-sdk/</url>
  <supportContact>hello@memfault.com</supportContact>
  <releases>
    <release version="{SDK_VERSION}" date="{SDK_DATE}">Memfault CMSIS Pack based on Memfault Firmware SDK {SDK_VERSION}</release>
  </releases>
  <keywords>
    <keyword>Memfault</keyword>
    <keyword>Memfault Firmware SDK</keyword>
  </keywords>
  <components>
    <bundle Cbundle="Memfault" Cclass="Utility" Cversion="{SDK_VERSION}">
      <description></description>
      <doc>https://docs.memfault.com/</doc>
      <component Cgroup="Memfault" Cversion="{SDK_VERSION}">
        <description>Release Version "{SDK_VERSION}"</description>
        <files>
          <file category="include" name="components/include/"></file>
          <file category="include" name="ports/include/"></file>
        </files>
      </component>
      <!-- Template files end user is responsible for filling in -->
      <component Cgroup="Memfault Platform Port" Cversion="{SDK_VERSION}">
        <description>Release version {SDK_VERSION}</description>
        <files>
          <file attr="template" select="Metric keys config" category="other" name="ports/templates/memfault_metrics_heartbeat_config.def" version="1.0.0"></file>
          <file attr="template" select="SDK config" category="header" name="ports/templates/memfault_platform_config.h" version="1.0.0"></file>
          <file attr="template" select="Log config" category="header" name="ports/templates/memfault_platform_log_config.h" version="1.0.0"></file>
          <file attr="template" select="Platform port" category="source" name="ports/templates/memfault_platform_port.c" version="1.0.0"></file>
          <file attr="template" select="Trace reason keys config" category="other" name="ports/templates/memfault_trace_reason_user_config.def" version="1.0.0"></file>
        </files>
      </component>
    </bundle>
  </components>
</package>
"""


def get_file_element(file_name, common_prefix):
    relative_path = os.path.relpath(
        file_name,
        common_prefix,
    )
    # convert to posix style paths as required by CMSIS-Pack spec
    relative_path = pathlib.PureWindowsPath(relative_path).as_posix()
    logging.debug("Adding %s", relative_path)
    ele = ET.fromstring(  # noqa: S314
        """<file category="source" name="{PATH}"></file>""".format(PATH=relative_path)
    )
    ele.tail = "\n          "
    return ele


def recursive_glob_backport(dir_glob):
    # Find first directory wildcard and walk the tree from there
    glob_root = dir_glob.split("/*")[0]

    for base, _dirs, files in os.walk(glob_root):
        for file_name in files:
            file_path = os.path.join(base, file_name)

            # We use fnmatch to make sure the full glob matches the files
            # found from the recursive scan
            #
            # fnmatch expects unix style paths.
            file_path_unix = file_path.replace("\\", "/")

            if fnmatch.fnmatch(file_path_unix, dir_glob):
                yield file_path


def files_to_link(dir_glob, common_prefix):
    try:
        files = glob.glob(dir_glob, recursive=True)
    except TypeError:
        # Python < 3.5 do not support "recursive=True" arg for glob.glob
        files = recursive_glob_backport(dir_glob)
    files = recursive_glob_backport(dir_glob)
    for file_name in files:
        yield get_file_element(file_name, common_prefix)


def get_current_sdk_version(memfault_sdk_dir):
    with open(f"{memfault_sdk_dir}/components/include/memfault/version.h", "r") as f:
        contents = f.read()

    match = re.search(r".major = (\d+), .minor = (\d+), .patch = (\d+)", contents, re.MULTILINE)
    assert match
    return f"{match.group(1)}.{match.group(2)}.{match.group(3)}"


def build_cmsis_pack(
    memfault_sdk_dir,
    components,
    target_port,
    output_file,
    just_print,
):
    if not os.path.isdir(memfault_sdk_dir) or not os.path.isfile(
        "{}/VERSION".format(memfault_sdk_dir)
    ):
        raise Exception(
            "Invalid path to memfault-firmware-sdk (missing VERSION file) at {}".format(
                memfault_sdk_dir
            )
        )
    common_prefix = memfault_sdk_dir

    logging.debug("===Determined Path Information===")
    logging.debug("Memfault Firmware SDK Path: %s", memfault_sdk_dir)

    sdk_version = get_current_sdk_version(memfault_sdk_dir)

    tree = ET.ElementTree(
        ET.fromstring(  # noqa: S314
            PDSC_TEMPLATE.format(
                SDK_VERSION=sdk_version, SDK_DATE=datetime.date.today().strftime("%Y-%m-%d")
            )
        )
    )
    root = tree.getroot()

    # the pdsc file name must exactly match these fields
    psdc_vendor = root.find(".//vendor").text  # type: ignore[union-attr]
    psdc_name = root.find(".//name").text  # type: ignore[union-attr]
    psdc_file_name = f"{psdc_vendor}.{psdc_name}.pdsc"

    component_nodes = root.findall(".//component")
    memfault_component = None
    for component_node in component_nodes:
        cclass = component_node.get("Cgroup", "")
        if cclass == "Memfault":
            memfault_component = component_node

    assert memfault_component

    file_resources = memfault_component.findall(".//files")[0]

    for component in components:
        for ele in files_to_link(
            dir_glob="{}/components/{}/**/*.c".format(memfault_sdk_dir, component),
            common_prefix=common_prefix,
        ):
            file_resources.append(ele)

    if target_port is not None:
        port_folder_name = "_".join(os.path.split(target_port))
        port_folder_name = "memfault_{}".format(port_folder_name)

        for ele in files_to_link(
            dir_glob="{}/ports/{}/*.c".format(memfault_sdk_dir, target_port),
            common_prefix=common_prefix,
        ):
            file_resources.append(ele)

        # The DA1469x port also uses FreeRTOS so pick that up automatically when selected
        if target_port == "dialog/da1469x":
            for ele in files_to_link(
                dir_glob="{}/ports/freertos/**/*.c".format(memfault_sdk_dir),
                common_prefix=common_prefix,
            ):
                file_resources.append(ele)

    if just_print:
        # use sys.stdout.buffer to write raw bytes
        tree.write(sys.stdout.buffer, encoding="utf-8", xml_declaration=True)
        return

    with tempfile.TemporaryDirectory() as tmpdir_str:
        tree.write(f"{tmpdir_str}/{psdc_file_name}", encoding="utf-8", xml_declaration=True)

        # add all the components + ports files to the pack
        for directory in ["components", "ports"]:
            shutil.copytree(f"{memfault_sdk_dir}/{directory}", f"{tmpdir_str}/{directory}")

        shutil.make_archive("memfault", "zip", tmpdir_str)
        shutil.move("memfault.zip", output_file)


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="""
Generate a CMSIS-Pack distribution from the memfault-firmware-sdk, optionally
including a specific target port.

Example Usage:

$ python cmsis_pack_bundle.py --memfault-sdk-dir /path/to/memfault-firmware-sdk
""",
    )
    parser.add_argument(
        "-m",
        "--memfault-sdk-dir",
        help="The directory memfault-firmware-sdk was copied to",
    )
    parser.add_argument(
        "--target-port", help="An optional port to include in the pack, i.e dialog/da145xx"
    )

    parser.add_argument(
        "-c",
        "--components",
        help="The components to include in the pack.",
        default="core,demo,util,metrics,panics",
    )

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--output-file",
        help="The output file to write the pack to",
        default="memfault_firmware_sdk.pack",
    )

    group.add_argument(
        "--print-pdsc",
        default=False,
        action="store_true",
        help=(
            "Print the generated pdsc file to stdout, instead of generating the pack. Useful for"
            " debugging."
        ),
    )

    parser.add_argument(
        "--verbose",
        default=False,
        action="store_true",
        help="enable verbose logging for debug",
    )

    args = parser.parse_args()

    memfault_sdk_dir = args.memfault_sdk_dir
    if memfault_sdk_dir is None:
        memfault_sdk_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

    memfault_sdk_dir = os.path.realpath(memfault_sdk_dir)
    components = args.components.split(",")

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    build_cmsis_pack(
        memfault_sdk_dir=memfault_sdk_dir,
        components=components,
        target_port=args.target_port,
        output_file=args.output_file,
        just_print=args.print_pdsc,
    )
