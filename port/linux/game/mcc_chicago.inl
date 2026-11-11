#include "mcc_cache.h"

static void mcc_chicago_blend(
    struct shader_transparent_chicago const *chicago,
    struct pixel_shader_definition *pixel_shader,
    boolean fade_enabled)
{
    short stage;

    if (!mcc_cache_tags_loaded() || !(chicago->extra_flags & 4) ||
        chicago->framebuffer_blend_function != _framebuffer_blend_function_double_multiply)
        return;

    /* MCC's opt-in CE blend treats black as transparent. Retain the
     * destination and add its modulation, instead of multiplying it twice. */
    SetRenderStateSmart(D3DRS_DESTBLEND, D3DBLEND_ONE);
    if (fade_enabled)
    {
        stage = (short)PIN(chicago->maps.count, 0, NUMBER_OF_SHADER_TRANSPARENT_MAPS);
        /* The native fade stage approaches half grey for double multiply.
         * This blend instead approaches black, its transparent value. */
        pixel_shader->rgb_inputs[stage] &= 0xFFFF0000;
    }
}
