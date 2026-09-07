"""Compile the shared protocol constants; this is not a network runtime test."""
import os
from pathlib import Path
import struct
import subprocess
import sys

import pytest

from tools import coff_compare as cc

ROOT = Path(__file__).resolve().parents[1]
UNIT = "source/networking/network_server_manager"
SYMBOL = "_network_protocol_contract"
EXPECTED = (0, 1, 1, 2, 4, 0x141E, 0x141F, 4, 7, 400, 0x7F000001)
PROBE = """
unsigned long const network_protocol_contract[] =
{
    _byte_order_host,
    _byte_order_network,
    FLAG(_connection_create_server_bit),
    FLAG(_connection_create_clientside_client_bit),
    FLAG(_connection_create_serverside_client_bit),
    NETWORK_GAME_SERVER_PORT,
    NETWORK_GAME_CLIENT_PORT,
    _rejection_code_game_is_full,
    NUMBER_OF_SERVER_REJECTION_CODES,
    DATAGRAM_MAXIMUM_SIZE,
    IPV4_LOOPBACK_ADDRESS,
};
"""


class ProtocolConstantMismatch(AssertionError):
    """Raised only for an actual compiled constant-value mismatch."""


def require_constants(obj):
    symbol = cc.symbol(obj, SYMBOL)
    assert symbol["section"] > 0 and symbol["value"] == 0
    section = obj["sections"][symbol["section"] - 1]
    assert section["reloc_count"] == 0
    raw = bytes(cc._section_bytes(obj, section))
    assert len(raw) == 4 * len(EXPECTED)
    actual = struct.unpack(f"<{len(EXPECTED)}I", raw)
    if actual != EXPECTED:
        raise ProtocolConstantMismatch((actual, EXPECTED))


@pytest.fixture(scope="module")
def compiler(tmp_path_factory):
    cl = ROOT / "xbox/bin/vc7/CL.Exe"
    if not cl.is_file() or not (ROOT / f"build/split/{UNIT}.obj").is_file():
        pytest.skip("locally supplied VC7 compiler/January target is unavailable")
    folder = tmp_path_factory.mktemp("network-protocol-contract")
    headers = ("bungie_net/common/message_header.h",
               "bungie_net/network/transport_address_constants.h",
               "networking/network_connection.h",
               "networking/network_game_protocol.h")

    def compile_one(name, mutation=None):
        parts = ['#include "cseries/cseries.h"']
        for header in headers:
            if mutation and header == mutation[0]:
                text = (ROOT / "source" / header).read_text()
                assert text.count(mutation[1]) == 1
                parts.append(text.replace(mutation[1], mutation[2]))
            else:
                parts.append(f'#include "{header}"')
        source, output = folder / (name + ".c"), folder / (name + ".obj")
        source.write_text("\n".join(parts) + "\n" + PROBE, encoding="ascii")
        result = subprocess.run(
            [sys.executable, "-B", "tools/campaign/gate.py", UNIT,
             "--source", str(source), "--out", str(output)],
            cwd=ROOT, env=dict(os.environ, HALO_CL=str(cl)),
            capture_output=True, text=True,
        )
        assert result.returncode == 0, result.stdout + result.stderr
        return cc.load(output)

    require_constants(compile_one("actual"))
    return compile_one


def test_actual_owner_constants(compiler):
    assert callable(compiler)


@pytest.mark.parametrize("name,mutation", [
    ("wrong-direction", ("bungie_net/common/message_header.h",
                         "_byte_order_network,", "_byte_order_network = 0,")),
    ("wrong-port", ("networking/network_game_protocol.h",
                    "NETWORK_GAME_SERVER_PORT = 0x141E,",
                    "NETWORK_GAME_SERVER_PORT = 0x141F,")),
    ("wrong-rejection", ("networking/network_game_protocol.h",
                         "_rejection_code_game_is_full,",
                         "_rejection_code_game_is_full = 3,")),
    ("wrong-datagram-capacity", ("networking/network_connection.h",
                                 "DATAGRAM_MAXIMUM_SIZE = 400,",
                                 "DATAGRAM_MAXIMUM_SIZE = 401,")),
    ("wrong-loopback", ("bungie_net/network/transport_address_constants.h",
                        "IPV4_LOOPBACK_ADDRESS = 0x7F000001,",
                        "IPV4_LOOPBACK_ADDRESS = 0x0100007F,")),
])
def test_compiled_bad_constant_is_detected(compiler, name, mutation):
    # Compilation/setup errors cannot satisfy the expected mismatch control.
    obj = compiler(name, mutation)
    with pytest.raises(ProtocolConstantMismatch):
        require_constants(obj)
