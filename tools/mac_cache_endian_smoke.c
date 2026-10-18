#include "cache_endian.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks, failures;
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; \
    printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static void put16(unsigned char *p, unsigned int v)
{ p[0] = v; p[1] = v >> 8; }
static void put32(unsigned char *p, uint32_t v)
{ p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
static uint32_t native32(const unsigned char *p)
{ uint32_t v; memcpy(&v, p, 4); return v; }
static unsigned int native16(const unsigned char *p)
{ uint16_t v; memcpy(&v, p, 2); return v; }
static uint32_t disk32(const unsigned char *p)
{ return p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static void records(void)
{
    const size_t sizes[] = {0x800,0x24,0x20,0x18,0x20,12,0x40,0x40,0x48};
    const unsigned int h[] = {0,4,8,0x10,0x14,0x64,0x7fc};
    unsigned char data[0x802], before[0x802], mask[0x800];
    unsigned int r, i, j, n;
    for (r = 0; r < sizeof(sizes)/sizeof(sizes[0]); ++r) {
        n = sizes[r];
        for (i = 0; i < sizeof(data); ++i) data[i] = (i * 37 + 13) & 255;
        memcpy(before, data, sizeof(data));
        memset(mask, 0, sizeof(mask));
        /* Deliberately unaligned to catch unsafe native overlays. */
        CHECK(mac_cache_record_convert(data+1, n, r, 0));
        for (i = 0; i < n; i += 4) {
            int word = r == 1 || r == 3 || r == 5 ||
                (r == 2 && i < 24) || (r == 4 && i != 12) ||
                (r == 6 && (i < 0x10 || (i >= 0x20 && i < 0x34))) ||
                (r == 7 && i < 0x1c) ||
                (r == 8 && (i == 0 || (i >= 8 && i < 0x38)));
            if (r == 0) for (j = 0; j < 7; ++j) if (i == h[j]) word = 1;
            if (word) {
                CHECK(native32(data+1+i) == disk32(before+1+i));
                memset(mask+i, 1, 4);
            }
        }
        if (r == 0) {
            CHECK(native16(data+1+0x60) == (unsigned int)(before[1+0x60] | before[1+0x61]<<8));
            mask[0x60] = mask[0x61] = 1;
        }
        if (r == 8) {
            CHECK(native16(data+1+4) == (unsigned int)(before[5] | before[6]<<8));
            mask[4] = mask[5] = 1;
        }
        for (i = 0; i < n; ++i) if (!mask[i]) CHECK(data[i+1] == before[i+1]);
        CHECK(data[0] == before[0] && data[n+1] == before[n+1]);
        CHECK(mac_cache_record_convert(data+1, n, r, 1));
        CHECK(memcmp(data, before, sizeof(data)) == 0);
        CHECK(!mac_cache_record_convert(data+1, n-1, r, 0));
        CHECK(!mac_cache_record_convert(data+1, n+1, r, 0));
        CHECK(!mac_cache_record_convert(data+1, n, r, 2));
        CHECK(memcmp(data, before, sizeof(data)) == 0);
    }
    CHECK(!mac_cache_record_convert(data, 12, (enum mac_cache_record)99, 0));
    CHECK(!mac_cache_record_convert(NULL, 12, MAC_CACHE_GPU_BUFFER, 0));
}
static void scripts(void)
{
    const unsigned int capacity = 19001;
    const size_t size = 0x38 + 19001 * 0x14;
    unsigned char *allocation = malloc(size+2), *backup = malloc(size+2), *p, *node;
    unsigned int i, j;
    const unsigned int bad_offsets[] = {0x20,0x22,0x24,0x28,0x2c,0x2e,0x30};
    CHECK(allocation && backup);
    if (!allocation || !backup) exit(2);
    memset(allocation, 0xa5, size+2);
    p = allocation+1;
    memcpy(p, "script node", 12);
    put16(p+0x20, capacity); put16(p+0x22, 20);
    p[0x24] = 1; p[0x25] = 0;
    put32(p+0x28, UINT32_C(0x64407440));
    put16(p+0x2c, 1); put16(p+0x2e, 3); put16(p+0x30, 2);
    put16(p+0x32, 0x8001); put32(p+0x34, UINT32_C(0x803a6038));
    for (i = 0; i < capacity; ++i) {
        node = p+0x38+i*20;
        for (j = 0; j < 8; j += 2) put16(node+j, (i+j+0x8001)&65535);
        put32(node+8, UINT32_C(0xffffffff));
        put32(node+12, i*11);
        /* Dirty high bits, signed short low bits; convert entire value. */
        put32(node+16, UINT32_C(0xa5a58001)+i);
    }
    memcpy(backup, allocation, size+2);
    CHECK(mac_cache_syntax_convert(p, size, capacity, 0));
    CHECK(native16(p+0x20) == capacity && native16(p+0x32) == 0x8001);
    CHECK(native32(p+0x34) == UINT32_C(0x803a6038));
    CHECK(memcmp(p, backup+1, 32) == 0 && p[0x26] == 0xa5);
    for (i = 0; i < capacity; ++i) {
        node = p+0x38+i*20;
        for (j = 0; j < 8; j += 2) CHECK(native16(node+j) == ((i+j+0x8001)&65535));
        CHECK(native32(node+8) == UINT32_C(0xffffffff));
        CHECK(native32(node+12) == i*11);
        CHECK(native32(node+16) == UINT32_C(0xa5a58001)+i);
    }
    /* Invalid native data must also fail atomically when encoding. */
    CHECK(!mac_cache_syntax_convert(p, size, capacity-1, 1));
    CHECK(mac_cache_syntax_convert(p, size, capacity, 1));
    CHECK(memcmp(allocation, backup, size+2) == 0);
    for (i = 0; i < sizeof(bad_offsets)/sizeof(bad_offsets[0]); ++i) {
        unsigned int at = bad_offsets[i];
        p[at] = at == 0x24 ? 0 : 0xff;
        p[at+1] = 0xff;
        CHECK(!mac_cache_syntax_convert(p, size, capacity, 0));
        /* Restore only deliberately corrupted bytes; all others must match. */
        p[at] = backup[1+at]; p[at+1] = backup[2+at];
        CHECK(memcmp(allocation, backup, size+2) == 0);
    }
    CHECK(!mac_cache_syntax_convert(p, size-1, capacity, 0));
    CHECK(!mac_cache_syntax_convert(p, size+1, capacity, 0));
    CHECK(!mac_cache_syntax_convert(p, 0x37, capacity, 0));
    CHECK(!mac_cache_syntax_convert(NULL, size, capacity, 0));
    CHECK(!mac_cache_syntax_convert(p, size, 65535, 0));
    CHECK(!mac_cache_syntax_convert(p, size, capacity, 2));
    CHECK(memcmp(allocation, backup, size+2) == 0);
    free(allocation); free(backup);
}
static void addresses(void)
{
    size_t offset = 123;
    CHECK(mac_cache_address_offset(0x803a6024U, 0x803a6000U, 0x100, 0x20, &offset) && offset == 0x24);
    CHECK(mac_cache_address_offset(0x803a60e0U, 0x803a6000U, 0x100, 0x20, &offset) && offset == 0xe0);
    CHECK(!mac_cache_address_offset(0x803a6100U, 0x803a6000U, 0x100, 1, &offset));
    CHECK(!mac_cache_address_offset(0x803a5fffU, 0x803a6000U, 0x100, 1, &offset));
    CHECK(!mac_cache_address_offset(0xffffffffU, 0x803a6000U, 0x100, 32, &offset));
    CHECK(!mac_cache_address_offset(0x803a6000U, 0x803a6000U, 0x100, (size_t)-1, &offset));
    CHECK(!mac_cache_address_offset(0x803a6000U, 0x803a6000U, 0x100, 0, &offset));
    CHECK(!mac_cache_address_offset(0xfffffff8U, 0xfffffff0U, 0x100, 0x20, &offset));
    CHECK(offset == 0xe0);
    CHECK(mac_cache_address_offset(0xfffffff8U, 0xfffffff0U, 0x10, 8, &offset) && offset == 8);
}
int main(int argc, char **argv)
{
    records(); scripts(); addresses();
    if (argc == 2) {
        unsigned char disk[0x800] = {0}, native[0x800];
        FILE *f = fopen(argv[1], "rb");
        CHECK(f != NULL);
        if (f) {
            CHECK(fread(disk, 1, sizeof(disk), f) == sizeof(disk));
            fclose(f); memcpy(native, disk, sizeof(disk));
            CHECK(mac_cache_record_convert(native, sizeof(native), MAC_CACHE_HEADER, 0));
            printf("Existing map header: version=%lu tag_offset=%lu tag_bytes=%lu (recognition only)\n",
                (unsigned long)native32(native+4), (unsigned long)native32(native+0x10), (unsigned long)native32(native+0x14));
            CHECK(native32(native) == UINT32_C(0x68656164));
            CHECK(native32(native+0x7fc) == UINT32_C(0x666f6f74));
            CHECK(mac_cache_record_convert(native, sizeof(native), MAC_CACHE_HEADER, 1));
            CHECK(memcmp(native, disk, sizeof(disk)) == 0);
        }
    }
    printf("Cache endian: %lu checks, %lu failures; pointer=%lu bits\n", checks, failures, (unsigned long)(sizeof(void*)*8));
    return failures ? 1 : 0;
}
