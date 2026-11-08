/* Exercise the real Wasm allocator and map-window implementation. */
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "web_memory.h"

/* mcc_memory only needs platform declarations on native builds. */
#define __HALO_LINUX_PLATFORM_H
#include "../../linux/src/mcc_memory.c"

static int ce_enabled;
static unsigned settings_reads;
int config_boolean(const char *name)
{
    if (strcmp(name, "game.custom_edition")) abort();
    ++settings_reads;
    return ce_enabled;
}

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "memory check failed at %d\n", __LINE__); return 1; } } while (0)
#define WORKERS 4
#define BLOCKS 40
#define BLOCK_BYTES (32u * 1024u * 1024u)
static void *blocks[WORKERS][BLOCKS];
static unsigned counts[WORKERS];
/* Keep the deliberate failed allocation observable even when LLVM can elide
 * an unused malloc/free pair. */
static void *(*volatile allocate_bytes)(size_t) = malloc;

static void *allocate(void *argument)
{
    unsigned worker = (unsigned)(uintptr_t)argument;
    unsigned i;
    for (i = 0; i < BLOCKS; ++i) {
        unsigned char *p = malloc(BLOCK_BYTES);
        if (!p) break;
        blocks[worker][i] = p;
        ++counts[worker];
        p[0] = (unsigned char)worker;
        p[BLOCK_BYTES - 1] = (unsigned char)i;
    }
    return NULL;
}

int main(void)
{
    unsigned char *ce = (void *)CUSTOM_EDITION_TAG_CACHE_ADDRESS;
    unsigned char *xbox = (void *)0x80000000u;
    unsigned char *tags;
    pthread_t workers[WORKERS];
    unsigned i, j, count = 0;
    void *before;

    CHECK(!settings_reads && !web_custom_edition_tag_cache());
    ce_enabled = 1;
    web_memory_initialize();
    CHECK(settings_reads == 1);
    if (web_memory_bytes() < 0x88000000u) {
        CHECK(!web_custom_edition_tag_cache());
        CHECK(!mcc_memory_reserve(0x50000000u, 0x04000000u) && errno == ENOMEM);
        puts("PASS: insufficient map-window memory refused");
        return 0;
    }
    CHECK(web_custom_edition_tag_cache() == ce);
    ce_enabled = 0;
    web_memory_initialize();
    CHECK(!web_custom_edition_tag_cache());
    ce_enabled = 1;
    web_memory_initialize();
    CHECK(web_custom_edition_tag_cache() == ce);
    ce[0] = 0xA5;
    ce[CUSTOM_EDITION_TAG_CACHE_BYTES - 1] = 0x5A;
    xbox[0] = 0x3C;
    xbox[0x08000000u - 1] = 0xC3;
    CHECK(!mcc_memory_reserve(0x40440000u, 0x04000000u) && errno == EINVAL);
    CHECK(!mcc_memory_reserve(0x50000001u, 0x04000000u) && errno == EINVAL);
    CHECK(!mcc_memory_reserve(0x50000000u, 0x04000001u) && errno == EINVAL);
    CHECK(!mcc_memory_reserve(0x71000000u, 0x04000000u) && errno == EINVAL);
    tags = mcc_memory_reserve(0x50000000u, 0x04000000u);
    CHECK(tags == (void *)0x50000000u && !tags[0] && !tags[0x03FFFFFF]);
    tags[0] = 77;
    tags[0x03FFFFFF] = 88;
    CHECK(!mcc_memory_reserve(0x60000000u, 0x04000000u) && errno == EBUSY);
    mcc_memory_release(tags, 1);
    mcc_memory_release((void *)0x60000000u, 0x04000000u);
    mcc_memory_release(NULL, 0);
    CHECK(!mcc_memory_reserve(0x50000000u, 0x04000000u) && errno == EBUSY);
    mcc_memory_release(tags, 0x04000000u);
    tags = mcc_memory_reserve(0x50000000u, 0x04000000u);
    CHECK(tags && !tags[0] && !tags[0x03FFFFFF]);
    mcc_memory_release(tags, 0x04000000u);
    tags = mcc_memory_reserve(0x70000000u, 0x04000000u);
    CHECK(tags == (void *)0x70000000u);
    mcc_memory_release(tags, 0x04000000u);

    before = sbrk(0);
    CHECK(sbrk((intptr_t)(WEB_HEAP_LIMIT - (uintptr_t)before + 65536u)) == (void *)-1);
    CHECK(sbrk(0) == before);
    CHECK(!allocate_bytes(0x50000000u));
    for (i = 0; i < WORKERS; ++i)
        CHECK(!pthread_create(&workers[i], NULL, allocate, (void *)(uintptr_t)i));
    for (i = 0; i < WORKERS; ++i) CHECK(!pthread_join(workers[i], NULL));
    for (i = 0; i < WORKERS; ++i) {
        CHECK(counts[i] < BLOCKS);
        for (j = 0; j < counts[i]; ++j) {
            unsigned char *p = blocks[i][j];
            CHECK((uintptr_t)p < WEB_HEAP_LIMIT && BLOCK_BYTES <= WEB_HEAP_LIMIT - (uintptr_t)p);
            CHECK(p[0] == i && p[BLOCK_BYTES - 1] == j);
            free(p);
            ++count;
        }
    }
    CHECK(count >= 4);
    CHECK(ce[0] == 0xA5 && ce[CUSTOM_EDITION_TAG_CACHE_BYTES - 1] == 0x5A);
    CHECK(xbox[0] == 0x3C && xbox[0x08000000u - 1] == 0xC3);
    printf("PASS: map windows preserved across %u concurrent heap blocks and exhaustion\n", count);
    return 0;
}
