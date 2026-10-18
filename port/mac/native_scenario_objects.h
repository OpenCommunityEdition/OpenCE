#ifndef HALO_MAC_NATIVE_SCENARIO_OBJECTS_H
#define HALO_MAC_NATIVE_SCENARIO_OBJECTS_H
#include "retail_cache.h"

/* Owned, endian-correct view of the scenario data consumed by object placement.
   This is deliberately a subset, not a castable struct scenario. */
struct mac_native_scenario_name {
    char name[32];
    int16_t runtime_object_type,runtime_scenario_datum_index;
};
struct mac_native_scenario_object {
    int16_t palette_entry_index,name_index;
    uint16_t placement_flags;
    int16_t variant_number;
    float position[3],rotation[3];
    uint16_t on_bsp_flags,misc_flags;
    uint32_t unused;
    uint32_t change_colors[4];
    uint8_t region_permutations[8];
    uint32_t permutation_unused[2];
    uint8_t has_permutation;
    float unit_body_vitality;
    uint32_t unit_flags;
    uint8_t has_unit_state;
    int8_t multiplayer_team_index;
    uint8_t multiplayer_spawn_flags_pad;
    uint16_t multiplayer_spawn_flags;
    uint8_t has_vehicle_state;
    /* devices: scenario_device_datum at 0x28, then each kind's own fields */
    int16_t device_power_group,device_position_group;
    uint32_t device_flags;
    uint8_t has_device;
    float light_color[3],light_intensity,light_falloff_angle,light_cutoff_angle;
    uint8_t machine_flags;
    uint16_t control_flags;
    int16_t control_custom_name_index;
    /* weapons: scenario_weapon_datum at 0x48 (scenario_definitions.h) */
    int16_t weapon_rounds_total,weapon_rounds_loaded;
    uint16_t weapon_flags;
    uint8_t has_weapon;
};
struct mac_native_scenario_palette {
    uint32_t group_tag,handle;
    char *name;
    uint32_t name_length;
};
enum mac_native_scenario_object_family {
    MAC_SCENARIO_SCENERY,MAC_SCENARIO_BIPED,MAC_SCENARIO_VEHICLE,
    MAC_SCENARIO_EQUIPMENT,MAC_SCENARIO_WEAPON,MAC_SCENARIO_MACHINE,
    MAC_SCENARIO_CONTROL,MAC_SCENARIO_LIGHT_FIXTURE,MAC_SCENARIO_SOUND_SCENERY,
    MAC_SCENARIO_FAMILY_COUNT
};
struct mac_native_scenario_family {
    struct mac_native_scenario_object *placements;
    uint32_t placement_count;
    struct mac_native_scenario_palette *palette;
    uint32_t palette_count;
};
struct mac_native_scenario_objects {
    struct mac_native_scenario_name *names;
    uint32_t name_count;
    struct mac_native_scenario_family families[MAC_SCENARIO_FAMILY_COUNT];
    void *owner;
    size_t bytes_owned;
};
/* Validates every block range, object name index, finite pose, palette index,
   and salted typed palette reference before publishing this owned view. */
int mac_native_scenario_objects_decode(const struct mac_retail_cache *,
    struct mac_native_scenario_objects *,const char **error);
void mac_native_scenario_objects_free(struct mac_native_scenario_objects *);
#endif
