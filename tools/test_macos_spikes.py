"""Build and run the macOS feasibility spikes (port/macos/spikes).

Each spike is a stand-alone C program for arm64 macOS that settles one
question the macOS host depends on, and prints PASS or FAIL: <reason> as its
last line. No game data needed. Skipped on anything but arm64 macOS.
docs/macos_host.md records the results.

Two spikes fail on macOS 26, and are expected to: a process cannot map the
guest's range below 4 GB, and the kernel overwrites TPIDR_EL0. They are
strict expected failures, so a macOS that allows either makes them pass and
this test fail, which is the signal to look again at the in-process host.
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


def build_and_run(directory, source, flags=(), entitlements=None):
    directory.mkdir(parents=True, exist_ok=True)
    exe = directory / Path(source).stem
    build = subprocess.run(
        ["clang", "-O1", "-Wall", "-Werror", str(SPIKES / source), "-o", str(exe), *flags],
        capture_output=True,
        text=True,
    )
    if build.returncode != 0:
        return False, "build failed:\n" + build.stderr
    if entitlements:
        sign = subprocess.run(
            ["codesign", "-s", "-", "-f", "--entitlements", str(SPIKES / entitlements), str(exe)],
            capture_output=True,
            text=True,
        )
        if sign.returncode != 0:
            return False, "codesign failed:\n" + sign.stderr
    try:
        result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=60)
    except subprocess.TimeoutExpired:
        return False, "timed out"
    output = (result.stdout + result.stderr).strip()
    last = output.splitlines()[-1] if output else "(no output; status %d)" % result.returncode
    return result.returncode == 0 and last == "PASS", output


@pytest.mark.xfail(strict=True, reason="arm64 macOS refuses memory below 4 GB to a process")
def test_low_memory(tmp_path):
    # A host linked with a small page-zero is killed at exec; one with the
    # default page-zero cannot allocate below 4 GB even after freeing it.
    small = build_and_run(tmp_path / "small", "low_memory.c", ["-Wl,-pagezero_size,0x4000"])
    default = build_and_run(tmp_path / "default", "low_memory.c")
    assert small[0] or default[0], "small page-zero:\n" + small[1] + "\n\ndefault page-zero:\n" + default[1]


@pytest.mark.xfail(strict=True, reason="the macOS kernel overwrites TPIDR_EL0 on a context switch")
def test_thread_pointer(tmp_path):
    ok, output = build_and_run(tmp_path, "thread_pointer.c", ["-lpthread"])
    assert ok, output


def test_hypervisor(tmp_path):
    # The guest's address space inside a VM: code runs at 0x80000000 and
    # each hvc returns to the host.
    ok, output = build_and_run(
        tmp_path, "hypervisor.c", ["-framework", "Hypervisor"], entitlements="hypervisor.entitlements"
    )
    assert ok, output
