"""Runs the C tests of the guest's host (port/android/host) that need
neither Android nor the guest: the image loader's checks (host_loader.c),
the processor's registers and frame walk (host_arch.c), and the guest's
memory system calls and crash report (host_memory.c), on the build
machine's processor (AArch64 or x86-64), with its C compiler."""
import platform
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
HOST = ROOT / "port" / "android" / "host"
TESTS = ROOT / "port" / "android" / "tests"


def compiler():
    for name in ("clang", "cc", "gcc"):
        path = shutil.which(name)
        if path:
            return path
    return None


def run_test(tmp_path, name, sources):
    cc = compiler()
    if not cc:
        pytest.skip("no C compiler")
    if platform.machine().lower() not in ("x86_64", "amd64", "aarch64", "arm64"):
        pytest.skip("the host runs on AArch64 and x86-64 only")
    binary = tmp_path / name
    subprocess.run(
        [cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-O1", "-D_GNU_SOURCE",
         f"-I{ROOT / 'port' / 'android' / 'include'}", f"-I{HOST}",
         "-pthread", *[str(source) for source in sources], "-o", str(binary)],
        check=True,
    )
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr


def test_host_loader(tmp_path):
    run_test(tmp_path, "host_loader_test", [TESTS / "host_loader_test.c", HOST / "host_loader.c"])


def test_host_arch(tmp_path):
    run_test(tmp_path, "host_arch_test", [TESTS / "host_arch_test.c", HOST / "host_arch.c"])


def test_host_memory(tmp_path):
    run_test(tmp_path, "host_memory_test",
             [TESTS / "host_memory_test.c", HOST / "host_arch.c", HOST / "host_watch_hash.c"])
