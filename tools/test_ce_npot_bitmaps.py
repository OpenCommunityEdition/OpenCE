"""Real CE admission and Xbox descriptor code, with no retail assets or GPU.

Compile for the game's 32-bit ABI. Only tag iteration, logging and resource
registration are faked; bitmap validation, byte counts and descriptors are
extracted from the implementation. GPU/runtime checks remain separate.
"""
import re
import shutil
import subprocess
import sys

import pytest

from tools.harness import enum_with, function, read, structure


def array(source, name):
    return re.search(r"static [^;{}]*\b" + name + r"\s*\[[^;]*?\n\};", source).group()


def implementation():
    bitmap = read("source/bitmaps/bitmaps.c")
    group = read("source/bitmaps/bitmap_group.h")
    ce = read("port/linux/game/custom_edition_bitmaps.c")
    cache = read("source/cache/xbox_texture_cache.c")
    gpu = read("port/linux/src/xbox_textures.c")
    definitions = read("port/include/xdk/xdk_d3d8.h")
    parts = [PREAMBLE]
    parts += [enum_with(bitmap, name) for name in (
        "MAXIMUM_BITMAP_WIDTH =", "_bitmap_type_2d,", "_bitmap_format_a8,",
        "_bitmap_has_power_of_two_dimensions_bit,")]
    parts += [enum_with(group, "BITMAP_GROUP_TAG ="), structure(group, "bitmap_data"),
              structure(read("port/linux/src/xgpu.h"), "xgpu_texture_description"),
              enum_with(read("port/include/xdk/xdk_pdb.h"), "D3DFMT_DXT1 ="),
              enum_with(cache, "_bitmap_d3d_format_table_regular,"),
              array(bitmap, "bitmap_format_bits_per_pixel_table"),
              array(cache, "bitmap_d3d_format_tables")]
    parts += [
        line for line in definitions.splitlines()
        if re.match(r"#define (?:D3DCOMMON|D3DFORMAT|D3DSIZE|D3DTEXTURE)_", line)]
    parts += [function(gpu, "floor_log2")]
    parts += [function(bitmap, name) for name in (
        "bitmap_format_type_valid_width", "bitmap_format_type_valid_height",
        "bitmap_format_type_valid_depth", "bitmap_verify", "bitmap_format_get_bits_per_pixel",
        "bitmap_mipmap_get_width", "bitmap_mipmap_get_height", "bitmap_mipmap_get_depth",
        "bitmap_mipmap_get_pixel_count", "bitmap_get_pixel_count", "bitmap_get_pixel_data_size",
        "bitmap_mipmap_get_row_pitch")]
    parts += [function(ce, name) for name in (
        "power_of_two", "bitmap_format_drawable", "bitmap_readable", "bitmap_swizzle_possible",
        "custom_edition_bitmap_drawable")]
    parts += [WORLD, function(ce, "custom_edition_bitmaps_verify"),
              function(ce, "custom_edition_bitmap_pixels_arrived"),
              function(read("port/linux/game/custom_edition_cache.c"), "custom_edition_cache_contains")]
    parts += [function(cache, name) for name in (
        "bitmap_format_to_d3d_format", "bitmap_format_to_d3d_linear_format",
        "texture_cache_initialize_hardware_format")]
    parts += [enum_with(gpu, "_texel_unknown,"), structure(gpu, "format_information")]
    parts += [function(gpu, name) for name in (
        "format_information", "kind_compressed", "level_dimension", "xgpu_texture_describe",
        "level_bytes", "xgpu_texture_level_offset", "xgpu_texture_face_size", "xgpu_texture_level_pitch")]
    return "\n".join(parts)


