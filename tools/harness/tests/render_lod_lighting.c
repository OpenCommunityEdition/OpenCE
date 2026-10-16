#include "harness.h"
struct render_lighting { short point_light_count, point_light_indices[2]; };
struct object_render_state {
    long object_index, render_scene_index, render_frame_index, refresh_frame_index;
    real level_of_detail_pixels;
    struct render_lighting lighting, desired_lighting;
};
static struct object_render_state test_state;
static struct { long scene_index, frame_index; } render;
static struct { struct { unsigned long flags; } object; } object;
static int tick, configured, static_calls, dynamic_calls;
#define HALO_PORT_MAXIMUM_CACHED_OBJECT_RENDER_STATES 1
#define _object_static_lighting_recompute_bit 0
#define object_render_state_get(i) (&test_state)
#define object_get(i) (&object)
#define game_time_get() tick
#define render_lod_disabled() configured
#define lights_prepare_for_object_static(i, l) (static_calls++)
#define lights_prepare_for_object_dynamic(i, l) (dynamic_calls++)
#define object_light_interpolate FALSE
#define object_get_velocities(i, v, a) memset(v, 0, sizeof(*(v)))
#define object_try_and_get_and_verify_type(i, m) FALSE
#define interpolate_real_rgb_color(...) ((void)0)
#define interpolate_real_argb_color(...) ((void)0)
#define interpolate_normal(...) ((void)0)
#include "under_test.inc"
int main(int argc, char **argv)
{
    real pixels[] = {50, 150, 500};
    int expected[] = {5, 15, 60};
    int sample, mode, frame;
    for (sample = 0; sample < 3; sample++) for (mode = 0; mode < 2; mode++)
    {
        memset(&test_state, 0, sizeof(test_state));
        static_calls = dynamic_calls = 0;
        configured = mode;
        object.object.flags = FLAG(_object_static_lighting_recompute_bit);
        for (tick = 0; tick <= 60; tick++) for (frame = 0; frame < 8; frame++)
        {
            render.scene_index = render.frame_index = tick * 8 + frame + 1;
            object_render_state_refresh(0, 0, pixels[sample], FALSE);
        }
        CHECK(static_calls == expected[sample], "lighting retains authored cadence at %g pixels, LOD option %d: got %d",
            pixels[sample], mode, static_calls);
        CHECK(dynamic_calls == 61 * 8, "dynamic lights continue updating each scene");
    }
    return 0;
}
