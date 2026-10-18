#ifndef HALO_MAC_RETAIL_ANIMATION_GRAPH_H
#define HALO_MAC_RETAIL_ANIMATION_GRAPH_H
#include "retail_animation.h"
struct mac_animation_indices { uint32_t count; int16_t *indices; };
struct mac_animation_ik { char marker_name[32],attached_to_marker_name[32]; };
struct mac_animation_aiming {
    float negative_yaw_delta,positive_yaw_delta;
    int16_t negative_yaw_frame_count,positive_yaw_frame_count;
    float negative_pitch_delta,positive_pitch_delta;
    int16_t negative_pitch_frame_count,positive_pitch_frame_count;
};
struct mac_animation_type { char label[32];struct mac_animation_indices animations; };
struct mac_animation_overlay { int16_t animation_index,function_index,mode; };
struct mac_vehicle_suspension { int16_t mass_point_index,animation_index;float unknown4,unknown8;unsigned char opaque[8]; };
struct mac_vehicle_animation { struct mac_animation_aiming steering;struct mac_animation_indices animations;uint32_t suspension_count;struct mac_vehicle_suspension *suspensions; };
struct mac_animation_class {
    char label[32],grip_marker_name[32],hand_marker_name[32];
    struct mac_animation_aiming aiming;
    struct mac_animation_indices animations;
    uint32_t ik_count,type_count;
    struct mac_animation_ik *ik_points;
    struct mac_animation_type *types;
};
struct mac_animation_seat {
    char label[32];struct mac_animation_aiming looking;
    struct mac_animation_indices animations;
    uint32_t ik_count,class_count;
    struct mac_animation_ik *ik_points;
    struct mac_animation_class *classes;
};
/* Owned selection graph + complete uncompressed clips and vehicle animation
   rows. Node transforms remain the skeleton decoder's responsibility. */
struct mac_retail_animation_graph {
    uint32_t handle,animation_count,node_count,seat_count,sound_count;
    struct mac_animation_seat *seats;
    uint32_t overlay_count;
    struct mac_animation_overlay *overlays;
    struct mac_retail_animation *animations;
    struct mac_animation_indices weapon_animations,unit_damage_animations,first_person_weapon_animations;
    uint32_t *sound_handles;
    uint32_t vehicle_animation_count;
    struct mac_vehicle_animation *vehicle_animations;
    float limp_body_node_collision_radius;
    uint16_t flags;
};
int mac_retail_animation_graph_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_retail_animation_graph *,const char **error);
void mac_retail_animation_graph_free(struct mac_retail_animation_graph *);
#endif
