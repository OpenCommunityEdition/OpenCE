"""Runs the C tests of the guest's system call interface
(port/android/host/host_syscall.c, port/android/include/halo_guest_syscall.h)
with the build machine's C compiler, on its own kernel: the translation must
hold on an x86-64 host as on an AArch64 one. Also checks that the guest's
numbering (port/android/guest/libc/arch/generic/bits/syscall.h.in) agrees
with the interface's header and has no legacy calls."""
import re
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / "port" / "android" / "include" / "halo_guest_syscall.h"
NUMBERING = ROOT / "port" / "android" / "guest" / "libc" / "arch" / "generic" / "bits" / "syscall.h.in"

# calls the asm-generic numbering leaves out; musl would take other paths
# (open, stat, poll ...) that the host does not translate
LEGACY = {
    "open", "creat", "stat", "lstat", "fstat64", "stat64", "lstat64", "fstatat64", "poll", "select",
    "_newselect", "access", "pipe", "dup2", "mkdir", "rmdir", "unlink", "rename", "link", "symlink",
    "readlink", "chmod", "chown", "lchown", "fork", "vfork", "getdents", "time", "utime", "utimes",
    "alarm", "pause", "epoll_wait", "epoll_create", "signalfd", "inotify_init", "eventfd", "fcntl64",
    "mmap2", "lseek", "llseek", "ftruncate64", "truncate64", "statfs64", "fstatfs64", "ugetrlimit",
    "getuid32", "futimesat", "sendfile64", "socketcall", "ipc",
}


def compiler():
    for name in ("clang", "cc", "gcc"):
        path = shutil.which(name)
        if path:
            return path
    return None


def numbering():
    result = {}
    for line in NUMBERING.read_text().splitlines():
        match = re.match(r"#define __NR_(\w+)\s+(\d+)\s*$", line)
        if match:
            result[match.group(1)] = int(match.group(2))
    return result


def test_numbering_has_no_legacy_calls():
    names = set(numbering())
    assert "_llseek" in names
    assert not names & LEGACY, sorted(names & LEGACY)


def test_header_numbers_match_the_numbering():
    numbers = numbering()
    defined = re.findall(r"#define GUEST_SYS_(\w+) (\d+)", HEADER.read_text())
    assert defined
    for name, number in defined:
        assert numbers.get(name) == int(number), name
    assert len(set(number for _, number in defined)) == len(defined)


def test_host_syscall(tmp_path):
    cc = compiler()
    if not cc:
        pytest.skip("no C compiler")
    binary = tmp_path / "host_syscall_test"
    subprocess.run(
        [cc, "-std=gnu11", "-D_GNU_SOURCE", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
         "-Wno-sign-compare", "-Wno-missing-field-initializers", "-O1",
         f"-I{ROOT / 'port' / 'android' / 'include'}", f"-I{ROOT / 'port' / 'android' / 'host'}",
         str(ROOT / "port" / "android" / "tests" / "host_syscall_test.c"),
         "-o", str(binary)],
        check=True,
    )
    result = subprocess.run([str(binary), str(tmp_path)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
