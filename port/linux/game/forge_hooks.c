/*
 * OpenCE : Forge
 * Engine Symbol Bridge Hooks.
 */

#ifndef HALO_RELEASE
#define HALO_RELEASE
#endif
#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "camera/director.h"
#include "interface/hud.h"
#include "interface/hud_draw.h"
#include "interface/ui_widget.h"
#include "main/main.h"
#include "main/console.h"
#include "models/models.h"
#include "models/model_definitions.h"
#include "render/render.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "saved games/player_profile.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "text/unicode.h"
#include "network_chat.h"
#include "custom_edition_maps.h"
#include "tag_preview.h"
#include "text/text_group.h"
#include "tag_files/tag_groups.h"
#include <xtl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "forge_engine.h"

#undef fopen
#undef fclose
#undef fread
#undef fseek
#undef malloc
#undef free

#include "game/game_engine.h"
#include "objects/objects.h"
#include "units/units.h"
#include "networking/network_client_manager.h"
#include "../src/p2p.h"
#include "game_modes.h"

extern unsigned long system_milliseconds(void);
extern long hud_get_font_index(void);
extern void *cache_files_tag_instances(long *count);
extern void cache_files_set_tag_instances(void *instances, long count);
extern char const *pc_menu_function_name(long function_index);
extern char const *pc_menu_game_data_input_name(long function_index);
extern void main_menu_switch_to_single_player(void);
extern void game_connection_set(short connection);
extern void dispose_global_network_game_client(void);
extern void dispose_global_network_game_server(void);
extern void network_game_accept_remote_connections(boolean accept);
extern boolean network_game_is_active(void);
extern long player_ui_get_active_player_profile_index(short local_player_index);
extern void player_ui_get_active_player_profile(short local_player_index, struct player_profile *profile);
extern void player_ui_set_active_player_profile(short local_player_index, long profile_index, struct player_profile const *profile);
extern void player_ui_set_single_player_local_player_controller(short local_player_index, short controller_index);
extern void player_ui_remember_player1_profile(boolean remember);
extern void ui_play_audio_feedback_sound(short index);
extern void display_error_text_deferred(wchar_t const *error_text, short text_style);
extern short ui_widget_port_gametypes(long *indices, short maximum, short *last_used);
extern boolean ui_widget_port_gametype_choose(long profile_index);
extern boolean ui_widget_port_open(void *widget, char const *name, boolean *widget_deleted);
extern boolean ui_widget_port_game_variant_set(struct game_variant *variant, struct game_variant_options const *options);
extern struct game_variant *build_game_variant_slayer(struct game_variant *variant);
extern void game_variant_options_default(struct game_variant const *variant, struct game_variant_options *options);
extern boolean ui_widget_port_pointer_focused(void);
extern long player_ui_get_player1_last_used_profile_index(void);
extern short forge_key_state(int vk);

#define FORGE_MAP_KIND_FORGE_MAPS 6 /* 1 = MAP_KIND_MULTIPLAYER, 3 = MAP_KIND_CUSTOM_MULTIPLAYER, 4/5 = MCC, 6 = MAP_KIND_FORGE_MAPS */
static short s_forge_last_map_kind = 1;
static short s_forge_active_map_kind = 1;
static int s_is_forge_map_cache_valid = 0;
static short s_forge_only_ce_indices[4096];
static short s_forge_only_ce_count = 0;
static long s_save_forge_btn_tag = NONE;

/* Original engine symbols wrapped by Forge hooks */
extern void orig_network_chat_update(void);
extern void orig_network_chat_draw(short local_player_index);
extern boolean orig_network_chat_typing(void);
extern void orig_network_chat_handle_up(long machine_index, void const *entries, boolean from_stream);
extern void orig_network_chat_handle_down(void const *entries, boolean from_stream);
extern void orig_network_objects_handle_changes(void const *entries, short count);
extern void orig_director_update(real time_delta_sec);
extern director_perspective orig_director_get_perspective(short local_player_index);
extern struct observer_result const *orig_render_interpolation_camera(
	short local_player_index,
	struct observer_result const *fallback);
extern void orig_first_person_weapon_draw(void);
extern void orig_hud_render_weapon_interface(struct player_datum *player);
extern void orig_render_model(
	long model_index,
	real level_of_detail_pixels,
	struct real_matrix4x3 const *node_matrices,
	char const *region_permutation_indices,
	union real_rgb_color const *change_colors,
	real const *function_values,
	struct render_lighting const *lighting,
	union real_point3d const *centroid,
	real radius,
	struct render_model_effect const *model_effect,
	long unique_identifier,
	short forced_shader_permutation_index,
	unsigned long flags);
extern boolean orig_hs_compile_and_evaluate(char const *source);
extern void orig_hs_runtime_update(void);
extern void orig_cinematic_start(void);
extern boolean orig_scenario_trigger_volume_test_object(short trigger_volume_index, long object_index);
extern long orig_game_engine_remap_object_definition(long definition_index);
extern void orig_game_engine_post_rasterize_objects(void);
extern void rasterizer_models_begin(boolean sky);
extern void rasterizer_models_end(void);
extern void orig_menu_tags_loaded(char const *map_name);
extern void orig_menu_tags_unloaded(void);
extern boolean orig_pc_menu_event_function_invoke(
	void *widget,
	void *event,
	long function_index,
	boolean *widget_deleted);
extern void orig_pc_menu_game_data_function_invoke(
	void *widget,
	long function);

static unsigned long g_last_forge_frame_ms = 0;
static char g_last_forge_map_name[128] = "";

/* ---------- Widget Instance Layout (matches port/linux/game/menu_functions.c) */

struct forge_widget_instance
{
	long definition_tag_index;
	char const *name;
	short local_player_index;
	short horizontal_offset;
	short vertical_offset;
	short type;
	boolean visible;
	boolean render_regardless_of_controller_index;
	boolean disabled;
	boolean pause_game_time;
	boolean delete_recursion_lock;
	boolean widget_is_error_dialog;
	boolean close_if_local_player_controller_present;
	byte pad17;
	long creation_time;
	unsigned long milliseconds_to_auto_close;
	unsigned long auto_close_fade_time;
	real alpha_modifier;
	struct forge_widget_instance *previous;
	struct forge_widget_instance *next;
	struct forge_widget_instance *parent;
	struct forge_widget_instance *child;
	struct forge_widget_instance *focused_child;
	union
	{
		struct
		{
			wchar_t *text;
			short string_list_index;
		} text_box;
		struct
		{
			short selected_index;
			short last_list_tab_direction;
			void *list_items;
			word number_of_items;
			struct forge_widget_instance *extended_description;
			wchar_t *item_text;
		} list;
	} parameters;
	struct
	{
		short current_frame_index;
		short first_frame_index;
		short last_frame_index;
		short number_of_sprite_frames;
	} animation;
};

static struct forge_widget_instance *fw_descendant(struct forge_widget_instance *w, char const *name, long *nth)
{
	struct forge_widget_instance *c;
	for (c = w ? w->child : NULL; c; c = c->next)
	{
		struct forge_widget_instance *found;
		if (c->name && !strcmp(c->name, name) && (*nth)-- == 0)
			return c;
		found = fw_descendant(c, name, nth);
		if (found)
			return found;
	}
	return NULL;
}

static struct forge_widget_instance *fw_named(struct forge_widget_instance *w, char const *name, long nth)
{
	return w ? fw_descendant(w, name, &nth) : NULL;
}

static void fw_text_set_ascii(struct forge_widget_instance *tb, char const *ascii)
{
	int i;
	if (!tb || !ascii)
		return;
	if (tb->definition_tag_index != NONE)
	{
		uint8_t *def = (uint8_t *)tag_get('DeLa', tb->definition_tag_index);
		if (def)
			*(long *)(def + 0xF8) = NONE;
	}
	if (!tb->parameters.text_box.text)
	{
		tb->parameters.text_box.text = (wchar_t *)ui_widget_realloc(NULL, 256 * sizeof(wchar_t), __FILE__, __LINE__);
	}
	if (tb->parameters.text_box.text)
	{
		for (i = 0; ascii[i] && i < 254; i++)
			tb->parameters.text_box.text[i] = (wchar_t)(unsigned char)ascii[i];
		tb->parameters.text_box.text[i] = 0;
	}
}

/* ---------- Forge Edit Maps (User Created / Downloaded) Native Menus */

#define FORGE_MAX_EDIT_MAPS 64

static struct
{
	int tab; /* 0 = USER CREATED, 1 = DOWNLOADED */
	int count;
	int first;
	int chosen;
	char map_leaves[FORGE_MAX_EDIT_MAPS][64];
} s_forge_menu;

#ifdef HALO_WINDOWS
extern FILE *halo_windows_fopen(char const *filename, char const *mode);
#define forge_platform_fopen halo_windows_fopen
#else
extern FILE *halo_linux_fopen(char const *filename, char const *mode);
#define forge_platform_fopen halo_linux_fopen
#endif

static FILE *forge_open_data_file_rb(char const *rel_path)
{
	FILE *f;
	char d_path[512];

	if (!rel_path || !rel_path[0])
		return NULL;
	f = (FILE *)forge_host_fopen(rel_path, "rb");
	if (f)
		return f;
	if (_strnicmp(rel_path, "d:\\", 3) == 0 || _strnicmp(rel_path, "d:/", 3) == 0)
		return forge_platform_fopen(rel_path, "rb");
	snprintf(d_path, sizeof(d_path), "d:\\%s", rel_path);
	return forge_platform_fopen(d_path, "rb");
}

static int is_leaf_named_forge_suffix(char const *leaf_no_ext)
{
	size_t len;
	if (!leaf_no_ext)
		return 0;
	len = strlen(leaf_no_ext);
	if (len > 6 && _stricmp(leaf_no_ext + len - 6, "_forge") == 0)
		return 1;
	return 0;
}

static int is_forge_created_map_file(char const *map_path, char const *leaf_no_ext, int *out_is_user_created)
{
	FILE *f;
	uint8_t hdr[2048];
	uint32_t marker = 0;
	char txt_path[512];
	int has_flg1 = 0;
	int user_created = 0;

	f = forge_open_data_file_rb(map_path);
	if (!f && leaf_no_ext && leaf_no_ext[0])
	{
		snprintf(txt_path, sizeof(txt_path), "custom_maps/%s.map", leaf_no_ext);
		f = forge_open_data_file_rb(txt_path);
	}
	if (f)
	{
		if (fread(hdr, 1, sizeof(hdr), f) == sizeof(hdr))
		{
			uint32_t state_off = *(uint32_t const *)(hdr + 0x1E4);
			uint32_t state_sz = *(uint32_t const *)(hdr + 0x1E8);
			marker = *(uint32_t const *)(hdr + 0x1FC);
			if (marker == 0x464C4731u) /* 'FLG1' */
			{
				has_flg1 = 1;
				user_created = 1;
				if (state_off >= 0x800u && state_sz > 0 && state_sz < 4u * 1024u * 1024u)
				{
					char buf[1024];
					uint32_t rsz = (state_sz < sizeof(buf) - 1u) ? state_sz : (uint32_t)(sizeof(buf) - 1u);
					if (fseek(f, (long)state_off, SEEK_SET) == 0 && fread(buf, 1, rsz, f) == rsz)
					{
						buf[rsz] = 0;
						if (strstr(buf, "[OpenCE Forge Map]") != NULL)
							has_flg1 = 1;
						if (strstr(buf, "ForgeSource=UserCreated") != NULL)
							user_created = 1;
					}
				}
			}
			else if (is_leaf_named_forge_suffix(leaf_no_ext))
			{
				has_flg1 = 1;
				user_created = 1;
			}
		}
		fclose(f);
	}

	snprintf(txt_path, sizeof(txt_path), "custom_maps/%s.txt", leaf_no_ext);
	f = forge_open_data_file_rb(txt_path);
	if (f)
	{
		char buf[1024];
		size_t n = fread(buf, 1, sizeof(buf) - 1, f);
		buf[n] = 0;
		fclose(f);
		if (strstr(buf, "[OpenCE Forge Map]") != NULL)
			has_flg1 = 1;
		if (strstr(buf, "ForgeSource=UserCreated") != NULL)
			user_created = 1;
	}

	if (out_is_user_created)
		*out_is_user_created = user_created;
	return has_flg1;
}

static int is_custom_map_leaf_forge_made(char const *leaf_no_ext)
{
	char check_path[512];

	if (!leaf_no_ext || !leaf_no_ext[0])
		return 0;
	if (is_leaf_named_forge_suffix(leaf_no_ext))
		return 1;
	snprintf(check_path, sizeof(check_path), "custom_maps/%s.map", leaf_no_ext);
	return is_forge_created_map_file(check_path, leaf_no_ext, NULL);
}

static void ensure_forge_ce_map_partition_built(void)
{
	short total_ce, i;

	if (s_is_forge_map_cache_valid)
		return;
	s_is_forge_map_cache_valid = 1;
	s_forge_only_ce_count = 0;

	total_ce = custom_edition_maps_count(FALSE);
	for (i = 0; i < total_ce; i++)
	{
		short disp = custom_edition_maps_display_index_of(FALSE, i);
		char const *lvl = custom_edition_maps_level_name(disp);
		char const *leaf = lvl;
		if (lvl)
		{
			char const *bs = strrchr(lvl, '\\');
			char const *fs = strrchr(lvl, '/');
			if (bs && (!fs || bs > fs))
				leaf = bs + 1;
			else if (fs)
				leaf = fs + 1;
		}
		if (leaf && is_custom_map_leaf_forge_made(leaf))
		{
			if (s_forge_only_ce_count < 4096)
				s_forge_only_ce_indices[s_forge_only_ce_count++] = i;
		}
	}
}

void forge_menu_custom_edition_maps_look_again(void)
{
	s_is_forge_map_cache_valid = 0;
	custom_edition_maps_look_again();
}

short forge_menu_custom_edition_maps_count(boolean campaign)
{
	if (campaign)
		return custom_edition_maps_count(campaign);
	if (s_forge_active_map_kind == FORGE_MAP_KIND_FORGE_MAPS)
	{
		ensure_forge_ce_map_partition_built();
		return s_forge_only_ce_count;
	}
	return custom_edition_maps_count(FALSE);
}

short forge_menu_custom_edition_maps_display_index_of(boolean campaign, short index)
{
	if (campaign)
		return custom_edition_maps_display_index_of(campaign, index);
	if (s_forge_active_map_kind == FORGE_MAP_KIND_FORGE_MAPS)
	{
		ensure_forge_ce_map_partition_built();
		if (index < 0 || index >= s_forge_only_ce_count)
			return NONE;
		return custom_edition_maps_display_index_of(FALSE, s_forge_only_ce_indices[index]);
	}
	return custom_edition_maps_display_index_of(FALSE, index);
}

extern short ui_widget_port_multiplayer_maps(char const *const **names, short *last_used);
extern boolean ui_widget_port_multiplayer_map_choose(short level_index);

static short s_last_mp_maps_returned = 0;

short forge_menu_multiplayer_maps(char const *const **names, short *last_used)
{
	short total = ui_widget_port_multiplayer_maps(names, last_used);
	short all_ce = custom_edition_maps_count(FALSE);
	short xbox_cnt = (short)(total > all_ce ? (total - all_ce) : 13);
	short part_cnt;

	/* Map screen always initializes on MAP_KIND_MULTIPLAYER (1) */
	s_forge_active_map_kind = 1;
	s_forge_last_map_kind = 1;
	part_cnt = forge_menu_custom_edition_maps_count(FALSE);
	if (last_used && *last_used >= xbox_cnt)
		*last_used = 0;
	s_last_mp_maps_returned = (short)(xbox_cnt + part_cnt);
	return s_last_mp_maps_returned;
}

boolean forge_menu_multiplayer_map_choose(short level_index)
{
	char const *const *names = NULL;
	short last_used = 0;
	short total = ui_widget_port_multiplayer_maps(&names, &last_used);
	short all_ce = custom_edition_maps_count(FALSE);
	short xbox_cnt = (short)(total > all_ce ? (total - all_ce) : 13);

	ensure_forge_ce_map_partition_built();

	/* In menu_functions.c (map_list_choose), when kind is CUSTOM MULTIPLAYER (3) or FORGE MAPS (6),
	 * it passes: xbox_multiplayer_map_count(map_list.map_count) + chosen, where
	 * xbox_multiplayer_map_count(c) = MAX(c - forge_menu_custom_edition_maps_count(FALSE), 0).
	 * Recover the exact 0-based `chosen` index regardless of how many Forge or non-Forge CE maps exist! */
	if (s_forge_active_map_kind == FORGE_MAP_KIND_FORGE_MAPS || s_forge_active_map_kind == 3)
	{
		short active_ce = forge_menu_custom_edition_maps_count(FALSE);
		short mp_count = (s_last_mp_maps_returned > 0) ? s_last_mp_maps_returned : (short)(xbox_cnt + all_ce);
		short base = (short)((mp_count > active_ce) ? (mp_count - active_ce) : 0);
		short chosen = (short)(level_index - base);

		if (s_forge_active_map_kind == FORGE_MAP_KIND_FORGE_MAPS)
		{
			if (chosen < 0 || chosen >= s_forge_only_ce_count)
				return FALSE;
			level_index = (short)(xbox_cnt + s_forge_only_ce_indices[chosen]);
		}
		else
		{
			if (chosen < 0 || chosen >= all_ce)
				return FALSE;
			level_index = (short)(xbox_cnt + chosen);
		}
	}
	return ui_widget_port_multiplayer_map_choose(level_index);
}

