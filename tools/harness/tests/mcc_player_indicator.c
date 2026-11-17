#include "harness.h"
#include <wchar.h>

typedef struct { real x, y; } real_point2d;
typedef struct { short x, y; } point2d;
typedef struct { real x0, x1, y0, y1; } real_rectangle2d;
typedef struct { short x0, x1, y0, y1; } rectangle2d;
typedef struct { real alpha; } real_argb_color;
struct player_datum {
    long unit_index;
    wchar_t name[16];
    struct { long machine_index; } network_player_data;
};
struct bitmap_data { short width, height; point2d registration_point; } test_bitmap = {128, 64, {64, 31}};
struct bitmap_group { unsigned short flags; } test_group;
struct font_header { short ascending_height, descending_height; } test_font = {12, 3};
static struct player_datum test_player;
static struct { struct { real active_camouflage; } unit; } unit;
static struct {
    struct { int world_to_view; real z_near, z_far; } frustum;
    struct { real_point3d position; rectangle2d viewport_bounds; } camera;
} render;
static int mcc;
static real drawn_width, drawn_height, drawn_origin_y;
static short name_bottom;
#define mcc_cache_tags_loaded() mcc
#define bitmap_group_get(index) (&test_group)
#define player_get(index) (&test_player)
#define unit_get(index) (&unit)
#define unit_get_head_position(index, out) (*(out) = (real_point3d){0, 0, 0})
#define matrix4x3_transform_point(matrix, in, out) (*(out) = (real_point3d){0, 0, -1})
#define render_camera_view_to_screen(camera, frustum, in, out) ((out)->x=100, (out)->y=100, TRUE)
#define interface_get_tag_index(which) 0
#define bitmap_group_get_bitmap_from_sequence(index, sequence, frame) (&test_bitmap)
#define _texture_cache_bitmap_get_hardware_format(bitmap, a, b) TRUE
#define fast_ftol(value) ((long)(value))
#define PIN(value, low, high) ((value)<(low) ? (low) : (value)>(high) ? (high) : (value))
#define NUMBEROF(array) (sizeof(array)/sizeof((array)[0]))
#define hud_get_font_index() 0
#define distance3d(a, b) 1.0f
#define hud_player_name_in_sight(index, head) TRUE
#define font_definition_get(index) (&test_font)
#define hud_player_name_color(ally, color) ((color)->alpha=1)
#define draw_string_set_draw_mode(a,b,c,d,e) ((void)0)
#define hud_player_name_scale() 1.0f
#define rasterizer_text_set_scale(a,b,c) ((void)0)
#define rasterizer_draw_unicode_string(bounds,a,b,c,name) (name_bottom=(bounds)->y1)
#define network_voice_machine_speaking(index) FALSE
#define draw_unicode_string_compute_bounds(bounds,name,text,cursor) (*(text)=*(bounds))
#define draw_unicode_string_capital_middle(bounds,name) 0
#define network_voice_draw_icon(icon,a,b) ((void)0)

static void interface_draw_bitmap(struct bitmap_data const *b, point2d const *point,
    real_rectangle2d const *clip, real scale, real theta, real fade)
{
    drawn_width=b->width*scale;
    drawn_height=b->height*scale;
    drawn_origin_y=point->y-b->registration_point.y*scale;
}

#include "under_test.inc"

static void check_family(int is_mcc, unsigned short flags, real expected)
{
    mcc=is_mcc;
    test_group.flags=flags;
    hud_draw_friendly_indicator(0);
    hud_draw_player_name(0, TRUE, TRUE, 100);
    CHECK(drawn_width==128*expected && drawn_height==64*expected, "arrow dimensions");
    CHECK(drawn_origin_y==100-31*expected, "arrow registration point");
    CHECK(name_bottom==98-(short)(31*expected), "name follows arrow registration point");
    hud_draw_player_name(0, TRUE, FALSE, 100);
    CHECK(name_bottom==98, "name without indicator");
}

int main(int argc, char **argv)
{
    render.frustum.z_far=1000;
    if (!strcmp(argv[1], "plain")) check_family(TRUE, 0, 0.5f);
    else if (!strcmp(argv[1], "half")) check_family(TRUE, 0x10, 0.25f);
    else if (!strcmp(argv[1], "highres")) check_family(TRUE, 0x80, 0.25f);
    else {
        check_family(TRUE, 0x10, 0.25f);
        check_family(FALSE, 0, 1.0f); /* Xbox */
        check_family(FALSE, 0x10, 1.0f); /* CE */
        check_family(TRUE, 0x80, 0.25f);
        check_family(FALSE, 0x80, 1.0f);
    }
    return 0;
}
