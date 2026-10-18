#ifndef HALO_MAC_NATIVE_HUD_COMPONENT_LAYOUT_H
#define HALO_MAC_NATIVE_HUD_COMPONENT_LAYOUT_H
#include "interface/hud_definitions.h"


/* Private retail HUD records whose layouts are declared beside their users. */
struct number_hud_element_definition {
    struct hud_placement_definition placement; struct hud_color_definition colors;
    char digits; byte number_flags; char fractional_digits; byte pad; long unused[3];
};
struct hud_number_definition {
    struct tag_reference number_bitmap; char character_width; char screen_width; char x_offset; char y_offset;
    char decimal_point_width; char colon_width; short pad; long unused[19];
};
struct weapon_hud_overlay_item {
    struct hud_placement_definition placement; struct hud_color_definition colors;
    short frame_rate; short pad; short sequence_index; short type; long flags; long unused[14];
};
struct weapon_hud_overlay_definition { struct tag_reference bitmap; struct tag_block items; };
struct multitexture_overlay_hud_element_effector_definition {
    long unused0[16]; short destination_type; short destination; short source; word pad46;
    real in_bounds[2]; real out_bounds[2]; long unused58[16];
    real_rgb_color tint_color_lower_bounds; real_rgb_color tint_color_upper_bounds;
    short periodic_function; word padB2; real periodic_function_period; real periodic_function_phase;
    long unusedBC[8];
};
struct multitexture_overlay_hud_element_definition {
    word flags; short type; short framebuffer_blend_function; word pad06; long unused08[8];
    word map_flags[3]; short map_blending_function[2]; short pad32;
    real_vector2d map_scale[3]; real_vector2d map_offset[3]; struct tag_reference map[3];
    short map_clamp[3]; short pad9A; long unused9C[46]; struct tag_block functions; long unused160[32];
};
struct weapon_hud_element_header { short state_type; short runtime_flags; short use_on_map_type; short pad; long unused[7]; };
struct weapon_hud_static_element { struct weapon_hud_element_header header; struct static_hud_element_definition static_element; long unused[10]; };
struct weapon_hud_meter_element { struct weapon_hud_element_header header; struct meter_hud_element_definition meter_element; long unused[10]; };
struct weapon_hud_number_element { struct weapon_hud_element_header header; struct number_hud_element_definition number_element; word weapon_flags; short pad; long unused[9]; };
struct weapon_hud_overlays_element { short state_type; short runtime_flags; short use_on_map_type; short pad; long unused[7]; struct weapon_hud_overlay_definition overlays; long unused2[10]; };
struct weapon_hud_crosshair_definition { struct tag_reference bitmap; struct tag_block items; };
struct weapon_hud_crosshairs_element { short crosshair_type; short runtime_flags; short use_on_map_type; short pad; long unused[7]; struct weapon_hud_crosshair_definition crosshairs; long unused2[10]; };
struct weapon_hud_crosshair_item { struct hud_placement_definition placement; struct hud_color_definition colors; short frame_rate; short sequence_index; unsigned long flags; long unused[8]; };
struct hud_sound_definition { struct tag_reference sound; unsigned long state_flags; real scale; byte reserved[32]; };

typedef char native_hud_number_size[sizeof(struct hud_number_definition)==0x64?1:-1];
typedef char native_hud_overlay_size[sizeof(struct multitexture_overlay_hud_element_definition)==0x1e0?1:-1];
typedef char native_hud_effector_size[sizeof(struct multitexture_overlay_hud_element_effector_definition)==0xdc?1:-1];
#endif