static void scan_forge_edit_maps(int wanted_tab)
{
	s_forge_menu.tab = wanted_tab;
	s_forge_menu.count = 0;
	s_forge_menu.first = 0;
	s_forge_menu.chosen = 0;

	/* Scan Custom Edition maps discovered by the engine (works regardless of CWD) */
	{
		short total_ce = custom_edition_maps_count(FALSE);
		short i;
		for (i = 0; i < total_ce; i++)
		{
			short disp = custom_edition_maps_display_index_of(FALSE, i);
			char const *lvl = custom_edition_maps_level_name(disp);
			char const *leaf = lvl;
			char full_path[512];
			int is_user = 0;
			int k, already_listed = 0;

			if (!lvl || !lvl[0])
				continue;
			{
				char const *bs = strrchr(lvl, '\\');
				char const *fs = strrchr(lvl, '/');
				if (bs && (!fs || bs > fs))
					leaf = bs + 1;
				else if (fs)
					leaf = fs + 1;
			}
			if (!leaf || !leaf[0])
				continue;

			for (k = 0; k < s_forge_menu.count; k++)
			{
				if (_stricmp(s_forge_menu.map_leaves[k], leaf) == 0)
				{
					already_listed = 1;
					break;
				}
			}
			if (already_listed)
				continue;

			snprintf(full_path, sizeof(full_path), "custom_maps/%s.map", leaf);
			if (is_forge_created_map_file(full_path, leaf, &is_user))
			{
				if ((wanted_tab == 0 && is_user) || (wanted_tab == 1 && !is_user))
				{
					if (s_forge_menu.count < FORGE_MAX_EDIT_MAPS)
					{
						int idx = s_forge_menu.count++;
						snprintf(s_forge_menu.map_leaves[idx], sizeof(s_forge_menu.map_leaves[0]), "%s", leaf);
					}
				}
			}
		}
	}
}

static void update_forge_edit_maps_widget(struct forge_widget_instance *list)
{
	struct forge_widget_instance *spinner = fw_named(list, "edit_maps_kind_spinner", 0);
	struct forge_widget_instance *desc = list->parameters.list.extended_description;
	struct forge_widget_instance *child;
	int kind = spinner ? (int)spinner->parameters.list.selected_index : 0;
	int row_idx = 0;
	int focused_entry = -1;

	if (kind < 0) kind = 0;
	if (kind > 1) kind = 1;
	if (kind != s_forge_menu.tab)
	{
		scan_forge_edit_maps(kind);
	}

	for (child = list->child; child; child = child->next, row_idx++)
	{
		int item_slot;
		if (row_idx < 1)
			continue;
		if (!child->name || strncmp(child->name, "list_item_", 10) != 0)
			break;
		item_slot = s_forge_menu.first + (row_idx - 1);
		if (s_forge_menu.count == 0)
		{
			child->visible = (row_idx == 1) ? TRUE : FALSE;
			if (row_idx == 1)
			{
				fw_text_set_ascii(
					fw_named(child, "list_item_text", 0),
					kind == 0 ? "(No User Created Maps Yet)" : "(No Downloaded Forge Maps)");
			}
		}
		else
		{
			child->visible = (item_slot < s_forge_menu.count) ? TRUE : FALSE;
			if (item_slot < s_forge_menu.count)
			{
				fw_text_set_ascii(fw_named(child, "list_item_text", 0), s_forge_menu.map_leaves[item_slot]);
				if (child == list->focused_child)
					focused_entry = item_slot;
			}
		}
		if (fw_named(child, "list_item_arrows", 0))
			fw_named(child, "list_item_arrows", 0)->visible = (child == list->focused_child);
		if (fw_named(child, "scroll_up_button", 0))
			fw_named(child, "scroll_up_button", 0)->visible = FALSE;
		if (fw_named(child, "scroll_down_button", 0))
			fw_named(child, "scroll_down_button", 0)->visible = FALSE;
	}

	if (focused_entry >= 0 && focused_entry < s_forge_menu.count)
		s_forge_menu.chosen = focused_entry;

	if (desc)
	{
		struct forge_widget_instance *sp_name = fw_named(desc, "replay_level_right_name", 0);
		struct forge_widget_instance *sp_pic = fw_named(desc, "replay_level_right_pic", 0);
		struct forge_widget_instance *sp_data = fw_named(desc, "replay_level_right_data", 0);
		struct forge_widget_instance *mp_name = fw_named(desc, "mp_map_right_name", 0);
		struct forge_widget_instance *mp_pic = fw_named(desc, "mp_map_right_pic", 0);
		struct forge_widget_instance *mp_data = fw_named(desc, "mp_map_right_data", 0);
		long d_idx;

		/* Always hide the single-player right-hand widgets ('Truth and Reconciliation' default) and difficulty icons */
		if (sp_name) sp_name->visible = FALSE;
		if (sp_pic) sp_pic->visible = FALSE;
		if (sp_data) sp_data->visible = FALSE;
		for (d_idx = 0; d_idx < 3; d_idx++)
		{
			struct forge_widget_instance *di = fw_named(desc, "difficulty_indicator", d_idx);
			if (di) di->visible = FALSE;
		}

		if (s_forge_menu.count > 0 && s_forge_menu.chosen >= 0 && s_forge_menu.chosen < s_forge_menu.count)
		{
			char level_name[128];
			char info[192];
			short disp_idx;

			snprintf(level_name, sizeof(level_name), "custom_maps\\%s", s_forge_menu.map_leaves[s_forge_menu.chosen]);
			disp_idx = custom_edition_maps_display_index(level_name);
			snprintf(info, sizeof(info), "%s\r\n\r\nMap: %.16s\r\n\r\nPress OK to edit\r\nin Forge.",
				s_forge_menu.tab == 0 ? "User Created Map" : "Downloaded Map",
				s_forge_menu.map_leaves[s_forge_menu.chosen]);

			if (disp_idx != NONE)
			{
				wchar_t *desc_w = custom_edition_maps_description(disp_idx);
				if (desc_w)
				{
					int k;
					for (k = 0; info[k] && k < 254; k++)
						desc_w[k] = (wchar_t)(unsigned char)info[k];
					desc_w[k] = 0;
				}
				if (mp_name)
				{
					mp_name->visible = TRUE;
					mp_name->parameters.text_box.string_list_index = disp_idx;
				}
				if (mp_pic)
				{
					mp_pic->visible = TRUE;
					mp_pic->animation.current_frame_index = disp_idx;
				}
				if (mp_data)
				{
					mp_data->visible = TRUE;
					mp_data->parameters.text_box.string_list_index = disp_idx;
				}
			}
			else
			{
				if (mp_name) mp_name->visible = FALSE;
				if (mp_pic) mp_pic->visible = FALSE;
				if (mp_data) mp_data->visible = FALSE;
			}
		}
		else
		{
			if (mp_name) mp_name->visible = FALSE;
			if (mp_pic) mp_pic->visible = FALSE;
			if (mp_data) mp_data->visible = FALSE;
		}
	}
}

/* ---------- Forge Tag Options Browsers (ADD TAGS / INSTALLED TAGS / NAME TAG / DELETE / RESET) */

static struct
{
	int kind; /* 0 = Custom SP, 1 = Custom MP */
	int first;
	int chosen;
	int preserve_on_return;
} s_add_tags_map = { 1, 0, 0, 0 };

static struct
{
	int cat; /* 0 = Scenery, 1 = Vehicles, 2 = Devices, 3 = Weapons, 4 = Equipment */
	int first;
	int chosen;
	int preserve_on_return;
	int name_box_active;
	int dialog_open;
	unsigned long last_dialog_ms;
	unsigned long bs_next_ms;
	char custom_name[64];
	int prev_vk[256];
	char status_msg[128];
} s_tag_browser = { 0, 0, 0, 0, 0, 0, 0, 0, "", { 0 }, "" };

static struct
{
	int cat; /* 0 = Scenery, 1 = Vehicles, 2 = Devices, 3 = Weapons, 4 = Equipment */
	int first;
	int chosen;
	int preserve_on_return;
} s_inst_browser = { 0, 0, 0, 0 };

static int s_preview_rev = 1;

static void fw_text_set_wide(struct forge_widget_instance *tb, wchar_t const *wstr)
{
	int i;
	if (!tb || !wstr)
		return;
	if (tb->definition_tag_index != NONE)
	{
		uint8_t *def = (uint8_t *)tag_get('DeLa', tb->definition_tag_index);
		if (def)
			*(long *)(def + 0xF8) = NONE;
	}
	if (!tb->parameters.text_box.text)
	{
		tb->parameters.text_box.text = (wchar_t *)ui_widget_realloc(NULL, 256 * sizeof(wchar_t), __FILE__, __LINE__);
	}
	if (tb->parameters.text_box.text)
	{
		for (i = 0; wstr[i] && i < 254; i++)
			tb->parameters.text_box.text[i] = wstr[i];
		tb->parameters.text_box.text[i] = 0;
	}
}

static void fw_profile_name_show(struct forge_widget_instance *desc)
{
	static struct player_profile s_prof;
	static long s_read_idx = NONE;
	static unsigned long s_read_time = 0;
	static boolean s_read_good = FALSE;
	long idx;

	if (!desc)
		return;
	idx = player_ui_get_active_player_profile_index(0);
	if (idx != NONE)
	{
		player_ui_get_active_player_profile(0, &s_prof);
		s_read_idx = NONE;
	}
	else if ((idx = player_ui_get_player1_last_used_profile_index()) == NONE)
	{
		return;
	}
	else
	{
		if (idx != s_read_idx || system_milliseconds() - s_read_time > 1000)
		{
			s_read_idx = idx;
			s_read_good = player_profile_get(idx, &s_prof);
			s_read_time = system_milliseconds();
		}
		if (!s_read_good)
			return;
	}
	fw_text_set_wide(fw_named(desc, "current_profile_name", 0), s_prof.player_name);
}

static int fw_focused_row(struct forge_widget_instance *list)
{
	struct forge_widget_instance *child;
	int idx = 0;
	for (child = list ? list->child : NULL; child && idx < 11; child = child->next, idx++)
	{
		if (child == list->focused_child)
		{
			if (idx == 0)
				return 0;
			return (child->name && strncmp(child->name, "list_item_", 10) == 0) ? idx : -1;
		}
	}
	return -1;
}

static void fw_focus_row(struct forge_widget_instance *list, int row)
{
	struct forge_widget_instance *child = list ? list->child : NULL;
	int idx;
	for (idx = 0; child && child->next && idx < row; idx++)
		child = child->next;
	if (row < 0)
	{
		while (child && child->next)
			child = child->next;
	}
	if (child && list)
	{
		list->focused_child = child;
		list->parameters.list.selected_index = (short)(row < 0 ? 0 : row);
	}
}

static void fw_scroll_rows_at_edges(struct forge_widget_instance *list, int *first, int count)
{
	int shown = (count < 10) ? count : 10;
	int row = fw_focused_row(list);

	if (count <= 0)
	{
		*first = 0;
		return;
	}
	if (*first > count - shown)
		*first = (count > shown) ? (count - shown) : 0;
	if (*first < 0)
		*first = 0;

	if (!ui_widget_port_pointer_focused())
	{
		if (row >= 1 && row == shown && *first + shown < count)
		{
			(*first)++;
			fw_focus_row(list, --row);
		}
		else if (row == 1 && *first > 0)
		{
			(*first)--;
			fw_focus_row(list, ++row);
		}
	}
}

static void fw_wrap_label(char const *src, int max_cols, int max_lines, char *out, size_t out_sz)
{
	size_t oi = 0;
	int col = 0;
	int line = 1;
	char const *p = src ? src : "";

	if (!out || out_sz == 0)
		return;
	out[0] = 0;
	if (max_cols < 4)
		max_cols = 4;
	if (max_lines < 1)
		max_lines = 1;

	while (*p && oi + 4 < out_sz)
	{
		if (col >= max_cols)
		{
			if (line >= max_lines)
				break;
			out[oi++] = '\r';
			out[oi++] = '\n';
			line++;
			col = 0;
			while (*p == ' ' || *p == '_')
				p++;
			if (!*p)
				break;
		}
		if ((*p == ' ' || *p == '_' || *p == '-') && col >= max_cols - 6 && line < max_lines)
		{
			char const *next = (*p == '-') ? (p + 1) : (p + 1);
			int next_word_len = 0;
			while (next[next_word_len] && next[next_word_len] != ' ' &&
				   next[next_word_len] != '_' && next[next_word_len] != '-')
			{
				next_word_len++;
			}
			if (col + 1 + next_word_len > max_cols)
			{
				if (*p == '-')
					out[oi++] = '-';
				out[oi++] = '\r';
				out[oi++] = '\n';
				line++;
				col = 0;
				p++;
				continue;
			}
		}
		out[oi++] = *p++;
		col++;
	}
	out[oi] = 0;
}

static int add_tags_map_kind_count(int kind)
{
	return (int)custom_edition_maps_count(kind == 0 ? TRUE : FALSE);
}

static int resolve_add_tags_map_path(int kind, int entry, char *out_path, size_t out_sz)
{
	short disp;
	char const *lvl = NULL;
	char const *leaf = NULL;
	FILE *tf;

	if (!out_path || out_sz == 0 || entry < 0 || entry >= add_tags_map_kind_count(kind))
		return 0;
	out_path[0] = 0;

	disp = custom_edition_maps_display_index_of(kind == 0 ? TRUE : FALSE, (short)entry);
	lvl = custom_edition_maps_level_name(disp);
	if (!lvl || !lvl[0])
		return 0;

	leaf = strrchr(lvl, '\\');
	if (!leaf)
		leaf = strrchr(lvl, '/');
	leaf = leaf ? (leaf + 1) : lvl;

	snprintf(out_path, out_sz, "%s.map", lvl);
	tf = fopen(out_path, "rb");
	if (tf)
	{
		fclose(tf);
		return 1;
	}
	snprintf(out_path, out_sz, "custom_maps\\%s.map", leaf);
	tf = fopen(out_path, "rb");
	if (tf)
	{
		fclose(tf);
		return 1;
	}
	return 0;
}

static void update_forge_add_tags_map_widget(struct forge_widget_instance *list)
{
	struct forge_widget_instance *spinner = fw_named(list, "forge_add_tags_map_kind_spinner", 0);
	struct forge_widget_instance *desc = list ? list->parameters.list.extended_description : NULL;
	struct forge_widget_instance *child;
	int kind = spinner ? (int)spinner->parameters.list.selected_index : 1;
	int count;
	int row_idx = 0;
	int focused_entry = -1;

	if (kind < 0) kind = 0;
	if (kind > 1) kind = 1;
	if (kind != s_add_tags_map.kind)
	{
		s_add_tags_map.kind = kind;
		s_add_tags_map.first = 0;
		s_add_tags_map.chosen = 0;
		if (fw_focused_row(list) >= 1)
			fw_focus_row(list, 1);
	}

	count = add_tags_map_kind_count(kind);
	if (s_add_tags_map.chosen >= count)
		s_add_tags_map.chosen = (count > 0) ? (count - 1) : 0;
	if (s_add_tags_map.chosen < 0)
		s_add_tags_map.chosen = 0;

	fw_scroll_rows_at_edges(list, &s_add_tags_map.first, count);

	for (child = list ? list->child : NULL; child; child = child->next, row_idx++)
	{
		int item_slot;
		if (row_idx < 1)
			continue;
		if (!child->name || strncmp(child->name, "list_item_", 10) != 0)
			break;
		item_slot = s_add_tags_map.first + (row_idx - 1);
		if (count == 0)
		{
			child->visible = (row_idx == 1) ? TRUE : FALSE;
			if (row_idx == 1)
				fw_text_set_ascii(fw_named(child, "list_item_text", 0), "(No Maps in Category)");
			else if (child == list->focused_child)
				fw_focus_row(list, 1);
		}
		else
		{
			child->visible = (item_slot < count) ? TRUE : FALSE;
			if (item_slot < count)
			{
				wchar_t wtext[64];
				short disp = custom_edition_maps_display_index_of(kind == 0 ? TRUE : FALSE, (short)item_slot);
				wchar_t const *nm = custom_edition_maps_name(disp);
				wtext[0] = 0;
				if (nm)
				{
					ustrncpy(wtext, nm, 63);
					wtext[63] = 0;
				}
				fw_text_set_wide(fw_named(child, "list_item_text", 0), wtext);
				if (child == list->focused_child)
					focused_entry = item_slot;
			}
			else if (child == list->focused_child)
			{
				int shown = (count < 10) ? count : 10;
				fw_focus_row(list, (shown > 0) ? shown : 1);
			}
		}
		if (fw_named(child, "list_item_arrows", 0))
			fw_named(child, "list_item_arrows", 0)->visible =
				(child == list->focused_child || (count > 0 && item_slot == s_add_tags_map.chosen));
		if (fw_named(child, "scroll_up_button", 0))
			fw_named(child, "scroll_up_button", 0)->visible = FALSE;
		if (fw_named(child, "scroll_down_button", 0))
			fw_named(child, "scroll_down_button", 0)->visible = FALSE;
	}

	if (focused_entry >= 0 && focused_entry < count)
		s_add_tags_map.chosen = focused_entry;

	if (desc)
	{
		struct forge_widget_instance *sp_name = fw_named(desc, "replay_level_right_name", 0);
		struct forge_widget_instance *sp_pic = fw_named(desc, "replay_level_right_pic", 0);
		struct forge_widget_instance *sp_data = fw_named(desc, "replay_level_right_data", 0);
		struct forge_widget_instance *mp_name = fw_named(desc, "mp_map_right_name", 0);
		struct forge_widget_instance *mp_pic = fw_named(desc, "mp_map_right_pic", 0);
		struct forge_widget_instance *mp_data = fw_named(desc, "mp_map_right_data", 0);
		long d_idx;

		for (d_idx = 0; d_idx < 3; d_idx++)
		{
			struct forge_widget_instance *di = fw_named(desc, "difficulty_indicator", d_idx);
			if (di) di->visible = FALSE;
		}
		fw_profile_name_show(desc);

		if (count > 0 && s_add_tags_map.chosen >= 0 && s_add_tags_map.chosen < count)
		{
			short disp = custom_edition_maps_display_index_of(kind == 0 ? TRUE : FALSE, (short)s_add_tags_map.chosen);
			if (sp_name) sp_name->visible = FALSE;
			if (sp_pic) sp_pic->visible = FALSE;
			if (sp_data) sp_data->visible = FALSE;
			if (mp_name) { mp_name->visible = (disp != NONE); mp_name->parameters.text_box.string_list_index = disp; }
			if (mp_pic) { mp_pic->visible = (disp != NONE); mp_pic->animation.current_frame_index = disp; }
			if (mp_data) { mp_data->visible = (disp != NONE); mp_data->parameters.text_box.string_list_index = disp; }
		}
		else
		{
			if (sp_name) sp_name->visible = FALSE;
			if (sp_pic) sp_pic->visible = FALSE;
			if (sp_data) sp_data->visible = FALSE;
			if (mp_name) mp_name->visible = FALSE;
			if (mp_pic) mp_pic->visible = FALSE;
			if (mp_data) mp_data->visible = FALSE;
		}
	}
}

