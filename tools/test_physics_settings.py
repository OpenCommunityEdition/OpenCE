"""The physics' settings (port/linux/game/physics_jolt.c): game.ragdolls,
game.physics_debris, game.physics_debris_max and debug.physics_draw are
config.toml's, off by default, read from config.toml and from their
environment variables, in the settings table, and on Video Setup's Physics
screen (tools/port_settings.py), whose files are those the generator writes."""

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / "tools"))

import port_settings  # noqa: E402
from tools.test_linux_port import CONFIG_PROGRAM  # noqa: E402

SETTINGS = {
    "game.ragdolls": ("_config_boolean", "false", "HALO_RAGDOLLS"),
    "game.dismemberment": ("_config_boolean", "false", "HALO_DISMEMBERMENT"),
    "game.physics_debris": ("_config_boolean", "false", "HALO_PHYSICS_DEBRIS"),
    "game.physics_debris_max": ("_config_integer", "256", "HALO_PHYSICS_DEBRIS_MAX"),
    "debug.physics_draw": ("_config_boolean", "false", "HALO_PHYSICS_DRAW"),
}
SCREEN = "main_menu.settings_select.player_setup.player_profile_edit.video_settings.physics.xml"


@pytest.mark.parametrize("name", sorted(SETTINGS))
def test_the_setting_is_off_by_default(name):
    config = (ROOT / "port/linux/src/port_config.c").read_text()
    entry = re.search(r'\{ "' + re.escape(name) + r'", (\w+), "([^"]*)", "(\w+)", (\w+), (\w+),\n\t\t"', config)
    assert entry, f"{name} is not in port_config.c, or has no comment"
    assert entry.groups() == (*SETTINGS[name], "_environment_value", "_platform_all")


@pytest.mark.parametrize("name", sorted(SETTINGS))
def test_the_setting_is_documented(name):
    table = (ROOT / "port/linux/README.md").read_text()
    row = next(
        (line for line in table.splitlines() if line.startswith("| ") and f"`{name}`" in line.split(" | ")[0]), None
    )
    assert row, f"{name} is not in the settings table"
    assert SETTINGS[name][2] in row and f"`{SETTINGS[name][1]}`" in row


def test_the_physics_screen_lists_them():
    files = port_settings.settings_files()
    assert SCREEN in files
    screen = "\n".join(files[SCREEN])
    assert set(re.findall(r'setting="([^"]+)"', screen)) == {
        "game.ragdolls",
        "game.dismemberment",
        "game.physics_debris",
        "game.physics_debris_max",
    }
    video = "\n".join(files[SCREEN.replace(".physics.xml", ".xml")])
    assert "physics_settings_screen" in video
    # the generator's files are those checked in, and the game embeds the new one
    for name, lines in files.items():
        assert (ROOT / "port/assets/menus/ce" / name).read_text() == "\n".join(lines), name
    assert f'"ce/{SCREEN}"' in (ROOT / "port/assets/menus/menus.json").read_text()


def test_they_are_read_from_config_and_environment(tmp_path):
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("needs a C compiler")
    stubs = tmp_path / "stubs"
    (stubs / "SDL3").mkdir(parents=True)
    (stubs / "SDL3" / "SDL.h").write_text("")
    (stubs / "platform.h").write_text("void platform_log(const char *format, ...);\n")
    for name in ("port_config.c", "port_config.h"):
        shutil.copy(ROOT / "port/linux/src" / name, stubs / name)
    (tmp_path / "main.c").write_text(CONFIG_PROGRAM)
    program = tmp_path / "config_program"
    subprocess.run(
        [
            compiler,
            "-DHALO_ANDROID=1",
            "-DHALO_ARM64_GUEST=1",
            f"-I{stubs}",
            f"-I{ROOT}/port/third_party/tomlc17",
            "-o",
            str(program),
            str(tmp_path / "main.c"),
            str(stubs / "port_config.c"),
            str(ROOT / "port/third_party/tomlc17/tomlc17.c"),
            "-lpthread",
        ],
        check=True,
    )
    data = tmp_path / "data"
    data.mkdir()
    clean = {key: value for key, value in os.environ.items() if key not in {v[2] for v in SETTINGS.values()}}

    def get(name, **environment):
        result = subprocess.run(
            [str(program), "get", name],
            capture_output=True,
            text=True,
            env={**clean, "HALO_DATA_ROOT": str(data), **environment},
        )
        assert result.returncode == 0, result.stderr
        return result.stdout.strip()

    for name, (_, default, _) in SETTINGS.items():
        assert get(name) == default, name
    (data / "config.toml").write_text(
        "[game]\nragdolls = true\ndismemberment = true\nphysics_debris = true\nphysics_debris_max = 64\n\n"
        "[debug]\nphysics_draw = true\n"
    )
    assert [get(name) for name in SETTINGS] == ["true", "true", "true", "64", "true"]
    assert get("game.ragdolls", HALO_RAGDOLLS="false") == "false"
    assert get("game.physics_debris_max", HALO_PHYSICS_DEBRIS_MAX="512") == "512"
