/* The Mac's high-res HUD (mac_hud_hires.h): which embedded texture stands for
   a bitmap of an Xbox map, and its texels. The game's sources ask what the
   textures are (port/linux/game/hud_hires_tags.c names the bitmaps of the map
   loaded that they stand for); the texture cache (mac_gpu_textures.c) asks,
   as it makes the texture of a bitmap, whether one is drawn in its place.

   The texture is a PNG reduced to twice the bitmap's size, decoded when
   first wanted and made a GL texture of with its mip levels, so that a
   draw at the bitmap's size takes the level of that size and a larger
   window the sharper one. The other platforms' version is
   port/linux/src/hud_hires.c. Only 8-bit RGBA, non-interlaced PNGs are read
   (the ones tools/mac_hud_reduce.py writes), inflated by the system's zlib. */
#include "mac_hud_hires.h"
#include "../linux/include/hud_button_variants.h"
#include "../linux/include/input_prompts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

/* mac_gpu_textures.c */
extern void mac_gpu_textures_hud_provider(const struct mac_gpu_hud_provider *provider);
/* port/linux/src/port_config.c */
extern int config_boolean(const char *name);
extern unsigned long config_changes(void);
extern const char *config_string(const char *name);
/* port/linux/game/hud_hires_tags.c: the asset whose bitmap's pixels are at
   address (guest virtual), of this size, or -1 */
extern long hud_hires_asset_at(unsigned long address, long width, long height);

/* the most a PNG's inflated rows and its texels may take together */
#define MAXIMUM_DECODED_SIZE (48UL << 20)

/* ---------- what the game's sources ask */

long hud_hires_asset_count(void)
{
    return (long)mac_hud_assets_count;
}

char const *hud_hires_asset_tag(long asset)
{
    return mac_hud_assets[asset].tag;
}

long hud_hires_asset_bitmap(long asset)
{
    return (long)mac_hud_assets[asset].bitmap;
}

/* whether the texture can stand for a bitmap of this size: a whole multiple
   of it, the same both ways, and larger */
long hud_hires_asset_fits(long asset, long width, long height)
{
    const struct mac_hud_asset *embedded = &mac_hud_assets[asset];

    return width > 0 && height > 0 && embedded->width % (unsigned)width == 0 &&
        embedded->height % (unsigned)height == 0 &&
        embedded->width / (unsigned)width == embedded->height / (unsigned)height &&
        embedded->width / (unsigned)width > 1;
}

/* (no asset of the Mac's stands for only some of a bitmap's sprites: those are
   the menus' buttons) */
unsigned long hud_hires_asset_sprites(long asset)
{
    (void)asset;
    return 0;
}

int hud_hires_sprites_drawable(long asset, unsigned long address, unsigned long level0_size)
{
    (void)asset;
    (void)address;
    (void)level0_size;
    return 0;
}

/* The textures the game draws from a placeholder bitmap of its own making (the gameplay
   prompts' button faces, port/linux/game/hud_hires_tags.c): the placeholder's texels are
   never written, so the texture cache asks which asset stands for the address of its
   Data (hud_find) and draws that. */
#define MAXIMUM_PLACEHOLDERS 32
/* (a D3D texture's Data is a physical address; the engine's contiguous memory is mapped at this base:
   PLATFORM_CONTIGUOUS_BASE of native_resource_memory.h, which needs the engine's headers) */
#define CONTIGUOUS_BASE 0x80000000UL

static struct {
    unsigned long address;
    long asset;
} placeholders[MAXIMUM_PLACEHOLDERS];
static long placeholder_count;

void hud_hires_register_placeholder(long asset, unsigned long const *texture)
{
    long index;

    for (index = 0; index < placeholder_count && placeholders[index].asset != asset; index++)
        ;
    if (texture && index == placeholder_count && placeholder_count < MAXIMUM_PLACEHOLDERS)
        placeholder_count++;
    if (index < placeholder_count) {
        placeholders[index].address = texture ? (texture[1] | CONTIGUOUS_BASE) : 0;
        placeholders[index].asset = asset;
    }
}

/* ---------- what the texture cache asks */

static unsigned long crc_table[256];