static void update_forge_add_tags_browser_widget(struct forge_widget_instance *list)
{
	struct forge_widget_instance *spinner = fw_named(list, "forge_tag_cat_spinner", 0);
	struct forge_widget_instance *desc = list ? list->parameters.list.extended_description : NULL;
	struct forge_widget_instance *child;
	int cat = spinner ? (int)spinner->parameters.list.selected_index : 0;
	int count;
	int row_idx = 0;
	int focused_entry = -1;

	if (cat < 0) cat = 0;
	if (cat > 4) cat = 4;
	if (cat != s_tag_browser.cat)
	{
		s_tag_browser.cat = cat;
		s_tag_browser.first = 0;
		s_tag_browser.chosen = 0;
		s_tag_browser.status_msg[0] = 0;
		s_preview_rev++;
		if (fw_focused_row(list) >= 1)
			fw_focus_row(list, 1);
	}

	count = forge_ui_map_tag_count(cat);
	if (s_tag_browser.chosen >= count)
		s_tag_browser.chosen = (count > 0) ? (count - 1) : 0;
	if (s_tag_browser.chosen < 0)
		s_tag_browser.chosen = 0;

	fw_scroll_rows_at_edges(list, &s_tag_browser.first, count);

	for (child = list ? list->child : NULL; child; child = child->next, row_idx++)
	{
		int item_slot;
		if (row_idx < 1)
			continue;
		if (!child->name || strncmp(child->name, "list_item_", 10) != 0)
			break;
		item_slot = s_tag_browser.first + (row_idx - 1);
		if (count == 0)
		{
			child->visible = (row_idx == 1) ? TRUE : FALSE;
			if (row_idx == 1)
				fw_text_set_ascii(fw_named(child, "list_item_text", 0), "(No Tags in Category)");
			else if (child == list->focused_child)
				fw_focus_row(list, 1);
		}
		else
		{
			child->visible = (item_slot < count) ? TRUE : FALSE;
			if (item_slot < count)
			{
				char short_name[64];
				short_name[0] = 0;
				forge_ui_map_tag_get_info(cat, item_slot, short_name, sizeof(short_name), NULL, 0, NULL, 0, NULL, 0);
				fw_text_set_ascii(fw_named(child, "list_item_text", 0), short_name);
				if (child == list->focused_child)
					focused_entry = item_slot;
			}
			else if (child == list->focused_child)
			{
				int shown = (count < 10) ? count : 10;
				fw_focus_row(list, (shown > 0) ? shown : 1);
			}
		}
		if (fw_named(child, "list_item_arrows", 0))
			fw_named(child, "list_item_arrows", 0)->visible =
				(child == list->focused_child || (count > 0 && item_slot == s_tag_browser.chosen));
		if (fw_named(child, "scroll_up_button", 0))
			fw_named(child, "scroll_up_button", 0)->visible = FALSE;
		if (fw_named(child, "scroll_down_button", 0))
			fw_named(child, "scroll_down_button", 0)->visible = FALSE;
	}

	if (focused_entry >= 0 && focused_entry < count)
		s_tag_browser.chosen = focused_entry;

	if (desc)
	{
		struct forge_widget_instance *r_name = fw_named(desc, "forge_tag_right_name", 0);
		struct forge_widget_instance *r_data = fw_named(desc, "forge_tag_right_data", 0);
		fw_profile_name_show(desc);

		if (count > 0 && s_tag_browser.chosen >= 0 && s_tag_browser.chosen < count)
		{
			char short_name[64];
			char wrapped_title[96];
			char wrapped_item[96];
			char full_path[256];
			char type_str[64];
			char size_str[64];
			char info_buf[256];

			short_name[0] = 0;
			full_path[0] = 0;
			type_str[0] = 0;
			size_str[0] = 0;
			forge_ui_map_tag_get_info(
				cat,
				s_tag_browser.chosen,
				short_name, sizeof(short_name),
				full_path, sizeof(full_path),
				type_str, sizeof(type_str),
				size_str, sizeof(size_str));

			tag_preview_select(0, cat, s_tag_browser.chosen, s_preview_rev);
			fw_wrap_label(short_name, 14, 2, wrapped_title, sizeof(wrapped_title));
			fw_wrap_label(short_name, 16, 2, wrapped_item, sizeof(wrapped_item));
			fw_text_set_ascii(r_name, wrapped_title);

			if (s_tag_browser.status_msg[0])
			{
				snprintf(info_buf, sizeof(info_buf),
					"- %s\r\n- %s\r\n- %s\r\n\r\n%s",
					wrapped_item, type_str, size_str, s_tag_browser.status_msg);
			}
			else
			{
				snprintf(info_buf, sizeof(info_buf),
					"- %s\r\n- %s\r\n- %s",
					wrapped_item, type_str, size_str);
			}
			fw_text_set_ascii(r_data, info_buf);
		}
		else
		{
			tag_preview_clear();
			fw_text_set_ascii(r_name, "NO TAGS");
			fw_text_set_ascii(r_data, "- None\r\n- None\r\n- 0 KB");
		}
	}
}

static void update_forge_name_tag_dialog(struct forge_widget_instance *bar)
{
	struct forge_widget_instance *screen = bar;
	struct forge_widget_instance *tb;
	struct forge_widget_instance *input_btn;
	struct forge_widget_instance *confirm_btn;
	struct forge_widget_instance *back_btn;
	char buf[256];
	int vk;
	int shift_down;
	size_t len;
	unsigned long now_ms = system_milliseconds();

	while (screen && screen->parent)
		screen = screen->parent;
	tb = fw_named(screen, "forge_name_tag_text_box", 0);
	input_btn = fw_named(screen, "forge_name_tag_input_btn", 0);
	confirm_btn = fw_named(screen, "forge_name_tag_confirm_btn", 0);
	back_btn = fw_named(screen, "forge_name_tag_back_btn", 0);

	s_tag_browser.dialog_open = 1;
	s_tag_browser.last_dialog_ms = now_ms;

	shift_down = (forge_key_state(VK_SHIFT) & 0x8000) != 0;
	len = strlen(s_tag_browser.custom_name);

	for (vk = 1; vk < 254; vk++)
	{
		int down = (forge_key_state(vk) & 0x8000) != 0;
		if (vk == VK_BACK)
		{
			if (down && !s_tag_browser.prev_vk[vk])
			{
				s_tag_browser.name_box_active = 1;
				if (len > 0)
					s_tag_browser.custom_name[--len] = 0;
				s_tag_browser.bs_next_ms = now_ms + 350UL;
			}
			else if (down && s_tag_browser.prev_vk[vk] && now_ms >= s_tag_browser.bs_next_ms)
			{
				s_tag_browser.name_box_active = 1;
				if (len > 0)
					s_tag_browser.custom_name[--len] = 0;
				s_tag_browser.bs_next_ms = now_ms + 45UL;
			}
		}
		else if (down && !s_tag_browser.prev_vk[vk])
		{
			if (vk == 0x2E /* VK_DELETE */)
			{
				s_tag_browser.name_box_active = 1;
				s_tag_browser.custom_name[0] = 0;
				len = 0;
			}
			else if (vk >= 'A' && vk <= 'Z')
			{
				s_tag_browser.name_box_active = 1;
				if (len < 16)
				{
					s_tag_browser.custom_name[len++] = (char)(shift_down ? vk : (vk - 'A' + 'a'));
					s_tag_browser.custom_name[len] = 0;
				}
			}
			else if (vk >= '0' && vk <= '9')
			{
				s_tag_browser.name_box_active = 1;
				if (len < 16)
				{
					s_tag_browser.custom_name[len++] = (char)vk;
					s_tag_browser.custom_name[len] = 0;
				}
			}
			else if (vk >= 0x60 && vk <= 0x69 /* VK_NUMPAD0..9 */)
			{
				s_tag_browser.name_box_active = 1;
				if (len < 16)
				{
					s_tag_browser.custom_name[len++] = (char)('0' + (vk - 0x60));
					s_tag_browser.custom_name[len] = 0;
				}
			}
			else if (vk == VK_SPACE)
			{
				s_tag_browser.name_box_active = 1;
				if (len > 0 && len < 16)
				{
					s_tag_browser.custom_name[len++] = ' ';
					s_tag_browser.custom_name[len] = 0;
				}
			}
			else if (vk == 0xBD /* VK_OEM_MINUS */ || vk == 0x6D /* VK_SUBTRACT */)
			{
				s_tag_browser.name_box_active = 1;
				if (len < 16)
				{
					s_tag_browser.custom_name[len++] = shift_down ? '_' : '-';
					s_tag_browser.custom_name[len] = 0;
				}
			}
		}
		s_tag_browser.prev_vk[vk] = down;
	}

	if (tb)
	{
		fw_text_set_ascii(tb, "Enter Forge menu name for tag:");
	}
	if (input_btn)
	{
		int caret_on = s_tag_browser.name_box_active && (((now_ms / 450UL) & 1UL) == 0UL);
		snprintf(buf, sizeof(buf),
			caret_on ? "[C] %s_" : "[C] %s",
			s_tag_browser.custom_name);
		fw_text_set_ascii(input_btn, buf);
		input_btn->animation.current_frame_index = s_tag_browser.name_box_active ? 1 : 0;
	}
	if (confirm_btn)
		confirm_btn->animation.current_frame_index = (bar && bar->focused_child == confirm_btn) ? 1 : 0;
	if (back_btn)
		back_btn->animation.current_frame_index = (bar && bar->focused_child == back_btn) ? 1 : 0;
}

static void update_forge_installed_tags_widget(struct forge_widget_instance *list)
{
	struct forge_widget_instance *spinner = fw_named(list, "forge_inst_cat_spinner", 0);
	struct forge_widget_instance *desc = list ? list->parameters.list.extended_description : NULL;
	struct forge_widget_instance *child;
	int cat = spinner ? (int)spinner->parameters.list.selected_index : 0;
	int count;
	int row_idx = 0;
	int focused_entry = -1;

	if (cat < 0) cat = 0;
	if (cat > 4) cat = 4;
	if (cat != s_inst_browser.cat)
	{
		s_inst_browser.cat = cat;
		s_inst_browser.first = 0;
		s_inst_browser.chosen = 0;
		s_preview_rev++;
		if (fw_focused_row(list) >= 1)
			fw_focus_row(list, 1);
	}

	count = forge_ui_installed_tag_count(cat);
	if (s_inst_browser.chosen >= count)
		s_inst_browser.chosen = (count > 0) ? (count - 1) : 0;
	if (s_inst_browser.chosen < 0)
		s_inst_browser.chosen = 0;

	fw_scroll_rows_at_edges(list, &s_inst_browser.first, count);

	for (child = list ? list->child : NULL; child; child = child->next, row_idx++)
	{
		int item_slot;
		if (row_idx < 1)
			continue;
		if (!child->name || strncmp(child->name, "list_item_", 10) != 0)
			break;
		item_slot = s_inst_browser.first + (row_idx - 1);
		if (count == 0)
		{
			child->visible = (row_idx == 1) ? TRUE : FALSE;
			if (row_idx == 1)
				fw_text_set_ascii(fw_named(child, "list_item_text", 0), "(No Installed Tags in Category)");
			else if (child == list->focused_child)
				fw_focus_row(list, 1);
		}
		else
		{
			child->visible = (item_slot < count) ? TRUE : FALSE;
			if (item_slot < count)
			{
				char disp_name[64];
				disp_name[0] = 0;
				forge_ui_installed_tag_get_info(cat, item_slot, disp_name, sizeof(disp_name), NULL, 0, NULL, 0, NULL, 0, NULL);
				fw_text_set_ascii(fw_named(child, "list_item_text", 0), disp_name);
				if (child == list->focused_child)
					focused_entry = item_slot;
			}
			else if (child == list->focused_child)
			{
				int shown = (count < 10) ? count : 10;
				fw_focus_row(list, (shown > 0) ? shown : 1);
			}
		}
		if (fw_named(child, "list_item_arrows", 0))
			fw_named(child, "list_item_arrows", 0)->visible =
				(child == list->focused_child || (count > 0 && item_slot == s_inst_browser.chosen));
		if (fw_named(child, "scroll_up_button", 0))
			fw_named(child, "scroll_up_button", 0)->visible = FALSE;
		if (fw_named(child, "scroll_down_button", 0))
			fw_named(child, "scroll_down_button", 0)->visible = FALSE;
	}

	if (focused_entry >= 0 && focused_entry < count)
		s_inst_browser.chosen = focused_entry;

	if (desc)
	{
		struct forge_widget_instance *r_name = fw_named(desc, "forge_tag_right_name", 0);
		struct forge_widget_instance *r_data = fw_named(desc, "forge_tag_right_data", 0);
		fw_profile_name_show(desc);

		if (count > 0 && s_inst_browser.chosen >= 0 && s_inst_browser.chosen < count)
		{
			char disp_name[64];
			char wrapped_title[96];
			char wrapped_item[96];
			char full_path[256];
			char type_str[64];
			char size_str[64];
			char info_buf[256];
			int is_custom = 0;

			disp_name[0] = 0;
			full_path[0] = 0;
			type_str[0] = 0;
			size_str[0] = 0;
			forge_ui_installed_tag_get_info(
				cat,
				s_inst_browser.chosen,
				disp_name, sizeof(disp_name),
				full_path, sizeof(full_path),
				type_str, sizeof(type_str),
				size_str, sizeof(size_str),
				&is_custom);

			tag_preview_select(1, cat, s_inst_browser.chosen, s_preview_rev);
			fw_wrap_label(disp_name, 14, 2, wrapped_title, sizeof(wrapped_title));
			fw_wrap_label(disp_name, 16, 2, wrapped_item, sizeof(wrapped_item));
			fw_text_set_ascii(r_name, wrapped_title);

			snprintf(info_buf, sizeof(info_buf),
				"- %s\r\n- %s\r\n- %s",
				wrapped_item, type_str, size_str);
			fw_text_set_ascii(r_data, info_buf);
		}
		else
		{
			tag_preview_clear();
			fw_text_set_ascii(r_name, "NO TAGS");
			fw_text_set_ascii(r_data, "- None\r\n- None\r\n- 0 KB");
		}
	}
}

static void update_forge_installed_tag_action_dialog(struct forge_widget_instance *bar)
{
	struct forge_widget_instance *screen = bar;
	struct forge_widget_instance *tb;
	char disp_name[64];
	char full_path[256];
	char type_str[64];
	char size_str[64];
	char buf[256];
	int is_custom = 0;

	while (screen && screen->parent)
		screen = screen->parent;
	tb = fw_named(screen, "forge_installed_tag_text_box", 0);
	if (!tb)
		return;

	disp_name[0] = 0;
	full_path[0] = 0;
	type_str[0] = 0;
	size_str[0] = 0;
	if (forge_ui_installed_tag_get_info(
		s_inst_browser.cat,
		s_inst_browser.chosen,
		disp_name, sizeof(disp_name),
		full_path, sizeof(full_path),
		type_str, sizeof(type_str),
		size_str, sizeof(size_str),
		&is_custom))
	{
		snprintf(buf, sizeof(buf),
			"- %s\r\n- %s\r\n- %s\r\n\r\nDelete this tag from the cache?",
			disp_name, type_str, size_str);
	}
	else
	{
		snprintf(buf, sizeof(buf), "Delete this tag from the cache?");
	}
	fw_text_set_ascii(tb, buf);
}

boolean forge_player_profile_get(long player_profile_index, struct player_profile *profile)
{
	boolean ok = player_profile_get(player_profile_index, profile);
	if (ok && profile && g_forge_session_enabled)
	{
		int lv;
		for (lv = 0; lv < NUMBER_OF_SINGLE_PLAYER_LEVELS; lv++)
			profile->single_player_map_flags[lv] |= 1;
	}
	return ok;
}

static static boolean forge_controls_menu_event(
	struct forge_widget_instance *widget,
	void *event_ptr,
	char const *fn_name,
	long function_index,
	boolean *widget_deleted);

