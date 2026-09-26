"""Tests for loading Custom Edition and OpenSauce caches (port/linux/game/cache_file_formats.c).

The tests build complete but tiny Custom Edition caches and resource maps in
memory, so no game data is needed or stored, and run the report tool
port/tools/cache_file_report.c on them. The tool is compiled with clang,
warnings as errors and undefined behaviour trapping, so a crash or an
undefined operation on malformed input fails the test that caused it. The
module is also compiled as the game compiles it (gnu89).

When real maps are present (HALO_CUSTOM_EDITION_MAPS, or the gitignored
assets/custom_edition), test_real_maps_* also check the results recorded in
docs/custom_edition_caches.md.
"""
import os
from pathlib import Path
import random
import shutil
import struct
import subprocess
import sys
import zlib

import pytest

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / "port/linux/game/cache_file_formats.c"
TOOL = ROOT / "port/tools/cache_file_report.c"

BASE = 0x40440000
TAG_CACHE_BYTES = 0x01700000
TAG_CACHE_BYTES_UPGRADED = 0x02280000
HEADER_BYTES = 0x800
NONE = 0xFFFFFFFF

STRICT_FLAGS = ["-Wall", "-Wextra", "-Wpedantic", "-Werror"]
UB_TRAP_FLAGS = ["-fsanitize=undefined", "-fsanitize-trap=undefined"]


def code(text):
    """A four-character code as the files store it (MSVC multi-character value, little-endian)."""
    return struct.unpack(">I", text.encode("latin-1"))[0]


# ---------- the report tool


def find_clang():
    for candidate in (shutil.which("clang"), r"C:\Program Files\LLVM\bin\clang.exe"):
        if candidate and Path(candidate).is_file():
            return candidate
    return None


