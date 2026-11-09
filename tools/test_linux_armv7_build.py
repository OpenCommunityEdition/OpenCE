"""The armv7 (armhf) flags of the native Linux build (tools/linux_build.py,
configure.py --linux-arch armv7): the x86 flags are unchanged, the armv7 ones
drop the i686-only options and add the ARM target, signed char and the OpenGL
ES renderer.

    python -m pytest -q tools/test_linux_armv7_build.py

Needs no toolchain and no downloads.
"""

import io
import re
import subprocess
import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / "tools"))

import pytest

from tools import linux_build, ninja_syntax

X86 = SimpleNamespace(linux_arch="x86", linux_sysroot=None)
ARMV7 = SimpleNamespace(linux_arch="armv7", linux_sysroot=None)
ARMV7_SYSROOT = SimpleNamespace(linux_arch="armv7", linux_sysroot="/opt/armhf")


def test_is_armv7():
    assert not linux_build.is_armv7(X86)
    assert not linux_build.is_armv7(SimpleNamespace())
    assert linux_build.is_armv7(ARMV7)


def test_x86_helpers_are_the_old_constants():
    assert linux_build.abi_flags(X86) == linux_build.LINUX_ABI_FLAGS
    assert linux_build.posix_flags(X86) == linux_build.POSIX_FLAGS
    assert linux_build.link_target_flags(X86) == ["--target=i686-linux-gnu", "-m32"]
    assert linux_build.abi_flags(SimpleNamespace()) == linux_build.LINUX_ABI_FLAGS


def test_armv7_abi_flags():
    flags = linux_build.abi_flags(ARMV7)
    for gone in ("--target=i686-linux-gnu", "-m32", "-malign-double", "-freg-struct-return"):
        assert gone not in flags
    for kept in ("--target=armv7-linux-gnueabihf", "-fsigned-char", "-mfloat-abi=hard",
                 "-DHALO_GLES=1", "-DHALO_LINUX_ARMV7=1", "-fshort-wchar", "-fms-extensions"):
        assert kept in flags
    assert not any(flag.startswith("--sysroot") for flag in flags)


def test_armv7_posix_flags():
    flags = linux_build.posix_flags(ARMV7)
    assert "--target=i686-linux-gnu" not in flags
    assert "-m32" not in flags
    assert "--target=armv7-linux-gnueabihf" in flags
    assert "-fsigned-char" in flags
    assert "-D_GNU_SOURCE" in flags


def test_armv7_link_target_flags():
    flags = linux_build.link_target_flags(ARMV7)
    assert flags == ["--target=armv7-linux-gnueabihf", "-mfloat-abi=hard", "-fuse-ld=lld"]


def test_sysroot():
    assert linux_build.sysroot_flags(X86) == []
    assert linux_build.sysroot_flags(ARMV7_SYSROOT) == ["--sysroot=/opt/armhf"]
    assert "--sysroot=/opt/armhf" in linux_build.abi_flags(ARMV7_SYSROOT)
    assert "--sysroot=/opt/armhf" in linux_build.posix_flags(ARMV7_SYSROOT)
    assert "--sysroot=/opt/armhf" in linux_build.link_target_flags(ARMV7_SYSROOT)


def test_march_flag():
    assert linux_build.march_flag(ARMV7) == "-march=armv7-a"
    assert linux_build.march_flag(SimpleNamespace(port_portable=True)) == "-march=x86-64"
    assert linux_build.march_flag(SimpleNamespace()) == "-march=native"


@pytest.fixture(autouse=True)
def in_the_repository(monkeypatch):
    """(configure.py runs from the repository's root, and so does this)"""
    monkeypatch.chdir(ROOT)


def graph(**overrides):
    """The native Linux part of build.ninja, as configure.py writes it"""
    fields = {
        "build_dir": Path("build"), "linux_cc": "clang", "compiler_launcher": None,
        "port_release": False, "port_profile": False, "port_lto": "off",
        "port_portable": False, "port_pgo": "off", "port_pgo_profile": None,
        "linux_arch": "x86", "linux_sysroot": None,
    }
    fields.update(overrides)
    out = io.StringIO()
    linux_build.generate_linux_build(ninja_syntax.Writer(out), SimpleNamespace(**fields))
    return out.getvalue()


def test_x86_graph():
    text = graph()
    assert "--target=i686-linux-gnu" in text
    assert "-m32" in text
    assert "armv7" not in text


