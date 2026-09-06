"""Compile the public reset result contract; do not infer it from call bytes.

This is an API/ABI declaration test, not a server-state or full-link test.
The negative control mutates only a temporary copy of the owning header.
"""
import os
from pathlib import Path
import subprocess
import sys

import pytest

from tools import coff_compare as cc


ROOT = Path(__file__).resolve().parents[1]
UNIT = "source/networking/network_server_manager"
HEADER = ROOT / "source/networking/network_server_manager.h"
PROBE = """
boolean network_server_reset_result_contract(
    struct network_game_server *server)
{
    boolean result = network_game_server_reset_to_pregame(server);

    return result;
}
"""


@pytest.fixture(scope="module")
def compile_contract(tmp_path_factory):
    compiler = ROOT / "xbox/bin/vc7/CL.Exe"
    if not compiler.is_file() or not (ROOT / f"build/split/{UNIT}.obj").is_file():
        pytest.skip("locally supplied January target/compiler is unavailable")
    folder = tmp_path_factory.mktemp("server-reset-contract")

    def compile_one(name, header):
        source = folder / f"{name}.c"
        output = folder / f"{name}.obj"
        source.write_text(header + "\n" + PROBE, encoding="ascii")
        command = [sys.executable, "-B", "tools/campaign/gate.py", UNIT,
                   "--source", str(source), "--out", str(output),
                   "--fn", "_network_game_server_reset_to_pregame",
                   "--forbid-emitted-symbol", "_point_from_line3d"]
        result = subprocess.run(
            command, cwd=ROOT, capture_output=True, text=True,
            env=dict(os.environ, HALO_CL=str(compiler)),
        )
        return result, output

    # Establish that the same probe and toolchain work before the mutation.
    result, output = compile_one(
        "actual", '#include "networking/network_server_manager.h"')
    assert result.returncode == 0, result.stdout + result.stderr
    obj = cc.load(output)
    symbol = cc.symbol(obj, "_network_server_reset_result_contract")
    assert symbol["section"] > 0
    calls = cc.section_info(obj, symbol["name"])["relocations"]
    assert any(tuple(call["target"]) ==
               ("symbol", "_network_game_server_reset_to_pregame", 0)
               for call in calls)
    return compile_one


def test_public_reset_result_is_usable(compile_contract):
    # The fixture compiles a caller consuming the actual public return value.
    assert callable(compile_contract)


def test_void_reset_declaration_is_rejected(compile_contract):
    header = HEADER.read_text(encoding="ascii")
    correct = "boolean network_game_server_reset_to_pregame("
    assert header.count(correct) == 1
    mutant = header.replace(correct, "void network_game_server_reset_to_pregame(")
    result, output = compile_contract("void-result", mutant)
    assert result.returncode != 0
    assert not output.exists()
    # Do not accept unrelated setup/include failures as a successful control.
    assert "COMPILE FAILED" in result.stdout
    use = "    boolean result = network_game_server_reset_to_pregame(server);"
    line = (mutant + "\n" + PROBE).splitlines().index(use) + 1
    assert f"({line}) : error C2120: 'void' illegal with all types" in result.stdout
