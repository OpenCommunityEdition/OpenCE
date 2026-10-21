/* Android CE's bounded, stable pixel storage, outside the Xbox window. */
#ifndef ANDROID_CE_TEXTURE_CACHE_H
#define ANDROID_CE_TEXTURE_CACHE_H

struct bitmap_data;
typedef void (*android_ce_texture_load_proc)(struct bitmap_data *, void *);
void *android_ce_texture_get(struct bitmap_data *bitmap, unsigned long bytes,
	int load, android_ce_texture_load_proc read);
void android_ce_texture_delete(struct bitmap_data *bitmap);
void android_ce_texture_reset(void);
void android_ce_texture_idle(void);
int android_ce_texture_resident(const struct bitmap_data *bitmap);

/* Only an exact registered header grants the renderer access to heap pixels.
 * Cached map pixels are filled synchronously before this header is published. */
const void *android_ce_texture_pixels(const void *resource, unsigned long *bytes);
void android_ce_texture_gpu_forget(const void *resource);
void android_ce_texture_unbind(const void *resource);

#endif
