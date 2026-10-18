#ifndef HALO_MAC_NATIVE_LENS_FLARE_LAYOUT_H
#define HALO_MAC_NATIVE_LENS_FLARE_LAYOUT_H
#include "math/real_math.h"
#include "tag_files/tag_groups.h"
struct lens_flare_reflection {
    word flags; short type; short bitmap_index; word pad06; byte reserved08[0x14];
    real offset; real rotation_offset; byte reserved24[4];
    real radius_lower_bound; real radius_upper_bound; short radius_scale_function; word pad32;
    real brightness_lower_bound; real brightness_upper_bound; short brightness_scale_function; word pad3E;
    real_argb_color tint_color; real_argb_color animation_color_lower_bound;
    real_argb_color animation_color_upper_bound; word animation_flags; short animation_function;
    real animation_period; real animation_phase; byte reserved7C[4];
};
struct lens_flare_definition {
    real falloff_angle; real cutoff_angle; real runtime_cosine_falloff_angle;
    real runtime_cosine_cutoff_angle; real occlusion_radius; short occlusion_offset_direction;
    word pad16; real near_fade_distance; real far_fade_distance; struct tag_reference primary_map;
    word flags; word pad32; byte reserved34[0x4C]; short corona_rotation_function; word pad82;
    real corona_rotation_function_scale; byte reserved88[0x18]; real_vector2d corona_radius_scale;
    byte reservedA8[0x1C]; struct tag_block reflections; byte reservedD0[0x20];
};
typedef char native_lens_flare_reflection_size[sizeof(struct lens_flare_reflection)==0x80?1:-1];
typedef char native_lens_flare_size[sizeof(struct lens_flare_definition)==0xF0?1:-1];
typedef char native_lens_flare_reflections_offset[offsetof(struct lens_flare_definition,reflections)==0xC4?1:-1];
#endif
