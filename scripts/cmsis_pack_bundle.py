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
import tempfile
import xml.etree.ElementTree as ET  # noqa: N817

import lxml.etree
import requests

# For testing, it can be useful to point the tool to another github repo
GITHUB_REPO = os.getenv("GITHUB_REPO", "memfault/memfault-firmware-sdk")

PDSC_TEMPLATE = """<?xml version="1.0" encoding="utf-8"?>
<package schemaVersion="1.7.27" xmlns:xs="http://www.w3.org/2001/XMLSchema-instance" xs:noNamespaceSchemaLocation="https://raw.githubusercontent.com/Open-CMSIS-Pack/Open-CMSIS-Pack-Spec/v1.7.28/schema/PACK.xsd">
  <vendor>Memfault</vendor>
  <name>FirmwareSDK</name>
  <description>Memfault SDK</description>
  <url>https://github.com/{GITHUB_REPO}/releases/download/{SDK_VERSION}/</url>
  <supportContact>hello@memfault.com</supportContact>
  <repository type="git">https://github.com/{GITHUB_REPO}.git</repository>
  <changelogs>
    <changelog id="all" default="true" name="CHANGELOG.md"/>
  </changelogs>
  <license>License.txt</license>
  <licenseSets>
    <licenseSet id="all" default="true" gating="true">
      <license name="License.txt" title="Amended BSD-3 Clause License for the SDK files"/>
      <license name="ports/templates/apache-2.0.txt" title="Apache 2.0 for template files" spdx="Apache-2.0"/>
    </licenseSet>
  </licenseSets>
  <releases></releases>
  <keywords>
    <keyword>Memfault</keyword>
    <keyword>Memfault Firmware SDK</keyword>
  </keywords>
  <components>
    <bundle Cbundle="Memfault" Cclass="Utility" Cversion="{SDK_VERSION}">
      <description>Memfault Firmware SDK</description>
      <doc>https://docs.memfault.com/</doc>
      <component Cgroup="Memfault Core">
        <description>Memfault core components</description>
        <files>
          <file category="include" name="components/include/"></file>
          <file category="include" name="ports/include/"></file>
        </files>
      </component>
      <component Cgroup="Memfault Coredump RAM Backend">
        <description>RAM backed coredump implementation</description>
        <files>
          <file category="sourceC" name="ports/panics/src/memfault_platform_ram_backed_coredump.c"></file>
        </files>
      </component>
      <!-- Template files end user is responsible for filling in -->
      <component Cgroup="Memfault Platform Port">
        <description>Template files for Memfault Platform implementation</description>
        <files>
          <file attr="template" select="Platform Code" category="other" name="ports/templates/memfault_metrics_heartbeat_config.def"></file>
          <file attr="template" select="Platform Code" category="header" name="ports/templates/memfault_platform_config.h"></file>
          <file attr="template" select="Platform Code" category="header" name="ports/templates/memfault_platform_log_config.h"></file>
          <file attr="template" select="Platform Code" category="sourceC" name="ports/templates/memfault_platform_port.c"></file>
          <file attr="template" select="Platform Code" category="other" name="ports/templates/memfault_trace_reason_user_config.def"></file>
        </files>
      </component>
    </bundle>
  </components>
</package>
"""

PIDX_TEMPLATE = """<?xml version='1.0' encoding='UTF-8'?>
<index xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" schemaVersion="1.1.1" xsi:noNamespaceSchemaLocation="https://raw.githubusercontent.com/Open-CMSIS-Pack/Open-CMSIS-Pack-Spec/v1.7.28/schema/PackIndex.xsd">
    <vendor>Memfault</vendor>
    <url>https://github.com/{GITHUB_REPO}/releases/latest/download/</url>
    <timestamp>{TIMESTAMP}</timestamp>
    <pindex>
    </pindex>
</index>
"""


