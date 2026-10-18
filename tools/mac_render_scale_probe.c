/* mac.render_resolution's choices (port/mac/mac_render_scale.h), checked
   without a graphics card. */
#include <stdio.h>
#include <math.h>
#include "mac_render_scale.h"
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"line %d failed: %s\n",__LINE__,#x); } } while (0)
static int near(float a, float b) { return fabs(a - b) < 0.002; }
int main(void)
{
    int clamp;
    float s;
    /* the words */
    CHECK(mac_render_parse(NULL) == MAC_RENDER_LINES_AUTO);
    CHECK(mac_render_parse("") == MAC_RENDER_LINES_AUTO);
    CHECK(mac_render_parse("auto") == MAC_RENDER_LINES_AUTO);
    CHECK(mac_render_parse("native") == MAC_RENDER_LINES_NATIVE);
    CHECK(mac_render_parse("original") == 480);
    CHECK(mac_render_parse("720") == 720);
    CHECK(mac_render_parse("720p") == 720);
    CHECK(mac_render_parse("1280x800") == 800);
    CHECK(mac_render_parse("100") == 480);
    CHECK(mac_render_parse("99999") == MAC_RENDER_LINES_MAXIMUM);
    CHECK(mac_render_parse("rubbish") == MAC_RENDER_LINES_AUTO);
    /* the defaults: Intel with a real card native, the rest 480 lines */
    CHECK(mac_gpu_pick_render_lines(256, 0) == 0);
    CHECK(mac_gpu_pick_render_lines(128, 0) == 0);
    CHECK(mac_gpu_pick_render_lines(64, 0) == 480);
    CHECK(mac_gpu_pick_render_lines(32, 1) == 480);
    CHECK(mac_gpu_pick_render_lines(64, 1) == 480);
    CHECK(mac_gpu_pick_render_lines(256, 1) == 480);
    /* native: the MacBook's 1280x800 (a 768 column picture) is 800 lines */
    s = mac_render_scale_choose(0, 768, 1280, 800, 8192, 0, &clamp);
    CHECK(near(s, 800.0f / 480.0f) && clamp == 0);
    CHECK(mac_render_pixels(768, s) == 1280 && mac_render_pixels(480, s) == 800);
    /* the iBook's 1024x768: a 640 column picture, 768 lines */
    s = mac_render_scale_choose(0, 640, 1024, 768, 2048, mac_render_pixel_budget(32), &clamp);
    CHECK(near(s, 1.6f) && clamp == 0);
    CHECK(mac_render_pixels(640, s) == 1024 && mac_render_pixels(480, s) == 768);
    /* 1920x1080 */
    s = mac_render_scale_choose(0, 852, 1920, 1080, 8192, 0, &clamp);
    CHECK(near(s, 2.25f) && mac_render_pixels(852, s) == 1917);
    /* a window with a shape narrower than the picture's: the width limits */
    s = mac_render_scale_choose(0, 640, 1280, 1024, 8192, 0, &clamp);
    CHECK(near(s, 2.0f));
    /* fixed lines: the iBook's three sizes */
    s = mac_render_scale_choose(480, 640, 1024, 768, 2048, 0, &clamp);
    CHECK(near(s, 1.0f) && clamp == 0);
    s = mac_render_scale_choose(600, 640, 1024, 768, 2048, 0, &clamp);
    CHECK(near(s, 1.25f) && mac_render_pixels(640, s) == 800 && clamp == 0);
    s = mac_render_scale_choose(768, 640, 1024, 768, 2048, 0, &clamp);
    CHECK(near(s, 1.6f) && clamp == 0);
    /* more lines than the window has: the window's, and said so */
    s = mac_render_scale_choose(1080, 640, 1024, 768, 2048, 0, &clamp);
    CHECK(near(s, 1.6f) && (clamp & MAC_RENDER_CLAMP_WINDOW));
    /* the card's largest texture */
    s = mac_render_scale_choose(0, 852, 3840, 2160, 2048, 0, &clamp);
    CHECK(near(s, 2048.0f / 852.0f) && (clamp & MAC_RENDER_CLAMP_SIZE));
    /* the card's memory: the iBook's 32 MB cannot draw 1920x1080 */
    s = mac_render_scale_choose(0, 852, 1920, 1080, 4096, mac_render_pixel_budget(32), &clamp);
    CHECK(s < 2.25f && s > 1.0f && (clamp & MAC_RENDER_CLAMP_MEMORY));
    CHECK(mac_render_pixels(852, s) * mac_render_pixels(480, s) <= mac_render_pixel_budget(32) + 4000);
    /* never below 1, and no window: 1 */
    s = mac_render_scale_choose(0, 640, 400, 300, 0, 0, &clamp);
    CHECK(near(s, 1.0f));
    s = mac_render_scale_choose(0, 0, 0, 0, 0, 0, &clamp);
    CHECK(near(s, 1.0f) && clamp == 0);
    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
