"""Compare the tags of Custom Edition caches with the Xbox caches of the same levels.

Research for running Custom Edition maps (docs/custom_edition_caches.md): it
measures how far Custom Edition's tags are from the Xbox ones, without any tag
definitions. For every tag kept in both caches under the same group and name:

- its footprint: the distance from the tag's address to the next tag's
  address in its cache (its structure and the block data laid out after it);
- its structure size, estimated as the distance to the lowest address inside
  the footprint that the footprint itself points at (its first child block).

Equal footprints for every tag of a group, level after level, suggest the
group's layout is the same in both builds; they do not prove it. A group
whose structures match but whose footprints differ differs in its block data:
in content, or in the layout of nested elements.

With --blocks, the top-level blocks of every tag whose footprints differ are
compared too. They are found without definitions, as (count, address,
definition) fields of the structure whose address points into the tag's
footprint and whose definition pointer is 0, as caches leave it. Each block
that differs is counted by kind: present in one build only, a different
element count (content), or the same count over a different span of data up
to the next block's data (element size or nested content: the walk cannot
tell which).

    python tools/custom_edition_tag_footprints.py [--blocks] CUSTOM_EDITION_MAPS XBOX_MAPS

Both arguments are folders; levels present in both are compared. Xbox caches
may be compressed as on the disc. No map is read beyond its header and tag
data, and nothing is written.
"""

import argparse
import collections
from pathlib import Path
import struct
import sys
import zlib

CACHE_HEADER_BYTES = 0x800
CUSTOM_EDITION_VERSION = 609
XBOX_VERSION = 5
CUSTOM_EDITION_TAG_CACHE_ADDRESS = 0x40440000
CUSTOM_EDITION_TAG_INDEX_BYTES = 0x28
XBOX_TAG_INDEX_BYTES = 0x24
TAG_INSTANCE_BYTES = 0x20
TAG_BLOCK_BYTES = 12
MAXIMUM_BLOCK_COUNT = 0xFFFF
RESOURCE_MAP_NAMES = ("bitmaps.map", "sounds.map", "loc.map")


def group_name(tag):
    return struct.pack(">I", tag).decode("latin-1")


def read_cache(path):
    """The version and the tag data of a cache, decompressing an Xbox cache
    whose header declares more than the file holds."""
    raw = Path(path).read_bytes()
    version, file_length = struct.unpack_from("<iI", raw, 4)
    if version == XBOX_VERSION and file_length > len(raw):
        raw = raw[:CACHE_HEADER_BYTES] + zlib.decompress(raw[CACHE_HEADER_BYTES:])
    if version not in (XBOX_VERSION, CUSTOM_EDITION_VERSION):
        raise ValueError(f"{path}: cache version {version} is neither Xbox nor Custom Edition")
    offset, size = struct.unpack_from("<II", raw, 0x10)
    return version, raw[offset:offset + size]


