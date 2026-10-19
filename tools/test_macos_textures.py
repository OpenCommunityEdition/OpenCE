"""The macOS device's textures and buffers, as the app's Metal renderer holds
them: Xbox textures of every format the game's device takes, swizzled,
linear, compressed, cube maps and volumes, uploaded by the guest's
xbox_textures.c and d3d8_metal.c (built with HALO_METAL) and read back by
the app (port/macos/tests/vm_textures.c), compared with what this file
decodes of the same texels itself; and the contiguous window's mirror, the
stream buffer and render targets.

The texels are xorshift32's: the guest and this file make the same ones.
Compressed textures the renderer takes as BC1 to BC3 are decoded by the GPU,
whose interpolated colours may differ from the reference's rounding by a
little; every other texture must match exactly.

Needs `ninja macos macos_app` in `nix develop`; no window is shown.
"""

import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools import macos_guest_image  # noqa: E402

APP = ROOT / "build/macos/OpenCE.app/Contents/MacOS/OpenCE"

pytestmark = [
    pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only"),
    pytest.mark.skipif(not macos_guest_image.available() or shutil.which("ninja") is None,
                       reason="needs `ninja macos` in nix develop"),
    pytest.mark.skipif(not APP.is_file(), reason="needs `ninja macos_app`"),
]

# the GPU's BC decoding against the reference's, a channel
BC_TOLERANCE = 3


@pytest.fixture(scope="module")
def vm_textures(tmp_path_factory):
    work = tmp_path_factory.mktemp("vm_textures")
    return macos_guest_image.build_guest_image(
        [ROOT / "port/macos/tests/vm_textures.c", ROOT / "port/macos/tests/vm_textures_stubs.c",
         ROOT / "port/macos/tests/vm_nv2a_stubs.c", ROOT / "port/linux/src/xbox_textures.c",
         ROOT / "port/macos/guest/d3d8_metal.c", ROOT / "port/macos/guest/opence_render_writer.c"],
        work, "vm_textures", ["-DHALO_METAL", f"-I{ROOT / 'port/macos/include'}"],
        like="port/linux/src/xgpu_text.c")


def run(image, tmp_path, *arguments):
    """the guest's output, run in the app's place off the screen"""
    (tmp_path / "home").mkdir(exist_ok=True)
    environment = dict(os.environ, HOME=str(tmp_path / "home"), HALO_DATA_ROOT=str(tmp_path),
                       HALO_HOST_GUEST_IMAGE=str(image), HALO_HOST_CAPTURE=str(tmp_path / "frame.png"),
                       HALO_HOST_CAPTURE_FRAME="1", HALO_HOST_CAPTURE_SIZE="64x64")
    result = subprocess.run([str(APP), *arguments], cwd=tmp_path, env=environment, capture_output=True, text=True,
                            timeout=300)
    assert result.returncode == 0 and result.stdout.strip().endswith("ok"), \
        result.stdout[-3000:] + result.stderr[-3000:]
    assert "log:" not in result.stdout, result.stdout
    return result.stdout


def readbacks(output):
    """each line "<name> <w>x<h> <hex>" as {name: (w, h, bytes)}"""
    found = {}
    for line in output.splitlines():
        if line.startswith(("texture ", "mirror", "stream")) and " " in line and line.split()[-1] != "ok":
            head, size, data = line.rsplit(" ", 2)
            if "x" not in size:
                continue
            width, height = map(int, size.split("x"))
            found[head] = (width, height, bytes.fromhex(data))
    return found


# ---------- the texels

def xorshift_bytes(size, seed):
    state = seed or 1
    out = bytearray()
    while len(out) < size:
        state ^= (state << 13) & 0xFFFFFFFF
        state ^= state >> 17
        state ^= (state << 5) & 0xFFFFFFFF
        out += state.to_bytes(4, "little")
    return bytes(out[:size])


