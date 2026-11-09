#ifndef MCC_TEXTURE_CACHE_H
#define MCC_TEXTURE_CACHE_H
struct bitmap_data;
void *mcc_texture_cache_get(struct bitmap_data const *bitmap, boolean load);
void mcc_texture_cache_dispose(void);
#endif