boolean pc_menu_event_function_invoke(
	void *widget_ptr,
	void *event_ptr,
	long function_index,
	boolean *widget_deleted)
{
	struct forge_widget_instance *widget = (struct forge_widget_instance *)widget_ptr;
	char const *fn_name = pc_menu_function_name(function_index);
	char const *w_name = (widget && widget->name) ? widget->name : "";
	char const *p_name = (widget && widget->parent && widget->parent->name) ? widget->parent->name : "";

	/* Scope Forge session strictly to Multiplayer -> Forge flows */
	if ((fn_name && strcmp(fn_name, "gamemode select forge") == 0) ||
		(fn_name && strcmp(fn_name, "gamemode select choose") == 0 && strcmp(w_name, "gm_forge_item") == 0) ||
		strncmp(w_name, "forge_", 6) == 0 ||
		strncmp(p_name, "forge_", 6) == 0)
	{
		g_forge_session_enabled = 1;
		forge_runtime_init();
	}
	else if (fn_name &&
		((strcmp(fn_name, "gamemode select choose") == 0 && strcmp(w_name, "gm_forge_item") != 0) ||
		 strcmp(fn_name, "gamemode select standard") == 0 ||
		 strcmp(fn_name, "new campaign decision") == 0 ||
		 strcmp(fn_name, "campaign menu init") == 0 ||
		 strcmp(fn_name, "port coop begin") == 0 ||
		 strcmp(fn_name, "main menu initialize") == 0))
	{
		g_forge_session_enabled = 0;
		s_forge_active_map_kind = 1;
	}

	/* Forge -> Tag Options: ADD TAGS / INSTALLED TAGS / NAME TAG / DELETE / RESET */
	if (fn_name && strcmp(fn_name, "load game menu init") == 0)
	{
		if (strcmp(w_name, "forge_add_tags_map_list") == 0)
		{
			struct forge_widget_instance *sp;
			int cnt;
			g_forge_session_enabled = 1;
			forge_menu_custom_edition_maps_look_again();
			if (!s_add_tags_map.preserve_on_return)
			{
				s_add_tags_map.kind = 1;
				s_add_tags_map.first = 0;
				s_add_tags_map.chosen = 0;
			}
			s_add_tags_map.preserve_on_return = 0;
			sp = fw_named(widget, "forge_add_tags_map_kind_spinner", 0);
			if (sp)
				sp->parameters.list.selected_index = (short)s_add_tags_map.kind;
			cnt = add_tags_map_kind_count(s_add_tags_map.kind);
			fw_focus_row(widget, (cnt > 0) ? (1 + s_add_tags_map.chosen - s_add_tags_map.first) : 0);
			return TRUE;
		}
		if (strcmp(w_name, "forge_add_tags_browser_list") == 0)
		{
			struct forge_widget_instance *sp;
			int cnt;
			g_forge_session_enabled = 1;
			if (!s_tag_browser.preserve_on_return)
			{
				s_tag_browser.cat = 0;
				s_tag_browser.first = 0;
				s_tag_browser.chosen = 0;
				s_tag_browser.status_msg[0] = 0;
			}
			s_tag_browser.preserve_on_return = 0;
			sp = fw_named(widget, "forge_tag_cat_spinner", 0);
			if (sp)
				sp->parameters.list.selected_index = (short)s_tag_browser.cat;
			cnt = forge_ui_map_tag_count(s_tag_browser.cat);
			fw_focus_row(widget, (cnt > 0) ? (1 + s_tag_browser.chosen - s_tag_browser.first) : 0);
			s_preview_rev++;
			return TRUE;
		}
		if (strcmp(w_name, "forge_installed_tags_list") == 0)
		{
			struct forge_widget_instance *sp;
			int cnt;
			g_forge_session_enabled = 1;
			forge_runtime_init();
			if (!s_inst_browser.preserve_on_return)
			{
				s_inst_browser.cat = 0;
				s_inst_browser.first = 0;
				s_inst_browser.chosen = 0;
			}
			s_inst_browser.preserve_on_return = 0;
			sp = fw_named(widget, "forge_inst_cat_spinner", 0);
			if (sp)
				sp->parameters.list.selected_index = (short)s_inst_browser.cat;
			cnt = forge_ui_installed_tag_count(s_inst_browser.cat);
			if (s_inst_browser.chosen >= cnt)
				s_inst_browser.chosen = (cnt > 0) ? (cnt - 1) : 0;
			if (s_inst_browser.first > s_inst_browser.chosen)
				s_inst_browser.first = s_inst_browser.chosen;
			fw_focus_row(widget, (cnt > 0) ? (1 + s_inst_browser.chosen - s_inst_browser.first) : 0);
			s_preview_rev++;
			return TRUE;
		}
	}
	if (fn_name && strcmp(fn_name, "load game menu dispose") == 0)
	{
		if (strcmp(w_name, "forge_add_tags_browser_list") == 0 ||
			strcmp(w_name, "forge_installed_tags_list") == 0)
		{
			return TRUE;
		}
	}
	if (fn_name && strcmp(fn_name, "load game menu activated") == 0)
	{
		if (strcmp(w_name, "forge_add_tags_map_list") == 0 ||
			strcmp(p_name, "forge_add_tags_map_list") == 0)
		{
			struct forge_widget_instance *lw = (strcmp(w_name, "forge_add_tags_map_list") == 0) ? widget : widget->parent;
			char map_path[512];
			update_forge_add_tags_map_widget(lw);
			if (resolve_add_tags_map_path(s_add_tags_map.kind, s_add_tags_map.chosen, map_path, sizeof(map_path)) &&
				forge_ui_open_map_for_tag_browser(map_path))
			{
				s_add_tags_map.preserve_on_return = 1;
				s_tag_browser.preserve_on_return = 0;
				s_tag_browser.cat = 0;
				s_tag_browser.first = 0;
				s_tag_browser.chosen = 0;
				s_tag_browser.status_msg[0] = 0;
				s_preview_rev++;
				ui_play_audio_feedback_sound(2);
				ui_widget_port_open(
					widget_ptr,
					"pc\\main_menu\\forge_select\\forge_add_tags_browser_screen",
					widget_deleted);
				return TRUE;
			}
			ui_play_audio_feedback_sound(4);
			return FALSE;
		}
		if (strcmp(w_name, "forge_add_tags_browser_list") == 0 ||
			strcmp(p_name, "forge_add_tags_browser_list") == 0)
		{
			struct forge_widget_instance *lw = (strcmp(w_name, "forge_add_tags_browser_list") == 0) ? widget : widget->parent;
			int cnt;
			update_forge_add_tags_browser_widget(lw);
			cnt = forge_ui_map_tag_count(s_tag_browser.cat);
			if (cnt > 0 && s_tag_browser.chosen >= 0 && s_tag_browser.chosen < cnt)
			{
				char short_name[64];
				int vk;
				short_name[0] = 0;
				forge_ui_map_tag_get_info(
					s_tag_browser.cat,
					s_tag_browser.chosen,
					short_name, sizeof(short_name),
					NULL, 0, NULL, 0, NULL, 0);
				snprintf(s_tag_browser.custom_name, sizeof(s_tag_browser.custom_name), "%.14s", short_name);
				s_tag_browser.name_box_active = 0;
				s_tag_browser.dialog_open = 1;
				s_tag_browser.last_dialog_ms = system_milliseconds();
				s_tag_browser.bs_next_ms = 0;
				for (vk = 1; vk < 254; vk++)
					s_tag_browser.prev_vk[vk] = (forge_key_state(vk) & 0x8000) != 0;
				s_tag_browser.preserve_on_return = 1;
				ui_play_audio_feedback_sound(2);
				ui_widget_port_open(
					widget_ptr,
					"pc\\main_menu\\forge_select\\forge_name_tag_screen",
					widget_deleted);
				return TRUE;
			}
			ui_play_audio_feedback_sound(4);
			return FALSE;
		}
		if (strcmp(w_name, "forge_installed_tags_list") == 0 ||
			strcmp(p_name, "forge_installed_tags_list") == 0)
		{
			struct forge_widget_instance *lw = (strcmp(w_name, "forge_installed_tags_list") == 0) ? widget : widget->parent;
			int cnt;
			update_forge_installed_tags_widget(lw);
			cnt = forge_ui_installed_tag_count(s_inst_browser.cat);
			if (cnt > 0 && s_inst_browser.chosen >= 0 && s_inst_browser.chosen < cnt)
			{
				s_inst_browser.preserve_on_return = 1;
				ui_play_audio_feedback_sound(2);
				ui_widget_port_open(
					widget_ptr,
					"pc\\main_menu\\forge_select\\forge_installed_tag_action_screen",
					widget_deleted);
				return TRUE;
			}
			ui_play_audio_feedback_sound(4);
			return FALSE;
		}
	}

	/* Forge -> Edit Maps screen initialization & activation */
	if (fn_name && strcmp(fn_name, "load game menu init") == 0 &&
		strcmp(w_name, "forge_edit_maps_list") == 0)
	{
		g_forge_session_enabled = 1;
		dispose_global_network_game_client();
		dispose_global_network_game_server();
		network_game_accept_remote_connections(FALSE);
		forge_menu_custom_edition_maps_look_again();
		scan_forge_edit_maps(0);
		return TRUE;
	}
	if (fn_name && strcmp(fn_name, "load game menu activated") == 0 &&
		(strcmp(w_name, "forge_edit_maps_list") == 0 ||
		 (widget && widget->parent && widget->parent->name && strcmp(widget->parent->name, "forge_edit_maps_list") == 0)))
	{
		if (s_forge_menu.count > 0 && s_forge_menu.chosen >= 0 && s_forge_menu.chosen < s_forge_menu.count)
		{
			char level_path[128];
			g_forge_session_enabled = 1;
			dispose_global_network_game_client();
			dispose_global_network_game_server();
			network_game_accept_remote_connections(FALSE);
			forge_menu_custom_edition_maps_look_again();
			snprintf(level_path, sizeof(level_path), "custom_maps\\%s", s_forge_menu.map_leaves[s_forge_menu.chosen]);
			player_ui_set_single_player_local_player_controller(0, 0);
			main_set_map_name(level_path);
			main_set_difficulty(1);
			game_connection_set(0);
			main_menu_switch_to_single_player();
			player_ui_remember_player1_profile(TRUE);
			ui_play_audio_feedback_sound(2);
			return TRUE;
		}
		ui_play_audio_feedback_sound(4);
		return FALSE;
	}

	/* Forge Tag Options -> Delete Installed Tag / Reset Stock Cache / Reset Custom Cache / Name Tag Dialog */
	if (fn_name && strcmp(fn_name, "port mods close") == 0)
	{
		if (w_name && strcmp(w_name, "forge_installed_tag_delete_btn") == 0)
		{
			forge_ui_installed_tag_delete(s_inst_browser.cat, s_inst_browser.chosen);
			s_preview_rev++;
			ui_play_audio_feedback_sound(2);
			return TRUE;
		}
		if (w_name && strcmp(w_name, "forge_confirm_reset_stock_ok") == 0)
		{
			forge_reset_stock_tag_cache();
			s_preview_rev++;
			ui_play_audio_feedback_sound(2);
			return TRUE;
		}
		if (w_name && strcmp(w_name, "forge_confirm_reset_custom_ok") == 0)
		{
			forge_clear_custom_tag_cache();
			s_preview_rev++;
			ui_play_audio_feedback_sound(2);
			return TRUE;
		}
		if (w_name && strcmp(w_name, "forge_name_tag_back_btn") == 0)
		{
			s_tag_browser.name_box_active = 0;
			s_tag_browser.dialog_open = 0;
			ui_play_audio_feedback_sound(3);
			return TRUE;
		}
	}
	if (fn_name && strcmp(fn_name, "port mods toggle") == 0 &&
		w_name && strcmp(w_name, "forge_name_tag_input_btn") == 0)
	{
		int vk;
		s_tag_browser.name_box_active = 1;
		s_tag_browser.dialog_open = 1;
		s_tag_browser.last_dialog_ms = system_milliseconds();
		for (vk = 0; vk < 256; vk++)
			s_tag_browser.prev_vk[vk] = (GetAsyncKeyState(vk) & 0x8000) ? 1 : 0;
		ui_play_audio_feedback_sound(2);
		return TRUE;
	}
	if (fn_name && strcmp(fn_name, "port mods save") == 0 &&
		w_name && strcmp(w_name, "forge_name_tag_confirm_btn") == 0)
	{
		s_tag_browser.name_box_active = 0;
		s_tag_browser.dialog_open = 0;
		if (forge_ui_extract_selected_map_tag(s_tag_browser.cat, s_tag_browser.chosen, s_tag_browser.custom_name))
		{
			snprintf(s_tag_browser.status_msg, sizeof(s_tag_browser.status_msg),
				"Added: [C] %s", s_tag_browser.custom_name);
			s_preview_rev++;
			ui_play_audio_feedback_sound(2);
		}
		else
		{
			snprintf(s_tag_browser.status_msg, sizeof(s_tag_browser.status_msg),
				"Extraction failed!");
			ui_play_audio_feedback_sound(4);
		}
		return TRUE;
	}

	/* Refresh Custom Edition & saved Forge maps on Single Player / Multiplayer level list init */
	if (fn_name && strcmp(fn_name, "initialize sp level list solo") == 0)
	{
		boolean res;
		struct forge_widget_instance *spinner;
		dispose_global_network_game_client();
		dispose_global_network_game_server();
		network_game_accept_remote_connections(FALSE);
		forge_menu_custom_edition_maps_look_again();
		s_forge_active_map_kind = 1;
		s_forge_last_map_kind = -1;
		res = orig_pc_menu_event_function_invoke(widget_ptr, event_ptr, function_index, widget_deleted);
		if (g_forge_session_enabled)
		{
			spinner = fw_named(widget, "list_item_0_map_kind_spinner", 0);
			if (spinner)
				spinner->parameters.list.selected_index = 1;
		}
		return res;
	}
	if (fn_name && strcmp(fn_name, "mp level list initialize") == 0)
	{
		boolean res;
		struct forge_widget_instance *spinner;
		forge_menu_custom_edition_maps_look_again();
		s_forge_active_map_kind = 1;
		s_forge_last_map_kind = 1;
		res = orig_pc_menu_event_function_invoke(widget_ptr, event_ptr, function_index, widget_deleted);
		spinner = fw_named(widget, "list_item_0_map_kind_spinner", 0);
		if (spinner)
			spinner->parameters.list.selected_index = 1;
		return res;
	}

	if (fn_name && strcmp(fn_name, "solo level set map") == 0)
	{
		boolean res;
		struct forge_widget_instance *spinner = fw_named(widget, "list_item_0_map_kind_spinner", 0);
		dispose_global_network_game_client();
		dispose_global_network_game_server();
		network_game_accept_remote_connections(FALSE);
		if (!g_forge_session_enabled)
		{
			short sel = spinner ? spinner->parameters.list.selected_index : 0;
			if (spinner && sel == FORGE_MAP_KIND_FORGE_MAPS)
				spinner->parameters.list.selected_index = 3;
			s_forge_active_map_kind = sel;
			res = orig_pc_menu_event_function_invoke(widget_ptr, event_ptr, function_index, widget_deleted);
			if (spinner && sel == FORGE_MAP_KIND_FORGE_MAPS)
				spinner->parameters.list.selected_index = FORGE_MAP_KIND_FORGE_MAPS;
			return res;
		}
		if (spinner && s_forge_last_map_kind == FORGE_MAP_KIND_FORGE_MAPS)
			spinner->parameters.list.selected_index = 3;
		s_forge_active_map_kind = s_forge_last_map_kind;
		res = orig_pc_menu_event_function_invoke(widget_ptr, event_ptr, function_index, widget_deleted);
		if (spinner && s_forge_last_map_kind == FORGE_MAP_KIND_FORGE_MAPS)
			spinner->parameters.list.selected_index = FORGE_MAP_KIND_FORGE_MAPS;
		if (g_forge_session_enabled && res)
		{
			char cur_map_copy[128];
			char const *cur = main_get_map_name();
			snprintf(cur_map_copy, sizeof(cur_map_copy), "%s", (cur && cur[0]) ? cur : "levels\\test\\bloodgulch\\bloodgulch");
			dispose_global_network_game_client();
			dispose_global_network_game_server();
			network_game_accept_remote_connections(FALSE);
			player_ui_set_single_player_local_player_controller(0, 0);
			main_set_map_name(cur_map_copy);
			main_set_difficulty(1);
			game_connection_set(0);
			main_menu_switch_to_single_player();
			player_ui_remember_player1_profile(TRUE);
			ui_play_audio_feedback_sound(2);
			return FALSE; /* Returning FALSE prevents opening difficulty_select_list_screen */
		}
		return res;
	}

	/* Point 7: Multiplayer Map Select (FORGE MAPS tab works in Standard/Ranked MP & Forge; Forge skips Gametype Select) */
	if (fn_name && strcmp(fn_name, "mp level select") == 0)
	{
		boolean res;
		struct forge_widget_instance *spinner = fw_named(widget, "list_item_0_map_kind_spinner", 0);
		short sel = spinner ? spinner->parameters.list.selected_index : s_forge_last_map_kind;
		int is_forge_tab = (sel == FORGE_MAP_KIND_FORGE_MAPS || (g_forge_session_enabled && s_forge_last_map_kind == FORGE_MAP_KIND_FORGE_MAPS));
		if (spinner && is_forge_tab)
			spinner->parameters.list.selected_index = 3;
		s_forge_active_map_kind = is_forge_tab ? FORGE_MAP_KIND_FORGE_MAPS : sel;
		res = orig_pc_menu_event_function_invoke(widget_ptr, event_ptr, function_index, widget_deleted);
		if (spinner && is_forge_tab)
			spinner->parameters.list.selected_index = FORGE_MAP_KIND_FORGE_MAPS;
		if (g_forge_session_enabled && res)
		{
			long gtypes[64];
			short last_gt = 0;
			struct game_variant forge_var;
			struct game_variant_options forge_opts;

			if (ui_widget_port_gametypes(gtypes, 64, &last_gt) > 0)
				ui_widget_port_gametype_choose(gtypes[0]);

			memset(&forge_var, 0, sizeof(forge_var));
			build_game_variant_slayer(&forge_var);
			ustrncpy(
				forge_var.human_readable_game_description,
				L"Forge",
				NUMBEROF(forge_var.human_readable_game_description) - 1);
			forge_var.universal_variant.teams = FALSE;
			forge_var.universal_variant.score_to_win = 0x3FFFFFFF; /* Endless Forge session: never ends on kills */
			forge_var.universal_variant.respawn_time = 90;
			forge_var.universal_variant.suicide_penalty = 0;

			game_variant_options_default(&forge_var, &forge_opts);
			forge_opts.time_limit = 0; /* No time limit */
			forge_opts.vehicle_set[0] = VARIANT_VEHICLE_SET_PC; /* All map vehicles enabled */
			forge_opts.vehicle_set[1] = VARIANT_VEHICLE_SET_PC;
			ui_widget_port_game_variant_set(&forge_var, &forge_opts);

			ui_widget_port_open(
				widget_ptr,
				"pc\\main_menu\\multiplayer_type_select\\server_settings\\server_settings_screen",
				widget_deleted);
			return FALSE; /* Returning FALSE prevents XML handler from opening gametype_select_screen_wrapper */
		}
		return res;
	}

	/* In-Game Escape Pause Menu -> "SAVE MAP.." button handler
	 * Note: ui_widget_event_handler_function_invoke subtracts PC_MENU_FUNCTION_BASE (0x100 = 256) before calling pc_menu_event_function_invoke */
	if (function_index == 0x7777 ||
		function_index == (0x7777 - 256) ||
		function_index == (0x7777 - 102) ||
		(widget && widget->definition_tag_index == s_save_forge_btn_tag && s_save_forge_btn_tag != NONE) ||
		(w_name && strcmp(w_name, "save_forge_button") == 0))
	{
		ui_widgets_close_all();
		if (widget_deleted)
			*widget_deleted = TRUE;
		ui_play_audio_feedback_sound(2);
		forge_trigger_save_map_action();
		return TRUE;
	}

	/* Settings -> Controls Setup ("FORGE" and "FORGE PAD" tabs) */
	if (fn_name &&
		(strcmp(fn_name, "controls screen init") == 0 ||
		 strcmp(fn_name, "controls screen defaults") == 0 ||
		 strcmp(fn_name, "controls screen change set") == 0 ||
		 strcmp(fn_name, "controls binding slot") == 0 ||
		 strcmp(fn_name, "controls begin binding") == 0))
	{
		if (forge_controls_menu_event(widget, event_ptr, fn_name, function_index, widget_deleted))
			return TRUE;
	}

	return orig_pc_menu_event_function_invoke(widget_ptr, event_ptr, function_index, widget_deleted);
}

