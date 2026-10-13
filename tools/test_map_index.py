"""Tests of the maps' index tool (tools/map_index.py): the torrents and
the index it makes of a folder of Custom Edition maps, what it leaves out,
and the index it checks.

The tests build tiny Custom Edition caches in memory (a valid header and
random tag data), so no game data is needed.
"""
import hashlib
import random
import struct
import subprocess
import zipfile
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import map_index  # noqa: E402

TOOL = ROOT / "tools/map_index.py"


def code(text):
    return struct.unpack(">I", text.encode("latin-1"))[0]


def cache_bytes(name, size=300_000, checksum=0x12345678, version=609, opensauce=False, seed=1):
    """A Custom Edition cache of size bytes: its header, then random data."""
    data = bytearray(random.Random(seed).randbytes(size))
    header = bytearray(0x800)
    struct.pack_into("<I", header, 0, code("head"))
    struct.pack_into("<i", header, 0x04, version)
    struct.pack_into("<I", header, 0x08, size)
    struct.pack_into("<I", header, 0x10, 0x1000)
    struct.pack_into("<I", header, 0x14, 0x2000)
    header[0x20:0x20 + len(name)] = name.encode("latin-1")
    header[0x40:0x40 + 10] = b"01.00.00.0"
    struct.pack_into("<h", header, 0x60, 1)
    struct.pack_into("<I", header, 0x64, checksum)
    if opensauce:
        struct.pack_into("<I", header, 0x70, code("yelo"))
        struct.pack_into("<H", header, 0x76, 1)
    struct.pack_into("<I", header, 0x7FC, code("foot"))
    data[:0x800] = header
    return bytes(data)


def run(*arguments):
    return subprocess.run([sys.executable, "-I", str(TOOL), *arguments], capture_output=True, text=True)


def test_identify_accepts_a_custom_edition_cache(tmp_path):
    path = tmp_path / "test.map"
    path.write_bytes(cache_bytes("test", checksum=0xABCDEF01))
    identity = map_index.identify(path)
    assert identity["name"] == "test"
    assert identity["checksum"] == 0xABCDEF01
    assert identity["scenario_type"] == 1


def test_identify_refuses_what_the_game_refuses(tmp_path):
    (tmp_path / "xbox.map").write_bytes(cache_bytes("xbox", version=5))
    (tmp_path / "yelo.map").write_bytes(cache_bytes("yelo", opensauce=True))
    (tmp_path / "junk.map").write_bytes(b"not a map at all" * 1000)
    for name in ("xbox", "yelo", "junk"):
        with pytest.raises(map_index.MapError):
            map_index.identify(tmp_path / f"{name}.map")


def test_make_writes_torrents_and_the_index(tmp_path):
    maps = tmp_path / "maps"
    maps.mkdir()
    (maps / "alpha.map").write_bytes(cache_bytes("alpha", size=700_000, checksum=0x11111111, seed=1))
    (maps / "beta.map").write_bytes(cache_bytes("beta", size=300_000, checksum=0x22222222, seed=2))
    # another version of alpha: another entry
    (maps / "alpha_v2.map").write_bytes(cache_bytes("alpha", size=700_001, checksum=0x33333333, seed=3))
    (maps / "yelo.map").write_bytes(cache_bytes("yelo", opensauce=True, seed=4))
    resources = tmp_path / "ce"
    resources.mkdir()
    (resources / "bitmaps.map").write_bytes(random.Random(5).randbytes(50_000))
    torrents = tmp_path / "torrents"
    index = tmp_path / "index.txt"
    result = run("make", "--maps", str(maps), "--resource-maps", str(resources), "--torrents", str(torrents),
                 "--index", str(index), "--jobs", "2")
    assert result.returncode == 0, result.stdout + result.stderr
    entries = map_index.read_index(index)
    assert set(entries) == {("alpha.map", 0x11111111), ("alpha_v2.map", 0x33333333), ("beta.map", 0x22222222),
                            ("bitmaps.map", 0)}
    assert "yelo.map: needs OpenSauce" in result.stdout
    for entry in entries.values():
        torrent = map_index.bdecode((torrents / f"{entry.info_hash}.torrent").read_bytes())
        info = torrent[b"info"]
        assert info[b"name"] == entry.file_name.encode()
        assert info[b"length"] == entry.size
        assert info[b"piece length"] == entry.piece_length
        assert hashlib.sha1(map_index.bencode(info)).hexdigest() == entry.info_hash
        # the pieces are the file's
        source = (resources if entry.file_name == "bitmaps.map" else maps) / entry.file_name
        data = source.read_bytes()
        pieces = b"".join(hashlib.sha1(data[offset:offset + entry.piece_length]).digest()
                          for offset in range(0, len(data), entry.piece_length))
        assert info[b"pieces"] == pieces
    # the index checks, and a second run hashes nothing new
    result = run("check", "--index", str(index), "--torrents", str(torrents))
    assert result.returncode == 0, result.stdout
    result = run("make", "--maps", str(maps), "--resource-maps", str(resources), "--torrents", str(torrents),
                 "--index", str(index))
    assert result.returncode == 0, result.stdout + result.stderr
    assert "0 files to hash" in result.stdout


