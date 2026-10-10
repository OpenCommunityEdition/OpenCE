/* MCC-only runtime state. Nothing in this interface aliases a Custom Edition
 * cache, allocator, conversion flag, resource offset or map identity. */
#ifndef MCC_RUNTIME_H
#define MCC_RUNTIME_H

#include "mcc_cache_format.h"
struct bitmap_data;

struct mcc_runtime {
    unsigned char *tags;
    unsigned char *tag_index;
    uint32_t used;
    uint32_t capacity;
    struct mcc_cache_source source;
    struct mcc_cache_report report;
    /* Owned by the corresponding MCC adapter, released even on failed loads. */
    void *geometry;
    void *audio;
    void *bitmaps;
};

/* Only ranges actually loaded into tags are accessible here. BSP data is
 * checked against its own region by the geometry adapter. */
void *mcc_runtime_pointer(struct mcc_runtime *runtime, uint32_t address, uint32_t bytes);
int mcc_runtime_read(struct mcc_runtime *runtime, uint32_t offset, uint32_t bytes, void *out);

/* Appends aligned runtime data below the lowest BSP, never overwriting tags. */
void *mcc_runtime_allocate(struct mcc_runtime *runtime, uint32_t bytes);

int mcc_geometry_prepare(struct mcc_runtime *runtime);
void mcc_geometry_dispose(struct mcc_runtime *runtime);
int mcc_geometry_bsp_load(struct mcc_runtime *runtime, uint32_t handle,
    uint32_t offset, uint32_t size, void *address, void **structure);
void mcc_geometry_bsp_unload(struct mcc_runtime *runtime);
/* Exact MCC-owned hardware/payload identities, used by its tag validator. */
void *mcc_geometry_buffer_data(struct mcc_runtime *runtime, void const *hardware,
    int index_buffer, unsigned long *bytes);
int mcc_geometry_contains(struct mcc_runtime *runtime, void const *address, unsigned long bytes);
short mcc_geometry_part_palette(struct mcc_runtime *runtime, void const *vertex_buffer,
    unsigned char const **nodes);

int mcc_audio_prepare(struct mcc_runtime *runtime);
int mcc_audio_read(struct mcc_runtime *runtime, uint32_t offset, uint32_t bytes, void *out);
int mcc_audio_contains(struct mcc_runtime *runtime, uint32_t offset, uint32_t bytes);
void mcc_audio_dispose(struct mcc_runtime *runtime);
int mcc_bitmaps_prepare(struct mcc_runtime *runtime);
int mcc_hud_bitmaps_prepare(struct mcc_runtime *runtime);
int mcc_bitmaps_read(struct mcc_runtime *runtime, long tag, uint32_t offset,
    uint32_t bytes, void *out);
int mcc_bitmaps_contains(struct mcc_runtime *runtime, uint32_t offset, uint32_t bytes);
int mcc_bitmaps_valid(struct mcc_runtime *runtime, struct bitmap_data *bitmap);
void const *mcc_bitmaps_pixels(struct mcc_runtime *runtime,
    struct bitmap_data const *bitmap, uint32_t *bytes);
void mcc_bitmaps_dispose(struct mcc_runtime *runtime);

/* Returns the maximum exclusive virtual stream offset after conversion. */
uint32_t mcc_audio_stream_end(struct mcc_runtime *runtime);
uint32_t mcc_bitmaps_stream_end(struct mcc_runtime *runtime);

#endif
