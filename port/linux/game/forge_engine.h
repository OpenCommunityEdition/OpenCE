/*
 * OpenCE : Forge
 * Runtime Engine Header - Shared across all Forge runtime modules.
 */

#ifndef FORGE_ENGINE_H
#define FORGE_ENGINE_H

#include <stdint.h>
#include "forge_cache_builder.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Forge Release Version & Network Protocol Isolation (v2.10.0, 0x462A) */

#include "halo_port_limits.h"

#define FORGE_VERSION_STRING "2.10.0"
#define FORGE_NETWORK_VERSION 0x462A
#define FORGE_GAME_KIND 0x46
#ifndef HALO_PORT_NETWORK_VERSION_STOCK
#define HALO_PORT_NETWORK_VERSION_STOCK HALO_PORT_NETWORK_VERSION
#endif
/* Standard (non-Forge) sessions use OpenCE's own HALO_PORT_NETWORK_VERSION
 * (port/linux/include/halo_port_limits.h), read in forge_get_network_version(),
 * so an upstream protocol bump never leaves Forge unable to join stock games. */

extern int g_forge_session_enabled;
long forge_get_network_version(void);

/* Engine function (source/main/main.h) used by several Forge modules */
char *main_get_map_name(void);
char const *forge_get_active_map_name(void);
void *forge_host_fopen(char const *path, char const *mode);
int forge_host_remove(char const *path);
int forge_host_rename(char const *old_path, char const *new_path);
void forge_server_mark_dirty(void);
void forge_server_autosave_tick(void);
void forge_deferred_client_save_tick(void);
int forge_server_save_map_if_dirty(int force_full_export);

#ifndef _WIN32
extern int strcasecmp(char const *a, char const *b);
extern int strncasecmp(char const *a, char const *b, size_t n);
#ifndef _stricmp
#define _stricmp strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp strncasecmp
#endif
#ifndef _wcsicmp
#define _wcsicmp(a, b) ustricmp((wchar_t const *)(a), (wchar_t const *)(b))
#endif
#endif

#ifndef VK_LBUTTON
#define VK_LBUTTON   0x01
#define VK_RBUTTON   0x02
#endif

#ifndef VK_BACK
#define VK_BACK      0x08
#define VK_TAB       0x09
#define VK_RETURN    0x0D
#define VK_SHIFT     0x10
#define VK_CONTROL   0x11
#define VK_ESCAPE    0x1B
#define VK_SPACE     0x20
#define VK_PRIOR     0x21
#define VK_NEXT      0x22
#define VK_END       0x23
#define VK_HOME      0x24
#define VK_LEFT      0x25
#define VK_UP        0x26
#define VK_RIGHT     0x27
#define VK_DOWN      0x28
#define VK_DELETE    0x2E
#define VK_F1        0x70
#define VK_F2        0x71
#define VK_F3        0x72
#define VK_F4        0x73
#define VK_F5        0x74
#define VK_F6        0x75
#define VK_OEM_PLUS  0xBB
#define VK_OEM_MINUS 0xBD
#define VK_OEM_4     0xDB
#define VK_OEM_6     0xDD
#endif

/* ---------- Forge Editor Modes */

enum forge_editor_mode
{
	_forge_mode_play = 0,         /* Normal Spartan / Player control */
	_forge_mode_3d_monitor = 1,   /* Mode 2: 3D In-Engine 343 Guilty Spark Monitor & Physics Beam */
	NUMBER_OF_FORGE_MODES
};

enum forge_ui_modal
{
	_forge_modal_none = 0,
	_forge_modal_spawn_palette,
	_forge_modal_object_properties,

	_forge_modal_save_copy_as
};

enum forge_palette_category
{
	_forge_cat_weapons = 0,
	_forge_cat_equipment,
	_forge_cat_vehicles,
	_forge_cat_bipeds,
	_forge_cat_scenery,
	_forge_cat_barriers_devices,
	_forge_cat_lights,
	_forge_cat_spawns_teleporters,
	_forge_cat_objectives,
	_forge_cat_ai_squads,
	NUMBER_OF_FORGE_PALETTE_CATEGORIES
};

#define _forge_cat_count NUMBER_OF_FORGE_PALETTE_CATEGORIES

