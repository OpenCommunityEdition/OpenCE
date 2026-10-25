"""The new engine's importer over the real maps: every stock map in
assets/maps, and the Custom Edition maps in assets/custom_maps, are imported
by engine/crates/import's opence-import, whose counts are checked, and then
loaded through the cache folder twice (imported and its pack written, then
read from its pack) with the same counts. Needs nix develop's cargo and the
maps; skipped without them."""

from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
ENGINE = ROOT / "engine"
MAPS = ROOT / "assets" / "maps"
CUSTOM_MAPS = ROOT / "assets" / "custom_maps"

# each stock map's tags, structure BSPs and bitmaps (those its BSPs'
# shaders reach), as the Xbox build 01.01.14.2342's caches hold them
STOCK = {
    "a10": (3357, 9, 74),
    "a30": (3215, 2, 53),
    "a50": (3311, 4, 48),
    "b30": (3325, 2, 49),
    "b40": (3647, 13, 71),
    "beavercreek": (1755, 1, 32),
    "bloodgulch": (1806, 1, 25),
    "boardingaction": (1735, 1, 23),
    "c10": (3369, 6, 46),
    "c20": (1860, 4, 39),
    "c40": (3208, 13, 72),
    "carousel": (1716, 1, 22),
    "chillout": (1702, 1, 32),
    "d20": (2743, 5, 51),
    "d40": (2978, 10, 94),
    "damnation": (1758, 1, 30),
    "hangemhigh": (1681, 1, 25),
    "longest": (1696, 1, 25),
    "prisoner": (1685, 1, 28),
    "putput": (1675, 1, 20),
    "ratrace": (1710, 1, 35),
    "sidewinder": (1757, 1, 32),
    "ui": (983, 1, 0),
    "wizard": (1704, 1, 17),
}

pytestmark = pytest.mark.skipif(
    shutil.which("cargo") is None or not (MAPS / "bloodgulch.map").is_file(),
    reason="needs nix develop's cargo and the stock maps in assets/maps",
)


@pytest.fixture(scope="module")
def importer() -> Path:
    subprocess.run(
        ["cargo", "build", "--release", "--quiet", "-p", "opence_import", "--bin", "opence-import"],
        cwd=ENGINE,
        check=True,
    )
    return ENGINE / "target" / "release" / "opence-import"


def run(importer: Path, maps: list[Path], *options: str) -> dict[str, dict]:
    result = subprocess.run(
        [str(importer), *options, *map(str, maps)],
        capture_output=True,
        text=True,
        timeout=600,
    )
    lines = [json.loads(line) for line in result.stdout.splitlines()]
    return {line["map"]: line for line in lines}


def stock_maps() -> list[Path]:
    return sorted(MAPS.glob("*.map"))


def test_every_stock_map_imports_with_its_counts(importer: Path) -> None:
    levels = run(importer, stock_maps())
    refused = {name: level["error"] for name, level in levels.items() if "error" in level}
    assert refused == {}
    for name, (tags, bsps, bitmaps) in STOCK.items():
        if name not in levels:
            continue  # (a map not in this copy of the game)
        level = levels[name]
        assert level["kind"] == "xbox", name
        assert (level["tags"], level["bsps"], level["bitmaps"]) == (tags, bsps, bitmaps), name
        # every image's pixels are converted for the GPU
        assert level["textures"] == level["images"], name
    assert set(levels) >= {"bloodgulch"}


def test_the_custom_edition_maps_present_import(importer: Path) -> None:
    maps = sorted(CUSTOM_MAPS.glob("*.map"))
    maps = [path for path in maps if path.stem not in ("bitmaps", "sounds", "loc")]
    if not maps:
        pytest.skip("no Custom Edition maps in assets/custom_maps")
    levels = run(importer, maps)
    for name, level in levels.items():
        assert "error" not in level, (name, level.get("error"))
        assert level["kind"] == "custom_edition", name
        assert level["bsps"] >= 1, name


def test_the_cache_folder_imports_once_then_reads_packs(importer: Path, tmp_path: Path) -> None:
    maps = [MAPS / f"{name}.map" for name in ("bloodgulch", "c20", "ui")]
    maps = [path for path in maps if path.is_file()]
    imported = run(importer, maps, "--cache", str(tmp_path))
    assert {level.get("from") for level in imported.values()} == {"map"}
    assert len(list(tmp_path.glob("*.ocpak"))) == len(maps)
    packed = run(importer, maps, "--cache", str(tmp_path))
    assert {level.get("from") for level in packed.values()} == {"pack"}
    for name, level in packed.items():
        del level["from"]
        del imported[name]["from"]
        assert level == imported[name], name
