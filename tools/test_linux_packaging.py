"""Checks the Linux packages' desktop entry and AppStream metadata."""

import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
PACKAGING = ROOT / "packaging" / "linux"
APP_ID = "io.github.opencommunityedition.OpenCE"


def test_metadata():
    component = ET.parse(PACKAGING / (APP_ID + ".metainfo.xml")).getroot()
    assert component.tag == "component"
    assert component.get("type") == "desktop-application"
    assert component.findtext("id") == APP_ID

    launchable = component.find("launchable")
    assert launchable is not None
    assert launchable.get("type") == "desktop-id"
    assert launchable.text == APP_ID + ".desktop"

    # (appimagetool validates these with appstreamcli, which fails the build)
    for attribute in component.iter("content_attribute"):
        assert attribute.text in ("none", "mild", "moderate", "intense"), attribute.get("id")

    lines = (PACKAGING / "opence.desktop").read_text().splitlines()
    assert "Exec=opence" in lines
    assert "Icon=" + APP_ID in lines


def _stage(arch, names, tmp_path):
    import linux_package

    src = tmp_path / "bin"
    src.mkdir()
    for name in names:
        (src / name).write_bytes(b"x")
    out = tmp_path / "AppDir"
    linux_package.stage_appdir(arch, src, out)
    return out


def _check_common(out):
    assert 'HALO_UPDATE_AUTO="${HALO_UPDATE_AUTO:-false}"' in (out / "AppRun").read_text()
    assert (out / (APP_ID + ".png")).is_file()
    assert (out / "usr/share/icons/hicolor/512x512/apps" / (APP_ID + ".png")).is_file()
    assert (out / "usr/share/metainfo" / (APP_ID + ".metainfo.xml")).is_file()
    assert (out / "usr/share/metainfo" / (APP_ID + ".appdata.xml")).is_file()
    desktop = (out / (APP_ID + ".desktop")).read_text().splitlines()
    assert "Exec=AppRun" in desktop
    assert "Exec=opence" not in desktop
    assert (out / "AppRun").stat().st_mode & 0o111 == 0o111


def test_appdir_x86_64(tmp_path):
    out = _stage("x86_64", ["halo", "libSDL3.so.0", "SDL3-LICENSE.txt"], tmp_path)
    for name in ("halo", "libSDL3.so.0", "SDL3-LICENSE.txt"):
        assert (out / "usr/bin" / name).is_file()
    _check_common(out)
    apprun = (out / "AppRun").read_text()
    assert apprun.startswith("#!/bin/sh\n")
    assert 'exec "$HERE/usr/bin/halo" "$@"' in apprun
    assert "HALO_DATA_ROOT" in apprun
    assert "HALO_CONFIG_FILE" in apprun


def test_pack_appimage(tmp_path, monkeypatch):
    import linux_package

    calls = []

    def fake_run(cmd, **kwargs):
        calls.append((cmd, kwargs))

    monkeypatch.setattr(linux_package.subprocess, "run", fake_run)
    linux_package.pack_appimage("x86_64", tmp_path / "AppDir", tmp_path / "out.AppImage", "appimagetool")
    cmd, kwargs = calls[0]
    assert cmd == ["appimagetool", "--appimage-extract-and-run", str(tmp_path / "AppDir"), str(tmp_path / "out.AppImage")]
    assert kwargs["env"]["ARCH"] == "x86_64"
    assert kwargs["check"] is True


def test_appdir_armv7(tmp_path):
    out = _stage("armv7", ["halo", "libSDL3.so.0", "libSDL3.so"], tmp_path)
    for name in ("halo", "libSDL3.so.0", "libSDL3.so"):
        assert (out / "usr/bin" / name).is_file()
    _check_common(out)
    apprun = (out / "AppRun").read_text()
    assert apprun.startswith("#!/bin/sh\n")
    assert 'exec "$HERE/usr/bin/halo" "$@"' in apprun
    # (it must find the SDL3 beside it: the executable has no search path of its own)
    assert 'LD_LIBRARY_PATH="$HERE/usr/bin' in apprun


def test_pack_appimage_armv7(tmp_path, monkeypatch):
    import linux_package

    calls = []
    monkeypatch.setattr(linux_package.subprocess, "run", lambda cmd, **kw: calls.append((cmd, kw)))
    runtime = tmp_path / "runtime-armv7"
    linux_package.pack_appimage(
        "armv7", tmp_path / "AppDir", tmp_path / "out.AppImage", "appimagetool", runtime_file=runtime
    )
    cmd, kwargs = calls[0]
    assert kwargs["env"]["ARCH"] == "armhf"
    assert cmd[cmd.index("--runtime-file") + 1] == str(runtime)
    assert cmd[-2:] == [str(tmp_path / "AppDir"), str(tmp_path / "out.AppImage")]


