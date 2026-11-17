#!/usr/bin/env python3
"""Stages the built game into an AppDir for the Linux packages.

x86_64 stages the 32-bit game (build/linux layout: halo, libSDL3.so.0,
SDL3-LICENSE.txt); aarch64 stages halo_host, halo_guest.elf and the SDL
libraries found beside them; armv7 (AppImage only) stages the 32-bit ARM halo
and the SDL libraries found beside it.

    python tools/linux_package.py x86_64 build/linux out/AppDir
"""

import argparse
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PACKAGING = ROOT / "packaging" / "linux"
APP_ID = "io.github.opencommunityedition.OpenCE"
DESKTOP = PACKAGING / "opence.desktop"
METAINFO = PACKAGING / (APP_ID + ".metainfo.xml")
ICON = ROOT / "port" / "assets" / "icon" / "opence-icon.png"

# The game's self-updater offers the upstream project's builds, which an
# AppImage (read-only) and a Flatpak cannot apply: off unless asked for.
NO_UPDATE = 'export HALO_UPDATE_AUTO="${HALO_UPDATE_AUTO:-false}"\n'
APPRUN_HEAD = '#!/bin/sh\nHERE=$(dirname "$(readlink -f "$0")")\n' + NO_UPDATE
# The AppImage mounts read-only: the settings and the game's data live in the
# user's data folder, as the Flatpak's do.
APPRUN_STATE = (
    'export HALO_DATA_ROOT="${HALO_DATA_ROOT:-$HOME/.local/share/halo}"\n'
    'export HALO_CONFIG_FILE="${HALO_CONFIG_FILE:-$HALO_DATA_ROOT/config.toml}"\n'
    'mkdir -p "$HALO_DATA_ROOT"\n'
)
APPRUN = {
    "x86_64": APPRUN_HEAD + APPRUN_STATE + 'exec "$HERE/usr/bin/halo" "$@"\n',
    # (the armv7 executable has no search path of its own, unlike the x86_64 portable build's: its SDL3 is
    # found beside it only through this)
    "armv7": APPRUN_HEAD + APPRUN_STATE
    + 'export LD_LIBRARY_PATH="$HERE/usr/bin${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
    + 'exec "$HERE/usr/bin/halo" "$@"\n',
    "aarch64": APPRUN_HEAD
    + 'export LD_LIBRARY_PATH="$HERE/usr/bin${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
    + 'export HALO_GUEST_ELF="$HERE/usr/bin/halo_guest.elf"\n'
    + 'exec "$HERE/usr/bin/halo_host" "$@"\n',
}


def require(path):
    if not Path(path).is_file():
        raise SystemExit(f"missing input file: {path}")
    return Path(path)


def copy(src, dst):
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(require(src), dst)