class Cache:
    """The tags of one cache: address and footprint by (group, name)."""

    def __init__(self, path):
        version, self.tag_data = read_cache(path)
        instances, _, _, count = struct.unpack_from("<IIII", self.tag_data, 0)
        if version == CUSTOM_EDITION_VERSION:
            self.base = CUSTOM_EDITION_TAG_CACHE_ADDRESS
        else:
            self.base = instances - XBOX_TAG_INDEX_BYTES
        end = self.base + len(self.tag_data)
        addresses = {}
        boundaries = []
        for index in range(count):
            at = instances - self.base + index * TAG_INSTANCE_BYTES
            group, = struct.unpack_from("<I", self.tag_data, at)
            name_address, address, in_resource_map = struct.unpack_from("<III", self.tag_data, at + 0x10)
            if version == XBOX_VERSION:
                in_resource_map = 0
            if not self.base <= address < end:
                # not loaded yet (structure BSPs) or held by a resource map
                continue
            name_at = name_address - self.base
            name = self.tag_data[name_at:self.tag_data.index(b"\0", name_at)].decode("latin-1")
            # every address in the tag data bounds the tag before it, even the
            # sound headers Custom Edition keeps for sounds held by sounds.map
            boundaries.append(address)
            if not in_resource_map:
                addresses[(group_name(group), name)] = address
        boundaries = sorted(set(boundaries)) + [end]
        following = {address: next_address for address, next_address in zip(boundaries, boundaries[1:])}
        self.addresses = addresses
        self.footprints = {key: following[address] - address for key, address in addresses.items()}

    def structure_size(self, key):
        """The distance from the tag to the lowest address inside its
        footprint that the footprint points at, or None."""
        address = self.addresses[key]
        start = address - self.base
        end = start + self.footprints[key]
        values = struct.unpack_from(f"<{(end - start) // 4}I", self.tag_data, start)
        inside = [value for value in values if address < value < address + self.footprints[key]]
        return min(inside) - address if inside else None

    def top_level_blocks(self, key):
        """The tag's blocks, by offset in its structure: (element count, span
        of data per element up to the next block's data)."""
        address = self.addresses[key]
        footprint = self.footprints[key]
        structure = self.structure_size(key)
        if structure is None:
            return {}
        start = address - self.base
        found = {}
        for offset in range(0, structure - TAG_BLOCK_BYTES + 1, 4):
            count, pointer, definition = struct.unpack_from("<iII", self.tag_data, start + offset)
            if 0 < count <= MAXIMUM_BLOCK_COUNT and address < pointer < address + footprint and not definition:
                found[offset] = (count, pointer - address)
        starts = sorted({relative for _, relative in found.values()}) + [footprint]
        return {offset: (count, (starts[starts.index(relative) + 1] - relative) // count)
                for offset, (count, relative) in found.items()}


def block_differences(custom_edition, xbox, key):
    """How a tag's top-level blocks differ between the two caches: blocks
    present in one only, with other element counts, or with the same count
    over another span of data."""
    left, right = custom_edition.top_level_blocks(key), xbox.top_level_blocks(key)
    kinds = collections.Counter()
    for offset in set(left) | set(right):
        ours, theirs = left.get(offset), right.get(offset)
        if ours == theirs:
            continue
        if ours is None or theirs is None:
            kinds["presence"] += 1
        elif ours[0] != theirs[0]:
            kinds["count"] += 1
        else:
            kinds["span"] += 1
    return kinds


def compare(custom_edition, xbox, blocks=False):
    """Per group: how many tags both caches keep, how many have the same
    footprint, how many the same estimated structure size, and with
    `blocks`, how the blocks of the others differ."""
    result = collections.defaultdict(collections.Counter)
    for key in set(custom_edition.footprints) & set(xbox.footprints):
        counts = result[key[0]]
        counts["shared"] += 1
        if custom_edition.footprints[key] == xbox.footprints[key]:
            counts["same footprint"] += 1
        elif blocks:
            for kind, number in block_differences(custom_edition, xbox, key).items():
                counts[f"block {kind}"] += number
        sizes = custom_edition.structure_size(key), xbox.structure_size(key)
        if None not in sizes:
            counts["structures measured"] += 1
            if sizes[0] == sizes[1]:
                counts["same structure"] += 1
    return result


def model_pairs(custom_edition, xbox):
    """Models named alike, gbxmodel ('mod2') in Custom Edition and model
    ('mode') on the Xbox: how many, and how many with the same footprint."""
    names = ({name for group, name in custom_edition.footprints if group == "mod2"} &
             {name for group, name in xbox.footprints if group == "mode"})
    same = sum(custom_edition.footprints[("mod2", name)] == xbox.footprints[("mode", name)] for name in names)
    return len(names), same


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--blocks", action="store_true",
                        help="also compare the top-level blocks of tags whose footprints differ")
    parser.add_argument("custom_edition", type=Path, help="folder of Custom Edition caches")
    parser.add_argument("xbox", type=Path, help="folder of Xbox caches of the same levels")
    arguments = parser.parse_args(argv)

    levels = sorted(path.name for path in arguments.custom_edition.glob("*.map")
                    if path.name not in RESOURCE_MAP_NAMES and (arguments.xbox / path.name).is_file())
    if not levels:
        print("no level is in both folders", file=sys.stderr)
        return 1
    totals = collections.defaultdict(collections.Counter)
    identical_in = collections.Counter()
    present_in = collections.Counter()
    only_custom_edition = collections.Counter()
    only_xbox = collections.Counter()
    model_totals = [0, 0]
    for level in levels:
        custom_edition = Cache(arguments.custom_edition / level)
        xbox = Cache(arguments.xbox / level)
        for group, counts in compare(custom_edition, xbox, arguments.blocks).items():
            totals[group].update(counts)
            present_in[group] += 1
            if counts["same footprint"] == counts["shared"]:
                identical_in[group] += 1
        ce_groups = {group for group, _ in custom_edition.footprints}
        xbox_groups = {group for group, _ in xbox.footprints}
        only_custom_edition.update(ce_groups - xbox_groups)
        only_xbox.update(xbox_groups - ce_groups)
        pairs, same = model_pairs(custom_edition, xbox)
        model_totals[0] += pairs
        model_totals[1] += same

    print(f"levels compared: {len(levels)} ({', '.join(level[:-4] for level in levels)})")
    identical = sorted(group for group in present_in if identical_in[group] == present_in[group])
    print(f"groups with the same footprint for every shared tag in every level ({len(identical)}): "
          f"{' '.join(identical)}")
    print("groups with differences (footprints equal / tags shared; structures equal / measured):")
    for group in sorted(set(present_in) - set(identical)):
        counts = totals[group]
        blocks = ""
        if arguments.blocks:
            blocks = (f"; blocks differing in count {counts['block count']}, "
                      f"presence {counts['block presence']}, span {counts['block span']}")
        print(f"  {group!r}: footprints {counts['same footprint']}/{counts['shared']}, "
              f"structures {counts['same structure']}/{counts['structures measured']}, "
              f"identical in {identical_in[group]} of {present_in[group]} levels{blocks}")
    print(f"groups only in Custom Edition caches (levels): {dict(sorted(only_custom_edition.items()))}")
    print(f"groups only in Xbox caches (levels): {dict(sorted(only_xbox.items()))}")
    print(f"models named alike, 'mod2' vs 'mode': {model_totals[0]}, with the same footprint: {model_totals[1]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
