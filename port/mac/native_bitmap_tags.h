#ifndef HALO_MAC_NATIVE_BITMAP_TAGS_H
#define HALO_MAC_NATIVE_BITMAP_TAGS_H
#include "retail_cache.h"
struct bitmap_group;
struct bitmap_data;
struct mac_native_tags;
struct mac_native_bitmap_tag {
    struct bitmap_group *root;
    void *owner;
    uint32_t handle,group,image_count,level_count,pixel_digest;
    /* Retained bytes at decode time; query memory_usage_get for live bytes. */
    size_t root_bytes,bytes_owned;
};
struct mac_native_bitmap_memory_usage {
    size_t total_owned_bytes;
    size_t encoded_mapped_bytes;
    size_t encoded_payload_bytes;
    size_t host_sample_bytes;
    size_t rgba_mip_bytes;
    size_t auxiliary_bytes;
};
/* Owned native root, sequences/sprites and resident POT 2D, 3D or six-face
   cubemap mip chains for supported formats. Encoded pixels and PPC's host
   sampling view are retained at decode; RGBA mip views are validated during
   decode and materialized on first getter use, then retained until owner free.
   Output must be empty. Unsupported image families fail; no raw-root fallback.
   Editor import metadata is retained without a runtime editor buffer.
   External pixels are
   the unmodified entire bitmaps.map; source storage can be released on success.
   The D3D descriptor points to unchanged encoded Xbox bytes. PPC's
   bitmap_data.base_address is a separate host-endian sampling view; decoded
   RGBA levels (including slice-major volume mip views) are the native GL
   upload view. Cubemap face selection is explicit. */
int mac_native_bitmap_decode(const struct mac_retail_cache *,uint32_t,
    const void *external_pixels,size_t external_bytes,struct mac_native_bitmap_tag *,const char **);
void mac_native_bitmap_owner_free(void *);
void mac_native_bitmap_tag_free(struct mac_native_bitmap_tag *);
/* Reports currently retained owner storage by representation. Lazy RGBA
   views increase both this value and the native tag registry total. */
/* The bitmap tag's path, as the cache names it ("" if unknown). Lives with the owner. */
const char *mac_native_bitmap_owner_name(const void *owner);
int mac_native_bitmap_memory_usage_get(void *owner,
    struct mac_native_bitmap_memory_usage *out);
/* Install a stable registry-state copy before publishing this owner. */
int mac_native_bitmap_owner_accounting_bind(void *owner,
    const struct mac_native_tags *,uint32_t handle);
void *mac_native_bitmap_hardware(void *owner,const struct bitmap_data *);
const unsigned char *mac_native_bitmap_rgba(void *owner,uint32_t image,uint32_t mip,
    uint32_t *width,uint32_t *height,size_t *bytes);
const unsigned char *mac_native_bitmap_rgba_face(void *owner,uint32_t image,
    uint32_t face,uint32_t mip,uint32_t *width,uint32_t *height,size_t *bytes);
/* Original, unmodified little-endian DXT1/3/5 block bytes for a resident
   2D image or cubemap face/mip. The returned pointer lives with the owner. */
const unsigned char *mac_native_bitmap_encoded_face(void *owner,uint32_t image,
    uint32_t face,uint32_t mip,uint32_t *format,uint32_t *width,
    uint32_t *height,size_t *bytes);
/* Owned slice-major RGBA view for a retail 3D bitmap mip. */
const unsigned char *mac_native_bitmap_rgba_volume(void *owner,uint32_t image,
    uint32_t mip,uint32_t *width,uint32_t *height,uint32_t *depth,size_t *bytes);
/* Resolve a live borrowed native bitmap hardware object by exact identity.
   The owner is cache-local and must remain bound/alive for the returned view.
   Materialization and owner teardown are serialized on the engine thread;
   returned RGBA pointers remain stable until owner teardown. */
const unsigned char *mac_native_bitmap_rgba_for_hardware(void *owner,
    const void *hardware,uint32_t mip,uint32_t *image,uint32_t *width,
    uint32_t *height,size_t *bytes);
const unsigned char *mac_native_bitmap_rgba_volume_for_hardware(void *owner,
    const void *hardware,uint32_t mip,uint32_t *width,uint32_t *height,
    uint32_t *depth,size_t *bytes);
/* Explicit resident-cache scope; owners must outlive the binding and must be
   unbound before disposal. Engine-thread only. Unknown bitmap access fails. */
int mac_native_bitmap_cache_bind(struct mac_native_bitmap_tag *,size_t count);
void mac_native_bitmap_cache_unbind(void);
int mac_native_bitmap_cache_contains(const void *hardware);
unsigned long mac_native_bitmap_cache_requests(void);
/* Changes on bind/unbind so GPU copies cannot outlive a map identity. */
unsigned long mac_native_bitmap_cache_generation(void);
const unsigned char *mac_native_bitmap_cache_rgba(const void *hardware,
    uint32_t mip,uint32_t *width,uint32_t *height,size_t *bytes);
/* Resolve a resident native bitmap texture's selected cubemap face and mip.
   Face zero is the +X face; remaining face indices follow D3DCUBEMAP_FACES. */
const unsigned char *mac_native_bitmap_cache_rgba_face(const void *hardware,
    uint32_t face,uint32_t mip,uint32_t *width,uint32_t *height,size_t *bytes);
/* Owner-checked original DXT payload for the exact resident hardware object. */
const unsigned char *mac_native_bitmap_cache_encoded_face(const void *hardware,
    uint32_t face,uint32_t mip,uint32_t *format,uint32_t *width,
    uint32_t *height,size_t *bytes);
/* Which bitmap a resident hardware object is: its tag's path, its index in the
   tag and its size, and whether it is a cube map. 0 if it is none of the bound
   bitmaps (or is a volume). For texture packs. */
int mac_native_bitmap_cache_identify(const void *hardware,const char **name,
    uint32_t *index,uint32_t *width,uint32_t *height,int *cube);
const unsigned char *mac_native_bitmap_cache_rgba_volume(const void *hardware,
    uint32_t mip,uint32_t *width,uint32_t *height,uint32_t *depth,size_t *bytes);
#endif
