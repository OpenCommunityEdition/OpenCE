#!/usr/bin/env python3
"""Pack the hardcore pad, authored respawn meshes, and timer recordings.

The game reads this pack from the binary. It does not need hardcore_maps.
The maps this was built from stay on the machine that produced the pack:

    python3 tools/hardcore_assets.py MAPS_DIR port/linux/game/hardcore_assets_data.c
"""

import struct
import sys
import zlib
from pathlib import Path

TAG_BASE = 0x40440000
MARKER_VERTICES = 512
MARKER_INDICES = 1024
AREA_VERTICES = 12288
AREA_TRIANGLES = 40000
AREA_PARTS = 4
AREA_PLACES = 2
SOUND_CUES = [
    "timerbeep",
    "1_minute", "2_minutes", "3_minutes", "4_minutes", "5_minutes",
    "6_minutes", "7_minutes", "8_minutes", "9_minutes", "10_minutes",
    "11_minutes", "12_minutes", "13_minutes", "14_minutes", "15_minutes",
    "16_minutes", "17_minutes", "18_minutes", "19_minutes", "20_minutes",
    "21_minutes", "22_minutes", "23_minutes", "24_minutes", "25_minutes",
    "26_minutes", "27_minutes", "28_minutes", "29_minutes", "30_minutes",
    "30_seconds_left", "20_seconds",
    "10", "9", "8", "7", "6", "5", "4", "3", "2", "1",
    "rocket", "camo", "overshield",
]
IMA_INDEX = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]
IMA_STEP = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767,
]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def s32(data, offset):
    return struct.unpack_from("<i", data, offset)[0]


def s16(data, offset):
    return struct.unpack_from("<h", data, offset)[0]


def f32(data, offset):
    return struct.unpack_from("<f", data, offset)[0]


def place(address, tag_size):
    if address < TAG_BASE:
        return None
    offset = address - TAG_BASE
    if offset >= tag_size:
        return None
    return offset


def span(offset, size, limit):
    return size > 0 and offset < limit and size <= limit - offset


def name_at(tag, tag_size, address):
    offset = place(address, tag_size)
    if offset is None:
        return None
    end = tag.find(b"\0", offset, min(tag_size, offset + 256))
    if end < 0:
        return None
    return tag[offset:end].decode("latin1")


def find_named(tag, tag_size, name, group=None):
    if tag_size < 0x28:
        return None
    count = u32(tag, 0x0C)
    table = place(u32(tag, 0), tag_size)
    if count == 0 or count > 65535 or table is None or not span(table, count * 0x20, tag_size):
        return None
    for index in range(count):
        instance = table + index * 0x20
        if group and tag[instance:instance + 4] != group:
            continue
        if name_at(tag, tag_size, u32(tag, instance + 0x10)) == name:
            return u32(tag, instance + 0x14)
    return None


def load_cache(path):
    data = path.read_bytes()
    if data[:4] != b"daeh" or s32(data, 4) != 7:
        raise SystemExit(f"{path.name}: not a version 7 cache")
    tag_offset = u32(data, 0x10)
    tag_size = u32(data, 0x14)
    if tag_offset < 0x800 or tag_size < 0x28 or tag_offset + tag_size > len(data):
        raise SystemExit(f"{path.name}: tag data does not fit")
    return data, data[tag_offset:tag_offset + tag_size], tag_size


def pixel_span(width, height, mipmaps):
    if width < 1 or height < 1 or mipmaps < 0:
        return 0
    total = 0
    for level in range(mipmaps + 1):
        mip_width = max(1, width >> level)
        mip_height = max(1, height >> level)
        total += mip_width * mip_height * 4
    return total


