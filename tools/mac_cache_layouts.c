/* Independent layout witness: actual game declarations on each target ABI. */
#include "tools/mac_compile_probe_prefix.h"
#define __inline static __inline__
#include "cseries.h"
#include "real_math.h"
#include "tag_files/tag_groups.h"
#include "memory/data.h"
#include "hs/hs_scenario_definitions.h"
struct location;
#include "physics/point_physics.h"
#include "scenario/wind_definitions.h"
#include "scenario/scenario_definitions.h"
#include "sound/sound_environment_definitions.h"
#include "cache_endian.h"
/* Harness copies use libc; do not require the game memory subsystem. */
#undef memcpy

static int failures;
#define OFFSET(t, f, o) do { if (offsetof(struct t, f) != (o)) { \
    printf("FAIL %s.%s offset=%lu expected=%u\n", #t, #f, \
    (unsigned long)offsetof(struct t, f), (unsigned int)(o)); ++failures; } } while (0)
#define SIZE(t, s) do { if (sizeof(struct t) != (s)) ++failures; } while (0)
int main(void)
{
    unsigned char disk[0x40];
    unsigned char sound_disk[0x48];
    struct point_physics_definition native;
    struct wind_definition wind;
    struct sound_environment_definition sound;
    unsigned int i;
    SIZE(data_array, 0x38); SIZE(hs_syntax_node, 0x14);
    OFFSET(data_array, maximum_count, 0x20); OFFSET(data_array, size, 0x22);
    OFFSET(data_array, valid, 0x24); OFFSET(data_array, identifier_zero_invalid, 0x25);
    OFFSET(data_array, signature, 0x28); OFFSET(data_array, first_free_absolute_index, 0x2c);
    OFFSET(data_array, count, 0x2e); OFFSET(data_array, actual_count, 0x30);
    OFFSET(data_array, next_identifier, 0x32); OFFSET(data_array, data, 0x34);
    OFFSET(hs_syntax_node, datum_header, 0); OFFSET(hs_syntax_node, index, 2);
    OFFSET(hs_syntax_node, type, 4); OFFSET(hs_syntax_node, flags, 6);
    OFFSET(hs_syntax_node, next_node_index, 8); OFFSET(hs_syntax_node, string_offset, 12);
    OFFSET(hs_syntax_node, data, 16);
    OFFSET(scenario, hs_syntax_data, 0x474);
    OFFSET(scenario, hs_string_constants, 0x488);
    OFFSET(scenario, structure_bsp_references, 0x5a4);
    OFFSET(scenario, players, 0x354);
    SIZE(point_physics_definition, 0x40);
    OFFSET(point_physics_definition, flags, 0);
    OFFSET(point_physics_definition, runtime_mass_over_radius_cubed, 4);
    OFFSET(point_physics_definition, runtime_water_buoyancy_scale, 8);
    OFFSET(point_physics_definition, runtime_air_buoyancy_scale, 12);
    OFFSET(point_physics_definition, unused1, 0x10);
    OFFSET(point_physics_definition, density, 0x20);
    OFFSET(point_physics_definition, air_friction, 0x24);
    OFFSET(point_physics_definition, water_friction, 0x28);
    OFFSET(point_physics_definition, contact_friction, 0x2c);
    OFFSET(point_physics_definition, elasticity, 0x30);
    OFFSET(point_physics_definition, unused2, 0x34);
    SIZE(wind_definition, 0x40);
    OFFSET(wind_definition, velocity_lower_bound, 0);
    OFFSET(wind_definition, velocity_upper_bound, 4);
    OFFSET(wind_definition, variation_area, 8);
    OFFSET(wind_definition, variation_area.yaw, 8);
    OFFSET(wind_definition, variation_area.pitch, 12);
    OFFSET(wind_definition, local_variation_weight, 0x10);
    OFFSET(wind_definition, local_variation_rate, 0x14);
    OFFSET(wind_definition, damping, 0x18);
    OFFSET(wind_definition, unused, 0x1c);
    SIZE(sound_environment_definition, 0x48);
    OFFSET(sound_environment_definition, unknown, 0);
    OFFSET(sound_environment_definition, priority, 4);
    OFFSET(sound_environment_definition, unused0, 6);
    OFFSET(sound_environment_definition, room_intensity, 8);
    OFFSET(sound_environment_definition, room_intensity_hf, 12);
    OFFSET(sound_environment_definition, room_rolloff, 0x10);
    OFFSET(sound_environment_definition, decay_time, 0x14);
    OFFSET(sound_environment_definition, decay_hf_ratio, 0x18);
    OFFSET(sound_environment_definition, reflections_intensity, 0x1c);
    OFFSET(sound_environment_definition, reflections_delay, 0x20);
    OFFSET(sound_environment_definition, reverb_intensity, 0x24);
    OFFSET(sound_environment_definition, reverb_delay, 0x28);
    OFFSET(sound_environment_definition, diffusion, 0x2c);
    OFFSET(sound_environment_definition, density, 0x30);
    OFFSET(sound_environment_definition, hf_reference, 0x34);
    OFFSET(sound_environment_definition, unused1, 0x38);
    /* Known LE IEEE floats, then read through actual engine members. */
    for (i = 0; i < sizeof(disk); i += 4) {
        disk[i] = 0; disk[i+1] = 0; disk[i+2] = 0x80; disk[i+3] = 0x3f;
    }
    disk[0] = 0x81; disk[1] = disk[2] = disk[3] = 0;
    if (!mac_cache_record_convert(disk, sizeof(disk), MAC_CACHE_POINT_PHYSICS, 0)) ++failures;
    memcpy(&native, disk, sizeof(native));
    if (native.flags != 0x81 || native.runtime_mass_over_radius_cubed != 1.0f ||
        native.runtime_water_buoyancy_scale != 1.0f || native.runtime_air_buoyancy_scale != 1.0f ||
        native.density != 1.0f || native.air_friction != 1.0f || native.water_friction != 1.0f ||
        native.contact_friction != 1.0f || native.elasticity != 1.0f) ++failures;
    for (i = 0; i < sizeof(disk); i += 4) {
        disk[i] = 0; disk[i+1] = 0; disk[i+2] = 0x80; disk[i+3] = 0xbf;
    }
    if (!mac_cache_record_convert(disk, sizeof(disk), MAC_CACHE_WIND, 0)) ++failures;
    memcpy(&wind, disk, sizeof(wind));
    if (wind.velocity_lower_bound != -1.0f || wind.velocity_upper_bound != -1.0f ||
        wind.variation_area.yaw != -1.0f || wind.variation_area.pitch != -1.0f ||
        wind.local_variation_weight != -1.0f || wind.local_variation_rate != -1.0f ||
        wind.damping != -1.0f) ++failures;
    for (i = 0; i < sizeof(sound_disk); i += 4) {
        sound_disk[i] = 0; sound_disk[i+1] = 0; sound_disk[i+2] = 0x20; sound_disk[i+3] = 0x40;
    }
    sound_disk[0] = 0x78; sound_disk[1] = 0x56; sound_disk[2] = 0x34; sound_disk[3] = 0x12;
    sound_disk[4] = 0xfe; sound_disk[5] = 0xff;
    if (!mac_cache_record_convert(sound_disk, sizeof(sound_disk), MAC_CACHE_SOUND_ENVIRONMENT, 0)) ++failures;
    memcpy(&sound, sound_disk, sizeof(sound));
    if (sound.unknown != 0x12345678 || sound.priority != -2 ||
        sound.room_intensity != 2.5f || sound.room_intensity_hf != 2.5f ||
        sound.room_rolloff != 2.5f || sound.decay_time != 2.5f ||
        sound.decay_hf_ratio != 2.5f || sound.reflections_intensity != 2.5f ||
        sound.reflections_delay != 2.5f || sound.reverb_intensity != 2.5f ||
        sound.reverb_delay != 2.5f || sound.diffusion != 2.5f ||
        sound.density != 2.5f || sound.hf_reference != 2.5f) ++failures;
    printf("Engine script/scalar-tag layouts and decoded floats: %s\n", failures ? "FAIL" : "pass");
    return failures ? 1 : 0;
}
