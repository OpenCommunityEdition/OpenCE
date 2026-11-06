"""Runs the C tests of the boot code the Android and Linux hosts share
(port/android/host/host_boot.c) with the build machine's C compiler. The unit
includes SDL3/SDL.h, so the test is skipped without pkg-config and SDL3."""

import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent


def compiler():
    for name in ("clang", "cc", "gcc"):
        path = shutil.which(name)
        if path:
            return path
    return None


def sdl3_flags():
    if not shutil.which("pkg-config"):
        return None
    result = subprocess.run(
        ["pkg-config", "--cflags", "--libs", "sdl3"],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return None
    return result.stdout.split()


def test_host_boot(tmp_path):
    cc = compiler()
    if not cc:
        pytest.skip("no C compiler")
    sdl = sdl3_flags()
    if sdl is None:
        pytest.skip("no pkg-config or SDL3")
    binary = tmp_path / "host_boot_test"
    data = tmp_path / "work"
    data.mkdir()
    subprocess.run(
        [
            cc,
            "-std=gnu11",
            "-D_GNU_SOURCE",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-O1",
            "-Iport/guest/include",
            "-Iport/android/host",
            "-Iport/third_party/tomlc17",
            "port/android/tests/host_boot_test.c",
            "port/android/host/host_boot.c",
            "port/third_party/tomlc17/tomlc17.c",
            *sdl,
            "-o",
            str(binary),
        ],
        check=True,
        cwd=ROOT,
    )
    result = subprocess.run(
        [str(binary), str(data)], check=False, capture_output=True, text=True
    )
    assert result.returncode == 0, result.stdout + result.stderr
