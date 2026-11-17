"""Runs the C tests of the host's runtime page size (host_page_size in
port/android/host/host_memory.c) with the build machine's C compiler.
host_memory.c is Linux code (mmap, ucontext, /proc), so the test is skipped
off Linux."""

import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent


def compiler():
    for name in ("clang", "cc", "gcc"):
        path = shutil.which(name)
        if path:
            return path
    return None


def test_host_page_size(tmp_path):
    if not sys.platform.startswith("linux"):
        pytest.skip("host_memory.c is Linux code")
    cc = compiler()
    if not cc:
        pytest.skip("no C compiler")
    binary = tmp_path / "host_page_size_test"
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
            "port/android/tests/host_page_size_test.c",
            "port/android/host/host_memory.c",
            "port/android/host/host_watch_hash.c",
            "-lpthread",
            "-o",
            str(binary),
        ],
        check=True,
        cwd=ROOT,
    )
    result = subprocess.run([str(binary)], check=False, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