def stage_appdir(arch, bin_dir, out):
    if arch not in APPRUN:
        raise SystemExit(f"unknown architecture: {arch}")
    bin_dir = Path(bin_dir)
    out = Path(out)
    dest = out / "usr" / "bin"
    if arch == "x86_64":
        names = ["halo", "libSDL3.so.0", "SDL3-LICENSE.txt"]
    else:
        names = ["halo"] if arch == "armv7" else ["halo_host", "halo_guest.elf"]
        libs = sorted(p.name for p in bin_dir.glob("libSDL3.so*"))
        if not libs:
            raise SystemExit(f"missing input file: {bin_dir / 'libSDL3.so*'}")
        names += libs
        if arch == "armv7" and (bin_dir / "SDL3-LICENSE.txt").is_file():
            names.append("SDL3-LICENSE.txt")
    for name in names:
        copy(bin_dir / name, dest / name)
    for name in ("halo", "halo_host"):
        if (dest / name).exists():
            (dest / name).chmod(0o755)

    apprun = out / "AppRun"
    apprun.write_text(APPRUN[arch])
    apprun.chmod(apprun.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    desktop = require(DESKTOP).read_text().replace("Exec=opence", "Exec=AppRun")
    out.mkdir(parents=True, exist_ok=True)
    (out / (APP_ID + ".desktop")).write_text(desktop)
    apps = out / "usr" / "share" / "applications"
    apps.mkdir(parents=True, exist_ok=True)
    (apps / (APP_ID + ".desktop")).write_text(desktop)

    copy(METAINFO, out / "usr" / "share" / "metainfo" / METAINFO.name)
    # (appimagetool looks for the older .appdata.xml name and warns without it)
    copy(METAINFO, out / "usr" / "share" / "metainfo" / (APP_ID + ".appdata.xml"))
    copy(ICON, out / (APP_ID + ".png"))
    copy(ICON, out / "usr" / "share" / "icons" / "hicolor" / "512x512" / "apps" / (APP_ID + ".png"))
    return out


APPIMAGE_ARCH = {"x86_64": "x86_64", "aarch64": "aarch64", "armv7": "armhf"}


def pack_appimage(arch, appdir, output, appimagetool, runtime_file=None):
    # The AppImage runtime is x86_64, so the 32-bit x86 game is packed with ARCH=x86_64.
    env = dict(os.environ, ARCH=APPIMAGE_ARCH[arch])
    cmd = [appimagetool, "--appimage-extract-and-run"]
    if runtime_file is not None:
        cmd += ["--runtime-file", str(runtime_file)]
    subprocess.run(cmd + [str(appdir), str(output)], env=env, check=True)
    return Path(output)


FLATPAK_ARCH = {"x86_64": "x86_64", "aarch64": "aarch64"}
LAUNCHER_HEAD = (
    "#!/bin/sh\n"
    'export LD_LIBRARY_PATH="/app/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
    'export HALO_DATA_ROOT="${HALO_DATA_ROOT:-$HOME/.local/share/halo}"\n'
    + NO_UPDATE
)
LAUNCHER = {
    "x86_64": LAUNCHER_HEAD
    + 'export HALO_CONFIG_FILE="${HALO_CONFIG_FILE:-$HALO_DATA_ROOT/config.toml}"\n'
    + 'exec /app/bin/halo "$@"\n',
    "aarch64": LAUNCHER_HEAD
    + 'export HALO_GUEST_ELF="${HALO_GUEST_ELF:-/app/bin/halo_guest.elf}"\n'
    + 'exec /app/bin/halo_host "$@"\n',
}


def flatpak_binaries(arch, bin_dir):
    if arch == "x86_64":
        return ["halo", "libSDL3.so.0", "SDL3-LICENSE.txt"]
    libs = sorted(p.name for p in Path(bin_dir).glob("libSDL3.so*"))
    if not libs:
        raise SystemExit(f"missing input file: {Path(bin_dir) / 'libSDL3.so*'}")
    return ["halo_host", "halo_guest.elf"] + libs


def flatpak_manifest(arch, bin_dir):
    """Returns the flatpak-builder manifest; source paths are relative to its directory."""
    if arch not in FLATPAK_ARCH:
        raise SystemExit(f"unknown architecture: {arch}")
    binaries = flatpak_binaries(arch, bin_dir)
    exe = "halo" if arch == "x86_64" else "halo_host"
    launcher = LAUNCHER[arch]
    sources = [{"type": "file", "path": name} for name in binaries]
    sources.append({"type": "file", "path": DESKTOP.name, "dest-filename": APP_ID + ".desktop"})
    sources.append({"type": "file", "path": METAINFO.name})
    sources.append({"type": "file", "path": ICON.name, "dest-filename": APP_ID + ".png"})
    commands = [f"install -Dm755 {exe} /app/bin/{exe}"]
    if arch == "aarch64":
        commands.append("install -Dm644 halo_guest.elf /app/bin/halo_guest.elf")
    for name in binaries:
        if name.startswith("libSDL3") or name == "SDL3-LICENSE.txt":
            commands.append(f"install -Dm644 {name} /app/lib/{name}")
    if arch == "x86_64":
        # (the mount points of the extensions: the sandbox cannot make them)
        commands.append("mkdir -p /app/lib/i386-linux-gnu/GL")
    quoted_launcher = launcher.replace("'", "'\\''")
    commands += [
        f"install -Dm644 {APP_ID}.desktop /app/share/applications/{APP_ID}.desktop",
        f"install -Dm644 {METAINFO.name} /app/share/metainfo/{METAINFO.name}",
        f"install -Dm644 {APP_ID}.png /app/share/icons/hicolor/512x512/apps/{APP_ID}.png",
        f"printf '{quoted_launcher}' > /app/bin/opence",
        "chmod 755 /app/bin/opence",
    ]
    manifest = {
        "app-id": APP_ID,
        "runtime": "org.freedesktop.Platform",
        "runtime-version": "25.08",
        "sdk": "org.freedesktop.Sdk",
        "command": "opence",
        "finish-args": [
            "--socket=wayland",
            "--socket=x11",
            "--socket=pulseaudio",
            "--device=all",
            "--share=ipc",
            # (multiplayer, the server browser, map sharing and UPnP need it:
            # without it the sandbox has no network and the game says the
            # System Link cable may be disconnected)
            "--share=network",
            "--filesystem=xdg-data/halo:create",
        ],
        "modules": [
            {"name": "opence", "buildsystem": "simple", "build-commands": commands, "sources": sources}
        ],
    }
    if arch == "x86_64":
        manifest["add-extensions"] = {
            "org.freedesktop.Platform.Compat.i386": {
                "directory": "lib/i386-linux-gnu",
                "version": "25.08",
                "autodelete": True,
            },
            # (as the runtime declares its own GL extension: without the
            # library path and the merged directories the 32-bit Mesa is not
            # found, and the window has no GLX visual)
            "org.freedesktop.Platform.GL32": {
                "directory": "lib/i386-linux-gnu/GL",
                "version": "25.08",
                "versions": "25.08;25.08-extra;1.4",
                "subdirectories": True,
                "autodelete": False,
                "no-autodownload": True,
                "add-ld-path": "lib",
                "merge-dirs": "vulkan/icd.d;glvnd/egl_vendor.d;egl/egl_external_platform.d;OpenCL/vendors;lib/dri;lib/d3d;lib/gbm;vulkan/explicit_layer.d;vulkan/implicit_layer.d;vdpau",
                "download-if": "active-gl-driver",
                "enable-if": "active-gl-driver",
                "autoprune-unless": "active-gl-driver",
            },
        }
        # (the 32-bit game's system calls are not allowed through otherwise)
        manifest["finish-args"].append("--allow=multiarch")
    return manifest


def stage_flatpak(arch, bin_dir, dest):
    """Copies the files into dest and writes manifest.json beside them."""
    dest = Path(dest)
    for name in flatpak_binaries(arch, bin_dir):
        copy(Path(bin_dir) / name, dest / name)
    for src in (DESKTOP, METAINFO, ICON):
        copy(src, dest / src.name)
    path = dest / "manifest.json"
    path.write_text(json.dumps(flatpak_manifest(arch, bin_dir), indent=2) + "\n")
    return path


def pack_flatpak(arch, manifest_path, output):
    with tempfile.TemporaryDirectory() as tmp:
        repo = Path(tmp) / "repo"
        build = Path(tmp) / "build"
        subprocess.run(
            [
                "flatpak-builder",
                "--arch=" + FLATPAK_ARCH[arch],
                "--disable-rofiles-fuse",
                "--force-clean",
                "--repo=" + str(repo),
                str(build),
                str(manifest_path),
            ],
            check=True,
        )
        subprocess.run(
            ["flatpak", "build-bundle", "--arch=" + FLATPAK_ARCH[arch], str(repo), str(output), APP_ID],
            check=True,
        )
    return Path(output)


def main(argv=None):
    argv = sys.argv[1:] if argv is None else list(argv)
    if argv and argv[0] == "flatpak":
        parser = argparse.ArgumentParser(prog="linux_package.py flatpak")
        parser.add_argument("--arch", required=True, choices=sorted(FLATPAK_ARCH))
        parser.add_argument("--bin-dir", required=True, help="directory holding the built files")
        parser.add_argument("--out", required=True, help="Flatpak bundle to write")
        args = parser.parse_args(argv[1:])
        with tempfile.TemporaryDirectory() as tmp:
            manifest = stage_flatpak(args.arch, args.bin_dir, Path(tmp) / "src")
            pack_flatpak(args.arch, manifest, args.out)
        return 0
    if argv and argv[0] == "appimage":
        parser = argparse.ArgumentParser(prog="linux_package.py appimage")
        parser.add_argument("--arch", required=True, choices=sorted(APPRUN))
        parser.add_argument("--bin-dir", required=True, help="directory holding the built files")
        parser.add_argument("--out", required=True, help="AppImage file to write")
        parser.add_argument("--appimagetool", default="appimagetool")
        parser.add_argument("--runtime-file", default=None, help="AppImage runtime to embed")
        args = parser.parse_args(argv[1:])
        with tempfile.TemporaryDirectory() as tmp:
            appdir = stage_appdir(args.arch, args.bin_dir, Path(tmp) / "AppDir")
            pack_appimage(args.arch, appdir, args.out, args.appimagetool, args.runtime_file)
        return 0
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("arch", choices=sorted(APPRUN))
    parser.add_argument("bin_dir", help="directory holding the built files")
    parser.add_argument("out", help="AppDir to write")
    args = parser.parse_args(argv)
    stage_appdir(args.arch, args.bin_dir, args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