/* ---------- Settings -> Controls Setup ("FORGE" & "FORGE PAD" Tabs) Implementation */

static struct
{
	int initialized;
	int32_t pending_vk[FORGE_BIND_COUNT];
	int32_t pending_gp[FORGE_BIND_COUNT];
	float pending_look_sens;
	float pending_fly_speed;
	int slot; /* 0 = Keyboard/Mouse, 1 = Controller */
	int capturing_action; /* -1 when idle, 0..FORGE_BIND_COUNT-1 when waiting for input */
	int capturing_slot;
	unsigned long capture_start_ms;
	int prev_vk_down[256];
	int prev_gp_down[FORGE_GP_COUNT];
} s_forge_controls_ui = { 0, { 0 }, { 0 }, 1.0f, 1.0f, 0, -1, 0, 0, { 0 }, { 0 } };

static char const *const s_forge_bind_labels[FORGE_BIND_COUNT] = {
	"FORGE / SPARTAN",
	"OBJECT PALETTE",
	"3RD-PERSON VIEW",
	"QUICK SAVE MAP",
	"ROTATE ITEM",
	"SNAP TO FLOOR",
	"SNAP TO SURFACE",
	"ROTATION SNAP",
	"OBJECT OPTIONS",
	"DELETE OBJECT",
	"COPY OBJECT"
};

static struct forge_widget_instance *fw_screen_of(struct forge_widget_instance *w)
{
	while (w && w->parent)
		w = w->parent;
	return w;
}

static struct forge_widget_instance *fw_control_row(struct forge_widget_instance *w)
{
	for (; w; w = w->parent)
	{
		if (w->name && strncmp(w->name, "op_command_", 11) == 0)
			return w;
	}
	return NULL;
}

static int fw_controls_group(struct forge_widget_instance *w)
{
	struct forge_widget_instance *sp = fw_named(fw_screen_of(w), "group_spinner", 0);
	return sp ? (int)sp->parameters.list.selected_index : 0;
}

static void forge_controls_ui_ensure_loaded(void)
{
	int i;
	if (g_forge.bind_vk[0] == 0 && g_forge.bind_gp[0] == 0)
	{
		forge_controls_init_defaults();
		forge_controls_load();
	}
	if (!s_forge_controls_ui.initialized)
	{
		for (i = 0; i < FORGE_BIND_COUNT; i++)
		{
			s_forge_controls_ui.pending_vk[i] = g_forge.bind_vk[i];
			s_forge_controls_ui.pending_gp[i] = g_forge.bind_gp[i];
		}
		s_forge_controls_ui.pending_look_sens = (g_forge.controller_look_sens >= 0.25f) ? g_forge.controller_look_sens : 1.0f;
		s_forge_controls_ui.pending_fly_speed = (g_forge.controller_fly_speed >= 0.25f) ? g_forge.controller_fly_speed : 1.0f;
		s_forge_controls_ui.capturing_action = -1;
		s_forge_controls_ui.initialized = 1;
	}
}

