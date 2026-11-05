#ifndef COVENANT_EMITTER_LIGHTING_H
#define COVENANT_EMITTER_LIGHTING_H

int covenant_emitter_lighting_enabled(void);
void covenant_emitter_lighting_update(void);
void covenant_emitter_lighting_frame_end(void);
void covenant_emitter_lighting_forget(void);
void covenant_emitter_lighting_window_begin(void);
void covenant_emitter_lighting_submitted(long light, long rasterizer);
int covenant_emitter_lighting_selected(long rasterizer);
int covenant_emitter_lighting_vehicle_occlusion(long light);
long covenant_emitter_lighting_occlusion_ignore(long light);
float covenant_emitter_lighting_radius(long light, float radius);

/* Returns 0 for no mask, 1 for the AR screen, 2 for exposed needles. */
int covenant_emitter_lighting_cone(long rasterizer, float axis[3], float origin[3], float *radius);

#endif
