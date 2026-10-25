"""The flake's tooling (rust_tools Phase 5): `nix fmt` formats each language
the repository is written in, the pre-commit hook refuses an unformatted
file, the shell carries nix-output-monitor and direnv's hook, and
`nix flake check` runs the workflow's Rust checks. Skipped without nix."""

from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
SYSTEM = "aarch64-darwin"

pytestmark = pytest.mark.skipif(shutil.which("nix") is None, reason="needs nix")

# each language unformatted, as treefmt finds it under the repository
UNFORMATTED = {
    "engine/crates/a/src/lib.rs": "pub fn a( )->u32{1}\n",
    "engine/app/Sources/B/b.swift": "func b( ) -> Int {return 1}\n",
    "nix/c.nix": "{a=1;}\n",
    "tools/d.py": "x = {  'a':1 }\n",
}


def nix(*args: str, cwd: Path = ROOT) -> str:
    result = subprocess.run(
        ["nix", "--extra-experimental-features", "nix-command flakes", *args],
        cwd=cwd,
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip()


@pytest.fixture(scope="module")
def formatter() -> Path:
    return Path(nix("build", "--no-link", "--print-out-paths", f".#formatter.{SYSTEM}"))


def tree(tmp: Path) -> Path:
    """A git repository holding the unformatted files and the flake's root
    marker (treefmt's projectRootFile)."""
    (tmp / "flake.nix").write_text("{ }\n")
    for name, text in UNFORMATTED.items():
        (tmp / name).parent.mkdir(parents=True, exist_ok=True)
        (tmp / name).write_text(text)
    subprocess.run(["git", "init", "-q"], cwd=tmp, check=True)
    subprocess.run(["git", "add", "."], cwd=tmp, check=True)
    return tmp


def test_nix_fmt_formats_rust_swift_nix_and_python(formatter: Path, tmp_path: Path) -> None:
    repo = tree(tmp_path)
    executable = next((formatter / "bin").iterdir())
    subprocess.run([str(executable)], cwd=repo, check=True, capture_output=True)
    for name, text in UNFORMATTED.items():
        assert (repo / name).read_text() != text, f"{name} was left unformatted"


def test_the_hook_refuses_an_unformatted_file(tmp_path: Path) -> None:
    config = ROOT / ".pre-commit-config.yaml"
    if shutil.which("pre-commit") is None or not config.exists():
        pytest.skip("needs nix develop's pre-commit and its config")
    repo = tree(tmp_path)
    result = subprocess.run(
        ["pre-commit", "run", "treefmt", "-c", str(config), "--files", *UNFORMATTED],
        cwd=repo,
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0, result.stdout


def test_the_shell_carries_nom_and_the_hooks() -> None:
    shell = json.loads(
        nix(
            "eval",
            "--json",
            f".#devShells.{SYSTEM}.default",
            "--apply",
            "s: { inputs = map (p: p.name) s.nativeBuildInputs; hook = s.shellHook; }",
        )
    )
    assert any(name.startswith("nix-output-monitor") for name in shell["inputs"])
    assert ".pre-commit-config.yaml" in shell["hook"]


def test_the_envrc_uses_nix_direnv() -> None:
    envrc = (ROOT / ".envrc").read_text()
    assert "nix_direnv_version" in envrc and "use flake" in envrc


def test_flake_check_runs_the_workflows_checks_and_fails_on_each() -> None:
    checks = json.loads(nix("eval", "--json", f".#checks.{SYSTEM}", "--apply", "builtins.attrNames"))
    assert {"fmt", "clippy", "tests", "deny", "coverage", "treefmt"} <= set(checks)
    # each fails rather than warns: clippy denies its warnings, the tests
    # run in the profile the suite tests in
    clippy = nix("derivation", "show", f".#checks.{SYSTEM}.clippy")
    assert "--deny warnings" in clippy
    profile = nix("eval", "--raw", f".#checks.{SYSTEM}.tests.CARGO_PROFILE")
    assert profile == "test-opt"