def test_flatpak_rejects_armv7(tmp_path):
    with pytest.raises(SystemExit):
        _manifest("armv7", ["halo", "libSDL3.so.0"], tmp_path)


def test_appdir_aarch64(tmp_path):
    out = _stage("aarch64", ["halo_host", "halo_guest.elf", "libSDL3.so.0", "libSDL3.so"], tmp_path)
    for name in ("halo_host", "halo_guest.elf", "libSDL3.so.0", "libSDL3.so"):
        assert (out / "usr/bin" / name).is_file()
    _check_common(out)
    apprun = (out / "AppRun").read_text()
    assert apprun.startswith("#!/bin/sh\n")
    assert "LD_LIBRARY_PATH" in apprun
    assert "usr/bin/halo_guest.elf" in apprun
    assert 'exec "$HERE/usr/bin/halo_host" "$@"' in apprun


def _manifest(arch, names, tmp_path):
    import linux_package

    src = tmp_path / "bin"
    src.mkdir()
    for name in names:
        (src / name).write_bytes(b"x")
    return linux_package.flatpak_manifest(arch, src)


def _check_flatpak_common(manifest):
    commands = "\n".join(manifest["modules"][0]["build-commands"])
    assert 'HALO_UPDATE_AUTO="${HALO_UPDATE_AUTO:-false}"' in commands
    assert manifest["app-id"] == APP_ID
    assert manifest["runtime"] == "org.freedesktop.Platform"
    assert manifest["runtime-version"] == "25.08"
    assert manifest["sdk"] == "org.freedesktop.Sdk"
    assert manifest["command"] == "opence"
    assert "--filesystem=xdg-data/halo:create" in manifest["finish-args"]
    # (multiplayer and the other online features need the network)
    assert "--share=network" in manifest["finish-args"]
    (module,) = manifest["modules"]
    assert module["name"] == "opence"
    assert module["buildsystem"] == "simple"
    assert all(s["type"] == "file" for s in module["sources"])
    return module


def test_flatpak_manifest_x86_64(tmp_path):
    manifest = _manifest("x86_64", ["halo", "libSDL3.so.0", "SDL3-LICENSE.txt"], tmp_path)
    module = _check_flatpak_common(manifest)
    paths = [s["path"] for s in module["sources"]]
    for name in ("halo", "libSDL3.so.0", "SDL3-LICENSE.txt"):
        assert name in paths
    commands = "\n".join(module["build-commands"])
    assert "exec /app/bin/halo " in commands
    assert "mkdir -p /app/lib/i386-linux-gnu/GL" in commands
    assert "HALO_CONFIG_FILE" in commands
    ext = manifest["add-extensions"]
    assert ext["org.freedesktop.Platform.Compat.i386"]["directory"] == "lib/i386-linux-gnu"
    assert ext["org.freedesktop.Platform.GL32"]["directory"] == "lib/i386-linux-gnu/GL"
    assert ext["org.freedesktop.Platform.GL32"]["no-autodownload"] is True
    assert ext["org.freedesktop.Platform.GL32"]["add-ld-path"] == "lib"
    # (the NVIDIA driver's 32-bit extension is on branch 1.4: without it in
    # the list it is not mounted and the game draws on the other GPU)
    assert "1.4" in ext["org.freedesktop.Platform.GL32"]["versions"].split(";")
    assert "--allow=multiarch" in manifest["finish-args"]


def test_flatpak_manifest_aarch64(tmp_path):
    manifest = _manifest("aarch64", ["halo_host", "halo_guest.elf", "libSDL3.so.0", "libSDL3.so"], tmp_path)
    module = _check_flatpak_common(manifest)
    paths = [s["path"] for s in module["sources"]]
    for name in ("halo_host", "halo_guest.elf", "libSDL3.so.0", "libSDL3.so"):
        assert name in paths
    assert "add-extensions" not in manifest
    commands = "\n".join(module["build-commands"])
    assert "exec /app/bin/halo_host" in commands
    assert "HALO_GUEST_ELF" in commands


def test_pack_flatpak(tmp_path, monkeypatch):
    import linux_package

    calls = []
    monkeypatch.setattr(linux_package.subprocess, "run", lambda cmd, **kw: calls.append((cmd, kw)))
    linux_package.pack_flatpak("aarch64", tmp_path / "manifest.json", tmp_path / "out.flatpak")
    build, bundle = calls[0][0], calls[1][0]
    assert build[0] == "flatpak-builder"
    assert "--arch=aarch64" in build
    assert "--disable-rofiles-fuse" in build
    assert "--force-clean" in build
    assert build[-1] == str(tmp_path / "manifest.json")
    assert bundle[:3] == ["flatpak", "build-bundle", "--arch=aarch64"]
    assert bundle[-2:] == [str(tmp_path / "out.flatpak"), APP_ID]
    assert all(kw["check"] is True for _, kw in calls)


