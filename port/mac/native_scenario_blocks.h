#ifndef HALO_MAC_NATIVE_SCENARIO_BLOCKS_H
#define HALO_MAC_NATIVE_SCENARIO_BLOCKS_H
#include "native_scenario_objects.h"

/* Field-for-field native block set that can be copied into the corresponding
   struct scenario tag_block fields on 32-bit Mac. It is not a scenario root. */
struct mac_native_scenario_reference {
    uint32_t group_tag;
    char *name;
    int32_t name_length,index;
};
/* Exact retail struct scenario_object_palette_entry layout (48 bytes). */
struct mac_native_scenario_palette_entry {
    struct mac_native_scenario_reference reference;
    uint32_t unused[8];
};
struct mac_native_scenario_object_core {
    int16_t palette_entry_index,name_index;uint16_t placement_flags;int16_t variant_number;
    float position[3],rotation[3];uint16_t on_bsp_flags,misc_flags;uint32_t unused;
};
struct mac_native_scenario_permutation {
    uint32_t change_colors[4];uint8_t region_permutations[8];uint32_t unused[2];
};
struct mac_native_scenario_vehicle_record {
    struct mac_native_scenario_object_core object;
    struct mac_native_scenario_permutation permutation;
    float body_vitality;uint32_t flags;uint8_t unit_unused[8];
    int8_t multiplayer_team_index;uint8_t multiplayer_spawn_flags_pad;
    uint16_t multiplayer_spawn_flags;uint8_t vehicle_unused[0x1c];
};
struct mac_native_scenario_native_family {
    int32_t placement_count;
    void *placements;
    int32_t palette_count;
    struct mac_native_scenario_palette_entry *palette;
};
struct mac_native_scenario_blocks {
    struct mac_native_scenario_name *object_names;
    int32_t object_name_count;
    struct mac_native_scenario_native_family families[MAC_SCENARIO_FAMILY_COUNT];
    struct mac_native_scenario_objects decoded;
    void *owner;
    size_t bytes_owned;
};
/* Supports BG's populated scenery, vehicle and sound-scenery blocks and its
   biped palette. Any nonempty unsupported object family is rejected. */
int mac_native_scenario_blocks_decode(const struct mac_retail_cache *,
    struct mac_native_scenario_blocks *,const char **error);
void mac_native_scenario_blocks_free(struct mac_native_scenario_blocks *);
#endif
