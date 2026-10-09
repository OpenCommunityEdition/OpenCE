"""Tests for loading Custom Edition caches, and refusing those that need
OpenSauce (port/linux/game/cache_file_formats.c).

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
# a handle of the map a resource map was built with, which names nothing here
STALE_HANDLE = 0xE1AB0037
TAG_CACHE_BYTES = 0x01700000
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
    item.u32(bitmap + 0x20, STALE_HANDLE)  # the tag in the map bitmaps.map was built with
    item.u32(bitmap + 0x24, NONE)
    return bytes(item.bytes)


SOUND_ENTRY_FIELDS = {"sample_rate": 1, "encoding": 1, "longest_permutation_length": 1234}


def sound_item(samples_offset, samples_size, pitch_ranges=1, compression=1):
    """A sound as sounds.map holds it: the header again, then pitch ranges and
    permutations whose addresses count from the first pitch range. The
    entry's header has the fields the map's copy leaves zero, and the
    permutations the runtime fields of the map sounds.map was built with."""
    header = bytearray(0xA4)
    struct.pack_into("<H", header, 0x06, SOUND_ENTRY_FIELDS["sample_rate"])
    struct.pack_into("<hh", header, 0x6C, SOUND_ENTRY_FIELDS["encoding"], compression)
    struct.pack_into("<i", header, 0x84, SOUND_ENTRY_FIELDS["longest_permutation_length"])
    struct.pack_into("<iII", header, 0x98, pitch_ranges, 0x5A1D818, 0xD39A3C)  # stale editing-kit pointers
    body = Blob()
    ranges = [body.reserve(0x48) for _ in range(pitch_ranges)]
    for pitch_range in ranges:
        permutation = body.reserve(0x7C)
        mouth = body.add(b"MOUTHDAT")
        body.block(pitch_range + 0x3C, 1, permutation)
        body.u16(permutation + 0x28, compression)
        body.u32(permutation + 0x2C, 0x0BADF00D)
        body.u32(permutation + 0x30, 0x0BADF00D)
        body.u32(permutation + 0x34, STALE_HANDLE)
        body.u32(permutation + 0x3C, STALE_HANDLE)
        body.data(permutation + 0x40, samples_size, flags=1, file_offset=samples_offset)
        body.data(permutation + 0x54, 8, data_address=mouth)
    return bytes(header + body.bytes)


def string_list_item(strings=("hello", "world!")):
    item = Blob()
    group = item.reserve(0x0C)
    references = item.reserve(len(strings) * 20)
    item.block(group, len(strings), references)
    for index, text in enumerate(strings):
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


GBXMODEL_PART = {"vertex_count": 3, "vertex_offset": 0, "strip_triangle_count": 1, "strip_offset": 0,
                 "triangle_type": 1, "vertex_type": 4, "local_node_count": 0}


def add_gbxmodel(blob, part_options):
    """A gbxmodel of one node and one geometry of one part, whose geometry
    lies in the synthetic model data (vertices below 0x200, strips after)."""
    options = dict(GBXMODEL_PART, **part_options)
    model = blob.reserve(0xE8)
    node = blob.reserve(0x9C)
    geometry = blob.reserve(0x30)
    part = blob.reserve(0x84)
    shader = blob.reserve(0x20)
    blob.block(model + 0xB8, 1, node)
    blob.block(model + 0xD0, 1, geometry)
    blob.block(model + 0xDC, 1, shader)
    blob.block(geometry + 0x24, 1, part)
    blob.u16(part + 0x44, options["triangle_type"])
    blob.u32(part + 0x48, options["strip_triangle_count"])
    blob.u32(part + 0x4C, options["strip_offset"])
    blob.u16(part + 0x54, options["vertex_type"])
    blob.u32(part + 0x58, options["vertex_count"])
    blob.u32(part + 0x64, options["vertex_offset"])
    blob.bytes[part - blob.base + 0x6B] = options["local_node_count"]
    return model


# the type Custom Edition gives each shader group, and the one this build does
SHADER_TYPES = {"senv": (3, 3), "soso": (4, 4), "sotr": (5, 5), "schi": (6, 6), "scex": (7, 6),
                "swat": (8, 7), "sgla": (9, 8), "smet": (10, 9), "spla": (11, 10)}


def add_shader(blob, group, options):
    """A shader of `group` with the type Custom Edition gives it; a chicago
    extended shader with four-stage and two-stage maps and extra flags."""
    size = {"scex": 0x78, "schi": 0x6C}.get(group, 0x28)
    shader = blob.reserve(size)
    blob.u16(shader + 0x24, options.get("type", SHADER_TYPES[group][0]))
    if group == "scex":
        four_stage = options.get("four_stage", 2)
        two_stage = options.get("two_stage", 1)
        blob.block(shader + 0x54, four_stage, blob.reserve(0xDC * four_stage) if four_stage else 0)
        blob.block(shader + 0x60, two_stage, blob.reserve(0xDC * two_stage) if two_stage else 0)
        blob.u32(shader + 0x6C, options.get("extra_flags", 0x5))
    return shader


def add_hud_placement(blob, placement, scale, flags):
    struct.pack_into("<ff", blob.bytes, placement + 4 - blob.base, *scale)
    blob.u16(placement + 0x0C, flags)


def add_weapon_hud(blob, placements, bitmap_index=None):
    """A weapon HUD interface with a static element for each of the first two
    placements and a crosshair of one item for the third, each (scale, flags);
    the first static and the crosshair draw the bitmap of tag bitmap_index."""
    hud = blob.reserve(0x17C)
    statics = blob.reserve(2 * 0xB4)
    crosshairs = blob.reserve(0x68)
    items = blob.reserve(0x6C)
    blob.block(hud + 0x60, 2, statics)
    blob.block(hud + 0x84, 1, crosshairs)
    blob.block(crosshairs + 0x34, 1, items)
    add_hud_placement(blob, statics + 0x24, *placements[0])
    add_hud_placement(blob, statics + 0xB4 + 0x24, *placements[1])
    add_hud_placement(blob, items, *placements[2])
    if bitmap_index is not None:
        blob.u32(statics + 0x48 + 0x0C, bitmap_index)
        blob.u32(crosshairs + 0x24 + 0x0C, bitmap_index)
    return hud


def add_animation_graph(blob, overlay_animation_index):
    """An animation graph of one animation and one object overlay."""
    graph = blob.reserve(0x80)
    overlay = blob.reserve(0x14)
    animation = blob.reserve(0xB4)
    blob.block(graph + 0x00, 1, overlay)
    blob.block(graph + 0x74, 1, animation)
    blob.u16(overlay, overlay_animation_index)
    return graph


BSP_MATERIAL = {"vertex_count": 3, "lightmap_vertex_count": 3, "bitmap_index": 0, "vertex_type": 0,
                "vertices_size": None, "vertices_offset": 0x3C0, "normal": (0.0, 0.0, 1.0)}
BSP_MATERIAL_OFFSET = 0x2C0


def structure_bsp_bytes(size, address, material=None):
    """A structure BSP: its header, and a structure with one lightmap of one
    material when `material` gives that material's options."""
    bsp = bytearray(size)
    struct.pack_into("<IiIiII", bsp, 0, address + 0x18, 0, 0, 0, 0, code("sbsp"))
    if material is not None:
        options = dict(BSP_MATERIAL, **material)
        structure, lightmap, element = 0x18, 0x2A0, BSP_MATERIAL_OFFSET
        vertex_count, lightmap_vertex_count = options["vertex_count"], options["lightmap_vertex_count"]
        vertices_size = options["vertices_size"]
        if vertices_size is None:
            vertices_size = vertex_count * 56 + lightmap_vertex_count * 20
        struct.pack_into("<iII", bsp, structure + 0x104, 1, address + lightmap, 0)
        struct.pack_into("<h", bsp, lightmap, options["bitmap_index"])
        struct.pack_into("<iII", bsp, lightmap + 0x14, 1, address + element, 0)
        struct.pack_into("<h", bsp, element + 0xB0, options["vertex_type"])
        struct.pack_into("<i", bsp, element + 0xB4, vertex_count)
        struct.pack_into("<i", bsp, element + 0xC8, lightmap_vertex_count)
        struct.pack_into("<iIIII", bsp, element + 0xD8, vertices_size, 0, 0, address + options["vertices_offset"], 0)
        if options["vertices_offset"] + vertices_size > size:
            return bsp  # the vertices say they lie beyond the BSP
        for vertex_index in range(vertex_count):
            vertex = options["vertices_offset"] + vertex_index * 56
            struct.pack_into("<9f", bsp, vertex + 12, *options["normal"], 1.0, 0.0, 0.0, 0.0, 1.0, 0.0)
        for vertex_index in range(lightmap_vertex_count):
            vertex = options["vertices_offset"] + vertex_count * 56 + vertex_index * 20
            struct.pack_into("<3f", bsp, vertex, 0.0, 0.0, 1.0)
    return bsp


