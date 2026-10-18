/* Native boundary for cache capabilities that are not part of the Mac
   renderer yet. Custom Edition's fixed-address tag cache is deliberately not
   emulated by an unrelated heap allocation. */
#include "port/linux/src/port_config.h"

#include <stddef.h>
#include <stdlib.h>

void platform_log(const char *format, ...);

void *halo_custom_edition_tag_cache(void)
{
    static int reported;

    if (config_boolean("game.custom_edition") && !reported) {
        platform_log("Custom Edition maps are unavailable: the native runtime does not reserve the required fixed-address tag cache");
        reported = 1;
    }
    return NULL;
}

struct texel_channel_order {
    unsigned long address;
    unsigned char order;
};

static struct texel_channel_order *texels;
static size_t texel_count;
static size_t texel_capacity;

/* Cache-file metadata is attached to the exact CPU texel address while its
   bitmap is live. The native sampler can query this before freeing or reusing
   the allocation. The game and renderer share the engine thread. */
void halo_custom_edition_texels_channels(const void *pixels, unsigned char order)
{
    size_t i;
    unsigned long address = (unsigned long)pixels;

    if (order > 2)
        order = 0;
    for (i = 0; i < texel_count && texels[i].address != address; ++i) {}
    if (i < texel_count) {
        if (order == 0)
            texels[i] = texels[--texel_count];
        else
            texels[i].order = order;
    } else if (order != 0) {
        if (texel_count == texel_capacity) {
            size_t capacity = texel_capacity ? texel_capacity * 2 : 64;
            struct texel_channel_order *grown;
            if (capacity < texel_capacity || capacity > (size_t)-1 / sizeof(*grown)) {
                platform_log("Custom Edition texel metadata capacity overflow");
                return;
            }
            grown = (struct texel_channel_order *)realloc(texels, capacity * sizeof(*grown));
            if (!grown) {
                platform_log("no memory to track Custom Edition texel channel order");
                return;
            }
            texels = grown;
            texel_capacity = capacity;
        }
        texels[texel_count].address = address;
        texels[texel_count].order = order;
        ++texel_count;
    }
}

unsigned char mac_custom_edition_texels_order(const void *pixels)
{
    size_t i;
    unsigned long address = (unsigned long)pixels;
    for (i = 0; i < texel_count; ++i)
        if (texels[i].address == address)
            return texels[i].order;
    return 0;
}

void halo_custom_edition_texels_forget(void)
{
    free(texels);
    texels = NULL;
    texel_count = 0;
    texel_capacity = 0;
}
