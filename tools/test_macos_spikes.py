"""Build and run the macOS feasibility spikes (port/macos/spikes).

Each spike is a stand-alone C program for arm64 macOS that settles one
question the macOS host depends on, and prints PASS or FAIL: <reason> as its
last line. No game data needed. Skipped on anything but arm64 macOS.
"""

import platform
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
SPIKES = ROOT / "port/macos/spikes"

pytestmark = pytest.mark.skipif(
    sys.platform != "darwin" or platform.machine() != "arm64",
    reason="the spikes run on arm64 macOS only",
)


def build_and_run(tmp_path, source, flags=()):
    exe = tmp_path / Path(source).stem
    build = subprocess.run(
        ["clang", "-O1", "-Wall", "-Werror", str(SPIKES / source), "-o", str(exe), *flags],
        capture_output=True, text=True,
    )
    if build.returncode != 0:
        return False, "build failed:\n" + build.stderr
    try:
        result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=30)
    except subprocess.TimeoutExpired:
        return False, "timed out"
    output = (result.stdout + result.stderr).strip()
    last = output.splitlines()[-1] if output else "(no output; status %d)" % result.returncode
    return result.returncode == 0 and last == "PASS", output


def test_low_memory(tmp_path):
    # Either a host linked with a small page-zero, or one that frees the
    # range below 4 GB at start-up, must be able to map the guest's range.
    (tmp_path / "small").mkdir()
    (tmp_path / "default").mkdir()
    small = build_and_run(tmp_path / "small", "low_memory.c", ["-Wl,-pagezero_size,0x4000"])
    default = build_and_run(tmp_path / "default", "low_memory.c")
    assert small[0] or default[0], (
        "small page-zero:\n" + small[1] + "\n\ndefault page-zero:\n" + default[1])


def test_exec_image(tmp_path):
    ok, output = build_and_run(tmp_path, "exec_image.c", ["-Wl,-pagezero_size,0x4000"])
    assert ok, output


def test_thread_pointer(tmp_path):
    ok, output = build_and_run(tmp_path, "thread_pointer.c", ["-lpthread"])
    assert ok, output
