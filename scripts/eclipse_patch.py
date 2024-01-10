#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

"""
A script which can be used to add the memfault-firmware-sdk to a project using an Eclipse-based IDE
"""

import argparse
import fnmatch
import glob
import logging
import os
import re
import xml.etree.ElementTree as ET  # noqa: N817


def get_depth_from_parent(project_dir, memfault_dir):
    common_prefix = os.path.commonprefix([memfault_dir, project_dir])
    depth = 1
    dirname = project_dir

    # some projects are in the root of the project dir- if the memfault dir is
    # in the same directory, return a PROJECT_LOC value of 0 for the link
    # position
    if dirname == common_prefix:
        return dirname, 0

    # for the normal case, walk the directory parents until we find the common
    # parent for the project and memfault dirs
    while True:
        parent_dir = os.path.dirname(dirname)
        if os.path.samefile(parent_dir, common_prefix):
            return common_prefix, depth
        elif parent_dir == dirname:
            raise RuntimeError(
                "Couldn't compute depth, aborting at directory {}".format(parent_dir)
            )
        depth += 1
        dirname = parent_dir


def generate_link_element(name, path, path_type="1"):
    ele = ET.fromstring(  # noqa: S314
        """
\t<link>
\t\t<name>{NAME}</name>
\t\t<type>{PATH_TYPE}</type>
\t\t<locationURI>{PATH}</locationURI>
\t</link>
""".format(NAME=name, PATH=path, PATH_TYPE=path_type)
    )
    ele.tail = "\n\t"
    return ele


def generate_linked_resources():
    ele = ET.fromstring(  # noqa: S314
        """
\t<linkedResources>
</linkedResources>
"""
    )
    ele.tail = "\n\t"
    return ele


def get_file_element(file_name, virtual_dir, common_prefix, parent_dir, path_type="1"):
    name = "{}/{}".format(virtual_dir, os.path.basename(file_name))

    relative_path = os.path.relpath(
        file_name,
        common_prefix,
    )

    # Note: We replace '\' with '/' because eclipse on windows expects '/' for paths
    path = os.path.join(parent_dir, relative_path).replace("\\", "/")
    logging.debug("Adding %s", name)
    return generate_link_element(name, path, path_type=path_type)


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


def files_to_link(dir_glob, virtual_dir, common_prefix, parent_dir):
    try:
        files = glob.glob(dir_glob, recursive=True)
    except TypeError:
        # Python < 3.5 do not support "recursive=True" arg for glob.glob
        files = recursive_glob_backport(dir_glob)
    files = recursive_glob_backport(dir_glob)
    for file_name in files:
        # Note:
        #  - xtensa targets (i.e ESP) use CMake/Make so no need to add to eclipse based projects
        #  - skip adding "memfault_demo_http" from demo component
        if "xtensa" in file_name or ("http" in os.path.relpath(file_name, start=common_prefix)):
            continue
        logging.debug("Adding %s", file_name)

        yield get_file_element(file_name, virtual_dir, common_prefix, parent_dir)


