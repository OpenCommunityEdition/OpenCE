#ifndef HALO_MAC_RETAIL_COLLISION_MODEL_H
#define HALO_MAC_RETAIL_COLLISION_MODEL_H
#include "retail_collision.h"
struct mac_collision_effect { uint32_t group,handle; };
struct mac_collision_resistance {
    uint32_t flags;int16_t indirect_material,shield_material,shield_failure_function;
    float body_vitality,body_system_shock,friendly_resistance,area_threshold,body_damaged_threshold,body_destroyed_threshold;
    float shield_vitality,shield_failure_threshold,maximum_shield_failure,minimum_shield_stun_damage,shield_stun_time,shield_recharge_time,shield_damaged_threshold,shield_recharge_velocity;
    /* localized, area, body damaged/depleted/destroyed, shield damaged/depleted/recharging */
    struct mac_collision_effect effects[8];
};
struct mac_collision_material { char name[32];uint32_t flags;int16_t type;float leak,shield_multiplier,body_multiplier; };
struct mac_collision_region { char name[32];uint32_t flags;float threshold;struct mac_collision_effect effect;uint32_t permutation_count;char permutations[32][32]; };
struct mac_collision_sphere { int16_t node;float center[3],radius; };
struct mac_collision_model_node {
    char name[32];int16_t region,parent,sibling,child,runtime_damage_part;
    uint32_t bsp_count;struct mac_retail_collision *bsps;
};
struct mac_retail_collision_model {
    uint32_t handle;struct mac_collision_resistance resistance;float bounds[6];
    uint32_t material_count,region_count,sphere_count,node_count;
    struct mac_collision_material materials[32];struct mac_collision_region regions[32];
    struct mac_collision_sphere spheres[32];struct mac_collision_model_node nodes[64];
};
/* Owned native semantic coll data, not engine layouts. All non-padding root,
   resistance/material/region/node/sphere fields and all eight BSP arrays are
   decoded. Unused/editor padding is omitted. Nonempty obsolete modifiers are
   unsupported and rejected (no element layout recovered in this source).
   Referenced effect payloads remain external; salted handles/groups validated.
   Cache must be a validated immutable mac_retail_open view. Output empty on
   failure. Free successful output before reuse. Total BSP bytes capped64MiB. */
int mac_retail_collision_model_decode(const struct mac_retail_cache *,uint32_t handle,
    struct mac_retail_collision_model *,const char **error);
void mac_retail_collision_model_free(struct mac_retail_collision_model *);
#endif
