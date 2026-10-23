/* CE textures can exceed the Android Xbox window's current-frame capacity.
 * Keep individual allocations stable rather than repeatedly moving/reloading
 * a packed arena. This cache owns only synchronous, cached CE map pixels;
 * dynamic textures and other map storage keep their established paths. */
#ifdef HALO_ANDROID
#include "cseries.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "android_ce_texture_cache.h"
#include <stdlib.h>
#include <string.h>

/* cseries redirects free to debug_free, but calloc is the runtime allocator.
 * These cache-owned allocations must return to the matching runtime heap. */
#undef free

#define ANDROID_CE_TEXTURE_BYTES (128UL * 1024UL * 1024UL)
#define ANDROID_CE_TEXTURE_ENTRIES 8192
#define ANDROID_CE_TEXTURE_BUCKETS 1024

struct android_ce_texture
{
	struct android_ce_texture *bitmap_next, *resource_next;
	struct android_ce_texture *older, *newer;
	struct bitmap_data *bitmap;
	unsigned long header[5];
	void *pixels;
	unsigned long bytes, frame;
};

static struct android_ce_texture *bitmap_buckets[ANDROID_CE_TEXTURE_BUCKETS];
static struct android_ce_texture *resource_buckets[ANDROID_CE_TEXTURE_BUCKETS];
static struct android_ce_texture *oldest, *newest;
static unsigned long cache_bytes, cache_count, cache_frame;
static unsigned long cache_loads, cache_evictions, cache_failed;
static unsigned long cache_peak;
extern void platform_log(const char *format, ...);
extern void halo_custom_edition_texels_channels(const void *pixels, unsigned char order);

static unsigned long android_ce_texture_bucket(const void *pointer)
{
	return (((unsigned long)pointer >> 4) * 2654435761UL) % ANDROID_CE_TEXTURE_BUCKETS;
}

static void android_ce_texture_report(void)
{
	/* One summary per unload/flush; no per-frame logging or pixel readback. */
	platform_log("textures: Android CE cache peak=%lu limit=%lu loads=%lu evictions=%lu failures=%lu",
		cache_peak, ANDROID_CE_TEXTURE_BYTES, cache_loads, cache_evictions, cache_failed);
}

static void android_ce_texture_failed(void)
{
	if (!cache_failed++)
		platform_log("textures: Android CE cache allocation failed (used=%lu limit=%lu entries=%lu)",
			cache_bytes, ANDROID_CE_TEXTURE_BYTES, cache_count);
}

static void android_ce_texture_unlink(struct android_ce_texture *entry)
{
	if (entry->older)
		entry->older->newer = entry->newer;
	else
		oldest = entry->newer;
	if (entry->newer)
		entry->newer->older = entry->older;
	else
		newest = entry->older;
}

static void android_ce_texture_touch(struct android_ce_texture *entry)
{
	if (entry != newest)
	{
		android_ce_texture_unlink(entry);
		entry->older = newest;
		entry->newer = NULL;
		if (newest)
			newest->newer = entry;
		else
			oldest = entry;
		newest = entry;
	}
	entry->frame = cache_frame;
}

static void android_ce_texture_release(struct android_ce_texture *entry)
{
	struct android_ce_texture **link;

	/* Remove bindings and GPU copies before either address can be reused. */
	android_ce_texture_unbind(entry->header);
	android_ce_texture_gpu_forget(entry->header);
	halo_custom_edition_texels_channels(entry->pixels, 0);
	link = &bitmap_buckets[android_ce_texture_bucket(entry->bitmap)];
	while (*link != entry)
		link = &(*link)->bitmap_next;
	*link = entry->bitmap_next;
	link = &resource_buckets[android_ce_texture_bucket(entry->header)];
	while (*link != entry)
		link = &(*link)->resource_next;
	*link = entry->resource_next;
	android_ce_texture_unlink(entry);
	if (entry->bitmap->base_address == entry->pixels)
		entry->bitmap->base_address = NULL;
	cache_bytes -= entry->bytes;
	cache_count--;
	free(entry->pixels);
	free(entry);
}

