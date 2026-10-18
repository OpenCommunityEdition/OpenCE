#ifndef HALO_MAC_NATIVE_OBJECT_TAGS_H
#define HALO_MAC_NATIVE_OBJECT_TAGS_H
#include "retail_cache.h"
struct mac_native_object_tag {
    void *root,*owner;
    uint32_t handle,group;
    uint32_t field_digest,converted_values;
    size_t root_bytes,bytes_owned;
};
/* Produces an actual native biped_definition, including inherited object/unit
   fields and every nested block. Dependency handles remain salted references;
   the registry must resolve them to separately converted tags before use.
   Complete storage conversion is not full gameplay validation: run semantic
   tag validation before constructors consume cached indices or enum values. */
/* Complete engine object_definition for scenery and sound scenery. Retail
   trailing unused padding is not an engine field. References require separate
   dependency loading before placement constructors can consume these tags. */
int mac_native_scenery_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_sound_scenery_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_machine_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error);
int mac_native_control_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error);
int mac_native_light_fixture_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error);
int mac_native_biped_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
/* Complete vehicle_definition conversion, including inherited object/unit
   blocks and all four vehicle references. Runtime dependencies still need
   separate loading before vehicle placement or simulation. */
int mac_native_vehicle_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
/* Complete native weapon_definition with inherited object/item and nested
   magazine/ammunition/trigger/firing-effect blocks. Nonempty unused_block is
   unsupported and rejected rather than retaining an unconverted pointer. */
int mac_native_weapon_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
/* Complete projectile material responses and inherited object data; complete
   contrail states and effect shader (including secondary bitmap reference).
   These own storage, but callers still must load referenced dependencies. */
int mac_native_projectile_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_contrail_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
/* Complete owned effect and particle-system storage; semantic runtime
   validation and referenced assets must be resolved before execution. */
int mac_native_particle_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
/* Complete owned environment shader root; bitmap/lens dependencies must be
   loaded separately before lighting or rendering consumers run. */
/* Complete owned storage; render dependency validation remains separate. */
int mac_native_material_effects_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
int mac_native_damage_effect_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
int mac_native_continuous_damage_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
int mac_native_chicago_shader_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
int mac_native_model_shader_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_glass_shader_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_plasma_shader_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
int mac_native_water_shader_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error);
int mac_native_fog_decode(const struct mac_retail_cache *cache,uint32_t handle,
    struct mac_native_object_tag *out,const char **error);
int mac_native_meter_shader_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_environment_shader_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_effect_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_particle_system_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
/* Native equipment root and inherited object/item blocks; dependency handles
   must be resolved before constructors or pickup callbacks consume it. */
int mac_native_equipment_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
/* Actor and actor-variant tags (campaign AI). The variant's references to its
   actor, unit, weapon, equipment and upgrade still need separate loading. */
int mac_native_actor_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_actor_variant_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_garbage_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
/* Owned item-collection permutations for actual engine equipment selection.
   Weights must yield a positive signed-short total for random_item; referenced
   weapons/equipment still need separate native dependency loading. */
int mac_native_item_collection_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
/* Complete owned HUD number, unit-HUD and weapon-HUD records. Bitmap and
   sound references must be loaded separately before interface consumers. */
int mac_native_hud_number_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_unit_hud_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_weapon_hud_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_color_table_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
int mac_native_grenade_hud_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_lens_flare_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
int mac_native_light_volume_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
int mac_native_glow_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
int mac_native_flag_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
int mac_native_lightning_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
int mac_native_camera_track_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
void mac_native_object_owner_free(void *owner);
void mac_native_object_tag_free(struct mac_native_object_tag *);
/* Enumerates direct dependencies of an unchanged, successfully decoded root.
   Includes predicted-resource handles (group zero means unspecified), omits
   NONE, preserves duplicates, and stops when the visitor returns zero.
   This inventories loading work; it does not prove transitive readiness.
   The root must remain alive, including after ownership moves to a registry. */
typedef int (*mac_native_reference_visitor)(void *,uint32_t group,uint32_t handle);
int mac_native_object_visit_references(const struct mac_native_object_tag *,
    mac_native_reference_visitor,void *context);
#endif