def test_the_info_dictionary_is_what_the_game_rebuilds():
    """The game's seeding host remakes a torrent's info dictionary from the
    index line and the file (torrent.c): exactly {length, name, piece
    length, pieces}, keys in that order."""
    pieces = b"\x01" * 40
    info = map_index.info_dictionary("x.map", 500_000, 262144, pieces)
    assert map_index.bencode(info) == (b"d6:lengthi500000e4:name5:x.map12:piece lengthi262144e6:pieces40:"
                                          + pieces + b"e")


def test_the_committed_index_is_well_formed():
    entries = map_index.read_index(map_index.DEFAULT_INDEX)
    assert len(entries) > 5000
    for (name, checksum), entry in entries.items():
        assert name.endswith(".map")
        assert entry.piece_length >= 262144 and entry.piece_length & (entry.piece_length - 1) == 0
        assert entry.size > 0x800
    assert ("bitmaps.map", 0) in entries and ("sounds.map", 0) in entries and ("loc.map", 0) in entries


def test_sources_notes_the_archives_zips_that_hold_each_map(tmp_path):
    """The archives' zips (halonet: by name, halomaps: <file id>-...) are
    matched to the index by a file's name, size and header checksum; a zip
    with another version of the map is not one of its sources."""
    maps = tmp_path / "maps"
    maps.mkdir()
    alpha = cache_bytes("alpha", size=400_000, checksum=0x11111111, seed=1)
    (maps / "alpha.map").write_bytes(alpha)
    (maps / "beta.map").write_bytes(cache_bytes("beta", size=300_000, checksum=0x22222222, seed=2))
    index = tmp_path / "index.txt"
    result = run("make", "--maps", str(maps), "--torrents", str(tmp_path / "torrents"), "--index", str(index),
                 "--jobs", "2")
    assert result.returncode == 0, result.stdout + result.stderr
    halonet = tmp_path / "halonet"
    halomaps = tmp_path / "halomaps"
    halonet.mkdir()
    halomaps.mkdir()
    with zipfile.ZipFile(halonet / "alpha pack.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("maps/Alpha.map", alpha)
        archive.writestr("readme.txt", "alpha")
    with zipfile.ZipFile(halomaps / "1234-alpha.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("alpha.map", alpha)
    # (another version of beta: not beta's source)
    with zipfile.ZipFile(halomaps / "77-beta.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("beta.map", cache_bytes("beta", size=300_000, checksum=0x99999999, seed=2))
    (halomaps / "not-a-zip.zip").write_bytes(b"junk")
    result = run("sources", "--halonet", str(halonet), "--halomaps", str(halomaps), "--index", str(index))
    assert result.returncode == 0, result.stdout + result.stderr
    entries = map_index.read_index(index)
    assert entries[("alpha.map", 0x11111111)].sources == ["halonet:alpha pack.zip", "halomaps:1234"]
    assert entries[("beta.map", 0x22222222)].sources == []
    # the line the game reads: the sources after the info hash
    line = next(line for line in index.read_text(encoding="utf-8").splitlines() if line.startswith("alpha.map"))
    assert line.split("\t")[5:] == ["halonet:alpha pack.zip", "halomaps:1234"]
    # an archive not given keeps its columns; a rerun gives the same
    result = run("sources", "--halomaps", str(halomaps), "--index", str(index))
    assert result.returncode == 0, result.stdout + result.stderr
    assert map_index.read_index(index)[("alpha.map", 0x11111111)].sources == ["halonet:alpha pack.zip",
                                                                               "halomaps:1234"]
    # and the check takes the columns
    assert run("check", "--index", str(index)).returncode == 0


def test_the_committed_index_names_the_archives_zips():
    entries = map_index.read_index(map_index.DEFAULT_INDEX)
    with_sources = [entry for entry in entries.values() if entry.sources]
    assert len(with_sources) > 5000
    for entry in with_sources:
        assert all(map_index.SOURCE_PATTERN.match(source) for source in entry.sources)
