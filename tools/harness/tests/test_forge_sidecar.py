"""Sidecar `.txt` map summary parsing and gametype detection (custom_edition_maps.c)."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run


def generated(fault=None):
    src = read("port/linux/game/custom_edition_maps.c")
    code = function(src, "custom_edition_map_format_forge_summary")
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


@pytest.mark.parametrize("case", ["basic", "modes", "sp_default", "invalid"])
def test_case(case):
    status, output = run(build("forge_sidecar", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("fault,case", [
    (('if (placed_obj_cnt == 0 && header_obj_cnt > 0', 'if (0 && header_obj_cnt > 0'), 'basic'),
    (('has_koth = 1;', 'has_koth = 0;'), 'modes'),
    (('is_sp_base = 1;', 'is_sp_base = 0;'), 'sp_default'),
    (('strstr(buf, "[OpenCE Forge Map]") == NULL && strstr(buf, "[ForgeState]") == NULL', '0'), 'invalid'),
])
def test_negative_control(fault, case):
    status, output = run(build("forge_sidecar", generated(fault)), case)
    assert status == CHECK_FAILED, output
