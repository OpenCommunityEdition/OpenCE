"""The macOS host running the guest image in its VM (docs/macos_host.md).

After `ninja macos`: the host loads build/macos/halo_guest.elf, the guest's
main runs, logs through its own standard error (port/linux/src/xbox_kernel.c's
platform_log, "halo-linux: ...", which reaches the host as a write system
call), and goes on through its start-up to look for the maps: with no game
data, the game stops itself there (its debug.txt says "no valid map
directory exists") and the guest ends the process through exit, not by a
crash. With the game data in assets/maps, a run under debug.exit_after
ends with status 0, and one under debug.network_test hosts a game on
bloodgulch and logs its player's state each second.

The runs never touch the home folder: HOME and XDG_DATA_HOME (where the
platform layer keeps saves) are the test's own.

Each run is headless: debug.null_renderer, debug.hidden_window, and SDL's
dummy video and audio drivers.
"""

import os
import platform
import re
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
HOST = ROOT / "build/macos/halo_host"
IMAGE = ROOT / "build/macos/halo_guest.elf"
MAPS = ROOT / "assets/maps"

pytestmark = [
    pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only"),
    pytest.mark.skipif(not (HOST.is_file() and IMAGE.is_file()), reason="needs `ninja macos`"),
]


def run_host(data_root, home, exit_after=2, timeout=120, extra=None):
    environment = dict(os.environ)
    (home / "saves").mkdir(parents=True, exist_ok=True)
    environment.update({
        "HOME": str(home),
        "XDG_DATA_HOME": str(home / "saves"),
        "HALO_DATA_ROOT": str(data_root),
        "HALO_EXIT_AFTER": str(exit_after),
        "HALO_NULL_RENDERER": "1",
        "HALO_HIDDEN_WINDOW": "1",
        "SDL_VIDEO_DRIVER": "dummy",
        "SDL_AUDIO_DRIVER": "dummy",
        **(extra or {}),
    })
    return subprocess.run([str(HOST), str(IMAGE)], cwd=data_root, env=environment, capture_output=True,
                          text=True, timeout=timeout)


def test_the_guest_runs_its_main_and_exits_by_itself(tmp_path):
    data_root = tmp_path / "data"
    data_root.mkdir()
    result = run_host(data_root, tmp_path / "home")
    output = result.stdout + result.stderr
    assert "halo_host: guest image 88000000-" in output, output[-4000:]
    assert "halo-linux: " in output, output[-4000:]
    assert "halo_host: the guest exited with status" in output, output[-4000:]
    assert "halo_host: guest crash" not in output, output[-4000:]
    log = (data_root / "debug.txt").read_text(errors="replace")
    assert "no valid map directory exists" in log, log[-4000:]
    # its settings and saves stayed where they were told to be
    assert (data_root / "config.toml").is_file()
    assert (tmp_path / "home" / "saves" / "halo-linux").is_dir()


@pytest.mark.skipif(not (MAPS / "ui.map").is_file(), reason="needs the game data in assets/maps")
def test_a_run_with_the_game_data_ends_cleanly_after_exit_after(tmp_path):
    result = run_host(game_data(tmp_path), tmp_path / "home", exit_after=5)
    output = result.stdout + result.stderr
    assert result.returncode == 0, output[-4000:]
    assert "halo_host: guest crash" not in output, output[-4000:]


def game_data(tmp_path):
    # (a data root of its own, so that debug.txt and config.toml are the
    # test's, with the maps linked into it)
    data_root = tmp_path / "data"
    data_root.mkdir()
    (data_root / "maps").symlink_to(MAPS)
    return data_root


@pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game data in assets/maps")
def test_a_hosted_network_game_plays_headless(tmp_path):
    result = run_host(game_data(tmp_path), tmp_path / "home", exit_after=30, timeout=180,
                      extra={"HALO_NETWORK_TEST": "host:bloodgulch", "HALO_NET_ADDRESS": "127.0.0.1"})
    output = result.stdout + result.stderr
    assert result.returncode == 0, output[-4000:]
    assert "halo_host: guest crash" not in output, output[-4000:]
    assert "network test: hosting bloodgulch" in output, output[-4000:]
    assert "network test: starting the game" in output, output[-4000:]
    # the local player joined its own game: its state each second
    ticks = re.findall(r"network test: tick \d+ player 0: \(", output)
    assert len(ticks) >= 5, output[-4000:]
