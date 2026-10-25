#!/usr/bin/env python3
"""Builds OpenCE Engine.app, the new engine (conductor/tracks/native_engine_20261021).

  engine_build.py [--out build/engine] [--codesign codesign] [--rust folder]
  engine_build.py --rust-only folder

The Rust workspace in engine/ is built as a static library (the ffi crate)
and its UniFFI bindings are written (the Rust half, in out/rust), the
bindings are copied into the Swift package in engine/app, Xcode's Swift
builds the package against the library, and the executable is laid out in a
bundle signed ad hoc. --rust-only builds the Rust half alone, as the flake's
engine-rust package does with crane; --rust takes a Rust half built already
(nix build .#engine-rust) instead of running cargo. The shaders are compiled first with Xcode's Metal
toolchain (xcodebuild -downloadComponent MetalToolchain), so that an error in
one stops the build rather than the app's start. With --test, the tests run
instead: cargo deny and the workspace's tests under nextest, then the
package's XCTests (--suite picks one).
Run it inside nix develop, or from the flake's engine package.
"""

from __future__ import annotations

import argparse
import os
import plistlib
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ENGINE = ROOT / "engine"
PACKAGE = ENGINE / "app"
APP_NAME = "OpenCE Engine"
EXECUTABLE = "OpenCEEngine"
LIBRARY = "libopence_ffi.a"
FFI_MODULE = "opence_ffiFFI"
SNAPSHOT_HEADER = "opence_snapshot.h"
# where macOS mounts the Metal toolchain Xcode downloads
CRYPTEXES = "/private/var/run/com.apple.security.cryptexd/mnt"


def cargo(*args: str) -> None:
    subprocess.run(["cargo", *args], cwd=ENGINE, check=True)


def deny_command(config: Path = ENGINE / "deny.toml") -> list[str]:
    """cargo deny's licence, ban and source checks (the advisories need the
    network)."""
    return ["cargo", "deny", "--offline", "--config", str(config), "check", "licenses", "bans", "sources"]


def rust_test_commands(profile: str = "test-opt") -> list[list[str]]:
    """The Rust half's test step: the dependencies' licences, then the
    workspace's tests under nextest (engine/.config/nextest.toml), built
    optimised with the debug checks kept."""
    return [
        deny_command(),
        ["cargo", "nextest", "run", "--locked", "--workspace", "--cargo-profile", profile],
    ]


def rust_half(rust: Path) -> None:
    """Builds the library into rust/lib, and writes its UniFFI bindings and
    the snapshot's header into rust/bindings."""
    cargo("build", "--release", "--locked", "-p", "opence_ffi", "--lib")
    target = Path(os.environ.get("CARGO_TARGET_DIR", ENGINE / "target"))
    bindings = rust / "bindings"
    for folder in (rust / "lib", bindings):
        shutil.rmtree(folder, ignore_errors=True)
        folder.mkdir(parents=True)
    library = rust / "lib" / LIBRARY
    shutil.copy(target / "release" / LIBRARY, library)
    cargo(
        "run",
        "--release",
        "--locked",
        "-p",
        "opence_ffi",
        "--features",
        "bindgen",
        "--bin",
        "uniffi-bindgen",
        "--",
        "generate",
        "--library",
        str(library),
        "--language",
        "swift",
        "--out-dir",
        str(bindings),
    )
    subprocess.run(
        [
            "cbindgen",
            "--quiet",
            "--config",
            str(ENGINE / "crates" / "ffi" / "cbindgen.toml"),
            "--crate",
            "opence_ffi",
            "--output",
            str(bindings / SNAPSHOT_HEADER),
            str(ENGINE / "crates" / "ffi"),
        ],
        check=True,
    )


