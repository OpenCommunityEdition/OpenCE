#ifndef HALO_MAC_NATIVE_ACTOR_ASSETS_H
#define HALO_MAC_NATIVE_ACTOR_ASSETS_H
#include "native_tags.h"
#include "native_bitmap_tags.h"
struct mac_engine_world;
struct mac_actor_assets {
    struct mac_native_tags tags;
    uint32_t biped,weapon_count,weapons[4],published_count;
    /* Borrowed owner pointers after publication: registry alone frees them. */
    struct mac_native_bitmap_tag *bitmaps;
    unsigned bitmap_count,bitmap_capacity;
    int bitmap_cache_bound;
};
/* Atomic owned asset subset: multiplayer biped and its initial weapons, their
   world models with real buffers, collision models, optional animation graphs,
   light attachments, trigger projectile roots and their contrail attachments.
   Projectile rendering/physics dependencies beyond attachments are not loaded.
   Includes weapon first-person models and animation graphs.
   Excludes shader/effect/lens/HUD payloads and dialogue sounds;
   does not establish gameplay dependency closure or invoke object_new.
   Output must be empty, and remains empty on any failure. Does not bind global
   tag_get; caller binds only after success and unbinds before disposal. */
int mac_actor_assets_load(const struct mac_retail_cache *,struct mac_actor_assets *,const char **);
/* The title map's: an empty registry (no player biped or weapons). */
int mac_actor_assets_load_title_music(const struct mac_retail_cache *,struct mac_actor_assets *,const char **);
int mac_actor_assets_load_empty(const struct mac_retail_cache *,struct mac_actor_assets *,const char **);
/* Additionally owns contrail primary/secondary bitmap resources. The full
   external bitmaps.map buffer may be released after successful loading. */
int mac_actor_assets_load_with_bitmaps(const struct mac_retail_cache *,const void *,size_t,
    struct mac_actor_assets *,const char **);
/* Add an owned referenced bitmap before binding the texture cache. */
int mac_actor_assets_load_bitmap(const struct mac_retail_cache *,const void *,size_t,
    struct mac_actor_assets *,uint32_t,const char **);
/* Adds a scenery root with optional native model/collision/animation and
   supported attachments. Looping-sound definitions are converted and
   published, but their sound payloads are not loaded and playback is not
   established. On failure, already published dependencies remain owned by
   assets; dispose the batch before retrying. */
int mac_actor_assets_load_scenery(const struct mac_retail_cache *,struct mac_actor_assets *,
    uint32_t,const char **);
/* Constructor dependencies for a sound-scenery root. Sound payloads remain
   unloaded; this does not enable sound playback. */
int mac_actor_assets_load_sound_scenery(const struct mac_retail_cache *,struct mac_actor_assets *,
    uint32_t,const char **);
int mac_actor_assets_load_light_fixture(const struct mac_retail_cache *cache,
    struct mac_actor_assets *assets,uint32_t handle,const char **error);
int mac_actor_assets_load_machine(const struct mac_retail_cache *cache,
    struct mac_actor_assets *assets,uint32_t handle,const char **error);
int mac_actor_assets_load_control(const struct mac_retail_cache *cache,
    struct mac_actor_assets *assets,uint32_t handle,const char **error);
/* Adds one complete native vehicle definition and its constructor/physics
   dependencies to an already-created asset registry. Existing native tags
   remain owned by the batch on failure; dispose the batch before retrying. */
int mac_actor_assets_load_vehicle(const struct mac_retail_cache *,struct mac_actor_assets *,
    uint32_t,const char **);
/* Vehicle roots with antenna bitmap references need the separate external
   bitmaps.map bytes supplied explicitly. */
int mac_actor_assets_load_vehicle_with_bitmaps(const struct mac_retail_cache *,const void *,size_t,
    struct mac_actor_assets *,uint32_t,const char **);
/* Loads the vehicle's looping-sound dependencies from the actual external
   sounds.map bytes. Samples are copied into native owned tag storage; this
   does not decode or play audio. */
int mac_actor_assets_load_vehicle_with_resources(const struct mac_retail_cache *,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *,uint32_t,const char **);
/* Loads the actual BSP background-sound and sound-environment palettes and
   closes referenced sound definitions from the supplied sounds.map bytes. */
int mac_actor_assets_load_looping_sound_dependencies(const struct mac_retail_cache *,
    const void *sounds,size_t sound_bytes,struct mac_actor_assets *,const char **error);
int mac_actor_assets_load_world_sounds(const struct mac_retail_cache *,
    const struct mac_engine_world *,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *,const char **);
/* Shared owned model/animation loading for native world assets. Model requires
   a real handle; animation accepts NONE for an absent optional graph. */
int mac_actor_assets_load_model(const struct mac_retail_cache *,struct mac_actor_assets *,uint32_t,const char **);
int mac_actor_assets_load_animation(const struct mac_retail_cache *,struct mac_actor_assets *,uint32_t,const char **);
/* Loads equipment model/collision/animation, supported attachments and direct
   sound definitions. Pickup effects and material shaders remain separate.
   Failed batches retain already published dependencies; dispose before retry. */
int mac_actor_assets_load_equipment_with_sounds(const struct mac_retail_cache *,
    const void *sounds,size_t sound_bytes,struct mac_actor_assets *,uint32_t,const char **);
/* Resolves collection permutations to native weapons/equipment before publishing
   the collection. This is asset ownership, not item spawning or pickup execution. */
int mac_actor_assets_load_item_collection_with_sounds(const struct mac_retail_cache *,
    const void *sounds,size_t sound_bytes,struct mac_actor_assets *,uint32_t,const char **);
/* Also resolves effect/particle/contrail attachments on items and their trigger
   projectiles, using actual external pixel and sound resources. */
int mac_actor_assets_load_item_collection_with_resources(const struct mac_retail_cache *,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *,uint32_t,const char **);
int mac_actor_assets_bind_texture_cache(struct mac_actor_assets *);
void mac_actor_assets_unbind_texture_cache(struct mac_actor_assets *);
void mac_actor_assets_free(struct mac_actor_assets *);
/* Load a globals-listed weapon, including model/animation, trigger and
   effect dependencies, before binding the immutable session cache. */
int mac_actor_assets_load_weapon_with_resources(const struct mac_retail_cache *,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *,uint32_t,const char **);
/* Loads a biped's source-declared effect, material-effect and sound closure
   using the real external asset bytes. Kept separate from the legacy compact
   object fixture loader, whose callers may not have external resource files. */
int mac_actor_assets_load_projectile_with_resources(const struct mac_retail_cache *,
    const void *,size_t,const void *,size_t,struct mac_actor_assets *,uint32_t,const char **);
int mac_actor_assets_load_biped_with_resources(const struct mac_retail_cache *,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *,uint32_t,const char **);
/* A biped a campaign scenario places, with the weapons it starts
   with. */
int mac_actor_assets_load_placed_biped(const struct mac_retail_cache *,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *,uint32_t,const char **);
/* An actor variant the AI places (campaign): its actor, unit, weapon, equipment
   and upgrade variant, loaded as for placed bipeds and weapons. */
int mac_actor_assets_load_actor_variant(const struct mac_retail_cache *,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *,uint32_t,const char **);
/* Debris an effect creates, with its effects, model and collision. */
int mac_actor_assets_load_garbage_with_resources(const struct mac_retail_cache *,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *,uint32_t,const char **);
#endif
