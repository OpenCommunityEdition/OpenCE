"""OpenCE.app (tools/macos_app.py, port/macos/app): its Info.plist, and the
bundle `ninja macos_app` makes.

The bundle's tests need a built app; the Info.plist and graph tests do not.
"""

import io
import os
import plistlib
import platform
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path
from types import SimpleNamespace

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools import macos_app, ninja_syntax  # noqa: E402

APP = ROOT / "build/macos/OpenCE.app"
IMAGE = ROOT / "build/macos/halo_guest.elf"
CODESIGN = "/usr/bin/codesign"
on_macos = pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only")
built = pytest.mark.skipif(not (APP / "Contents/MacOS/OpenCE").is_file(), reason="needs `ninja macos_app`")


def test_info_plist():
    info = plistlib.loads(macos_app.info_plist("1.2.3"))
    assert info["CFBundleExecutable"] == "OpenCE"
    assert info["CFBundleIdentifier"] == "org.opence.OpenCE"
    assert info["CFBundlePackageType"] == "APPL"
    assert info["CFBundleShortVersionString"] == "1.2.3"
    assert info["LSMinimumSystemVersion"] == "26.0"
    # an AppKit app at the screen's backing resolution
    assert info["NSPrincipalClass"] == "NSApplication"
    assert info["NSHighResolutionCapable"] is True


@pytest.mark.skipif(shutil.which("ninja") is None, reason="needs ninja (nix develop)")
def test_app_graph(monkeypatch):
    from tools.linux_build import generate_linux_build
    from tools.macos_build import generate_macos_build

    monkeypatch.chdir(ROOT)
    sln = SimpleNamespace(build_dir=Path("build"), linux_cc=None, compiler_launcher=None, port_release=False,
                          port_profile=False, port_lto="full", port_portable=False, port_pgo="off",
                          port_pgo_profile=None, macos_guest_cc=None)
    out = io.StringIO()
    n = ninja_syntax.Writer(out)
    n.variable("python", f'"{sys.executable}"')
    generate_linux_build(n, sln)
    generate_macos_build(n, sln)
    flat = re.sub(r" \$\n +", " ", out.getvalue())
    assert re.search(r"^build macos_app: phony build/macos/OpenCE\.app/Contents/MacOS/OpenCE$", flat, re.MULTILINE)
    # the app is linked with the host library and bundles the guest image
    rule = re.search(r"^build build/macos/OpenCE\.app/Contents/MacOS/OpenCE: macos_app (.*)$", flat, re.MULTILINE)
    assert rule
    inputs = rule.group(1)
    assert "build/macos/libhalo_host.a" in inputs and "build/macos/halo_guest.elf" in inputs
    assert "port/macos/app/Sources/OpenCE/" in inputs


@on_macos
@built
def test_bundle_layout():
    contents = APP / "Contents"
    executable = contents / "MacOS/OpenCE"
    assert executable.stat().st_mode & 0o111
    # a 64-bit Mach-O executable for arm64
    magic, cpu, _, kind = struct.unpack_from("<IIII", executable.read_bytes())
    assert (magic, cpu, kind) == (0xFEEDFACF, 0x0100000C, 2)
    info = plistlib.loads((contents / "Info.plist").read_bytes())
    assert info["CFBundleExecutable"] == "OpenCE"
    # the guest image, as built
    assert (contents / "Resources/halo_guest.elf").read_bytes() == IMAGE.read_bytes()


@on_macos
@built
def test_bundle_signature_has_the_hypervisor_entitlement():
    # (Apple's codesign: the Nix shell's, sigtool's, signs only executables)
    verify = subprocess.run([CODESIGN, "--verify", "--strict", str(APP)], capture_output=True, text=True)
    assert verify.returncode == 0, verify.stderr
    entitlements = subprocess.run([CODESIGN, "-d", "--entitlements", "-", "--xml", str(APP)],
                                  capture_output=True)
    assert entitlements.returncode == 0, entitlements.stderr
    assert plistlib.loads(entitlements.stdout).get("com.apple.security.hypervisor") is True


@on_macos
@built
@pytest.mark.skipif(not (ROOT / "assets/maps/ui.map").is_file(), reason="needs assets/maps")
def test_app_runs_the_guest_beside_appkit(tmp_path):
    # the app's guest thread runs the game to the main menu, headless, while
    # AppKit has the main thread; the host ends the process at exit_after
    (tmp_path / "maps").symlink_to(ROOT / "assets/maps")
    (tmp_path / "home").mkdir()
    environment = dict(os.environ, HOME=str(tmp_path / "home"), HALO_DATA_ROOT=str(tmp_path), HALO_EXIT_AFTER="3",
                       HALO_NULL_RENDERER="1", HALO_HIDDEN_WINDOW="1", SDL_VIDEO_DRIVER="dummy",
                       SDL_AUDIO_DRIVER="dummy")
    run = subprocess.run([str(APP / "Contents/MacOS/OpenCE")], cwd=tmp_path, env=environment, capture_output=True,
                         text=True, timeout=120)
    assert run.returncode == 0, run.stderr[-2000:]
    assert "exiting after debug.exit_after" in run.stderr + run.stdout
    assert "starting main menu music" in (tmp_path / "debug.txt").read_text(errors="replace")
