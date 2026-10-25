"""display.frame_time (port/linux/game/frame_time.c): the setting exists,
is off by default, and is in the settings table; the game draws it only
when it is on (tools/test_macos_frames.py captures it)."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def test_the_setting_is_off_by_default():
    config = (ROOT / "port/linux/src/port_config.c").read_text()
    entry = re.search(r'\{ "display\.frame_time", (\w+), "([^"]*)", "(\w+)", (\w+), (\w+),', config)
    assert entry, "display.frame_time is not in port_config.c"
    assert entry.groups() == ("_config_boolean", "false", "HALO_FRAME_TIME", "_environment_value", "_platform_all")


def test_the_setting_is_documented():
    table = (ROOT / "port/linux/README.md").read_text()
    assert re.search(r"^\| `display\.frame_time` \| `false` \| `HALO_FRAME_TIME` \| .+ \|$", table, re.M)


def test_the_game_draws_it_once_a_frame():
    # after the game's frame statistics, once for all the windows
    source = (ROOT / "source/rasterizer/xbox/rasterizer_xbox.c").read_text()
    assert (
        "rasterizer_frame_statistics_draw();\n\t\t/* port: display.frame_time's overlay */\n\t\tframe_time_draw();"
        in source
    )
