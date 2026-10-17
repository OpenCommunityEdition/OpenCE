"""Mods (docs/mods.md) in the game: a mod of tag files made here (a sound, a bitmap and a unicode string list, as the
Halo Editing Kit writes them) played over Blood Gulch's tags, the same files damaged, which the game must leave out,
a model made from the map's own (its nodes, regions and markers, with geometry of its own), and the same with a
marker moved or a node renamed, which it must leave out, and the load order: alphabetical, moved as mods ask, as
load_order.toml keeps it, and mods that say they cannot be played together. Needs the Linux build
(build/linux/halo), assets/maps/bloodgulch.map and xvfb-run."""

import os
import shutil
import signal
import struct
import subprocess
import zlib
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
FP_MODEL = "weapons/assault rifle/fp/fp"
MODEL = "weapons/assault rifle/assault rifle"
VEHICLE_MODEL = "vehicles/warthog/warthog"
# where an Xbox map's tags are in memory
TAG_BASE = 0x803A6000


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


class MapTags:
    """an Xbox map's tags (assets/maps: compressed after their header), read as the game has them in memory"""

    def __init__(self, path: Path):
        data = path.read_bytes()
        data = data[:0x800] + zlib.decompress(data[0x800:])
        offset, size = struct.unpack_from("<ii", data, 0x10)
        self.tags = data[offset:offset + size]
        instances, _, _, count = struct.unpack_from("<IiIi", self.tags, 0)
        self.instances = {}
        for index in range(count):
            group, _, _, tag_index, name, definition, _, _ = struct.unpack_from("<4sIIiIIII", self.tags,
                                                                                 instances - TAG_BASE + index * 0x20)
            self.instances[tag_index] = (group[::-1], self.string(name), definition)

    def string(self, address: int) -> str:
        start = address - TAG_BASE
        return self.tags[start:self.tags.index(b"\0", start)].decode()

    def block(self, address: int, offset: int, size: int) -> list:
        """a block's elements (the bytes of each) of the structure at `address`"""
        count, elements = struct.unpack_from("<iI", self.tags, address - TAG_BASE + offset)
        return [(elements + size * index, self.tags[elements - TAG_BASE + size * index:][:size]) for index in range(count)]

    def find(self, group: bytes, name: str) -> int:
        for group_found, name_found, definition in self.instances.values():
            if group_found == group and name_found == name.replace("/", "\\"):
                return definition
        raise LookupError(name)


