/* Android's stock-lighting implementation of the optional renderer hooks.
   The build excludes the feature modules, so saved settings and host glow
   policy cannot create extra lights, textures, traces or material overrides.
   Overshield surfaces still follow host policy and keep their map reset. */
#include "cseries.h"
#include "real_math.h"
#include "plasma_lighting.h"
#include "plasma_model_shader.h"
#include "rocket_lighting.h"
#include "muzzle_lighting.h"
#include "covenant_lighting.h"
#include "covenant_emitter_lighting.h"
#include "ammo_counter_lighting.h"
#include "needler_lighting.h"
#include "shield_glow.h"
#include "overshield_effect.h"

void plasma_lighting_prepare(long light, long object, long definition) {}
float plasma_lighting_radius(long light, long object, float radius) { return radius; }
int plasma_lighting_vehicle_occlusion(long light) { return FALSE; }
void plasma_lighting_frame_end(void) {}
void plasma_lighting_reset(void) {}
void plasma_lighting_window_begin(void) {}
void plasma_lighting_submitted(long light, long rasterizer) {}
int plasma_lighting_selected(long rasterizer) { return FALSE; }
void plasma_model_lighting_reset(void) {}
void plasma_model_lighting_slot(long light, short slot) {}
unsigned int plasma_model_lighting_mask(void) { return 0; }
void plasma_falloff_begin(long rasterizer) {}
void plasma_falloff_specular_begin(long rasterizer) {}
void plasma_specular_begin(long rasterizer) {}
unsigned long plasma_specular_color(unsigned long material) { return material; }
void plasma_falloff_end(void) {}
void plasma_falloff_dispose(void) {}

void rocket_lighting_update(void) {}
void rocket_lighting_forget(void) {}
void rocket_lighting_frame_end(void) {}
void rocket_lighting_window_begin(void) {}
void rocket_lighting_submitted(long light, long rasterizer) {}
int rocket_lighting_selected(long rasterizer) { return FALSE; }
float rocket_lighting_radius(long light, long object, float radius) { return radius; }

void muzzle_lighting_shot(long weapon, short trigger) {}
void muzzle_lighting_update(void) {}
void muzzle_lighting_frame_end(void) {}
void muzzle_lighting_forget(void) {}
void muzzle_lighting_window_begin(void) {}
void muzzle_lighting_submitted(long light, long rasterizer) {}
int muzzle_lighting_selected(long rasterizer) { return FALSE; }
float muzzle_lighting_radius(long light, float radius) { return radius; }
int muzzle_lighting_vehicle_occlusion(long light) { return FALSE; }

void covenant_lighting_update(void) {}
void covenant_lighting_forget(void) {}
void covenant_lighting_frame_end(void) {}
void covenant_lighting_window_begin(void) {}
void covenant_lighting_submitted(long light, long rasterizer) {}
int covenant_lighting_selected(long rasterizer) { return FALSE; }
float covenant_lighting_radius(long light, long object, float radius) { return radius; }
float covenant_lighting_scale(const char *name) { return 0.0f; }
void covenant_lighting_color(long object, long definition, real_rgb_color *color) {}
int covenant_lighting_owns(long light, long object) { return FALSE; }

int covenant_emitter_lighting_enabled(void) { return FALSE; }
void covenant_emitter_lighting_update(void) {}
void covenant_emitter_lighting_frame_end(void) {}
void covenant_emitter_lighting_forget(void) {}
void covenant_emitter_lighting_window_begin(void) {}
void covenant_emitter_lighting_submitted(long light, long rasterizer) {}
int covenant_emitter_lighting_selected(long rasterizer) { return FALSE; }
long covenant_emitter_lighting_occlusion_ignore(long light) { return NONE; }
float covenant_emitter_lighting_radius(long light, float radius) { return radius; }
int covenant_emitter_lighting_cone(long rasterizer, float axis[3], float origin[3], float *radius) { return 0; }

int ammo_counter_lighting_position(long weapon, real_point3d *position, real_vector3d *direction) { return 0; }
void ammo_counter_lighting_screen(struct shader *shader, real_rgb_color *color) {}
void ammo_counter_lighting_draw_begin(struct shader *shader, unsigned long output) {}
void ammo_counter_lighting_draw_end(void) {}
void ammo_counter_lighting_forget(void) {}
int needler_lighting_position(long weapon, real_point3d *position, real_vector3d *direction) { return 0; }
void needler_lighting_material(struct shader *shader, real_rgb_color *color) {}
void needler_lighting_forget(void) {}
void needler_lighting_transparent(struct shader *shader, struct pixel_shader_definition *pixel, int stages) {}

boolean shield_glow_enabled(void) { return FALSE; }
void shield_glow_reset(void) { overshield_effect_reset(); }
void shield_glow_update(void) {}
void shield_glow_frame_end(void) {}
void shield_glow_window_begin(void) {}
void shield_glow_submitted(long light, long rasterizer) {}
boolean shield_glow_selected(long rasterizer) { return FALSE; }
float shield_glow_radius(long light, float radius) { return radius; }

int covenant_emitter_lighting_vehicle_occlusion(long light) { (void)light; return 0; }
