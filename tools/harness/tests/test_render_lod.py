"""Live render-only detail policy and the cinematic cryotube exception."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, function, read, run, mutated, CHECK_FAILED
import port_settings


def generated(fault=None):
    source = read("port/linux/game/render_lod.c")
    if fault == "exception":
        source = mutated(source, '"levels\\\\a10\\\\devices\\\\cryotube\\\\cryotube"', '"wrong model"')
    if fault == "live":
        source = mutated(source, "read_at != changes", "(read_at != changes && changes == 0)")
    return (("under_test.inc", "\n".join(function(source, name) for name in (
        "render_lod_disabled", "render_lod_model_disabled", "render_lod_particle_visible", "render_lod_geometry_level"))),)


@pytest.mark.parametrize("case", ["default", "live", "cryotube", "projection", "geometry"])
def test_policy(case):
    status, output = run(build("render_lod", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("fault,case", [("exception", "cryotube"), ("live", "live")])
def test_negative(fault, case):
    status, output = run(build("render_lod", generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_graphics_setting():
    rows = port_settings.SCREENS["video_settings/graphics"]["rows"]
    row, = [r for r in rows if r[1] == "display.disable_lod"]
    assert row[0] == "DISABLE LOD:"
    assert row[2] == [("OFF", "false"), ("ON", "true")]
    assert row[4] is None
    for name, lines in port_settings.settings_files().items():
        assert "\n".join(lines) == read("port/assets/menus/ce/" + name)
