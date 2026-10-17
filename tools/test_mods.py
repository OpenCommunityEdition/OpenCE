"""Mods (docs/mods.md) in the game: a mod of tag files made here (a sound, a bitmap and a unicode string list, as the
Halo Editing Kit writes them) played over Blood Gulch's tags, the same files damaged, which the game must leave out,
and the load order: alphabetical, moved as mods ask, as load_order.toml keeps it, and mods that say they cannot be
played together. Needs the Linux build (build/linux/halo), assets/maps/bloodgulch.map and xvfb-run."""

import os
import shutil
import signal
import struct
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
GAME = ROOT / "build" / "linux" / "halo"
MAPS = ROOT / "assets" / "maps"
LEVEL = "levels\\test\\bloodgulch\\bloodgulch"

# the map's tags the mod replaces
SOUND = "sound/sfx/weapons/assault rifle/fire"
BITMAP = "levels/test/bloodgulch/bitmaps/blood cliff rock"
STRINGS = "ui/hud/hud_item_messages"


def header(group: bytes, version: int) -> bytes:
    """a tag file's 0x40-byte header: its group, its size, its group's version and its signature"""
    data = bytearray(0x40)
    data[0x24:0x28] = group
    struct.pack_into(">I", data, 0x2C, 0x40)
    struct.pack_into(">H", data, 0x38, version)
    data[0x3C:0x40] = b"blam"
    return bytes(data)


def sound_file(frames: int = 2205) -> bytes:
    """a sound of one pitch range of one permutation: mono 16-bit PCM at 22 kHz (sound_definitions.h)"""
    samples = b"".join(struct.pack(">h", (index % 64 - 32) * 256) for index in range(frames))
    root = bytearray(0xA4)
    struct.pack_into(">h", root, 0x04, 4)  # sound_class: weapon fire
    struct.pack_into(">ff", root, 0x14, 1.0, 1.0)  # random_pitch_bounds
    struct.pack_into(">I", root, 0x70, int.from_bytes(b"snd!", "big"))  # promotion_sound, none
    struct.pack_into(">i", root, 0x7C, -1)
    struct.pack_into(">I", root, 0x98, 1)  # pitch_ranges
    pitch_range = bytearray(0x48)
    pitch_range[0:7] = b"default"
    struct.pack_into(">f", pitch_range, 0x20, 1.0)  # natural_pitch
    struct.pack_into(">h", pitch_range, 0x2C, 1)  # actual_permutation_count
    struct.pack_into(">I", pitch_range, 0x3C, 1)  # permutations
    permutation = bytearray(0x7C)
    permutation[0:4] = b"fire"
    struct.pack_into(">f", permutation, 0x24, 1.0)  # gain
    struct.pack_into(">hh", permutation, 0x28, 0, -1)  # compression: none; next_permutation_index
    struct.pack_into(">I", permutation, 0x40, len(samples))  # samples
    return header(b"snd!", 4) + bytes(root) + bytes(pitch_range) + bytes(permutation) + samples


def bitmap_file(count: int = 1, size: int = 32) -> bytes:
    """a bitmap of one sequence of `count` 2D A8R8G8B8 bitmaps (bitmap_group.h)"""
    pixels_size = size * size * 4
    pixels = b"".join(bytes([0x40 + index * 16, 0x80, 0xC0, 0xFF]) * (size * size) for index in range(count))
    root = bytearray(0x6C)
    struct.pack_into(">I", root, 0x30, len(pixels))  # processed pixel data
    struct.pack_into(">I", root, 0x54, 1)  # sequences
    struct.pack_into(">I", root, 0x60, count)  # bitmaps
    sequence = bytearray(0x40)
    sequence[0:5] = b"rocks"
    struct.pack_into(">hh", sequence, 0x20, 0, count)
    bitmaps = b""
    for index in range(count):
        bitmap = bytearray(0x30)
        bitmap[0:4] = b"bitm"
        struct.pack_into(">hhhhhH", bitmap, 4, size, size, 1, 0, 11, 1)  # 2D, A8R8G8B8, power of two
        struct.pack_into(">i", bitmap, 0x18, index * pixels_size)
        bitmaps += bytes(bitmap)
    return header(b"bitm", 7) + bytes(root) + pixels + bytes(sequence) + bitmaps


def strings_file(texts: list) -> bytes:
    """a unicode string list (text_group.h): its strings in UTF-16, terminated"""
    encoded = [(text + "\0").encode("utf-16-le") for text in texts]
    root = struct.pack(">I", len(texts)) + bytes(8)
    entries = b"".join(struct.pack(">I", len(data)) + bytes(16) for data in encoded)
    return header(b"ustr", 1) + root + entries + b"".join(encoded)


@pytest.fixture(scope="module")
def game(tmp_path_factory) -> Path:
    """the Linux build, copied with its SDL to a folder of its own: its config.toml, which it writes beside
    itself, is the test's (not build/linux's)"""
    if not GAME.is_file() or not (MAPS / "bloodgulch.map").is_file() or not shutil.which("xvfb-run"):
        pytest.skip("needs build/linux/halo, assets/maps/bloodgulch.map and xvfb-run")
    folder = tmp_path_factory.mktemp("game")
    for name in ("halo", "libSDL3.so.0"):
        if (GAME.parent / name).is_file():
            shutil.copy2(GAME.parent / name, folder / name)
    return folder / "halo"


