/* Native storage with the engine headers' actual renderer declarations.
   These records do not initialize a renderer or claim drawing capability. */
#include "cseries.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_lights.h"
#include "rasterizer/xbox/rasterizer_xbox_pixel_shader.h"
#include "interface/hud.h"
struct rasterizer_frame_begin_parameters global_frame_parameters;
struct pixel_shader_definition pixel_shader;
struct rasterizer_frame_statistics_globals rasterizer_frame_statistics;
struct rasterizer_lights rasterizer_lights;
boolean rasterizer_model_cortana_hack;
struct hud_globals_definition *hud_globals;
