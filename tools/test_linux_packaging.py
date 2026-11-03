"""Checks the Linux packages' desktop entry and AppStream metadata."""

import sys
import xml.etree.ElementTree as ET
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
PACKAGING = ROOT / "packaging" / "linux"
APP_ID = "io.github.fullhavocjosh.OpenCE"


def test_metadata():
    component = ET.parse(PACKAGING / (APP_ID + ".metainfo.xml")).getroot()
    assert component.tag == "component"
    assert component.get("type") == "desktop-application"
    assert component.findtext("id") == APP_ID

    launchable = component.find("launchable")
    assert launchable is not None
    assert launchable.get("type") == "desktop-id"
    assert launchable.text == APP_ID + ".desktop"

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
    assert (out / (APP_ID + ".png")).is_file()
    assert (out / "usr/share/icons/hicolor/512x512/apps" / (APP_ID + ".png")).is_file()
    assert (out / "usr/share/metainfo" / (APP_ID + ".metainfo.xml")).is_file()
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
    assert manifest["app-id"] == APP_ID
    assert manifest["runtime"] == "org.freedesktop.Platform"
    assert manifest["runtime-version"] == "24.08"
    assert manifest["sdk"] == "org.freedesktop.Sdk"
    assert manifest["command"] == "opence"
    assert "--filesystem=xdg-data/halo:create" in manifest["finish-args"]
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
    assert "exec /app/bin/halo " in "\n".join(module["build-commands"])
    ext = manifest["add-extensions"]
    assert ext["org.freedesktop.Platform.Compat.i386"]["directory"] == "lib/i386-linux-gnu"
    assert ext["org.freedesktop.Platform.GL32"]["directory"] == "lib/i386-linux-gnu/GL"
    assert ext["org.freedesktop.Platform.GL32"]["no-autodownload"] is True


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
                   "bin-aarch64", "ninja linux_arm64_guest linux_arm64_host"):
        assert needle in text


def test_workflow_packages():
    path = ROOT / ".github" / "workflows" / "linux-packages.yml"
    text = path.read_text()
    for needle in ("package-x86_64", "package-aarch64", "appimagetool",
                   "flatpak-builder", "tools/linux_package.py"):
        assert needle in text
    yaml = pytest.importorskip("yaml")
    assert "jobs" in yaml.safe_load(text)