def extract_marker(cache, tag, tag_size):
    model = find_named(tag, tag_size, "scenery\\spawn_marker_nhe\\overflow_spawn_marker")
    bitmap = find_named(tag, tag_size, "scenery\\spawn_marker_nhe\\bitmaps\\spawn_marker_nhe")
    model_offset = place(model or 0, tag_size)
    if model is None or bitmap is None or model_offset is None or not span(model_offset, 0xE8, tag_size):
        raise SystemExit("spawn marker tags were not in prisoner.map")
    geometry_count = s32(tag, model_offset + 0xD0)
    geometry_offset = place(u32(tag, model_offset + 0xD4), tag_size)
    if geometry_count < 1 or geometry_count > 8 or geometry_offset is None:
        raise SystemExit("spawn marker geometry is missing")
    best = None
    best_vertices = 0
    for geometry_index in range(geometry_count):
        geometry = geometry_offset + geometry_index * 0x30
        part_count = s32(tag, geometry + 0x24)
        part_offset = place(u32(tag, geometry + 0x28), tag_size)
        if part_count < 1 or part_offset is None or not span(part_offset, 0x84, tag_size):
            continue
        vertices = s32(tag, part_offset + 0x58)
        triangles = s32(tag, part_offset + 0x48)
        if (vertices < 3 or vertices > MARKER_VERTICES or triangles < 1 or
                triangles + 2 > MARKER_INDICES or s16(tag, part_offset + 0x44) != 1 or
                s16(tag, part_offset + 0x54) != 4):
            continue
        if vertices > best_vertices:
            best_vertices = vertices
            best = part_offset
    if best is None:
        raise SystemExit("spawn marker mesh was not usable")
    vertex_count = s32(tag, best + 0x58)
    triangle_count = s32(tag, best + 0x48)
    vertex_file = u32(tag, 0x14)
    index_relative = u32(tag, 0x1C)
    vertex_at = vertex_file + u32(tag, best + 0x64)
    index_at = vertex_file + index_relative + u32(tag, best + 0x4C)
    index_count = triangle_count + 2
    vertices = cache[vertex_at:vertex_at + vertex_count * 0x44]
    indices = cache[index_at:index_at + index_count * 2]
    if len(vertices) != vertex_count * 0x44 or len(indices) != index_count * 2:
        raise SystemExit("spawn marker mesh ran past the cache")
    bitmap_offset = place(bitmap, tag_size)
    bitmap_count = s32(tag, bitmap_offset + 0x60)
    bitmaps_offset = place(u32(tag, bitmap_offset + 0x64), tag_size)
    if bitmap_count < 1 or bitmaps_offset is None or not span(bitmaps_offset, 0x30, tag_size):
        raise SystemExit("spawn marker bitmap is missing")
    bitmap_data = bytearray(tag[bitmaps_offset:bitmaps_offset + 0x30])
    width, height = struct.unpack_from("<hh", bitmap_data, 0x04)  # checked below from known layout
    # struct bitmap_data: signature, width, height, depth, type, format, flags, ...
    signature, width, height, depth, bitmap_type, bitmap_format, flags = struct.unpack_from("<IhhhhhH", bitmap_data, 0)
    mipmaps = struct.unpack_from("<h", bitmap_data, 0x14)[0]
    pixels_offset = struct.unpack_from("<i", bitmap_data, 0x18)[0]
    pixels_size = struct.unpack_from("<i", bitmap_data, 0x1C)[0]
    expected = pixel_span(width, height, mipmaps)
    if (signature != 0x6269746D or bitmap_type != 0 or bitmap_format != 10 or depth != 1 or
            width < 1 or height < 1 or (width & (width - 1)) or (height & (height - 1)) or
            expected == 0 or pixels_size != expected or pixels_offset < 0):
        raise SystemExit(
            f"spawn marker bitmap is not the expected 32-bit texture "
            f"({signature:#x} {width}x{height} format {bitmap_format})")
    pixels = cache[pixels_offset:pixels_offset + expected]
    if len(pixels) != expected:
        raise SystemExit("spawn marker pixels ran past the cache")
    payload = struct.pack("<III", vertex_count, index_count, expected) + bytes(bitmap_data) + vertices + indices + pixels
    return payload, vertex_count, triangle_count, width, height, expected


def has_randoms(text):
    return text is not None and "randoms" in text


def find_group(tag, tag_size, group, name=None):
    if tag_size < 0x28:
        return None
    count = u32(tag, 0x0C)
    table = place(u32(tag, 0), tag_size)
    if count == 0 or count > 65535 or table is None or not span(table, count * 0x20, tag_size):
        return None
    for index in range(count):
        instance = table + index * 0x20
        if tag[instance:instance + 4] != group:
            continue
        if name is not None and name_at(tag, tag_size, u32(tag, instance + 0x10)) != name:
            continue
        return u32(tag, instance + 0x14)
    return None


