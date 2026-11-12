"""Android upload-ring ownership with delayed GPU completion.

Compile the host's real fence and upload functions against a queued fake GL.
This checks synchronization contracts, not a particular device or map.
"""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def frame_fences(tmp_path_factory):
    compiler = shutil.which(os.environ.get("CC", "clang"))
    if not compiler:
        pytest.skip("clang required")
    host = (ROOT / "port/android/host/host_gl.c").read_text()
    work = tmp_path_factory.mktemp("android-frame-fences")
    # Keep both the synchronization and the unsynchronized upload real.
    (work / "frame_fences.inc").write_text(host[host.index("#define FRAME_FENCE_SLOTS"):])
    executable = work / ("test.exe" if os.name == "nt" else "test")
    # This is host code (64-bit on Android), not the ILP32 game guest.
    target = ["-fuse-ld=lld"] if os.name == "nt" else []
    result = subprocess.run(
        [compiler, *target, "-std=gnu89", "-Wall", "-Wextra", "-Werror",
         "-Wno-unused-function", "-I", str(work),
         str(ROOT / "port/android/tests/frame_fences_test.c"), "-o", str(executable)],
        capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return executable


@pytest.mark.parametrize("case", ["signaled", "satisfied", "timeout", "failed", "creation_failed", "empty", "replacement"])
def test_upload_waits_for_queued_draw(frame_fences, case):
    result = subprocess.run([str(frame_fences), case], capture_output=True, text=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr
