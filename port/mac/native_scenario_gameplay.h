#ifndef HALO_MAC_NATIVE_SCENARIO_GAMEPLAY_H
#define HALO_MAC_NATIVE_SCENARIO_GAMEPLAY_H
#include "retail_cache.h"
#include <stdint.h>

/* Owned subset of the populated multiplayer scenario blocks. This is not a
   converted scenario root; unsupported scenario data remains unavailable. */
struct mac_native_scenario_gameplay {
    void *netgame_flags;
    uint32_t netgame_flag_count;
    void *netgame_equipment;
    uint32_t netgame_equipment_count;
    void *starting_equipment;
    uint32_t starting_equipment_count;
    void *device_groups;
    uint32_t device_group_count;
    /* campaign blocks (see native_scenario_gameplay.c for what each keeps) */
    void *trigger_volumes;
    uint32_t trigger_volume_count;
    void *cutscene_flags;
    uint32_t cutscene_flag_count;
    void *camera_points;
    uint32_t camera_point_count;
    void *chapter_titles;
    uint32_t chapter_title_count;
    void *starting_profiles;
    uint32_t starting_profile_count;
    void *recorded_animations;
    uint32_t recorded_animation_count;
    void *ai_encounters;
    uint32_t ai_encounter_count;
    void *ai_command_lists;
    uint32_t ai_command_list_count;
    void *ai_conversations;
    uint32_t ai_conversation_count;
    void *ai_actor_palette;
    uint32_t ai_actor_palette_count;
    void *ai_animation_references;
    uint32_t ai_animation_reference_count;
    void *ai_script_references;
    uint32_t ai_script_reference_count;
    void *ai_recording_references;
    uint32_t ai_recording_reference_count;
    void *bsp_switch_volumes; /* 4 shorts a volume */
    uint32_t bsp_switch_volume_count;
    void *owner;
};

/* The weapons the starting profiles hand out, by tag handle (campaign levels;
   a profile may name none). Returns the number written (at most capacity). */
uint32_t mac_native_scenario_gameplay_profile_weapons(const struct mac_native_scenario_gameplay *,
    uint32_t *handles,uint32_t capacity);
/* The tags the AI blocks name (actor variants, animation graphs, conversation
   dialogue), as group and handle pairs. Returns the number written (at most
   capacity); *total is how many there are. */
uint32_t mac_native_scenario_gameplay_ai_references(const struct mac_native_scenario_gameplay *,
    uint32_t *groups,uint32_t *handles,uint32_t capacity,uint32_t *total);
int mac_native_scenario_gameplay_decode(const struct mac_retail_cache *,
    struct mac_native_scenario_gameplay *, const char **error);
void mac_native_scenario_gameplay_free(struct mac_native_scenario_gameplay *);
#endif
