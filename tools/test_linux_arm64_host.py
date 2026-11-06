"""The Linux arm64 host's start-up (port/linux-arm64/host/host_linux_main.c,
host_loader.c): with no game image it says so and exits 1, and so does it with
an image that is not an AArch64 executable. Needs the host built on Linux
arm64 (ninja linux_arm64_host) and no game data: the data root is a temp
folder holding an empty maps/ui.map, and there is no display to show the
error dialog on.

    python -m pytest -q tools/test_linux_arm64_host.py
"""

import os
import platform
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

HOST = ROOT / "build/linux-arm64/halo_host"


def _needs_host():
    if platform.system() != "Linux":
        pytest.skip("the Linux arm64 host is built and run on Linux")
    if platform.machine() not in ("aarch64", "arm64"):
        pytest.skip("the Linux arm64 host runs on arm64")
    if not HOST.is_file():
        pytest.skip("needs ninja linux_arm64_host's build/linux-arm64/halo_host")


def _data_root(tmp_path):
    """a data root the host accepts as holding the game data (maps/ui.map)"""
    root = tmp_path / "data"
    (root / "maps").mkdir(parents=True)
    (root / "maps" / "ui.map").touch()
    return root


def _run(data_root, image):
    """the host run to its end: (exit status, its stderr)"""
    environment = {
        key: value for key, value in os.environ.items() if not key.startswith("HALO_")
    }
    for name in ("DISPLAY", "WAYLAND_DISPLAY", "DBUS_SESSION_BUS_ADDRESS"):
        environment.pop(name, None)
    environment["HALO_DATA_ROOT"] = str(data_root)
    environment["HALO_GUEST_ELF"] = str(image)
    result = subprocess.run(
        [str(HOST)],
        check=False,
        cwd=data_root,
        env=environment,
        stdin=subprocess.DEVNULL,
        capture_output=True,
        text=True,
        errors="replace",
        timeout=30,
    )
    return result.returncode, result.stderr


def test_missing_game_image(tmp_path):
    _needs_host()
    status, errors = _run(_data_root(tmp_path), tmp_path / "missing.elf")
    assert status == 1
    assert "cannot read the game image" in errors


def test_image_that_is_not_an_executable(tmp_path):
    _needs_host()
    image = tmp_path / "zeroes.elf"
    image.write_bytes(bytes(64))
    status, errors = _run(_data_root(tmp_path), image)
    assert status == 1
    # host_loader.c's host_load_image: the header check comes first
    assert "the guest image is not an AArch64 executable" in errors
    assert "cannot load the game image" in errors
