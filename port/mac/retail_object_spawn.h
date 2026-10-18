#ifndef HALO_MAC_RETAIL_OBJECT_SPAWN_H
#define HALO_MAC_RETAIL_OBJECT_SPAWN_H
#include "retail_cache.h"
struct mac_spawn_reference { uint32_t group,handle; };
struct mac_spawn_object {
    uint32_t handle,runtime_flags;int16_t type,forced_shader;uint16_t flags;
    float radius,bounding_offset[3],origin_offset[3],acceleration_scale,render_radius;
    /* model, animation, collision, physics, modifier shader, creation effect */
    struct mac_spawn_reference refs[6];
    uint32_t attachment_count,function_count,color_count;
    struct mac_spawn_reference attachments[8];
};
struct mac_spawn_magazine {
    uint32_t flags;int16_t recharged,initial,total_max,loaded_max,reloaded;
    int16_t initial_loaded,initial_spare;float reload_time,chamber_time;
    struct mac_spawn_reference reload_effect,chamber_effect;
    uint32_t ammunition_count;struct mac_spawn_reference ammunition[8];
    int16_t ammunition_rounds[8];
};
struct mac_spawn_trigger {
    uint32_t flags;int16_t magazine,rounds_per_shot,minimum_loaded,tracer_rounds,projectiles_per_shot;
    float initial_rate,final_rate;struct mac_spawn_reference charging_effect,projectile;
};
struct mac_spawn_weapon {
    struct mac_spawn_object object;uint32_t flags;char label[32];
    struct mac_spawn_reference first_person_model,first_person_animation,hud;
    uint32_t magazine_count,trigger_count;
    struct mac_spawn_magazine magazines[2];struct mac_spawn_trigger triggers[2];
};
struct mac_retail_object_spawn {
    struct mac_spawn_object biped;uint32_t unit_flags,biped_flags;
    int16_t default_team,grenade_type,grenade_count,pelvis_node,head_node;
    float standing_height,crouching_height,radius,standing_camera,crouching_camera;
    uint32_t weapon_count;struct mac_spawn_weapon weapons[4];
};
/* Native constructor-input audit bundle, NOT a complete converted engine tag
   and never suitable for casting to object_definition/biped_definition.
   Decodes the fields documented above plus validates their salted references.
   Does not create objects, convert model/animation/effect payloads, or initialize
   engine pools/type registries. Output is zero on any failure. Cache must be
   a validated immutable mac_retail_open view. NONE weapon entries stay NONE. */
int mac_retail_object_spawn_decode(const struct mac_retail_cache *,
    struct mac_retail_object_spawn *,const char **error);
#endif