void android_ce_texture_delete(struct bitmap_data *bitmap)
{
	struct android_ce_texture *entry;

	for (entry = bitmap_buckets[android_ce_texture_bucket(bitmap)]; entry; entry = entry->bitmap_next)
		if (entry->bitmap == bitmap)
		{
			android_ce_texture_release(entry);
			return;
		}
}

int android_ce_texture_resident(const struct bitmap_data *bitmap)
{
	struct android_ce_texture *entry;
	for (entry = bitmap_buckets[android_ce_texture_bucket(bitmap)]; entry; entry = entry->bitmap_next)
		if (entry->bitmap == bitmap)
			return 1;
	return 0;
}

void android_ce_texture_reset(void)
{
	if (cache_count || cache_failed)
		android_ce_texture_report();
	while (oldest)
		android_ce_texture_release(oldest);
	cache_frame = cache_loads = cache_evictions = cache_failed = 0;
	cache_peak = 0;
}

void android_ce_texture_idle(void)
{
	/* Equality alone defines current-frame ownership. Avoid wrap aliasing. */
	if (++cache_frame == 0)
	{
		struct android_ce_texture *entry;
		for (entry = oldest; entry; entry = entry->newer)
			entry->frame = 0;
		cache_frame = 1;
	}
}

const void *android_ce_texture_pixels(const void *resource, unsigned long *bytes)
{
	struct android_ce_texture *entry;

	for (entry = resource_buckets[android_ce_texture_bucket(resource)]; entry; entry = entry->resource_next)
		if (entry->header == resource)
		{
			*bytes = entry->bytes;
			return entry->pixels;
		}
	*bytes = 0;
	return NULL;
}

void *android_ce_texture_get(struct bitmap_data *bitmap, unsigned long bytes,
	int load, android_ce_texture_load_proc read)
{
	struct android_ce_texture *entry;
	unsigned long bucket;

	if (!bitmap)
		return NULL;
	bucket = android_ce_texture_bucket(bitmap);
	for (entry = bitmap_buckets[bucket]; entry; entry = entry->bitmap_next)
		if (entry->bitmap == bitmap)
		{
			android_ce_texture_touch(entry);
			return entry->header;
		}
	if (!load)
		return NULL;
	if (!bytes || bytes > ANDROID_CE_TEXTURE_BYTES || !read)
	{
		android_ce_texture_failed();
		return NULL;
	}
	/* LRU order makes room without moving survivors. Preserve every entry
	 * touched this frame, including headers already bound by earlier stages. */
	while (oldest && oldest->frame != cache_frame &&
		(cache_bytes > ANDROID_CE_TEXTURE_BYTES - bytes || cache_count == ANDROID_CE_TEXTURE_ENTRIES))
	{
		android_ce_texture_release(oldest);
		cache_evictions++;
	}
	if (cache_bytes > ANDROID_CE_TEXTURE_BYTES - bytes || cache_count == ANDROID_CE_TEXTURE_ENTRIES)
	{
		android_ce_texture_failed();
		return NULL;
	}
	entry = calloc(1, sizeof(*entry));
	if (!entry)
	{
		android_ce_texture_failed();
		return NULL;
	}
	entry->pixels = calloc(1, bytes);
	if (!entry->pixels)
	{
		free(entry);
		android_ce_texture_failed();
		return NULL;
	}
	entry->bitmap = bitmap;
	entry->bytes = bytes;
	bitmap->base_address = entry->pixels;
	read(bitmap, entry->header);
	/* A heap pointer is not an Xbox physical address. The renderer resolves
	 * this header through the registry, with an allocation-length check. */
	entry->header[1] = (unsigned long)entry->pixels;
	entry->bitmap_next = bitmap_buckets[bucket];
	bitmap_buckets[bucket] = entry;
	bucket = android_ce_texture_bucket(entry->header);
	entry->resource_next = resource_buckets[bucket];
	resource_buckets[bucket] = entry;
	entry->older = newest;
	if (newest)
		newest->newer = entry;
	else
		oldest = entry;
	newest = entry;
	entry->frame = cache_frame;
	cache_bytes += bytes;
	cache_count++;
	cache_loads++;
	if (cache_bytes > cache_peak)
		cache_peak = cache_bytes;
	return entry->header;
}
#endif
