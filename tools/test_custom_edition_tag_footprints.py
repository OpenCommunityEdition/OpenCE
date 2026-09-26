"""Tests for tools/custom_edition_tag_footprints.py, on synthetic caches."""

import struct
import zlib

from tools import custom_edition_tag_footprints as footprints

CUSTOM_EDITION_BASE = 0x40440000
XBOX_BASE = 0x803A6000


def code(text):
    return struct.unpack(">I", text.encode("latin-1"))[0]


def cache(path, version, tags, compress=False):
    """A cache holding `tags`: (group, name, structure bytes, child bytes[,
    child count]). Each structure starts with a block of its child data,
    which follows it."""
    base = CUSTOM_EDITION_BASE if version == 609 else XBOX_BASE
    index_bytes = 0x28 if version == 609 else 0x24
    tag_data = bytearray(index_bytes + len(tags) * 0x20)
    struct.pack_into("<IIII", tag_data, 0, base + index_bytes, 0, 0, len(tags))
    for index, (group, name, structure_bytes, child_bytes, *child_count) in enumerate(tags):
        name_address = base + len(tag_data)
        tag_data += name.encode() + b"\0"
        while len(tag_data) % 4:
            tag_data.append(0)
        address = base + len(tag_data)
        body = bytearray(structure_bytes + child_bytes)
        if child_bytes:
            struct.pack_into("<iI", body, 0, child_count[0] if child_count else 1, address + structure_bytes)
        tag_data += body
        struct.pack_into("<IIII", tag_data, index_bytes + index * 0x20, code(group), 0xFFFFFFFF, 0xFFFFFFFF, index)
        struct.pack_into("<II", tag_data, index_bytes + index * 0x20 + 0x10, name_address, address)
    header = bytearray(0x800)
    struct.pack_into("<IiI", header, 0, code("head"), version, 0x800 + len(tag_data))
    struct.pack_into("<II", header, 0x10, 0x800, len(tag_data))
    body = zlib.compress(bytes(tag_data)) if compress else bytes(tag_data)
    path.write_bytes(bytes(header) + body)
    return path


def test_identical_groups_and_content_and_structure_differences(tmp_path, capsys):
    (tmp_path / "ce").mkdir()
    (tmp_path / "xbox").mkdir()
    cache(tmp_path / "ce" / "level.map", 609, [
        ("scen", "rocks\\rock", 0x40, 0x20),
        ("weap", "weapons\\rifle", 0x80, 0x30),     # more block data: content
        ("matg", "globals\\globals", 0x60, 0x10),   # a larger structure
        ("mod2", "models\\rock", 0x40, 0x100),
        ("devc", "ui\\input", 0x20, 0),
    ])
    cache(tmp_path / "xbox" / "level.map", 5, [
        ("scen", "rocks\\rock", 0x40, 0x20),
        ("weap", "weapons\\rifle", 0x80, 0x20),
        ("matg", "globals\\globals", 0x50, 0x10),
        ("mode", "models\\rock", 0x40, 0x80),
        ("sotr", "shaders\\glow", 0x20, 0),
    ], compress=True)
    assert footprints.main([str(tmp_path / "ce"), str(tmp_path / "xbox")]) == 0
    output = capsys.readouterr().out
    assert "levels compared: 1 (level)" in output
    assert "every level (1): scen" in output
    assert "'weap': footprints 0/1, structures 1/1" in output
    assert "'matg': footprints 0/1, structures 0/1" in output
    assert "groups only in Custom Edition caches (levels): {'devc': 1, 'mod2': 1}" in output
    assert "groups only in Xbox caches (levels): {'mode': 1, 'sotr': 1}" in output
    assert "models named alike, 'mod2' vs 'mode': 1, with the same footprint: 0" in output


def test_blocks_are_told_apart_by_count_presence_and_span(tmp_path, capsys):
    (tmp_path / "ce").mkdir()
    (tmp_path / "xbox").mkdir()
    cache(tmp_path / "ce" / "level.map", 609, [
        ("weap", "weapons\\rifle", 0x80, 0x40, 2),
        ("matg", "globals\\globals", 0x60, 0x10, 1),
        ("effe", "effects\\spark", 0x40, 0x20, 1),
        ("scen", "rocks\\rock", 0x40, 0x20),
    ])
    cache(tmp_path / "xbox" / "level.map", 5, [
        ("weap", "weapons\\rifle", 0x80, 0x80, 4),     # more elements: content
        ("matg", "globals\\globals", 0x60, 0x20, 1),   # same count, more data
        ("effe", "effects\\spark", 0x40, 0),           # no block at all
        ("scen", "rocks\\rock", 0x40, 0x20),
    ])
    assert footprints.main(["--blocks", str(tmp_path / "ce"), str(tmp_path / "xbox")]) == 0
    output = capsys.readouterr().out
    assert "'weap': footprints 0/1, structures 1/1, identical in 0 of 1 levels; " \
           "blocks differing in count 1, presence 0, span 0" in output
    assert "'matg': footprints 0/1, structures 1/1, identical in 0 of 1 levels; " \
           "blocks differing in count 0, presence 0, span 1" in output
    assert "'effe': footprints 0/1, structures 0/0, identical in 0 of 1 levels; " \
           "blocks differing in count 0, presence 1, span 0" in output
    assert "every level (1): scen" in output


def test_resource_held_tags_and_sound_headers_bound_their_neighbours(tmp_path):
    """A Custom Edition tag held by a resource map is not compared, but an
    address it keeps in the tag data still ends the tag before it."""
    path = cache(tmp_path / "level.map", 609, [
        ("lsnd", "sound\\loop", 0x40, 0),
        ("snd!", "sound\\held", 0xA4, 0),
    ])
    data = bytearray(path.read_bytes())
    struct.pack_into("<I", data, 0x800 + 0x28 + 0x20 + 0x18, 1)
    path.write_bytes(data)
    level = footprints.Cache(path)
    # the looping sound's structure, then the held sound's name (10
    # characters, a terminator and alignment), then the held sound's header
    assert level.footprints == {("lsnd", "sound\\loop"): 0x40 + 12}


def test_structure_size_is_the_first_child_block(tmp_path):
    path = cache(tmp_path / "level.map", 609, [("weap", "weapons\\rifle", 0x3E8, 0x40)])
    level = footprints.Cache(path)
    assert level.structure_size(("weap", "weapons\\rifle")) == 0x3E8
    path = cache(tmp_path / "level.map", 609, [("scen", "rocks\\rock", 0x40, 0)])
    assert footprints.Cache(path).structure_size(("scen", "rocks\\rock")) is None
