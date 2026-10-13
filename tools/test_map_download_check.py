"""The map downloads' file handling (port/linux/src/map_archives_zip.c) alone,
as tools/map_download_check.c runs it outside the game (ninja linux builds it
as build/linux/map_download_check): a map taken out of made-up zips, damaged
ones refused, and a file checked against the info hash the index tool
(tools/map_index.py) gives it, another file refused. No game data and no
network.
"""
import hashlib
import random
import subprocess
import sys
import zipfile
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import map_index  # noqa: E402

CHECK = ROOT / "build/linux/map_download_check"
# (a few pieces of 256 KB, the last a short one)
FILE_SIZE = 3 * 256 * 1024 + 12345
PIECE_LENGTH = 256 * 1024

needs_check = pytest.mark.skipif(not CHECK.exists(), reason="needs ninja linux's build/linux/map_download_check")


def run(*arguments):
    return subprocess.run([str(CHECK), *map(str, arguments)], capture_output=True, text=True, timeout=60)


def info_hash(name, data, piece_length):
    pieces = b"".join(hashlib.sha1(data[offset:offset + piece_length]).digest()
                      for offset in range(0, len(data), piece_length))
    return hashlib.sha1(map_index.bencode(map_index.info_dictionary(name, len(data), piece_length, pieces))).hexdigest()


@needs_check
def test_a_map_is_taken_out_of_an_archives_zip(tmp_path):
    """the game's unzip: the file of the name and size in any folder,
    deflated or stored; anything else refused"""
    data = random.Random(9).randbytes(FILE_SIZE)
    for method in (zipfile.ZIP_DEFLATED, zipfile.ZIP_STORED):
        archive_path = tmp_path / f"pack{method}.zip"
        with zipfile.ZipFile(archive_path, "w", method) as archive:
            archive.writestr("readme.txt", "a map")
            # (one of the name but another size first: not it)
            archive.writestr("old/Made_Up.map", data[:1000])
            archive.writestr("maps/Made_Up.map", data)
        out = tmp_path / f"out{method}.map"
        result = run("unzip", archive_path, "made_up.map", FILE_SIZE, out)
        assert result.returncode == 0, result.stdout + result.stderr
        assert out.read_bytes() == data
    # no such file, a damaged zip, a zip cut short: nothing written
    whole = (tmp_path / f"pack{zipfile.ZIP_DEFLATED}.zip").read_bytes()
    out = tmp_path / "bad.map"
    for label, zip_bytes, size in (("no file of that size", whole, FILE_SIZE + 1),
                                   ("damaged data", whole[:2000] + bytes(len(whole) - 4000) + whole[-2000:], FILE_SIZE),
                                   ("cut short", whole[:len(whole) // 2], FILE_SIZE),
                                   ("not a zip", b"junk" * 100, FILE_SIZE)):
        bad = tmp_path / "bad.zip"
        bad.write_bytes(zip_bytes)
        result = run("unzip", bad, "made_up.map", size, out)
        assert result.returncode != 0, label
        assert "failed" in result.stdout, label
        assert not out.exists(), label


@needs_check
def test_a_download_is_checked_against_the_index(tmp_path):
    """the game's check (map_archives_verify): a file is the index's when its
    info hash, as tools/map_index.py makes it, is the index's; another
    version of it, one cut short, or the hash of another name are not"""
    data = random.Random(11).randbytes(FILE_SIZE)
    good = tmp_path / "Made_Up.map"
    good.write_bytes(data)
    expected = info_hash("Made_Up.map", data, PIECE_LENGTH)
    result = run("verify", good, "Made_Up.map", FILE_SIZE, PIECE_LENGTH, expected)
    assert result.returncode == 0, result.stdout + result.stderr
    assert "verified" in result.stdout
    # (the hash's case does not matter)
    assert run("verify", good, "Made_Up.map", FILE_SIZE, PIECE_LENGTH, expected.upper()).returncode == 0
    other = bytearray(data)
    other[FILE_SIZE // 2] ^= 1
    changed = tmp_path / "changed.map"
    changed.write_bytes(bytes(other))
    short = tmp_path / "short.map"
    short.write_bytes(data[:-1])
    for label, path, name in (("another version", changed, "Made_Up.map"),
                              ("cut short", short, "Made_Up.map"),
                              ("another name", good, "made_up.map")):
        result = run("verify", path, name, FILE_SIZE, PIECE_LENGTH, expected)
        assert result.returncode != 0, label
        assert "failed" in result.stdout, label


def test_the_index_is_read_by_the_game():
    """the committed index starts as map_downloads.c reads it (INDEX_FORMAT),
    and each entry has the columns it reads"""
    text = (ROOT / "port/assets/network/map_index.txt").read_text(encoding="utf-8")
    assert text.startswith("# halo map index v1\n")
    source = (ROOT / "port/linux/src/map_downloads.c").read_text(encoding="utf-8")
    assert '#define INDEX_FORMAT "# halo map index v1"' in source
    entries = [line.split("\t") for line in text.splitlines() if line and not line.startswith("#")]
    assert len(entries) > 5000
    for fields in entries:
        assert len(fields) >= 5 and len(fields[4]) == 40, fields[0]
