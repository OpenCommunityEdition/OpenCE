/*
SHADER_PRECOMPILE.H

Every GPU program a map can draw with, made when the map's first view is
drawn instead of in the frame that first needs it (shader_precompile.c).
*/

#ifndef SHADER_PRECOMPILE_H
#define SHADER_PRECOMPILE_H

struct shader;

/* a map has loaded: its programs are made in its first view
(render_initialize_for_new_map) */
void shader_precompile_new_map(void);
/* the first view of the map, its window begun (render_window): makes them
then, once */
void shader_precompile_window(void);

/* an effect's shader with its bitmap group's first bitmap, as its sprites,
contrails and lightning draw it */
void shader_precompile_effect(struct shader const *shader, long bitmap_group_index);
/* every model of the map drawn into the shadow that is begun
(structure_render_precompile_shaders) */
void shader_precompile_model_shadows(void);
/* every model of the map drawn under the fog screen that is set up, the
screen drawn over the structure after each (structure_fog_screens_precompile_shaders) */
void shader_precompile_fog_screen_models(void);

/* the passes whose state is the game's own, in the game's sources */
void structure_render_precompile_shaders(long light_index);
void structure_fog_screens_precompile_shaders(void);
void rasterizer_environment_precompile_light_gel(void const *light_definition);
void rasterizer_decals_precompile_shaders(void);
void rasterizer_widgets_precompile_shaders(void);
void rasterizer_detail_objects_precompile_shaders(void);
void rasterizer_sun_glows_precompile_shaders(void);
void lightning_precompile_shaders(void);
void glows_precompile_shaders(void);

#endif
