"""The macOS host running the guest image in its VM (docs/macos_host.md).

After `ninja macos`: the host loads build/macos/halo_guest.elf, the guest's
main runs, logs through its own standard error (port/linux/src/xbox_kernel.c's
platform_log, "halo-linux: ...", which reaches the host as a write system
call), and the guest ends the process itself through exit, not by a crash.
That needs no game data. With the game data in assets/maps, a run under
debug.exit_after ends with status 0.

Each run is headless: debug.null_renderer, debug.hidden_window, and SDL's
dummy video and audio drivers.
"""

import os
import platform
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


def run_host(data_root, exit_after=2, timeout=120):
    environment = dict(os.environ)
    environment.update({
        "HALO_DATA_ROOT": str(data_root),
        "HALO_EXIT_AFTER": str(exit_after),
        "HALO_NULL_RENDERER": "1",
        "HALO_HIDDEN_WINDOW": "1",
        "SDL_VIDEO_DRIVER": "dummy",
        "SDL_AUDIO_DRIVER": "dummy",
    })
    return subprocess.run([str(HOST), str(IMAGE)], cwd=data_root, env=environment, capture_output=True,
                          text=True, timeout=timeout)


def test_the_guest_runs_its_main_and_exits_by_itself(tmp_path):
    result = run_host(tmp_path)
    output = result.stdout + result.stderr
    assert "halo_host: guest image 88000000-" in output, output[-4000:]
    assert "halo-linux: " in output, output[-4000:]
    assert "halo_host: the guest exited with status" in output, output[-4000:]
    assert "halo_host: guest crash" not in output, output[-4000:]


@pytest.mark.skipif(not (MAPS / "ui.map").is_file(), reason="needs the game data in assets/maps")
def test_a_run_with_the_game_data_ends_cleanly_after_exit_after():
    result = run_host(ROOT / "assets", exit_after=5)
    output = result.stdout + result.stderr
    assert result.returncode == 0, output[-4000:]
    assert "halo_host: guest crash" not in output, output[-4000:]
