#ifndef HALO_MAC_NATIVE_WIDGET_LAYOUT_H
#define HALO_MAC_NATIVE_WIDGET_LAYOUT_H
/* Retail PC layouts of the glow (glw!) and lightning (elec) widget
   definitions, as the game's own glow.c and lightning.c read them (the same
   declarations as port/linux/game/tag_schema_render.c, which the schema
   generator cannot include). */
#include "math/real_math.h"
#include "tag_files/tag_groups.h"
#include "shaders/shader_definitions.h"
struct glow_definition
{
	char attachment_marker[32];
	short number_of_particles;
	short boundary_effect;
	short particle_distribution;
	short trailing_particle_distribution;
	unsigned long flags;
	long unused02C[7];
	short render_mode;
	short render_orientation;
	long render_flags;
	short particle_rotational_velocity_attachment_index;
	short pad052;
	real particle_rotational_velocity;
	real particle_rotational_velocity_scale_lower_bound;
	real particle_rotational_velocity_scale_upper_bound;
	short effect_rotational_velocity_attachment_index;
	short pad062;
	real effect_rotational_velocity;
	real effect_rotational_velocity_scale_lower_bound;
	real effect_rotational_velocity_scale_upper_bound;
	short effect_translational_velocity_attachment_index;
	short pad072;
	real effect_translational_velocity;
	real effect_translational_velocity_scale_lower_bound;
	real effect_translational_velocity_scale_upper_bound;
	short distance_to_object_attachment_index;
	short pad082;
	real minimum_distance_to_object;
	real maximum_distance_to_object;
	real distance_to_object_scale_lower_bound;
	real distance_to_object_scale_upper_bound;
	long unused094[2];
	short particle_size_attachment_index;
	short pad09E;
	real particle_size_lower_bound;
	real particle_size_upper_bound;
	real particle_size_scale_lower_bound;
	real particle_size_scale_upper_bound;
	short color_attachment_index;
	short pad0B2;
	real_argb_color color_lower_bound;
	real_argb_color color_upper_bound;
	real_argb_color scale_color_lower_bound;
	real_argb_color scale_color_upper_bound;
	real color_rate_of_change;
	real percentage_edge_fade;
	real trailing_particle_generation_frequency;
	real trailing_particle_lifetime;
	real trailing_particle_velocity;
	real trailing_particle_minimum_t;
	real trailing_particle_maximum_t;
	long unused110[13];
	struct tag_reference texture;
};

struct lightning_marker_definition
{
	char attachment_marker[32];
	word flags;
	short type;
	short octaves_to_next_marker;
	word pad26;
	byte reserved28[0x4C];
	real_vector3d random_position_bounds;
	real random_jitter_offset;
	real thickness;
	real_argb_color tint;
	byte reserved98[0x4C];
};

struct lightning_shader
{
	struct shader_effect_definition shader;
};

struct lightning_definition
{
	word flags;
	short count;
	byte reserved04[0x10];
	real near_fade_distance;
	real far_fade_distance;
	byte reserved1C[0x10];
	short jitter_scale_source;
	short thickness_scale_source;
	short tint_modulation_source;
	short brightness_scale_source;
	struct tag_reference map;
	byte reserved44[0x54];
	struct tag_block markers;
	struct tag_block shaders;
	byte reservedB0[0x58];
};
typedef char native_glow_size[sizeof(struct glow_definition)==0x154?1:-1];
typedef char native_glow_texture_offset[offsetof(struct glow_definition,texture)==0x144?1:-1];
typedef char native_lightning_marker_size[sizeof(struct lightning_marker_definition)==0xE4?1:-1];
typedef char native_lightning_shader_size[sizeof(struct lightning_shader)==0xB4?1:-1];
typedef char native_lightning_size[sizeof(struct lightning_definition)==0x108?1:-1];
/* the flag (flag) widget definition, as the game's flags.c reads it (the
   waving cloth of the CTF flag) */
struct flag_attachment_point
{
	short height_to_next_attachment;
	short pad2;
	long unused[4];
	char marker_name[32];
};

struct flag_definition
{
	unsigned long flags;
	short trailing_edge_shape;
	short trailing_edge_offset;
	short attached_edge_shape;
	short padA;
	short width;
	short height;
	real cell_width_scale;
	real cell_height_scale;
	struct tag_reference shader_red;
	struct tag_reference physics;
	real wind_noise;
	long unused3C[2];
	struct tag_reference shader_blue;
	struct tag_block attachment_points;
};

#endif