def patch_project(
    project_dir,
    memfault_sdk_dir,
    components,
    location_prefix=None,
    target_port=None,
    output_dir=None,
):
    project_file = "{}/.project".format(project_dir)

    if not os.path.isfile(project_file):
        raise RuntimeError("Could not location project file at {}".format(project_file))

    if not os.path.isdir(memfault_sdk_dir) or not os.path.isfile(
        "{}/VERSION".format(memfault_sdk_dir)
    ):
        raise RuntimeError("Could not locate memfault-firmware-sdk at {}".format(memfault_sdk_dir))

    if location_prefix is None:
        # No prefix was given so paths will be generated relative to project root
        common_prefix, depth = get_depth_from_parent(project_dir, memfault_sdk_dir)
        parent_dir = "PARENT-{}-PROJECT_LOC".format(depth)
    else:
        common_prefix = memfault_sdk_dir
        relative_path = os.path.relpath(
            memfault_sdk_dir,
            os.path.commonprefix([memfault_sdk_dir, location_prefix[1]]),
        )
        parent_dir = os.path.join(location_prefix[0], relative_path)

    logging.debug("===Determined Path Information===")
    logging.debug("Project Path:               %s", project_dir)
    logging.debug("Memfault Firmware SDK Path: %s", memfault_sdk_dir)
    logging.debug("Eclipse Memfault Root:      %s", parent_dir)

    tree = ET.parse(project_file)  # noqa: S314
    root = tree.getroot()

    linked_resources_roots = root.findall(".//linkedResources")
    if len(linked_resources_roots) == 0:
        linked_resources = generate_linked_resources()
        root.append(linked_resources)
    elif len(linked_resources_roots) == 1:
        linked_resources = linked_resources_roots[0]
    else:
        raise RuntimeError(
            "Located {} linked resources in Eclipse project file but expected 1".format(
                len(linked_resources_roots)
            )
        )

    # We want this script to be idempotent so remove any "memfault_" sources already
    # added. We will just be adding them back below.
    for link in linked_resources.findall("link"):
        name = link.find(".//name")
        if name is not None and "memfault_" in name.text:  # pyright: ignore[reportGeneralTypeIssues]
            linked_resources.remove(link)

    comp_folder_name = "memfault_components"

    linked_resources.append(
        generate_link_element(comp_folder_name, "virtual:/virtual", path_type="2")
    )

    for component in components:
        for ele in files_to_link(
            dir_glob="{}/components/{}/**/*.c".format(memfault_sdk_dir, component),
            virtual_dir=comp_folder_name,
            common_prefix=common_prefix,
            parent_dir=parent_dir,
        ):
            linked_resources.append(ele)

    include_folder_name = "memfault_includes"
    linked_resources.append(
        generate_link_element(include_folder_name, "virtual:/virtual", path_type="2")
    )
    for inc_name in ["components", "ports"]:
        inc_path = os.path.join(memfault_sdk_dir, inc_name, "include")
        ele = get_file_element(
            file_name=inc_path,
            virtual_dir=os.path.join(include_folder_name, inc_name),
            common_prefix=common_prefix,
            parent_dir=parent_dir,
            path_type="2",
        )
        linked_resources.append(ele)

    if target_port is not None:
        port_folder_name = "_".join(os.path.split(target_port))
        port_folder_name = "memfault_{}".format(port_folder_name)

        linked_resources.append(
            generate_link_element(port_folder_name, "virtual:/virtual", path_type="2")
        )

        for ele in files_to_link(
            dir_glob="{}/ports/{}/*.c".format(memfault_sdk_dir, target_port),
            virtual_dir=port_folder_name,
            common_prefix=common_prefix,
            parent_dir=parent_dir,
        ):
            linked_resources.append(ele)

        # The DA1469x port also uses FreeRTOS so pick that up automatically when selected
        if target_port == "dialog/da1469x":
            for ele in files_to_link(
                dir_glob="{}/ports/freertos/**/*.c".format(memfault_sdk_dir),
                virtual_dir=port_folder_name,
                common_prefix=common_prefix,
                parent_dir=parent_dir,
            ):
                linked_resources.append(ele)

    output_location = project_file if output_dir is None else os.path.join(output_dir, ".project")
    logging.info("Writing result to %s", output_location)
    tree.write(output_location)