def extract_area_map(cache, tag, tag_size):
    scenario = find_group(tag, tag_size, b"rncs")
    scenario_offset = place(scenario or 0, tag_size)
    if scenario is None or scenario_offset is None or not span(scenario_offset, 0x228, tag_size):
        return None
    scenery_count = s32(tag, scenario_offset + 0x210)
    palette_count = s32(tag, scenario_offset + 0x21C)
    scenery_offset = place(u32(tag, scenario_offset + 0x214), tag_size)
    palette_offset = place(u32(tag, scenario_offset + 0x220), tag_size)
    if (scenery_count < 1 or scenery_count > 8192 or palette_count < 1 or palette_count > 512 or
            scenery_offset is None or palette_offset is None or
            not span(scenery_offset, scenery_count * 72, tag_size) or
            not span(palette_offset, palette_count * 48, tag_size)):
        return None
    places = []
    model_name = None
    palette_index = None
    for index in range(scenery_count):
        datum = scenery_offset + index * 72
        palette = s16(tag, datum)
        if palette < 0 or palette >= palette_count:
            continue
        path = name_at(tag, tag_size, u32(tag, palette_offset + palette * 48 + 4))
        if not has_randoms(path):
            continue
        if palette_index is None:
            palette_index = palette
            scenery = find_group(tag, tag_size, b"necs", path)
            scenery_placed = place(scenery or 0, tag_size)
            if scenery is None or scenery_placed is None or not span(scenery_placed, 0x40, tag_size):
                return None
            model_name = name_at(tag, tag_size, u32(tag, scenery_placed + 0x2C))
            if not model_name:
                return None
        if palette != palette_index or len(places) >= AREA_PLACES:
            continue
        places.append((
            f32(tag, datum + 8), f32(tag, datum + 12), f32(tag, datum + 16),
            f32(tag, datum + 0x14), f32(tag, datum + 0x18), f32(tag, datum + 0x1C),
        ))
    if not places or not model_name:
        return None
    model = find_group(tag, tag_size, b"2dom", model_name)
    model_offset = place(model or 0, tag_size)
    if model is None or model_offset is None or not span(model_offset, 0xE8, tag_size):
        return None
    node = (0.0, 0.0, 0.0)
    node_count = s32(tag, model_offset + 0xB8)
    node_offset = place(u32(tag, model_offset + 0xBC), tag_size)
    if node_count >= 1 and node_offset is not None and span(node_offset, 52, tag_size):
        node = (f32(tag, node_offset + 40), f32(tag, node_offset + 44), f32(tag, node_offset + 48))
    vertex_file = u32(tag, 0x14)
    index_relative = u32(tag, 0x1C)
    geometry_count = s32(tag, model_offset + 0xD0)
    part_offset = place(u32(tag, model_offset + 0xD4), tag_size)
    if (geometry_count < 1 or geometry_count > 8 or part_offset is None or
            not span(part_offset, geometry_count * 0x30, tag_size)):
        return None
    best_vertices = 0
    best_count = 0
    best_address = 0
    for geometry_index in range(geometry_count):
        geometry = part_offset + geometry_index * 0x30
        count = s32(tag, geometry + 0x24)
        address = u32(tag, geometry + 0x28)
        offset = place(address, tag_size)
        if count < 1 or count > 32 or offset is None or not span(offset, count * 0x84, tag_size):
            continue
        total = 0
        for candidate in range(count):
            part = offset + candidate * 0x84
            verts = s32(tag, part + 0x58)
            tris = s32(tag, part + 0x48)
            if (3 <= verts <= AREA_VERTICES and 1 <= tris <= AREA_TRIANGLES and
                    s16(tag, part + 0x44) == 1 and s16(tag, part + 0x54) == 4):
                total += verts
        if total > best_vertices:
            best_vertices = total
            best_count = count
            best_address = address
    if best_vertices < 3:
        return None
    parts_at = place(best_address, tag_size)
    parts = []
    for part_index in range(best_count):
        if len(parts) >= AREA_PARTS:
            break
        part = parts_at + part_index * 0x84
        verts = s32(tag, part + 0x58)
        tris = s32(tag, part + 0x48)
        if not (3 <= verts <= AREA_VERTICES and 1 <= tris <= AREA_TRIANGLES and
                s16(tag, part + 0x44) == 1 and s16(tag, part + 0x54) == 4):
            continue
        vertex_at = vertex_file + u32(tag, part + 0x64)
        index_at = vertex_file + index_relative + u32(tag, part + 0x4C)
        index_count = tris + 2
        vertices = cache[vertex_at:vertex_at + verts * 0x44]
        indices = cache[index_at:index_at + index_count * 2]
        if len(vertices) != verts * 0x44 or len(indices) != index_count * 2:
            return None
        parts.append((verts, index_count, vertices, indices))
    if not parts:
        return None
    payload = struct.pack("<I", len(places))
    for place_values in places:
        payload += struct.pack("<6f", *place_values)
    payload += struct.pack("<3fI", node[0], node[1], node[2], len(parts))
    triangles = 0
    for verts, index_count, vertices, indices in parts:
        payload += struct.pack("<II", verts, index_count) + vertices + indices
        triangles += index_count - 2
    return payload, len(places), len(parts), triangles, model_name