class Map:
    """A complete, minimal Custom Edition cache and its resource maps.

    Every option changes one thing from the valid default, and `where`
    records the file offsets of the fields tests corrupt afterwards."""

    def __init__(self, opensauce_flags=None, trailing=b"",
                 extra_tags=(), bsp_gap=None, bitmap_pixels_size=16, sound_samples_size=32,
                 font_style_reference=NONE, pitch_ranges=1, bsp_sizes=(0x1000,), sound_compression=1,
                 model=None, bsp_material=None, shaders=(), animation_overlay=None,
                 weapon_hud=None, hud_bitmap_flags=None, in_map_bitmap=None, strings_name="test\\strings",
                 strings=("hello", "world!"), name=b"test", tags_checksum=0):
        self.bsp_sizes = bsp_sizes
        # the header's name and the tag data's checksum, by which Chimera's
        # map list knows a map
        self.name = name
        self.tags_checksum = tags_checksum
        self.sound_compression = sound_compression
        # opt-in tags and content, so the defaults above keep their counts
        self.model = model
        self.bsp_material = bsp_material
        self.shaders = shaders
        self.animation_overlay = animation_overlay
        self.weapon_hud = weapon_hud
        self.hud_bitmap_flags = hud_bitmap_flags
        # (width, height, type, format, flags) of the bitmap kept in the map
        self.in_map_bitmap = in_map_bitmap
        self.strings_name = strings_name
        self.strings = strings
        self.addresses = {}
        # OpenSauce's header (its signature, version 1 and these flags) where a
        # Custom Edition cache has padding, when not None
        self.opensauce_flags = opensauce_flags
        self.trailing = trailing
        self.extra_tags = extra_tags
        self.bsp_gap = bsp_gap
        self.bitmap_pixels_size = bitmap_pixels_size
        self.sound_samples_size = sound_samples_size
        self.font_style_reference = font_style_reference
        self.pitch_ranges = pitch_ranges
        self.where = {}

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
            ("test\\sound", sound_item(samples_offset, self.sound_samples_size, self.pitch_ranges,
                                       self.sound_compression)),
        ])
        loc, _ = resource_map_file(3, [
            (self.strings_name, string_list_item(self.strings)),
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
            ("ustr", self.strings_name, True),
            ("font", "test\\font", True),
            ("hmt ", "test\\hud messages", True),
            ("weap", "test\\weapon", False),
        ] + list(self.extra_tags)
        if self.model is not None:
            tags.append(("mod2", "test\\model", False))
        tags += [(group, f"test\\shader {index}", False) for index, (group, _) in enumerate(self.shaders)]
        if self.animation_overlay is not None:
            tags.append(("antr", "test\\animations", False))
        if self.weapon_hud is not None:
            tags.append(("wphi", "test\\weapon hud", False))
        # further structure BSPs go last, so the indices above never move
        bsp_tag_indices = [1] + [len(tags) + extra for extra in range(len(self.bsp_sizes) - 1)]
        tags += [("sbsp", f"test\\bsp {extra + 2}", False) for extra in range(len(self.bsp_sizes) - 1)]
        resource_indices = {"test\\external bitmap": 1, self.strings_name: 0, "test\\font": 1,
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
        if self.bsp_gap is None:
            bsp_addresses = [BASE + TAG_CACHE_BYTES - size for size in self.bsp_sizes]
        else:
            bsp_addresses = [BASE + self.bsp_gap for _ in self.bsp_sizes]

        tag_data = Blob(BASE)
        index = tag_data.reserve(0x28)
        instances = tag_data.reserve(len(tags) * 0x20)
        tag_data.u32(index + 0x00, instances)
        tag_data.u32(index + 0x04, salt << 16)
        tag_data.u32(index + 0x08, self.tags_checksum)
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
        if self.in_map_bitmap is not None:
            width, height, kind, bitmap_format, flags = self.in_map_bitmap
            tag_data.u16(bitmap + 0x04, width)
            tag_data.u16(bitmap + 0x06, height)
            tag_data.u16(bitmap + 0x0A, kind)
            tag_data.u16(bitmap + 0x0C, bitmap_format)
            tag_data.u16(bitmap + 0x0E, flags)
        address_of["in map bitmap data"] = bitmap
        address_of["test\\in map bitmap"] = group
        # the header the map keeps of a sound held by sounds.map
        sound = tag_data.reserve(0xA4)
        tag_data.u32(sound + 0x98, self.pitch_ranges)
        address_of["test\\sound"] = sound
        address_of["test\\weapon"] = tag_data.add(bytes(0x100))
        for group_name, name, external in self.extra_tags:
            address_of[name] = tag_data.add(bytes(0x40))
        if self.model is not None:
            address_of["test\\model"] = add_gbxmodel(tag_data, self.model)
        for index, (group, options) in enumerate(self.shaders):
            address_of[f"test\\shader {index}"] = add_shader(tag_data, group, options)
        if self.animation_overlay is not None:
            address_of["test\\animations"] = add_animation_graph(tag_data, self.animation_overlay)
        if self.weapon_hud is not None:
            hud_bitmap_index = None
            if self.hud_bitmap_flags is not None:
                tag_data.u16(address_of["test\\in map bitmap"] + 6, self.hud_bitmap_flags)
                hud_bitmap_index = [name for _, name, _ in tags].index("test\\in map bitmap")
            address_of["test\\weapon hud"] = add_weapon_hud(tag_data, self.weapon_hud, hud_bitmap_index)
        for tag_index, (group_name, name, external) in enumerate(tags):
            if name in address_of:
                tag_data.u32(instances + tag_index * 0x20 + 0x14, address_of[name])

        # the file
        data = bytearray(HEADER_BYTES)
        for bsp_index, (size, address) in enumerate(zip(self.bsp_sizes, bsp_addresses)):
            data += structure_bsp_bytes(size, address, self.bsp_material if bsp_index == 0 else None)
        data += model_data + in_map_pixels + tag_data.bytes
        file_length = len(data)
        data += self.trailing
        header = data
        struct.pack_into("<IiIIII", header, 0, code("head"), 609, file_length, 0,
                         tag_data_offset, len(tag_data.bytes))
        header[0x20:0x20 + len(self.name) + 1] = self.name + b"\0"
        header[0x40:0x40 + 14] = b"01.00.00.0609\0"
        struct.pack_into("<h", header, 0x60, 1)
        struct.pack_into("<I", header, 0x7FC, code("foot"))
        if self.opensauce_flags is not None:
            struct.pack_into("<IhH", header, 0x70, code("yelo"), 1, self.opensauce_flags)
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
            "bsp_material": bsp_offset + BSP_MATERIAL_OFFSET,
        }
        if self.model is not None:
            self.where["model"] = tag_data_offset + (address_of["test\\model"] - BASE)
        self.addresses = dict(address_of, instances=instances)
        self.tag_indices = {name: index for index, (_, name, _) in enumerate(tags)}
        self.salt = salt
        return bytes(data)

    def write(self, folder, name="test.map", resource_maps=True):
        folder.mkdir(parents=True, exist_ok=True)
        path = folder / name
        path.write_bytes(self.build())
        if resource_maps:
            for type_name, data in self.resource_maps().items():
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
    # the tool loads and converts what it can from the bytes alone; running
    # takes the game (docs/custom_edition_caches.md)
    assert report["milestone.run"].startswith("not observed by this tool")
    assert report["convert"] == "ok"


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


def test_a_protected_scenario_is_given_the_scenario_group(report_tool, tmp_path):
    """Map protection renames the scenario tag's group (to 'prot', say). Halo PC
    used the tag the header names regardless, so the loader gives it the
    scenario's group and loads the map."""
    cache = Map()
    path = cache.write(tmp_path)
    instance_case(0, 0x00, "<I", code("prot"))(cache, path)
    returncode, report = report_one(report_tool, path)
    assert returncode == 0
    assert report["load"] == "ok"
    assert report["scenario_regrouped"] == "1"


def test_a_structure_bsp_instance_with_its_load_address_loads(report_tool, tmp_path):
    """Invader writes the address a structure BSP loads at in its tag
    instance (cursed-damnation); it has none until it is loaded."""
    cache = Map()
    path = instance_case(1, 0x14, "<I", BASE + 0x1500000)(cache, cache.write(tmp_path))
    returncode, report = report_one(report_tool, path)
    assert returncode == 0 and report["load"] == "ok"


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


OPENSAUCE_REFUSED = ("an OpenSauce map that needs OpenSauce (its header asks for memory upgrades, mod data files or "
                     "the like), which this build does not run")


@pytest.mark.parametrize("flags", [1, 2, 4, 1 << 15])
def test_caches_that_need_opensauce_are_refused(report_tool, tmp_path, flags):
    """A cache whose OpenSauce header asks for anything of OpenSauce's
    (memory upgrades, mod data files, any flag) is refused as it is
    identified."""
    path = Map(opensauce_flags=flags).write(tmp_path)
    returncode, report = report_one(report_tool, path)
    assert returncode == 1
    assert report["identify"] == OPENSAUCE_REFUSED
    assert "load" not in report


def test_caches_that_only_carry_opensauce_data_load(report_tool, tmp_path):
    """A cache with OpenSauce's header asking for nothing, and its
    project_yellow and project_yellow_globals tags, runs as stock Custom
    Edition runs it: neither is read (SPV3's backwards-compatible a50.map)."""
    path = Map(opensauce_flags=0, extra_tags=[("yelo", "test\\project yellow", False),
                                              ("gelo", "test\\project yellow globals", False)]).write(tmp_path)
    returncode, report = report_one(report_tool, path)
    assert returncode == 0
    assert report["identify"] == "ok" and report["load"] == "ok"


def test_resource_maps_are_found_in_the_maps_directory_option(report_tool, tmp_path):
    Map().write(tmp_path / "maps")
    moved = tmp_path / "elsewhere" / "test.map"
    moved.parent.mkdir()
    (tmp_path / "maps" / "test.map").rename(moved)
    returncode, report = report_one(report_tool, moved, "--maps", tmp_path / "maps")
    assert returncode == 0 and report["load"] == "ok"


# ---------- geometry checked at load, and conversion for this build


def converted(report_tool, cache, folder):
    """Loads and converts `cache`, returning the report and the converted tags
    as they would sit at BASE (empty when nothing was converted)."""
    path = cache.write(folder)
    dump = folder / "converted_tags.bin"
    returncode, report = report_one(report_tool, path, "--dump-tags", dump)
    return returncode, report, dump.read_bytes() if dump.is_file() else b""


def u16_at(tags, address):
    return struct.unpack_from("<H", tags, address - BASE)[0]


def s16_at(tags, address):
    return struct.unpack_from("<h", tags, address - BASE)[0]


def u32_at(tags, address):
    return struct.unpack_from("<I", tags, address - BASE)[0]


def test_valid_geometry_loads_and_is_counted(report_tool, tmp_path):
    returncode, report, _ = converted(report_tool, Map(model={}, bsp_material={}), tmp_path)
    assert returncode == 0
    assert report["load"] == "ok" and report["convert"] == "ok"
    assert report["structure_bsp_materials_checked"] == "1"
    assert report["model_data"] == f"0x400 bytes at {hex(HEADER_BYTES + 0x1000)}, strips from 0x200"


def test_a_material_without_a_lightmap_has_no_lightmap_vertices(report_tool, tmp_path):
    material = {"bitmap_index": -1, "lightmap_vertex_count": 0}
    returncode, report, _ = converted(report_tool, Map(bsp_material=material), tmp_path)
    assert returncode == 0 and report["structure_bsp_materials_checked"] == "1"


BAD_MODEL_PART = "a model part's strip or vertices lie outside the model data or are not of the kind Custom Edition writes"
MALFORMED_MODEL_PARTS = {
    "vertices beyond the vertex data": {"vertex_offset": 0x1C0},
    "vertices unaligned": {"vertex_offset": 2},
    "no vertices": {"vertex_count": 0},
    "more vertices than strips can index": {"vertex_count": 0x10000},
    "strip beyond the index data": {"strip_offset": 0x1FE},
    "strip unaligned": {"strip_offset": 1},
    "empty strip": {"strip_triangle_count": 0},
    "huge strip": {"strip_triangle_count": 0x7FFFFFFF},
    "not a strip": {"triangle_type": 0},
    "compressed vertices": {"vertex_type": 5},
    "too many local nodes": {"local_node_count": 23},
}


@pytest.mark.parametrize("case", sorted(MALFORMED_MODEL_PARTS))
def test_malformed_model_parts_are_rejected(report_tool, tmp_path, case):
    returncode, report, _ = converted(report_tool, Map(model=MALFORMED_MODEL_PARTS[case]), tmp_path)
    assert returncode == 1
    assert report["load"] == BAD_MODEL_PART


def test_model_blocks_outside_the_tags_are_rejected(report_tool, tmp_path):
    cache = Map(model={})
    path = patched(cache.write(tmp_path), cache.where["model"] + 0xD4, "<I", BASE + 0x1000000)
    returncode, report = report_one(report_tool, path)
    assert returncode == 1
    assert report["load"] == "a tag block or tag data field lies outside the loaded tags"


BAD_BSP_GEOMETRY = "a structure BSP's lightmaps or their materials do not have the documented layout"
MALFORMED_BSP_MATERIALS = {
    "compressed vertices": {"vertex_type": 1},
    "vertex data of the wrong size": {"vertices_size": 3 * 56},
    "lightmap vertices without a lightmap": {"bitmap_index": -1},
    "no lightmap vertices with a lightmap": {"lightmap_vertex_count": 0},
    "vertices beyond the BSP": {"vertices_offset": 0xF80},
    "vertices unaligned": {"vertices_offset": 0x3C2},
    "negative vertex count": {"vertex_count": -1, "lightmap_vertex_count": -1, "vertices_size": 0},
    "normal too long": {"normal": (0.0, 0.0, 1.5)},
    "normal not a number": {"normal": (0.0, float("nan"), 1.0)},
}


@pytest.mark.parametrize("case", sorted(MALFORMED_BSP_MATERIALS))
def test_malformed_structure_bsp_materials_are_rejected(report_tool, tmp_path, case):
    returncode, report, _ = converted(report_tool, Map(bsp_material=MALFORMED_BSP_MATERIALS[case]), tmp_path)
    assert returncode == 1
    assert report["load"] == BAD_BSP_GEOMETRY


def test_structure_bsp_lightmaps_outside_the_bsp_are_rejected(report_tool, tmp_path):
    cache = Map(bsp_material={})
    path = patched(cache.write(tmp_path), cache.where["bsp"] + 0x18 + 0x108, "<I", BASE)
    _, report = report_one(report_tool, path)
    assert report["load"] == BAD_BSP_GEOMETRY


def test_shaders_are_renumbered_as_this_build_numbers_them(report_tool, tmp_path):
    groups = sorted(SHADER_TYPES)
    cache = Map(shaders=[(group, {}) for group in groups])
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0 and report["convert"] == "ok"
    for index, group in enumerate(groups):
        shader = cache.addresses[f"test\\shader {index}"]
        assert s16_at(tags, shader + 0x24) == SHADER_TYPES[group][1], group
    assert report["shaders_renumbered"] == "5"  # scex, swat, sgla, smet, spla


def test_chicago_extended_shaders_become_chicago_shaders(report_tool, tmp_path):
    cache = Map(shaders=[("scex", {"four_stage": 2, "two_stage": 1, "extra_flags": 0x5}),
                         ("scex", {"four_stage": 0, "two_stage": 1, "extra_flags": 0x2})])
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0
    assert report["chicago_extended_shaders_converted"] == "2"
    for index, (maps_count, extra_flags) in enumerate([(2, 0x5), (1, 0x2)]):
        shader = cache.addresses[f"test\\shader {index}"]
        instance = cache.addresses["instances"] + cache.tag_indices[f"test\\shader {index}"] * 0x20
        assert u32_at(tags, instance) == code("schi")
        assert u32_at(tags, instance + 0x04) == NONE  # the parent groups stay
        # the four-stage maps, or the two-stage ones when there are no others
        assert u32_at(tags, shader + 0x54) == maps_count
        assert u32_at(tags, shader + 0x60) == extra_flags
        assert tags[shader - BASE + 0x64:shader - BASE + 0x6C] == bytes(8)


def test_a_shader_with_another_groups_type_is_given_its_groups(report_tool, tmp_path):
    cache = Map(shaders=[("swat", {"type": 7})])
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0
    assert report["shaders_mistyped"] == "1"
    shader = cache.addresses["test\\shader 0"]
    assert u16_at(tags, shader + 0x24) == SHADER_TYPES["swat"][1]


@pytest.mark.parametrize("bitmap, made_linear, flags", [
    ((3840, 64, 0, 11, 0x81), 1, 0x90),  # birdcage's needler plasma: linear, no longer "power of two"
    ((256, 64, 0, 11, 0x01), 0, 0x01),   # powers of two: swizzled as before
    ((96, 96, 0, 14, 0x03), 0, 0x03),    # DXT1: linear ones cannot be compressed, so it stays
    ((96, 96, 0, 17, 0x04), 0, 0x04),    # P8: nor palettized
    ((96, 96, 2, 11, 0x00), 0, 0x00),    # a cube map: only 2D bitmaps can be linear
    ((100, 50, 0, 11, 0x10), 0, 0x10),   # linear already
], ids=["npot", "power of two", "dxt1", "p8", "cube map", "linear"])
def test_bitmaps_of_sides_only_halo_pc_draws_are_made_linear(report_tool, tmp_path, bitmap, made_linear, flags):
    cache = Map(in_map_bitmap=bitmap)
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0 and report["bitmaps_made_linear"] == str(made_linear)
    assert u16_at(tags, cache.addresses["in map bitmap data"] + 0x0E) == flags


def test_bitmaps_name_their_own_tag(report_tool, tmp_path):
    cache = Map()
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0 and report["bitmaps_prepared"] == "2"
    for name in ("test\\in map bitmap", "test\\external bitmap"):
        index = cache.tag_indices[name]
        group = u32_at(tags, cache.addresses["instances"] + index * 0x20 + 0x14)
        bitmap = u32_at(tags, group + 0x64)
        assert u32_at(tags, bitmap + 0x20) == ((cache.salt + index) << 16 | index), name
        assert u32_at(tags, bitmap + 0x24) == NONE
        assert u32_at(tags, bitmap + 0x28) == 0 and u32_at(tags, bitmap + 0x2C) == 0


def sound_parts(tags, cache):
    """The sound's header, and its first permutation (after conversion)."""
    header = cache.addresses["test\\sound"]
    pitch_ranges = u32_at(tags, header + 0x9C)
    permutation = u32_at(tags, pitch_ranges + 0x40) if pitch_ranges else None
    return header, permutation


def test_sounds_take_what_the_maps_copy_lacks_from_sounds_map(report_tool, tmp_path):
    cache = Map()
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0 and report["sounds_undecodable"] == "0"
    header, permutation = sound_parts(tags, cache)
    assert u16_at(tags, header + 0x06) == SOUND_ENTRY_FIELDS["sample_rate"]
    assert s16_at(tags, header + 0x6C) == SOUND_ENTRY_FIELDS["encoding"]
    assert s16_at(tags, header + 0x6E) == 1
    assert u32_at(tags, header + 0x84) == SOUND_ENTRY_FIELDS["longest_permutation_length"]
    # its permutations name the sound itself, and have no cache block or samples
    index = cache.tag_indices["test\\sound"]
    handle = (cache.salt + index) << 16 | index
    assert [u32_at(tags, permutation + offset) for offset in (0x2C, 0x30, 0x34, 0x3C)] == [NONE, 0, handle, handle]


def test_sounds_this_build_cannot_decode_are_made_unplayable(report_tool, tmp_path):
    cache = Map(sound_compression=3)  # Ogg Vorbis
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0
    assert report["sounds_undecodable"] == "1"
    assert u32_at(tags, cache.addresses["test\\sound"] + 0x98) == 0


def test_animation_overlays_naming_missing_animations_are_disabled(report_tool, tmp_path):
    cache = Map(animation_overlay=1)  # the graph has one animation
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0 and report["animation_overlays_disabled"] == "1"
    overlay = u32_at(tags, cache.addresses["test\\animations"] + 0x04)
    assert s16_at(tags, overlay) == -1


def test_animation_overlays_naming_real_animations_are_kept(report_tool, tmp_path):
    cache = Map(animation_overlay=0)
    _, report, tags = converted(report_tool, cache, tmp_path)
    assert report["animation_overlays_disabled"] == "0"
    assert s16_at(tags, u32_at(tags, cache.addresses["test\\animations"] + 0x04)) == 0


def f32_at(tags, address):
    return struct.unpack_from("<f", tags, address - BASE)[0]


def test_hud_elements_with_the_high_resolution_scale_are_halved(report_tool, tmp_path):
    """Halo PC draws them from bitmaps at twice the Xbox's size; this build
    ignores the flag, so the scale takes it in and the flag goes. Halo PC
    reads it only on statics, meters and numbers: a crosshair's item keeps
    its scale and flag."""
    cache = Map(weapon_hud=[((1.0, 0.5), 4 | 1), ((1.0, 1.0), 1), ((2.0, 2.0), 4)])
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0 and report["hud_placements_rescaled"] == "1"
    hud = cache.addresses["test\\weapon hud"]
    statics = u32_at(tags, hud + 0x64)
    items = u32_at(tags, u32_at(tags, hud + 0x88) + 0x38)
    assert (f32_at(tags, statics + 0x28), f32_at(tags, statics + 0x2C), u16_at(tags, statics + 0x30)) == (0.5, 0.25, 1)
    assert (f32_at(tags, items + 4), f32_at(tags, items + 8), u16_at(tags, items + 0x0C)) == (2.0, 2.0, 4)


def test_hud_elements_without_the_high_resolution_scale_keep_theirs(report_tool, tmp_path):
    cache = Map(weapon_hud=[((1.0, 1.0), 0), ((0.75, 0.75), 3), ((1.0, 1.0), 2)])
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0 and report["hud_placements_rescaled"] == "0"
    statics = u32_at(tags, cache.addresses["test\\weapon hud"] + 0x64)
    assert (f32_at(tags, statics + 0xB4 + 0x28), u16_at(tags, statics + 0xB4 + 0x30)) == (0.75, 3)


@pytest.mark.parametrize("flag", [1 << 4, 1 << 7], ids=["half hud scale", "force hud use highres scale"])
def test_hud_elements_drawing_a_halo_pc_half_scale_bitmap_are_halved(report_tool, tmp_path, flag):
    """Halo PC's bitmap flags halve the scale of every HUD element drawing the
    bitmap, as the element's own high resolution scale does: the first static
    and the crosshair's item draw it, the second static does not."""
    cache = Map(weapon_hud=[((1.0, 0.5), 1), ((1.0, 1.0), 1), ((2.0, 2.0), 0)], hud_bitmap_flags=flag)
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0 and report["hud_placements_rescaled"] == "2"
    hud = cache.addresses["test\\weapon hud"]
    statics = u32_at(tags, hud + 0x64)
    items = u32_at(tags, u32_at(tags, hud + 0x88) + 0x38)
    assert (f32_at(tags, statics + 0x28), f32_at(tags, statics + 0x2C), u16_at(tags, statics + 0x30)) == (0.5, 0.25, 1)
    assert (f32_at(tags, statics + 0xB4 + 0x28), u16_at(tags, statics + 0xB4 + 0x30)) == (1.0, 1)
    assert (f32_at(tags, items + 4), f32_at(tags, items + 8)) == (1.0, 1.0)


@pytest.mark.parametrize("name, tags_checksum, listed", [
    (b"rev_snowcast_cavebeta", 1683840640, True),
    (b"Rev_Snowcast_CaveBeta", 1683840640, True),
    (b"rev_snowcast_cavebeta", 1683840641, False),
], ids=["listed", "name in another case", "another checksum"])
def test_maps_chimera_lists_get_halo_pc_behaviours(report_tool, tmp_path, name, tags_checksum, listed):
    """Chimera's map list knows a map by its name in lower case and its tag
    data checksum. This one relies on, among others, bitmaps' half HUD scale
    flags being ignored, so the static drawing a bitmap with one keeps its
    scale."""
    cache = Map(weapon_hud=[((1.0, 0.5), 1), ((1.0, 1.0), 1), ((2.0, 2.0), 0)], hud_bitmap_flags=1 << 4,
                name=name, tags_checksum=tags_checksum)
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0
    assert report["halo_pc_behaviours"] == (
        "gearbox_chicago_multiply gearbox_meters hud_number_scale disable_bitmap_hud_scale_flags" if listed else "none")
    assert report["hud_placements_rescaled"] == ("0" if listed else "2")


SCORE_HINT = 'Hold "%s" for score'


def test_the_score_hint_names_the_back_button(report_tool, tmp_path):
    """Halo PC formats the hint with its score key's name; this build copies
    it as it is, so the placeholder becomes the Xbox button."""
    cache = Map(strings_name="ui\\multiplayer_game_text", strings=[""] * 100 + [SCORE_HINT])
    returncode, report, tags = converted(report_tool, cache, tmp_path)
    assert returncode == 0 and report["score_hint_converted"] == "1"
    assert "Hold BACK for score".encode("utf-16-le") in tags
    assert SCORE_HINT.encode("utf-16-le") not in tags


def test_other_strings_keep_their_placeholders(report_tool, tmp_path):
    for strings_name, strings in (("test\\strings", [""] * 100 + [SCORE_HINT]),
                                  ("ui\\multiplayer_game_text", [SCORE_HINT] * 100)):
        cache = Map(strings_name=strings_name, strings=strings)
        returncode, report, tags = converted(report_tool, cache, tmp_path / str(len(strings_name)))
        assert returncode == 0 and report["score_hint_converted"] == "0"
        assert SCORE_HINT.encode("utf-16-le") in tags


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
    "external scenario": (instance_case(0, 0x18, "<I", 1), "load",
                          "a tag of a group that resource maps never hold is marked as held by one"),
    "bsp count": (tag_data_case("scenario", 0x5A4, "<i", 33), "load",
                  "the scenario's structure BSP block is not valid"),
    "bsp block outside": (tag_data_case("scenario", 0x5A8, "<I", 0x10), "load",
                          "the scenario's structure BSP block is not valid"),
    "bsp reference to a weapon": (tag_data_case("reference", 0x1C, "<I", (0xE174 + 8) << 16 | 8), "load",
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


def test_a_file_length_of_zero_is_the_whole_file(report_tool, tmp_path):
    """Invader leaves the header's file length 0 (blood_covenantv3)."""
    path = Map().write(tmp_path, "test.map")
    returncode, report = report_one(report_tool, patched(path, 0x08, "<I", 0))
    assert returncode == 0 and report["identify"] == "ok" and report["load"] == "ok"
    assert report["file_length"] == hex(path.stat().st_size)


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
    cache = Map()
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
        (folder / "test.map").write_bytes(targets["map"])
        for type_name in resource_maps:
            (folder / f"{type_name}.map").write_bytes(targets[type_name])
        run_report(report_tool, folder / "test.map")


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


def test_real_maps_opensauce_caches_are_recognized(report_tool, real_maps):
    """Each .yelo map present is either refused for needing OpenSauce or
    identified as a Custom Edition cache (a .yelo that asks for nothing of
    OpenSauce's; the game never looks for .yelo files)."""
    paths = sorted(real_maps.glob("*.yelo"))
    if not paths:
        pytest.skip("no OpenSauce caches present")
    _, blocks = run_report(report_tool, *paths)
    for block in blocks:
        assert block["identify"] in (OPENSAUCE_REFUSED, "ok"), block["file"]


def test_real_maps_convert_as_recorded(report_tool, real_maps):
    """The map run in the native build (docs/custom_edition_caches.md)
    converts as recorded there."""
    recorded = {
        "bloodgulch.map": {"structure_bsp_materials_checked": "79", "shaders_renumbered": "21",
                           "chicago_extended_shaders_converted": "10", "bitmaps_prepared": "676",
                           "animation_overlays_disabled": "0",
                           "sounds_undecodable": "39", "hud_placements_rescaled": "32",
                           "score_hint_converted": "1"},
    }
    present = [real_maps / name for name in recorded if (real_maps / name).is_file()]
    if not present:
        pytest.skip("the recorded map is not present")
    returncode, blocks = run_report(report_tool, *present)
    assert returncode == 0
    for block in blocks:
        name = Path(block["file"]).name
        assert block["convert"] == "ok", name
        assert {key: block[key] for key in recorded[name]} == recorded[name], name
