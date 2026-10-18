#include "xbox_swap.h"

/* the 0x800-byte cache file header, as the engine's struct cache_file_header (cache_files.c) lays it out:
   the signature, version, file length, tag data offset and size, checksum and footer signature are words;
   the rest is bytes (names, reserved) */
void mac_xbox_cache_header_to_native(void *header)
{
    static const unsigned short word_at[] = { 0x00, 0x04, 0x08, 0x10, 0x14, 0x64, 0x7fc };
    unsigned char *b = (unsigned char *)header;
    unsigned i;
    /* a header already in host order (its signature 'head' reads as that value) is left as it is */
    if (b[0] != 'd' || b[1] != 'a' || b[2] != 'e' || b[3] != 'h') return;
    for (i = 0; i < sizeof(word_at) / sizeof(word_at[0]); ++i) {
        unsigned char *p = b + word_at[i];
        uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
        *(uint32_t *)(void *)p = v;
    }
}
