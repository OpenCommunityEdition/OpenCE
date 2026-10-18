#ifndef HALO_MAC_NATIVE_TAGS_H
#define HALO_MAC_NATIVE_TAGS_H
#include "retail_cache.h"
#include <stdio.h>
/* Metadata is copied. Published roots must be COMPLETE engine-native tags,
   produced by a supported family decoder, never raw disk bytes or audit bundles.
   The registry owns published allocations and invokes destroy(owner) at disposal.
   Publishing is single-threaded during loading; lookups do not mutate entries.
   Creation requires an immutable cache successfully opened by mac_retail_open.
   Free an existing registry before creating another in the same wrapper. */
struct mac_native_tags { void *state; };
int mac_native_tag_group_matches(uint32_t actual_group,uint32_t requested_group);
int mac_native_tags_create(const struct mac_retail_cache *,struct mac_native_tags *,const char **);
void mac_native_tags_free(struct mac_native_tags *);
/* Transfers ownership only on success. Duplicate publication is refused.
   owned_bytes includes root and all allocations reachable from this owner. */
int mac_native_tags_publish(struct mac_native_tags *,uint32_t handle,uint32_t group,
    void *root,size_t root_bytes,size_t owned_bytes,void *owner,void (*destroy)(void *),const char **);
/* Only a fully converted retail mod2 model may use this entry point. The
   runtime root becomes mode; mod2 lookup cannot expose a different layout. */
int mac_native_tags_publish_model(struct mac_native_tags *,uint32_t handle,
    void *root,size_t root_bytes,size_t owned_bytes,void *owner,void (*destroy)(void *),const char **);
/* Only published roots count as loaded. Unknown/unconverted handles return NONE. */
uint32_t mac_native_tags_group(const struct mac_native_tags *,uint32_t handle);
/* Resolves an absolute cache-table index only when its native root has been
   published. Full salted handles remain required by all other registry APIs. */
uint32_t mac_native_tags_handle_at_index(const struct mac_native_tags *,uint32_t index);
uint32_t mac_native_tags_find(const struct mac_native_tags *,uint32_t group,const char *name);
/* Enumerates published roots in table order, including inherited groups.
   UINT32_MAX group means all; cursor is advanced and exhausted walks stay ended. */
uint32_t mac_native_tags_next(const struct mac_native_tags *,uint32_t group,uint32_t *cursor);
uint32_t mac_native_tags_find_metadata(const struct mac_native_tags *,uint32_t group,const char *name);
const void *mac_native_tags_get(const struct mac_native_tags *,uint32_t group,uint32_t handle,
    size_t minimum_root_bytes,const char **);
const char *mac_native_tags_name(const struct mac_native_tags *,uint32_t handle);
size_t mac_native_tags_owned_bytes(const struct mac_native_tags *);
/* Prints, by group, the map's tags with no native root (unconverted families). */
void mac_native_tags_report_unconverted(const struct mac_native_tags *,FILE *);
int mac_native_tags_checksum(const struct mac_native_tags *,uint32_t *out);
/* Account for retained allocations added to or removed from a published
   owner. Owners use this while materializing lazy, owner-retained views. */
int mac_native_tags_owner_bytes_reserve(struct mac_native_tags *,uint32_t handle,
    void *owner,size_t bytes,const char **error);
int mac_native_tags_owner_bytes_release(struct mac_native_tags *,uint32_t handle,
    void *owner,size_t bytes);
/* Tags that are not in the retail cache (the menus', built from XML). They
   follow the cache's tags: extend adds empty entries, define names one and
   gives it a root the registry does not own (nothing is freed or counted),
   set_root changes a defined entry's root, and truncate lets the entries
   from the count on go. */
uint32_t mac_native_tags_count(const struct mac_native_tags *);
int mac_native_tags_extend(struct mac_native_tags *,uint32_t count);
int mac_native_tags_define(struct mac_native_tags *,uint32_t index,uint32_t handle,uint32_t group,
    const char *name,void *root,size_t root_bytes);
int mac_native_tags_set_root(struct mac_native_tags *,uint32_t handle,void *root);
void mac_native_tags_truncate(struct mac_native_tags *,uint32_t count);
/* The root of a published tag whatever its group (NULL if none). */
void *mac_native_tags_root(const struct mac_native_tags *,uint32_t index,uint32_t *handle,
    uint32_t *group,const char **name);
#ifdef HALO_MAC_NATIVE_TAGS_TESTING
int mac_native_tags_test_set_budget(struct mac_native_tags *,size_t budget);
#endif
#endif