# format: (kind, bytes a texel or block, linear)
FORMATS = {
    0x00: ("l8", 1, False), 0x01: ("al8", 1, False), 0x02: ("a1r5g5b5", 2, False),
    0x03: ("x1r5g5b5", 2, False), 0x04: ("a4r4g4b4", 2, False), 0x05: ("r5g6b5", 2, False),
    0x06: ("a8r8g8b8", 4, False), 0x07: ("x8r8g8b8", 4, False), 0x0B: ("p8", 1, False),
    0x0C: ("dxt1", 8, False), 0x0E: ("dxt3", 16, False), 0x0F: ("dxt5", 16, False),
    0x10: ("a1r5g5b5", 2, True), 0x11: ("r5g6b5", 2, True), 0x12: ("a8r8g8b8", 4, True),
    0x13: ("l8", 1, True), 0x16: ("r8b8", 2, True), 0x17: ("g8b8", 2, True), 0x19: ("a8", 1, False),
    0x1A: ("a8l8", 2, False), 0x1B: ("al8", 1, True), 0x1C: ("x1r5g5b5", 2, True),
    0x1D: ("a4r4g4b4", 2, True), 0x1E: ("x8r8g8b8", 4, True), 0x1F: ("a8", 1, True),
    0x20: ("a8l8", 2, True), 0x24: ("yuy2", 2, True), 0x25: ("uyvy", 2, True),
    0x27: ("r6g5b5", 2, False), 0x28: ("g8b8", 2, False), 0x29: ("r8b8", 2, False),
    0x2A: ("d24s8", 4, False), 0x2C: ("d16", 2, False), 0x32: ("l16", 2, False),
    0x33: ("v16u16", 4, False), 0x38: ("r5g5b5a1", 2, False), 0x39: ("r4g4b4a4", 2, False),
    0x3A: ("a8b8g8r8", 4, False), 0x3B: ("b8g8r8a8", 4, False), 0x3C: ("r8g8b8a8", 4, False),
}
COMPRESSED = ("dxt1", "dxt3", "dxt5")
CUBE_FACE_ALIGNMENT = 128
# Halo PC's channel orders: the channel (r, g, b, a) each is sampled from
CHANNEL_SOURCES = [(0, 1, 2, 3), (2, 1, 3, 0), (3, 3, 3, 0)]


def swizzled(code, width_log, height_log, levels=1, depth_log=0, cube=False):
    """a swizzled texture's format word"""
    return ((code << 8) | ((3 if depth_log else 2) << 4) | (4 if cube else 0) | (levels << 16) |
            (width_log << 20) | (height_log << 24) | (depth_log << 28))


