"""Check independent MCC script relocation and native extension behavior."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def native_mcc_harness(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler or sys.platform != "win32":
        pytest.skip("the script harness currently uses native x86 Windows game headers")
    directory = tmp_path_factory.mktemp("mcc-script-behavior")
    includes = ["port/windows/include/crt", "port/windows/include", "source", "source/cseries",
                "source/math", "source/tag_files", "source/rasterizer", "source/structures",
                "source/cache", "source/render", "port/include/xdk", "port/linux/game"]
    command = [compiler, "--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-fms-extensions",
               "-std=gnu89", "-O1", "-DDEBUG", "-Dxbox", "-D_CRT_SECURE_NO_WARNINGS",
               "-Wno-nonportable-include-path", "-Wno-c99-compat", "-Wno-visibility",
               "-include", str(ROOT / "port/windows/include/halo_windows_prefix.h"),
               "-iquote", str(ROOT / "port/linux/include")]
    command += ["-I" + str(ROOT / path) for path in includes]
    def build(name):
        binary = directory / (name + ".exe")
        extra = [str(ROOT / "port/linux/game/mcc_syntax.c")] if name == "mcc_scripts" else []
        result = subprocess.run(command + [str(ROOT / "tools/harness/tests" / (name + ".c")),
                                str(ROOT / "port/linux/game" / (name + ".c"))] + extra + ["-o", str(binary)],
                                capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        return binary
    return build


@pytest.fixture(scope="module")
def scripts_tool(native_mcc_harness):
    return native_mcc_harness("mcc_scripts")


@pytest.mark.parametrize("case", ["link", "alias", "extension", "segment", "core_segment", "global", "unknown",
                                  "bad_child", "child_salt", "short_data", "capacity", "full_capacity",
                                  "negative_count", "unterminated", "skull_read", "skull_wrong_type", "skull_write", "developer_read", "developer_cast", "developer_write", "skull_disable", "skull_enable", "skull_computed", "developer_capacity", "developer_allocation"])
def test_mcc_script_linking(scripts_tool, case):
    result = subprocess.run([str(scripts_tool), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("skull", ["sputnik", "catch", "boom"])
@pytest.mark.parametrize("case", ["skull_read", "skull_wrong_type", "skull_disable", "skull_enable", "skull_computed"])
def test_additional_disabled_skulls(scripts_tool, skull, case):
    result = subprocess.run([str(scripts_tool), case, "debug_ice_cream_flavor_status_"+skull],
                            capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("case", ["prediction_enable", "prediction_disable", "prediction_computed"])
def test_prediction_policy(scripts_tool, case):
    result = subprocess.run([str(scripts_tool), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("kind", ["real", "long", "boolean"])
def test_disabled_skull_result_matches_native_constant_type(scripts_tool, kind):
    result = subprocess.run([str(scripts_tool), "skull_disable", "debug_ice_cream_flavor_status_sputnik", kind],
                            capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


def test_mcc_vehicle_masks_and_legacy_isolation(native_mcc_harness):
    result = subprocess.run([str(native_mcc_harness("mcc_objects"))], capture_output=True,
                            text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


def test_mcc_player_input_and_profile_slots(native_mcc_harness):
    result = subprocess.run([str(native_mcc_harness("mcc_player"))], capture_output=True,
                            text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("counter", "012345")
def test_mcc_scratch_globals_are_map_local(scripts_tool, counter):
    result = subprocess.run([str(scripts_tool), "scratch", counter], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
