/*
HALO_LINUX_SOURCE_FIXUPS.H

The port's functions that the game's sources call, declared for them.
*/

#ifndef __HALO_LINUX_SOURCE_FIXUPS_H
#define __HALO_LINUX_SOURCE_FIXUPS_H

/* frames between the 30 Hz ticks (port/linux/game/render_interpolation.c);
the platform layer reads the display.interpolation setting */
struct observer_result;
struct render_camera;
struct real_matrix4x3;
int halo_interpolation_enabled(void);
float game_time_get_tick_fraction(void);
void render_interpolation_tick(void);
void render_interpolation_reset(void);
void render_interpolation_frame_begin(void);
void render_interpolation_frame_end(void);
float render_interpolation_fraction(void);
struct real_matrix4x3 *render_interpolation_object_node_matrices(long object_index);
struct observer_result const *render_interpolation_camera(short local_player_index,
	struct observer_result const *observer);
void render_interpolation_first_person(short local_player_index, struct real_matrix4x3 *node_matrices,
	short node_count, struct render_camera const *camera);
float render_interpolation_game_time_sec(long ticks);

/* the host's physics world for ragdolls and debris (port/linux/game/physics_jolt.c):
the structure bsp's collision triangles sent when it loads or switches,
everything removed with the map or a saved game's load, and each tick's
ragdolls stepped after the objects' update */
void physics_jolt_structure_bsp_reconnect(void);
void physics_jolt_dispose_from_old_map(void);
void physics_jolt_game_state_loaded(void);
void physics_jolt_update(float seconds);
/* dead bipeds' ragdolls (port/linux/game/ragdolls.c): the damage done to an
object, and a unit's death (not a feigned one), with its pose */
struct damage_data;
void ragdolls_damage(long object_index, struct damage_data const *damage);
void ragdolls_unit_died(long unit_index);
/* the node matrices an object is drawn with: its ragdoll's, else `nodes`
(render_objects.c, render_interpolation.c) */
struct real_matrix4x3 const *ragdolls_node_matrices(long object_index, struct real_matrix4x3 const *nodes);
/* a torn ragdoll's model drawn a piece at a time: the draws, and each one's
node matrices from those drawn (render_objects.c) */
long ragdolls_piece_count(long object_index);
struct real_matrix4x3 const *ragdolls_piece_node_matrices(long object_index, struct real_matrix4x3 const *nodes,
	long piece);
/* particles as rigid bodies (port/linux/game/physics_debris.c, particles.c):
a particle that struck the level made one, and one that is a body moved as
the host has it, settled once the body sleeps */
void physics_debris_particle_collided(long particle_index);
unsigned char physics_debris_particle_update(long particle_index, float dt, unsigned char *settled);
/* the host's bodies drawn as wireframes (debug.physics_draw,
port/linux/game/physics_draw.c, render_debug.c) */
void physics_draw_render(void);

/* the port's menus' screen for one of ui.map's, by name, else the name
(port/linux/game/menu_tags.c) */
char const *pc_menus_screen(char const *name);
/* the Custom Edition tag cache window, or NULL unless the
game.custom_edition setting reserved it (port/linux/src/xbox_memory.c) */
void *halo_custom_edition_tag_cache(void);
/* where Halo PC keeps the channels of the pixels a Custom Edition bitmap
just arrived at (an enum custom_edition_channel_order,
port/linux/game/cache_file_formats.h), which the renderer then samples in
this build's order; forgotten together when the map goes
(port/linux/src/xbox_textures.c) */
void halo_custom_edition_texels_channels(const void *texels, unsigned char channel_order);
void halo_custom_edition_texels_forget(void);
/* whether a Halo Custom Edition map's multiplayer vehicles are chosen by
their placements' spawn flags, as in retail Halo, and whether a vehicle
placement is placed in the running game
(port/linux/game/custom_edition_objects.c) */
struct scenario_object_datum;
unsigned char custom_edition_vehicles_by_placement(void);
unsigned char custom_edition_vehicle_placement_allowed(struct scenario_object_datum const *placement);

/* the width of the screen the game draws, 480 lines tall: the device's or
the display's shape, or 640 (port/linux/src/d3d8_gl.c) */
long halo_screen_width(void);
/* the screen's pixels to the Xbox's one (port/linux/src/d3d8_gl.c) */
float halo_screen_scale(void);
/* takes up a new width between frames (F11); returns the width */
long halo_screen_commit(void);
/* the shadow maps' pixels for each of their 128 texels each way, a power of
two (display.shadow_resolution) */
long halo_shadow_map_scale(void);
/* while TRUE, drawing shifts right to center 640-column layouts */
void halo_screen_ui_offset(unsigned char centered);
/* names a model lighting vertex program, whose draws can be lit for each
pixel (port/linux/src/d3d8_gl.c, display.per_pixel_lighting) */
void halo_vertex_shader_lighting(unsigned long handle);
/* display.anti_aliasing's pass over a window's 3D view, before the HUD and
menus (source/render/render.c): the window's bounds on the screen */
void halo_screen_anti_alias(short x0, short y0, short x1, short y1);
/* the mouse in the menus (source/interface/ui_widget.c) */
#include "halo_ui_pointer.h"

#endif