def ima_expand(nibble, predictor, index):
    step = IMA_STEP[index]
    difference = step >> 3
    if nibble & 1:
        difference += step >> 2
    if nibble & 2:
        difference += step >> 1
    if nibble & 4:
        difference += step
    if nibble & 8:
        difference = -difference
    predictor += difference
    predictor = max(-32768, min(32767, predictor))
    index += IMA_INDEX[nibble]
    index = max(0, min(88, index))
    return predictor, index


def decode_adpcm(source, channels):
    block_bytes = 36 * channels
    blocks = len(source) // block_bytes
    samples = [0] * (blocks * 64 * channels)
    for block in range(blocks):
        data = source[block * block_bytes:(block + 1) * block_bytes]
        output = block * 64 * channels
        for channel in range(channels):
            header = data[channel * 4:(channel + 1) * 4]
            predictor = struct.unpack_from("<h", header, 0)[0]
            index = header[2] if header[2] <= 88 else 88
            samples[output + channel] = predictor & 0xFFFF
            for group in range(8):
                nibbles = data[4 * channels + (group * channels + channel) * 4:
                               4 * channels + (group * channels + channel) * 4 + 4]
                for byte in range(4):
                    sample = group * 8 + byte * 2 + 1
                    predictor, index = ima_expand(nibbles[byte] & 0xF, predictor, index)
                    samples[output + sample * channels + channel] = predictor & 0xFFFF
                    if sample + 1 < 64:
                        predictor, index = ima_expand(nibbles[byte] >> 4, predictor, index)
                        samples[output + (sample + 1) * channels + channel] = predictor & 0xFFFF
    pcm = struct.pack("<" + "H" * len(samples), *samples)
    return pcm


def wav_bytes(pcm, channels, rate):
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF", len(pcm) + 36, b"WAVE", b"fmt ", 16, 1, channels, rate,
        rate * channels * 2, channels * 2, 16, b"data", len(pcm))
    return header + pcm