def linear(code, width, height, pitch):
    """a linear texture's format and size words"""
    return (code << 8) | (2 << 4), (width - 1) | ((height - 1) << 12) | ((pitch // 64 - 1) << 24)


class Texture:
    def __init__(self, format_word, size_word):
        self.code = (format_word >> 8) & 0xFF
        self.kind, self.bytes, self.linear = FORMATS[self.code]
        self.cube = bool(format_word & 4)
        if size_word:
            self.width = (size_word & 0xFFF) + 1
            self.height = ((size_word >> 12) & 0xFFF) + 1
            self.depth, self.levels = 1, 1
            self.pitch = (((size_word >> 24) & 0xFF) + 1) * 64
            self.linear = True
        else:
            self.width = 1 << ((format_word >> 20) & 0xF)
            self.height = 1 << ((format_word >> 24) & 0xF)
            self.depth = 1 << ((format_word >> 28) & 0xF)
            self.levels = ((format_word >> 16) & 0xF) or 1
            self.pitch = self.width * self.bytes
        if (format_word >> 4) & 0xF != 3:
            self.depth = 1

    @property
    def compressed(self):
        return self.kind in COMPRESSED

    def side(self, base, level):
        return max(base >> level, 1)

    def level_size(self, level):
        width, height, depth = (self.side(self.width, level), self.side(self.height, level),
                                self.side(self.depth, level))
        if self.compressed:
            return ((width + 3) // 4) * ((height + 3) // 4) * self.bytes * depth
        if self.linear:
            return self.pitch * height
        return width * height * depth * self.bytes

    def level_offset(self, level):
        return sum(self.level_size(index) for index in range(level))

    def face_size(self):
        size = self.level_offset(self.levels)
        if self.cube:
            size = (size + CUBE_FACE_ALIGNMENT - 1) & ~(CUBE_FACE_ALIGNMENT - 1)
        return size

    def size(self):
        return self.face_size() * (6 if self.cube else 1)


# ---------- decoding, independently of xbox_textures.c

def e5(value):
    return (value << 3) | (value >> 2)


def e6(value):
    return (value << 2) | (value >> 4)


def e4(value):
    return value * 0x11


def clamp(value):
    return max(0, min(255, value))


def yuv(y, u, v):
    c, d, e = y - 16, u - 128, v - 128
    return (clamp((298 * c + 409 * e + 128) >> 8), clamp((298 * c - 100 * d - 208 * e + 128) >> 8),
            clamp((298 * c + 516 * d + 128) >> 8), 255)


def palette_colour(palette, index):
    if palette is None:
        return (index, index, index, 255)
    word = int.from_bytes(palette[index * 4:index * 4 + 4], "little")
    return ((word >> 16) & 0xFF, (word >> 8) & 0xFF, word & 0xFF, word >> 24)


def texel(kind, data, at, palette, row, x):
    """one texel's (r, g, b, a)"""
    b = data[at:at + 4].ljust(4, b"\0")
    w16 = b[0] | (b[1] << 8)
    if kind == "a8r8g8b8":
        return (b[2], b[1], b[0], b[3])
    if kind == "x8r8g8b8":
        return (b[2], b[1], b[0], 255)
    if kind == "r5g6b5":
        return (e5(w16 >> 11), e6((w16 >> 5) & 0x3F), e5(w16 & 0x1F), 255)
    if kind == "a1r5g5b5":
        return (e5((w16 >> 10) & 0x1F), e5((w16 >> 5) & 0x1F), e5(w16 & 0x1F), 255 if w16 & 0x8000 else 0)
    if kind == "x1r5g5b5":
        return (e5((w16 >> 10) & 0x1F), e5((w16 >> 5) & 0x1F), e5(w16 & 0x1F), 255)
    if kind == "a4r4g4b4":
        return (e4((w16 >> 8) & 0xF), e4((w16 >> 4) & 0xF), e4(w16 & 0xF), e4(w16 >> 12))
    if kind == "l8":
        return (b[0], b[0], b[0], 255)
    if kind == "al8":
        return (b[0], b[0], b[0], b[0])
    if kind == "a8":
        return (255, 255, 255, b[0])
    if kind == "a8l8":
        return (b[0], b[0], b[0], b[1])
    if kind == "p8":
        return palette_colour(palette, b[0])
    if kind == "g8b8":
        return (b[0], b[1], 0, 255)
    if kind == "r8b8":
        return (b[1], 0, b[0], 255)
    if kind == "r6g5b5":
        return (e6(w16 >> 10), e5((w16 >> 5) & 0x1F), e5(w16 & 0x1F), 255)
    if kind == "l16":
        return (b[1], b[1], b[1], 255)
    if kind == "v16u16":
        return (b[1], b[3], 0, 255)
    if kind == "a8b8g8r8":
        return (b[0], b[1], b[2], b[3])
    if kind == "b8g8r8a8":
        return (b[1], b[2], b[3], b[0])
    if kind == "r8g8b8a8":
        return (b[3], b[2], b[1], b[0])
    if kind == "r5g5b5a1":
        return (e5(w16 >> 11), e5((w16 >> 6) & 0x1F), e5((w16 >> 1) & 0x1F), 255 if w16 & 1 else 0)
    if kind == "r4g4b4a4":
        return (e4(w16 >> 12), e4((w16 >> 8) & 0xF), e4((w16 >> 4) & 0xF), e4(w16 & 0xF))
    if kind == "d24s8":
        return (b[3], b[3], b[3], 255)
    if kind == "d16":
        return (b[1], b[1], b[1], 255)
    if kind in ("yuy2", "uyvy"):
        pair = row + (x & ~1) * 2
        p = data[pair:pair + 4]
        if kind == "yuy2":
            return yuv(p[2 if x & 1 else 0], p[1], p[3])
        return yuv(p[3 if x & 1 else 1], p[0], p[2])
    raise AssertionError(kind)


def morton_masks(width, height, depth):
    masks, bit, mask_bit = [0, 0, 0], 1, 1
    while True:
        done = True
        for axis, side in enumerate((width, height, depth)):
            if bit < side:
                masks[axis] |= mask_bit
                mask_bit <<= 1
                done = False
        bit <<= 1
        if done:
            return masks


def spread(mask, value):
    result, bit = 0, 1
    while value and bit <= mask:
        if mask & bit:
            if value & 1:
                result |= bit
            value >>= 1
        bit <<= 1
    return result


def colour565(value):
    return (e5(value >> 11), e6((value >> 5) & 0x3F), e5(value & 0x1F))


def mix(a, b, weight_a, weight_b, divisor):
    return tuple((x * weight_a + y * weight_b) // divisor for x, y in zip(a, b))


def dxt_colours(block, dxt1):
    c0, c1 = block[0] | (block[1] << 8), block[2] | (block[3] << 8)
    p0, p1 = colour565(c0), colour565(c1)
    if c0 > c1 or not dxt1:
        palette = [p0 + (255,), p1 + (255,), mix(p0, p1, 2, 1, 3) + (255,), mix(p0, p1, 1, 2, 3) + (255,)]
    else:
        palette = [p0 + (255,), p1 + (255,), mix(p0, p1, 1, 1, 2) + (255,), (0, 0, 0, 0)]
    bits = int.from_bytes(block[4:8], "little")
    return [palette[(bits >> (index * 2)) & 3] for index in range(16)]


def dxt_block(kind, block):
    """a 4x4 block's 16 (r, g, b, a), row by row"""
    if kind == "dxt1":
        return dxt_colours(block, True)
    colours = dxt_colours(block[8:], False)
    if kind == "dxt3":
        alpha = [e4((block[index // 2] >> ((index & 1) * 4)) & 0xF) for index in range(16)]
    else:
        a0, a1 = block[0], block[1]
        values = [a0, a1]
        if a0 > a1:
            values += [((8 - index) * a0 + (index - 1) * a1) // 7 for index in range(2, 8)]
        else:
            values += [((6 - index) * a0 + (index - 1) * a1) // 5 for index in range(2, 6)] + [0, 255]
        bits = int.from_bytes(block[2:8], "little")
        alpha = [values[(bits >> (index * 3)) & 7] for index in range(16)]
    return [colour[:3] + (a,) for colour, a in zip(colours, alpha)]


def decode_level(texture, data, base, level, palette):
    """the level's slices, each a list of rows of (r, g, b, a)"""
    width, height, depth = (texture.side(texture.width, level), texture.side(texture.height, level),
                            texture.side(texture.depth, level))
    source = base + texture.level_offset(level)
    slices = []
    if texture.compressed:
        blocks_x, blocks_y = (width + 3) // 4, (height + 3) // 4
        for z in range(depth):
            rows = [[None] * width for _ in range(height)]
            for by in range(blocks_y):
                for bx in range(blocks_x):
                    at = source + ((z * blocks_y + by) * blocks_x + bx) * texture.bytes
                    colours = dxt_block(texture.kind, data[at:at + texture.bytes])
                    for y in range(4):
                        for x in range(4):
                            if bx * 4 + x < width and by * 4 + y < height:
                                rows[by * 4 + y][bx * 4 + x] = colours[y * 4 + x]
            slices.append(rows)
    elif texture.linear:
        row_texels = texture.pitch // texture.bytes
        if texture.kind in ("yuy2", "uyvy"):
            row_texels &= ~1
        rows = []
        for y in range(height):
            row = source + y * texture.pitch
            rows.append([texel(texture.kind, data, row + x * texture.bytes, palette, row, x)
                         if x < row_texels else (0, 0, 0, 0) for x in range(width)])
        slices.append(rows)
    else:
        mask_x, mask_y, mask_z = morton_masks(width, height, depth)
        for z in range(depth):
            rows = []
            for y in range(height):
                rows.append([])
                for x in range(width):
                    at = source + (spread(mask_x, x) | spread(mask_y, y) | spread(mask_z, z)) * texture.bytes
                    rows[-1].append(texel(texture.kind, data, at, palette, at, x))
            slices.append(rows)
    return slices


def expected(texture, data, palette, order):
    """{(face, level, slice): bytes} of what the texture holds"""
    result = {}
    for face in range(6 if texture.cube else 1):
        for level in range(texture.levels):
            for index, rows in enumerate(decode_level(texture, data, face * texture.face_size(), level, palette)):
                pixels = bytearray()
                for row in rows:
                    for colour in row:
                        pixels += bytes(colour[source] for source in CHANNEL_SOURCES[order])
                result[(face, level, index)] = bytes(pixels)
    return result


def gpu_decoded(texture, order):
    """whether the renderer takes the texels as BC1 to BC3 (xbox_textures.c,
    metal_upload)"""
    return (texture.compressed and texture.depth == 1 and texture.width % 4 == 0 and texture.height % 4 == 0
            and order == 0)


# ---------- the cases

LINEAR_A8R8G8B8 = linear(0x12, 20, 6, 128)
# (a pitch narrower than the width: the rest of each row is black)
NARROW_A8R8G8B8 = linear(0x12, 20, 6, 64)
LINEAR_R5G6B5 = linear(0x11, 10, 3, 64)
LINEAR_YUY2 = linear(0x24, 8, 4, 64)
LINEAR_UYVY = linear(0x25, 6, 2, 64)

# (format word, size word, seed, palette seed, channel order, rewritten)
CASES = [
    (swizzled(0x06, 4, 3, levels=3), 0, 0x1234, 0, 0, 0),
    (swizzled(0x07, 3, 3), 0, 0x2345, 0, 0, 0),
    (swizzled(0x05, 3, 3, levels=2), 0, 0x3456, 0, 0, 0),
    (swizzled(0x02, 3, 2), 0, 0x4567, 0, 0, 0),
    (swizzled(0x03, 2, 3), 0, 0x5678, 0, 0, 0),
    (swizzled(0x04, 2, 4, levels=2), 0, 0x6789, 0, 0, 0),
    (swizzled(0x00, 3, 3), 0, 0x789A, 0, 0, 0),
    (swizzled(0x01, 3, 3), 0, 0x89AB, 0, 0, 0),
    (swizzled(0x19, 3, 2), 0, 0x9ABC, 0, 0, 0),
    (swizzled(0x1A, 2, 2), 0, 0xABCD, 0, 0, 0),
    (swizzled(0x0B, 3, 3), 0, 0xBCDE, 0x5150, 0, 0),
    (swizzled(0x0B, 2, 2), 0, 0xCDEF, 0, 0, 0),
    (swizzled(0x27, 2, 2), 0, 0x1111, 0, 0, 0),
    (swizzled(0x28, 2, 2), 0, 0x2222, 0, 0, 0),
    (swizzled(0x29, 2, 2), 0, 0x3333, 0, 0, 0),
    (swizzled(0x32, 2, 2), 0, 0x4444, 0, 0, 0),
    (swizzled(0x33, 2, 2), 0, 0x5555, 0, 0, 0),
    (swizzled(0x2A, 2, 2), 0, 0x6666, 0, 0, 0),
    (swizzled(0x2C, 2, 2), 0, 0x7777, 0, 0, 0),
    (swizzled(0x38, 2, 2), 0, 0x8888, 0, 0, 0),
    (swizzled(0x39, 2, 2), 0, 0x9999, 0, 0, 0),
    (swizzled(0x3A, 2, 2), 0, 0xAAAA, 0, 0, 0),
    (swizzled(0x3B, 2, 2), 0, 0xBBBB, 0, 0, 0),
    (swizzled(0x3C, 2, 2), 0, 0xCCCC, 0, 0, 0),
    # compressed: BC1 to BC3 down to 1x1 levels, and decoded where they cannot be
    (swizzled(0x0C, 3, 3, levels=4), 0, 0xD001, 0, 0, 0),
    (swizzled(0x0E, 4, 3, levels=2), 0, 0xD002, 0, 0, 0),
    (swizzled(0x0F, 3, 3, levels=2), 0, 0xD003, 0, 0, 0),
    (swizzled(0x0C, 3, 3), 0, 0xD004, 0, 1, 0),
    (swizzled(0x0F, 3, 3), 0, 0xD005, 0, 2, 0),
    (swizzled(0x0C, 1, 3), 0, 0xD006, 0, 0, 0),
    (swizzled(0x06, 2, 2), 0, 0xD007, 0, 1, 0),
    # linear
    (*LINEAR_A8R8G8B8, 0xE001, 0, 0, 0),
    (*NARROW_A8R8G8B8, 0xE002, 0, 0, 0),
    (*LINEAR_R5G6B5, 0xE003, 0, 0, 0),
    (*LINEAR_YUY2, 0xE004, 0, 0, 0),
    (*LINEAR_UYVY, 0xE005, 0, 0, 0),
    # cube maps and volumes
    (swizzled(0x06, 3, 3, levels=2, cube=True), 0, 0xF001, 0, 0, 0),
    (swizzled(0x0C, 3, 3, levels=2, cube=True), 0, 0xF002, 0, 0, 0),
    (swizzled(0x07, 2, 2, levels=2, depth_log=2), 0, 0xF003, 0, 0, 0),
    (swizzled(0x0F, 2, 2, depth_log=1), 0, 0xF004, 0, 0, 0),
    # written again once drawn
    (swizzled(0x06, 3, 3), 0, 0xA001, 0, 0, 1),
    (swizzled(0x0C, 3, 3), 0, 0xA002, 0, 0, 1),
]


def compare(name, got, want, tolerance):
    width, height, pixels = got
    assert len(pixels) == len(want), f"{name}: {len(pixels)} bytes, {len(want)} expected"
    for index, (a, b) in enumerate(zip(pixels, want)):
        if abs(a - b) > tolerance:
            pixel = index // 4
            raise AssertionError(f"{name}: pixel ({pixel % width}, {pixel // width}) is "
                                 f"{pixels[pixel * 4:pixel * 4 + 4].hex()}, expected {want[pixel * 4:pixel * 4 + 4].hex()}")


def test_every_format_reads_back_as_decoded(vm_textures, tmp_path):
    arguments = [":".join("%x" % field for field in case) for case in CASES]
    found = readbacks(run(vm_textures, tmp_path, "textures", *arguments))
    checked = 0
    for number, (format_word, size_word, seed, palette_seed, order, rewrite) in enumerate(CASES):
        texture = Texture(format_word, size_word)
        palette = xorshift_bytes(1024, palette_seed) if palette_seed else None
        tolerance = BC_TOLERANCE if gpu_decoded(texture, order) else 0
        passes = [("first", seed), ("again", seed)] + ([("rewritten", seed + 1)] if rewrite else [])
        for name, pass_seed in passes:
            want = expected(texture, xorshift_bytes(texture.size(), pass_seed), palette, order)
            for (face, level, index), pixels in want.items():
                key = f"texture {number} {name} face {face} level {level} slice {index}"
                assert key in found, f"{key} was not read back (case {arguments[number]})"
                compare(f"{key} ({texture.kind} {texture.width}x{texture.height}x{texture.depth})", found[key],
                        pixels, tolerance)
                checked += 1
    assert checked == sum(1 for key in found if key.startswith("texture "))


def test_the_mirror_and_the_stream_hold_the_words_given(vm_textures, tmp_path):
    output = run(vm_textures, tmp_path, "buffers")
    found = readbacks(output)
    words = [((0x01020304 * (index + 1)) ^ 0x80706050) & 0xFFFFFFFF for index in range(16)]
    assert found["mirror"][2] == b"".join(word.to_bytes(4, "little") for word in words)
    words[0] = 0xA0B0C0D1
    assert found["mirror rewritten"][2] == b"".join(word.to_bytes(4, "little") for word in words)
    stream = [0x11223344 + index * 0x01010101 for index in range(16)]
    assert found["stream"][2] == b"".join(word.to_bytes(4, "little") for word in stream)
    # three rewrites in consecutive frames are uploaded again, the fourth
    # makes the page volatile: drawn through the stream instead
    assert [line for line in output.splitlines() if line.startswith("rewrite")] == [
        "rewrite 1 mirrored", "rewrite 2 mirrored", "rewrite 3 mirrored", "rewrite 4 refused", "rewrite 5 refused"]


def test_render_targets_are_found_again(vm_textures, tmp_path):
    output = run(vm_textures, tmp_path, "targets")
    assert any(line.startswith("targets ") for line in output.splitlines())
