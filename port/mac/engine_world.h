#ifndef HALO_MAC_ENGINE_WORLD_H
#define HALO_MAC_ENGINE_WORLD_H
#include "retail_cache.h"
#include "native_scenario_blocks.h"
struct structure_bsp;
struct mac_engine_world { void *state; };
struct mac_engine_location { int32_t leaf; int16_t cluster; uint16_t bonus; };
struct mac_engine_world_info {
    uint32_t leaves,clusters,spawns,collision_materials;
    uint32_t fog_planes,fog_regions,fog_palette,runtime_decals;
    uint32_t scenario_decal_palette,clusters_with_fog,clusters_with_decals;
    uint32_t netgame_flags,netgame_equipment,starting_equipment,sky_references;
};
/* Scoped native scenario/BSP subset. Owns all eight collision arrays,
   real structure leaf records and cluster palette metadata, and spawn records.
   Optional graph conversion adds portal/PVS visibility. Enabling surfaces
   also builds legacy per-cluster material-group surface lists from real leaf
   references; unconverted render/AI fields must not be used. Constructor tests
   supply separate native actor services.
   Creation requires a validated immutable mac_retail_open cache.
   Free before reuse. All calls run on the engine thread, without reentrancy. */
int mac_engine_world_create(const struct mac_retail_cache *,uint32_t bsp,
    struct mac_engine_world *,const char **error);
void mac_engine_world_free(struct mac_engine_world *);
/* Borrow the converted native BSP structure while its world owner is live. */
struct structure_bsp *mac_engine_world_structure(struct mac_engine_world *);
int mac_engine_world_info(const struct mac_engine_world *,struct mac_engine_world_info *);
/* Owned copy of the BSP's byte-encoded triangular cluster sound-distance
   matrix. The pointer remains valid until mac_engine_world_free. */
int mac_engine_world_sound_cluster_data(const struct mac_engine_world *,const unsigned char **,uint32_t *);
/* True only for spans owned by this world's converted scenario syntax/string
   storage. It never falls back to source-cache bytes. */
int mac_engine_world_scenario_cache_contains(const struct mac_engine_world *,const void *,size_t);
/* Return one validated native scenario sky reference, preserving NONE. This
   does not load or certify the referenced sky definition or render assets. */
int mac_engine_world_scenario_hud_messages(const struct mac_engine_world *,uint32_t *handle);
/* The weapons the starting profiles name (campaign levels). */
uint32_t mac_engine_world_profile_weapons(const struct mac_engine_world *,uint32_t *handles,uint32_t capacity);
/* The tags the AI blocks name (group and handle pairs; see
   mac_native_scenario_gameplay_ai_references). */
uint32_t mac_engine_world_ai_references(const struct mac_engine_world *,uint32_t *groups,
    uint32_t *handles,uint32_t capacity,uint32_t *total);
/* The tags the level's scripts name, as group and handle (campaign levels). */
int mac_engine_world_script_references(const struct mac_engine_world *,
    uint32_t *groups,uint32_t *handles,uint32_t capacity,uint32_t *count);
int mac_engine_world_sky_handle(const struct mac_engine_world *,uint32_t index,uint32_t *handle);
/* True when the native world data needed by the bounded object-update decal
   and collision-fog paths is represented. This does not prove external decal
   definitions, bitmaps, or runtime decal services are initialized. */
int mac_engine_world_object_update_supported(const struct mac_engine_world *);
/* Return the scenario decal palette's native cache handles for an external
   owner to resolve while the source cache is available. */
int mac_engine_world_decal_handles(const struct mac_engine_world *,uint32_t *handles,
    uint32_t capacity,uint32_t *count);
/* Enumerate one validated BSP sound palette (lsnd or snde) for an external
   native-tag owner. Entries retain palette order, including NONE handles. */
int mac_engine_world_sound_handles(const struct mac_engine_world *,uint32_t group,
    uint32_t *handles,uint32_t capacity,uint32_t *count);
