"""Runs the C tests of the guest's host (port/android/host) that need
neither Android nor the guest: the image loader's checks (host_loader.c),
the processor's registers and frame walk (host_arch.c), and the guest's
memory system calls and crash report (host_memory.c), on the build
machine's processor (AArch64 or x86-64), with its C compiler."""
import platform
import shutil
import subprocess
import sys
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


def test_x32_boundary(tmp_path):
    """the x32 guest passes the host its pointers zero-extended
    (port/android/tests/x32_boundary_test.c), through the guest's own import
    stubs (tools/android_imports.py) and code generation (tools/guest_arch.py)"""
    if platform.machine().lower() not in ("x86_64", "amd64"):
        pytest.skip("the x32 guest runs on x86-64")
    cc, ld, objcopy, nm = (shutil.which(name) for name in ("clang", "ld.lld", "llvm-objcopy", "llvm-nm"))
    if not (cc and ld and objcopy and nm):
        pytest.skip("needs clang, ld.lld, llvm-objcopy and llvm-nm")
    sys.path.insert(0, str(ROOT / "tools"))
    from guest_arch import X32
    imports = tmp_path / "imports.list"
    imports.write_text("host_check\nhost_pointer\nhost_check_returned\n")
    subprocess.run([sys.executable, str(ROOT / "tools" / "android_imports.py"), "--arch", "x32",
                    str(tmp_path / "imports.s"), str(imports)], check=True)
    subprocess.run([cc, *X32.target_flags, "-O2", "-ffreestanding", "-c", str(TESTS / "x32_boundary_guest.c"),
                    "-o", str(tmp_path / "guest.o")], check=True)
    subprocess.run([cc, f"--target={X32.assembler_target('gnu')}", "-c", str(tmp_path / "imports.s"),
                    "-o", str(tmp_path / "imports.o")], check=True)
    (tmp_path / "guest.ld").write_text(
        "SECTIONS { . = 0x60000000; .text : { *(.text .text.*) } .rodata : { *(.rodata .rodata.*) } "
        ".data : { *(.data .data.*) } /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame) } }\n")
    subprocess.run([ld, "-m", X32.ld_emulation, "-static", "-nostdlib", "-T", str(tmp_path / "guest.ld"),
                    str(tmp_path / "guest.o"), str(tmp_path / "imports.o"), "-o", str(tmp_path / "guest.elf")],
                   check=True)
    subprocess.run([objcopy, "-O", "binary", str(tmp_path / "guest.elf"), str(tmp_path / "guest.bin")], check=True)
    symbols = {}
    for line in subprocess.run([nm, str(tmp_path / "guest.elf")], capture_output=True, text=True,
                               check=True).stdout.splitlines():
        address, _, name = line.split()
        symbols[name] = int(address, 16) - 0x60000000
    binary = tmp_path / "x32_boundary_test"
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Werror", "-O1", "-D_GNU_SOURCE", "-pthread",
                    str(TESTS / "x32_boundary_test.c"), "-o", str(binary)], check=True)
    result = subprocess.run([str(binary), str(tmp_path / "guest.bin"), hex(symbols["guest_entry"]),
                             hex(symbols["__host_import_table"])], capture_output=True, text=True)
    assert result.returncode == 0 and result.stdout.strip() == "ok", result.stdout + result.stderr


def test_image_base_agrees():
    """tools/guest_arch.py's image_base is the host's HALO_GUEST_IMAGE_BASE
    for this processor's guest"""
    cc = compiler()
    if not cc:
        pytest.skip("no C compiler")
    sys.path.insert(0, str(ROOT / "tools"))
    from guest_arch import GUEST_ARCHES
    machine = platform.machine().lower()
    arch = next((arch for arch in GUEST_ARCHES.values()
                 if arch.elf_arch == {"amd64": "x86_64", "arm64": "aarch64"}.get(machine, machine)), None)
    if arch is None:
        pytest.skip("no guest for this processor")
    output = subprocess.run([cc, "-E", "-dM", "-x", "c", "-include",
                             str(ROOT / "port" / "android" / "include" / "halo_android_abi.h"), "/dev/null"],
                            capture_output=True, text=True, check=True).stdout
    value = next(line.split()[2] for line in output.splitlines() if line.startswith("#define HALO_GUEST_IMAGE_BASE "))
    assert int(value.rstrip("uU"), 16) == arch.image_base
