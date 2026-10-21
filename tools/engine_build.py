#!/usr/bin/env python3
"""Builds OpenCE Engine.app, the new engine (conductor/tracks/native_engine_20261021).

  engine_build.py [--out build/engine] [--codesign codesign]

The Rust workspace in engine/ is built as a static library (the ffi crate),
its UniFFI bindings are written into the Swift package in engine/app, Xcode's
Swift builds the package against the library, and the executable is laid out
in a bundle signed ad hoc. With --test, the package's XCTests run instead.
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


def cargo(*args: str) -> None:
    subprocess.run(["cargo", *args], cwd=ENGINE, check=True)


def rust_library() -> Path:
    cargo("build", "--release", "--locked", "-p", "opence_ffi", "--lib")
    return ENGINE / "target" / "release" / LIBRARY


def swift_bindings(library: Path, out: Path) -> None:
    """Writes the UniFFI bindings and the snapshot's header where Package.swift expects them."""
    generated = out / "uniffi"
    shutil.rmtree(generated, ignore_errors=True)
    cargo(
        "run", "--release", "--locked", "-p", "opence_ffi", "--features", "bindgen",
        "--bin", "uniffi-bindgen", "--",
        "generate", "--library", str(library), "--language", "swift", "--out-dir", str(generated),
    )
    ffi = PACKAGE / "Sources" / FFI_MODULE
    core = PACKAGE / "Sources" / "OpenCEEngineCore"
    for folder in (ffi, core):
        shutil.rmtree(folder, ignore_errors=True)
        folder.mkdir(parents=True)
    shutil.copy(generated / f"{FFI_MODULE}.h", ffi / f"{FFI_MODULE}.h")
    subprocess.run(
        [
            "cbindgen", "--quiet", "--config", str(ENGINE / "crates" / "ffi" / "cbindgen.toml"),
            "--crate", "opence_ffi", "--output", str(ffi / SNAPSHOT_HEADER), str(ENGINE / "crates" / "ffi"),
        ],
        check=True,
    )
    (ffi / "module.modulemap").write_text(
        f'module {FFI_MODULE} {{\n    header "{FFI_MODULE}.h"\n    header "{SNAPSHOT_HEADER}"\n    export *\n}}\n'
    )
    shutil.copy(generated / "opence_ffi.swift", core / "opence_ffi.swift")


def swift(command: str, library: Path, out: Path) -> Path:
    """Runs swift build or swift test with Xcode's Swift (nixpkgs' cannot
    link against the macOS 26 SDK); returns the build folder."""
    scratch = out / "swift"
    environment = {
        key: value
        for key, value in os.environ.items()
        if key not in ("SDKROOT", "DEVELOPER_DIR", "MACOSX_DEPLOYMENT_TARGET", "CC", "CXX", "LD")
    }
    subprocess.run(
        [
            "/usr/bin/xcrun", "swift", command, "-c", "release", "--quiet", "--disable-sandbox",
            "--package-path", str(PACKAGE),
            "--scratch-path", str(scratch),
            "--cache-path", str(scratch / "cache"),
            "--config-path", str(scratch / "config"),
            "--security-path", str(scratch / "security"),
            # (and the C++ library Jolt, the physics, needs)
            "-Xlinker", str(library), "-Xlinker", "-lc++",
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
    arguments = parser.parse_args()
    out = arguments.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    library = rust_library()
    swift_bindings(library, out)
    if arguments.test:
        swift("test", library, out)
        return
    executable = swift("build", library, out) / EXECUTABLE
    bundle(executable, arguments.codesign, arguments.version, out / f"{APP_NAME}.app")


if __name__ == "__main__":
    main()