def swift_bindings(rust: Path) -> None:
    """Copies the Rust half's bindings where Package.swift expects them."""
    bindings = rust / "bindings"
    ffi = PACKAGE / "Sources" / FFI_MODULE
    core = PACKAGE / "Sources" / "OpenCEEngineCore"
    for folder in (ffi, core):
        shutil.rmtree(folder, ignore_errors=True)
        folder.mkdir(parents=True)
    for header in (f"{FFI_MODULE}.h", SNAPSHOT_HEADER):
        shutil.copy(bindings / header, ffi / header)
    (ffi / "module.modulemap").write_text(
        f'module {FFI_MODULE} {{\n    header "{FFI_MODULE}.h"\n    header "{SNAPSHOT_HEADER}"\n    export *\n}}\n'
    )
    shutil.copy(bindings / "opence_ffi.swift", core / "opence_ffi.swift")


def xcode_environment() -> dict[str, str]:
    """The environment without nix develop's SDK and compilers, for Xcode's tools."""
    return {
        key: value
        for key, value in os.environ.items()
        if key not in ("SDKROOT", "DEVELOPER_DIR", "MACOSX_DEPLOYMENT_TARGET", "CC", "CXX", "LD")
    }


def metal_compiler(environment: dict[str, str]) -> list[str]:
    """The command that compiles a shader. xcrun finds the Metal toolchain
    only for the user who downloaded it, not for nix's build users, so
    without it the toolchain's mounted copy is used with the SDK's root."""
    command = ["/usr/bin/xcrun", "-sdk", "macosx", "metal"]
    found = subprocess.run([*command, "--version"], env=environment, capture_output=True, text=True)
    if found.returncode == 0:
        return command
    mounted = sorted(Path(CRYPTEXES).glob("com.apple.MobileAsset.MetalToolchain-*/Metal.xctoolchain/usr/bin/metal"))
    if not mounted:
        raise SystemExit("Xcode's Metal toolchain is not installed: xcodebuild -downloadComponent MetalToolchain")
    sdk = subprocess.run(
        ["/usr/bin/xcrun", "-sdk", "macosx", "--show-sdk-path"],
        env=environment,
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()
    return [str(mounted[-1]), "-isysroot", sdk]


def check_shaders(out: Path) -> None:
    """Compiles each of the renderer's shaders as the app will at run time
    (ShaderSources.swift), failing on the first that has an error."""
    folder = out / "shaders"
    folder.mkdir(parents=True, exist_ok=True)
    environment = xcode_environment()
    compiler = metal_compiler(environment)
    for shader in sorted((PACKAGE / "Shaders").glob("*.metal")):
        made = subprocess.run(
            [*compiler, "-c", str(shader), "-o", str(folder / f"{shader.stem}.air")],
            env=environment,
            capture_output=True,
            text=True,
        )
        if made.returncode != 0:
            raise SystemExit(f"{shader.name} does not compile:\n{made.stderr}")


def swift(command: str, library: Path, out: Path, extra: tuple[str, ...] = ()) -> Path:
    """Runs swift build or swift test with Xcode's Swift (nixpkgs' cannot
    link against the macOS 26 SDK); returns the build folder."""
    scratch = out / "swift"
    environment = xcode_environment()
    subprocess.run(
        [
            "/usr/bin/xcrun",
            "swift",
            command,
            "-c",
            "release",
            "--quiet",
            "--disable-sandbox",
            "--package-path",
            str(PACKAGE),
            "--scratch-path",
            str(scratch),
            "--cache-path",
            str(scratch / "cache"),
            "--config-path",
            str(scratch / "config"),
            "--security-path",
            str(scratch / "security"),
            # (and the C++ library Jolt, the physics, needs)
            "-Xlinker",
            str(library),
            "-Xlinker",
            "-lc++",
            *extra,
        ],
        env=environment,
        check=True,
    )
    return scratch / "release"


def info_plist(version: str) -> bytes:
    return plistlib.dumps(
        {
            "CFBundleDevelopmentRegion": "en",
            "CFBundleExecutable": EXECUTABLE,
            "CFBundleIdentifier": "org.opence.engine",
            "CFBundleInfoDictionaryVersion": "6.0",
            "CFBundleName": APP_NAME,
            "CFBundleDisplayName": APP_NAME,
            "CFBundlePackageType": "APPL",
            "CFBundleShortVersionString": version,
            "CFBundleVersion": version,
            "LSMinimumSystemVersion": "26.0",
            "NSHighResolutionCapable": True,
            "NSPrincipalClass": "NSApplication",
        }
    )


def bundle(executable: Path, codesign: str, version: str, app: Path) -> None:
    """Lays out the bundle in a staging folder and renames it into place."""
    staging = app.with_name(app.name + ".partial")
    shutil.rmtree(staging, ignore_errors=True)
    (staging / "Contents" / "MacOS").mkdir(parents=True)
    shutil.copy(executable, staging / "Contents" / "MacOS" / EXECUTABLE)
    (staging / "Contents" / "Info.plist").write_bytes(info_plist(version))
    # the HUD's redraws (HudPass.swift)
    shutil.copytree(ROOT / "port" / "assets" / "hud", staging / "Contents" / "Resources" / "hud")
    # the renderer's shaders, compiled at run time (ShaderSources.swift)
    shutil.copytree(ROOT / "engine" / "app" / "Shaders", staging / "Contents" / "Resources" / "shaders")
    # the menus' fonts and the shell's, with their licences (MenuFonts.swift,
    # TextPass.swift): Overpass and OpenCE only
    fonts = staging / "Contents" / "Resources" / "fonts"
    fonts.mkdir()
    for name in (
        "Overpass-750.ttf",
        "Overpass-900.ttf",
        "Overpass-OFL.txt",
        "OpenCE-Regular.ttf",
        "OpenCE-OFL.txt",
        "OverpassMono-Regular.ttf",
        "OverpassMono-OFL.txt",
    ):
        shutil.copy(ROOT / "port" / "assets" / "fonts" / name, fonts / name)
    subprocess.run([codesign, "-s", "-", "-f", str(staging)], check=True)
    shutil.rmtree(app, ignore_errors=True)
    staging.rename(app)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", type=Path, default=ROOT / "build" / "engine")
    # (Apple's: sigtool signs Mach-O files only, not bundles, as
    # tools/macos_build.py's HALO_MACOS_APP_CODESIGN)
    parser.add_argument("--codesign", default=os.environ.get("HALO_MACOS_APP_CODESIGN", "/usr/bin/codesign"))
    parser.add_argument("--version", default="0.1.0")
    parser.add_argument("--test", action="store_true", help="run the package's XCTests instead of making the app")
    parser.add_argument("--filter", help="with --test, only the tests whose names match this")
    parser.add_argument(
        "--suite",
        choices=("all", "rust", "swift"),
        default="all",
        help="with --test, the Rust half's tests (cargo deny and nextest), the XCTests, or both",
    )
    parser.add_argument("--rust", type=Path, help="the Rust half built already, instead of running cargo")
    parser.add_argument("--rust-only", type=Path, help="build only the Rust half, into this folder")
    arguments = parser.parse_args()
    if arguments.rust_only:
        rust_half(arguments.rust_only.resolve())
        return
    if arguments.test and arguments.suite != "swift":
        for command in rust_test_commands():
            subprocess.run(command, cwd=ENGINE, check=True)
        if arguments.suite == "rust":
            return
    out = arguments.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    check_shaders(out)
    rust = arguments.rust.resolve() if arguments.rust else out / "rust"
    if not arguments.rust:
        rust_half(rust)
    swift_bindings(rust)
    library = rust / "lib" / LIBRARY
    if arguments.test:
        swift("test", library, out, ("--filter", arguments.filter) if arguments.filter else ())
        return
    executable = swift("build", library, out) / EXECUTABLE
    bundle(executable, arguments.codesign, arguments.version, out / f"{APP_NAME}.app")


if __name__ == "__main__":
    main()