static void forge_controls_snapshot_input(void)
{
	int vk, gp;
	float lx, ly, rx, ry, lt, rt;
	forge_poll_controller_state(&lx, &ly, &rx, &ry, &lt, &rt);
	for (vk = 1; vk < 255; vk++)
		s_forge_controls_ui.prev_vk_down[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
	for (gp = 1; gp < FORGE_GP_COUNT; gp++)
		s_forge_controls_ui.prev_gp_down[gp] = forge_gp_button_down(gp);
}

boolean forge_controls_menu_event(
	struct forge_widget_instance *widget,
	void *event_ptr,
	char const *fn_name,
	long function_index,
	boolean *widget_deleted)
{
	int group = fw_controls_group(widget);
	int i;

	if (strcmp(fn_name, "controls screen init") == 0)
	{
		s_forge_controls_ui.initialized = 0;
		forge_controls_ui_ensure_loaded();
		orig_pc_menu_event_function_invoke(widget, event_ptr, function_index, widget_deleted);
		return TRUE;
	}
	if (strcmp(fn_name, "controls screen defaults") == 0)
	{
		forge_controls_ui_ensure_loaded();
		if (group == 4 || group == 5)
		{
			int32_t saved_vk[FORGE_BIND_COUNT];
			int32_t saved_gp[FORGE_BIND_COUNT];
			float saved_ls = g_forge.controller_look_sens;
			float saved_fs = g_forge.controller_fly_speed;
			for (i = 0; i < FORGE_BIND_COUNT; i++)
			{
				saved_vk[i] = g_forge.bind_vk[i];
				saved_gp[i] = g_forge.bind_gp[i];
			}
			forge_controls_init_defaults();
			for (i = 0; i < FORGE_BIND_COUNT; i++)
			{
				s_forge_controls_ui.pending_vk[i] = g_forge.bind_vk[i];
				s_forge_controls_ui.pending_gp[i] = g_forge.bind_gp[i];
				g_forge.bind_vk[i] = saved_vk[i];
				g_forge.bind_gp[i] = saved_gp[i];
			}
			s_forge_controls_ui.pending_look_sens = 1.0f;
			s_forge_controls_ui.pending_fly_speed = 1.0f;
			g_forge.controller_look_sens = saved_ls;
			g_forge.controller_fly_speed = saved_fs;
			s_forge_controls_ui.capturing_action = -1;
			return TRUE;
		}
		return orig_pc_menu_event_function_invoke(widget, event_ptr, function_index, widget_deleted);
	}
	if (strcmp(fn_name, "controls screen change set") == 0)
	{
		forge_controls_ui_ensure_loaded();
		for (i = 0; i < FORGE_BIND_COUNT; i++)
		{
			g_forge.bind_vk[i] = s_forge_controls_ui.pending_vk[i];
			g_forge.bind_gp[i] = s_forge_controls_ui.pending_gp[i];
		}
		g_forge.controller_look_sens = s_forge_controls_ui.pending_look_sens;
		g_forge.controller_fly_speed = s_forge_controls_ui.pending_fly_speed;
		forge_controls_save();
		return orig_pc_menu_event_function_invoke(widget, event_ptr, function_index, widget_deleted);
	}
	if (strcmp(fn_name, "controls binding slot") == 0)
	{
		forge_controls_ui_ensure_loaded();
		if (group == 4)
		{
			s_forge_controls_ui.slot = !s_forge_controls_ui.slot;
			orig_pc_menu_event_function_invoke(widget, event_ptr, function_index, widget_deleted);
			return TRUE;
		}
		if (group == 5)
		{
			struct forge_widget_instance *row = fw_control_row(widget);
			int row_idx = row ? (atoi(row->name + 11) - 1) : -1;
			int dir = 1;
			if ((GetAsyncKeyState(VK_LEFT) & 0x8000) || (GetAsyncKeyState('A') & 0x8000) || forge_gp_button_down(FORGE_GP_DPAD_LEFT))
			{
				dir = -1;
			}
			else if (event_ptr)
			{
				short const *ev_s = (short const *)event_ptr;
				if ((ev_s[0] == 2 && ev_s[2] == 14) || (ev_s[0] == 0 && ev_s[2] < 0))
					dir = -1;
			}
			if (row_idx == 0)
			{
				s_forge_controls_ui.pending_look_sens += (float)dir * 0.25f;
				if (s_forge_controls_ui.pending_look_sens < 0.25f) s_forge_controls_ui.pending_look_sens = 0.25f;
				if (s_forge_controls_ui.pending_look_sens > 2.50f) s_forge_controls_ui.pending_look_sens = 2.50f;
				return TRUE;
			}
			if (row_idx == 1)
			{
				s_forge_controls_ui.pending_fly_speed += (float)dir * 0.25f;
				if (s_forge_controls_ui.pending_fly_speed < 0.25f) s_forge_controls_ui.pending_fly_speed = 0.25f;
				if (s_forge_controls_ui.pending_fly_speed > 2.50f) s_forge_controls_ui.pending_fly_speed = 2.50f;
				return TRUE;
			}
			s_forge_controls_ui.slot = 1;
			return TRUE;
		}
		s_forge_controls_ui.slot = !s_forge_controls_ui.slot;
		return orig_pc_menu_event_function_invoke(widget, event_ptr, function_index, widget_deleted);
	}
	if (strcmp(fn_name, "controls begin binding") == 0)
	{
		forge_controls_ui_ensure_loaded();
		if (group == 4)
		{
			struct forge_widget_instance *row = fw_control_row(widget);
			int row_idx = row ? (atoi(row->name + 11) - 1) : -1;
			if (row_idx >= 0 && row_idx < FORGE_BIND_COUNT)
			{
				s_forge_controls_ui.capturing_action = row_idx;
				s_forge_controls_ui.capturing_slot = s_forge_controls_ui.slot;
				s_forge_controls_ui.capture_start_ms = system_milliseconds();
				forge_controls_snapshot_input();
				return TRUE;
			}
		}
		else if (group == 5)
		{
			struct forge_widget_instance *row = fw_control_row(widget);
			int row_idx = row ? (atoi(row->name + 11) - 1) : -1;
			if (row_idx == 0)
			{
				s_forge_controls_ui.pending_look_sens += 0.25f;
				if (s_forge_controls_ui.pending_look_sens > 2.50f)
					s_forge_controls_ui.pending_look_sens = 0.25f;
				return TRUE;
			}
			if (row_idx == 1)
			{
				s_forge_controls_ui.pending_fly_speed += 0.25f;
				if (s_forge_controls_ui.pending_fly_speed > 2.50f)
					s_forge_controls_ui.pending_fly_speed = 0.25f;
				return TRUE;
			}
		}
		return orig_pc_menu_event_function_invoke(widget, event_ptr, function_index, widget_deleted);
	}
	return FALSE;
}

static void forge_controls_menu_update(struct forge_widget_instance *list)
{
	int group = fw_controls_group(list);
	struct forge_widget_instance *row;
	struct forge_widget_instance *focused_row = fw_control_row(list ? list->focused_child : NULL);
	float lx, ly, rx, ry, lt, rt;

	forge_controls_ui_ensure_loaded();

	if (group < 4)
	{
		s_forge_controls_ui.capturing_action = -1;
		for (row = list ? list->child : NULL; row; row = row->next)
		{
			if (row->name && strncmp(row->name, "op_command_", 11) == 0)
			{
				int idx = atoi(row->name + 11) - 1;
				if (idx >= 0 && idx < 7)
				{
					row->vertical_offset = (short)(103 + idx * 30);
				}
				else if (idx >= 7)
				{
					row->visible = FALSE;
					if (list->focused_child == row)
					{
						list->focused_child = list->child;
						list->parameters.list.selected_index = 0;
					}
				}
			}
		}
		return;
	}

	forge_poll_controller_state(&lx, &ly, &rx, &ry, &lt, &rt);

	/* Poll for new Keyboard/Mouse or Controller button press when capturing */
	if (s_forge_controls_ui.capturing_action >= 0 && s_forge_controls_ui.capturing_action < FORGE_BIND_COUNT)
	{
		unsigned long now_ms = system_milliseconds();
		int act = s_forge_controls_ui.capturing_action;
		int vk, gp;

		if (now_ms - s_forge_controls_ui.capture_start_ms >= 120)
		{
			int esc_down = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
			int del_down = ((GetAsyncKeyState(VK_DELETE) & 0x8000) || (GetAsyncKeyState(VK_BACK) & 0x8000)) != 0;

			if (esc_down && !s_forge_controls_ui.prev_vk_down[VK_ESCAPE])
			{
				s_forge_controls_ui.capturing_action = -1;
			}
			else if (del_down && (!s_forge_controls_ui.prev_vk_down[VK_DELETE] && !s_forge_controls_ui.prev_vk_down[VK_BACK]))
			{
				if (s_forge_controls_ui.capturing_slot == 0)
					s_forge_controls_ui.pending_vk[act] = 0;
				else
					s_forge_controls_ui.pending_gp[act] = FORGE_GP_NONE;
				s_forge_controls_ui.capturing_action = -1;
			}
			else
			{
				/* Check physical gamepad buttons first -> binds Controller column (slot 1) */
				for (gp = 1; gp < FORGE_GP_COUNT; gp++)
				{
					int down = forge_gp_button_down(gp);
					if (down && !s_forge_controls_ui.prev_gp_down[gp])
					{
						int other;
						for (other = 0; other < FORGE_BIND_COUNT; other++)
						{
							if (s_forge_controls_ui.pending_gp[other] == gp)
							{
								if ((act == FORGE_BIND_PALETTE && other == FORGE_BIND_OBJECT_OPTIONS) ||
									(act == FORGE_BIND_OBJECT_OPTIONS && other == FORGE_BIND_PALETTE))
								{
									continue;
								}
								s_forge_controls_ui.pending_gp[other] = FORGE_GP_NONE;
							}
						}
						s_forge_controls_ui.pending_gp[act] = gp;
						s_forge_controls_ui.capturing_action = -1;
						break;
					}
				}

				/* Check keyboard & mouse buttons -> binds Key/Mouse column (slot 0) */
				if (s_forge_controls_ui.capturing_action >= 0)
				{
					for (vk = 1; vk < 254; vk++)
					{
						int down;
						if (vk == VK_ESCAPE || vk == VK_DELETE || vk == VK_BACK)
							continue;
						if (vk == VK_SHIFT || vk == VK_CONTROL || vk == 0x12 /* VK_MENU */)
							continue;
						if ((vk == VK_LBUTTON || vk == VK_RETURN) && (now_ms - s_forge_controls_ui.capture_start_ms < 260))
							continue;
						down = (GetAsyncKeyState(vk) & 0x8000) != 0;
						if (down && !s_forge_controls_ui.prev_vk_down[vk])
						{
							char const *vname = forge_vk_display_name(vk);
							if (vname && strcmp(vname, "-") != 0)
							{
								int other;
								for (other = 0; other < FORGE_BIND_COUNT; other++)
								{
									if (s_forge_controls_ui.pending_vk[other] == vk)
										s_forge_controls_ui.pending_vk[other] = 0;
								}
								s_forge_controls_ui.pending_vk[act] = vk;
								s_forge_controls_ui.capturing_action = -1;
								break;
							}
						}
					}
				}
			}
		}

		for (vk = 1; vk < 255; vk++)
			s_forge_controls_ui.prev_vk_down[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
		for (gp = 1; gp < FORGE_GP_COUNT; gp++)
			s_forge_controls_ui.prev_gp_down[gp] = forge_gp_button_down(gp);
	}

	/* Populate rows for group == 4 (FORGE) or group == 5 (FORGE PAD) */
	for (row = list ? list->child : NULL; row; row = row->next)
	{
		int idx;
		struct forge_widget_instance *lbl_w;
		struct forge_widget_instance *bind0_w;
		struct forge_widget_instance *bind1_w;
		char col0[64], col1[64];

		if (!row->name || strncmp(row->name, "op_command_", 11) != 0)
			continue;
		idx = atoi(row->name + 11) - 1;
		lbl_w = fw_named(row, "command_label", 0);
		bind0_w = fw_named(row, "command_binding", 0);
		bind1_w = fw_named(row, "command_binding", 1);

		if (group == 4)
		{
			if (idx < 0 || idx >= FORGE_BIND_COUNT)
			{
				row->visible = FALSE;
				continue;
			}
			row->visible = TRUE;
			row->vertical_offset = (short)(98 + idx * 24);
			fw_text_set_ascii(lbl_w, s_forge_bind_labels[idx]);

			if (s_forge_controls_ui.capturing_action == idx && s_forge_controls_ui.capturing_slot == 0)
				snprintf(col0, sizeof(col0), "PRESS KEY/PAD");
			else if (row == focused_row && s_forge_controls_ui.slot == 0)
				snprintf(col0, sizeof(col0), "> %s <", forge_vk_display_name(s_forge_controls_ui.pending_vk[idx]));
			else
				snprintf(col0, sizeof(col0), "%s", forge_vk_display_name(s_forge_controls_ui.pending_vk[idx]));

			if (s_forge_controls_ui.capturing_action == idx && s_forge_controls_ui.capturing_slot == 1)
				snprintf(col1, sizeof(col1), "PRESS KEY/PAD");
			else if (row == focused_row && s_forge_controls_ui.slot == 1)
				snprintf(col1, sizeof(col1), "> %s <", forge_gp_display_name(s_forge_controls_ui.pending_gp[idx]));
			else
				snprintf(col1, sizeof(col1), "%s", forge_gp_display_name(s_forge_controls_ui.pending_gp[idx]));

			fw_text_set_ascii(bind0_w, col0);
			fw_text_set_ascii(bind1_w, col1);
		}
		else if (group == 5)
		{
			if (idx == 0 || idx == 1)
			{
				int pct = (int)((idx == 0 ? s_forge_controls_ui.pending_look_sens : s_forge_controls_ui.pending_fly_speed) * 100.0f + 0.5f);
				row->visible = TRUE;
				row->vertical_offset = (short)(103 + idx * 30);
				fw_text_set_ascii(lbl_w, (idx == 0) ? "PAD LOOK SENS" : "PAD FLY SPEED");
				if (row == focused_row)
					snprintf(col0, sizeof(col0), "< %d%% >", pct);
				else
					snprintf(col0, sizeof(col0), "%d%%", pct);
				snprintf(col1, sizeof(col1), "LEFT / RIGHT");
				fw_text_set_ascii(bind0_w, col0);
				fw_text_set_ascii(bind1_w, col1);
			}
			else
			{
				row->visible = FALSE;
				if (list->focused_child == row)
				{
					list->focused_child = list->child;
					list->parameters.list.selected_index = 0;
				}
			}
		}
	}

	if (list && list->parameters.list.extended_description)
	{
		list->parameters.list.extended_description->parameters.text_box.string_list_index =
			(s_forge_controls_ui.capturing_action >= 0) ? 2 : (focused_row ? 1 : 0);
	}
}

void pc_menu_game_data_function_invoke(
	void *widget_ptr,
	long function)
{
	struct forge_widget_instance *widget = (struct forge_widget_instance *)widget_ptr;
	char const *fn_name = pc_menu_game_data_input_name(function);
	char const *w_name = (widget && widget->name) ? widget->name : "";
	char const *p_name = (widget && widget->parent && widget->parent->name) ? widget->parent->name : "";

	if (strncmp(w_name, "forge_", 6) == 0 || strncmp(p_name, "forge_", 6) == 0)
	{
		g_forge_session_enabled = 1;
	}

	if (fn_name && strcmp(fn_name, "load game list update") == 0 &&
		strcmp(w_name, "forge_edit_maps_list") == 0)
	{
		update_forge_edit_maps_widget(widget);
		return;
	}
	if (fn_name && strcmp(fn_name, "load game list update") == 0 &&
		strcmp(w_name, "forge_add_tags_map_list") == 0)
	{
		update_forge_add_tags_map_widget(widget);
		return;
	}
	if (fn_name && strcmp(fn_name, "load game list update") == 0 &&
		strcmp(w_name, "forge_add_tags_browser_list") == 0)
	{
		update_forge_add_tags_browser_widget(widget);
		return;
	}
	if (fn_name && strcmp(fn_name, "load game list update") == 0 &&
		strcmp(w_name, "forge_name_tag_button_bar") == 0)
	{
		update_forge_name_tag_dialog(widget);
		return;
	}
	if (fn_name && strcmp(fn_name, "load game list update") == 0 &&
		strcmp(w_name, "forge_installed_tags_list") == 0)
	{
		update_forge_installed_tags_widget(widget);
		return;
	}
	if (fn_name && strcmp(fn_name, "load game list update") == 0 &&
		strcmp(w_name, "forge_installed_tag_button_bar") == 0)
	{
		update_forge_installed_tag_action_dialog(widget);
		return;
	}
	if (fn_name && strcmp(fn_name, "controls update menu") == 0)
	{
		int group = fw_controls_group(widget);
		if (group >= 4)
		{
			forge_controls_menu_update(widget);
			return;
		}
		orig_pc_menu_game_data_function_invoke(widget_ptr, function);
		forge_controls_menu_update(widget);
		return;
	}

	/* Point 5: Cycle Forge map chooser across 3 tabs when in Forge mode:
	 *   1 = MULTIPLAYER
	 *   3 = CUSTOM MULTIPLAYER (non-Forge custom maps only)
	 *   6 = FORGE MAPS (Forge-made/edited maps only)
	 */
	if (g_forge_session_enabled && fn_name &&
		(strcmp(fn_name, "mp map list update") == 0 ||
		 strcmp(fn_name, "solo map list update") == 0))
	{
		struct forge_widget_instance *spinner = fw_named(widget, "list_item_0_map_kind_spinner", 0);
		short kind = spinner ? spinner->parameters.list.selected_index : 1;

		if (spinner)
		{
			if (s_forge_last_map_kind < 0)
			{
				kind = 1;
			}
			else if (kind != s_forge_last_map_kind)
			{
				if (s_forge_last_map_kind == 1)
					kind = (kind == 0) ? FORGE_MAP_KIND_FORGE_MAPS : 3;
				else if (s_forge_last_map_kind == 3)
					kind = (kind == 2) ? 1 : FORGE_MAP_KIND_FORGE_MAPS;
				else if (s_forge_last_map_kind == FORGE_MAP_KIND_FORGE_MAPS)
					kind = (kind == (FORGE_MAP_KIND_FORGE_MAPS - 1)) ? 3 : 1;
				else
					kind = 1;
			}
			else if (kind != 1 && kind != 3 && kind != FORGE_MAP_KIND_FORGE_MAPS)
			{
				kind = 1;
			}

			/* Whenever switching tabs, pulse menu_functions' internal kind for one call so s_levels resets first=chosen=0 */
			if (kind != s_forge_last_map_kind)
			{
				s_forge_active_map_kind = 1;
				spinner->parameters.list.selected_index = (kind == 1) ? 0 : 1;
				orig_pc_menu_game_data_function_invoke(widget_ptr, function);
				s_forge_last_map_kind = kind;
			}

			s_forge_active_map_kind = kind;
			spinner->parameters.list.selected_index = (kind == FORGE_MAP_KIND_FORGE_MAPS) ? 3 : kind;
			orig_pc_menu_game_data_function_invoke(widget_ptr, function);
			spinner->parameters.list.selected_index = kind;
			return;
		}
	}
	else if (!g_forge_session_enabled && fn_name &&
		(strcmp(fn_name, "solo map list update") == 0 ||
		 strcmp(fn_name, "mp map list update") == 0))
	{
		struct forge_widget_instance *spinner = fw_named(widget, "list_item_0_map_kind_spinner", 0);
		if (spinner)
		{
			short kind = spinner->parameters.list.selected_index;
			if (strcmp(fn_name, "mp map list update") == 0 && s_forge_last_map_kind == 1 && kind == 0)
			{
				kind = FORGE_MAP_KIND_FORGE_MAPS;
				spinner->parameters.list.selected_index = FORGE_MAP_KIND_FORGE_MAPS;
			}
			if (kind != s_forge_last_map_kind && (kind == 3 || kind == FORGE_MAP_KIND_FORGE_MAPS) && (s_forge_last_map_kind == 3 || s_forge_last_map_kind == FORGE_MAP_KIND_FORGE_MAPS))
			{
				s_forge_active_map_kind = 1;
				spinner->parameters.list.selected_index = 1;
				orig_pc_menu_game_data_function_invoke(widget_ptr, function);
			}
			s_forge_active_map_kind = kind;
			spinner->parameters.list.selected_index = (kind == FORGE_MAP_KIND_FORGE_MAPS) ? 3 : kind;
			orig_pc_menu_game_data_function_invoke(widget_ptr, function);
			if (kind == FORGE_MAP_KIND_FORGE_MAPS)
				spinner->parameters.list.selected_index = FORGE_MAP_KIND_FORGE_MAPS;
			s_forge_last_map_kind = spinner->parameters.list.selected_index;
			return;
		}
	}

	orig_pc_menu_game_data_function_invoke(widget_ptr, function);

	/* Point 7: Hide gametype rows on Server Setup screen when hosting a Multiplayer Forge session */
	if (g_forge_session_enabled && fn_name && strcmp(fn_name, "server settings update") == 0)
	{
		static char const *const gt_rows[] = {
			"op_game_type", "op_player_options", "op_item_options",
			"op_vehicle_options", "op_indicator_options", "op_team_options"
		};
		int r;
		for (r = 0; r < 6; r++)
		{
			struct forge_widget_instance *row_w = fw_named(widget, gt_rows[r], 0);
			if (row_w)
				row_w->visible = FALSE;
		}
	}
}

/* ---------- Frame Update, HUD, Camera, 343 Guilty Spark Model & Console Hooks */

static void forge_reset_transient_session_state(int clear_saved_map_state)
{
	g_forge.monitor_cam_initialized = 0;
	g_forge.grabbed_live_index = -1;
	g_forge.grabbed_object_index = NONE;
	g_forge.hovered_object_index = NONE;
	g_forge.hovered_live_index = -1;
	g_forge.hovered_object_label[0] = 0;
	forge_3d_reset_session_tracking();
	if (clear_saved_map_state)
	{
		g_forge.live_instance_count = 0;
		g_forge.has_saved_map_once = 0;
		g_forge.save_map_name[0] = 0;
		g_forge.save_map_name_len = 0;
		memset(&g_forge.edit_state, 0, sizeof(g_forge.edit_state));
		forge_network_reset_session();
	}
}

int forge_is_main_menu_active(void)
{
	struct scenario *scen = global_scenario_try_and_get();
	char const *cur_map = forge_get_active_map_name();
	if (!scen || scen->type == 2 /* _scenario_type_main_menu */)
		return 1;
	if (cur_map && (_stricmp(cur_map, "levels\\ui\\ui") == 0 || _stricmp(cur_map, "ui") == 0))
		return 1;
	return 0;
}

void network_chat_update(void)
{
	unsigned long now = system_milliseconds();
	float dt = 0.016f;
	char const *cur_map = forge_get_active_map_name();

	orig_network_chat_update();

	/* When on the UI main menu (levels\ui\ui), main_load_ui_scenario leaves main_get_map_name()
	 * set to the previous playable map while global_scenario_get()->type == _scenario_type_main_menu (2).
	 * Reset Forge monitor/modal state and never run Forge camera/update hooks on the UI background scene! */
	if (forge_is_main_menu_active())
	{
		g_last_forge_map_name[0] = 0;
		g_forge.active_modal = _forge_modal_none;
		g_forge.mode = _forge_mode_play;
		forge_reset_transient_session_state(0);
		return;
	}

	/* Auto-enable g_forge_session_enabled if we joined a Forge game via standard Join Game */
	if (!g_forge_session_enabled)
	{
		struct game_variant const *v = game_engine_get_variant();
		if (v && (_wcsicmp(v->human_readable_game_description, L"Forge") == 0 ||
				  v->universal_variant.score_to_win == 0x3FFFFFFF))
		{
			g_forge_session_enabled = 1;
		}
	}

	/* Reset per-map monitor state only when switching between different active maps */
	if (cur_map && cur_map[0] && _stricmp(cur_map, g_last_forge_map_name) != 0)
	{
		int had_previous_map = (g_last_forge_map_name[0] != 0);
		snprintf(g_last_forge_map_name, sizeof(g_last_forge_map_name), "%s", cur_map);
		g_forge.active_modal = _forge_modal_none;
		if (had_previous_map)
			forge_reset_transient_session_state(1);
	}

	if (g_last_forge_frame_ms != 0)
	{
		unsigned long diff = now - g_last_forge_frame_ms;
		if (diff > 0 && diff < 250)
			dt = (float)diff / 1000.0f;
	}
	g_last_forge_frame_ms = now;

	forge_3d_update(dt);
}

void network_chat_draw(short local_player_index)
{
	long font_index;
	struct font_header *font;
	short line_height = 13;
	rectangle2d window;

	orig_network_chat_draw(local_player_index);

	/* Never draw Forge HUD overlay on the main menu (levels\ui\ui) or outside a Forge session */
	if (!g_forge_session_enabled || forge_is_main_menu_active())
		return;

	font_index = hud_get_font_index();
	if (font_index != NONE)
	{
		font = font_definition_get(font_index);
		if (font)
			line_height = (short)(font->ascending_height + font->descending_height + font->leading_height);
	}
	if (line_height <= 0)
		line_height = 13;

	window = render.camera.window_bounds;
	offset_rectangle2d(&window, -render.camera.viewport_bounds.x0, -render.camera.viewport_bounds.y0);

	forge_3d_draw_hud(
		local_player_index,
		window.x0,
		window.y0,
		window.x1,
		window.y1,
		font_index,
		line_height);
}

boolean network_chat_typing(void)
{
	if (orig_network_chat_typing())
		return TRUE;
	/* Only block native keyboard/Escape input when a Forge modal is open on screen! */
	if (g_forge_session_enabled && g_forge.initialized &&
		!forge_is_main_menu_active() &&
		g_forge.active_modal != _forge_modal_none)
	{
		return TRUE;
	}
	return FALSE;
}

void network_chat_handle_up(long machine_index, void const *entries, boolean from_stream)
{
	if (entries && from_stream)
	{
		char const *text = (char const *)entries + 4;
		if (forge_network_intercept_chat_packet(machine_index, text, 1))
			return;
	}
	orig_network_chat_handle_up(machine_index, entries, from_stream);
}

void network_chat_handle_down(void const *entries, boolean from_stream)
{
	if (entries && from_stream)
	{
		char const *text = (char const *)entries + 4;
		if (forge_network_intercept_chat_packet(NONE, text, 0))
			return;
	}
	orig_network_chat_handle_down(entries, from_stream);
}

void network_objects_handle_changes(void const *entries, short count)
{
	forge_network_pre_handle_object_changes(entries, count);
	orig_network_objects_handle_changes(entries, count);
	forge_network_post_handle_object_changes(entries, count);
}

void director_update(real time_delta_sec)
{
	orig_director_update(time_delta_sec);
	if (g_forge_session_enabled && g_forge.initialized && !forge_is_main_menu_active())
	{
		forge_3d_apply_monitor_camera(0);
	}
}

director_perspective director_get_perspective(short local_player_index)
{
	if (g_forge_session_enabled && g_forge.initialized && !forge_is_main_menu_active())
	{
		if (g_forge.mode == _forge_mode_3d_monitor && g_forge.third_person_view)
		{
			return _director_perspective_third_person; /* 3rd-person perspective so Monitor is drawn */
		}
	}
	return orig_director_get_perspective(local_player_index);
}

struct observer_result const *render_interpolation_camera(
	short local_player_index,
	struct observer_result const *fallback)
{
	struct observer_result const *base = orig_render_interpolation_camera(local_player_index, fallback);
	if (g_forge_session_enabled && g_forge.initialized && !forge_is_main_menu_active())
	{
		forge_render_begin_monitor_visibility();
	}
	if (base && g_forge_session_enabled && g_forge.initialized &&
		!forge_is_main_menu_active() && local_player_index == 0 &&
		g_forge.mode == _forge_mode_3d_monitor)
	{
		static struct observer_result s_live_obs;
		s_live_obs = *base;
		if (forge_3d_fill_live_observer_result(&s_live_obs))
			return &s_live_obs;
	}
	return base;
}

void first_person_weapon_draw(void)
{
	if (g_forge_session_enabled && g_forge.initialized &&
		g_forge.mode != _forge_mode_play &&
		!forge_is_main_menu_active())
	{
		/* Hide player's first-person hands & weapon model when in Forge Mode */
		return;
	}
	orig_first_person_weapon_draw();
}

extern void rasterizer_set_stencil_mode(long stencil_mode);
extern void rasterizer_set_frustum_z(real z_near, real z_far);

void game_engine_post_rasterize_objects(void)
{
	if (g_forge_session_enabled && g_forge.initialized && !forge_is_main_menu_active())
	{
		forge_render_end_monitor_visibility();
	}
	orig_game_engine_post_rasterize_objects();

	if (g_forge_session_enabled && g_forge.initialized && !forge_is_main_menu_active())
	{
		/* Render 3D Translucent Objective/Spawn/Teleporter Markers and 3D Spawn Menu Preview
		 * AFTER sky, opaque objects, BSP lightmaps, BSP diffuse textures, and fog have been drawn!
		 * Reset stencil mode to NONE and restore camera frustum constants so world markers never get rejected! */
		rasterizer_set_stencil_mode(0);
		rasterizer_set_frustum_z(0.0f, 0.0f);
		rasterizer_models_begin(FALSE);
		forge_render_world_3d_overlays(NULL);
		rasterizer_models_end();
	}
}

void hud_render_weapon_interface(struct player_datum *player)
{
	if (g_forge_session_enabled && g_forge.initialized &&
		g_forge.mode != _forge_mode_play &&
		!forge_is_main_menu_active())
	{
		/* Hide weapon HUD, weapon reticle, and 'No Weapon' prompt in Forge mode (unarmed reticle drawn in forge_3d_draw_hud) */
		return;
	}
	orig_hud_render_weapon_interface(player);
}

void render_model(
	long model_index,
	real level_of_detail_pixels,
	struct real_matrix4x3 const *node_matrices,
	char const *region_permutation_indices,
	union real_rgb_color const *change_colors,
	real const *function_values,
	struct render_lighting const *lighting,
	union real_point3d const *centroid,
	real radius,
	struct render_model_effect const *model_effect,
	long unique_identifier,
	short forced_shader_permutation_index,
	unsigned long flags)
{
	if (g_forge_session_enabled && g_forge.initialized && !forge_is_main_menu_active())
	{
		if (unique_identifier != NONE)
		{
			/* Point 10: Hide floating gun or child items attached to any player biped that is in Monitor mode */
			{
				struct object_datum *obj = (struct object_datum *)object_try_and_get_and_verify_type(unique_identifier, (unsigned long)-1);
				if (obj && obj->object.parent_object_index != NONE &&
					forge_is_unit_in_monitor_mode(obj->object.parent_object_index))
				{
					return;
				}
			}
			if (g_forge.mode == _forge_mode_3d_monitor &&
				!g_forge.third_person_view &&
				player_control_get_unit_index(0) == unique_identifier)
			{
				return;
			}

			/* Render dynamic Cached Tag objects and Monitor player units */
			if (forge_monitor_render_player_unit(unique_identifier, lighting, flags))
				return;

			/* For the currently held Forge object, bypass 30 Hz render interpolation and use live 60+ Hz node matrices */
			if (unique_identifier == g_forge.grabbed_object_index && g_forge.grabbed_live_index >= 0)
			{
				struct object_datum *grab_obj = (struct object_datum *)object_try_and_get_and_verify_type(unique_identifier, (unsigned long)-1);
				if (grab_obj && grab_obj->object.node_matrices.size > 0)
				{
					struct real_matrix4x3 const *live_mats =
						(struct real_matrix4x3 const *)object_header_block_get(unique_identifier, &grab_obj->object.node_matrices);
					if (live_mats)
						node_matrices = live_mats;
				}
			}
		}
	}

	/* Ensure custom scenery/device models inside .map files never disappear at distance due to
	 * non-zero super_low detail_cutoff_pixels[0] or missing low-LOD permutation geometry_indices */
	if (unique_identifier != NONE && model_index != NONE)
	{
		struct object_datum *scen_obj = (struct object_datum *)object_try_and_get_and_verify_type(
			unique_identifier,
			_object_mask_scenery | _object_mask_machine | _object_mask_light_fixture | _object_mask_control);
		if (scen_obj)
		{
			struct model *m = model_definition_get(model_index);
			if (m)
			{
				if (m->detail_cutoff_pixels[0] > 0.0f)
					m->detail_cutoff_pixels[0] = 0.0f;
				if (m->geometries.count <= 1 && level_of_detail_pixels < m->detail_cutoff_pixels[4] + 1.0f)
					level_of_detail_pixels = m->detail_cutoff_pixels[4] + 1.0f;
				if (level_of_detail_pixels < 32.0f)
					level_of_detail_pixels = 32.0f;
			}
		}
	}

	orig_render_model(
		model_index,
		level_of_detail_pixels,
		node_matrices,
		region_permutation_indices,
		change_colors,
		function_values,
		lighting,
		centroid,
		radius,
		model_effect,
		unique_identifier,
		forced_shader_permutation_index,
		flags);
}

boolean hs_compile_and_evaluate(char const *source)
{
	if (source)
	{
		char const *s = source;
		while (*s == ' ' || *s == '\t')
			s++;
		if (_stricmp(s, "third_person") == 0 ||
			_stricmp(s, "forge_third_person") == 0 ||
			_stricmp(s, "3rd_person") == 0)
		{
			g_forge.third_person_view = !g_forge.third_person_view;
			console_printf(
				FALSE,
				"Forge 3rd-Person View: %s",
				g_forge.third_person_view ? "ON" : "OFF");
			forge_set_status(
				"3rd-Person Forge View: %s",
				g_forge.third_person_view ? "ON" : "OFF");
			return TRUE;
		}
		if (_stricmp(s, "forge") == 0 || _stricmp(s, "forge_mode") == 0)
		{
			g_forge_session_enabled = 1;
			g_forge.mode = (g_forge.mode == _forge_mode_play) ? _forge_mode_3d_monitor : _forge_mode_play;
			console_printf(
				FALSE,
				"Mode: %s",
				g_forge.mode == _forge_mode_3d_monitor ? "Forge Mode" : "Spartan Play");
			return TRUE;
		}
	}
	return orig_hs_compile_and_evaluate(source);
}

/* ---------- Point 1: Singleplayer Maps Load Straight In (No Cutscenes, No Script Trigger Points) */

void hs_runtime_update(void)
{
	if (g_forge_session_enabled && !forge_is_main_menu_active())
	{
		/* Suppress scenario startup/continuous scripts in Forge mode so SP maps load straight in with no cutscenes */
		return;
	}
	orig_hs_runtime_update();
}

void cinematic_start(void)
{
	if (g_forge_session_enabled && !forge_is_main_menu_active())
	{
		return;
	}
	orig_cinematic_start();
}

boolean scenario_trigger_volume_test_object(short trigger_volume_index, long object_index)
{
	if (g_forge_session_enabled && !forge_is_main_menu_active())
	{
		/* Suppress all script trigger volumes and BSP switch trigger volumes in Forge mode */
		return FALSE;
	}
	return orig_scenario_trigger_volume_test_object(trigger_volume_index, object_index);
}

/* ---------- Point 5: Bypass Multiplayer Gametype Vehicle/Weapon Remapping in Forge Mode */

extern boolean custom_edition_cache_tags_loaded(void);

long game_engine_remap_object_definition(long definition_index)
{
	if (g_forge_session_enabled || g_forge.has_saved_map_once || custom_edition_cache_tags_loaded())
	{
		/* Never remap or reject Scorpion, Banshee, Rocket Warthog, Shade Turret, or custom Forge vehicles! */
		return definition_index;
	}
	return orig_game_engine_remap_object_definition(definition_index);
}

/* ---------- Point 2 & 6: In-Game Escape Pause Menu "SAVE FORGE MAP" Patch & Map Unload Cleanup */

struct forge_pause_tag_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

static void *s_pause_saved_instances = NULL;
static long s_pause_saved_count = 0;
static void *s_pause_alloc_instances = NULL;

void menu_tags_unloaded(void)
{
	tag_preview_clear();
	forge_runtime_on_map_unload();
	s_save_forge_btn_tag = NONE;
	g_last_forge_map_name[0] = 0;
	g_forge.live_instance_count = 0;
	forge_reset_transient_session_state(0);
	if (s_pause_saved_instances)
	{
		cache_files_set_tag_instances(s_pause_saved_instances, s_pause_saved_count);
		s_pause_saved_instances = NULL;
		s_pause_saved_count = 0;
	}
	if (s_pause_alloc_instances)
	{
		free(s_pause_alloc_instances);
		s_pause_alloc_instances = NULL;
	}
	orig_menu_tags_unloaded();
}

void menu_tags_loaded(char const *map_name)
{
	/* Clean up any leftover LAN/Internet browser client or server when loading a solo map so LEAVE GAME returns to the main menu */
	if (map_name && _stricmp(map_name, "ui") != 0 && _stricmp(map_name, "levels\\ui\\ui") != 0 &&
		(game_connection() == _game_connection_local || !network_game_is_active()))
	{
		dispose_global_network_game_client();
		dispose_global_network_game_server();
		network_game_accept_remote_connections(FALSE);
	}

	orig_menu_tags_loaded(map_name);

	s_save_forge_btn_tag = NONE;
	g_last_forge_map_name[0] = 0;
	g_forge.live_instance_count = 0;

	if (map_name && (_stricmp(map_name, "ui") == 0 || _stricmp(map_name, "levels\\ui\\ui") == 0))
	{
		forge_reset_transient_session_state(0);
		g_forge_session_enabled = 0;
		g_forge.mode = _forge_mode_play;
		g_forge.active_modal = _forge_modal_none;
		forge_menu_custom_edition_maps_look_again();
	}
	else
	{
		struct game_variant const *v = game_engine_get_variant();
		if (!g_forge_session_enabled && v &&
			(_wcsicmp(v->human_readable_game_description, L"Forge") == 0 ||
			 v->universal_variant.score_to_win == 0x3FFFFFFF))
		{
			g_forge_session_enabled = 1;
		}
		forge_reset_transient_session_state(1);

	}

	forge_normalize_mcc_hud_tags();

	/* Patch "SAVE FORGE MAP" button into the in-game Escape Pause Menu when in a Forge session */
	if (g_forge_session_enabled && map_name && strcmp(map_name, "ui") != 0)
	{
		long settings_btn_tag = tag_loaded('DeLa', "pc\\pause\\settings_button");
		long existing_count = 0;
		struct forge_pause_tag_instance *instances =
			(struct forge_pause_tag_instance *)cache_files_tag_instances(&existing_count);

		if (settings_btn_tag != NONE && instances && existing_count > 0 && existing_count < 65000)
		{
			static uint8_t s_pause_btn_def[0x3EC];
			static struct tag_block s_pause_ustr_def;
			static struct tag_data s_pause_ustr_entry;
			static wchar_t s_pause_wtext[32];
			static uint8_t s_pause_h_dst[16 * 0x48];
			static uint8_t s_pause_children[8][17 * 0x50];
			int child_slot_used = 0;
			uint8_t const *settings_def = (uint8_t const *)tag_get('DeLa', settings_btn_tag);
			struct forge_pause_tag_instance *grown;
			uint8_t *btn_def = s_pause_btn_def;
			struct tag_block *ustr_def = &s_pause_ustr_def;
			struct tag_data *ustr_entry = &s_pause_ustr_entry;
			wchar_t *wtext = s_pause_wtext;

			if (s_pause_alloc_instances)
			{
				free(s_pause_alloc_instances);
				s_pause_alloc_instances = NULL;
			}
			grown = (struct forge_pause_tag_instance *)malloc((size_t)(existing_count + 2) * sizeof(*grown));

			if (grown && settings_def)
			{
				static char const *const label = "SAVE MAP..";
				long max_salt = 0xE174;
				long i, ustr_tag, btn_tag;
				int32_t h_count;
				uint8_t const *h_src;

				memset(ustr_def, 0, sizeof(*ustr_def));
				memset(ustr_entry, 0, sizeof(*ustr_entry));
				memset(wtext, 0, sizeof(s_pause_wtext));
				for (i = 0; label[i]; i++)
					wtext[i] = (wchar_t)(unsigned char)label[i];
				wtext[i] = 0;

				ustr_entry->size = (long)((i + 1) * sizeof(wchar_t));
				ustr_entry->address = wtext;
				ustr_def->count = 1;
				ustr_def->address = ustr_entry;

				memcpy(grown, instances, (size_t)existing_count * sizeof(*grown));
				for (i = 0; i < existing_count; i++)
				{
					if (instances[i].tag_index != NONE)
					{
						long s = (long)(((unsigned long)instances[i].tag_index >> 16) & 0x7FFFu);
						if (s > max_salt)
							max_salt = s;
					}
				}

				ustr_tag = ((max_salt + 1) << 16) | (existing_count & 0xFFFF);
				btn_tag = ((max_salt + 2) << 16) | ((existing_count + 1) & 0xFFFF);
				s_save_forge_btn_tag = btn_tag;

				memset(&grown[existing_count], 0, sizeof(grown[0]));
				grown[existing_count].group_tag = 'ustr';
				grown[existing_count].parent_group_tags[0] = NONE;
				grown[existing_count].parent_group_tags[1] = NONE;
				grown[existing_count].tag_index = ustr_tag;
				grown[existing_count].name = "pc\\pause\\save_forge_button text";
				grown[existing_count].base_address = ustr_def;

				memcpy(btn_def, settings_def, 0x3EC);
				snprintf((char *)(btn_def + 0x04), 32, "save_forge_button");
				/* Set text_label_string_list at offset 0xEC to our ustr_tag */
				*(int32_t *)(btn_def + 0xEC + 0x00) = 'ustr';
				*(char const **)(btn_def + 0xEC + 0x04) = "pc\\pause\\save_forge_button text";
				*(int32_t *)(btn_def + 0xEC + 0x08) = 0;
				*(int32_t *)(btn_def + 0xEC + 0x0C) = ustr_tag;

				/* Clone event_handlers at offset 0x54 -> run function 0x7777 & close all pause widgets */
				h_count = *(int32_t const *)(settings_def + 0x54);
				h_src = *(uint8_t const *const *)(settings_def + 0x58);
				if (h_count > 0 && h_count <= 16 && h_src)
				{
					uint8_t *h_dst = s_pause_h_dst;
					int h;
					memcpy(h_dst, h_src, (size_t)h_count * 0x48u);
					for (h = 0; h < h_count; h++)
					{
						uint8_t *hp = h_dst + (size_t)h * 0x48u;
						/* _event_handler_run_function_bit (bit 7) | _event_handler_close_all_widgets_bit (bit 2) */
						*(int32_t *)(hp + 0x00) = (1 << 7) | (1 << 2);
						*(int16_t *)(hp + 0x06) = 0x7777;
						*(int32_t *)(hp + 0x08) = 'DeLa';
						*(void **)(hp + 0x0C) = NULL;
						*(int32_t *)(hp + 0x10) = 0;
						*(int32_t *)(hp + 0x14) = NONE;
					}
					*(uint8_t **)(btn_def + 0x58) = h_dst;
				}

				memset(&grown[existing_count + 1], 0, sizeof(grown[0]));
				grown[existing_count + 1].group_tag = 'DeLa';
				grown[existing_count + 1].parent_group_tags[0] = NONE;
				grown[existing_count + 1].parent_group_tags[1] = NONE;
				grown[existing_count + 1].tag_index = btn_tag;
				grown[existing_count + 1].name = "pc\\pause\\save_forge_button";
				grown[existing_count + 1].base_address = btn_def;

				s_pause_saved_instances = instances;
				s_pause_saved_count = existing_count;
				s_pause_alloc_instances = grown;
				cache_files_set_tag_instances(grown, existing_count + 2);

				/* Insert save_forge_button into the pause menu's column list right before settings_button */
				for (i = 0; i < existing_count; i++)
				{
					uint8_t *w_def;
					int32_t c_count, c, found_at = -1;
					uint8_t *c_arr;

					if (grown[i].group_tag != 'DeLa' || !grown[i].base_address)
						continue;
					w_def = (uint8_t *)grown[i].base_address;
					if (*(int16_t const *)(w_def + 0x00) != 3) /* _widget_type_column_list */
						continue;
					c_count = *(int32_t const *)(w_def + 0x3E0);
					c_arr = *(uint8_t *const *)(w_def + 0x3E4);
					if (c_count <= 0 || c_count > 16 || !c_arr)
						continue;
					for (c = 0; c < c_count; c++)
					{
						int32_t child_tag = *(int32_t const *)(c_arr + (size_t)c * 0x50u + 0x0C);
						long abs_idx = child_tag & 0xFFFF;
						if (child_tag == settings_btn_tag ||
							(child_tag != NONE && abs_idx >= 0 && abs_idx < existing_count &&
							 grown[abs_idx].name && strcmp(grown[abs_idx].name, "pc\\pause\\settings_button") == 0))
						{
							found_at = c;
							break;
						}
					}
					if (found_at >= 0 && child_slot_used < 8)
					{
						uint8_t *new_children = s_pause_children[child_slot_used++];
						int16_t old_y0 = *(int16_t const *)(c_arr + 0 * 0x50u + 0x36);
						int16_t spacing = (c_count + 1 <= 4) ? 29 : 25;
						int16_t base_y = (old_y0 >= 60) ? (int16_t)(old_y0 - 20) : 0;
						long list_tag_idx = grown[i].tag_index;
						long s_idx;

						if (found_at > 0)
							memcpy(new_children, c_arr, (size_t)found_at * 0x50u);
						memcpy(new_children + (size_t)found_at * 0x50u, c_arr + (size_t)found_at * 0x50u, 0x50u);
						*(int32_t *)(new_children + (size_t)found_at * 0x50u + 0x00) = 'DeLa';
						*(char const **)(new_children + (size_t)found_at * 0x50u + 0x04) = "pc\\pause\\save_forge_button";
						*(int32_t *)(new_children + (size_t)found_at * 0x50u + 0x08) = 0;
						*(int32_t *)(new_children + (size_t)found_at * 0x50u + 0x0C) = btn_tag;

						for (c = found_at; c < c_count; c++)
						{
							uint8_t *dst_c = new_children + (size_t)(c + 1) * 0x50u;
							memcpy(dst_c, c_arr + (size_t)c * 0x50u, 0x50u);
						}
						for (c = 0; c < c_count + 1; c++)
						{
							uint8_t *dst_c = new_children + (size_t)c * 0x50u;
							*(int16_t *)(dst_c + 0x36) = (int16_t)(base_y + c * spacing);
						}
						*(int32_t *)(w_def + 0x3E0) = c_count + 1;
						*(uint8_t **)(w_def + 0x3E4) = new_children;
						*(int16_t *)(w_def + 0x28) = (int16_t)(*(int16_t const *)(w_def + 0x28) + spacing); /* bounds.y1 */

						/* Shift the pause list widget itself upward inside its parent screen widget so all 4 options sit centered inside the box */
						if (old_y0 < 60)
						{
							int16_t lift = (old_y0 > 10) ? 14 : 22;
							for (s_idx = 0; s_idx < existing_count; s_idx++)
							{
								uint8_t *sw_def;
								int32_t sc_count, sc;
								uint8_t *sc_arr;
								if (grown[s_idx].group_tag != 'DeLa' || !grown[s_idx].base_address)
									continue;
								sw_def = (uint8_t *)grown[s_idx].base_address;
								sc_count = *(int32_t const *)(sw_def + 0x3E0);
								sc_arr = *(uint8_t *const *)(sw_def + 0x3E4);
								if (sc_count <= 0 || sc_count > 16 || !sc_arr)
									continue;
								for (sc = 0; sc < sc_count; sc++)
								{
									if (*(int32_t const *)(sc_arr + (size_t)sc * 0x50u + 0x0C) == list_tag_idx)
									{
										*(int16_t *)(sc_arr + (size_t)sc * 0x50u + 0x36) =
											(int16_t)(*(int16_t const *)(sc_arr + (size_t)sc * 0x50u + 0x36) - lift);
									}
								}
							}
						}
					}
				}
			}
			else if (grown)
			{
				free(grown);
			}
		}
	}

	/* Ensure local tag cache is extracted and loaded when in a Forge session */
	if (g_forge_session_enabled)
		forge_runtime_init();
}

/* ---------- Point 11: Serve embedded .bmp / .txt sidecars from custom_maps\<name>.map header */

#define FORGE_EMBED_FILE_SENTINEL ((FILE *)(uintptr_t)0x7F046E99u)

static struct
{
	int active;
	uint8_t *data;
	size_t size;
	size_t pos;
} s_embed_vfile;

static FILE *try_open_embedded_map_sidecar(char const *filename)
{
	char const *leaf;
	char const *dot;
	size_t stem_len;
	char stem[128];
	char map_path[512];
	int is_bmp = 0, is_txt = 0;
	FILE *mf;
	uint8_t hdr[0x200];
	uint32_t sig = 0, emb_off = 0, emb_sz = 0;

	if (!filename || !filename[0])
		return NULL;
	leaf = strrchr(filename, '\\');
	if (!leaf)
		leaf = strrchr(filename, '/');
	leaf = leaf ? (leaf + 1) : filename;
	dot = strrchr(leaf, '.');
	if (!dot)
		return NULL;
	if (_stricmp(dot, ".bmp") == 0)
		is_bmp = 1;
	else if (_stricmp(dot, ".txt") == 0)
		is_txt = 1;
	else
		return NULL;

	stem_len = (size_t)(dot - leaf);
	if (stem_len == 0 || stem_len >= sizeof(stem))
		return NULL;
	memcpy(stem, leaf, stem_len);
	stem[stem_len] = 0;

	snprintf(map_path, sizeof(map_path), "custom_maps/%s.map", stem);
	mf = forge_open_data_file_rb(map_path);
	if (!mf)
		return NULL;
	if (fread(hdr, 1, sizeof(hdr), mf) != sizeof(hdr))
	{
		fclose(mf);
		return NULL;
	}
	memcpy(&sig, hdr + 0x1FCu, 4);
	if (sig == 0x464C4731u)
	{
		if (is_bmp)
		{
			memcpy(&emb_off, hdr + 0x1ECu, 4);
			memcpy(&emb_sz, hdr + 0x1F0u, 4);
		}
		else if (is_txt)
		{
			memcpy(&emb_off, hdr + 0x1F4u, 4);
			memcpy(&emb_sz, hdr + 0x1F8u, 4);
		}
	}
	if (emb_off >= 0x800u && emb_sz > 0 && emb_sz <= 4u * 1024u * 1024u)
	{
		uint8_t *buf = (uint8_t *)malloc(emb_sz);
		if (buf)
		{
			if (fseek(mf, (long)emb_off, SEEK_SET) == 0 && fread(buf, 1, emb_sz, mf) == emb_sz)
			{
				fclose(mf);
				if (s_embed_vfile.data)
					free(s_embed_vfile.data);
				s_embed_vfile.active = 1;
				s_embed_vfile.data = buf;
				s_embed_vfile.size = emb_sz;
				s_embed_vfile.pos = 0;
				return FORGE_EMBED_FILE_SENTINEL;
			}
			free(buf);
		}
	}
	fclose(mf);
	return NULL;
}

FILE *forge_ce_fopen(char const *filename, char const *mode)
{
	FILE *fp = forge_platform_fopen(filename, mode);
	if (!fp)
		fp = try_open_embedded_map_sidecar(filename);
	return fp;
}

size_t forge_ce_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	if (stream == FORGE_EMBED_FILE_SENTINEL)
	{
		size_t total = size * nmemb;
		size_t avail;
		if (!s_embed_vfile.active || !s_embed_vfile.data || size == 0 || nmemb == 0)
			return 0;
		avail = (s_embed_vfile.pos < s_embed_vfile.size) ? (s_embed_vfile.size - s_embed_vfile.pos) : 0;
		if (total > avail)
			total = (avail / size) * size;
		if (total > 0)
		{
			memcpy(ptr, s_embed_vfile.data + s_embed_vfile.pos, total);
			s_embed_vfile.pos += total;
		}
		return total / size;
	}
	return fread(ptr, size, nmemb, stream);
}

int forge_ce_fseek(FILE *stream, long offset, int whence)
{
	if (stream == FORGE_EMBED_FILE_SENTINEL)
	{
		long target = 0;
		if (!s_embed_vfile.active)
			return -1;
		if (whence == SEEK_SET)
			target = offset;
		else if (whence == SEEK_CUR)
			target = (long)s_embed_vfile.pos + offset;
		else if (whence == SEEK_END)
			target = (long)s_embed_vfile.size + offset;
		else
			return -1;
		if (target < 0 || (size_t)target > s_embed_vfile.size)
			return -1;
		s_embed_vfile.pos = (size_t)target;
		return 0;
	}
	return fseek(stream, offset, whence);
}

long forge_ce_ftell(FILE *stream)
{
	if (stream == FORGE_EMBED_FILE_SENTINEL)
	{
		if (!s_embed_vfile.active)
			return -1L;
		return (long)s_embed_vfile.pos;
	}
	return ftell(stream);
}

int forge_ce_fclose(FILE *stream)
{
	if (stream == FORGE_EMBED_FILE_SENTINEL)
	{
		if (s_embed_vfile.data)
		{
			free(s_embed_vfile.data);
			s_embed_vfile.data = NULL;
		}
		s_embed_vfile.active = 0;
		s_embed_vfile.size = 0;
		s_embed_vfile.pos = 0;
		return 0;
	}
	return fclose(stream);
}

/* ---------- Disable Menu, Mouse, Keyboard & Controller Interaction When halo.exe Is Unfocused */

struct forge_sdl_ui_pointer
{
	float x, y;
	float click_x, click_y;
	BOOL moved;
	int left_clicks, right_clicks;
	int wheel_steps;
	int downs;
	float down_x, down_y;
	BOOL touch;
};

struct forge_sdl_input_state
{
	unsigned char keys[512];
	unsigned char mouse_buttons[8];
	float mouse_dx, mouse_dy;
	float mouse_wheel;
	BOOL focused;
	BOOL mouse_released;
	BOOL ui_pointer;
	BOOL menus;
};

struct forge_sdl_keystroke
{
	BYTE virtual_key;
	CHAR ascii;
	BYTE flags;
};

extern void orig_platform_input_read(struct forge_sdl_input_state *state, BOOL consume_motion);
extern int platform_window_is_focused(void);
extern short platform_vk_key_down(int vk);

int forge_window_is_focused(void)
{
	return platform_window_is_focused();
}

short forge_key_state(int vk)
{
	if (!forge_window_is_focused())
		return 0;
	return platform_vk_key_down(vk);
}

void platform_input_read(struct forge_sdl_input_state *state, BOOL consume_motion)
{
	static int s_esc_suppressed_until_up = 0;

	orig_platform_input_read(state, consume_motion);
	if (state && s_tag_browser.dialog_open)
	{
		if ((system_milliseconds() - s_tag_browser.last_dialog_ms) < 250UL)
		{
			memset(state->keys, 0, sizeof(state->keys));
		}
		else
		{
			s_tag_browser.dialog_open = 0;
			s_tag_browser.name_box_active = 0;
		}
	}
	else if (state && g_forge_session_enabled && g_forge.initialized && !forge_is_main_menu_active())
	{
		/* Suppress SDL_SCANCODE_ESCAPE (41) from opening the native Pause Menu when Escape is pressed
		 * to cancel/return a held object on the Physics Beam or close a Forge modal, and keep it
		 * suppressed until the Escape key is released. */
		if (state->keys[41])
		{
			if (g_forge.active_modal != _forge_modal_none ||
				(g_forge.mode == _forge_mode_3d_monitor && g_forge.grabbed_live_index >= 0))
			{
				s_esc_suppressed_until_up = 1;
			}
			if (s_esc_suppressed_until_up)
				state->keys[41] = 0;
		}
		else
		{
			s_esc_suppressed_until_up = 0;
		}
	}
}

/* ---------- Server Browser & LAN Browser 'FORGE' Gamemode Display & Cross-Version Compatibility */

extern byte game_modes_kind(struct game_variant const *variant, struct game_variant_options const *options);
extern wchar_t const *game_modes_kind_name(long game_engine_index, byte kind, boolean abbreviated);
extern wchar_t const *game_modes_variant_kind_name(struct game_variant const *variant, struct game_variant_options const *options);

byte forge_game_modes_kind(struct game_variant const *variant, struct game_variant_options const *options)
{
	if (variant && (_wcsicmp(variant->human_readable_game_description, L"Forge") == 0 ||
					variant->universal_variant.score_to_win == 0x3FFFFFFF))
	{
		g_forge_session_enabled = 1;
	}
	if (g_forge_session_enabled ||
		(variant && _wcsicmp(variant->human_readable_game_description, L"Forge") == 0))
	{
		return FORGE_GAME_KIND;
	}
	return game_modes_kind(variant, options);
}

wchar_t const *forge_game_modes_kind_name(long game_engine_index, byte kind, boolean abbreviated)
{
	if ((kind & ~0x80) == FORGE_GAME_KIND)
		return L"FORGE";
	return game_modes_kind_name(game_engine_index, kind, abbreviated);
}

wchar_t const *forge_game_modes_variant_kind_name(struct game_variant const *variant, struct game_variant_options const *options)
{
	if (variant && (_wcsicmp(variant->human_readable_game_description, L"Forge") == 0 ||
					variant->universal_variant.score_to_win == 0x3FFFFFFF))
	{
		g_forge_session_enabled = 1;
	}
	if (g_forge_session_enabled ||
		(variant && _wcsicmp(variant->human_readable_game_description, L"Forge") == 0))
	{
		return L"FORGE";
	}
	return game_modes_variant_kind_name(variant, options);
}

int forge_p2p_lobby_games(struct p2p_listing *games, int maximum_count)
{
	int count = p2p_lobby_games(games, maximum_count);
	int i;

	if (games)
	{
		unsigned short local_net_ver = (unsigned short)forge_get_network_version();
		for (i = 0; i < count; i++)
		{
			if (games[i].network_version == FORGE_NETWORK_VERSION ||
				(games[i].kind & ~0x80) == FORGE_GAME_KIND ||
				(games[i].gametype[0] && _stricmp(games[i].gametype, "Forge") == 0))
			{
				games[i].kind = FORGE_GAME_KIND;
			}
			if (games[i].network_version == FORGE_NETWORK_VERSION ||
				games[i].network_version == HALO_PORT_NETWORK_VERSION_STOCK)
			{
				games[i].network_version = local_net_ver;
			}
		}
	}
	return count;
}

extern byte orig_network_game_client_advertised_game_kind(
	struct network_game_client *client,
	struct network_advertised_game const *game);
extern boolean orig_network_game_client_advertised_game_compatible(
	struct network_game_client *client,
	struct network_advertised_game const *game,
	boolean tell);

int forge_p2p_join_listing(const struct p2p_listing *listing)
{
	if (listing)
	{
		if ((listing->kind & ~0x80) == FORGE_GAME_KIND ||
			listing->network_version == FORGE_NETWORK_VERSION ||
			(listing->gametype[0] && _stricmp(listing->gametype, "Forge") == 0))
		{
			g_forge_session_enabled = 1;
			forge_runtime_init();
		}
		else
		{
			g_forge_session_enabled = 0;
		}
	}
	return p2p_join_listing(listing);
}

static int forge_advertised_game_is_forge(
	struct network_game_client *client,
	struct network_advertised_game const *game,
	unsigned short *out_version)
{
	struct network_advertised_game const *base;
	struct network_advertised_game_summary summary;
	long index;

	if (out_version)
		*out_version = 0;
	if (!client || !game)
		return 0;

	base = network_game_client_get_available_games(client);
	if (!base)
		return 0;

	/* sizeof(struct network_advertised_game) == 0xE4 */
	index = (long)(((char const *)game - (char const *)base) / 0xE4);
	if (index < 0 || index >= 9)
		return 0;

	memset(&summary, 0, sizeof(summary));
	if (!network_game_client_port_advertised_game_summary(index, &summary))
		return 0;

	if (out_version)
		*out_version = summary.version;

	if (summary.version == FORGE_NETWORK_VERSION ||
		(summary.kind & ~0x80) == FORGE_GAME_KIND ||
		_wcsicmp(summary.name, L"Forge") == 0)
	{
		return 1;
	}
	return 0;
}

byte network_game_client_advertised_game_kind(
	struct network_game_client *client,
	struct network_advertised_game const *game)
{
	if (forge_advertised_game_is_forge(client, game, NULL))
		return FORGE_GAME_KIND;
	return orig_network_game_client_advertised_game_kind(client, game);
}

boolean network_game_client_advertised_game_compatible(
	struct network_game_client *client,
	struct network_advertised_game const *game,
	boolean tell)
{
	unsigned short remote_version = 0;
	if (forge_advertised_game_is_forge(client, game, &remote_version))
	{
		g_forge_session_enabled = 1;
		forge_runtime_init();
	}
	else if (remote_version == HALO_PORT_NETWORK_VERSION_STOCK)
	{
		g_forge_session_enabled = 0;
	}
	return orig_network_game_client_advertised_game_compatible(client, game, tell);
}

/* ---------- MCC-Converted Custom Edition HUD Draw Scaling Wrappers */

struct forge_hud_number_element
{
	int16_t offset_x;
	int16_t offset_y;
	float scale_i;
	float scale_j;
	int16_t multiplayer_scaling_flags;
	int16_t pad0;
	int32_t unused0[5];
	uint8_t colors[0x20];
	int8_t digits;
	uint8_t number_flags;
	int8_t fractional_digits;
	uint8_t pad1;
	int32_t unused1[3];
};

extern void orig_hud_draw_bitmap_direct(
	struct bitmap_data const *bitmap,
	short placement,
	point2d const *point,
	real_rectangle2d const *clip,
	real scale,
	real theta,
	pixel32 color,
	boolean is_interface_bitmap);

extern void orig_hud_draw_numbers(
	short local_player_index,
	struct hud_absolute_placement_definition const *absolute_placement,
	struct number_hud_element_definition const *numbers,
	short value,
	short decimal_value,
	short draw_flags,
	long flash_reference_time,
	real override_scale);

void hud_draw_bitmap_direct(
	struct bitmap_data const *bitmap,
	short placement,
	point2d const *point,
	real_rectangle2d const *clip,
	real scale,
	real theta,
	pixel32 color,
	boolean is_interface_bitmap)
{
	if (forge_is_mcc_hud_active())
	{
		int16_t bw = bitmap ? *(int16_t const *)((uint8_t const *)bitmap + 0x04) : 0;
		int16_t bh = bitmap ? *(int16_t const *)((uint8_t const *)bitmap + 0x06) : 0;
		float clip_w = clip ? (clip->x1 - clip->x0) : 0.0f;
		/* 2x bitmaps (hud_waypoints 256x256, hud_damage_arrows 462/512) need 0.5f;
		 * 4x bitmaps (hud_counter_numbers 72x48, hud_msg_icons 93x93) need 0.25f. */
		if ((bw == 256 && bh == 256) || clip_w > 0.5f)
			scale *= 0.5f;
		else
			scale *= 0.25f;
	}
	orig_hud_draw_bitmap_direct(
		bitmap,
		placement,
		point,
		clip,
		scale,
		theta,
		color,
		is_interface_bitmap);
}

void hud_draw_numbers(
	short local_player_index,
	struct hud_absolute_placement_definition const *absolute_placement,
	struct number_hud_element_definition const *numbers,
	short value,
	short decimal_value,
	short draw_flags,
	long flash_reference_time,
	real override_scale)
{
	if (forge_is_mcc_hud_active() && numbers)
	{
		struct forge_hud_number_element adjusted;
		memcpy(&adjusted, numbers, sizeof(adjusted));
		if (adjusted.digits == 3 && adjusted.fractional_digits == 1 && (adjusted.number_flags & 5) == 5)
		{
			adjusted.offset_x = (int16_t)(adjusted.offset_x - 8);
			adjusted.offset_y = (int16_t)(adjusted.offset_y - 16);
		}
		orig_hud_draw_numbers(
			local_player_index,
			absolute_placement,
			(struct number_hud_element_definition const *)&adjusted,
			value,
			decimal_value,
			draw_flags,
			flash_reference_time,
			override_scale);
		return;
	}
	orig_hud_draw_numbers(
		local_player_index,
		absolute_placement,
		numbers,
		value,
		decimal_value,
		draw_flags,
		flash_reference_time,
		override_scale);
}