def target_flag_sets():
    if sys.platform == "win32":
        return [["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]]
    # the game is 32-bit; the module is written for any width
    return [["-m32"], []]


@pytest.fixture(scope="session")
def report_tool(tmp_path_factory):
    clang = find_clang()
    if clang is None:
        pytest.skip("clang is needed to build the report tool")
    folder = tmp_path_factory.mktemp("cache-file-report")
    output = folder / ("cache_file_report.exe" if sys.platform == "win32" else "cache_file_report")
    errors = []
    for target in target_flag_sets():
        command = [clang, *target, "-std=c99", *STRICT_FLAGS, *UB_TRAP_FLAGS, "-O1", "-g",
                   f"-I{MODULE.parent}", str(MODULE), str(TOOL), "-o", str(output)]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode == 0:
            return output
        errors.append(result.stdout + result.stderr)
    # a compile error is a failure, not a missing tool: report it
    pytest.fail("could not build the report tool:\n" + "\n".join(errors))


def test_module_compiles_as_game_code_without_warnings(tmp_path):
    """The game compiles port/linux/game with -std=gnu89 -w; check the
    warnings it hides."""
    clang = find_clang()
    if clang is None:
        pytest.skip("clang is needed")
    for target in target_flag_sets():
        command = [clang, *target, "-std=gnu89", *STRICT_FLAGS, "-Wno-long-long", "-c", str(MODULE),
                   "-o", str(tmp_path / "cache_file_formats.o")]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode == 0:
            return
    pytest.fail(result.stdout + result.stderr)


def run_report(tool, *arguments):
    result = subprocess.run([str(tool), *map(str, arguments)], capture_output=True, text=True)
    # 0: everything loaded, 1: something did not; anything else is a crash
    # or an undefined-behaviour trap
    assert result.returncode in (0, 1), (result.returncode, result.stdout, result.stderr)
    blocks = []
    for chunk in result.stdout.split("\n\n"):
        fields = {}
        for line in chunk.splitlines():
            key, _, value = line.partition(": ")
            fields[key] = value
        if fields.get("file"):
            blocks.append(fields)
    return result.returncode, blocks


def report_one(tool, path, *options):
    returncode, blocks = run_report(tool, *options, path)
    assert len(blocks) == 1
    return returncode, blocks[0]


# ---------- synthetic files


class Blob:
    """Bytes that will sit at `base`, addressed by that base."""

    def __init__(self, base=0):
        self.base = base
        self.bytes = bytearray()

    def reserve(self, size, align=4):
        while len(self.bytes) % align:
            self.bytes.append(0)
        address = self.base + len(self.bytes)
        self.bytes.extend(bytes(size))
        return address

    def add(self, data, align=4):
        address = self.reserve(len(data), align)
        self.put(address, data)
        return address

    def put(self, address, data):
        offset = address - self.base
        self.bytes[offset:offset + len(data)] = data

    def u16(self, address, value):
        struct.pack_into("<H", self.bytes, address - self.base, value & 0xFFFF)

    def u32(self, address, value):
        struct.pack_into("<I", self.bytes, address - self.base, value & 0xFFFFFFFF)

    def block(self, address, count, element_address):
        self.u32(address, count)
        self.u32(address + 4, element_address)
        self.u32(address + 8, 0)

    def data(self, address, size, flags=0, file_offset=0, data_address=0):
        for index, value in enumerate((size, flags, file_offset, data_address, 0)):
            self.u32(address + index * 4, value)


def resource_map_file(map_type, items):
    """A resource map: header, item data, names, index (OpenSauce s_data_file_header)."""
    data = bytearray(16)
    placed = []
    for name, payload in items:
        while len(data) % 4:
            data.append(0)
        placed.append((name, len(data), len(payload)))
        data.extend(payload)
    names_offset = len(data)
    name_offsets = []
    for name, _, _ in placed:
        name_offsets.append(len(data) - names_offset)
        data.extend(name.encode("latin-1") + b"\0")
    index_offset = len(data)
    for (name, offset, size), name_offset in zip(placed, name_offsets):
        data.extend(struct.pack("<iii", name_offset, size, offset))
    struct.pack_into("<iiii", data, 0, map_type, names_offset, index_offset, len(items))
    return data, {name: offset for name, offset, _ in placed}


def bitmap_item(pixels_offset, pixels_size):
    """A bitmap tag as bitmaps.map holds it: addresses count from the item."""
    item = Blob()
    group = item.reserve(0x6C)
    item.data(group + 0x1C, 0x40)  # a compressed color plate the cache build left out
    sequence = item.reserve(0x40)
    sprite = item.reserve(0x20)
    bitmap = item.reserve(0x30)
    item.block(group + 0x54, 1, sequence)
    item.block(sequence + 0x34, 1, sprite)
    item.block(group + 0x60, 1, bitmap)
    item.u32(bitmap, code("bitm"))
    item.u16(bitmap + 0x0E, 1 << 8)  # the pixels are in bitmaps.map
    item.u32(bitmap + 0x18, pixels_offset)
    item.u32(bitmap + 0x1C, pixels_size)
    item.u32(bitmap + 0x24, NONE)
    return bytes(item.bytes)


def sound_item(samples_offset, samples_size, pitch_ranges=1):
    """A sound as sounds.map holds it: the header again, then pitch ranges and
    permutations whose addresses count from the first pitch range."""
    header = bytearray(0xA4)
    struct.pack_into("<iII", header, 0x98, pitch_ranges, 0x5A1D818, 0xD39A3C)  # stale editing-kit pointers
    body = Blob()
    ranges = [body.reserve(0x48) for _ in range(pitch_ranges)]
    for pitch_range in ranges:
        permutation = body.reserve(0x7C)
        mouth = body.add(b"MOUTHDAT")
        body.block(pitch_range + 0x3C, 1, permutation)
        body.data(permutation + 0x40, samples_size, flags=1, file_offset=samples_offset)
        body.data(permutation + 0x54, 8, data_address=mouth)
    return bytes(header + body.bytes)


def string_list_item():
    item = Blob()
    group = item.reserve(0x0C)
    references = item.reserve(2 * 20)
    item.block(group, 2, references)
    for index, text in enumerate(("hello", "world!")):
        encoded = text.encode("utf-16-le") + b"\0\0"
        item.data(references + index * 20, len(encoded), data_address=item.add(encoded))
    return bytes(item.bytes)


def font_item(style_reference=NONE):
    item = Blob()
    font = item.reserve(0x9C)
    table = item.reserve(12)
    entries = item.reserve(3 * 2)
    characters = item.reserve(2 * 20)
    pixels = item.add(bytes(range(8)))
    item.block(font + 0x30, 1, table)
    item.block(table, 3, entries)
    item.block(font + 0x7C, 2, characters)
    item.data(font + 0x88, 8, data_address=pixels)
    for reference in range(4):
        item.u32(font + 0x3C + reference * 16, code("font"))
        item.u32(font + 0x3C + reference * 16 + 12, style_reference)
    return bytes(item.bytes)


def hud_message_text_item():
    item = Blob()
    group = item.reserve(0x80)
    text = item.add("text".encode("utf-16-le"))
    elements = item.reserve(2 * 2)
    messages = item.reserve(0x40)
    item.data(group, 8, data_address=text)
    item.block(group + 0x14, 2, elements)
    item.block(group + 0x20, 1, messages)
    return bytes(item.bytes)


class Map:
    """A complete, minimal Custom Edition cache and its resource maps.

    Every option changes one thing from the valid default, and `where`
    records the file offsets of the fields tests corrupt afterwards."""

    def __init__(self, opensauce=None, mod_name="", definitions=b"", trailing=b"",
                 extra_tags=(), bsp_gap=None, bitmap_pixels_size=16, sound_samples_size=32,
                 font_style_reference=NONE, pitch_ranges=1, bsp_sizes=(0x1000,)):
        self.bsp_sizes = bsp_sizes
        self.opensauce = opensauce
        self.mod_name = mod_name
        self.definitions = definitions
        self.trailing = trailing
        self.extra_tags = extra_tags
        self.bsp_gap = bsp_gap
        self.bitmap_pixels_size = bitmap_pixels_size
        self.sound_samples_size = sound_samples_size
        self.font_style_reference = font_style_reference
        self.pitch_ranges = pitch_ranges
        self.where = {}

    @property
    def upgraded(self):
        return bool(self.opensauce and self.opensauce.get("flags", 0) & 1)

    def resource_maps(self):
        # each map's first item sits right after its 16-byte header
        pixels_offset = 16
        bitmaps, _ = resource_map_file(1, [
            ("test\\external bitmap__pixels", bytes(range(16))),
            ("test\\external bitmap", bitmap_item(pixels_offset, self.bitmap_pixels_size)),
        ])
        samples_offset = 16
        sounds, _ = resource_map_file(2, [
            ("test\\sound__permutations", bytes(32)),
            ("test\\sound", sound_item(samples_offset, self.sound_samples_size, self.pitch_ranges)),
        ])
        loc, _ = resource_map_file(3, [
            ("test\\strings", string_list_item()),
            ("test\\font", font_item(self.font_style_reference)),
            ("test\\hud messages", hud_message_text_item()),
        ])
        return {"bitmaps": bitmaps, "sounds": sounds, "loc": loc}

    def build(self):
        tags = [
            ("scnr", "test\\scenario", False),
            ("sbsp", "test\\bsp", False),
            ("bitm", "test\\in map bitmap", False),
            ("bitm", "test\\external bitmap", True),
            ("snd!", "test\\sound", True),
            ("ustr", "test\\strings", True),
            ("font", "test\\font", True),
            ("hmt ", "test\\hud messages", True),
            ("weap", "test\\weapon", False),
        ] + list(self.extra_tags)
        # further structure BSPs go last, so the indices above never move
        bsp_tag_indices = [1] + [len(tags) + extra for extra in range(len(self.bsp_sizes) - 1)]
        tags += [("sbsp", f"test\\bsp {extra + 2}", False) for extra in range(len(self.bsp_sizes) - 1)]
        resource_indices = {"test\\external bitmap": 1, "test\\strings": 0, "test\\font": 1,
                            "test\\hud messages": 2}
        salt = 0xE174

        # the file before the tag data: header, structure BSPs one after
        # another, model data, pixels
        model_data = bytes(range(256)) * 4
        in_map_pixels = bytes(range(32))
        bsp_offset = HEADER_BYTES
        model_offset = bsp_offset + sum(self.bsp_sizes)
        pixels_offset = model_offset + len(model_data)
        tag_data_offset = pixels_offset + len(in_map_pixels)
        tag_cache_bytes = TAG_CACHE_BYTES_UPGRADED if self.upgraded else TAG_CACHE_BYTES
        if self.bsp_gap is None:
            bsp_addresses = [BASE + tag_cache_bytes - size for size in self.bsp_sizes]
        else:
            bsp_addresses = [BASE + self.bsp_gap for _ in self.bsp_sizes]

        tag_data = Blob(BASE)
        index = tag_data.reserve(0x28)
        instances = tag_data.reserve(len(tags) * 0x20)
        tag_data.u32(index + 0x00, instances)
        tag_data.u32(index + 0x04, salt << 16)
        tag_data.u32(index + 0x0C, len(tags))
        tag_data.u32(index + 0x10, 3)
        tag_data.u32(index + 0x14, model_offset)
        tag_data.u32(index + 0x18, 3)
        tag_data.u32(index + 0x1C, 0x200)
        tag_data.u32(index + 0x20, len(model_data))
        tag_data.u32(index + 0x24, code("tags"))
        for tag_index, (group, name, external) in enumerate(tags):
            instance = instances + tag_index * 0x20
            tag_data.u32(instance, code(group))
            tag_data.u32(instance + 0x04, NONE)
            tag_data.u32(instance + 0x08, NONE)
            tag_data.u32(instance + 0x0C, ((salt + tag_index) << 16) | tag_index)
            tag_data.u32(instance + 0x10, tag_data.add(name.encode("latin-1") + b"\0", align=1))
            tag_data.u32(instance + 0x18, 1 if external else 0)
            if external and group != "snd!":
                tag_data.u32(instance + 0x14, resource_indices[name])
        address_of = {}
        # the scenario and its structure BSP references
        scenario = tag_data.reserve(0x5B0)
        reference = tag_data.reserve(0x20 * len(self.bsp_sizes))
        tag_data.block(scenario + 0x5A4, len(self.bsp_sizes), reference)
        file_offset = bsp_offset
        for bsp_index, size in enumerate(self.bsp_sizes):
            element = reference + bsp_index * 0x20
            tag_index = bsp_tag_indices[bsp_index]
            tag_data.u32(element + 0x00, file_offset)
            tag_data.u32(element + 0x04, size)
            tag_data.u32(element + 0x08, bsp_addresses[bsp_index])
            tag_data.u32(element + 0x10, code("sbsp"))
            tag_data.u32(element + 0x1C, ((salt + tag_index) << 16) | tag_index)
            file_offset += size
        address_of["test\\scenario"] = scenario
        # a bitmap kept in the map, its pixels in the map too
        group = tag_data.reserve(0x6C)
        bitmap = tag_data.reserve(0x30)
        tag_data.block(group + 0x60, 1, bitmap)
        tag_data.u32(bitmap + 0x18, pixels_offset)
        tag_data.u32(bitmap + 0x1C, len(in_map_pixels))
        address_of["test\\in map bitmap"] = group
        # the header the map keeps of a sound held by sounds.map
        sound = tag_data.reserve(0xA4)
        tag_data.u32(sound + 0x98, self.pitch_ranges)
        address_of["test\\sound"] = sound
        address_of["test\\weapon"] = tag_data.add(bytes(0x100))
        for group_name, name, external in self.extra_tags:
            address_of[name] = tag_data.add(bytes(0x40))
        for tag_index, (group_name, name, external) in enumerate(tags):
            if name in address_of:
                tag_data.u32(instances + tag_index * 0x20 + 0x14, address_of[name])

        # the file
        data = bytearray(HEADER_BYTES)
        for size, address in zip(self.bsp_sizes, bsp_addresses):
            bsp = bytearray(size)
            struct.pack_into("<IiIiII", bsp, 0, address + 0x18, 0, 0, 0, 0, code("sbsp"))
            data += bsp
        data += model_data + in_map_pixels + tag_data.bytes
        file_length = len(data)
        definitions_offset = file_length
        data += self.definitions + self.trailing
        header = data
        struct.pack_into("<IiIIII", header, 0, code("head"), 609, file_length, 0,
                         tag_data_offset, len(tag_data.bytes))
        header[0x20:0x20 + 5] = b"test\0"
        header[0x40:0x40 + 14] = b"01.00.00.0609\0"
        struct.pack_into("<h", header, 0x60, 1)
        struct.pack_into("<I", header, 0x7FC, code("foot"))
        if self.opensauce is not None:
            self.write_opensauce(header, definitions_offset)
        checksum = zlib.crc32(bytes(data[bsp_offset:bsp_offset + sum(self.bsp_sizes)]))
        checksum = zlib.crc32(model_data, checksum)
        checksum = zlib.crc32(bytes(tag_data.bytes), checksum)
        struct.pack_into("<I", header, 0x64, ~checksum & 0xFFFFFFFF)

        self.where = {
            "tag_data": tag_data_offset,
            "index": tag_data_offset,
            "instances": tag_data_offset + (instances - BASE),
            "scenario": tag_data_offset + (scenario - BASE),
            "reference": tag_data_offset + (reference - BASE),
            "bsp": bsp_offset,
            "in_map_bitmap": tag_data_offset + (bitmap - BASE),
            "sound_header": tag_data_offset + (sound - BASE),
            "tag_count": len(tags),
        }
        return bytes(data)

    def write_opensauce(self, header, definitions_offset):
        options = self.opensauce
        at = 0x70
        struct.pack_into("<IhH", header, at, code("yelo"), options.get("version", 1), options.get("flags", 0))
        header[at + 8] = options.get("project_yellow", 2)
        header[at + 9] = options.get("project_yellow_globals", 2)
        struct.pack_into("<f", header, at + 0x0C, options.get("amount", 1.5))
        if self.definitions:
            struct.pack_into("<III", header, at + 0x10, len(self.definitions), 0x100,
                             options.get("definitions_offset", definitions_offset))
        mod_name = self.mod_name.encode("latin-1")
        header[at + 0x40:at + 0x40 + len(mod_name) + 1] = mod_name + b"\0"
        build = b"000001.26.09.26.1200.ship\0"
        header[at + 0x70:at + 0x70 + len(build)] = build

    def write(self, folder, name="test.map", resource_maps=True):
        folder.mkdir(parents=True, exist_ok=True)
        path = folder / name
        path.write_bytes(self.build())
        if resource_maps:
            for type_name, data in self.resource_maps().items():
                if self.mod_name and self.opensauce and self.opensauce.get("flags", 0) & 2:
                    (folder / "data_files").mkdir(exist_ok=True)
                    (folder / "data_files" / f"{self.mod_name}-{type_name}.map").write_bytes(data)
                else:
                    (folder / f"{type_name}.map").write_bytes(data)
        return path


def patched(path, offset, fmt, value):
    data = bytearray(path.read_bytes())
    struct.pack_into(fmt, data, offset, value)
    path.write_bytes(data)
    return path


# ---------- supported formats


def test_minimal_custom_edition_cache_loads_with_every_resource(report_tool, tmp_path):
    path = Map().write(tmp_path)
    returncode, report = report_one(report_tool, path)
    assert returncode == 0
    assert report["format"] == "Custom Edition cache"
    assert report["version"] == "609" and report["build"] == "01.00.00.0609" and report["name"] == "test"
    assert report["opensauce_header"] == "no"
    assert report["load"] == "ok"
    assert report["tag_cache_bytes"] == hex(TAG_CACHE_BYTES)
    assert report["tags"] == "9" and report["scenario_tag"] == "0"
    assert report["resource_tags"] == "bitmaps 1, sounds 1, loc 3"
    # the in-map bitmap and the resource bitmap; one permutation's samples
    assert report["bitmap_data_ranges_checked"] == "2"
    assert report["sound_sample_ranges_checked"] == "1"
    assert report["computed_checksum"] == report["checksum"]
    assert report["warnings"] == "none"
    assert report["structure_bsps"] == "1"
    assert report["lowest_structure_bsp_address"] == hex(BASE + TAG_CACHE_BYTES - 0x1000)
    assert report["milestone.recognize"] == "yes"
    assert report["milestone.load"] == "yes"
    assert report["milestone.run"].startswith("no")


def test_relocation_counts_every_resource_pointer(report_tool, tmp_path):
    """bitmap: sequences, sprites, bitmaps (3); sound: pitch ranges,
    permutations, mouth data (3); strings: block + 2 strings (3); font:
    tables, table entries, characters, pixels (4); HUD text: text, 2 blocks (3)"""
    _, report = report_one(report_tool, Map().write(tmp_path))
    assert report["relocated_pointers"] == "16"


def test_several_pitch_ranges_are_relocated(report_tool, tmp_path):
    _, report = report_one(report_tool, Map(pitch_ranges=3).write(tmp_path))
    assert report["load"] == "ok"
    assert report["sound_sample_ranges_checked"] == "3"


def test_several_structure_bsps_share_the_top_of_the_tag_cache(report_tool, tmp_path):
    """Each BSP is loaded alone at the top of the tag cache; the largest sets
    how much room is left for the tags, and the checksum covers them all,
    packed one after another (none of the sample maps has more than one)."""
    returncode, report = report_one(report_tool, Map(bsp_sizes=(0x1000, 0x3000, 0x2000)).write(tmp_path))
    assert returncode == 0
    assert report["structure_bsps"] == "3"
    assert report["largest_structure_bsp_bytes"] == "0x3000"
    assert report["lowest_structure_bsp_address"] == hex(BASE + TAG_CACHE_BYTES - 0x3000)
    assert report["computed_checksum"] == report["checksum"]
    assert report["warnings"] == "none"


def test_resource_maps_are_recognized_by_type(report_tool, tmp_path):
    Map().write(tmp_path)
    returncode, blocks = run_report(report_tool, tmp_path / "bitmaps.map", tmp_path / "sounds.map",
                                    tmp_path / "loc.map")
    assert returncode == 0
    assert [(b["format"], b["resource_map_type"], b["resource_map"]) for b in blocks] == [
        ("Custom Edition resource map", "bitmaps", "ok"),
        ("Custom Edition resource map", "sounds", "ok"),
        ("Custom Edition resource map", "loc", "ok"),
    ]
    assert [b["resource_items"] for b in blocks] == ["2", "2", "3"]


def test_opensauce_cache_with_memory_upgrades_uses_the_larger_tag_cache(report_tool, tmp_path):
    path = Map(opensauce={"flags": 1}).write(tmp_path, "test.yelo")
    returncode, report = report_one(report_tool, path)
    assert returncode == 0
    assert report["opensauce_header"] == "yes"
    assert report["opensauce_flags"] == "memory upgrades"
    assert report["tag_cache_bytes"] == hex(TAG_CACHE_BYTES_UPGRADED)
    assert report["lowest_structure_bsp_address"] == hex(BASE + TAG_CACHE_BYTES_UPGRADED - 0x1000)
    assert report["load"] == "ok"


def test_opensauce_structure_bsp_beyond_the_stock_tag_cache_needs_the_upgrade_flag(report_tool, tmp_path):
    """A BSP placed at the top of the upgraded tag cache is out of range for a
    cache that does not declare memory upgrades."""
    upgraded = Map(opensauce={"flags": 1})
    path = upgraded.write(tmp_path, "test.yelo")
    data = bytearray(path.read_bytes())
    struct.pack_into("<H", data, 0x70 + 6, 0)
    path.write_bytes(data)
    _, report = report_one(report_tool, path)
    assert report["load"] == "a structure BSP does not fit in the file or in the tag cache"


def test_opensauce_definitions_are_accepted_after_the_cache_data(report_tool, tmp_path):
    definitions = zlib.compress(b"CheApe definitions" * 8)
    path = Map(opensauce={"flags": 1}, definitions=definitions).write(tmp_path, "test.yelo")
    returncode, report = report_one(report_tool, path)
    assert returncode == 0
    assert report["opensauce_definitions"].startswith(hex(path.stat().st_size - len(definitions)))
    assert report["trailing_bytes"] == "0x0"
    assert report["warnings"] == "none"


def test_trailing_data_is_reported_not_rejected(report_tool, tmp_path):
    path = Map(trailing=b"\xAA" * 100).write(tmp_path)
    returncode, report = report_one(report_tool, path)
    assert returncode == 0
    assert report["trailing_bytes"] == "0x64"
    assert report["warnings"] == "trailing data"


def test_checksum_mismatch_is_reported_not_rejected(report_tool, tmp_path):
    path = patched(Map().write(tmp_path), 0x64, "<I", 0x12345678)
    returncode, report = report_one(report_tool, path)
    assert returncode == 0
    assert report["load"] == "ok"
    assert report["warnings"] == "checksum mismatch"


def test_opensauce_tags_are_reported(report_tool, tmp_path):
    path = Map(opensauce={"flags": 1}, extra_tags=[("yelo", "test\\project yellow", False),
                                                   ("gelo", "test\\project yellow globals", False)]).write(tmp_path)
    returncode, report = report_one(report_tool, path)
    assert returncode == 0
    assert report["warnings"] == "OpenSauce tags"


def test_mod_data_files_are_required_when_the_cache_names_them(report_tool, tmp_path):
    cache = Map(opensauce={"flags": 3}, mod_name="testmod")
    path = cache.write(tmp_path / "with_mod", "test.yelo")
    returncode, report = report_one(report_tool, path)
    assert returncode == 0
    assert report["opensauce_flags"] == "memory upgrades, mod data files"
    assert report["opensauce_mod_name"] == "testmod"
    assert report["resource_map.bitmaps"].replace("\\", "/").endswith("data_files/testmod-bitmaps.map (ok)")

    # the stock files alone are not enough
    stock_only = tmp_path / "stock_only"
    Map().write(stock_only)
    missing = stock_only / "test.yelo"
    missing.write_bytes(path.read_bytes())
    returncode, report = report_one(report_tool, missing)
    assert returncode == 1
    assert report["resource_map.bitmaps"].endswith("(not found)")
    assert report["load"] == "a map needs a resource map that was not supplied"
    assert report["milestone.load"] == "no"

    # unless the substitution is asked for, and then it is reported
    returncode, report = report_one(report_tool, missing, "--stock-data-files")
    assert returncode == 0
    assert report["resource_maps_substituted"] == "stock files instead of mod set 'testmod'"
    assert report["load"] == "ok"


def test_resource_maps_are_found_in_the_maps_directory_option(report_tool, tmp_path):
    Map().write(tmp_path / "maps")
    moved = tmp_path / "elsewhere" / "test.map"
    moved.parent.mkdir()
    (tmp_path / "maps" / "test.map").rename(moved)
    returncode, report = report_one(report_tool, moved, "--maps", tmp_path / "maps")
    assert returncode == 0 and report["load"] == "ok"


# ---------- the original (Xbox) format


def xbox_header(build=b"01.01.14.2342", version=5, file_length=0x4000, name=b"b30"):
    header = bytearray(0x4000)
    struct.pack_into("<IiI", header, 0, code("head"), version, file_length)
    struct.pack_into("<II", header, 0x10, HEADER_BYTES, 0x100)
    header[0x20:0x20 + len(name)] = name
    header[0x40:0x40 + len(build)] = build
    struct.pack_into("<I", header, 0x7FC, code("foot"))
    return bytes(header)


@pytest.mark.parametrize("build", [b"01.01.14.2342", b"01.10.12.2276"])
def test_xbox_caches_are_recognized_and_left_to_the_original_loader(report_tool, tmp_path, build):
    """The module names the format and loads nothing: cache_file_header_verify
    (source/cache/cache_files.c) alone judges Xbox caches, unchanged."""
    path = tmp_path / "b30.map"
    path.write_bytes(xbox_header(build))
    returncode, report = report_one(report_tool, path)
    assert returncode == 0
    assert report["format"] == "Xbox cache"
    assert report["identify"] == "ok"
    assert report["build"] == build.decode()
    assert "load" not in report and "milestone.load" not in report


def test_compressed_xbox_caches_are_recognized(report_tool, tmp_path):
    """Retail Xbox maps declare their decompressed length, beyond the file."""
    path = tmp_path / "a10.map"
    path.write_bytes(xbox_header(file_length=0x10000000))
    _, report = report_one(report_tool, path)
    assert report["format"] == "Xbox cache" and report["identify"] == "ok"


def test_other_cache_versions_are_named_not_loaded(report_tool, tmp_path):
    path = tmp_path / "pc.map"
    path.write_bytes(xbox_header(build=b"01.00.00.0564", version=7))
    returncode, report = report_one(report_tool, path)
    assert returncode == 1
    assert report["format"] == "cache of an unknown version"
    assert report["identify"] == "the cache version is not one this build knows"


def test_unrelated_files_are_not_recognized(report_tool, tmp_path):
    path = tmp_path / "notes.map"
    path.write_bytes(b"not a map at all, just some text" * 4)
    returncode, report = report_one(report_tool, path)
    assert returncode == 1
    assert report["format"] == "not a Halo map file"


# ---------- malformed input: every check, one at a time


def header_case(offset, fmt, value):
    return lambda cache, path: patched(path, offset, fmt, value)


def tag_data_case(where, delta, fmt, value):
    return lambda cache, path: patched(path, cache.where[where] + delta, fmt, value)


def instance_case(tag_index, delta, fmt, value):
    return lambda cache, path: patched(path, cache.where["instances"] + tag_index * 0x20 + delta, fmt, value)


MALFORMED_CACHES = {
    "bad footer": (header_case(0x7FC, "<I", 0), "identify", "the header or footer signature is wrong"),
    "unterminated name": (header_case(0x20, "32s", b"N" * 32), "identify",
                          "a name or build string in the header is not terminated"),
    "unterminated build": (header_case(0x40, "32s", b"9" * 32), "identify",
                           "a name or build string in the header is not terminated"),
    "file length beyond file": (header_case(0x08, "<I", 0x7FFFFFFF), "identify",
                                "the file length in the header does not fit the file or the size limit"),
    "file length inside header": (header_case(0x08, "<I", 0x10), "identify",
                                  "the file length in the header does not fit the file or the size limit"),
    "compressed": (header_case(0x0C, "<I", 0x1000), "identify",
                   "the cache is compressed, which Custom Edition caches never are"),
    "tag data in header": (header_case(0x10, "<I", 0x100), "identify",
                           "the tag data range in the header does not fit the file or the tag cache"),
    "tag data beyond file": (header_case(0x14, "<I", 0x7FFFFFF0), "identify",
                             "the tag data range in the header does not fit the file or the tag cache"),
    "tag data too small": (header_case(0x14, "<I", 0x10), "identify",
                           "the tag data range in the header does not fit the file or the tag cache"),
    "tag index signature": (tag_data_case("index", 0x24, "<I", code("sgat")), "load",
                            "the tag index signature is not 'tags'"),
    "no tags": (tag_data_case("index", 0x0C, "<i", 0), "load", "the tag instances do not fit in the tag data"),
    "negative tag count": (tag_data_case("index", 0x0C, "<i", -5), "load",
                           "the tag instances do not fit in the tag data"),
    "too many tags": (tag_data_case("index", 0x0C, "<i", 0x7FFFFFF), "load",
                      "the tag instances do not fit in the tag data"),
    "instances outside": (tag_data_case("index", 0x00, "<I", 0x10000000), "load",
                          "the tag instances do not fit in the tag data"),
    "instances over index": (tag_data_case("index", 0x00, "<I", BASE + 4), "load",
                             "the tag instances do not fit in the tag data"),
    "instances unaligned": (tag_data_case("index", 0x00, "<I", BASE + 0x29), "load",
                            "the tag instances do not fit in the tag data"),
    "model data beyond file": (tag_data_case("index", 0x20, "<I", 0x7FFFFFFF), "load",
                               "the model vertex and index data do not fit in the file"),
    "index data beyond model data": (tag_data_case("index", 0x1C, "<I", 0x10000), "load",
                                     "the model vertex and index data do not fit in the file"),
    "model data in header": (tag_data_case("index", 0x14, "<I", 0x10), "load",
                             "the model vertex and index data do not fit in the file"),
    "handle": (instance_case(3, 0x0C, "<I", 0xE1770009), "load",
               "a tag handle does not match its position in the index"),
    "name outside": (instance_case(2, 0x10, "<I", 0x3FFFFFFF), "load",
                     "a tag name lies outside the tag data or is not terminated"),
    "tag address outside": (instance_case(8, 0x14, "<I", BASE + 0x1000000), "load",
                            "a tag's address lies outside the tag data"),
    "null tag address": (instance_case(8, 0x14, "<I", 0), "load", "a tag's address lies outside the tag data"),
    "external weapon": (instance_case(8, 0x18, "<I", 1), "load",
                        "a tag of a group that resource maps never hold is marked as held by one"),
    "external sound header outside": (instance_case(4, 0x14, "<I", BASE - 0x100), "load",
                                      "a tag's address lies outside the tag data"),
    "scenario index": (tag_data_case("index", 0x04, "<I", 0xE17400FF), "load",
                       "the scenario tag is missing, misplaced or not a scenario"),
    "scenario salt": (tag_data_case("index", 0x04, "<I", 0x12340000), "load",
                      "the scenario tag is missing, misplaced or not a scenario"),
    "scenario group": (instance_case(0, 0x00, "<I", code("weap")), "load",
                       "the scenario tag is missing, misplaced or not a scenario"),
    "external scenario": (instance_case(0, 0x18, "<I", 1), "load",
                          "a tag of a group that resource maps never hold is marked as held by one"),
    "bsp count": (tag_data_case("scenario", 0x5A4, "<i", 33), "load",
                  "the scenario's structure BSP block is not valid"),
    "bsp block outside": (tag_data_case("scenario", 0x5A8, "<I", 0x10), "load",
                          "the scenario's structure BSP block is not valid"),
    "bsp reference to a weapon": (tag_data_case("reference", 0x1C, "<I", (0xE174 + 8) << 16 | 8), "load",
                                  "the scenario's structure BSP block is not valid"),
    "bsp tag already loaded": (instance_case(1, 0x14, "<I", BASE + 0x28), "load",
                               "the scenario's structure BSP block is not valid"),
    "bsp beyond file": (tag_data_case("reference", 0x04, "<i", 0x7FFFFF00), "load",
                        "a structure BSP does not fit in the file or in the tag cache"),
    "bsp in header": (tag_data_case("reference", 0x00, "<i", 0x10), "load",
                      "a structure BSP does not fit in the file or in the tag cache"),
    "bsp over tag data": (tag_data_case("reference", 0x08, "<I", BASE + 0x100), "load",
                          "a structure BSP does not fit in the file or in the tag cache"),
    "bsp beyond tag cache": (tag_data_case("reference", 0x08, "<I", BASE + TAG_CACHE_BYTES - 0x800), "load",
                             "a structure BSP does not fit in the file or in the tag cache"),
    "bsp signature": (lambda cache, path: patched(path, cache.where["bsp"] + 0x14, "<I", code("psbs")), "load",
                      "a structure BSP header is not valid"),
    "bsp xbox vertex buffers": (lambda cache, path: patched(path, cache.where["bsp"] + 0x04, "<i", 12), "load",
                                "a structure BSP header is not valid"),
    "bsp pointer": (lambda cache, path: patched(path, cache.where["bsp"], "<I", BASE), "load",
                    "a structure BSP header is not valid"),
    "bitmap resource index": (instance_case(3, 0x14, "<I", 7), "load",
                              "a tag's entry is missing from its resource map"),
    "bitmap resource name": (instance_case(3, 0x14, "<I", 0), "load",
                             "a tag's entry is missing from its resource map"),
    "string resource index": (instance_case(5, 0x14, "<I", 0xFFFFFFFF), "load",
                              "a tag's entry is missing from its resource map"),
    "in-map pixels beyond file": (tag_data_case("in_map_bitmap", 0x1C, "<I", 0x7FFFFFFF), "load",
                                  "bitmap pixels or sound samples lie outside their file"),
    "in-map bitmaps block outside": (tag_data_case("in_map_bitmap", -0x6C + 0x64, "<I", 0x10), "load",
                                     "a tag block or tag data field lies outside the loaded tags"),
}


@pytest.mark.parametrize("case", sorted(MALFORMED_CACHES))
def test_malformed_caches_are_rejected_with_the_specific_reason(report_tool, tmp_path, case):
    corrupt, stage, message = MALFORMED_CACHES[case]
    cache = Map()
    path = cache.write(tmp_path)
    corrupt(cache, path)
    returncode, report = report_one(report_tool, path)
    assert returncode == 1
    if stage == "identify":
        assert report["identify"] == message
        assert "milestone.load" not in report
    else:
        assert report["identify"] == "ok"
        assert report["load"] == message
        assert report["milestone.load"] == "no"


MALFORMED_OPENSAUCE = {
    "header version": ({"flags": 1, "version": 3}, "the OpenSauce header is not valid (version, tag versions or memory upgrade)"),
    "project_yellow version": ({"flags": 1, "project_yellow": 1}, "the OpenSauce header is not valid (version, tag versions or memory upgrade)"),
    "project_yellow_globals version": ({"flags": 1, "project_yellow_globals": 3}, "the OpenSauce header is not valid (version, tag versions or memory upgrade)"),
    "memory upgrade amount": ({"flags": 1, "amount": 2.0}, "the OpenSauce header is not valid (version, tag versions or memory upgrade)"),
    "negative memory upgrade": ({"flags": 1, "amount": -1.0}, "the OpenSauce header is not valid (version, tag versions or memory upgrade)"),
    "nan memory upgrade": ({"flags": 1, "amount": float("nan")}, "the OpenSauce header is not valid (version, tag versions or memory upgrade)"),
    "undefined flag": ({"flags": 1 | 1 << 5}, "the OpenSauce header sets flags OpenSauce does not define"),
    "mod data files without a name": ({"flags": 2}, "the OpenSauce header is not valid (version, tag versions or memory upgrade)"),
}


@pytest.mark.parametrize("case", sorted(MALFORMED_OPENSAUCE))
def test_malformed_opensauce_headers_are_rejected(report_tool, tmp_path, case):
    options, message = MALFORMED_OPENSAUCE[case]
    path = Map(opensauce=options).write(tmp_path, "test.yelo")
    returncode, report = report_one(report_tool, path)
    assert returncode == 1
    assert report["identify"] == message


def test_opensauce_definitions_outside_the_file_are_rejected(report_tool, tmp_path):
    path = Map(opensauce={"flags": 1, "definitions_offset": 0x10}, definitions=b"x" * 64).write(tmp_path, "test.yelo")
    _, report = report_one(report_tool, path)
    assert report["identify"] == "the OpenSauce tag definitions lie outside the file"
    path = Map(opensauce={"flags": 1, "definitions_offset": 0x7FFFFFF0}, definitions=b"x" * 64).write(tmp_path, "test.yelo")
    _, report = report_one(report_tool, path)
    assert report["identify"] == "the OpenSauce tag definitions lie outside the file"


def test_unterminated_opensauce_strings_are_rejected(report_tool, tmp_path):
    path = Map(opensauce={"flags": 1}).write(tmp_path, "test.yelo")
    _, report = report_one(report_tool, patched(path, 0x70 + 0x40, "32s", b"m" * 32))
    assert report["identify"] == "a name or build string in the header is not terminated"


def test_too_small_files_are_rejected(report_tool, tmp_path):
    tiny = tmp_path / "tiny.map"
    tiny.write_bytes(b"12345678")
    _, report = report_one(report_tool, tiny)
    assert report["identify"] == "the file is too small to hold a header"
    headless = tmp_path / "headless.map"
    headless.write_bytes(struct.pack("<I", code("head")) + bytes(0x100))
    _, report = report_one(report_tool, headless)
    assert report["identify"] == "the file is too small to hold a header"


def test_missing_and_wrong_resource_maps_are_named(report_tool, tmp_path):
    path = Map().write(tmp_path)
    (tmp_path / "sounds.map").unlink()
    returncode, report = report_one(report_tool, path)
    assert returncode == 1
    assert report["resource_map.sounds"].endswith("(not found)")
    assert report["load"] == "a map needs a resource map that was not supplied"

    (tmp_path / "sounds.map").write_bytes((tmp_path / "loc.map").read_bytes())
    _, report = report_one(report_tool, path)
    assert report["resource_map.sounds"].endswith("(a resource map is not of the type needed)")
    assert report["load"] == "a map needs a resource map that was not supplied"


def resource_map_case(name, mutate):
    def corrupt(folder):
        path = folder / f"{name}.map"
        data = bytearray(path.read_bytes())
        mutate(data)
        path.write_bytes(data)
    return corrupt


def index_entry(data, item_index):
    return struct.unpack_from("<iiii", data, 0)[2] + item_index * 12


MALFORMED_RESOURCE_MAPS = {
    "index beyond file": (resource_map_case("bitmaps", lambda d: struct.pack_into("<i", d, 8, len(d))),
                          "resource_map.bitmaps", "a resource map header is not valid"),
    "names after index": (resource_map_case("bitmaps", lambda d: struct.pack_into("<i", d, 4, len(d) - 4)),
                          "resource_map.bitmaps", "a resource map header is not valid"),
    "count beyond file": (resource_map_case("loc", lambda d: struct.pack_into("<i", d, 12, 0x10000000)),
                          "resource_map.loc", "a resource map header is not valid"),
    "item data beyond names": (resource_map_case("sounds", lambda d: struct.pack_into("<i", d, index_entry(d, 1) + 4, 0x100000)),
                               "resource_map.sounds", "a resource map entry lies outside the file or its name is not terminated"),
    "item name beyond names": (resource_map_case("loc", lambda d: struct.pack_into("<i", d, index_entry(d, 0), 0x100000)),
                               "resource_map.loc", "a resource map entry lies outside the file or its name is not terminated"),
    "item data in header": (resource_map_case("loc", lambda d: struct.pack_into("<i", d, index_entry(d, 2) + 8, 4)),
                            "resource_map.loc", "a resource map entry lies outside the file or its name is not terminated"),
}


@pytest.mark.parametrize("case", sorted(MALFORMED_RESOURCE_MAPS))
def test_malformed_resource_maps_are_rejected(report_tool, tmp_path, case):
    corrupt, key, message = MALFORMED_RESOURCE_MAPS[case]
    path = Map().write(tmp_path)
    corrupt(tmp_path)
    returncode, report = report_one(report_tool, path)
    assert returncode == 1
    assert report[key].endswith(f"({message})")
    assert report["load"] == "a map needs a resource map that was not supplied"


def test_resource_tags_outside_their_entries_are_rejected(report_tool, tmp_path):
    def corrupt_first_block(data):
        # the bitmap definition is entry 1; its sequence block's address
        entry = index_entry(data, 1)
        item_offset = struct.unpack_from("<i", data, entry + 8)[0]
        struct.pack_into("<I", data, item_offset + 0x54 + 4, 0x10000)

    path = Map().write(tmp_path)
    resource_map_case("bitmaps", corrupt_first_block)(tmp_path)
    _, report = report_one(report_tool, path)
    assert report["load"] == "a tag held by a resource map does not have the documented layout"


def test_font_style_references_must_be_empty(report_tool, tmp_path):
    _, report = report_one(report_tool, Map(font_style_reference=0xE1740003).write(tmp_path))
    assert report["load"] == "a tag held by a resource map does not have the documented layout"


def test_resource_pixels_and_samples_beyond_their_files_are_rejected(report_tool, tmp_path):
    _, report = report_one(report_tool, Map(bitmap_pixels_size=0x7FFFFFFF).write(tmp_path / "pixels"))
    assert report["load"] == "bitmap pixels or sound samples lie outside their file"
    _, report = report_one(report_tool, Map(sound_samples_size=0x100000).write(tmp_path / "samples"))
    assert report["load"] == "bitmap pixels or sound samples lie outside their file"


def test_resource_tags_must_fit_below_the_structure_bsp(report_tool, tmp_path):
    cache = Map()
    cache.build()
    tag_data_bytes = len(cache.build()) - cache.where["tag_data"]
    # the BSP starts right where the tag data ends: no room for resource tags
    _, report = report_one(report_tool, Map(bsp_gap=tag_data_bytes).write(tmp_path))
    assert report["load"] == "the tag data and the tags held by resource maps do not fit below the structure BSP"


def test_missing_sound_entry_is_named(report_tool, tmp_path):
    path = Map().write(tmp_path)
    data = bytearray((tmp_path / "sounds.map").read_bytes())
    names_offset = struct.unpack_from("<i", data, 4)[0]
    at = data.index(b"test\\sound\0", names_offset + 1)
    data[at:at + 10] = b"test\\sounx"
    (tmp_path / "sounds.map").write_bytes(data)
    _, report = report_one(report_tool, path)
    assert report["load"] == "a tag's entry is missing from its resource map"


def test_seeded_corruption_never_crashes_the_loader(report_tool, tmp_path):
    """Sampled robustness, not proof: flip bytes in the header, tag index,
    tag data and resource maps of a valid cache and require a clean verdict
    (exit 0 or 1, checked by run_report) every time."""
    cache = Map(opensauce={"flags": 1})
    pristine = cache.build()
    resource_maps = cache.resource_maps()
    generator = random.Random(20260926)
    for trial in range(120):
        folder = tmp_path / f"trial{trial}"
        folder.mkdir()
        data = bytearray(pristine)
        targets = {"map": data}
        for type_name, contents in resource_maps.items():
            targets[type_name] = bytearray(contents)
        for _ in range(generator.randint(1, 8)):
            name = generator.choice(sorted(targets))
            target = targets[name]
            if name == "map" and generator.random() < 0.7:
                offset = generator.randrange(cache.where["tag_data"], len(target))
            else:
                offset = generator.randrange(len(target))
            target[offset] = generator.randrange(256)
        (folder / "test.yelo").write_bytes(targets["map"])
        for type_name in resource_maps:
            (folder / f"{type_name}.map").write_bytes(targets[type_name])
        run_report(report_tool, folder / "test.yelo")


# ---------- real maps, when present (never committed)


def real_maps_directory():
    configured = os.environ.get("HALO_CUSTOM_EDITION_MAPS")
    folder = Path(configured) if configured else ROOT / "assets" / "custom_edition"
    return folder if (folder / "bitmaps.map").is_file() else None


STOCK_CUSTOM_EDITION_MAPS = [
    "beavercreek", "bloodgulch", "boardingaction", "carousel", "chillout", "damnation", "dangercanyon",
    "deathisland", "gephyrophobia", "hangemhigh", "icefields", "infinity", "longest", "prisoner",
    "putput", "ratrace", "sidewinder", "timberland", "ui", "wizard",
]


@pytest.fixture(scope="module")
def real_maps():
    folder = real_maps_directory()
    if folder is None:
        pytest.skip("no Custom Edition maps: set HALO_CUSTOM_EDITION_MAPS or extract them to assets/custom_edition")
    return folder


def test_real_maps_every_stock_map_loads_with_a_matching_checksum(report_tool, real_maps):
    present = [real_maps / f"{name}.map" for name in STOCK_CUSTOM_EDITION_MAPS if (real_maps / f"{name}.map").is_file()]
    if not present:
        pytest.skip("no stock Custom Edition maps present")
    returncode, blocks = run_report(report_tool, *present)
    assert returncode == 0
    for block in blocks:
        assert block["load"] == "ok", block["file"]
        assert block["computed_checksum"] == block["checksum"], block["file"]
        assert block["warnings"] == "none", block["file"]


def test_real_maps_resource_maps_are_valid(report_tool, real_maps):
    returncode, blocks = run_report(report_tool, real_maps / "bitmaps.map", real_maps / "sounds.map",
                                    real_maps / "loc.map")
    assert returncode == 0
    assert [b["resource_map"] for b in blocks] == ["ok", "ok", "ok"]


def test_real_maps_every_opensauce_cache_is_recognized_and_explained(report_tool, real_maps):
    """Each OpenSauce cache either loads or names what it lacks; the ones in
    the documented sample behave as recorded."""
    paths = sorted(real_maps.glob("*.yelo")) + [
        path for path in sorted(real_maps.glob("*.map"))
        if path.stem not in STOCK_CUSTOM_EDITION_MAPS and path.stem not in ("bitmaps", "sounds", "loc")]
    if not paths:
        pytest.skip("no OpenSauce caches present")
    _, blocks = run_report(report_tool, *paths)
    recorded = {
        "beavercreek_halo3.yelo": ("ok", "OpenSauce tags"),
        "celer_exile_odst_v2.yelo": ("ok", "checksum mismatch, trailing data, OpenSauce tags"),
        "fy_killzone.yelo": ("a map needs a resource map that was not supplied", "OpenSauce tags"),
        "extinctionrevanepic2.map": ("a map needs a resource map that was not supplied", "OpenSauce tags"),
    }
    for block in blocks:
        name = Path(block["file"]).name
        assert block["identify"] == "ok", name
        assert block["format"] == "Custom Edition cache", name
        assert block["milestone.run"].startswith("no"), name
        if name in recorded:
            assert (block["load"], block["warnings"]) == recorded[name], name