static unsigned long crc32_bytes(const unsigned char *bytes, unsigned long size)
{
    unsigned long crc = 0xffffffffUL, index;

    if (!crc_table[255]) {
        unsigned long entry, bit;

        for (entry = 0; entry < 256; entry++) {
            unsigned long value = entry;

            for (bit = 0; bit < 8; bit++)
                value = value & 1 ? 0xedb88320UL ^ (value >> 1) : value >> 1;
            crc_table[entry] = value;
        }
    }
    for (index = 0; index < size; index++)
        crc = crc_table[(crc ^ bytes[index]) & 0xff] ^ (crc >> 8);
    return crc ^ 0xffffffffUL;
}

static int hud_enabled(void)
{
    static unsigned long read_at = (unsigned long)-1;
    static int enabled;

    if (read_at != config_changes()) {
        read_at = config_changes();
        enabled = config_boolean("display.high_res_hud") != 0;
    }
    return enabled;
}

/* the asset standing for the bitmap whose texels are here (not whether the
   setting is on): the bitmap of a tag the map has, of the size its texture
   fits, with the pixels the texture was drawn for (another language's maps,
   or modified ones, have others: drawn as they are) */
static long hud_find(unsigned long address, unsigned long width, unsigned long height, unsigned long level0_size)
{
    static unsigned char reported[256];
    long asset, index;

    for (index = 0; address && index < placeholder_count; index++) {
        if (placeholders[index].address == address)
            return placeholders[index].asset;
    }
    asset = hud_hires_asset_at(address, (long)width, (long)height);
    if (asset < 0 || asset >= hud_hires_asset_count())
        return -1;
    if (crc32_bytes((const unsigned char *)address, level0_size) != mac_hud_assets[asset].crc) {
        if (asset < 256 && !reported[asset]) {
            reported[asset] = 1;
            fprintf(stderr, "high-res hud: %s bitmap %d is not the one its texture was drawn for here "
                "(another language's or a modified map): drawn as it is\n",
                mac_hud_assets[asset].tag, mac_hud_assets[asset].bitmap);
        }
        return -1;
    }
    return asset;
}

/* ---------- decoding */

static unsigned long big_endian_long(const unsigned char *bytes)
{
    return ((unsigned long)bytes[0] << 24) | ((unsigned long)bytes[1] << 16) |
        ((unsigned long)bytes[2] << 8) | bytes[3];
}

static unsigned char paeth(unsigned char left, unsigned char up, unsigned char up_left)
{
    int estimate = (int)left + up - up_left;
    int to_left = abs(estimate - left), to_up = abs(estimate - up), to_up_left = abs(estimate - up_left);

    if (to_left <= to_up && to_left <= to_up_left)
        return left;
    return to_up <= to_up_left ? up : up_left;
}

/* the PNG's texels, RGBA in rows top first, and its size; NULL if it is not
   one that tools/mac_hud_reduce.py writes */