def extract_sounds(cache, tag, tag_size):
    clips = []
    for cue in SOUND_CUES:
        if cue == "timerbeep":
            sound_name = "sound\\timer\\beeps\\timerbeep"
        else:
            sound_name = "sound\\timer\\cortana\\" + cue
        address = find_named(tag, tag_size, sound_name)
        if address is None:
            raise SystemExit(f"missing timer recording {sound_name}")
        sound_offset = place(address, tag_size)
        if sound_offset is None or not span(sound_offset, 0xA4, tag_size):
            raise SystemExit(f"{cue}: sound tag is short")
        rate_enum = s16(tag, sound_offset + 0x06)
        encoding = s16(tag, sound_offset + 0x6C)
        pitch_count = s32(tag, sound_offset + 0x98)
        pitch_offset = place(u32(tag, sound_offset + 0x9C), tag_size)
        if (pitch_count != 1 or rate_enum not in (0, 1) or encoding not in (0, 1) or
                s16(tag, sound_offset + 0x6E) != 1 or pitch_offset is None or
                not span(pitch_offset, 0x48, tag_size)):
            raise SystemExit(f"{cue}: unexpected sound layout")
        permutation_count = s32(tag, pitch_offset + 0x3C)
        permutation_offset = place(u32(tag, pitch_offset + 0x40), tag_size)
        if (permutation_count != 1 or permutation_offset is None or
                not span(permutation_offset, 0x7C, tag_size) or
                s16(tag, permutation_offset + 0x28) != 1):
            raise SystemExit(f"{cue}: unexpected permutation")
        sample_bytes = u32(tag, permutation_offset + 0x40)
        sample_flags = u32(tag, permutation_offset + 0x44)
        sample_file = u32(tag, permutation_offset + 0x48)
        channels = encoding + 1
        rate = 44100 if rate_enum else 22050
        if sample_flags & 1 or sample_bytes == 0 or sample_bytes % (36 * channels) or sample_file + sample_bytes > len(cache):
            raise SystemExit(f"{cue}: samples do not fit")
        pcm = decode_adpcm(cache[sample_file:sample_file + sample_bytes], channels)
        if len(pcm) > rate * channels * 2 * 4:
            raise SystemExit(f"{cue}: longer than the four seconds the player accepts")
        clips.append((cue, wav_bytes(pcm, channels, rate), rate, channels, len(pcm) // (channels * 2)))
    return clips


def entry(kind, name, payload):
    raw = name.encode("ascii")
    if len(raw) > 31:
        raise SystemExit(f"name too long: {name}")
    return struct.pack("<I32sI", kind, raw, len(payload)) + payload


def words(data):
    padded = data + b"\0" * ((-len(data)) % 4)
    values = struct.unpack(f"<{len(padded) // 4}I", padded)
    lines = []
    for start in range(0, len(values), 8):
        chunk = values[start:start + 8]
        lines.append("\t" + ", ".join(f"0x{word:08x}" for word in chunk) + ",")
    return lines


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: hardcore_assets.py MAPS_DIR OUTPUT.c")
    maps = Path(sys.argv[1])
    output = Path(sys.argv[2])
    prisoner = maps / "prisoner.map"
    cache, tag, tag_size = load_cache(prisoner)
    marker, vertices, triangles, width, height, pixels = extract_marker(cache, tag, tag_size)
    sounds = extract_sounds(cache, tag, tag_size)
    sections = [entry(1, "marker", marker)]
    print(f"marker {vertices} verts {triangles} tris {width}x{height} {pixels} px")
    areas = 0
    for path in sorted(maps.glob("*.map")):
        area_cache, area_tag, area_tag_size = load_cache(path)
        extracted = extract_area_map(area_cache, area_tag, area_tag_size)
        if extracted is None:
            continue
        payload, place_count, part_count, triangle_count, model_name = extracted
        leaf = path.stem
        sections.append(entry(2, leaf, payload))
        areas += 1
        print(f"area {leaf}: {place_count} places {part_count} parts {triangle_count} tris ({model_name})")
    for cue, wav, rate, channels, frames in sounds:
        sections.append(entry(3, cue, wav))
        print(f"sound {cue}: {frames} frames {rate} Hz {channels} ch")
    pack = struct.pack("<III", 0x4B505645, 1, len(sections)) + b"".join(sections)
    compressed = struct.pack(">I", len(pack)) + zlib.compress(pack, 9)
    print(f"pack {len(pack)} bytes, compressed {len(compressed)} bytes, {areas} areas, {len(sounds)} sounds")
    lines = [
        "/* generated by tools/hardcore_assets.py. do not edit. */",
        "/* the pad, the authored respawn meshes, and the timer recordings. */",
        "",
        f"const unsigned long hardcore_assets_compressed_bytes = {len(compressed)};",
        "const unsigned int hardcore_assets_words[] = {",
        *words(compressed),
        "};",
        "",
    ]
    output.write_text("\n".join(lines))
    print(f"wrote {output}")


if __name__ == "__main__":
    main()
