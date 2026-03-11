#
# Copyright (c) Memfault, Inc.
# See LICENSE for details
#

"""
smpmgr Memfault group (128) commands.

Example usage (note: plugin-path must be a directory containing this file):

$ uvx smpmgr --ble xx:xx:xx:xx:xx:xx --plugin-path=. memfault device-info
⠇ Connecting to xx:xx:xx:xx:xx:xx... OK
DeviceInfoReadResponse(
    header=Header(op=<OP.READ_RSP: 1>, version=<Version.V2: 1>, flags=<Flag.UNUSED: 0>, length=108, group_id=128, sequence=1, command_id=0),
    version=<Version.V2: 1>,
    sequence=1,
    smp_data=b'\t\x00\x00l\x00\x80\x01\x00\xbfmdevice_serialp65071B2E77FB7451phardware_versionjnrf54l15dkmsoftware_typecappocurrent_versionl0.0.1+d07bd2\xff',
    device_serial='abcdef',
    hardware_version='nrf54l15dk',
    software_type='app',
    current_version='0.0.1+d07bd2'
)

$ uvx smpmgr --ble xx:xx:xx:xx:xx:xx --plugin-path=. memfault project-key
⠋ Connecting to xx:xx:xx:xx:xx:xx... OK
ProjectKeyReadResponse(
    header=Header(op=<OP.READ_RSP: 1>, version=<Version.V2: 1>, flags=<Flag.UNUSED: 0>, length=24, group_id=128, sequence=1, command_id=1),
    version=<Version.V2: 1>,
    sequence=1,
    smp_data=b'\t\x00\x00\x18\x00\x80\x01\x01\xbfkproject_keyidummy-key\xff',
    project_key='dummy-key'
)
"""

import asyncio
import logging
from enum import IntEnum, unique
from typing import cast

import rich
import smp.error as smperr
import smp.message as smpmsg
import typer
from smpmgr.common import Options, connect_with_spinner, get_smpclient

app = typer.Typer(name="memfault", help="Memfault MCUmgr group 128")
logger = logging.getLogger(__name__)


@unique
class MEMFAULT_RET_RC(IntEnum):  # noqa: N801
    OK = 0
    UNKNOWN = 1
    NO_PROJECT_KEY = 2


class MemfaultErrorV1(smperr.ErrorV1):
    _GROUP_ID = 128


class MemfaultErrorV2(smperr.ErrorV2[MEMFAULT_RET_RC]):
    _GROUP_ID = 128


class DeviceInfoReadRequest(smpmsg.ReadRequest):
    _GROUP_ID = 128
    _COMMAND_ID = 0


class DeviceInfoReadResponse(smpmsg.ReadResponse):
    _GROUP_ID = 128
    _COMMAND_ID = 0

    device_serial: str
    hardware_version: str
    software_type: str
    current_version: str


class DeviceInfoRead(DeviceInfoReadRequest):
    _Response = DeviceInfoReadResponse
    _ErrorV1 = MemfaultErrorV1
    _ErrorV2 = MemfaultErrorV2


class ProjectKeyReadRequest(smpmsg.ReadRequest):
    _GROUP_ID = 128
    _COMMAND_ID = 1


class ProjectKeyReadResponse(smpmsg.ReadResponse):
    _GROUP_ID = 128
    _COMMAND_ID = 1

    project_key: str


class ProjectKeyRead(ProjectKeyReadRequest):
    _Response = ProjectKeyReadResponse
    _ErrorV1 = MemfaultErrorV1
    _ErrorV2 = MemfaultErrorV2


@app.command()
def device_info(ctx: typer.Context) -> None:
    """Read Memfault device information (serial, hardware version, software type, current version)."""

    options = cast("Options", ctx.obj)
    smpclient = get_smpclient(options)

    async def f() -> None:
        await connect_with_spinner(smpclient, options.timeout)

        r = await smpclient.request(DeviceInfoRead())
        rich.print(r)

    asyncio.run(f())


@app.command()
def project_key(ctx: typer.Context) -> None:
    """Read Memfault project key."""

    options = cast("Options", ctx.obj)
    smpclient = get_smpclient(options)

    async def f() -> None:
        await connect_with_spinner(smpclient, options.timeout)

        r = await smpclient.request(ProjectKeyRead())
        rich.print(r)

    asyncio.run(f())
