/* The game initializes these Xbox pixel-shader records in progress_bar.c.
   They are real typed storage; this does not claim that the native renderer
   can submit Xbox shader programs. */
#include "source/rasterizer/xbox/rasterizer_xbox_pixel_shader.h"

struct pixel_shader_definition blur_shader;
struct pixel_shader_definition regular_shader;