enum forge_snap_angle_mode
{
	_forge_snap_off = 0,
	_forge_snap_15_deg,
	_forge_snap_45_deg,
	_forge_snap_90_deg,
	NUMBER_OF_FORGE_SNAP_MODES
};

enum forge_physics_mode
{
	_forge_physics_normal = 0,
	_forge_physics_fixed = 1,
	_forge_physics_phased = 2,
	NUMBER_OF_FORGE_PHYSICS_MODES = 3
};

enum forge_ai_order_mode
{
	_forge_ai_order_default = 0,
	_forge_ai_order_guard = 1,
	_forge_ai_order_patrol = 2,
	_forge_ai_order_charge = 3,
	_forge_ai_order_vehicle = 4,
	NUMBER_OF_FORGE_AI_ORDERS = 5
};

#define FORGE_MAX_WELD_GROUPS 16
#define NUMBER_OF_FORGE_LIGHT_COLORS 8
#define NUMBER_OF_FORGE_LIGHT_INTENSITIES 6
#define NUMBER_OF_FORGE_LIGHT_RADII 6

/* ---------- Runtime Palette Catalog Entry */

struct forge_catalog_entry
{
	uint32_t group_tag;
	uint8_t category;
	uint8_t source_cache_id; /* 0 = xbox_catalog, 1 = customforgeasset, 2 = custom cached [C], 3 = map-local [MAP], 4 = session-streamed [C] */
	uint16_t pad;
	char tag_path[FORGE_TAG_PATH_MAX];
	char display_name[64];
};

#define FORGE_MAX_CATALOG_ENTRIES 2048

/* ---------- Live Placed Instance Tracking */

struct forge_live_instance
{
	int32_t active;
	int32_t object_index;      /* Engine object datum index, or -1 if marker-only */
	int32_t actor_index;       /* Engine AI actor datum index, or -1 */
	int32_t edit_entry_index;  /* Index in edit_state */
	uint32_t group_tag;
	uint8_t source_cache_id;
	uint8_t is_ai_unit;
	uint8_t is_default_scenario_object; /* 1 if adopted from the base map's default scenario objects */
	uint8_t pad_inst;          /* 0x40 = awaiting streamed tag, 0x80 = client unlinked spawn flag, or structure_bsp_reference_index for spawns */
	uint8_t physics_mode;      /* 0 = Normal, 1 = Fixed, 2 = Phased */
	uint8_t ai_order;          /* 0 = Default, 1 = Guard, 2 = Patrol, 3 = Charge, 4 = Enter Vehicle */
	uint8_t group_id;          /* 0 = None, 1..16 = Weld Group */
	uint8_t light_color_preset;     /* 0=Default, 1=White, 2=Red, 3=Blue, 4=Green, 5=Amber, 6=Purple, 7=Cyan */
	uint8_t light_intensity_preset; /* 0=100%, 1=25%, 2=50%, 3=150%, 4=200%, 5=300% */
	uint8_t light_radius_preset;    /* 0=Default, 1=2u, 2=4u, 3=8u, 5=20u */
	uint8_t spawned_once;      /* 1 after initial runtime spawn (tracks weapon/eqip/vehi respawns) */
	uint8_t pad_inst2;
	uint16_t gametype_flags;   /* 0 = All Modes, bit0=CTF, bit1=Slayer, bit2=Oddball, bit3=KOTH, bit4=Race */
	int16_t netgame_flag_type; /* Scenario netgame_flag type (6=tele_from, 7=tele_to, 8=hill, 2=oddball, 0=ctf) or -1 */
	int16_t team_designator;   /* Reference/Channel ID or Team Index */
	int16_t respawn_timer_sec; /* Respawn timer in seconds (0 = Default/Never) */
	int16_t pad_respawn;
	long death_tick;           /* Engine game_time_get() tick when item picked up / unit died, or -1 */
	float orig_x, orig_y, orig_z; /* Original scenario position if is_default_scenario_object == 1 */
	float x, y, z;
	float yaw, pitch, roll;
	char tag_path[FORGE_TAG_PATH_MAX];
	char label[64];
};

#define FORGE_MAX_LIVE_INSTANCES 1024

/* ---------- Global Runtime Forge State */

struct forge_runtime_state
{
	int initialized;
	enum forge_editor_mode mode;
	enum forge_ui_modal active_modal;
	enum forge_palette_category current_category;
	int h3_menu_stage; /* 0 = Halo 3 category list, 1 = Halo 3 item list */
	int palette_cursor;
	int palette_scroll;
	int props_cursor;
	int props_scroll;
	int active_weld_group; /* 1..16 quick weld target for Ctrl+G */
	enum forge_snap_angle_mode snap_mode;
	int third_person_view; /* Toggleable via console `third_person` or F4 to inspect 343 Guilty Spark */

	/* Monitor 6DOF Free-Cam State */
	int monitor_cam_initialized;
	float cam_x, cam_y, cam_z;
	float cam_yaw, cam_pitch, cam_roll;
	float cam_speed;

	/* Physics Beam / Object Grab State */
	int32_t grabbed_object_index;
	int32_t grabbed_live_index;
	int grabbed_is_new_spawn;
	float grab_orig_x, grab_orig_y, grab_orig_z;
	float grab_orig_yaw, grab_orig_pitch, grab_orig_roll;
	float grab_distance;
	float grab_yaw, grab_pitch, grab_roll;
	int32_t hovered_object_index;
	int32_t hovered_live_index;
	char hovered_object_label[128];
	float ray_hit_x, ray_hit_y, ray_hit_z;
	int ray_hit_valid;

	/* 10-Point Linked KOTH Hill Circle Placement State */
	int koth_group_placing;
	int koth_group_hill_id;
	float koth_group_radius;
	int32_t koth_group_live_indices[10];

	/* Catalogs from cached_tags / forgeasset.map (0) and customforgeasset.map (1) */
	struct forge_catalog_entry catalog[FORGE_MAX_CATALOG_ENTRIES];
	int catalog_count;

	/* Save Map state: prompts for name on initial save, then quick-updates on subsequent saves */
	int has_saved_map_once;
	char save_map_name[64];
	int save_map_name_len;

	/* Status banner message */
	char status_message[128];
	uint32_t status_message_until_ms;

	/* Scenario Edit State & Live Instances */
	struct forge_scenario_edit_state edit_state;
	struct forge_live_instance live_instances[FORGE_MAX_LIVE_INSTANCES];
	int live_instance_count;

	/* Forge Controls & Controller State */
	int using_controller;
	int bind_vk[11];
	int bind_gp[11];
	float controller_look_sens;
	float controller_fly_speed;

	/* Per-Player Multiplayer Monitor vs. Spartan State & Orientation */
	uint8_t player_is_monitor[128];
	uint8_t player_has_monitor_pos[128];
	float player_monitor_pos[128][3];
	float player_monitor_yaw[128];
	float player_monitor_pitch[128];
	uint8_t player_has_render_pos[128];
	float player_monitor_render_pos[128][3];
	float player_monitor_render_yaw[128];
	float player_monitor_render_pitch[128];
};

enum forge_bind_action
{
	FORGE_BIND_TOGGLE_MODE = 0,
	FORGE_BIND_PALETTE = 1,
	FORGE_BIND_THIRD_PERSON = 2,
	FORGE_BIND_QUICK_SAVE = 3,
	FORGE_BIND_ROTATE_HOLD = 4,
	FORGE_BIND_SNAP_FLOOR = 5,
	FORGE_BIND_SNAP_SURFACE = 6,
	FORGE_BIND_ROT_SNAP = 7,
	FORGE_BIND_OBJECT_OPTIONS = 8,
	FORGE_BIND_DELETE_OBJECT = 9,
	FORGE_BIND_COPY_OBJECT = 10,
	FORGE_BIND_COUNT = 11
};

enum forge_gamepad_button
{
	FORGE_GP_NONE = 0,
	FORGE_GP_A = 1,
	FORGE_GP_B = 2,
	FORGE_GP_X = 3,
	FORGE_GP_Y = 4,
	FORGE_GP_BLACK = 5,
	FORGE_GP_WHITE = 6,
	FORGE_GP_LTRIGGER = 7,
	FORGE_GP_RTRIGGER = 8,
	FORGE_GP_DPAD_UP = 9,
	FORGE_GP_DPAD_DOWN = 10,
	FORGE_GP_DPAD_LEFT = 11,
	FORGE_GP_DPAD_RIGHT = 12,
	FORGE_GP_START = 13,
	FORGE_GP_BACK = 14,
	FORGE_GP_LTHUMB = 15,
	FORGE_GP_RTHUMB = 16,
	FORGE_GP_COUNT = 17
};

extern struct forge_runtime_state g_forge;

void forge_controls_init_defaults(void);
void forge_controls_load(void);
void forge_controls_save(void);
char const *forge_vk_display_name(int vk);
char const *forge_gp_display_name(int gp);
int forge_gp_button_down(int gp);
void forge_poll_controller_state(float *out_lx, float *out_ly, float *out_rx, float *out_ry, float *out_lt, float *out_rt);

/* ---------- Prototypes: forge_runtime_tags.c */

void forge_runtime_init(void);
void forge_runtime_reload_catalogs(void);
int forge_is_multiplayer_session(void);
int forge_catalog_entry_allowed(struct forge_catalog_entry const *entry);
uint32_t forge_get_base_map_tag_count(void);
long forge_resolve_engine_tag(uint32_t group_tag, char const *tag_path, uint32_t *out_actual_group);
int forge_clear_custom_tag_cache(void);
int forge_reset_stock_tag_cache(void);
int forge_ui_open_map_for_tag_browser(char const *map_path);
int forge_ui_map_tag_count(int category_tab);
int forge_ui_map_tag_get_info(int category_tab, int list_index, char *out_short_name, size_t short_sz, char *out_full_path, size_t path_sz, char *out_type_str, size_t type_sz, char *out_size_str, size_t size_sz);
int forge_ui_extract_selected_map_tag(int category_tab, int list_index, char const *custom_display_name);
int forge_ui_installed_tag_count(int category_tab);
int forge_ui_installed_tag_get_info(int category_tab, int list_index, char *out_display_name, size_t disp_sz, char *out_full_path, size_t path_sz, char *out_type_str, size_t type_sz, char *out_size_str, size_t size_sz, int *out_is_custom);
int forge_ui_installed_tag_delete(int category_tab, int list_index);
struct tag_injector_mesh;
int forge_ui_build_tag_preview_mesh(int is_installed_mode, int category_tab, int list_index, struct tag_injector_mesh *mesh);
int forge_ui_save_copy_as(char const *new_map_name);
void forge_trigger_save_map_action(void);
void forge_set_pending_sidecar_leaf(char const *leaf);
int forge_has_pending_or_active_sidecar(void);
void forge_load_saved_map_state_if_present(char const *map_name);
void forge_snap_to_scenario_placement_origin(uint32_t group_tag, char const *tag_path, float *io_x, float *io_y, float *io_z);
void forge_set_status(char const *fmt, ...);
void forge_draw_screen_rect(short x0, short y0, short x1, short y1, uint32_t argb);
int forge_monitor_render_player_unit(long object_index, void const *lighting, unsigned long flags);
void forge_render_world_3d_overlays(void const *lighting);
int forge_instance_supports_light_props(int32_t live_idx);
void forge_apply_instance_light_settings(int32_t live_idx);

/* ---------- Prototypes: forge_ai.c */

int32_t forge_ai_spawn_unit_live(
	char const *actor_variant_path,
	float x, float y, float z, float yaw,
	int16_t team_designator,
	int32_t *out_unit_index);
void forge_ai_update_unit_team(
	long actor_index,
	long unit_index,
	int16_t team_designator);
void forge_ai_apply_order(
	long actor_index,
	long unit_index,
	uint8_t ai_order,
	float home_x,
	float home_y,
	float home_z,
	long cur_tick);

/* ---------- Prototypes: forge_3d_mode.c */

void forge_3d_update(float dt);
void forge_3d_apply_monitor_camera(short local_player_index);
int forge_3d_fill_live_observer_result(void *out_observer_result);
int forge_is_main_menu_active(void);
void forge_3d_draw_hud(short local_player_index, short window_x0, short window_y0, short window_x1, short window_y1, long font_index, short line_height);
int32_t forge_3d_spawn_object_at_slot(
	int32_t forced_live_idx,
	uint32_t group_tag,
	char const *tag_path,
	uint8_t source_cache_id,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int16_t team_designator,
	int broadcast_net);
int32_t forge_3d_spawn_object_at(
	uint32_t group_tag,
	char const *tag_path,
	uint8_t source_cache_id,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int16_t team_designator,
	int broadcast_net);