def test_workflow_builds():
    text = (ROOT / ".github" / "workflows" / "linux-packages.yml").read_text()
    for needle in ("ubuntu-24.04-arm", "release-3.4.16", "bin-x86_64",
                   "bin-aarch64", "ninja linux_arm64_guest linux_arm64_host",
                   "build-armv7", "bin-armv7", "linux_armv7"):
        assert needle in text


def test_workflow_packages():
    path = ROOT / ".github" / "workflows" / "linux-packages.yml"
    text = path.read_text()
    for needle in ("package-x86_64", "package-aarch64", "appimagetool",
                   "flatpak-builder", "tools/linux_package.py",
                   "package-armv7", "packages-armv7", "OpenCE-armv7.AppImage",
                   "runtime-armhf"):
        assert needle in text
    yaml = pytest.importorskip("yaml")
    assert "jobs" in yaml.safe_load(text)


def test_package_builds_are_release_builds():
    # the arm64 and armv7 packages are release builds (assertions off) and, on
    # main, know their build number, as the x86_64 one does through ci_build.py
    text = (ROOT / ".github" / "workflows" / "linux-packages.yml").read_text()
    assert text.count("HALO_BUILD_NUMBER=$GITHUB_RUN_NUMBER") == 2
    assert "configure.py --guest-lld-bin /usr/bin --release" in text
    assert "--lto=off --pgo=off --release" in text


def test_package_jobs_check_what_they_pack():
    text = (ROOT / ".github" / "workflows" / "linux-packages.yml").read_text()
    # the glibc floors port/linux/README.md gives, for the three AppImages
    assert "check_glibc.py 2.29 bin/halo bin/libSDL3.so.0" in text
    assert "check_glibc.py 2.38 bin/halo_host bin/libSDL3.so.0" in text
    assert "check_glibc.py 2.38 bin/halo bin/libSDL3.so.0" in text
    # and the two Flatpaks run in their sandbox
    assert text.count("name: Check the Flatpak") == 2
    assert "test -d /app/lib/i386-linux-gnu/GL && ldd /app/bin/halo" in text


def test_readme_gives_the_glibc_floors_the_ci_checks():
    # (the table's cells may be padded to line up: spaces count once)
    readme = re.sub(r" {2,}", " ", (ROOT / "port" / "linux" / "README.md").read_text())
    for row in ("| x86_64 | 2.29 (32-bit) |", "| arm64 | 2.38 |", "| armv7 | 2.38 (32-bit ARM) |"):
        assert row in readme


def test_armv7_sdl_is_built_with_wayland():
    # SDL leaves Wayland out ("Failed to get wayland-scanner version") unless the
    # build computer has wayland-scanner and the armhf root has the protocols
    for name in ("build.yml", "linux-packages.yml"):
        text = (ROOT / ".github" / "workflows" / name).read_text()
        assert "binfmt-support libwayland-bin" in text
        assert "wayland-protocols libdecor-0-dev" in text


def test_armv7_appimage_has_an_armv7_runtime():
    text = (ROOT / ".github" / "workflows" / "linux-packages.yml").read_text()
    # built from the project's source, pinned, in the ARMv7 container (its own armhf is ARMv6, whose
    # memory barrier a 64-bit kernel without CP15 barrier emulation traps)
    assert "TYPE2_RUNTIME_COMMIT: fedfad51acc2860b2916a1eef32ba4a500ff31ef" in text
    assert "docker_platform=linux/arm/v7" in text
    assert "Tag_CPU_arch: v7" in text and "c7, c10, #0x5" in text
    assert "needs: [build-armv7, runtime-armv7]" in text
    # and not downloaded as the ARMv6 one
    assert "type2-runtime/releases/download/continuous/runtime-armhf" not in text


def test_package_jobs_start_the_appimage_itself():
    text = (ROOT / ".github" / "workflows" / "linux-packages.yml").read_text()
    assert text.count("name: Start the AppImage") == 3
    assert "--platform linux/386" in text and "--platform linux/arm/v7" in text
    for name in ("OpenCE-x86_64.AppImage", "OpenCE-armv7.AppImage", "OpenCE-aarch64.AppImage"):
        assert name + " --appimage-extract-and-run" in text
    # (binfmt does not run an AppImage under qemu: the "AI\x02" marker is in the ELF header's zero bytes)
    assert "qemu-arm-static /pkg/OpenCE-armv7.AppImage --appimage-extract-and-run" in text
    assert "apt-get install -y libgles2 libegl1" in text