def patch_cproject(
    project_dir,
    output_dir=None,
):
    cproject_file = "{}/.cproject".format(project_dir)

    if not os.path.isfile(cproject_file):
        raise RuntimeError("Could not location project file at {}".format(cproject_file))

    tree = ET.parse(cproject_file)  # noqa: S314
    root = tree.getroot()

    with open(cproject_file) as cproject_xml:
        data = cproject_xml.read()

    # ElementTree parser doesn't preserve XML processing instructions.
    #
    # Since .cproject files relies on a few, we will grab them now so we can re-insert
    # them after patching the file
    match = re.search(r"(.*)<\s*{}".format(root.tag), data, re.DOTALL)
    processing_instruction_text = match.group(1) if match else ""

    options = root.findall(".//option")

    #
    # Add required Memfault include paths into build for all build configurations:
    #   ${MEMFAULT_FIRMWARE_SDK}/components/include
    #   ${MEMFAULT_FIRMWARE_SDK}/ports/include
    #

    def _find_include_nodes(option):
        return option.get("id", "").startswith((
            # this is the element id used by Dialog's Smart Snippets Studio
            # IDE (and possibly others)
            "ilg.gnuarmeclipse.managedbuild.cross.option.c.compiler.include.paths",
            # this is the element id used by NXP's MCUXpresso IDE
            "gnu.c.compiler.option.include.paths",
        ))

    memfault_sdk_include_paths = [
        "${workspace_loc:/${ProjName}/memfault_includes/components/include}",
        "${workspace_loc:/${ProjName}/memfault_includes/ports/include}",
    ]

    include_options = filter(_find_include_nodes, options)
    for include_option in include_options:
        list_option_values = include_option.findall(".//listOptionValue")
        tail = list_option_values[0].tail

        for include_path in list_option_values:
            path = include_path.get("value", "")
            if "memfault_includes" in path:
                include_option.remove(include_path)

        for path in memfault_sdk_include_paths:
            ele = ET.Element("listOptionValue", builtin="false", value='"{}"'.format(path))
            ele.tail = tail
            include_option.append(ele)

    #
    # Add GNU build id generation for all build configurations:
    #

    def _find_linker_flags(option):
        return option.get("id", "").startswith(
            "ilg.gnuarmeclipse.managedbuild.cross.option.c.linker.other"
        ) and option.get("name", "").startswith("Other linker flags")

    ld_flag_options = filter(_find_linker_flags, options)
    for ld_flag_option in ld_flag_options:
        value = ld_flag_option.get("value", "")
        if "-Wl,--build-id" not in value:
            ld_flag_option.set("value", value + " -Wl,--build-id")

    #
    # Overwrite original .cproject file with updates and pull back in processing instruction text
    # which was exctracted earlier
    #

    output_location = cproject_file if output_dir is None else os.path.join(output_dir, ".cproject")
    logging.info("Writing result to %s", output_location)
    tree.write(output_location)

    with open(output_location, "r") as out_f:
        new_contents = out_f.read()

    with open(output_location, "w") as out_f:
        out_f.write(processing_instruction_text)
        out_f.write(new_contents)


if __name__ == "__main__":
    logging.basicConfig(
        format="%(asctime)s,%(msecs)d %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s",
        datefmt="%Y-%m-%d:%H:%M:%S",
        level=logging.INFO,
    )
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="""
Patches an Eclipse .project file to include sources for memfault-firmware-sdk.

Example Usage:

# cd into directory with .project file
$ python eclipse_patch.py --project-dir . --memfault-sdk-dir /path/to/memfault-firmware-sdk
""",
    )
    parser.add_argument(
        "-p",
        "--project-dir",
        required=True,
        help="The directory with the Eclipse .project to update",
    )
    parser.add_argument(
        "-m",
        "--memfault-sdk-dir",
        required=True,
        help="The directory memfault-firmware-sdk was copied to",
    )
    parser.add_argument(
        "--target-port", help="The port to pick up for a project, i.e dialog/da145xx"
    )

    parser.add_argument(
        "-l",
        "--location-prefix",
        help=(
            "The default behavior will add memfault-firmware-sdk files to the eclipse project using"
            " paths relative to the project root. This can be used to control the root used instead"
        ),
    )

    parser.add_argument(
        "-c",
        "--components",
        help="The components to include in an eclipse project.",
        default="core,util,metrics,panics",
    )

    parser.add_argument(
        "--output",
        help=(
            "The directory to output result to. By default, the .project/.cproject files for the"
            " project will be overwritten"
        ),
    )
    parser.add_argument(
        "--verbose",
        default=False,
        action="store_true",
        help="enable verbose logging for debug",
    )

    args = parser.parse_args()

    project_dir = os.path.realpath(args.project_dir)
    memfault_sdk_dir = os.path.realpath(args.memfault_sdk_dir)
    components = args.components.split(",")

    if args.output and not os.path.isdir(args.output):
        raise RuntimeError("Output directory does not exist: {}".format(args.output))

    if args.location_prefix:
        location_prefix = args.location_prefix.split("=")
        if len(location_prefix) != 2:
            raise RuntimeError("Location Prefix must be of form 'VAR=/path/'")
    else:
        location_prefix = None

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    patch_project(
        project_dir=project_dir,
        memfault_sdk_dir=memfault_sdk_dir,
        components=components,
        location_prefix=location_prefix,
        target_port=args.target_port,
        output_dir=args.output,
    )

    patch_cproject(
        project_dir=project_dir,
        output_dir=args.output,
    )

    logging.info(
        "Hurray, .project & .cproject have been succesfully patched! Be sure to 'Refresh' project"
        " to synchronize changes!"
    )
