#include "cache_endian.h"
#include <string.h>

/* Layout evidence: source/cache/cache_files{,_windows}.c,
   source/scenario/scenario_definitions.h, source/memory/data.h and
   source/hs/hs_scenario_definitions.h. Never overlay native pointers. */
static uint16_t read16(const unsigned char *p, int native)
{
    uint16_t v;
    if (native) { memcpy(&v, p, 2); return v; }
    return (uint16_t)((unsigned int)p[0] | ((unsigned int)p[1] << 8));
}
static uint32_t read32(const unsigned char *p, int native)
{
    uint32_t v;
    if (native) { memcpy(&v, p, 4); return v; }
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void convert16(unsigned char *p, int encode)
{
    uint16_t v = read16(p, encode);
    if (!encode) memcpy(p, &v, 2);
    else { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }
}
static void convert32(unsigned char *p, int encode)
{
    uint32_t v = read32(p, encode);
    if (!encode) memcpy(p, &v, 4);
    else {
        p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8);
        p[2] = (unsigned char)(v >> 16); p[3] = (unsigned char)(v >> 24);
    }
}
int mac_cache_record_convert(void *bytes, size_t size,
    enum mac_cache_record record, int encode)
{
    static const unsigned int header_words[] = {0,4,8,0x10,0x14,0x64,0x7fc};
    unsigned char *p = (unsigned char *)bytes;
    unsigned int i, words;
    size_t required;
    if (!p || (encode != 0 && encode != 1)) return 0;
    switch (record) {
    case MAC_CACHE_HEADER: required = 0x800; words = 0; break;
    case MAC_CACHE_TAG_HEADER: required = 0x24; words = 9; break;
    case MAC_CACHE_TAG_INSTANCE: required = 0x20; words = 6; break;
    case MAC_CACHE_BSP_HEADER: required = 0x18; words = 6; break;
    case MAC_CACHE_BSP_REFERENCE: required = 0x20; words = 3; break;
    case MAC_CACHE_GPU_BUFFER: required = 12; words = 3; break;
    case MAC_CACHE_POINT_PHYSICS: required = 0x40; words = 4; break;
    case MAC_CACHE_WIND: required = 0x40; words = 7; break;
    case MAC_CACHE_SOUND_ENVIRONMENT: required = 0x48; words = 1; break;
    default: return 0;
    }
    if (size != required) return 0;
    for (i = 0; i < words; ++i) convert32(p + i * 4, encode);
    if (record == MAC_CACHE_HEADER) {
        for (i = 0; i < sizeof(header_words)/sizeof(header_words[0]); ++i)
            convert32(p + header_words[i], encode);
        convert16(p + 0x60, encode);
    }
    if (record == MAC_CACHE_BSP_REFERENCE)
        for (i = 0x10; i < 0x20; i += 4) convert32(p + i, encode);
    /* point_physics.h: runtime coefficients are consumed by gameplay too.
       Only unused1/unused2 are preserved as uninterpreted bytes. */
    if (record == MAC_CACHE_POINT_PHYSICS)
        for (i = 0x20; i < 0x34; i += 4) convert32(p + i, encode);
    /* wind_definitions.h: seven floats, followed by unused bytes.
       sound_environment_definitions.h: unknown long, signed priority,
       twelve floats. Preserve unused0 and unused1 verbatim. */
    if (record == MAC_CACHE_SOUND_ENVIRONMENT) {
        convert16(p + 4, encode);
        for (i = 8; i < 0x38; i += 4) convert32(p + i, encode);
    }
    return 1;
}
int mac_cache_syntax_convert(void *bytes, size_t size,
    unsigned int expected_capacity, int encode)
{
    static const unsigned int shorts[] = {0x20,0x22,0x2c,0x2e,0x30,0x32};
    unsigned char *p = (unsigned char *)bytes;
    unsigned int capacity, count, actual, first, i, j;
    unsigned char *node;
    if (!p || size < 0x38 || (encode != 0 && encode != 1) ||
        expected_capacity > 32767) return 0;
    capacity = read16(p + 0x20, encode);
    count = read16(p + 0x2e, encode);
    actual = read16(p + 0x30, encode);
    first = read16(p + 0x2c, encode);
    if (capacity != expected_capacity || size != 0x38 + (size_t)capacity * 0x14 ||
        read16(p + 0x22, encode) != 0x14 || !p[0x24] ||
        read32(p + 0x28, encode) != UINT32_C(0x64407440) ||
        count > capacity || actual > count || first > capacity) return 0;
    for (i = 0; i < sizeof(shorts)/sizeof(shorts[0]); ++i)
        convert16(p + shorts[i], encode);
    convert32(p + 0x28, encode);
    convert32(p + 0x34, encode);
    for (i = 0; i < capacity; ++i) {
        node = p + 0x38 + (size_t)i * 0x14;
        for (j = 0; j < 8; j += 2) convert16(node + j, encode);
        for (j = 8; j < 0x14; j += 4) convert32(node + j, encode);
    }
    return 1;
}
int mac_cache_address_offset(uint32_t address, uint32_t base,
    size_t span, size_t object_size, size_t *offset)
{
    uint32_t delta;
    if (!offset || address < base || !object_size ||
        (uint64_t)span > UINT64_C(0x100000000) - base) return 0;
    delta = address - base;
    if ((size_t)delta > span || object_size > span - (size_t)delta) return 0;
    *offset = (size_t)delta;
    return 1;
}
