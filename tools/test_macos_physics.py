"""Jolt Physics in the macOS host (docs/macos_host.md, "Physics").

Jolt is vendored unchanged in port/third_party/jolt. Its README names the
upstream, the release, the commit, the release archive's checksum and a
checksum of the vendored tree, which is checked here, so that an edit to
the copy is caught.
"""

import hashlib
import os
import re
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
JOLT = ROOT / "port/third_party/jolt"
PHYSICS_LIBRARY = ROOT / "build/macos/libhalo_physics.a"


def tree_checksum(folder: Path) -> str:
    """the SHA-256 of each file's path and SHA-256, in path order, without
    the README (which holds the checksum)"""
    digest = hashlib.sha256()
    for path in sorted(p for p in folder.rglob("*") if p.is_file() and p.name != "README.md"):
        relative = path.relative_to(folder).as_posix()
        digest.update(f"{relative} {hashlib.sha256(path.read_bytes()).hexdigest()}\n".encode())
    return digest.hexdigest()


def test_the_readme_names_the_upstream():
    readme = (JOLT / "README.md").read_text(encoding="utf-8")
    assert "https://github.com/jrouwe/JoltPhysics" in readme
    assert "v5.6.0" in readme
    assert re.search(r"\b[0-9a-f]{40}\b", readme), "the release's commit"
    assert re.search(r"archive[^\n]*\n?[^\n]*\b[0-9a-f]{64}\b", readme), "the release archive's SHA-256"
    assert "MIT" in readme and (JOLT / "LICENSE").is_file()
    assert "port/macos/host/physics" in readme, "what uses it"


def test_the_tree_matches_its_checksum():
    readme = (JOLT / "README.md").read_text(encoding="utf-8")
    match = re.search(r"Tree checksum[^`]*`([0-9a-f]{64})`", readme)
    assert match, "the README's tree checksum"
    assert tree_checksum(JOLT) == match.group(1)
    assert (JOLT / "Jolt/Jolt.h").is_file() and (JOLT / "Jolt/Physics/Ragdoll/Ragdoll.h").is_file()


# ---------- the world and the bridge, natively (port/macos/tests/
# host_physics_test.c, host_physics_bridge_test.c)

PROGRAMS = ["host_physics_test", "host_physics_bridge_test"]


@pytest.fixture(scope="module")
def programs(tmp_path_factory):
    built = subprocess.run(["ninja", str(PHYSICS_LIBRARY.relative_to(ROOT))], cwd=ROOT, capture_output=True, text=True)
    assert built.returncode == 0, built.stdout + built.stderr
    folder = tmp_path_factory.mktemp("physics")
    for name in PROGRAMS:
        build = subprocess.run(
            [
                os.environ.get("CC", "cc"),
                "-std=gnu11",
                "-O1",
                "-Wall",
                "-Werror",
                "-mmacosx-version-min=26.0",
                "-Iport/macos/host",
                "-Iport/macos/include",
                f"port/macos/tests/{name}.c",
                str(PHYSICS_LIBRARY),
                "-lc++",
                "-o",
                str(folder / name),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        assert build.returncode == 0, build.stderr
    return folder


def cases():
    found = []
    for name in PROGRAMS:
        source = (ROOT / f"port/macos/tests/{name}.c").read_text(encoding="utf-8")
        found += [(name, case) for case in re.findall(r'\{"(\w+)", \w+\}', source)]
    return found


@pytest.mark.parametrize("program,case", cases())
def test_physics(programs, program, case):
    result = subprocess.run([str(programs / program), case], capture_output=True, text=True, timeout=120)
    assert (result.returncode, result.stdout.strip()) == (0, "ok"), result.stdout + result.stderr