def model_file(tags: MapTags, name: str, move_marker: bool = False, rename_node: bool = False,
               gbxmodel: bool = False) -> bytes:
    """A model tag file (the Xbox's .model, as invader-extract writes one, or Halo PC's .gbxmodel) of the map's model
    `name`: its nodes, regions, permutations and markers (on its permutations, as the tool keeps them), its detail
    levels' cutoffs (from super high to super low, as a tag file has them), one shader of the map's, and geometry of
    its own, one triangle on its first node, its links to other parts the tool's none (0 and 0); its first marker
    moved, or its first node renamed."""
    model = tags.find(b"mode", name)
    root = tags.tags[model - TAG_BASE:][:0xE8]
    nodes = tags.block(model, 0xB8, 0x9C)
    regions = tags.block(model, 0xC4, 0x4C)
    shader = tags.block(model, 0xDC, 0x20)[0][1]
    # the map's markers, by their region and permutation: name, node, rotation, translation
    markers = {}
    for address, marker in tags.block(model, 0xAC, 0x40):
        for _, instance in tags.block(address, 0x34, 0x20):
            region, permutation, node = instance[0], instance[1], instance[2]
            translation, rotation = struct.unpack_from("<3f", instance, 4), struct.unpack_from("<4f", instance, 16)
            markers.setdefault((region, permutation), []).append((marker[:32], node, rotation, translation))
    if move_marker:
        first = min(markers)
        name_bytes, node, rotation, translation = markers[first][0]
        markers[first][0] = (name_bytes, node, rotation, (translation[0] + 0.5,) + translation[1:])
    out = bytearray(header(b"mod2", 5) if gbxmodel else header(b"mode", 4))
    flags, checksum = struct.unpack_from("<II", root, 0)
    out += struct.pack(">II5f", flags, checksum, *reversed(struct.unpack_from("<5f", root, 8))) + bytes(0x30 - 0x1C)
    out += struct.pack(">2f", *struct.unpack_from("<2f", root, 0x30)) + bytes(0xAC - 0x38)
    for count in (0, len(nodes), len(regions), 1, 1):
        out += struct.pack(">I", count) + bytes(8)
    for index, (_, node) in enumerate(nodes):
        node_name = (b"renamed" if rename_node and index == 0 else node[:32].split(b"\0")[0]).ljust(32, b"\0")
        out += node_name + struct.pack(">3h2x3f4ff", *struct.unpack_from("<3h", node, 32),
                                       *struct.unpack_from("<3f4ff", node, 40)) + bytes(0x9C - 0x48)
    permutations = []
    for region_index, (address, region) in enumerate(regions):
        perms = tags.block(address, 0x40, 0x58)
        permutations.append(perms)
        out += region[:0x40] + struct.pack(">I", len(perms)) + bytes(8)
    for region_index, perms in enumerate(permutations):
        for permutation_index, (_, permutation) in enumerate(perms):
            geometries = [0 if index != -1 else -1 for index in struct.unpack_from("<5h", permutation, 0x40)]
            out += (permutation[:0x20] + struct.pack(">I", struct.unpack_from("<I", permutation, 0x20)[0]) +
                    bytes(0x40 - 0x24) + struct.pack(">5h2x", *geometries) +
                    struct.pack(">I", len(markers.get((region_index, permutation_index), []))) + bytes(8))
        for permutation_index in range(len(perms)):
            for name_bytes, node, rotation, translation in markers.get((region_index, permutation_index), []):
                out += name_bytes + struct.pack(">h2x4f3f", node, *rotation, *translation) + bytes(16)
    # one geometry of one part: a triangle on the first node, drawn with the map's first shader
    out += bytes(0x24) + struct.pack(">I", 1) + bytes(8)
    part = bytearray(0x84 if gbxmodel else 0x68)
    struct.pack_into(">IhBB", part, 0, 0, 0, 0, 0)
    struct.pack_into(">I", part, 0x20, 3)
    struct.pack_into(">I", part, 0x38, 1)
    out += part
    for position in ((0, 0, 0), (0.1, 0, 0), (0, 0.1, 0)):
        out += struct.pack(">3f3f3f3f2f2h2f", *position, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, -1, 1, 0)
    out += struct.pack(">3H", 0, 1, 2)
    shader_name = tags.string(struct.unpack_from("<I", shader, 4)[0]).encode()
    out += shader[:4][::-1] + struct.pack(">IIi", 0, len(shader_name), -1) + struct.pack(">h", 0) + bytes(14)
    out += shader_name + b"\0"
    return bytes(out)


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


def test_models(game, tmp_path):
    # the map's model with geometry of its own plays (Halo PC's .gbxmodel); with a marker moved, or a node renamed
    # (the Xbox's .model), it changes what the machines agree on, and is left out
    data, saves = folders(tmp_path)
    tags = MapTags(MAPS / "bloodgulch.map")
    write_mod(data, {f"{FP_MODEL}.gbxmodel": model_file(tags, FP_MODEL, gbxmodel=True),
                     f"{MODEL}.model": model_file(tags, MODEL, move_marker=True),
                     f"{VEHICLE_MODEL}.model": model_file(tags, VEHICLE_MODEL, rename_node=True)})
    lines = run_game(game, data, saves)
    assert "its markers are not the map's" in lines, lines
    assert "its nodes are not the map's" in lines, lines
    assert "1 models from their files (1 new), 0 given back, 2 left out" in lines, lines
