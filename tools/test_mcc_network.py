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


@pytest.mark.parametrize("case", ["host", "receive", "prediction", "rewind_host", "rewind_client", "damage", "legacy", "campaign"])
def test_mcc_network(mcc_network_tool, case):
    run = subprocess.run([str(mcc_network_tool), case], capture_output=True, text=True)
    assert run.returncode == 0, run.stdout + run.stderr


def test_legacy_inventory_wire_unchanged():
    from harness import structure
    source = (ROOT / "port/linux/game/network_objects.c").read_text()
    inventory = structure(source, "distributed_inventory")
    assert "grenade_counts[NUMBER_OF_UNIT_GRENADE_TYPES]" in inventory
    assert "mcc" not in inventory


def test_map_sharing_keeps_mcc_packets_and_names_separate(tmp_path):
    from harness import enum_with, function, structure
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is required")
    header = (ROOT / "port/linux/game/network_distributed.h").read_text()
    share = (ROOT / "port/linux/game/map_share.c").read_text()
    ce = (ROOT / "port/linux/game/custom_edition_cache.c").read_text()
    code = r'''
#include <string.h>
typedef int boolean;
typedef unsigned char byte;
typedef unsigned short word;
typedef word message_header;
#define FALSE 0
#define csmemcpy memcpy
#define csstrlen strlen
#define CUSTOM_EDITION_LEVEL_NAME_PREFIX "custom_maps\\"
#ifndef _WIN32
#define _strnicmp strncasecmp
#endif
''' + enum_with(header, "_distributed_message_player_prediction") + structure(header, "distributed_message_header")
    code += function(share, "map_share_handles_message") + function(ce, "custom_edition_level_name")
    code += r'''
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)
int main(void) {
 struct distributed_message_header h = {0};
 int i;
 CHECK(_distributed_message_mcc_grenades == 87);
 CHECK(_distributed_message_mcc_campaign == 88);
 for (i = 0; i < NUMBER_OF_DISTRIBUTED_MESSAGES; ++i) {
  h.type = i;
  CHECK(!!map_share_handles_message((word *)&h, sizeof(h)) == (i >= 83 && i <= 86));
 }
 h.type = _distributed_message_map_share_request;
 for (i = 0; i < sizeof(h); ++i) CHECK(!map_share_handles_message((word *)&h, i));
 CHECK(custom_edition_level_name("custom_maps\\test"));
 CHECK(!custom_edition_level_name("mcc_maps\\package\\m0"));
 CHECK(!custom_edition_level_name("mcc_maps\\a10"));
 CHECK(!custom_edition_level_name("levels\\a10\\a10"));
 return 0;
}
'''
    path = tmp_path / "routing.c"
    path.write_text(code)
    exe = tmp_path / ("routing.exe" if os.name == "nt" else "routing")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else ["-m32"]
    built = subprocess.run([clang, *target, str(path), "-o", str(exe)], capture_output=True, text=True)
    assert built.returncode == 0, built.stderr
    assert subprocess.run([str(exe)], timeout=10).returncode == 0
