"""Native HS files in downloadable mutator manifests."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
from mutator_index import RefusedError, mutator_entry


def test_native_script_manifest(tmp_path):
    folder = tmp_path / "native"
    folder.mkdir()
    (folder / "mutator.toml").write_text('format = 5\n[script]\nfile = "main.hsc"\n')
    source = b"(script static void frame (begin))"
    (folder / "main.hsc").write_bytes(source)
    entry = mutator_entry(folder)
    assert [file["path"] for file in entry["files"]] == ["mutator.toml", "main.hsc"]
    assert entry["files"][1]["size"] == len(source)


@pytest.mark.parametrize("name", ["main.lua", "main.txt", "../main.hsc", ".main.hsc", "nested/main.hsc", "nested\\main.hsc"])
def test_invalid_script_names(tmp_path, name):
    folder = tmp_path / "invalid"
    folder.mkdir()
    # TOML literal strings keep the backslash case intact.
    (folder / "mutator.toml").write_text(f"format = 5\n[script]\nfile = '{name}'\n")
    with pytest.raises(RefusedError, match="script is a .hsc file's name"):
        mutator_entry(folder)
