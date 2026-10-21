"""The new engine's build: the bundle tools/engine_build.py lays out, and the
dependencies' licences (cargo deny). Run inside nix develop."""

from __future__ import annotations

import plistlib
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
ENGINE = ROOT / "engine"


@pytest.fixture(scope="module")
def app(tmp_path_factory: pytest.TempPathFactory) -> Path:
    if shutil.which("cargo") is None or not Path("/usr/bin/xcrun").exists():
        pytest.skip("needs nix develop's cargo and Xcode's Swift")
    out = tmp_path_factory.mktemp("engine")
    subprocess.run(["python3", str(ROOT / "tools" / "engine_build.py"), "--out", str(out)], check=True)
    return out / "OpenCE Engine.app"


def test_the_bundle_holds_the_executable_and_its_plist(app: Path) -> None:
    info = plistlib.loads((app / "Contents" / "Info.plist").read_bytes())
    assert info["CFBundleExecutable"] == "OpenCEEngine"
    assert info["LSMinimumSystemVersion"] == "26.0"
    assert (app / "Contents" / "MacOS" / "OpenCEEngine").is_file()


def test_the_executable_links_the_rust_library(app: Path) -> None:
    result = subprocess.run(
        [str(app / "Contents" / "MacOS" / "OpenCEEngine"), "--version"],
        capture_output=True, text=True, check=True, timeout=30,
    )
    assert result.stdout.strip() == "OpenCE Engine 0.1.0"


def test_the_bundle_is_signed(app: Path) -> None:
    subprocess.run(["/usr/bin/codesign", "--verify", "--strict", str(app)], check=True)


def test_the_dependencies_licences_are_permissive() -> None:
    if shutil.which("cargo-deny") is None:
        pytest.skip("needs nix develop's cargo-deny")
    subprocess.run(
        ["cargo", "deny", "--offline", "check", "licenses", "bans", "sources"],
        cwd=ENGINE, check=True,
    )