PREAMBLE = r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef int boolean;
typedef int BOOL;
typedef unsigned long DWORD;
typedef unsigned short word;
typedef unsigned char byte;
union point2d { struct {short x, y;} coordinates; long value; };
typedef struct { DWORD Common, Data, Lock, Format, Size; } D3DBaseTexture;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define FLAG(n) (1U << (n))
#define TEST_FLAG(value, bit) ((value) & FLAG(bit))
#define VALID_INDEX(n, count) ((unsigned)(n) < (unsigned)(count))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define match_assert(file,line,test) assert(test)
#define match_vassert(file,line,test,message) assert(test)
#define error(...) ((void)0)
#define IDirect3DBaseTexture8_Register(texture,address) ((void)0)
#define rasterizer_xbox_bitmap_get_max_mipmap_count(bitmap) ((bitmap)->mipmap_count)
"""

WORLD = r"""
static struct custom_edition_cache_globals {
    void *tag_cache;
    unsigned long loaded_bytes;
    int tags_loaded;
} custom_edition_cache_globals;
struct bitmap_group { struct { long count; struct bitmap_data *address; } bitmaps; };
static struct bitmap_group group;
static int mcc_loaded;
#define mcc_cache_tags_loaded() mcc_loaded
#define TAG_BLOCK_GET_ELEMENT(block,index,type) (&(block)->address[index])
#define custom_edition_cache_tag_name(cache,bytes,index) "fixture"
static struct bitmap_group *custom_edition_cache_tag_get(
    void *cache, unsigned long bytes, unsigned long index, unsigned long tag, unsigned long size)
{
    return &group;
}
#define reordered_bitmap_index(tag,index) NONE
#define xbox_pointer(pointer) (pointer)
#define _custom_edition_channels_xbox 0
#define halo_custom_edition_texels_channels(pointer,order) ((void)0)
static struct {struct {int channel_order;} reordered_bitmaps[1];} custom_edition_bitmaps_globals;
static int rebuilds;
#define rasterizer_xbox_bitmap_rebuild_hardware_format(bitmap) (++rebuilds)
static struct bitmap_group *custom_edition_cache_tag_next(
    void *cache, unsigned long bytes, unsigned long tag, unsigned long size, int32_t *index)
{
    if (*index != NONE) return NULL;
    *index = 0;
    return &group;
}
"""

CASES = r"""
static struct bitmap_data fixture(short format)
{
    struct bitmap_data bitmap;
    memset(&bitmap, 0, sizeof(bitmap));
    bitmap.signature = BITMAP_GROUP_TAG;
    bitmap.width = 200; bitmap.height = 112; bitmap.depth = 1;
    bitmap.format = format;
    bitmap.flags = FLAG(_bitmap_compressed_bit);
    bitmap.pixels_size = bitmap_get_pixel_data_size(&bitmap);
    return bitmap;
}
static int admitted(struct bitmap_data *bitmap)
{
    group.bitmaps.count = 1; group.bitmaps.address = bitmap;
    return custom_edition_bitmaps_verify(NULL, 0);
}
static struct xgpu_texture_description describe(struct bitmap_data *bitmap)
{
    D3DBaseTexture texture;
    struct xgpu_texture_description description;
    texture_cache_initialize_hardware_format(bitmap, &texture);
    xgpu_texture_describe(texture.Format, texture.Size, &description);
    return description;
}
int main(int argc, char **argv)
{
    struct bitmap_data bitmap, other;
    struct xgpu_texture_description desc;
    short format;
    int mode = argc > 1 ? argv[1][0] : 0;
    bitmap = fixture(_bitmap_format_dxt1);
    custom_edition_cache_globals.tag_cache = &bitmap;
    custom_edition_cache_globals.loaded_bytes = sizeof(bitmap);
    custom_edition_cache_globals.tags_loaded = TRUE;
    if (mode == 'a') {
        for (format = _bitmap_format_dxt1; format <= _bitmap_format_dxt5; format++) {
            bitmap = fixture(format);
            assert(admitted(&bitmap));
            assert(!custom_edition_bitmap_drawable(&bitmap));
            desc = describe(&bitmap);
            assert(desc.width == 200 && desc.height == 112 && desc.depth == 1);
            assert(desc.levels == 1 && desc.compressed && !desc.linear && !desc.cube_map);
            assert(xgpu_texture_face_size(&desc) == (format == _bitmap_format_dxt1 ? 11200 : 22400));
            assert(xgpu_texture_face_size(&desc) == bitmap.pixels_size);
            assert(xgpu_texture_level_pitch(&desc, 0) == (format == _bitmap_format_dxt1 ? 400 : 800));
            bitmap.width = 4096; bitmap.height = 4092;
            bitmap.pixels_size = bitmap_get_pixel_data_size(&bitmap);
            assert(admitted(&bitmap));
            desc = describe(&bitmap);
            assert(desc.width == 4096 && desc.height == 4092);
        }
    } else if (mode == 'r') {
        bitmap.pixels_size--; assert(!admitted(&bitmap));
        bitmap = fixture(_bitmap_format_dxt1); bitmap.mipmap_count = 1;
        bitmap.pixels_size = bitmap_get_pixel_data_size(&bitmap); assert(admitted(&bitmap));
        desc = describe(&bitmap); assert(desc.levels == 1); /* upstream draws only the first NPOT level */
        bitmap = fixture(_bitmap_format_dxt1); bitmap.width = 201; assert(!admitted(&bitmap));
        bitmap = fixture(_bitmap_format_dxt1); bitmap.height = 111; assert(admitted(&bitmap));
        bitmap = fixture(_bitmap_format_dxt1); bitmap.width = MAXIMUM_BITMAP_WIDTH + 1;
        assert(!admitted(&bitmap));
        bitmap = fixture(_bitmap_format_dxt1); bitmap.flags |= FLAG(_bitmap_linear_bit);
        assert(!admitted(&bitmap));
        bitmap = fixture(_bitmap_format_dxt1); bitmap.flags = 0; assert(!admitted(&bitmap));
        bitmap = fixture(_bitmap_format_p8_bump); bitmap.flags = 0;
        bitmap.pixels_size = bitmap_get_pixel_data_size(&bitmap); assert(admitted(&bitmap));
        bitmap = fixture(_bitmap_format_dxt1); bitmap.type = _bitmap_type_cube_map;
        bitmap.width = bitmap.height = 96;
        bitmap.pixels_size = bitmap_get_pixel_data_size(&bitmap); assert(admitted(&bitmap));
        bitmap.type = _bitmap_type_3d; bitmap.depth = 4;
        bitmap.pixels_size = bitmap_get_pixel_data_size(&bitmap); assert(admitted(&bitmap));
    } else if (mode == 'o') {
        assert(custom_edition_cache_contains(&bitmap, sizeof(bitmap)));
        assert(!custom_edition_cache_contains(&bitmap, sizeof(bitmap) + 1));
        assert(!custom_edition_cache_contains((char *)&bitmap + sizeof(bitmap), 1));
        assert(!custom_edition_cache_contains((void *)~0UL, 2));
        other = bitmap; assert(!custom_edition_cache_contains(&other, sizeof(other)));
        desc = describe(&other); assert(desc.width == 200 && desc.height == 112);
        mcc_loaded = TRUE; desc = describe(&bitmap);
        assert(desc.width == 200 && desc.height == 112);
        mcc_loaded = FALSE; custom_edition_cache_globals.tags_loaded = FALSE;
        assert(!custom_edition_cache_contains(&bitmap, sizeof(bitmap)));
        desc = describe(&bitmap); assert(desc.width == 200 && desc.height == 112);
        custom_edition_cache_globals.tags_loaded = TRUE;
        desc = describe(&bitmap); assert(desc.width == 200 && desc.height == 112);
    } else if (mode == 'p') {
        bitmap.base_address = &other;
        group.bitmaps.count = 1; group.bitmaps.address = &bitmap;
        custom_edition_bitmap_pixels_arrived(NULL, 0, 0, 0, &other);
        assert(!rebuilds);
        bitmap.width = 128; bitmap.height = 64;
        custom_edition_bitmap_pixels_arrived(NULL, 0, 0, 0, &other);
        assert(rebuilds == 1);
        bitmap = fixture(_bitmap_format_a8r8g8b8);
        bitmap.base_address = &other; bitmap.flags = FLAG(_bitmap_linear_bit);
        bitmap.pixels_size = bitmap_get_pixel_data_size(&bitmap);
        custom_edition_bitmap_pixels_arrived(NULL, 0, 0, 0, &other);
        assert(rebuilds == 2);
    } else if (mode == 'l') {
        bitmap.width = 256; bitmap.height = 128; bitmap.mipmap_count = 3;
        bitmap.pixels_size = bitmap_get_pixel_data_size(&bitmap);
        assert(admitted(&bitmap) && custom_edition_bitmap_drawable(&bitmap));
        desc = describe(&bitmap);
        assert(desc.width == 256 && desc.height == 128 && desc.levels == 4 && !desc.linear);
        bitmap = fixture(_bitmap_format_a8r8g8b8);
        bitmap.flags = FLAG(_bitmap_linear_bit);
        bitmap.pixels_size = bitmap_get_pixel_data_size(&bitmap);
        assert(admitted(&bitmap) && custom_edition_bitmap_drawable(&bitmap));
        desc = describe(&bitmap);
        assert(desc.width == 200 && desc.height == 112 && desc.linear && !desc.compressed);
        assert(desc.pitch == 832 && xgpu_texture_face_size(&desc) == 832 * 112);
    } else return 2;
    puts("bitmap contract passed");
    return 0;
}
"""


@pytest.fixture(scope="module")
def bitmap_probe(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is required for the 32-bit bitmap probe")
    directory = tmp_path_factory.mktemp("ce-npot")
    source = directory / "probe.c"
    source.write_text(implementation() + CASES)
    executable = directory / ("probe.exe" if sys.platform == "win32" else "probe")
    target = (["--target=i686-pc-windows-msvc", "-fuse-ld=lld"]
              if sys.platform == "win32" else ["-m32"])
    result = subprocess.run([clang, *target, "-std=gnu89", "-O1",
                             "-fsanitize=undefined", "-fsanitize-trap=undefined",
                             str(source), "-o", str(executable)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return executable


@pytest.mark.parametrize("case", ["admission", "rejections", "ownership", "pixels", "legacy"])
def test_bitmap_contract(bitmap_probe, case):
    result = subprocess.run([str(bitmap_probe), case], capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr
