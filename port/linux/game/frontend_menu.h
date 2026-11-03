/* Optional settings presentation. Pause roots remain owned by each map family. */
#ifndef FRONTEND_MENU_H
#define FRONTEND_MENU_H
#include <stddef.h>

struct widget_instance;
struct event_record;
struct halo_menus;
struct rasterizer_dynamic_screen_geometry_parameters;
struct dynamic_screen_vertex;

int frontend_menu_requested(void);
char const *frontend_menu_style(void);
int frontend_menu_reload_required(void);
void frontend_menu_reflow_text(wchar_t *text);
int frontend_menu_bitmap_region(long tag, short *width, short *height);
void frontend_menu_map_loaded(char const *name, int mcc);
void frontend_menu_unloaded(void);
void frontend_menu_session_reset(void);
int frontend_menu_row_drawable(struct widget_instance *widget);
int frontend_menu_available(void);
char const *frontend_menu_root(char const *legacy);
long frontend_menu_destination(long tag);
void frontend_menu_attach(struct widget_instance *widget);
short frontend_menu_legacy_description_index(struct widget_instance *list, short index);
int frontend_menu_legacy_controls_help(struct widget_instance *list);
int frontend_menu_shortcut(struct widget_instance *widget, struct event_record *event);
int frontend_layout_width(int screen);
int frontend_layout_geometry(char const *name, int screen, short *left, short *right, short *shift);
int frontend_menu_geometry(long tag, short *left, short *right, short *shift);
int frontend_menu_wrap(long tag);
int frontend_menu_stretch_bitmap(long tag);
long frontend_menu_font(long tag, long normal);
void frontend_menu_text_begin(long tag);
void frontend_menu_text_end(void);
void frontend_menu_clip_text(short left, short right, short *clipped_left, short *clipped_right);
void frontend_panel_begin(long tag);
void frontend_panel_end(void);
int frontend_panel_draw(struct rasterizer_dynamic_screen_geometry_parameters *parameters,
    struct dynamic_screen_vertex *vertices, long source_width);
void frontend_glyphs_dispose(void);
struct halo_menus const *frontend_menu_files_load(void);

#define FRONTEND_MENU_SCREEN "pc\\modern\\settings"
#define FRONTEND_MENU_PAUSED "pc\\modern\\paused\\settings"
#define FRONTEND_MENU_XBOX_SETTINGS "pc\\frontend_legacy\\paused\\settings"
#define FRONTEND_MENU_PC "pc\\main_menu\\main_menu"
#define FRONTEND_MENU_XBOX "ui\\shell\\main_menu\\main_menu"

#endif