def run_game(game: Path, data: Path, saves: Path) -> str:
    """Blood Gulch with the mods, for a few seconds: the mods' lines of the output and debug.txt"""
    (data / "maps").symlink_to(MAPS.resolve())
    (data / "init.txt").write_text(f"map_name {LEVEL}\n", encoding="ascii")
    # (a window, hidden on a display of its own: debug.exit_after ends a
    # game that has one, and the textures are loaded, a bitmap's from its
    # file; such a run is never asked about mods that conflict)
    environment = {**os.environ, "HALO_DATA_ROOT": str(data), "HALO_SAVE_ROOT": str(saves),
                   "HALO_EXIT_AFTER": "25", "HALO_HIDDEN_WINDOW": "1", "SDL_VIDEO_DRIVER": "x11",
                   "HALO_DISPLAY_MODE": "windowed", "HALO_WINDOW_SIZE": "640x480",
                   "HALO_RESOLUTION_SCALING": "original", "HALO_VOLUME": "0", "HALO_NET_ONLINE": "0"}
    environment.pop("WAYLAND_DISPLAY", None)
    process = subprocess.Popen(["xvfb-run", "-a", "-s", "-screen 0 640x480x24", str(game)], cwd=game.parent,
                            env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                            start_new_session=True)
    try:
        output, _ = process.communicate(timeout=180)
    except subprocess.TimeoutExpired:
        os.killpg(process.pid, signal.SIGKILL)
        process.communicate()
        pytest.fail("the game did not exit")
    assert process.returncode == 0, output[-4000:]
    # (the mods found at startup are logged before debug.txt opens: on the output)
    log = output + (data / "debug.txt").read_text(errors="replace")
    return "\n".join(line for line in log.splitlines() if "mods:" in line)


def write_mod(data: Path, files: dict, folder: str = "test", toml: str = 'name = "Test"\nversion = "1"\n') -> None:
    mod = data / "mods" / folder
    (mod / "tags").mkdir(parents=True)
    (mod / "mod.toml").write_text(toml, encoding="utf-8")
    for path, content in files.items():
        target = mod / "tags" / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)


def test_a_mod_plays_over_the_map(game, tmp_path):
    data, saves = tmp_path / "data", tmp_path / "saves"
    data.mkdir()
    saves.mkdir()
    write_mod(data, {
        f"{SOUND}.sound": sound_file(),
        f"{BITMAP}.bitmap": bitmap_file(),
        f"{STRINGS}.unicode_string_list": strings_file(["MOD TEXT"] * 4),
    })
    lines = run_game(game, data, saves)
    assert "1 string lists from their files (1 new)" in lines, lines
    assert "1 bitmaps from their files (1 new)" in lines, lines
    assert "1 sounds from their files (1 new)" in lines, lines
    assert "left out:" not in lines, lines


def test_damaged_files_are_left_out(game, tmp_path):
    data, saves = tmp_path / "data", tmp_path / "saves"
    data.mkdir()
    saves.mkdir()
    sound = bytearray(sound_file())
    struct.pack_into(">I", sound, 0x40 + 0x98, 0x7FFFFFFF)  # more pitch ranges than the file holds
    strings = bytearray(strings_file(["MOD TEXT"]))
    struct.pack_into(">I", strings, 0x4C, 0x10000)  # a string past the file's end
    write_mod(data, {
        f"{SOUND}.sound": bytes(sound),
        f"{BITMAP}.bitmap": bitmap_file()[:-8],  # cut short
        f"{STRINGS}.unicode_string_list": bytes(strings),
    })
    lines = run_game(game, data, saves)
    assert "0 string lists from their files (0 new), 0 given back, 1 left out" in lines, lines
    assert "0 bitmaps from their files (0 new), 0 given back, 1 left out" in lines, lines
    assert "0 sounds from their files (0 new), 0 given back, 1 left out" in lines, lines


def folders(tmp_path):
    data, saves = tmp_path / "data", tmp_path / "saves"
    data.mkdir()
    saves.mkdir()
    return data, saves


def test_the_load_order(game, tmp_path):
    # alphabetical, but a_first asks to load after z_last, and b_middle asks to load before a_first: the sound of the
    # mod loaded last (a_first's) plays, and b_middle's, damaged, is never read
    data, saves = folders(tmp_path)
    damaged = bytearray(sound_file())
    struct.pack_into(">I", damaged, 0x40 + 0x98, 0x7FFFFFFF)
    write_mod(data, {f"{SOUND}.sound": sound_file()}, "a_first", 'name = "First"\nload_after = ["last"]\n')
    write_mod(data, {f"{SOUND}.sound": bytes(damaged)}, "b_middle", 'load_before = ["A_First"]\n')
    write_mod(data, {}, "z_last", 'id = "last"\n')
    lines = run_game(game, data, saves)
    assert "mods: load order b_middle, z_last, a_first" in lines, lines
    assert "1 sounds from their files (1 new), 0 given back, 0 left out" in lines, lines


def test_load_order_toml(game, tmp_path):
    # the player's order is kept, a mod it does not name goes where the default order puts it, and a mod turned off
    # plays nothing
    data, saves = folders(tmp_path)
    for folder in ("a", "b", "c", "d"):
        write_mod(data, {f"{SOUND}.sound": sound_file()} if folder == "c" else {}, folder, "")
    (data / "mods" / "load_order.toml").write_text('order = ["c", "a", "d"]\ndisabled = ["c"]\n', encoding="utf-8")
    lines = run_game(game, data, saves)
    assert "mods: load order c (off), a, b, d" in lines, lines
    assert "mods: c (" not in lines, lines


def test_incompatible_mods(game, tmp_path):
    # a run nobody watches is not asked: it plays them all, and says which conflict
    data, saves = folders(tmp_path)
    write_mod(data, {}, "german", 'name = "German text"\nincompatible = ["hd_audio"]\n')
    write_mod(data, {}, "hd_audio", 'name = "HD audio"\n')
    lines = run_game(game, data, saves)
    assert "mods: German text says it cannot be played with HD audio." in lines, lines
    assert "mods: played as they are, nobody being asked" in lines, lines
