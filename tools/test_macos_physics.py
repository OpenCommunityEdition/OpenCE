"""Jolt Physics in the macOS host (docs/macos_host.md, "Physics").

Jolt is vendored unchanged in port/third_party/jolt. Its README names the
upstream, the release, the commit, the release archive's checksum and a
checksum of the vendored tree, which is checked here, so that an edit to
the copy is caught.
"""

import hashlib
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
JOLT = ROOT / "port/third_party/jolt"


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