def get_file_element(file_name, common_prefix, sdk_version):
    relative_path = os.path.relpath(
        file_name,
        common_prefix,
    )
    # convert to posix style paths as required by CMSIS-Pack spec
    relative_path = pathlib.PureWindowsPath(relative_path).as_posix()
    logging.debug("Adding %s", relative_path)
    ele = ET.fromstring(  # noqa: S314
        """<file category="sourceC" name="{PATH}"></file>""".format(
            PATH=relative_path,
        )
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


def files_to_link(dir_glob, common_prefix, sdk_version):
    try:
        files = glob.glob(dir_glob, recursive=True)
    except TypeError:
        # Python < 3.5 do not support "recursive=True" arg for glob.glob
        files = recursive_glob_backport(dir_glob)
    files = recursive_glob_backport(dir_glob)
    for file_name in files:
        yield get_file_element(file_name, common_prefix, sdk_version)


def get_latest_pdsc():
    """Retrieve the contents of the latest pdsc file"""
    # Optionally override the release to fetch
    if release := os.getenv("CMSIS_SDK_BASE_RELEASE"):
        url = f"https://github.com/{GITHUB_REPO}/releases/download/{release}/Memfault.FirmwareSDK.pdsc"
    else:
        url = f"https://github.com/{GITHUB_REPO}/releases/latest/download/Memfault.FirmwareSDK.pdsc"
    logging.debug("Fetching %s", url)
    resp = requests.get(url)

    # There should always be a pidx/pdsc file available. Only skip if the env
    # var is set and it's a 404, which is only useful before the first release.
    if resp.status_code == 404 or os.getenv("SKIP_MISSING_PRIOR_RELEASE"):
        logging.debug("Skipping missing file")
        return None

    resp.raise_for_status()
    return resp.text


def lxml_reindent_tree(root):
    """Using lxml, re-indent the tree to produce consistent output"""
    parser = lxml.etree.XMLParser(remove_blank_text=True)
    tree = lxml.etree.fromstring(ET.tostring(root, encoding="utf-8"), parser=parser)  # noqa: S320
    lxml.etree.indent(tree, space="    ")
    return lxml.etree.ElementTree(tree)


def write_pidx_file(sdk_version):
    utcnow = datetime.datetime.now().isoformat()  # noqa: DTZ005
    pidx_tree = ET.ElementTree(
        ET.fromstring(PIDX_TEMPLATE.format(GITHUB_REPO=GITHUB_REPO, TIMESTAMP=utcnow))  # noqa: S314
    )

    root = pidx_tree.getroot()
    pindex_node = root.find("./pindex")
    assert pindex_node is not None

    # if there's an existing name="FirmwareSDK" node, remove it before inserting
    # the new one
    for node in pindex_node.findall("./pdsc[@name='FirmwareSDK']"):
        pindex_node.remove(node)

    # unlike the pdsc file, we always overwrite the <pdsc> node in the pidx with
    # the latest version
    release_node = ET.fromstring(  # noqa: S314
        f'<pdsc url="https://github.com/{GITHUB_REPO}/releases/latest/download/"'
        f' vendor="Memfault" name="FirmwareSDK" version="{sdk_version}"/>'
    )
    pindex_node.append(release_node)

    # use lxml to re-indent the tree
    pidx_tree = lxml_reindent_tree(root)
    # and write it to Memfault.pidx
    pidx_tree.write("Memfault.pidx", encoding="utf-8", xml_declaration=True)


def build_cmsis_pack(
    memfault_sdk_dir,
    components,
    target_port,
    pack_only,
):
    common_prefix = memfault_sdk_dir

    if not os.path.isdir(memfault_sdk_dir) or not os.path.isfile(
        "{}/VERSION".format(memfault_sdk_dir)
    ):
        raise ValueError(
            "Invalid path to memfault-firmware-sdk (missing VERSION file) at {}".format(
                memfault_sdk_dir
            )
        )

    logging.debug("===Determined Path Information===")
    logging.debug("Memfault Firmware SDK Path: %s", memfault_sdk_dir)

    if sdk_version := os.getenv("SDK_VERSION"):
        logging.debug("Using SDK_VERSION env var: %s", sdk_version)
    else:
        # read the version from the memfault-firmware-sdk
        with open(f"{memfault_sdk_dir}/components/include/memfault/version.h", "r") as f:
            contents = f.read()

        match = re.search(r".major = (\d+), .minor = (\d+), .patch = (\d+)", contents, re.MULTILINE)
        assert match
        sdk_version = f"{match.group(1)}.{match.group(2)}.{match.group(3)}"
    # get the current data as YYYY-MM-DD. normally we'd use UTC, but the packchk
    # tool has a built-in check that the date is not in the future, and the
    # reference date it uses is locale time, so we need to use that.
    sdk_date = datetime.datetime.now().strftime("%Y-%m-%d")  # noqa: DTZ005
    tree = ET.ElementTree(
        ET.fromstring(  # noqa: S314
            PDSC_TEMPLATE.format(
                GITHUB_REPO=GITHUB_REPO,
                SDK_VERSION=sdk_version,
                SDK_DATE=sdk_date,
            )
        )
    )
    root = tree.getroot()

    # the pdsc file name must exactly match these fields
    psdc_vendor = root.find(".//vendor").text  # pyright: ignore[reportOptionalMemberAccess]
    psdc_name = root.find(".//name").text  # pyright: ignore[reportOptionalMemberAccess]
    psdc_file_name = f"{psdc_vendor}.{psdc_name}.pdsc"

    component_nodes = root.findall(".//component")
    memfault_component = None
    for component_node in component_nodes:
        cgroup = component_node.get("Cgroup", "")
        if cgroup == "Memfault Core":
            memfault_component = component_node

    assert memfault_component

    file_resources = memfault_component.findall(".//files")[0]

    for component in components:
        for ele in files_to_link(
            dir_glob="{}/components/{}/**/*.c".format(memfault_sdk_dir, component),
            common_prefix=common_prefix,
            sdk_version=sdk_version,
        ):
            file_resources.append(ele)

    if target_port is not None:
        port_folder_name = "_".join(os.path.split(target_port))
        port_folder_name = "memfault_{}".format(port_folder_name)

        for ele in files_to_link(
            dir_glob="{}/ports/{}/*.c".format(memfault_sdk_dir, target_port),
            common_prefix=common_prefix,
            sdk_version=sdk_version,
        ):
            file_resources.append(ele)

        # The DA1469x port also uses FreeRTOS so pick that up automatically when selected
        if target_port == "dialog/da1469x":
            for ele in files_to_link(
                dir_glob="{}/ports/freertos/**/*.c".format(memfault_sdk_dir),
                common_prefix=common_prefix,
                sdk_version=sdk_version,
            ):
                file_resources.append(ele)

    # Fetch the latest PDSC file from the Memfault Firmware SDK github release,
    # and use it to populate the release node in the pdsc file.
    # First insert the new release node to the start of the list
    url = f"https://github.com/{GITHUB_REPO}/releases/download/{sdk_version}/Memfault.FirmwareSDK.{sdk_version}.pack"
    release_node = ET.fromstring(  # noqa: S314
        f'<release version="{sdk_version}" date="{sdk_date}" url="{url}"'
        f' tag="{sdk_version}">Memfault CMSIS Pack for Memfault Firmware SDK {sdk_version}. See'
        " release notes at"
        f" https://github.com/memfault/memfault-firmware-sdk/releases/tag/{sdk_version}</release>"
    )
    root_releases_node = root.find("./releases")
    assert root_releases_node is not None
    root_releases_node.append(release_node)
    if pdsc_text := get_latest_pdsc():
        logging.debug("Loading releases from previous pdsc file")
        pdsc = ET.fromstring(pdsc_text)  # noqa: S314
        # get the package/releases node
        releases_node = pdsc.find("./releases")
        assert releases_node is not None

        # and copy the fetched previous releases children to the new release node
        for child in releases_node:
            root_releases_node.append(child)

    # use lxml to re-indent the tree
    pdsc_tree = lxml_reindent_tree(root)

    # Set the output file name based on the psdc values. See
    # https://open-cmsis-pack.github.io/Open-CMSIS-Pack-Spec/main/html/createPackPublish.html#cp_WebDownload
    # for details.
    pack_output_file = f"{psdc_vendor}.{psdc_name}.{sdk_version}.pack"

    with tempfile.TemporaryDirectory() as tmpdir_str:
        pdsc_tree.write(f"{tmpdir_str}/{psdc_file_name}", encoding="utf-8", xml_declaration=True)
        # add all the components + ports files to the pack
        for directory in ["components", "ports"]:
            shutil.copytree(f"{memfault_sdk_dir}/{directory}", f"{tmpdir_str}/{directory}")

        aux_files = [
            "CHANGELOG.md",
            "License.txt",
            "README.md",
        ]
        for aux_file in aux_files:
            shutil.copy(f"{memfault_sdk_dir}/{aux_file}", f"{tmpdir_str}/{aux_file}")

        shutil.make_archive("memfault", "zip", tmpdir_str)
        shutil.move("memfault.zip", pack_output_file)

    if pack_only:
        return

    # write the .pdsc file
    pdsc_tree.write(f"{psdc_vendor}.{psdc_name}.pdsc", encoding="utf-8", xml_declaration=True)

    # write the .pidx file
    write_pidx_file(sdk_version)


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

    parser.add_argument(
        "--pack-only",
        action="store_true",
        help="Only generate the .pack file, don't output the .pdsc and .pidx files",
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
        pack_only=args.pack_only,
    )
