"""Exercise MCC's independent inventory packets, prediction and throw ledger."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def mcc_network_tool(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is required for MCC network checks")
    work = tmp_path_factory.mktemp("mcc-network")
    # Compile all real new implementation bodies against deterministic
    # transport/clock/object adapters; no game data or sockets are needed.
    implementation = "\n".join(line for line in (ROOT / "port/linux/game/mcc_network.c").read_text().splitlines()
                               if not line.startswith("#include"))
    damage = (ROOT / "port/linux/game/mcc_network_damage.inc").read_text()
    prefix = (ROOT / "tools/harness/tests/mcc_network.c").read_text()
    source = work / "network.c"
    source.write_text(prefix.replace("/* MCC_INVENTORY_IMPLEMENTATION */", implementation)
                     .replace("/* MCC_DAMAGE_IMPLEMENTATION */", damage))
    binary = work / ("network.exe" if os.name == "nt" else "network")
    target = (["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
              if os.name == "nt" else ["-m32"])
    command = [clang, *target, "-std=gnu99", "-O1", "-Wall", "-Wextra", "-Werror",
               "-fsanitize=undefined", "-fsanitize-trap=undefined", str(source), "-o", str(binary)]
    if os.name != "nt":
        command.append("-lm")
    build = subprocess.run(command, capture_output=True, text=True)
    assert build.returncode == 0, build.stdout + build.stderr
    return binary


@pytest.mark.parametrize("case", ["host", "receive", "prediction", "rewind_host", "rewind_client", "damage", "legacy"])
def test_mcc_network(mcc_network_tool, case):
    run = subprocess.run([str(mcc_network_tool), case], capture_output=True, text=True)
    assert run.returncode == 0, run.stdout + run.stderr


def test_legacy_inventory_wire_unchanged():
    from harness import structure
    source = (ROOT / "port/linux/game/network_objects.c").read_text()
    inventory = structure(source, "distributed_inventory")
    assert "grenade_counts[NUMBER_OF_UNIT_GRENADE_TYPES]" in inventory
    assert "mcc" not in inventory
