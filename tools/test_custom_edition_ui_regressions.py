"""Compile the production CE adapters against a small, asset-free engine boundary."""
import os
from pathlib import Path
import subprocess

from tools.harness import enum_with, function, read, structure


def compile_run(tmp_path, source):
    c = tmp_path / "check.c"
    exe = tmp_path / ("check.exe" if os.name == "nt" else "check")
    c.write_text(source)
    result = subprocess.run([os.environ.get("CC", "clang"), "-m32", "-std=gnu89",
        "-Werror", "-Wno-unused-function", "-D_CRT_SECURE_NO_WARNINGS", str(c), "-o", str(exe)],
        capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr


COMMON = r"""
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define TRUE 1
#define FALSE 0
#define NONE -1
typedef int boolean;
#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#define _stricmp strcasecmp
#endif
"""


def test_texture_coordinates_follow_pixels_not_active_map(tmp_path):
    source = read("port/linux/src/xbox_textures.c")
    metadata = source[source.index("struct custom_edition_texels\n"):
        source.index("static void upload(", source.index("struct custom_edition_texels\n"))]
    code = COMMON + r"""
typedef unsigned long DWORD;
typedef unsigned int GLuint, GLenum;
typedef int BOOL;
#define NUMBER_OF_CUSTOM_EDITION_CHANNEL_ORDERS 4
#define _custom_edition_channels_xbox 0
#define GL_TEXTURE_2D 2
#define _texel_p8 8
#define platform_log(...) ((void)0)
#define text_hires_atlas_texture(...) 0
#define menu_art_texture(...) 0
#define hud_hires_placeholder_texture(...) 0
#define hud_hires_override_texture(...) 0
#define hud_hires_override_coverage(...) 0
#define hud_hires_override_point_threshold(...) 0
#define hud_hires_upscale_at(...) 0
#define hud_hires_upscale_texture(...) 0
#define high_res_hud() 0
#define texture_entry_pixels_valid(...) 0
#define decode_level(...) 0
struct info { int kind; };
static struct info format_information(unsigned long format) { struct info r = {0}; return r; }
"""
    code += structure(read("port/linux/src/xgpu.h"), "xgpu_texture_description")
    code += structure(source, "texture_entry") + metadata + function(source, "texture_entry_result")
    code += r"""
int main(void)
{
    struct texture_entry entry = {0};
    struct xgpu_texture_description shown;
    GLenum target;
    entry.address = 4096;
    entry.override = -1;
    entry.description.linear = TRUE;
    entry.description.pitch = 1280;
    entry.description.width = 320;
    entry.description.height = 240;
    texture_entry_result(&entry, &target, &shown);
    assert(shown.linear); /* Xbox loading picture, before a CE registration. */
    halo_custom_edition_texels_register((void *)8192, 0, TRUE);
    texture_entry_result(&entry, &target, &shown);
    assert(shown.linear); /* A CE map's bitmap does not change engine textures. */
    entry.address = 8192;
    texture_entry_result(&entry, &target, &shown);
    assert(!shown.linear && entry.description.linear && shown.pitch == 1280);
    halo_custom_edition_texels_channels((void *)8192, 1);
    texture_entry_result(&entry, &target, &shown);
    assert(shown.linear && custom_edition_texels_order(8192) == 1);
    halo_custom_edition_texels_register((void *)8192, 1, TRUE);
    halo_custom_edition_texels_channels((void *)8192, 0);
    texture_entry_result(&entry, &target, &shown);
    assert(shown.linear && custom_edition_texels_order(8192) == 0);
    halo_custom_edition_texels_register((void *)8192, 0, TRUE);
    halo_custom_edition_texels_forget();
    texture_entry_result(&entry, &target, &shown);
    assert(shown.linear && custom_edition_texel_count == 0); /* CE -> Xbox/MCC */
    return 0;
}
"""
    compile_run(tmp_path, code)


def test_new_game_is_confirmed_atomic_and_session_owned(tmp_path):
    code = COMMON + r"""
struct widget_instance { long definition_tag_index; };
struct network_game_server { int unused; } servers[2];
struct game_variant { int value; };
struct game_variant_options { int value; };
static struct network_game_server *server = &servers[0];
static int ce = 1, mcc, running = 1, map_allowed = 1, profile_allowed = 1;
static int map_changes, variant_changes, ended, selected, variant_value;
static const char *maps[2] = { "custom_maps\\alpha", "custom_maps\\beta" };
static struct { short chosen; } map_list;
static struct { short gametype_chosen, bank_count, gametype_count, bank[2]; long gametypes[2]; }
    multiplayer = {0, 1, 1, {0, 0}, {0x80000001, 0}};
static int custom_edition_cache_tags_loaded(void) { return ce; }
static int mcc_cache_tags_loaded(void) { return mcc; }
static int pc_menu_tag(long index) { return index == 99; }
static struct network_game_server *global_network_game_server_get(void) { return server; }
static void map_list_update(struct widget_instance *list) {}
static void gametype_list_update(struct widget_instance *list) {}
static short ui_widget_port_multiplayer_maps(const char *const **names, short *last)
{ *names = maps; return 2; }
static int game_engine_running(void) { return running; }
static void game_engine_end_game(void) { ended++; }
static int playlist_profile_get(long index, struct game_variant *variant)
{ variant->value = 42; return profile_allowed; }
static int playlist_profile_get_options(long index, struct game_variant_options *options)
{ options->value = 7; return profile_allowed; }
static int network_game_is_splitscreen_local(void) { return 0; }
static int cache_files_map_plays_multiplayer(const char *map, char *build) { return map_allowed; }
static void cache_files_show_multiplayer_unavailable(const char *map, char *build) {}
static int saved_game_file_get_path_to_enclosing_directory(long profile, char *path) { return 0; }
static void saved_game_file_remember_last_used_multiplayer_variant_directory(const char *path) {}
static int ui_widget_port_multiplayer_map_choose(short index)
{ if (!map_allowed) return 0; map_changes++; selected = index; return 1; }
static int ui_widget_port_game_variant_set(struct game_variant *v, struct game_variant_options *o)
{ variant_changes++; variant_value = v->value; assert(o->value == 7); return 1; }
"""
    code += read("port/linux/game/custom_edition_menu_session.inl")
    code += r"""
int main(void)
{
    struct widget_instance list = {1};
    boolean deleted = FALSE;
    assert(pc_menu_map_stash(&list));
    assert(!pc_menu_new_game_apply());
    assert(!map_changes && !variant_changes); /* Abandoned map cannot apply. */
    assert(pc_menu_map_stash(&list));
    custom_edition_menu_cancel_draft();
    assert(!pc_menu_new_game_begin(&list, &deleted));
    assert(!ended);
    assert(pc_menu_map_stash(&list));
    multiplayer.gametype_chosen = -1;
    assert(!pc_menu_new_game_begin(&list, &deleted));
    multiplayer.gametype_chosen = 0;
    profile_allowed = 0;
    assert(!pc_menu_new_game_begin(&list, &deleted));
    assert(!ended);
    profile_allowed = 1;
    assert(pc_menu_new_game_begin(&list, &deleted));
    assert(!deleted); /* Ending a match did not synchronously free a widget. */
    custom_edition_menu_cancel_draft(); /* Deliberate postgame UI closure. */
    map_allowed = 0;
    assert(!pc_menu_new_game_apply());
    assert(!map_changes && !variant_changes); /* Failed map: neither half applied. */
    map_allowed = 1;
    assert(pc_menu_map_stash(&list));
    assert(pc_menu_new_game_begin(&list, &deleted));
    maps[0] = "custom_maps\\beta"; maps[1] = "custom_maps\\alpha";
    profile_allowed = 0; /* Confirmed profile is a snapshot, not a stale file index. */
    assert(pc_menu_new_game_apply());
    assert(selected == 1 && map_changes == 1 && variant_changes == 1 && variant_value == 42);
    assert(!pc_menu_new_game_apply());
    assert(pc_menu_map_stash(&list));
    assert(pc_menu_restart_game(&deleted));
    assert(pc_menu_new_game_apply());
    assert(map_changes == 1 && variant_changes == 1); /* Restart discards draft. */
    profile_allowed = 1;
    assert(pc_menu_map_stash(&list));
    assert(pc_menu_new_game_begin(&list, &deleted));
    custom_edition_menu_server_dispose(server);
    assert(!pc_menu_new_game_apply());
    assert(pc_menu_map_stash(&list));
    assert(pc_menu_new_game_begin(&list, &deleted));
    server = &servers[1];
    assert(!pc_menu_new_game_apply());
    ce = 0;
    assert(!pc_menu_map_stash(&list) && !pc_menu_restart_game(&deleted));
    ce = 1; mcc = 1;
    assert(!pc_menu_map_stash(&list) && !pc_menu_restart_game(&deleted));
    mcc = 0; list.definition_tag_index = 99;
    assert(!pc_menu_map_stash(&list));
    return 0;
}
"""
    compile_run(tmp_path, code)


def test_map_caption_identity_survives_missing_stock_maps(tmp_path):
    code = COMMON + r"""
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))
static const char *maps[] = {"custom_maps\\my_map", "custom_maps\\infinity", "levels\\test\\bloodgulch\\bloodgulch"};
static short ui_widget_port_multiplayer_maps(const char *const **names, short *last)
{ *names = maps; return 3; }
""" + function(read("port/linux/game/menu_functions.c"), "ce_menu_stock_map")
    code += r"""
int main(void)
{
    assert(ce_menu_stock_map(0) == NONE);
    assert(ce_menu_stock_map(1) == 16);
    assert(ce_menu_stock_map(2) == 9);
    assert(ce_menu_stock_map(-1) == NONE && ce_menu_stock_map(3) == NONE);
    return 0;
}
"""
    compile_run(tmp_path, code)



def test_ce_npot_header_retains_mips_and_rejects_unsupported_layouts(tmp_path):
    source = read("port/linux/game/custom_edition_bitmaps.c")
    renderer = read("port/linux/src/xbox_textures.c")
    defines = "\n".join(line for line in read("port/include/xdk/xdk_d3d8.h").splitlines()
        if line.startswith(("#define D3DFORMAT_", "#define D3DSIZE_", "#define D3DTEXTURE_")))
    code = COMMON + defines + r"""
typedef unsigned long DWORD;
typedef int BOOL;
#define TEST_FLAG(flags, bit) ((flags) & (1 << (bit)))
struct bitmap_data { int type, depth, width, height, flags, format, mipmap_count, pixels_size; };
struct D3DBaseTexture { unsigned long Format, Size; };
static int ce = 1, mcc;
static int custom_edition_cache_tags_loaded(void) { return ce; }
static int mcc_cache_tags_loaded(void) { return mcc; }
static int bitmap_verify(struct bitmap_data *b, int import) { return b->width > 0 && b->height > 0; }
static long bitmap_get_pixel_data_size(struct bitmap_data *b) { return 14728; }
static int bitmap_format_to_d3d_format(int format, int flags) { return format; }
struct format_information { int kind, linear, bytes; };
static struct format_information format_information(unsigned long format)
{ struct format_information info = {1, 0, 8}; return info; }
static int kind_compressed(int kind) { return kind == 1; }
"""
    code += "\n".join(enum_with(source, member) for member in
        ["_bitmap_type_2d", "_bitmap_format_a8,", "_bitmap_has_power_of_two_dimensions_bit"])
    code += "\n".join(function(source, name) for name in ["power_of_two", "bitmap_format_drawable",
        "bitmap_npot_compressed", "bitmap_readable", "custom_edition_bitmap_hardware_format"])
    code += structure(read("port/linux/src/xgpu.h"), "xgpu_texture_description")
    code += "\n".join(function(renderer, name) for name in ["level_dimension", "xgpu_texture_describe",
        "level_bytes", "xgpu_texture_level_offset", "xgpu_texture_face_size"])
    code += r"""
int main(void)
{
    struct bitmap_data bitmap = {0, 1, 200, 112, 2, 14, 2, 14728};
    struct D3DBaseTexture header = {0};
    struct xgpu_texture_description description;
    assert(custom_edition_bitmap_hardware_format(&bitmap, &header));
    xgpu_texture_describe(header.Format, header.Size, &description);
    assert(description.width == 200 && description.height == 112);
    assert(description.levels == 3 && !description.linear && description.compressed);
    assert(xgpu_texture_face_size(&description) == 14728);
    assert(xgpu_texture_level_offset(&description, 2) == 14000);
    ce = 0;
    assert(!custom_edition_bitmap_hardware_format(&bitmap, &header));
    ce = 1; mcc = 1;
    assert(!custom_edition_bitmap_hardware_format(&bitmap, &header));
    mcc = 0;
    bitmap.pixels_size--;
    assert(!bitmap_readable(&bitmap));
    bitmap.pixels_size++;
    bitmap.type = _bitmap_type_3d;
    assert(!bitmap_readable(&bitmap));
    bitmap.type = _bitmap_type_2d;
    bitmap.format = _bitmap_format_a8;
    bitmap.flags = 0;
    assert(!bitmap_readable(&bitmap)); /* Unswizzled NPOT bytes are not Xbox swizzle. */
    bitmap.format = _bitmap_format_dxt1;
    bitmap.flags = 2;
    bitmap.width = 5000;
    assert(!bitmap_readable(&bitmap)); /* Size word cannot hold this width. */
    bitmap.width = 200;
    bitmap.flags |= (1 << _bitmap_swizzled_bit);
    assert(!bitmap_readable(&bitmap));
    return 0;
}
"""
    compile_run(tmp_path, code)


def test_gamepad_page_allocation_is_transactional(tmp_path):
    code = COMMON + r"""
#include <wchar.h>
#define MAX(a,b) ((a) > (b) ? (a) : (b))
struct widget_instance {
    struct widget_instance *parent, *child, *focused_child, *next, *previous;
    short horizontal_offset, vertical_offset;
    struct { struct { short selected_index; } list; } parameters;
};
static struct {
    short count[2], built_count[2], rows[2][16], built[2][16];
    struct widget_instance tpl[2];
    short base, pitch;
    long spinner_tag;
} gp;
static struct widget_instance lists[2];
static int allocation, fail_at = -1, live;
static struct widget_instance *gp_list(struct widget_instance *screen, short column) { return &lists[column]; }
static struct widget_instance *ui_widget_port_clone_row(struct widget_instance *source)
{
    struct widget_instance *row, *last;
    if (allocation++ == fail_at) return NULL;
    row = calloc(1, sizeof(*row)); live++; row->parent = source->parent;
    last = source->parent->child;
    if (!last) source->parent->child = row;
    else { while (last->next) last = last->next; last->next = row; row->previous = last; }
    return row;
}
static void ui_widget_delete(struct widget_instance *row)
{
    if (row->previous) row->previous->next = row->next;
    else row->parent->child = row->next;
    if (row->next) row->next->previous = row->previous;
    free(row); live--;
}
static int gp_slider(short code, const wchar_t *const **values, short *count, short **value)
{ static short v; static const wchar_t *labels[2] = {L"A",L"B"}; *values=labels;*count=2;*value=&v;return 1; }
static short gp_slider_x(short code) { return 0; }
static struct widget_instance *ui_widget_port_add_spinner(struct widget_instance *row, long tag,
    short x, short y, const wchar_t *const *values, short count)
{ return allocation++ == fail_at ? NULL : row; }
""" + function(read("port/linux/game/custom_edition_menu_gamepad.inl"), "gp_sync_columns")
    code += r"""
int main(void)
{
    struct widget_instance screen = {0};
    struct widget_instance *old[2];
    int failure, column;
    gp.spinner_tag = 1;
    for (column=0; column<2; column++) {
        gp.tpl[column].parent = &lists[column];
        gp.built_count[column] = -1;
        gp.count[column] = 1;
        gp.rows[column][0] = column;
    }
    assert(gp_sync_columns(&screen));
    assert(live == 2);
    old[0] = lists[0].child; old[1] = lists[1].child;
    for (failure=0; failure<8; failure++) {
        for (column=0; column<2; column++) {
            gp.count[column] = 2;
            gp.rows[column][0] = 10 + column;
            gp.rows[column][1] = 20 + column;
        }
        allocation=0; fail_at=failure;
        assert(!gp_sync_columns(&screen));
        assert(live==2 && lists[0].child==old[0] && lists[1].child==old[1]);
        assert(lists[0].focused_child==old[0] && lists[1].focused_child==old[1]);
        assert(gp.count[0]==1 && gp.rows[0][0]==0 && gp.count[1]==1 && gp.rows[1][0]==1);
    }
    fail_at=-1;
    for (column=0; column<2; column++) { gp.count[column]=2;gp.rows[column][0]=30+column; }
    assert(gp_sync_columns(&screen));
    assert(live==4 && gp.built[0][0]==30 && gp.built[1][0]==31);
    for (column=0; column<2; column++) {
        assert(lists[column].child->parent==&lists[column]);
        while(lists[column].child) ui_widget_delete(lists[column].child);
    }
    assert(!live);
    return 0;
}
"""
    compile_run(tmp_path, code)