static unsigned char *png_decode(const unsigned char *data, unsigned long size, unsigned long *png_width,
    unsigned long *png_height)
{
    unsigned long position = 8;
    unsigned long width = size >= 33 ? big_endian_long(data + 16) : 0;
    unsigned long height = size >= 33 ? big_endian_long(data + 20) : 0;
    unsigned long stride = width * 4, filtered_size = height * (stride + 1);
    unsigned char *compressed = NULL, *filtered = NULL, *pixels = NULL;
    unsigned long compressed_size = 0, row, column;
    uLongf inflated_size = (uLongf)filtered_size;
    int result;

    if (size < 33 || memcmp(data, "\x89PNG\r\n\x1a\n", 8) || memcmp(data + 12, "IHDR", 4) ||
        !width || !height || width > 4096 || height > 4096 ||
        data[24] != 8 || data[25] != 6 || data[28] != 0)
        return NULL;
    if (filtered_size + stride * height > MAXIMUM_DECODED_SIZE)
        return NULL;
    *png_width = width;
    *png_height = height;
    compressed = (unsigned char *)malloc(size);
    while (compressed && position + 12 <= size) {
        unsigned long length = big_endian_long(data + position);

        if (length > size - position - 12)
            break;
        if (!memcmp(data + position + 4, "IDAT", 4)) {
            memcpy(compressed + compressed_size, data + position + 8, length);
            compressed_size += length;
        } else if (!memcmp(data + position + 4, "IEND", 4)) {
            break;
        }
        position += 12 + length;
    }
    filtered = compressed ? (unsigned char *)malloc(filtered_size) : NULL;
    pixels = filtered ? (unsigned char *)malloc(stride * height) : NULL;
    if (!pixels)
        goto failed;
    result = uncompress(filtered, &inflated_size, compressed, (uLong)compressed_size);
    if ((result != Z_OK && result != Z_BUF_ERROR) || inflated_size != filtered_size)
        goto failed;
    /* (each row by its filter, the first pixel's 4 bytes, which have none to
       their left, apart; the first row has none above: zeroes) */
    for (row = 0; row < height; row++) {
        const unsigned char *line = filtered + row * (stride + 1) + 1;
        unsigned char filter = line[-1];
        unsigned char *out = pixels + row * stride;
        const unsigned char *above = row ? out - stride : NULL;

        if (filter > 4)
            goto failed;
        if (!above && filter == 2)
            filter = 0; /* (up: zero) */
        else if (!above && filter == 4)
            filter = 1; /* (Paeth of left, zero and zero: left) */
        switch (filter) {
        case 0:
            memcpy(out, line, stride);
            break;
        case 1:
            memcpy(out, line, 4);
            for (column = 4; column < stride; column++)
                out[column] = (unsigned char)(line[column] + out[column - 4]);
            break;
        case 2:
            for (column = 0; column < stride; column++)
                out[column] = (unsigned char)(line[column] + above[column]);
            break;
        case 3:
            for (column = 0; column < 4; column++)
                out[column] = (unsigned char)(line[column] + (above ? above[column] : 0) / 2);
            for (column = 4; column < stride; column++)
                out[column] = (unsigned char)(line[column] +
                    ((unsigned)out[column - 4] + (above ? above[column] : 0)) / 2);
            break;
        default:
            /* (Paeth of zero, up and zero: up) */
            for (column = 0; column < 4; column++)
                out[column] = (unsigned char)(line[column] + above[column]);
            for (column = 4; column < stride; column++)
                out[column] = (unsigned char)(line[column] + paeth(out[column - 4], above[column], above[column - 4]));
            break;
        }
    }
    free(compressed);
    free(filtered);
    return pixels;

failed:
    free(compressed);
    free(filtered);
    free(pixels);
    return NULL;
}

/* the asset whose texels draw for this one now: the PlayStation face for an Xbox button
   icon while the prompts show PlayStation faces (hud_button_variants.h), else itself */
static long hud_shown(long asset)
{
    static long found[256];
    static unsigned char known[256];
    long index;
    const struct hud_button_variant *variant;

    if (asset < 0 || asset >= hud_hires_asset_count() || asset >= 256)
        return asset;
    if (!known[asset]) {
        known[asset] = 1;
        found[asset] = -1;
        variant = hud_button_variant_find(mac_hud_assets[asset].tag);
        for (index = 0; variant && index < hud_hires_asset_count(); index++) {
            if (!strcmp(mac_hud_assets[index].tag, variant->playstation_tag))
                found[asset] = index;
        }
    }
    if (found[asset] < 0 || !input_prompt_glyphs_playstation(config_string("input.prompt_glyphs")))
        return asset;
    return found[asset];
}

static unsigned char *hud_decode(long asset, unsigned *width, unsigned *height)
{
    unsigned long png_width = 0, png_height = 0;
    unsigned char *pixels;
    const struct hud_button_variant *variant = NULL;
    long own = asset;

    if (asset < 0 || asset >= hud_hires_asset_count())
        return NULL;
    asset = hud_shown(asset);
    if (asset != own)
        variant = hud_button_variant_find(mac_hud_assets[own].tag);
    pixels = png_decode(mac_hud_assets[asset].png, mac_hud_assets[asset].png_size, &png_width, &png_height);
    if (!pixels) {
        fprintf(stderr, "high-res hud: could not decode the texture for %s bitmap %d\n",
            mac_hud_assets[asset].tag, mac_hud_assets[asset].bitmap);
        return NULL;
    }
    if (variant && (variant->scale != 1.0f || variant->x != 0.0f || variant->y != 0.0f)) {
        /* (a small icon: the face where that icon's art lies in its bitmap, whose size is the same
           32 texels across) */
        unsigned char *placed = hud_button_variant_place(pixels, png_width, png_height, 32, variant->scale,
            variant->x, variant->y);

        free(pixels);
        pixels = placed;
        if (!pixels)
            return NULL;
    }
    *width = (unsigned)png_width;
    *height = (unsigned)png_height;
    return pixels;
}

void mac_hud_hires_install(void)
{
    static const struct mac_gpu_hud_provider provider = { hud_find, hud_enabled, hud_shown, hud_decode };

    if (mac_hud_assets_count)
        mac_gpu_textures_hud_provider(&provider);
}
