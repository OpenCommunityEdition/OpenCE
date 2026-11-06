"""The native Linux arm64 host (tools/linux_arm64_build.py, port/linux-arm64):
the ninja graph has the linux_arm64_host target only on Linux arm64, and the
Android graph is left alone elsewhere. The host is built with the system's
SDL3 (pkg-config), as the NovaDeck's hand run does.

    python -m pytest -q tools/test_linux_arm64_build.py

Needs no toolchain and no downloads: the guest's fetches are stubbed out and
the graph is generated with a stand-in --guest-lld-bin.
"""

import io
import re
import sys
from pathlib import Path
from types import SimpleNamespace

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / "tools"))

from tools import linux_arm64_build, ninja_syntax


@pytest.fixture(autouse=True)
def in_the_repository(monkeypatch):
    """(configure.py runs from the repository's root, and so does this)"""
    monkeypatch.chdir(ROOT)


def graph(tmp_path, monkeypatch, system, machine, guest_cc=None, android_guest_cc=None):
    """build.ninja's linux-arm64 part, as configure.py writes it on |system|"""
    monkeypatch.setattr(linux_arm64_build, "fetch_gl_headers", lambda: None)
    monkeypatch.setattr(linux_arm64_build, "fetch_musl", lambda third_party: None)
    monkeypatch.setattr(linux_arm64_build, "fetch_sdl", lambda third_party: None)
    monkeypatch.setattr(
        linux_arm64_build,
        "platform",
        SimpleNamespace(system=lambda: system, machine=lambda: machine),
    )
    lld_bin = tmp_path / "guest-tools"
    lld_bin.mkdir(exist_ok=True)
    (lld_bin / "ld.lld").touch()
    (lld_bin / "llvm-ar").touch()
    sln = SimpleNamespace(
        guest_lld_bin=str(lld_bin),
        guest_cc=guest_cc,
        android_guest_cc=android_guest_cc,
    )
    out = io.StringIO()
    linux_arm64_build.generate_linux_arm64_build(ninja_syntax.Writer(out), sln)
    # long lines are wrapped with a "$\n" continuation; join them again
    return re.sub(r" \$\n +", " ", out.getvalue())


@pytest.mark.parametrize(
    "system, machine",
    [
        ("Darwin", "arm64"),
        ("Darwin", "x86_64"),
        ("Windows", "AMD64"),
        ("Linux", "x86_64"),
    ],
)
def test_no_host_target_off_linux_arm64(tmp_path, monkeypatch, system, machine):
    text = graph(tmp_path, monkeypatch, system, machine)
    # the guest is emitted whatever the platform is (an aarch64 ld.lld may
    # build it anywhere); the host is Linux arm64 only
    assert "linux_arm64_guest" in text
    assert "linux_arm64_host" not in text
    assert "halo_host" not in text


def test_guest_cc_selects_the_compiler(tmp_path, monkeypatch):
    text = graph(tmp_path, monkeypatch, "Linux", "x86_64", guest_cc="clang-22")
    assert "linux_arm64_guest_cc = clang-22" in text


def test_guest_cc_falls_back_to_android_guest_cc(tmp_path, monkeypatch):
    # --guest-cc is the native Linux arm64 guest's; without it the Android
    # guest's clang is used
    text = graph(tmp_path, monkeypatch, "Linux", "x86_64", android_guest_cc="clang-21")
    assert "linux_arm64_guest_cc = clang-21" in text


@pytest.mark.parametrize("machine", ["aarch64", "arm64"])
def test_host_target_on_linux_arm64(tmp_path, monkeypatch, machine):
    text = graph(tmp_path, monkeypatch, "Linux", machine)
    assert "build build/linux-arm64/halo_host: linux_arm64_host_link" in text
    assert "build linux_arm64_host: phony build/linux-arm64/halo_host" in text
    # built with the system's SDL3 (pkg-config), not the guest's
    assert "pkg-config --cflags sdl3" in text
    assert "pkg-config --libs sdl3" in text
    # glibc declares gettid only with _GNU_SOURCE
    assert "-D_GNU_SOURCE" in text
    assert "-DHALO_LINUX_ARM64_HOST" in text
    # a PIE puts the heap far above 4 GB, clear of the Custom Edition window
    assert "-fPIE" in text
    assert " -pie " in text
    assert "build/linux-arm64/host/host_import_table.c" in text
    # the shared posix, UPnP and mbedTLS sources, and GLES and EGL
    for part in (
        "posix_dtls.c.o",
        "mbedtls_",
        "miniupnpc_",
        "posix_files.c.o",
        "host_linux_data.c.o",
        "xiso.c.o",
    ):
        assert part in text
    assert "pkg-config --libs sdl3 glesv2 egl" in text


def test_ci_starts_the_arm64_host():
    text = (ROOT / ".github" / "workflows" / "build.yml").read_text()
    assert "build/linux-arm64/halo_host" in text
    assert 'tools/ci_smoke.py --no-defaults --require "Halo for Linux starting"' in text
