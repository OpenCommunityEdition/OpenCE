"""The new engine's build: the bundle tools/engine_build.py lays out, and the
dependencies' licences (cargo deny) and the test step. Run inside nix develop."""

from __future__ import annotations

import plistlib
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
ENGINE = ROOT / "engine"

sys.path.insert(0, str(ROOT / "tools"))

import engine_build  # noqa: E402


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


def test_the_swift_contract_tests_pass(app: Path, tmp_path: Path) -> None:
    # (after the app fixture, which has checked the tools are there)
    subprocess.run(["python3", str(ROOT / "tools" / "engine_build.py"), "--out", str(tmp_path), "--test",
                    "--suite", "swift"], check=True)


def test_the_dependencies_licences_are_permissive() -> None:
    if shutil.which("cargo-deny") is None:
        pytest.skip("needs nix develop's cargo-deny")
    subprocess.run(engine_build.deny_command(), cwd=ENGINE, check=True)


def test_a_forbidden_licence_fails_the_check(tmp_path: Path) -> None:
    if shutil.which("cargo-deny") is None:
        pytest.skip("needs nix develop's cargo-deny")
    allowed = (ENGINE / "deny.toml").read_text()
    # (half the dependencies are under MIT alone)
    assert '    "MIT",\n' in allowed
    forbidding = tmp_path / "deny.toml"
    forbidding.write_text(allowed.replace('    "MIT",\n', ""))
    run = subprocess.run(engine_build.deny_command(forbidding), cwd=ENGINE, capture_output=True, text=True)
    assert run.returncode != 0
    assert "rejected" in run.stderr


def test_the_test_step_checks_the_licences_then_runs_nextest_optimised() -> None:
    deny, nextest = engine_build.rust_test_commands()
    assert deny == engine_build.deny_command()
    assert nextest[:3] == ["cargo", "nextest", "run"]
    assert "--workspace" in nextest
    assert nextest[nextest.index("--cargo-profile") + 1] == "test-opt"