/* the fog tags the BSP's fog palette names (NONE for an empty entry) */
int mac_engine_world_fog_handles(const struct mac_engine_world *world,uint32_t *handles,uint32_t capacity,uint32_t *count);
/* Real structure_bsp lightmap-group cache handle, or UINT32_MAX when the BSP
   has no lightmap group. Resolve/load its bitmap assets while cache is alive. */
uint32_t mac_engine_world_lightmap_handle(const struct mac_engine_world *);
/* Material shader references from converted native lightmap/material arrays.
   Repeated handles are retained in material order; surfaces must be enabled. */
int mac_engine_world_material_shader_handles(const struct mac_engine_world *,uint32_t *,uint32_t,uint32_t *);
/* Read one reference from an attached scenario palette through the engine's
   native TAG_BLOCK layout. This returns a handle only; it does not load or
   certify the referenced definition's dependencies. */
int mac_engine_world_object_palette_handle(const struct mac_engine_world *,
    uint32_t family,uint32_t index,uint32_t *handle);
int mac_engine_world_object_palette_count(const struct mac_engine_world *,
    uint32_t family,uint32_t *count);
/* Opt-in bounded native surface/lightmap subset for surface lookup, camera
   visibility lists, and lightmap sampling: references, surfaces, lightmap/material ranges,
   shader/permutation identifiers, material centroid/plane/lighting, and packed
   environment/lightmap vertices. Cached point lights and breakable materials
   are rejected until their runtime owners are available. GPU resources are
   attached separately after the renderer is live. Call while the source cache remains
   mapped. A failed conversion leaves partial owned allocations; free the
   world before reuse. */
int mac_engine_world_enable_surfaces(struct mac_engine_world *,const struct mac_retail_cache *,const char **);
/* Scoped actual globals, restored after actual scenario_location_from_point.
   Solid/outside gives -1/-1; invalid query returns zero without engine access. */
int mac_engine_world_location(const struct mac_engine_world *,const float point[3],struct mac_engine_location *);
int mac_engine_world_spawn(const struct mac_engine_world *,uint32_t index,float point[3],float *facing,struct mac_engine_location *);
/* Opt-in native portal graph and PVS rows for actual structures.c traversal.
   Must use the same immutable cache supplied to create. Supports <=64 clusters
   to keep the stock reconnect traversal within its fixed result capacity.
   Includes native cluster PVS and portal-index lists. Cluster surface lists
   are built by enable_surfaces from leaf references; cluster partition
   storage and attachment services have separate lifecycles. */
int mac_engine_world_enable_graph(struct mac_engine_world *,const struct mac_retail_cache *,const char **);
/* Engine-thread callback with actual world globals restored afterward.
   Requires graph enabled; callback must return normally, without longjmp. */
int mac_engine_world_scope(const struct mac_engine_world *,int (*operation)(void *),void *context);
/* Campaign scenarios with more than one structure BSP (scenario_switch_structure_bsp).
   The other BSPs are converted beside the session's own, as it is (create_other_bsps,
   after the session's world has its graph and surfaces); their vertex buffers are made
   when one is switched to. A BSP's assets (shaders, lightmaps, fog, sounds) are loaded
   through other_bsp, which gives a world handle for the BSP at that index. */
int mac_engine_world_create_other_bsps(struct mac_engine_world *,const struct mac_retail_cache *,const char **);
uint32_t mac_engine_world_bsp_count(const struct mac_engine_world *);
int mac_engine_world_other_bsp(const struct mac_engine_world *,uint32_t index,struct mac_engine_world *out);
struct structure_bsp *mac_engine_world_structure_for_handle(long handle);
struct scenario_structure_bsp_reference;
struct structure_bsp *mac_engine_world_structure_for_reference(const struct scenario_structure_bsp_reference *,int make_active);
#endif