void forge_3d_move_instance(int32_t live_idx, float x, float y, float z, float yaw, float pitch, float roll, int broadcast_net);
void forge_3d_move_instance_ext(int32_t live_idx, float x, float y, float z, float yaw, float pitch, float roll, int is_held, int broadcast_net);
void forge_3d_delete_instance(int32_t live_idx, int broadcast_net);
int forge_is_unit_in_monitor_mode(long unit_index);
int forge_get_local_player_abs_index(void);
void forge_render_begin_monitor_visibility(void);
void forge_render_end_monitor_visibility(void);
void forge_apply_host_player_monitor_state(int player_abs_idx, int is_monitor, float x, float y, float z, float yaw, float pitch);
int32_t forge_3d_adopt_or_find_object_instance(long object_index);
int32_t forge_3d_adopt_default_object_at_slot(
	int32_t forced_live_idx,
	uint32_t group_tag,
	char const *tag_path,
	float orig_x,
	float orig_y,
	float orig_z);
void forge_init_map_scenario_entities(void);
void forge_3d_reset_session_tracking(void);
void forge_sync_instance_extended_props_to_edit_state(int32_t live_idx);
void forge_sync_live_markers_to_scenario(uint32_t group_tag);
void forge_undo_clear(void);

/* ---------- Prototypes: forge_network.c */

void forge_network_reset_session(void);
void forge_network_sync_tick(void);
void forge_network_send_monitor_mode(int is_monitor, float x, float y, float z, float yaw, float pitch);
void forge_network_send_object_props(int32_t live_idx);
void forge_network_send_spawn_ext(
	int32_t live_idx,
	uint32_t group_tag,
	char const *tag_path,
	uint8_t source_cache_id,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int16_t team_designator,
	long host_obj_idx,
	long host_def_idx);
void forge_network_send_transform_ext(
	int32_t live_idx,
	long obj_idx,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int is_held,
	int immediate);
void forge_network_send_delete_ext(int32_t live_idx, long obj_idx);
int forge_network_intercept_chat_packet(long machine_index, char const *text, int is_from_client_to_host);
void forge_network_pre_handle_object_changes(void const *entries, short count);
void forge_network_post_handle_object_changes(void const *entries, short count);
void forge_runtime_on_map_unload(void);
int forge_is_tag_map_local_only(uint32_t group_tag, char const *tag_path);
int forge_cache_active_map_tag(uint32_t group_tag, char const *tag_path);
void forge_menu_custom_edition_maps_look_again(void);
void forge_normalize_mcc_hud_tags(void);
int forge_is_mcc_hud_active(void);
int forge_window_is_focused(void);
short forge_key_state(int vk);

/* ---------- Session-RAM-Only Custom Tag Streaming (Zero Disk Writes) */
uint32_t forge_compute_catalog_hash(uint32_t group_tag, char const *tag_path);
int forge_zlib_compress_buffer(uint8_t const *src, uint32_t src_len, uint8_t **out_dst, uint32_t *out_dst_len);
int forge_zlib_uncompress_exact(uint8_t const *src, uint32_t src_len, uint8_t *dst, uint32_t exact_dst_len);
int forge_export_custom_tag_bundle(
	uint32_t hash,
	uint8_t *out_format,
	uint32_t *out_group_tag,
	char *out_tag_path,
	size_t path_sz,
	char *out_display_name,
	size_t disp_sz,
	uint8_t **out_raw_bytes,
	uint32_t *out_raw_size);
uint32_t forge_get_streamed_tag_ram_used(void);
long forge_install_ephemeral_tag_bundle(
	uint32_t expected_hash,
	uint8_t bundle_format,
	uint32_t root_group,
	char const *root_tag_path,
	char const *display_name,
	uint8_t const *uncomp_bytes,
	uint32_t uncomp_size,
	uint32_t *out_actual_group);
long forge_create_engine_object_for_instance(int32_t live_idx, long def_idx, int is_held);
unsigned char forge_tag_stream_handles_message(void const *message, unsigned short size);
void forge_tag_stream_handle_message(long machine_index, void const *message, unsigned short size, unsigned char from_stream);
int forge_network_get_tag_download_status(char *out_text, size_t out_sz);

#undef GetAsyncKeyState
#define GetAsyncKeyState(vk) forge_key_state(vk)

#ifdef __cplusplus
}
#endif

#endif /* FORGE_ENGINE_H */