def test_armv7_graph():
    text = graph(linux_arch="armv7")
    assert "--target=armv7-linux-gnueabihf" in text
    assert "-mfloat-abi=hard" in text
    assert "-fuse-ld=lld" in text
    assert "-DHALO_GLES=1" in text
    assert "i686" not in text
    assert "-m32" not in text


def test_armv7_links_the_tag_cache_window():
    text = graph(linux_arch="armv7")
    assert "-Wl,-T,tools/linux_armv7_tag_cache.ld" in text
    # (and an input of the link, so that a changed script relinks)
    assert "tools/linux_link_check.py tools/linux_armv7_tag_cache.ld" in text
    assert "tag_cache.ld" not in graph()


def test_tag_cache_script_matches_the_window():
    header = (ROOT / "port" / "linux" / "game" / "cache_file_formats.h").read_text()
    address = int(re.search(r"CUSTOM_EDITION_TAG_CACHE_ADDRESS (0x[0-9A-Fa-f]+)", header).group(1), 16)
    size = int(re.search(r"CUSTOM_EDITION_TAG_CACHE_BYTES (0x[0-9A-Fa-f]+)", header).group(1), 16)
    script = (ROOT / "tools" / "linux_armv7_tag_cache.ld").read_text()
    assert f".ce_tag_cache 0x{address:08x} (NOLOAD)" in script
    assert f". += 0x{size:x};" in script


def test_ci_runs_the_fenv_test():
    workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text()
    assert "port/linux-armv7/tests/fenv_test.c" in workflow
    assert "qemu-arm-static" in workflow
    assert (ROOT / "port" / "linux-armv7" / "tests" / "fenv_test.c").is_file()


def test_armv7_outputs_and_target():
    text = graph(linux_arch="armv7")
    assert "build/linux-armv7/halo" in text
    assert "build linux_armv7: phony" in text
    assert "build linux: phony" not in text


def test_armv7_never_uses_the_x86_profile():
    text = graph(linux_arch="armv7", port_pgo="train")
    assert "halo_linux.profdata" not in text
    assert "pgo-generate" not in text


def test_armv7_is_not_portable():
    with pytest.raises(ValueError):
        graph(linux_arch="armv7", port_portable=True)


def test_x86_target_name():
    assert "build linux: phony" in graph()


def test_armv7_graph_sysroot():
    assert "--sysroot=/r" in graph(linux_arch="armv7", linux_sysroot="/r")


def run_configure(tmp_path, *options):
    return subprocess.run(
        [sys.executable, str(ROOT / "configure.py"), *options],
        check=False,
        cwd=tmp_path,
        capture_output=True,
        text=True,
    )


def test_configure_rejects_armv7_portable(tmp_path):
    result = run_configure(tmp_path, "--linux-arch", "armv7", "--portable")
    assert result.returncode == 2
    assert "--portable" in result.stderr


def test_configure_rejects_armv7_pgo_train(tmp_path):
    result = run_configure(tmp_path, "--linux-arch", "armv7", "--pgo=train")
    assert result.returncode == 2
    assert "--pgo" in result.stderr


def test_configure_rejects_unknown_arch(tmp_path):
    assert run_configure(tmp_path, "--linux-arch", "bogus").returncode == 2


def test_configure_help_mentions_linux_arch(tmp_path):
    result = run_configure(tmp_path, "--help")
    assert result.returncode == 0
    assert "--linux-arch" in result.stdout


def test_prefix_header_accepts_arm():
    text = (ROOT / "port/linux/include/halo_linux_prefix.h").read_text()
    guard = next(line for line in text.splitlines() if line.startswith("#if !defined(__i386__)"))
    assert "defined(__arm__)" in guard
    assert "32-bit ARM" in text


def test_crash_report_reads_arm_registers():
    text = (ROOT / "port/linux/src/memory_watch.c").read_text()
    for register in ("arm_pc", "arm_fp", "arm_sp"):
        assert register in text
    assert "REG_EIP" in text


def test_ci_starts_the_armv7_build_under_qemu():
    text = (ROOT / ".github" / "workflows" / "build.yml").read_text()
    assert "qemu-arm-static -L" in text
    assert "build/linux-armv7/halo" in text
    assert "tools/linux_armv7_tag_cache.ld" in (ROOT / "tools" / "linux_build.py").read_text()
