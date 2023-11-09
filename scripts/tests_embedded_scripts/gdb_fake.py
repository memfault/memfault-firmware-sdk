#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import dataclasses
import sys
from typing import Any, Optional


def install_fake() -> None:
    sys.modules.setdefault("gdb", sys.modules[__name__])


MEMFAULT_MOCK_IMPLEMENTATION = True
COMPLETE_NONE = 0
COMMAND_USER = 0


class Command:
    def __init__(self, *args, **kwargs):
        pass


class Breakpoint:
    pass


@dataclasses.dataclass
class Type:
    sizeof: int


@dataclasses.dataclass
class Value:
    _value: Any
    type: Type  # noqa: A003

    def __int__(self) -> int:
        return int(self._value)

    def __str__(self) -> str:
        if self._value is None:
            return "<unavailable>"

        return str(self._value)


class Frame:
    def read_register(self, name) -> Value:
        if name.lower() == "control":
            return Value(0, type=Type(sizeof=1))

        return Value(None, type=Type(sizeof=4))

    def select(self):
        pass


def lookup_type(name):
    sizeof_by_name = {
        "unsigned char": 1,
        "unsigned short": 2,
        "unsigned int": 4,
        "unsigned long": 4,
        "unsigned long long": 8,
    }

    return Type(sizeof=sizeof_by_name[name])


def newest_frame() -> Optional[Frame]:
    return None


class _Inferior:
    def threads(self):
        return []

    def read_memory(self, addr, size):
        raise NotImplementedError


def inferiors():
    return []


def selected_inferior():
    return None


def selected_thread():
    return None


def execute(cmd: str, to_string=False) -> Any:
    raise NotImplementedError


class GdbError(Exception):
    pass


error = GdbError
