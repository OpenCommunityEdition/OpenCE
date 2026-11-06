"""The guests' memory layout agrees between their two halves, for each
guest architecture (tools/guest_arch.py), whatever computer runs the test:
the Xbox window that the guest hands out (port/linux/src/platform.h's
PLATFORM_CONTIGUOUS_SIZE) is the one the host reserves for it
(port/android/include/halo_android_abi.h's HALO_GUEST_WINDOW_SIZE, for the
host's processor), the image base and reserve are the build scripts', and
the caches are the window's (port/linux/include/halo_port_capacity.h).

The headers are run through clang's preprocessor for each target; nothing is
compiled, so neither a guest's C library nor an arm64 computer is needed."""
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Dict, Sequence

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from guest_arch import ARM64_32, GUEST_ARCHES, X32  # noqa: E402
import guest_image_check  # noqa: E402

ABI_HEADER = ROOT / "port" / "android" / "include" / "halo_android_abi.h"
# (the hosts' processors: the guest's ELF architecture)
HOST_TARGETS = {"aarch64": "aarch64-linux-gnu", "x86_64": "x86_64-linux-gnu"}
DESKTOP_WINDOW = 0x20000000
DEVELOPMENT_KIT_WINDOW = 0x08000000


def clang() -> str:
    cc = shutil.which("clang")
    if not cc:
        pytest.skip("needs clang")
    return cc


def macros(flags: Sequence[str], text: str) -> Dict[str, int]:
    """the integer macros that text defines, preprocessed with flags"""
    output = subprocess.run([clang(), *flags, "-E", "-dM", "-x", "c", "-"], input=text, capture_output=True,
                            text=True, check=True).stdout
    values = {}
    for line in output.splitlines():
        match = re.fullmatch(r"#define (\w+) \(?(0x[0-9A-Fa-f]+|\d+)[uUlL]*\)?", line.strip())
        if match:
            values[match.group(1)] = int(match.group(2), 0)
    return values


def without_includes(path: Path) -> str:
    """a header's text without its #include lines, which need the build's
    include paths and only declare"""
    return "".join(line for line in path.read_text().splitlines(keepends=True)
                   if not line.lstrip().startswith("#include"))


def guest_macros(flags: Sequence[str]) -> Dict[str, int]:
    text = (without_includes(ROOT / "port" / "linux" / "src" / "platform.h")
            + without_includes(ROOT / "port" / "linux" / "include" / "halo_port_capacity.h"))
    return macros(flags, text)


def host_macros(elf_arch: str) -> Dict[str, int]:
    return macros([f"--target={HOST_TARGETS[elf_arch]}", "-ffreestanding"], ABI_HEADER.read_text())


def guest_flags(arch) -> Sequence[str]:
    # (as tools/android_build.py's guest_abi_flags)
    return [*arch.target_flags, "-DHALO_GUEST=1", *(["-DHALO_GUEST_MACHO=1"] if arch.darwin_assembly else []),
            *arch.defines]


@pytest.mark.parametrize("name", sorted(GUEST_ARCHES))
def test_window_is_the_hosts(name):
    arch = GUEST_ARCHES[name]
    guest = guest_macros(guest_flags(arch))
    host = host_macros(arch.elf_arch)
    assert guest["PLATFORM_CONTIGUOUS_BASE"] == host["HALO_GUEST_WINDOW_BASE"]
    assert guest["PLATFORM_CONTIGUOUS_SIZE"] == host["HALO_GUEST_WINDOW_SIZE"]


@pytest.mark.parametrize("name", sorted(GUEST_ARCHES))
def test_image_range_is_the_hosts(name):
    arch = GUEST_ARCHES[name]
    host = host_macros(arch.elf_arch)
    assert host["HALO_GUEST_IMAGE_BASE"] == arch.image_base
    # (tools/guest_image_check.py's copy of HALO_GUEST_IMAGE_RESERVE)
    assert host["HALO_GUEST_IMAGE_RESERVE"] == guest_image_check.IMAGE_RESERVE
    window_end = host["HALO_GUEST_WINDOW_BASE"] + host["HALO_GUEST_WINDOW_SIZE"]
    image_end = arch.image_base + host["HALO_GUEST_IMAGE_RESERVE"]
    assert image_end <= host["HALO_GUEST_WINDOW_BASE"] or arch.image_base >= window_end


def test_window_sizes():
    """the x86-64 guest, whose image is below the window, has the desktop's
    window and caches; the AArch64 guests, whose image is right above it, and
    the web build have a development kit's window and the Xbox's caches"""
    x32 = guest_macros(guest_flags(X32))
    native = guest_macros(["--target=i686-linux-gnu"])
    assert x32["PLATFORM_CONTIGUOUS_SIZE"] == native["PLATFORM_CONTIGUOUS_SIZE"] == DESKTOP_WINDOW
    for name in ("HALO_PORT_TEXTURE_CACHE_PAGE_COUNT", "HALO_PORT_SOUND_CACHE_SIZE"):
        assert x32[name] == native[name], name
    small = [guest_macros(guest_flags(ARM64_32)), guest_macros(["--target=wasm32", "-DHALO_WEB=1"])]
    for values in small:
        assert values["PLATFORM_CONTIGUOUS_SIZE"] == DEVELOPMENT_KIT_WINDOW
        assert values["HALO_PORT_TEXTURE_CACHE_PAGE_COUNT"] == 0x580
        assert values["HALO_PORT_SOUND_CACHE_SIZE"] == 0x400000
