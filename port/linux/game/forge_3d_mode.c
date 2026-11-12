/*
 * OpenCE : Forge
 * Mode 2: 3D In-Engine Mode (Single-Player & Online Multiplayer / Co-op)
 * - 6DOF 343 Guilty Spark Monitor flight with native Mouse Look
 * - 3rd-Person Monitor View toggle (F4 or console `third_person`)
 * - Physics Beam / Raycast Object Grab, Move, Rotate (15/45/90 deg snap) & Ground Snap
 * - Halo 3-Style Left-Aligned Compact Forge Menu & Object Properties using small terminal font
 * - Interactive AI Unit Placement (Friendly vs. Enemy)
 * - "Inject Custom Tags" UI Modal & "Save Copy As..." Standalone Map Export Modal
 */

#ifndef HALO_RELEASE
#define HALO_RELEASE
#endif
#include "cseries.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math/real_math.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"
#include "camera/director.h"
#include "camera/observer.h"
#include "objects/objects.h"
#include "units/units.h"
#include "physics/collisions.h"
#include "interface/interface.h"
#include "interface/ui_widget.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "render/render_cameras.h"
#include "text/draw_string.h"
#include "text/font_group.h"
#include "items/items.h"
#include "items/item_definitions.h"

#include "objects/object_lights.h"
#include "objects/widgets/widgets.h"
#include "devices/devices.h"
#include "devices/device_light_fixtures.h"
#include "mcc_cache.h"
#include "forge_engine.h"

#undef fopen
#undef remove
#define fopen(p, m) ((FILE *)forge_host_fopen((p), (m)))
#define remove(p) forge_host_remove((p))

extern unsigned long system_milliseconds(void);
extern int halo_linux_mouse_look(short gamepad_index, float *yaw, float *pitch);
extern void platform_scoreboard_scroll(int open, long *notches, long *pages);
extern boolean render_camera_world_to_screen(
	const struct render_camera *camera,
	const struct render_frustum *frustum,
	const real_point3d *world_point,
	real_point2d *screen_point);
extern boolean rasterizer_bitmap_new(struct bitmap_data *bitmap);
extern void halo_menus_art_register(void const *texture, char const *png);
extern long hud_hires_asset_count(void);
extern char const *hud_hires_asset_tag(long asset);
extern void hud_hires_register_placeholder(long asset, unsigned long const *texture);
extern int config_boolean(char const *name);
extern char const *config_string(char const *name);
extern short controller_glyph_style(void);

struct controller_glyph_asset
{
	char const *tag;
	short bounds[4];
};
#include "controller_glyph_assets.h" 

typedef uint32_t forge_sdl_joystick_id;
typedef struct SDL_Gamepad forge_sdl_gamepad;
extern forge_sdl_joystick_id *SDL_GetGamepads(int *count);
extern forge_sdl_gamepad *SDL_GetGamepadFromID(forge_sdl_joystick_id instance_id);
extern _Bool SDL_GetGamepadButton(forge_sdl_gamepad *gamepad, int button);
extern int16_t SDL_GetGamepadAxis(forge_sdl_gamepad *gamepad, int axis);
extern void SDL_free(void *mem);
extern unsigned long tag_get_group_tag(long tag_index);


struct forge_font_char_metric
{
	word character;
	short character_width;
	short bitmap_width;
	short bitmap_height;
	short bitmap_origin_x;
	short bitmap_origin_y;
	short hardware_character_index;
	word pad;
	long pixels_offset;
};

static int s_controls_loaded = 0;
static int s_gp_btn_down[17];
static float s_gp_lx = 0.0f, s_gp_ly = 0.0f, s_gp_rx = 0.0f, s_gp_ry = 0.0f, s_gp_lt = 0.0f, s_gp_rt = 0.0f;

#define FORGE_CONTROLS_SCHEMA_VERSION 2

void forge_controls_init_defaults(void)
{
	g_forge.bind_vk[FORGE_BIND_TOGGLE_MODE]    = 0x70; /* VK_F1 */
	g_forge.bind_vk[FORGE_BIND_PALETTE]        = 0x71; /* VK_F2 */
	g_forge.bind_vk[FORGE_BIND_THIRD_PERSON]   = 0x73; /* VK_F4 */
	g_forge.bind_vk[FORGE_BIND_QUICK_SAVE]     = 0x74; /* VK_F5 */
	g_forge.bind_vk[FORGE_BIND_ROTATE_HOLD]    = 0xA4; /* VK_LMENU (Left Alt) */
	g_forge.bind_vk[FORGE_BIND_SNAP_FLOOR]     = 'Z';
	g_forge.bind_vk[FORGE_BIND_SNAP_SURFACE]   = 'F';
	g_forge.bind_vk[FORGE_BIND_ROT_SNAP]       = 'G';
	g_forge.bind_vk[FORGE_BIND_OBJECT_OPTIONS] = 'X';
	g_forge.bind_vk[FORGE_BIND_DELETE_OBJECT]  = 0x2E; /* VK_DELETE */
	g_forge.bind_vk[FORGE_BIND_COPY_OBJECT]    = 'C';

	/* Default Controller Layout:
	 * Grab/Drop Item: A
	 * Hold to Drop Item: B
	 * Inventory/Item Options: X
	 * Delete Item: Y
	 * Move/Rotate Selected Item: Left Stick
	 * Look/Rotate Selected Item: Right Stick
	 * Rotate Selected Item: Hold Right Trigger
	 * Switch Play/Edit Mode: D-Pad Up
	 * Move Up: Right Bumper (FORGE_GP_BLACK)
	 * Move Down: Left Bumper (FORGE_GP_WHITE) */
	g_forge.bind_gp[FORGE_BIND_TOGGLE_MODE]    = FORGE_GP_DPAD_UP;
	g_forge.bind_gp[FORGE_BIND_PALETTE]        = FORGE_GP_X;
	g_forge.bind_gp[FORGE_BIND_THIRD_PERSON]   = FORGE_GP_BACK;
	g_forge.bind_gp[FORGE_BIND_QUICK_SAVE]     = FORGE_GP_DPAD_DOWN;
	g_forge.bind_gp[FORGE_BIND_ROTATE_HOLD]    = FORGE_GP_RTRIGGER;
	g_forge.bind_gp[FORGE_BIND_SNAP_FLOOR]     = FORGE_GP_LTRIGGER;
	g_forge.bind_gp[FORGE_BIND_SNAP_SURFACE]   = FORGE_GP_RTHUMB;
	g_forge.bind_gp[FORGE_BIND_ROT_SNAP]       = FORGE_GP_DPAD_LEFT;
	g_forge.bind_gp[FORGE_BIND_OBJECT_OPTIONS] = FORGE_GP_X;
	g_forge.bind_gp[FORGE_BIND_DELETE_OBJECT]  = FORGE_GP_Y;
	g_forge.bind_gp[FORGE_BIND_COPY_OBJECT]    = FORGE_GP_DPAD_RIGHT;

	g_forge.controller_look_sens = 1.0f;
	g_forge.controller_fly_speed = 1.0f;
}

void forge_controls_load(void)
{
	FILE *fp;
	char line[256];
	int schema_ver = 1;
	int loaded_gp[FORGE_BIND_COUNT];
	int i;

	forge_controls_init_defaults();
	for (i = 0; i < FORGE_BIND_COUNT; i++)
		loaded_gp[i] = g_forge.bind_gp[i];
	s_controls_loaded = 1;

	fp = fopen("OpenCE Forge/forge_controls.ini", "r");
	if (!fp)
		return;
	while (fgets(line, sizeof(line), fp))
	{
		int idx = -1, vk = 0, gp = 0, sver = 0;
		float fval = 1.0f;
		if (sscanf(line, "controls_schema=%d", &sver) == 1)
		{
			schema_ver = sver;
		}
		else if (sscanf(line, "bind_%d=%d,%d", &idx, &vk, &gp) == 3)
		{
			if (idx >= 0 && idx < FORGE_BIND_COUNT)
			{
				if (idx == FORGE_BIND_ROTATE_HOLD && vk == 'R')
					vk = 0xA4; /* Upgrade legacy 'R' default to Left Alt (VK_LMENU) */
				g_forge.bind_vk[idx] = vk;
				loaded_gp[idx] = gp;
			}
		}
		else if (sscanf(line, "controller_look_sens=%f", &fval) == 1)
		{
			if (fval >= 0.2f && fval <= 4.0f)
				g_forge.controller_look_sens = fval;
		}
		else if (sscanf(line, "controller_fly_speed=%f", &fval) == 1)
		{
			if (fval >= 0.2f && fval <= 4.0f)
				g_forge.controller_fly_speed = fval;
		}
	}
	fclose(fp);

	if (schema_ver >= FORGE_CONTROLS_SCHEMA_VERSION)
	{
		for (i = 0; i < FORGE_BIND_COUNT; i++)
			g_forge.bind_gp[i] = loaded_gp[i];
	}
	else
	{
		/* Upgrade legacy v1 controller bindings in forge_controls.ini to the new v2 default controller layout */
		forge_controls_save();
	}
}

void forge_controls_save(void)
{
	FILE *fp = fopen("OpenCE Forge/forge_controls.ini", "w");
	int i;
	if (!fp)
		return;
	fprintf(fp, "controls_schema=%d\n", FORGE_CONTROLS_SCHEMA_VERSION);
	for (i = 0; i < FORGE_BIND_COUNT; i++)
		fprintf(fp, "bind_%d=%d,%d\n", i, g_forge.bind_vk[i], g_forge.bind_gp[i]);
	fprintf(fp, "controller_look_sens=%.2f\n", g_forge.controller_look_sens);
	fprintf(fp, "controller_fly_speed=%.2f\n", g_forge.controller_fly_speed);
	fclose(fp);
}

char const *forge_vk_display_name(int vk)
{
	static char s_buf[24];
	if (vk <= 0) return "-";
	if (vk >= 'A' && vk <= 'Z') { s_buf[0] = (char)vk; s_buf[1] = 0; return s_buf; }
	if (vk >= '0' && vk <= '9') { s_buf[0] = (char)vk; s_buf[1] = 0; return s_buf; }
	if (vk >= 0x70 && vk <= 0x7B) { snprintf(s_buf, sizeof(s_buf), "F%d", vk - 0x70 + 1); return s_buf; }
	switch (vk)
	{
	case 0x01: return "LMB";
	case 0x02: return "RMB";
	case 0x04: return "MMB";
	case 0x08: return "BKSP";
	case 0x09: return "TAB";
	case 0x0D: return "ENTER";
	case 0x10: case 0xA0: case 0xA1: return "SHIFT";
	case 0x11: case 0xA2: case 0xA3: return "CTRL";
	case 0xA4: return "L-ALT";
	case 0xA5: return "R-ALT";
	case 0x12: return "ALT";
	case 0x1B: return "ESC";
	case 0x20: return "SPACE";
	case 0x21: return "PGUP";
	case 0x22: return "PGDN";
	case 0x23: return "END";
	case 0x24: return "HOME";
	case 0x25: return "LEFT";
	case 0x26: return "UP";
	case 0x27: return "RIGHT";
	case 0x28: return "DOWN";
	case 0x2D: return "INS";
	case 0x2E: return "DEL";
	case 0xBD: return "-";
	case 0xBB: return "=";
	case 0xDB: return "[";
	case 0xDD: return "]";
	default:
		snprintf(s_buf, sizeof(s_buf), "KEY %02X", vk & 0xFF);
		return s_buf;
	}
}

char const *forge_gp_display_name(int gp)
{
	switch (gp)
	{
	case FORGE_GP_A:          return "A BUTTON";
	case FORGE_GP_B:          return "B BUTTON";
	case FORGE_GP_X:          return "X BUTTON";
	case FORGE_GP_Y:          return "Y BUTTON";
	case FORGE_GP_BLACK:      return "R BUMPER / RB";
	case FORGE_GP_WHITE:      return "L BUMPER / LB";
	case FORGE_GP_LTRIGGER:   return "L TRIGGER";
	case FORGE_GP_RTRIGGER:   return "R TRIGGER";
	case FORGE_GP_DPAD_UP:    return "D-PAD UP";
	case FORGE_GP_DPAD_DOWN:  return "D-PAD DOWN";
	case FORGE_GP_DPAD_LEFT:  return "D-PAD LEFT";
	case FORGE_GP_DPAD_RIGHT: return "D-PAD RIGHT";
	case FORGE_GP_START:      return "START";
	case FORGE_GP_BACK:       return "BACK";
	case FORGE_GP_LTHUMB:     return "L THUMB";
	case FORGE_GP_RTHUMB:     return "R THUMB";
	default:                  return "-";
	}
}

static char const *forge_gp_icon_token(int gp)
{
	switch (gp)
	{
	case FORGE_GP_A:          return "{A}";
	case FORGE_GP_B:          return "{B}";
	case FORGE_GP_X:          return "{X}";
	case FORGE_GP_Y:          return "{Y}";
	case FORGE_GP_BLACK:      return "{BL}";
	case FORGE_GP_WHITE:      return "{WH}";
	case FORGE_GP_LTRIGGER:   return "{LT}";
	case FORGE_GP_RTRIGGER:   return "{RT}";
	case FORGE_GP_DPAD_UP:    return "{DU}";
	case FORGE_GP_DPAD_DOWN:  return "{DD}";
	case FORGE_GP_DPAD_LEFT:  return "{DL}";
	case FORGE_GP_DPAD_RIGHT: return "{DR}";
	case FORGE_GP_START:      return "{ST}";
	case FORGE_GP_BACK:       return "{BK}";
	case FORGE_GP_LTHUMB:     return "{LS}";
	case FORGE_GP_RTHUMB:     return "{RS}";
	default:                  return "[-]";
	}
}

static char const *forge_action_prompt(int action)
{
	static char s_pbuf[16][24];
	static int s_pidx = 0;
	char *dst;

	if (!s_controls_loaded)
		forge_controls_load();
	if (action < 0 || action >= FORGE_BIND_COUNT)
		return "";
	if (g_forge.using_controller && g_forge.bind_gp[action] > 0)
		return forge_gp_icon_token(g_forge.bind_gp[action]);

	dst = s_pbuf[s_pidx++ & 15];
	snprintf(dst, 24, "[%s]", forge_vk_display_name(g_forge.bind_vk[action]));
	return dst;
}

void forge_poll_controller_state(float *out_lx, float *out_ly, float *out_rx, float *out_ry, float *out_lt, float *out_rt)
{
	int count = 0;
	forge_sdl_joystick_id *ids;
	forge_sdl_gamepad *pad = NULL;
	int any_gp_activity = 0;
	int i;

	if (!s_controls_loaded)
		forge_controls_load();

	memset(s_gp_btn_down, 0, sizeof(s_gp_btn_down));
	s_gp_lx = s_gp_ly = s_gp_rx = s_gp_ry = s_gp_lt = s_gp_rt = 0.0f;

	if (forge_window_is_focused())
	{
		ids = SDL_GetGamepads(&count);
		if (ids)
		{
			if (count > 0)
				pad = SDL_GetGamepadFromID(ids[0]);
			SDL_free(ids);
		}
	}

	if (pad)
	{
		int16_t raw_lx = SDL_GetGamepadAxis(pad, 0);
		int16_t raw_ly = SDL_GetGamepadAxis(pad, 1);
		int16_t raw_rx = SDL_GetGamepadAxis(pad, 2);
		int16_t raw_ry = SDL_GetGamepadAxis(pad, 3);
		int16_t raw_lt = SDL_GetGamepadAxis(pad, 4);
		int16_t raw_rt = SDL_GetGamepadAxis(pad, 5);
		int const deadzone = 6200;

		s_gp_btn_down[FORGE_GP_A]          = SDL_GetGamepadButton(pad, 0) != 0;
		s_gp_btn_down[FORGE_GP_B]          = SDL_GetGamepadButton(pad, 1) != 0;
		s_gp_btn_down[FORGE_GP_X]          = SDL_GetGamepadButton(pad, 2) != 0;
		s_gp_btn_down[FORGE_GP_Y]          = SDL_GetGamepadButton(pad, 3) != 0;
		s_gp_btn_down[FORGE_GP_BACK]       = SDL_GetGamepadButton(pad, 4) != 0;
		s_gp_btn_down[FORGE_GP_START]      = SDL_GetGamepadButton(pad, 6) != 0;
		s_gp_btn_down[FORGE_GP_LTHUMB]     = SDL_GetGamepadButton(pad, 7) != 0;
		s_gp_btn_down[FORGE_GP_RTHUMB]     = SDL_GetGamepadButton(pad, 8) != 0;
		s_gp_btn_down[FORGE_GP_WHITE]      = SDL_GetGamepadButton(pad, 9) != 0;
		s_gp_btn_down[FORGE_GP_BLACK]      = SDL_GetGamepadButton(pad, 10) != 0;
		s_gp_btn_down[FORGE_GP_DPAD_UP]    = SDL_GetGamepadButton(pad, 11) != 0;
		s_gp_btn_down[FORGE_GP_DPAD_DOWN]  = SDL_GetGamepadButton(pad, 12) != 0;
		s_gp_btn_down[FORGE_GP_DPAD_LEFT]  = SDL_GetGamepadButton(pad, 13) != 0;
		s_gp_btn_down[FORGE_GP_DPAD_RIGHT] = SDL_GetGamepadButton(pad, 14) != 0;

		if (raw_lt > 8000) s_gp_btn_down[FORGE_GP_LTRIGGER] = 1;
		if (raw_rt > 8000) s_gp_btn_down[FORGE_GP_RTRIGGER] = 1;

		if (raw_lt > 2000) s_gp_lt = (float)(raw_lt - 2000) / (float)(32767 - 2000);
		if (raw_rt > 2000) s_gp_rt = (float)(raw_rt - 2000) / (float)(32767 - 2000);

		if (abs((int)raw_lx) > deadzone)
			s_gp_lx = (float)(raw_lx > 0 ? (raw_lx - deadzone) : (raw_lx + deadzone)) / (float)(32767 - deadzone);
		if (abs((int)raw_ly) > deadzone)
			s_gp_ly = -(float)(raw_ly > 0 ? (raw_ly - deadzone) : (raw_ly + deadzone)) / (float)(32767 - deadzone);
		if (abs((int)raw_rx) > deadzone)
			s_gp_rx = (float)(raw_rx > 0 ? (raw_rx - deadzone) : (raw_rx + deadzone)) / (float)(32767 - deadzone);
		if (abs((int)raw_ry) > deadzone)
			s_gp_ry = -(float)(raw_ry > 0 ? (raw_ry - deadzone) : (raw_ry + deadzone)) / (float)(32767 - deadzone);

		for (i = 1; i <= 16; i++)
		{
			if (s_gp_btn_down[i])
				any_gp_activity = 1;
		}
		if ((float)fabs((double)s_gp_lx) > 0.12f || (float)fabs((double)s_gp_ly) > 0.12f ||
			(float)fabs((double)s_gp_rx) > 0.12f || (float)fabs((double)s_gp_ry) > 0.12f ||
			s_gp_lt > 0.15f || s_gp_rt > 0.15f)
		{
			any_gp_activity = 1;
		}
	}

	if (any_gp_activity)
	{
		g_forge.using_controller = 1;
	}
	else if (forge_window_is_focused())
	{
		static int const kbm_watch[] = {
			'W', 'A', 'S', 'D', 'Q', 'E', 'R', 'F', 'G', 'Z', 'X', 'C',
			0x01, 0x02, 0x0D, 0x12, 0x1B, 0x20, 0x25, 0x26, 0x27, 0x28, 0x70, 0x71, 0x73, 0x74, 0xA4
		};
		for (i = 0; i < (int)(sizeof(kbm_watch) / sizeof(kbm_watch[0])); i++)
		{
			if (GetAsyncKeyState(kbm_watch[i]) & 0x8000)
			{
				g_forge.using_controller = 0;
				break;
			}
		}
	}

	if (out_lx) *out_lx = s_gp_lx;
	if (out_ly) *out_ly = s_gp_ly;
	if (out_rx) *out_rx = s_gp_rx;
	if (out_ry) *out_ry = s_gp_ry;
	if (out_lt) *out_lt = s_gp_lt;
	if (out_rt) *out_rt = s_gp_rt;
}

int forge_gp_button_down(int gp)
{
	if (gp <= 0 || gp > 16)
		return 0;
	return s_gp_btn_down[gp];
}

static int forge_action_down(int action)
{
	int vk, gp;
	if (!s_controls_loaded)
		forge_controls_load();
	if (action < 0 || action >= FORGE_BIND_COUNT)
		return 0;
	vk = g_forge.bind_vk[action];
	gp = g_forge.bind_gp[action];
	if (action == FORGE_BIND_ROTATE_HOLD)
	{
		if ((GetAsyncKeyState(0xA4) & 0x8000) ||
			(GetAsyncKeyState(0x12) & 0x8000) ||
			(GetAsyncKeyState('R') & 0x8000))
		{
			return 1;
		}
	}
	if (vk > 0 && (GetAsyncKeyState(vk) & 0x8000))
		return 1;
	if (gp > 0 && forge_gp_button_down(gp))
		return 1;
	return 0;
}

int forge_get_local_player_abs_index(void)
{
	long unit_idx = player_control_get_unit_index(0);
	if (unit_idx != NONE)
	{
		struct unit_datum *u = (struct unit_datum *)object_try_and_get_and_verify_type(unit_idx, _object_mask_unit);
		if (u && u->unit.player_index != NONE)
			return (int)((uint32_t)u->unit.player_index & 0xFFFFu);
	}
	if (player_data)
	{
		long p_idx;
		for (p_idx = data_next_index(player_data, NONE); p_idx != NONE; p_idx = data_next_index(player_data, p_idx))
		{
			struct player_datum *pl = player_get(p_idx);
			if (pl && pl->local_player_index == 0)
				return (int)((uint32_t)p_idx & 0xFFFFu);
		}
	}
	return 0;
}

int forge_is_unit_in_monitor_mode(long unit_index)
{
	struct unit_datum *u;
	int p_abs;

	if (!g_forge_session_enabled || !g_forge.initialized || unit_index == NONE)
		return 0;
	u = (struct unit_datum *)object_try_and_get_and_verify_type(unit_index, _object_mask_biped);
	if (!u || u->object.type != _object_type_biped || u->unit.player_index == NONE)
		return 0;
	if (unit_index == player_control_get_unit_index(0))
		return (g_forge.mode == _forge_mode_3d_monitor) ? 1 : 0;
	p_abs = (int)((uint32_t)u->unit.player_index & 0xFFFFu);
	if (p_abs >= 0 && p_abs < 128)
		return g_forge.player_is_monitor[p_abs] ? 1 : 0;
	return 0;
}

static void forge_hide_spartan_linked_attachments_and_children(long unit_idx, int hide_linked)
{
	struct unit_datum *u;
	struct object_definition *def;
	long child_idx;
	short a;

	u = (struct unit_datum *)object_try_and_get_and_verify_type(unit_idx, _object_mask_unit);
	if (!u)
		return;

	/* Never set u->unit.active_camouflage = 1.0f during simulation ticks!
	 * Setting active_camouflage > 0.0f during simulation causes network_distributed.c on the host
	 * to broadcast _distributed_unit_camouflaged_bit to all clients. Instead, active_camouflage
	 * is applied only around the local render pass in forge_render_begin/end_monitor_visibility(). */
	if (!hide_linked)
	{
		u->unit.active_camouflage = 0.0f;
		SET_FLAG(u->unit.flags, _unit_active_camouflaged_bit, FALSE);
	}

	if (hide_linked && u->definition_index != NONE)
	{
		def = object_definition_get(u->definition_index);
		if (def)
		{
			for (a = 0; a < def->object.attachments.count; a++)
			{
				struct object_attachment_definition *att = TAG_BLOCK_GET_ELEMENT(
					&def->object.attachments, a, struct object_attachment_definition);
				if (att && att->type.group_tag == 'ligh' && att->type.index != NONE)
				{
					/* Clear _light_definition_dont_fade_active_camouflage_bit (bit 5) so helmet lights fade out completely */
					uint32_t *lflags = (uint32_t *)tag_get('ligh', att->type.index);
					if (lflags)
						*lflags &= ~(1u << 5);
				}
			}
		}
	}

	for (child_idx = u->object.first_child_object_index; child_idx != NONE; )
	{
		struct object_datum *c_obj = (struct object_datum *)object_try_and_get_and_verify_type(child_idx, (unsigned long)-1);
		if (!c_obj)
			break;
		SET_FLAG(c_obj->object.flags, _object_invisible_bit, hide_linked ? TRUE : FALSE);
		child_idx = c_obj->object.next_object_index;
	}
}

static float s_saved_render_camo[128];
static uint8_t s_saved_render_camo_valid[128];

void forge_render_begin_monitor_visibility(void)
{
	long p_idx;
	if (!g_forge_session_enabled || !g_forge.initialized || !player_data)
		return;
	for (p_idx = data_next_index(player_data, NONE); p_idx != NONE; p_idx = data_next_index(player_data, p_idx))
	{
		struct player_datum *pl = player_get(p_idx);
		int p_abs = (int)((uint32_t)p_idx & 0xFFFFu);
		if (!pl || pl->unit_index == NONE || p_abs < 0 || p_abs >= 128)
			continue;
		if (forge_is_unit_in_monitor_mode(pl->unit_index))
		{
			struct unit_datum *u = (struct unit_datum *)object_try_and_get_and_verify_type(pl->unit_index, _object_mask_unit);
			forge_hide_spartan_linked_attachments_and_children(pl->unit_index, 1);
			if (u)
			{
				s_saved_render_camo[p_abs] = u->unit.active_camouflage;
				s_saved_render_camo_valid[p_abs] = 1;
				u->unit.active_camouflage = 1.0f;
			}
		}
		else
		{
			s_saved_render_camo_valid[p_abs] = 0;
			forge_hide_spartan_linked_attachments_and_children(pl->unit_index, 0);
		}
	}
}

void forge_render_end_monitor_visibility(void)
{
	long p_idx;
	if (!g_forge_session_enabled || !g_forge.initialized || !player_data)
		return;
	for (p_idx = data_next_index(player_data, NONE); p_idx != NONE; p_idx = data_next_index(player_data, p_idx))
	{
		struct player_datum *pl = player_get(p_idx);
		int p_abs = (int)((uint32_t)p_idx & 0xFFFFu);
		if (!pl || pl->unit_index == NONE || p_abs < 0 || p_abs >= 128)
			continue;
		if (s_saved_render_camo_valid[p_abs])
		{
			struct unit_datum *u = (struct unit_datum *)object_try_and_get_and_verify_type(pl->unit_index, _object_mask_unit);
			if (u)
			{
				u->unit.active_camouflage = s_saved_render_camo[p_abs];
				SET_FLAG(u->unit.flags, _unit_active_camouflaged_bit, FALSE);
			}
			s_saved_render_camo_valid[p_abs] = 0;
		}
	}
}

#define FORGE_IN_MATCH_HUD_SCALE 0.75f
#define FORGE_COUNTER_HUD_SCALE (FORGE_IN_MATCH_HUD_SCALE * 0.5f)

static float s_hud_ui_scale = 1.0f;
static float s_hud_ui_origin_x = 0.0f;
static float s_hud_ui_origin_y = 0.0f;

static void set_forge_hud_ui_scale(float scale, float origin_x, float origin_y)
{
	s_hud_ui_scale = (scale > 0.05f) ? scale : 1.0f;
	s_hud_ui_origin_x = origin_x;
	s_hud_ui_origin_y = origin_y;
}

static void draw_quad(rectangle2d const *r, uint32_t argb)
{
	short x0, y0, x1, y1;
	if (!r || r->x1 <= r->x0 || r->y1 <= r->y0 || ((argb >> 24) & 0xFFu) == 0)
		return;
	if (s_hud_ui_scale != 1.0f)
	{
		x0 = (short)(s_hud_ui_origin_x + ((float)r->x0 - s_hud_ui_origin_x) * s_hud_ui_scale + 0.5f);
		y0 = (short)(s_hud_ui_origin_y + ((float)r->y0 - s_hud_ui_origin_y) * s_hud_ui_scale + 0.5f);
		x1 = (short)(s_hud_ui_origin_x + ((float)r->x1 - s_hud_ui_origin_x) * s_hud_ui_scale + 0.5f);
		y1 = (short)(s_hud_ui_origin_y + ((float)r->y1 - s_hud_ui_origin_y) * s_hud_ui_scale + 0.5f);
		if (x1 <= x0) x1 = (short)(x0 + 1);
		if (y1 <= y0) y1 = (short)(y0 + 1);
	}
	else
	{
		x0 = r->x0;
		y0 = r->y0;
		x1 = r->x1;
		y1 = r->y1;
	}
	forge_draw_screen_rect(x0, y0, x1, y1, argb);
}

static void draw_box_border(short x0, short y0, short x1, short y1, short thick, uint32_t argb)
{
	rectangle2d r;
	if (((argb >> 24) & 0xFFu) == 0 || thick <= 0)
		return;
	r.x0 = x0; r.y0 = y0; r.x1 = x1; r.y1 = (short)(y0 + thick); draw_quad(&r, argb);
	r.x0 = x0; r.y0 = (short)(y1 - thick); r.x1 = x1; r.y1 = y1; draw_quad(&r, argb);
	r.x0 = x0; r.y0 = y0; r.x1 = (short)(x0 + thick); r.y1 = y1; draw_quad(&r, argb);
	r.x0 = (short)(x1 - thick); r.y0 = y0; r.x1 = x1; r.y1 = y1; draw_quad(&r, argb);
}

static long get_forge_ui_font(int large, long fallback_font)
{
	long f = tag_loaded('font', large ? "ui\\large_ui" : "ui\\small_ui");
	if (f == NONE)
		f = tag_loaded('font', large ? "ui\\small_ui" : "ui\\large_ui");
	if (f == NONE)
		f = interface_get_tag_index(_interface_font_system);
	return (f != NONE) ? f : fallback_font;
}
#define get_small_forge_font(fb) get_forge_ui_font(0, (fb))
#define get_large_forge_font(fb) get_forge_ui_font(1, (fb))

/* ============================================================================
 * Editable Forge UI Theme & PNG Background System (OpenCE Forge/ui/forge_ui.xml)
 * ============================================================================ */

enum forge_ui_area_id
{
	_forge_ui_area_spawn_menu = 0,
	_forge_ui_area_preview_panel,
	_forge_ui_area_properties_menu,
	_forge_ui_area_top_bar,
	_forge_ui_area_bottom_bar,
	_forge_ui_area_save_modal,
	_forge_ui_area_maps_modal,
	_forge_ui_area_toast_bar,
	_forge_ui_area_markers,
	NUMBER_OF_FORGE_UI_AREAS
};

struct forge_ui_area_style
{
	char font_name[64];
	short font_style; /* -1=plain, 0=bold, 1=italic, 2=condense, 3=underline */
	float font_scale;
	uint32_t title_color;
	uint32_t text_color;
	uint32_t selected_color;
	uint32_t bg_color;
	uint32_t header_color;
	uint32_t highlight_color;
	uint32_t border_color;
	uint32_t image_tint;
	char bg_image[64];
	int use_image;
	struct bitmap_data bitmap;
	uint8_t dummy_pixels[16];
	int bitmap_initialized;
	int bitmap_valid;
};

static struct
{
	int initialized;
	int reload_rev;
	char registered_map[128];
	struct forge_ui_area_style areas[NUMBER_OF_FORGE_UI_AREAS];
} s_ui_theme;

static uint32_t parse_hex_color_attr(char const *str, uint32_t fallback)
{
	unsigned long val = 0;
	char const *p = str;
	size_t len;

	if (!p || !p[0])
		return fallback;
	if (*p == '#')
		p++;
	else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
		p += 2;
	len = strlen(p);
	if (len != 6 && len != 8)
		return fallback;
	val = strtoul(p, NULL, 16);
	if (len == 6)
		val |= 0xFF000000ul;
	return (uint32_t)val;
}

static short parse_font_style_attr(char const *str)
{
	if (!str || !str[0])
		return NONE;
	if (_stricmp(str, "bold") == 0)
		return 0; /* _text_style_bold */
	if (_stricmp(str, "italic") == 0)
		return 1; /* _text_style_italic */
	if (_stricmp(str, "condense") == 0 || _stricmp(str, "condensed") == 0)
		return 2; /* _text_style_condense */
	if (_stricmp(str, "underline") == 0)
		return 3; /* _text_style_underline */
	return NONE; /* plain */
}

static int extract_xml_attr(char const *tag_start, char const *tag_end, char const *attr_name, char *out_val, size_t out_sz)
{
	size_t nlen = strlen(attr_name);
	char const *p = tag_start;

	while (p && p + nlen + 2 < tag_end)
	{
		if (_strnicmp(p, attr_name, nlen) == 0 &&
			(p == tag_start || p[-1] == ' ' || p[-1] == '\t' || p[-1] == '\r' || p[-1] == '\n'))
		{
			char const *eq = p + nlen;
			while (eq < tag_end && (*eq == ' ' || *eq == '\t'))
				eq++;
			if (eq < tag_end && *eq == '=')
			{
				char quote;
				char const *val_start;
				char const *val_end;
				size_t copy_len;
				eq++;
				while (eq < tag_end && (*eq == ' ' || *eq == '\t'))
					eq++;
				if (eq < tag_end && (*eq == '"' || *eq == '\''))
				{
					quote = *eq++;
					val_start = eq;
					val_end = memchr(val_start, quote, (size_t)(tag_end - val_start));
					if (val_end)
					{
						copy_len = (size_t)(val_end - val_start);
						if (copy_len >= out_sz)
							copy_len = out_sz - 1;
						memcpy(out_val, val_start, copy_len);
						out_val[copy_len] = 0;
						return 1;
					}
				}
			}
		}
		p++;
	}
	return 0;
}

static void init_default_area_style(struct forge_ui_area_style *st, char const *default_png)
{
	snprintf(st->font_name, sizeof(st->font_name), "large");
	st->font_style = NONE;
	st->font_scale = 1.0f;
	st->title_color = 0xFFFFFFFFu;
	st->text_color = 0xFF2896FFu;
	st->selected_color = 0xFFFFFFFFu;
	st->bg_color = 0xC00A2649u;
	st->header_color = 0x00000000u;
	st->highlight_color = 0xD8225C9Cu;
	st->border_color = 0xFF2896FFu;
	st->image_tint = 0xFFFFFFFFu;
	snprintf(st->bg_image, sizeof(st->bg_image), "%s", default_png ? default_png : "none");
	st->use_image = (default_png && default_png[0] && _stricmp(default_png, "none") != 0) ? 1 : 0;
}

static void sync_and_register_area_png(struct forge_ui_area_style *st, int area_idx)
{
	char src_path[256];
	char rel_png[128];
	FILE *in_f;

	(void)area_idx;
	st->bitmap_valid = 0;
	if (!st->use_image || !st->bg_image[0] || _stricmp(st->bg_image, "none") == 0)
		return;

	snprintf(src_path, sizeof(src_path), "OpenCE Forge/ui/%s", st->bg_image);
	in_f = fopen(src_path, "rb");
	if (!in_f)
		return;
	fclose(in_f);

	snprintf(rel_png, sizeof(rel_png), "ui/%s", st->bg_image);

	if (!st->bitmap_initialized)
	{
		memset(&st->bitmap, 0, sizeof(st->bitmap));
		memset(st->dummy_pixels, 0xFF, sizeof(st->dummy_pixels));
		st->bitmap.signature = 'bitm';
		st->bitmap.width = 2;
		st->bitmap.height = 2;
		st->bitmap.depth = 1;
		st->bitmap.type = 0;   /* _bitmap_type_2d_texture */
		st->bitmap.format = 11; /* _bitmap_format_a8r8g8b8 */
		st->bitmap.flags = 1;  /* _bitmap_has_power_of_two_dimensions_bit */
		st->bitmap.base_address = st->dummy_pixels;
		if (rasterizer_bitmap_new(&st->bitmap) && st->bitmap.hardware_format)
			st->bitmap_initialized = 1;
	}

	if (st->bitmap_initialized && st->bitmap.hardware_format)
	{
		halo_menus_art_register(st->bitmap.hardware_format, rel_png);
		st->bitmap_valid = 1;
	}
}

static void parse_area_xml_node(char const *xml_text, char const *node_name, struct forge_ui_area_style *st)
{
	char open_pat[80];
	char const *p;
	char const *end;
	char val[128];

	snprintf(open_pat, sizeof(open_pat), "<%s", node_name);
	p = strstr(xml_text, open_pat);
	if (!p)
		return;
	end = strchr(p, '>');
	if (!end)
		return;

	if (extract_xml_attr(p, end, "font", val, sizeof(val)))
		snprintf(st->font_name, sizeof(st->font_name), "%s", val);
	if (extract_xml_attr(p, end, "style", val, sizeof(val)))
		st->font_style = parse_font_style_attr(val);
	if (extract_xml_attr(p, end, "size", val, sizeof(val)) || extract_xml_attr(p, end, "scale", val, sizeof(val)))
	{
		float sc = (float)atof(val);
		if (sc >= 0.25f && sc <= 4.0f)
			st->font_scale = sc;
	}
	if (extract_xml_attr(p, end, "title_color", val, sizeof(val)))
		st->title_color = parse_hex_color_attr(val, st->title_color);
	if (extract_xml_attr(p, end, "text_color", val, sizeof(val)) || extract_xml_attr(p, end, "color", val, sizeof(val)))
		st->text_color = parse_hex_color_attr(val, st->text_color);
	if (extract_xml_attr(p, end, "selected_color", val, sizeof(val)))
		st->selected_color = parse_hex_color_attr(val, st->selected_color);
	if (extract_xml_attr(p, end, "bg_color", val, sizeof(val)))
		st->bg_color = parse_hex_color_attr(val, st->bg_color);
	if (extract_xml_attr(p, end, "header_color", val, sizeof(val)))
		st->header_color = parse_hex_color_attr(val, st->header_color);
	if (extract_xml_attr(p, end, "highlight_color", val, sizeof(val)))
		st->highlight_color = parse_hex_color_attr(val, st->highlight_color);
	if (extract_xml_attr(p, end, "border_color", val, sizeof(val)))
		st->border_color = parse_hex_color_attr(val, st->border_color);
	if (extract_xml_attr(p, end, "image_tint", val, sizeof(val)))
		st->image_tint = parse_hex_color_attr(val, st->image_tint);
	if (extract_xml_attr(p, end, "background", val, sizeof(val)))
		snprintf(st->bg_image, sizeof(st->bg_image), "%s", val);
	if (extract_xml_attr(p, end, "use_image", val, sizeof(val)))
		st->use_image = (_stricmp(val, "true") == 0 || strcmp(val, "1") == 0 || _stricmp(val, "yes") == 0) ? 1 : 0;
}

static void forge_ui_theme_load(int is_hot_reload)
{
	static char const *const area_nodes[NUMBER_OF_FORGE_UI_AREAS] = {
		"spawn_menu",
		"preview_panel",
		"properties_menu",
		"top_bar",
		"bottom_bar",
		"save_modal",
		"maps_modal",
		"toast_bar",
		"markers"
	};
	static char const *const area_default_pngs[NUMBER_OF_FORGE_UI_AREAS] = {
		"panel_spawn_menu.png",
		"panel_preview.png",
		"panel_modal.png",
		"panel_top_bar.png",
		"panel_bottom_bar.png",
		"escape_menu_background.png",
		"panel_modal.png",
		"panel_bottom_bar.png",
		"none"
	};
	FILE *f;
	int i;

	if (is_hot_reload)
		s_ui_theme.reload_rev++;

	for (i = 0; i < NUMBER_OF_FORGE_UI_AREAS; i++)
		init_default_area_style(&s_ui_theme.areas[i], area_default_pngs[i]);

	/* Specific default font/color tuning per area matching base game menu UI styling */
	snprintf(s_ui_theme.areas[_forge_ui_area_top_bar].font_name, sizeof(s_ui_theme.areas[_forge_ui_area_top_bar].font_name), "small");
	snprintf(s_ui_theme.areas[_forge_ui_area_bottom_bar].font_name, sizeof(s_ui_theme.areas[_forge_ui_area_bottom_bar].font_name), "small");
	snprintf(s_ui_theme.areas[_forge_ui_area_toast_bar].font_name, sizeof(s_ui_theme.areas[_forge_ui_area_toast_bar].font_name), "small");
	snprintf(s_ui_theme.areas[_forge_ui_area_markers].font_name, sizeof(s_ui_theme.areas[_forge_ui_area_markers].font_name), "small");
	s_ui_theme.areas[_forge_ui_area_toast_bar].text_color = 0xFFFFFFFFu;
	s_ui_theme.areas[_forge_ui_area_markers].title_color = 0xFFFFFFFFu;
	s_ui_theme.areas[_forge_ui_area_markers].text_color = 0xFF2896FFu;
	s_ui_theme.areas[_forge_ui_area_markers].selected_color = 0xFFFFFFFFu;

	f = fopen("OpenCE Forge/ui/forge_ui.xml", "rb");
	if (f)
	{
		long sz;
		if (fseek(f, 0, SEEK_END) == 0 && (sz = ftell(f)) > 0 && sz < 262144 && fseek(f, 0, SEEK_SET) == 0)
		{
			char *xml_buf = (char *)malloc((size_t)sz + 1);
			if (xml_buf)
			{
				size_t rd = fread(xml_buf, 1, (size_t)sz, f);
				xml_buf[rd] = 0;
				for (i = 0; i < NUMBER_OF_FORGE_UI_AREAS; i++)
					parse_area_xml_node(xml_buf, area_nodes[i], &s_ui_theme.areas[i]);
				free(xml_buf);
			}
		}
		fclose(f);
	}

	for (i = 0; i < NUMBER_OF_FORGE_UI_AREAS; i++)
		sync_and_register_area_png(&s_ui_theme.areas[i], i);

	snprintf(s_ui_theme.registered_map, sizeof(s_ui_theme.registered_map), "%s",
		forge_get_active_map_name() ? forge_get_active_map_name() : "");
	s_ui_theme.initialized = 1;
}

static void forge_ui_theme_ensure_loaded(void)
{
	char const *cur_map = forge_get_active_map_name();
	if (!s_ui_theme.initialized)
	{
		forge_ui_theme_load(0);
	}
	else if (cur_map && strcmp(s_ui_theme.registered_map, cur_map) != 0)
	{
		/* Re-register PNG textures after map transitions because menu_tags_unloaded() clears menu art */
		int i;
		for (i = 0; i < NUMBER_OF_FORGE_UI_AREAS; i++)
			sync_and_register_area_png(&s_ui_theme.areas[i], i);
		snprintf(s_ui_theme.registered_map, sizeof(s_ui_theme.registered_map), "%s", cur_map);
	}
}

static void draw_ui_png_quad_uv(
	struct bitmap_data *bitmap,
	short x0, short y0,
	short x1, short y1,
	float u0, float v0,
	float u1, float v1,
	uint32_t tint_argb)
{
	struct dynamic_screen_vertex vertices[4];
	struct rasterizer_dynamic_screen_geometry_parameters parameters;
	int v;

	if (!bitmap || !bitmap->hardware_format || x1 <= x0 || y1 <= y0)
		return;

	for (v = 0; v < 4; v++)
	{
		real vx = (v % 3) ? (real)x1 : (real)x0;
		real vy = (v > 1) ? (real)y1 : (real)y0;
		if (s_hud_ui_scale != 1.0f)
		{
			vx = s_hud_ui_origin_x + (vx - s_hud_ui_origin_x) * s_hud_ui_scale;
			vy = s_hud_ui_origin_y + (vy - s_hud_ui_origin_y) * s_hud_ui_scale;
		}
		vertices[v].position.x = vx;
		vertices[v].position.y = vy;
		vertices[v].texture_coordinates.x = (v % 3) ? u1 : u0;
		vertices[v].texture_coordinates.y = (v > 1) ? v1 : v0;
		vertices[v].color = tint_argb;
	}

	memset(&parameters, 0, sizeof(parameters));
	parameters.meter_parameters = NULL;
	parameters.map_offset[0] = NULL;
	parameters.map_wrapped[0] = FALSE;
	parameters.map_wrapped[1] = FALSE;
	parameters.map_wrapped[2] = FALSE;
	parameters.map_scale[0].i = 1.0f;
	parameters.map_scale[0].j = 1.0f;
	parameters.map_texture_scale[0].i = 1.0f;
	parameters.map_texture_scale[0].j = 1.0f;
	parameters.point_sampled = FALSE;
	parameters.framebuffer_blend_function = 0;
	parameters.map[0] = bitmap;
	parameters.map[1] = NULL;
	parameters.map[2] = NULL;

	rasterizer_psuedo_dynamic_screen_quad_draw(&parameters, vertices);
}

static void draw_ui_png_quad(struct bitmap_data *bitmap, short x0, short y0, short x1, short y1, uint32_t tint_argb)
{
	draw_ui_png_quad_uv(bitmap, x0, y0, x1, y1, 0.0f, 0.0f, 1.0f, 1.0f, tint_argb);
}

/* ============================================================================
 * Base Game Menu Bitmap & Font Rendering Helpers
 * Draws authentic Halo CE / OpenCE menu tags (pausebox, objectives, option_bkds,
 * list_item_bkd, sel_list_item_bkd, text_button_background, arrow_sm_*, blue)
 * and ui\large_ui / ui\small_ui fonts directly in-game.
 * ============================================================================ */

static int draw_ce_menu_bitmap_uv(
	char const *tag_name,
	short frame_idx,
	short x0, short y0,
	short x1, short y1,
	float u0, float v0,
	float u1, float v1,
	uint32_t tint_argb)
{
	long tag_idx;
	struct bitmap_data *bmp;

	if (!tag_name || !tag_name[0] || x1 <= x0 || y1 <= y0)
		return 0;

	tag_idx = tag_loaded('bitm', tag_name);
	if (tag_idx == NONE)
		return 0;

	bmp = bitmap_group_get_bitmap_from_sequence(tag_idx, 0, frame_idx);
	if (!bmp && frame_idx != 0)
		bmp = bitmap_group_get_bitmap_from_sequence(tag_idx, 0, 0);
	if (!bmp || !bmp->hardware_format || bmp->width <= 0 || bmp->height <= 0)
		return 0;

	draw_ui_png_quad_uv(bmp, x0, y0, x1, y1, u0, v0, u1, v1, tint_argb);
	return 1;
}

static int draw_ce_menu_bitmap_cropped(
	char const *tag_name,
	short frame_idx,
	short x0, short y0,
	short x1, short y1,
	short crop_w, short crop_h,
	uint32_t tint_argb)
{
	long tag_idx = (tag_name && tag_name[0]) ? tag_loaded('bitm', tag_name) : NONE;
	struct bitmap_data *bmp = (tag_idx != NONE) ? bitmap_group_get_bitmap_from_sequence(tag_idx, 0, frame_idx) : NULL;
	if (!bmp && tag_idx != NONE && frame_idx != 0)
		bmp = bitmap_group_get_bitmap_from_sequence(tag_idx, 0, 0);
	if (!bmp || !bmp->hardware_format || bmp->width <= 0 || bmp->height <= 0 || x1 <= x0 || y1 <= y0)
		return 0;
	draw_ui_png_quad_uv(bmp, x0, y0, x1, y1, 0.0f, 0.0f,
		(crop_w > 0 && crop_w < bmp->width) ? ((float)crop_w / (float)bmp->width) : 1.0f,
		(crop_h > 0 && crop_h < bmp->height) ? ((float)crop_h / (float)bmp->height) : 1.0f,
		tint_argb);
	return 1;
}

/* Draws a 3-slice horizontally rounded Halo CE option/row highlight bar
 * using pc\bitmaps\option_bkds_sm so both left and right ends have crisp rounded caps. */
static int draw_ce_rounded_row_bar(
	short frame_idx,
	short x0, short y0,
	short x1, short y1)
{
	short w = (short)(x1 - x0);
	short cap = 16;
	float v1 = 28.0f / 32.0f;
	float u_cap = 16.0f / 256.0f;

	if (w <= 0 || y1 <= y0)
		return 0;
	if (w <= cap * 2)
		return draw_ce_menu_bitmap_cropped("pc\\bitmaps\\option_bkds_sm", frame_idx, x0, y0, x1, y1, 256, 28, 0xFFFFFFFFu);

	if (draw_ce_menu_bitmap_uv("pc\\bitmaps\\option_bkds_sm", frame_idx, x0, y0, (short)(x0 + cap), y1, 0.0f, 0.0f, u_cap, v1, 0xFFFFFFFFu) &&
		draw_ce_menu_bitmap_uv("pc\\bitmaps\\option_bkds_sm", frame_idx, (short)(x0 + cap), y0, (short)(x1 - cap), y1, u_cap, 0.0f, 1.0f - u_cap, v1, 0xFFFFFFFFu) &&
		draw_ce_menu_bitmap_uv("pc\\bitmaps\\option_bkds_sm", frame_idx, (short)(x1 - cap), y0, x1, y1, 1.0f - u_cap, 0.0f, 1.0f, v1, 0xFFFFFFFFu))
	{
		return 1;
	}
	return 0;
}

/* Draws the authentic 9-slice Halo CE pause/objectives container box with 1:1 rounded corners */
static void draw_ce_pause_box(short x0, short y0, short width, short height, int with_header)
{
	short x1 = (short)(x0 + width);
	short y1 = (short)(y0 + height);
	char const *top_left   = with_header ? "pc\\pause\\objectives_left"   : "pc\\pause\\pausebox_left";
	char const *top_center = with_header ? "pc\\pause\\objectives_center" : "pc\\pause\\pausebox_center";
	char const *top_right  = with_header ? "pc\\pause\\objectives_right"  : "pc\\pause\\pausebox_right";
	char const *obj_left   = "pc\\pause\\objectives_left";
	char const *obj_center = "pc\\pause\\objectives_center";
	char const *obj_right  = "pc\\pause\\objectives_right";
	short frame_idx = 1;
	short top_h = with_header ? 30 : 14;
	short bot_h = 14;
	float v_top_end = (float)top_h / 256.0f;
	float v_body_start = 32.0f / 256.0f;
	float v_body_end = 211.0f / 256.0f;
	float v_bot_end = 225.0f / 256.0f;

	if (width <= 32 || height <= 28)
		return;

	/* Top slice (rounded top corners; includes 29px dark header bar when with_header != 0,
	 * or clean blue rounded top cap from pausebox when with_header == 0) */
	if (draw_ce_menu_bitmap_uv(top_left, frame_idx, x0, y0, (short)(x0 + 16), (short)(y0 + top_h), 0.0f, 0.0f, 1.0f, v_top_end, 0xFFFFFFFFu) &&
		draw_ce_menu_bitmap_uv(top_center, frame_idx, (short)(x0 + 16), y0, (short)(x1 - 16), (short)(y0 + top_h), 0.0f, 0.0f, 1.0f, v_top_end, 0xFFFFFFFFu) &&
		draw_ce_menu_bitmap_uv(top_right, frame_idx, (short)(x1 - 16), y0, x1, (short)(y0 + top_h), 0.0f, 0.0f, 1.0f, v_top_end, 0xFFFFFFFFu) &&
		/* Middle body slice (clean translucent blue gradient from objectives_left/center/right) */
		draw_ce_menu_bitmap_uv(obj_left, frame_idx, x0, (short)(y0 + top_h), (short)(x0 + 16), (short)(y1 - bot_h), 0.0f, v_body_start, 1.0f, v_body_end, 0xFFFFFFFFu) &&
		draw_ce_menu_bitmap_uv(obj_center, frame_idx, (short)(x0 + 16), (short)(y0 + top_h), (short)(x1 - 16), (short)(y1 - bot_h), 0.0f, v_body_start, 1.0f, v_body_end, 0xFFFFFFFFu) &&
		draw_ce_menu_bitmap_uv(obj_right, frame_idx, (short)(x1 - 16), (short)(y0 + top_h), x1, (short)(y1 - bot_h), 0.0f, v_body_start, 1.0f, v_body_end, 0xFFFFFFFFu) &&
		/* Bottom slice (1:1 unstretched rounded bottom corners from objectives_left/center/right, no footer line) */
		draw_ce_menu_bitmap_uv(obj_left, frame_idx, x0, (short)(y1 - bot_h), (short)(x0 + 16), y1, 0.0f, v_body_end, 1.0f, v_bot_end, 0xFFFFFFFFu) &&
		draw_ce_menu_bitmap_uv(obj_center, frame_idx, (short)(x0 + 16), (short)(y1 - bot_h), (short)(x1 - 16), y1, 0.0f, v_body_end, 1.0f, v_bot_end, 0xFFFFFFFFu) &&
		draw_ce_menu_bitmap_uv(obj_right, frame_idx, (short)(x1 - 16), (short)(y1 - bot_h), x1, y1, 0.0f, v_body_end, 1.0f, v_bot_end, 0xFFFFFFFFu))
	{
		return;
	}

	/* Fallback if menu tags are not present */
	{
		rectangle2d body, hdr;
		body.x0 = x0; body.y0 = y0; body.x1 = x1; body.y1 = y1;
		draw_quad(&body, 0xC00A2649u);
		if (with_header)
		{
			hdr.x0 = (short)(x0 + 2); hdr.y0 = (short)(y0 + 2); hdr.x1 = (short)(x1 - 2); hdr.y1 = (short)(y0 + 28);
			draw_quad(&hdr, 0xE0000000u);
			hdr.y0 = (short)(y0 + 28); hdr.y1 = (short)(y0 + 29);
			draw_quad(&hdr, 0xFF2896FFu);
		}
		draw_box_border(x0, y0, x1, y1, 2, 0xFF2896FFu);
	}
}

static void draw_ce_blue_line(short x0, short y0, short x1, short height)
{
	if (height <= 0)
		height = 1;
	if (!draw_ce_menu_bitmap_cropped("pc\\bitmaps\\blue", 0, x0, y0, x1, (short)(y0 + height), 4, 4, 0xFFFFFFFFu))
	{
		rectangle2d r;
		r.x0 = x0; r.y0 = y0; r.x1 = x1; r.y1 = (short)(y0 + height);
		draw_quad(&r, 0xFF2896FFu);
	}
}

static struct
{
	char tag_name[64];
	struct bitmap_data *bmp;
	long asset;
} s_xbox_btn_cache[24];

static struct bitmap_data *get_xbox_hires_button_bmp(int slot, char const *asset_tag_name, short tex_size)
{
	long a, count;
	(void)slot;
	if (!asset_tag_name || !asset_tag_name[0])
		return NULL;
	for (a = 0; a < 24; a++)
	{
		if (s_xbox_btn_cache[a].tag_name[0] && strcmp(s_xbox_btn_cache[a].tag_name, asset_tag_name) == 0)
		{
			if (s_xbox_btn_cache[a].bmp && s_xbox_btn_cache[a].asset >= 0)
			{
				hud_hires_register_placeholder(
					s_xbox_btn_cache[a].asset,
					(unsigned long const *)s_xbox_btn_cache[a].bmp->hardware_format);
				return s_xbox_btn_cache[a].bmp;
			}
			return NULL;
		}
		if (!s_xbox_btn_cache[a].tag_name[0])
			break;
	}
	if (a >= 24)
		return NULL;
	snprintf(s_xbox_btn_cache[a].tag_name, sizeof(s_xbox_btn_cache[a].tag_name), "%s", asset_tag_name);
	s_xbox_btn_cache[a].asset = -1;
	count = hud_hires_asset_count();
	{
		long k;
		for (k = 0; k < count; k++)
		{
			char const *t = hud_hires_asset_tag(k);
			if (t && strcmp(t, asset_tag_name) == 0)
			{
				struct bitmap_data *b = bitmap_2d_new(tex_size, tex_size, 0, 11);
				if (b && rasterizer_bitmap_new(b))
				{
					s_xbox_btn_cache[a].bmp = b;
					s_xbox_btn_cache[a].asset = k;
					hud_hires_register_placeholder(k, (unsigned long const *)b->hardware_format);
					return b;
				}
				if (b)
					bitmap_delete(b);
				break;
			}
		}
	}
	return NULL;
}

static short forge_controller_glyph_asset_index(short style, short slot)
{
	short family;
	if (style >= 0 && style < 3 && (slot == 4 || slot == 5) &&
		strcmp(config_string("display.hud_bumper_style"), "modern") == 0)
		return (short)(32 + slot - 4);
	if (style < 3 || style >= 11 || slot < 0 || slot >= 14)
		return NONE;
	if (slot < 4)
		return (short)((style - 3) * 4 + slot);
	family = (short)(style < 4 ? 0 : style < 8 ? 1 : 2);
	return (short)(32 + family * 10 + slot - 4);
}

static int parse_xbox_icon_token(char const *p, int *out_len, int *out_icon)
{
	static struct
	{
		char const *tok;
		int len;
		int icon;
	} const tbl[] = {
		{ "{A}",    3, FORGE_GP_A },
		{ "{B}",    3, FORGE_GP_B },
		{ "{X}",    3, FORGE_GP_X },
		{ "{Y}",    3, FORGE_GP_Y },
		{ "{BL}",   4, FORGE_GP_BLACK },
		{ "{WH}",   4, FORGE_GP_WHITE },
		{ "{LT}",   4, FORGE_GP_LTRIGGER },
		{ "{RT}",   4, FORGE_GP_RTRIGGER },
		{ "{DU}",   4, FORGE_GP_DPAD_UP },
		{ "{DD}",   4, FORGE_GP_DPAD_DOWN },
		{ "{DL}",   4, FORGE_GP_DPAD_LEFT },
		{ "{DR}",   4, FORGE_GP_DPAD_RIGHT },
		{ "{ST}",   4, FORGE_GP_START },
		{ "{BK}",   4, FORGE_GP_BACK },
		{ "{LS}",   4, FORGE_GP_LTHUMB },
		{ "{RS}",   4, FORGE_GP_RTHUMB },
		{ "{DPAD}", 6, 17 }
	};
	int i;
	if (!p || p[0] != '{')
		return 0;
	for (i = 0; i < (int)(sizeof(tbl) / sizeof(tbl[0])); i++)
	{
		if (strncmp(p, tbl[i].tok, (size_t)tbl[i].len) == 0)
		{
			if (out_len) *out_len = tbl[i].len;
			if (out_icon) *out_icon = tbl[i].icon;
			return 1;
		}
	}
	return 0;
}

static short xbox_icon_advance_px(int icon_id, int use_large_font)
{
	short base = use_large_font ? 20 : 17;
	if (icon_id == FORGE_GP_LTRIGGER || icon_id == FORGE_GP_RTRIGGER)
		return (short)(base + 5);
	if (icon_id == FORGE_GP_LTHUMB || icon_id == FORGE_GP_RTHUMB)
		return (short)(base + 2);
	return base;
}

static void draw_xbox_button_icon(int icon_id, short x0, short y0, short height, float fit_scale)
{
	static char const *const face_assets[3][6] = {
		{
			"ui\\hud\\buttons\\classic_a", "ui\\hud\\buttons\\classic_b",
			"ui\\hud\\buttons\\classic_x", "ui\\hud\\buttons\\classic_y",
			"ui\\hud\\buttons\\classic_black", "ui\\hud\\buttons\\classic_white"
		},
		{
			"ui\\hud\\buttons\\gem_a", "ui\\hud\\buttons\\gem_b",
			"ui\\hud\\buttons\\gem_x", "ui\\hud\\buttons\\gem_y",
			"ui\\shell\\bitmaps\\black_butn", "ui\\shell\\bitmaps\\white_butn"
		},
		{
			"ui\\shell\\bitmaps\\a_butn", "ui\\shell\\bitmaps\\b_butn",
			"ui\\shell\\bitmaps\\x_butn", "ui\\shell\\bitmaps\\y_butn",
			"ui\\shell\\bitmaps\\black_butn", "ui\\shell\\bitmaps\\white_butn"
		}
	};
	short style = controller_glyph_style();
	short slot = -1;
	short h = (short)((float)height * fit_scale + 0.5f);
	if (h < 10) h = 10;

	if (icon_id >= FORGE_GP_A && icon_id <= FORGE_GP_WHITE)
		slot = (short)(icon_id - FORGE_GP_A);
	else if (icon_id == FORGE_GP_LTRIGGER || icon_id == FORGE_GP_RTRIGGER)
		slot = (short)((icon_id == FORGE_GP_LTRIGGER) ? 6 : 7);
	else if (icon_id == FORGE_GP_LTHUMB || icon_id == FORGE_GP_RTHUMB)
		slot = (short)((icon_id == FORGE_GP_LTHUMB) ? 8 : 9);

	if (slot >= 0)
	{
		short key = forge_controller_glyph_asset_index(style, slot);
		if (key != NONE)
		{
			struct controller_glyph_asset const *entry = &controller_glyph_assets[key];
			struct bitmap_data *bmp = get_xbox_hires_button_bmp(slot, entry->tag, 64);
			if (bmp)
			{
				float bw = (float)(entry->bounds[2] - entry->bounds[0]);
				float bh = (float)(entry->bounds[3] - entry->bounds[1]);
				short w = (short)((float)h * (slot < 6 ? 0.96f : (slot < 8 ? 1.25f : 1.05f)) + 0.5f);
				short draw_h = (slot == 4 || slot == 5) ? (short)((float)w * (bh / (bw > 1.0f ? bw : 1.0f)) + 0.5f) : (short)(h + 2);
				short iy0 = (short)(y0 - 1 + ((h + 2 - draw_h) / 2));
				draw_ui_png_quad_uv(bmp, x0, iy0, (short)(x0 + w), (short)(iy0 + draw_h),
					(float)entry->bounds[0] / 512.0f, (float)entry->bounds[1] / 512.0f,
					(float)entry->bounds[2] / 512.0f, (float)entry->bounds[3] / 512.0f,
					slot < 6 ? 0xFFFFFFFFu : 0xFF66CCFFu);
				return;
			}
		}
	}

	if (slot >= 0 && slot < 6)
	{
		short s_idx = (style >= 0 && style < 3) ? style : 0;
		struct bitmap_data *bmp = get_xbox_hires_button_bmp(slot, face_assets[s_idx][slot], 32);
		short w = (short)((float)h * (21.0f / 22.0f) + 0.5f);
		short iy0 = (short)(y0 - 1);
		if (bmp)
		{
			draw_ui_png_quad_uv(bmp, x0, iy0, (short)(x0 + w), (short)(iy0 + h + 2),
				0.0f, 0.0f, 21.0f / 32.0f, 26.0f / 32.0f, 0xFFFFFFFFu);
			return;
		}
	}
	else if (slot == 6 || slot == 7)
	{
		char const *tname = (slot == 6) ? "ui\\hud\\buttons\\trigger_left" : "ui\\hud\\buttons\\trigger_right";
		struct bitmap_data *bmp = get_xbox_hires_button_bmp(slot, tname, 64);
		short w = (short)((float)h * 1.25f + 0.5f);
		short iy0 = (short)(y0 - 1);
		if (bmp)
		{
			draw_ui_png_quad_uv(bmp, x0, iy0, (short)(x0 + w), (short)(iy0 + h + 2),
				0.0f, 0.0f, 50.0f / 64.0f, 35.0f / 64.0f, 0xFF66CCFFu);
			return;
		}
	}
	else if (slot == 8 || slot == 9)
	{
		char const *tname = (slot == 8) ? "ui\\hud\\buttons\\thumbstick_left" : "ui\\hud\\buttons\\thumbstick_right";
		struct bitmap_data *bmp = get_xbox_hires_button_bmp(slot, tname, 64);
		short w = (short)((float)h * 1.05f + 0.5f);
		short iy0 = (short)(y0 - 1);
		if (bmp)
		{
			draw_ui_png_quad_uv(bmp, x0, iy0, (short)(x0 + w), (short)(iy0 + h + 2),
				0.0f, 0.0f, 36.0f / 64.0f, 40.0f / 64.0f, 0xFF66CCFFu);
			return;
		}
	}
	else if ((icon_id >= FORGE_GP_DPAD_UP && icon_id <= FORGE_GP_DPAD_RIGHT) || icon_id == 17)
	{
		/* Authentic Original Xbox D-Pad cross with the active direction highlighted in bright white/cyan */
		short sz = (short)(h - 1);
		short arm = (short)(sz / 3);
		short cx, cy;
		rectangle2d r;
		if (arm < 3) arm = 3;
		sz = (short)(arm * 3);
		cx = (short)(x0 + 1);
		cy = (short)(y0 + 1);
		/* Outer cross shadow */
		r.x0 = (short)(cx + arm - 1); r.y0 = (short)(cy - 1); r.x1 = (short)(cx + arm * 2 + 1); r.y1 = (short)(cy + sz + 1);
		draw_quad(&r, 0xD0061224u);
		r.x0 = (short)(cx - 1); r.y0 = (short)(cy + arm - 1); r.x1 = (short)(cx + sz + 1); r.y1 = (short)(cy + arm * 2 + 1);
		draw_quad(&r, 0xD0061224u);
		/* Base cross in Xbox HUD cyan */
		r.x0 = (short)(cx + arm); r.y0 = cy; r.x1 = (short)(cx + arm * 2); r.y1 = (short)(cy + sz);
		draw_quad(&r, 0xFF1E6BB8u);
		r.x0 = cx; r.y0 = (short)(cy + arm); r.x1 = (short)(cx + sz); r.y1 = (short)(cy + arm * 2);
		draw_quad(&r, 0xFF1E6BB8u);
		/* Highlighted direction arm */
		if (icon_id == FORGE_GP_DPAD_UP)
		{
			r.x0 = (short)(cx + arm); r.y0 = cy; r.x1 = (short)(cx + arm * 2); r.y1 = (short)(cy + arm);
			draw_quad(&r, 0xFFFFFFFFu);
		}
		else if (icon_id == FORGE_GP_DPAD_DOWN)
		{
			r.x0 = (short)(cx + arm); r.y0 = (short)(cy + arm * 2); r.x1 = (short)(cx + arm * 2); r.y1 = (short)(cy + sz);
			draw_quad(&r, 0xFFFFFFFFu);
		}
		else if (icon_id == FORGE_GP_DPAD_LEFT)
		{
			r.x0 = cx; r.y0 = (short)(cy + arm); r.x1 = (short)(cx + arm); r.y1 = (short)(cy + arm * 2);
			draw_quad(&r, 0xFFFFFFFFu);
		}
		else if (icon_id == FORGE_GP_DPAD_RIGHT)
		{
			r.x0 = (short)(cx + arm * 2); r.y0 = (short)(cy + arm); r.x1 = (short)(cx + sz); r.y1 = (short)(cy + arm * 2);
			draw_quad(&r, 0xFFFFFFFFu);
		}
		else
		{
			r.x0 = (short)(cx + arm); r.y0 = (short)(cy + arm); r.x1 = (short)(cx + arm * 2); r.y1 = (short)(cy + arm * 2);
			draw_quad(&r, 0xFF66CCFFu);
		}
		return;
	}
	else if (icon_id == FORGE_GP_START || icon_id == FORGE_GP_BACK)
	{
		/* Original Xbox oval Start / Back pill button */
		short w = (short)(h + 2);
		short bh = (short)(h - 4);
		rectangle2d r;
		if (bh < 8) bh = 8;
		r.x0 = x0; r.y0 = (short)(y0 + 2); r.x1 = (short)(x0 + w); r.y1 = (short)(y0 + 2 + bh);
		draw_quad(&r, 0xE00A2240u);
		draw_box_border(r.x0, r.y0, r.x1, r.y1, 1, 0xFF66CCFFu);
		/* Inner arrow / pause bars */
		if (icon_id == FORGE_GP_START)
		{
			r.x0 = (short)(x0 + w / 2 - 2); r.y0 = (short)(y0 + 5); r.x1 = (short)(x0 + w / 2 + 3); r.y1 = (short)(y0 + bh);
			draw_quad(&r, 0xFFFFFFFFu);
		}
		else
		{
			r.x0 = (short)(x0 + 4); r.y0 = (short)(y0 + 5); r.x1 = (short)(x0 + w - 4); r.y1 = (short)(y0 + bh);
			draw_quad(&r, 0xFF66CCFFu);
		}
		return;
	}
}

static short measure_ce_menu_str_pixels(int use_large_font, long fallback_font, char const *ascii)
{
	long font_idx;
	struct font_header *fhdr;
	short total = 0;
	int i = 0;

	if (!ascii || !ascii[0])
		return 0;

	font_idx = use_large_font ? get_large_forge_font(fallback_font) : get_small_forge_font(fallback_font);
	fhdr = (font_idx != NONE) ? font_definition_get(font_idx) : NULL;

	while (ascii[i])
	{
		int tok_len = 0, icon_id = 0;
		if (ascii[i] == '{' && parse_xbox_icon_token(ascii + i, &tok_len, &icon_id))
		{
			total = (short)(total + xbox_icon_advance_px(icon_id, use_large_font));
			i += tok_len;
			continue;
		}
		if (fhdr)
		{
			unsigned char ch = (unsigned char)ascii[i];
			struct forge_font_char_metric *cm;
			if (ch == '|')
				ch = '/';
			cm = (struct forge_font_char_metric *)font_get_character_by_ascii_code(fhdr, (word)ch);
			if (cm)
				total = (short)(total + cm->character_width);
			else
				total = (short)(total + (use_large_font ? 8 : 6));
		}
		else
		{
			total = (short)(total + (use_large_font ? 8 : 6));
		}
		i++;
	}
	return total;
}

static int str_has_xbox_icon_tokens(char const *ascii)
{
	int i = 0;
	if (!ascii)
		return 0;
	while (ascii[i])
	{
		int tok_len = 0, icon_id = 0;
		if (ascii[i] == '{' && parse_xbox_icon_token(ascii + i, &tok_len, &icon_id))
			return 1;
		i++;
	}
	return 0;
}

static void draw_ce_menu_str_box(
	int use_large_font,
	long fallback_font,
	short x0, short y0,
	short x1, short y1,
	short justification,
	char const *ascii,
	uint32_t argb_col)
{
	real_argb_color col;
	long font_idx;
	struct font_header *fhdr;
	short box_w = (short)(x1 - x0);
	short total_w, scaled_total_w, cur_x;
	float fit_scale = 1.0f;
	short icon_h = use_large_font ? 18 : 15;
	int i = 0;

	if (!ascii || !ascii[0] || x1 <= x0 || y1 <= y0)
		return;

	total_w = measure_ce_menu_str_pixels(use_large_font, fallback_font, ascii);
	if (total_w <= 0)
		return;

	/* Keep font size 100% consistent across all buttons and UI rows; only scale down if a full-width
	 * HUD bar line exceeds the safezone width (box_w >= 480). */
	if (box_w >= 480 && total_w > box_w - 8)
	{
		fit_scale = (float)(box_w - 8) / (float)total_w;
		if (fit_scale < 0.65f)
			fit_scale = 0.65f;
	}

	col.alpha = (real)((argb_col >> 24) & 0xFFu) / 255.0f;
	col.red   = (real)((argb_col >> 16) & 0xFFu) / 255.0f;
	col.green = (real)((argb_col >> 8)  & 0xFFu) / 255.0f;
	col.blue  = (real)(argb_col         & 0xFFu) / 255.0f;

	font_idx = use_large_font ? get_large_forge_font(fallback_font) : get_small_forge_font(fallback_font);
	fhdr = (font_idx != NONE) ? font_definition_get(font_idx) : NULL;

	/* Fast path when no {ICON} tokens are present: use Halo's native text justification over wide symmetric bounds
	 * so centering, right-alignment, and left-alignment are pixel-exact and never wrap or vary in size. */
	if (!str_has_xbox_icon_tokens(ascii))
	{
		wchar_t wbuf[256];
		int wlen = 0;
		rectangle2d bounds;
		float S = s_hud_ui_scale;
		float eff_scale = S * fit_scale;
		float eff_ox = s_hud_ui_origin_x;
		float eff_oy = s_hud_ui_origin_y;
		float pivot_x = (justification == 2) ? ((float)(x0 + x1) * 0.5f) : ((justification == 1) ? (float)x1 : (float)x0);

		while (ascii[i] && wlen < 254)
		{
			unsigned char ch = (unsigned char)ascii[i++];
			if (ch == '|')
				ch = '/';
			wbuf[wlen++] = (wchar_t)ch;
		}
		wbuf[wlen] = 0;

		if (fit_scale != 1.0f && (float)fabs((double)(1.0f - eff_scale)) > 0.001f)
		{
			eff_ox = (s_hud_ui_origin_x * (1.0f - S) + pivot_x * S * (1.0f - fit_scale)) / (1.0f - eff_scale);
			eff_oy = (s_hud_ui_origin_y * (1.0f - S) + (float)y0 * S * (1.0f - fit_scale)) / (1.0f - eff_scale);
		}

		if (justification == 2 /* center */)
		{
			short mid_x = (short)((x0 + x1) / 2);
			bounds.x0 = (short)(mid_x - 1024);
			bounds.x1 = (short)(mid_x + 1024);
		}
		else if (justification == 1 /* right */)
		{
			bounds.x0 = (short)(x1 - 2048);
			bounds.x1 = x1;
		}
		else
		{
			bounds.x0 = x0;
			bounds.x1 = (short)(x0 + 2048);
		}
		bounds.y0 = y0;
		bounds.y1 = (short)(y0 + 128);

		if (eff_scale != 1.0f)
			rasterizer_text_set_scale(eff_scale, eff_ox, eff_oy);
		draw_string_set_draw_mode(font_idx, NONE, justification, 0, &col);
		rasterizer_draw_unicode_string(&bounds, NULL, NULL, 0, wbuf);
		if (eff_scale != 1.0f)
			rasterizer_text_set_scale(1.0f, 0.0f, 0.0f);
		return;
	}

	scaled_total_w = (short)((float)total_w * fit_scale + 0.5f);
	if (justification == 2 /* center */)
		cur_x = (short)(x0 + (box_w - scaled_total_w) / 2);
	else if (justification == 1 /* right */)
		cur_x = (short)(x1 - scaled_total_w);
	else
		cur_x = x0;

	while (ascii[i])
	{
		int tok_len = 0, icon_id = 0;
		wchar_t wbuf[256];
		int wlen = 0;
		short run_unscaled_w = 0;

		if (ascii[i] == '{' && parse_xbox_icon_token(ascii + i, &tok_len, &icon_id))
		{
			short adv = (short)((float)xbox_icon_advance_px(icon_id, use_large_font) * fit_scale + 0.5f);
			draw_xbox_button_icon(icon_id, cur_x, (short)(y0 + 1), icon_h, fit_scale);
			cur_x = (short)(cur_x + adv);
			i += tok_len;
			continue;
		}

		while (ascii[i] && wlen < 254)
		{
			unsigned char ch;
			if (ascii[i] == '{' && parse_xbox_icon_token(ascii + i, &tok_len, &icon_id))
				break;
			ch = (unsigned char)ascii[i++];
			if (ch == '|')
				ch = '/';
			wbuf[wlen++] = (wchar_t)ch;
			if (fhdr)
			{
				struct forge_font_char_metric *cm =
					(struct forge_font_char_metric *)font_get_character_by_ascii_code(fhdr, (word)ch);
				run_unscaled_w = (short)(run_unscaled_w + (cm ? cm->character_width : (use_large_font ? 8 : 6)));
			}
			else
			{
				run_unscaled_w = (short)(run_unscaled_w + (use_large_font ? 8 : 6));
			}
		}
		wbuf[wlen] = 0;

		if (wlen > 0)
		{
			rectangle2d bounds;
			short lead = fhdr ? fhdr->leading_width : 0;
			float S = s_hud_ui_scale;
			float eff_scale = S * fit_scale;
			float eff_ox = s_hud_ui_origin_x;
			float eff_oy = s_hud_ui_origin_y;

			if (fit_scale != 1.0f && (float)fabs((double)(1.0f - eff_scale)) > 0.001f)
			{
				eff_ox = (s_hud_ui_origin_x * (1.0f - S) + (float)cur_x * S * (1.0f - fit_scale)) / (1.0f - eff_scale);
				eff_oy = (s_hud_ui_origin_y * (1.0f - S) + (float)y0 * S * (1.0f - fit_scale)) / (1.0f - eff_scale);
			}

			/* Offset x0 by -leading_width because draw_unicode_string_partial adds font->leading_width once at line start */
			bounds.x0 = (short)(cur_x - lead);
			bounds.y0 = y0;
			bounds.x1 = (short)(cur_x + run_unscaled_w + 1024);
			bounds.y1 = (short)(y0 + 128);

			if (eff_scale != 1.0f)
				rasterizer_text_set_scale(eff_scale, eff_ox, eff_oy);
			draw_string_set_draw_mode(font_idx, NONE, 0 /* left */, 0, &col);
			rasterizer_draw_unicode_string(&bounds, NULL, NULL, 0, wbuf);
			if (eff_scale != 1.0f)
				rasterizer_text_set_scale(1.0f, 0.0f, 0.0f);

			cur_x = (short)(cur_x + (short)((float)run_unscaled_w * fit_scale + 0.5f));
		}
	}
}

static void draw_ce_menu_str(
	int use_large_font,
	long fallback_font,
	short x,
	short y,
	char const *ascii,
	uint32_t argb_col)
{
	draw_ce_menu_str_box(use_large_font, fallback_font, x, y, (short)(x + 2048), (short)(y + 128), 0, ascii, argb_col);
}

static void draw_themed_panel(enum forge_ui_area_id area_id, rectangle2d const *r, short header_height);

static void draw_sized_bottom_pill_bar(
	long fallback_font,
	short safe_x0,
	short safe_x1,
	short y_bottom,
	char const *line1,
	char const *line2,
	short frame_idx,
	enum forge_ui_area_id fallback_area,
	uint32_t text_col,
	short *out_top_y)
{
	short safe_w = (short)(safe_x1 - safe_x0);
	short w1 = measure_ce_menu_str_pixels(0, fallback_font, line1);
	short w2 = (line2 && line2[0]) ? measure_ce_menu_str_pixels(0, fallback_font, line2) : 0;
	short content_w = (w1 > w2) ? w1 : w2;
	short bar_w = (short)(content_w + 34);
	short bx0, bx1, by0, by1;

	if (bar_w < 140)
		bar_w = 140;
	if (bar_w > safe_w)
		bar_w = safe_w;

	bx0 = (short)(safe_x0 + (safe_w - bar_w) / 2);
	bx1 = (short)(bx0 + bar_w);

	if (line2 && line2[0])
	{
		short row1_y0 = (short)(y_bottom - 44);
		short row1_y1 = (short)(y_bottom - 22);
		short row2_y0 = (short)(y_bottom - 22);
		short row2_y1 = y_bottom;

		if (out_top_y)
			*out_top_y = row1_y0;
		set_forge_hud_ui_scale(FORGE_IN_MATCH_HUD_SCALE, (float)(safe_x0 + safe_x1) * 0.5f, (float)y_bottom);
		if (!draw_ce_rounded_row_bar(frame_idx, bx0, row1_y0, bx1, row1_y1))
		{
			rectangle2d r1 = { row1_y0, bx0, row1_y1, bx1 };
			draw_themed_panel(fallback_area, &r1, 0);
		}
		if (!draw_ce_rounded_row_bar(frame_idx, bx0, row2_y0, bx1, row2_y1))
		{
			rectangle2d r2 = { row2_y0, bx0, row2_y1, bx1 };
			draw_themed_panel(fallback_area, &r2, 0);
		}
		draw_ce_menu_str_box(0, fallback_font, bx0, (short)(row1_y0 + 2), bx1, row1_y1, 2 /* center */, line1, text_col);
		draw_ce_menu_str_box(0, fallback_font, bx0, (short)(row2_y0 + 2), bx1, row2_y1, 2 /* center */, line2, text_col);
	}
	else
	{
		by0 = (short)(y_bottom - 24);
		by1 = y_bottom;
		if (out_top_y)
			*out_top_y = by0;
		set_forge_hud_ui_scale(FORGE_IN_MATCH_HUD_SCALE, (float)(safe_x0 + safe_x1) * 0.5f, (float)y_bottom);
		if (!draw_ce_rounded_row_bar(frame_idx, bx0, by0, bx1, by1))
		{
			rectangle2d bbar = { by0, bx0, by1, bx1 };
			draw_themed_panel(fallback_area, &bbar, 0);
		}
		draw_ce_menu_str_box(0, fallback_font, bx0, (short)(by0 + 2), bx1, by1, 2 /* center */, line1, text_col);
	}
}

static void draw_ce_text_button(
	long fallback_font,
	short x0, short y0,
	short width, short height,
	int focused,
	char const *label)
{
	short x1 = (short)(x0 + width);
	short y1 = (short)(y0 + height);
	short text_y = (short)(y0 + (height - 18) / 2);
	uint32_t text_col = focused ? 0xFFFFFFFFu : 0xFF2896FFu;

	if (!draw_ce_menu_bitmap_cropped("pc\\bitmaps\\text_button_background", (short)(focused ? 1 : 0), x0, y0, x1, y1, 128, 21, 0xFFFFFFFFu))
	{
		rectangle2d r;
		r.x0 = x0; r.y0 = y0; r.x1 = x1; r.y1 = y1;
		draw_quad(&r, focused ? 0xD8143C70u : 0xB007162Cu);
		draw_box_border(x0, y0, x1, y1, 1, 0xFF2896FFu);
	}
	draw_ce_menu_str_box(0, fallback_font, x0, text_y, x1, (short)(y1 + 4), 2 /* center */, label, text_col);
}

static void draw_themed_panel(enum forge_ui_area_id area_id, rectangle2d const *r, short header_height)
{
	struct forge_ui_area_style *st = &s_ui_theme.areas[area_id];
	if (!r || r->x1 <= r->x0 || r->y1 <= r->y0)
		return;

	if (st->use_image && st->bitmap_valid && st->bitmap.hardware_format)
	{
		draw_ui_png_quad(&st->bitmap, r->x0, r->y0, r->x1, r->y1, st->image_tint);
	}
	else
	{
		draw_quad(r, st->bg_color);
	}

	if (header_height > 0 && ((st->header_color >> 24) & 0xFFu) != 0)
	{
		rectangle2d hdr;
		hdr.x0 = r->x0;
		hdr.y0 = r->y0;
		hdr.x1 = r->x1;
		hdr.y1 = (short)(r->y0 + header_height);
		draw_quad(&hdr, st->header_color);
	}

	if (((st->border_color >> 24) & 0xFFu) != 0)
	{
		draw_box_border(r->x0, r->y0, r->x1, r->y1, 1, st->border_color);
	}
}

static float snap_angle_rad(float angle_rad, enum forge_snap_angle_mode mode)
{
	float step_deg = 0.0f;
	float step_rad;
	if (mode == _forge_snap_15_deg) step_deg = 15.0f;
	else if (mode == _forge_snap_45_deg) step_deg = 45.0f;
	else if (mode == _forge_snap_90_deg) step_deg = 90.0f;
	else return angle_rad;

	step_rad = step_deg * (3.1415926535f / 180.0f);
	return (float)floor((double)(angle_rad / step_rad) + 0.5) * step_rad;
}

static struct object_datum *forge_valid_object_get(long object_index)
{
	if (object_index == NONE)
		return NULL;
	return (struct object_datum *)object_try_and_get_and_verify_type(object_index, (unsigned long)-1);
}

#define FORGE_MAX_WELD_FOLLOWERS 128

struct forge_weld_follower_state
{
	int32_t live_idx;
	float orig_x, orig_y, orig_z;
	float orig_yaw, orig_pitch, orig_roll;
	float rel_pos_f, rel_pos_l, rel_pos_u;
	float rel_fwd_f, rel_fwd_l, rel_fwd_u;
	float rel_up_f, rel_up_l, rel_up_u;
};

static struct forge_weld_follower_state s_weld_followers[FORGE_MAX_WELD_FOLLOWERS];
static int s_weld_follower_count = 0;
static unsigned long s_last_weld_net_send_ms = 0;

static boolean forge_collision_test_vector(
	unsigned long flags,
	real_point3d const *point,
	real_vector3d const *vector,
	long ignore_object_index,
	struct collision_result *collision)
{
	long temp_hidden[192];
	int hidden_count = 0;
	int i;
	boolean res;

	/* Temporarily mark the local player's unit, any Monitor-mode player units, ignore_object_index,
	 * and any active Weld Group follower objects with _object_invisible_bit so collision_test_vector skips all of them. */
	{
		long local_unit = player_control_get_unit_index(0);
		if (local_unit != NONE && hidden_count < 192)
		{
			struct object_datum *lo = forge_valid_object_get(local_unit);
			if (lo && !TEST_FLAG(lo->object.flags, _object_invisible_bit))
			{
				SET_FLAG(lo->object.flags, _object_invisible_bit, TRUE);
				temp_hidden[hidden_count++] = local_unit;
			}
		}
		if (ignore_object_index != NONE && hidden_count < 192)
		{
			struct object_datum *io = forge_valid_object_get(ignore_object_index);
			if (io && !TEST_FLAG(io->object.flags, _object_invisible_bit))
			{
				SET_FLAG(io->object.flags, _object_invisible_bit, TRUE);
				temp_hidden[hidden_count++] = ignore_object_index;
			}
		}
		if (g_forge.grabbed_live_index >= 0 &&
			ignore_object_index == g_forge.grabbed_object_index &&
			s_weld_follower_count > 0)
		{
			int w;
			for (w = 0; w < s_weld_follower_count && hidden_count < 192; w++)
			{
				int32_t fli = s_weld_followers[w].live_idx;
				if (fli >= 0 && fli < g_forge.live_instance_count &&
					g_forge.live_instances[fli].active &&
					g_forge.live_instances[fli].object_index != NONE)
				{
					long fobj = g_forge.live_instances[fli].object_index;
					struct object_datum *fo = forge_valid_object_get(fobj);
					if (fo && !TEST_FLAG(fo->object.flags, _object_invisible_bit))
					{
						SET_FLAG(fo->object.flags, _object_invisible_bit, TRUE);
						temp_hidden[hidden_count++] = fobj;
					}
				}
			}
		}
		if (player_data)
		{
			long p_idx;
			for (p_idx = data_next_index(player_data, NONE);
				 p_idx != NONE && hidden_count < 192;
				 p_idx = data_next_index(player_data, p_idx))
			{
				struct player_datum *pl = player_get(p_idx);
				if (pl && pl->unit_index != NONE && forge_is_unit_in_monitor_mode(pl->unit_index))
				{
					struct object_datum *po = forge_valid_object_get(pl->unit_index);
					if (po && !TEST_FLAG(po->object.flags, _object_invisible_bit))
					{
						SET_FLAG(po->object.flags, _object_invisible_bit, TRUE);
						temp_hidden[hidden_count++] = pl->unit_index;
					}
				}
			}
		}
	}

	res = collision_test_vector(flags, point, vector, ignore_object_index, collision);

	for (i = 0; i < hidden_count; i++)
	{
		struct object_datum *ho = forge_valid_object_get(temp_hidden[i]);
		if (ho)
			SET_FLAG(ho->object.flags, _object_invisible_bit, FALSE);
	}

	return res;
}

static void euler_to_forward_up(float yaw, float pitch, float roll, real_vector3d *forward, real_vector3d *up)
{
	float cy = (float)cos((double)yaw);
	float sy = (float)sin((double)yaw);
	float cp = (float)cos((double)pitch);
	float sp = (float)sin((double)pitch);
	float cr = (float)cos((double)roll);
	float sr = (float)sin((double)roll);

	forward->i = cy * cp;
	forward->j = sy * cp;
	forward->k = sp;

	up->i = -cy * sp * cr + sy * sr;
	up->j = -sy * sp * cr - cy * sr;
	up->k = cp * cr;
}

static void forward_up_to_euler(
	real_vector3d const *forward,
	real_vector3d const *up,
	float *out_yaw,
	float *out_pitch,
	float *out_roll)
{
	float fk = forward->k;
	float yaw, pitch, roll, cy, sy, cp, sp;

	if (fk > 1.0f) fk = 1.0f;
	if (fk < -1.0f) fk = -1.0f;
	pitch = (float)asin((double)fk);
	if (fk > 0.9995f)
	{
		yaw = (float)atan2((double)(-up->j), (double)(-up->i));
		roll = 0.0f;
	}
	else if (fk < -0.9995f)
	{
		yaw = (float)atan2((double)up->j, (double)up->i);
		roll = 0.0f;
	}
	else
	{
		yaw = (float)atan2((double)forward->j, (double)forward->i);
		cy = (float)cos((double)yaw);
		sy = (float)sin((double)yaw);
		cp = (float)cos((double)pitch);
		sp = (float)sin((double)pitch);
		roll = (float)atan2(
			(double)(sy * up->i - cy * up->j),
			(double)(-cy * sp * up->i - sy * sp * up->j + cp * up->k));
	}

	*out_yaw = yaw;
	*out_pitch = pitch;
	*out_roll = roll;
}

static void forge_ensure_default_object_edit_entry(struct forge_live_instance *inst)
{
	int32_t i, j;

	if (!inst || !inst->is_default_scenario_object)
		return;
	if (inst->edit_entry_index >= 0 && inst->edit_entry_index < (int32_t)g_forge.edit_state.default_edit_count)
		return;

	for (i = 0; i < (int32_t)g_forge.edit_state.default_edit_count; i++)
	{
		struct forge_default_object_edit *de = &g_forge.edit_state.default_edits[i];
		int already_bound = 0;
		float odx, ody, odz, max_xy_d2;
		if (!de->active || de->deleted || de->group_tag != inst->group_tag)
			continue;
		for (j = 0; j < g_forge.live_instance_count; j++)
		{
			if (&g_forge.live_instances[j] != inst &&
				g_forge.live_instances[j].is_default_scenario_object &&
				g_forge.live_instances[j].edit_entry_index == i)
			{
				already_bound = 1;
				break;
			}
		}
		if (already_bound)
			continue;
		odx = de->orig_x - inst->orig_x;
		ody = de->orig_y - inst->orig_y;
		odz = de->orig_z - inst->orig_z;
		max_xy_d2 = (inst->group_tag == 'scen' || inst->group_tag == 'mach' ||
					 inst->group_tag == 'lifi' || inst->group_tag == 'ctrl') ? 0.04f : 4.0f;
		if (odx * odx + ody * ody < max_xy_d2 && (float)fabs((double)odz) < 3.0f &&
			_stricmp(de->tag_path, inst->tag_path) == 0)
		{
			inst->edit_entry_index = i;
			inst->orig_x = de->orig_x;
			inst->orig_y = de->orig_y;
			inst->orig_z = de->orig_z;
			return;
		}
	}

	if (g_forge.edit_state.default_edit_count < FORGE_MAX_DEFAULT_OBJECT_EDITS)
	{
		struct forge_default_object_edit *de =
			&g_forge.edit_state.default_edits[g_forge.edit_state.default_edit_count];
		inst->edit_entry_index = g_forge.edit_state.default_edit_count++;
		memset(de, 0, sizeof(*de));
		de->active = 1;
		de->deleted = 0;
		de->group_tag = inst->group_tag;
		snprintf(de->tag_path, sizeof(de->tag_path), "%s", inst->tag_path);
		de->orig_x = inst->orig_x;
		de->orig_y = inst->orig_y;
		de->orig_z = inst->orig_z;
		de->new_x = inst->x;
		de->new_y = inst->y;
		de->new_z = inst->z;
		de->new_yaw = inst->yaw;
		de->new_pitch = inst->pitch;
		de->new_roll = inst->roll;
		de->gametype_flags = inst->gametype_flags;
		de->respawn_timer_sec = inst->respawn_timer_sec;
		de->physics_mode = inst->physics_mode;
		de->group_id = inst->group_id;
		de->light_color_preset = inst->light_color_preset;
		de->light_intensity_preset = inst->light_intensity_preset;
		de->light_radius_preset = inst->light_radius_preset;
	}
}

static int32_t adopt_default_scenario_object(long object_index)
{
	struct object_datum *obj;
	struct unit_datum *unit;
	long def_idx;
	uint32_t grp;
	char const *tname;
	int32_t i, live_idx;
	struct forge_live_instance *inst;

	obj = forge_valid_object_get(object_index);
	if (!obj)
		return -1;

	/* Never adopt a player's controlled biped */
	unit = (struct unit_datum *)object_try_and_get_and_verify_type(object_index, _object_mask_biped);
	if (unit && unit->unit.player_index != NONE)
		return -1;

	/* Check if already in live_instances */
	for (i = 0; i < g_forge.live_instance_count; i++)
	{
		if (g_forge.live_instances[i].active &&
			g_forge.live_instances[i].object_index == object_index)
		{
			return i;
		}
	}

	if (g_forge.live_instance_count >= FORGE_MAX_LIVE_INSTANCES)
		return -1;

	def_idx = obj->definition_index;
	grp = (def_idx != NONE) ? (uint32_t)tag_get_group_tag(def_idx) : 'scen';
	tname = (def_idx != NONE) ? tag_get_name(def_idx) : "scenario_object";
	if (!tname || !tname[0])
		tname = "scenario_object";

	if (obj->object.parent_object_index != NONE)
		object_detach(object_index);
	SET_FLAG(obj->object.flags, _object_cannot_be_garbage_bit, TRUE);
	SET_FLAG(obj->object.flags, _object_deleted_when_deactivated_bit, FALSE);
	object_set_automatic_deactivation(object_index, FALSE);

	live_idx = g_forge.live_instance_count++;
	inst = &g_forge.live_instances[live_idx];
	memset(inst, 0, sizeof(*inst));
	inst->active = 1;
	inst->object_index = object_index;
	inst->actor_index = NONE;
	inst->edit_entry_index = -1;
	inst->group_tag = grp;
	inst->source_cache_id = 0;
	inst->is_default_scenario_object = 1;
	inst->spawned_once = 1;
	inst->death_tick = -1;
	inst->orig_x = obj->object.position.x;
	inst->orig_y = obj->object.position.y;
	inst->orig_z = obj->object.position.z;
	inst->x = obj->object.position.x;
	inst->y = obj->object.position.y;
	inst->z = obj->object.position.z;
	forward_up_to_euler(&obj->object.forward, &obj->object.up, &inst->yaw, &inst->pitch, &inst->roll);
	snprintf(inst->tag_path, sizeof(inst->tag_path), "%s", tname);
	forge_snap_to_scenario_placement_origin(inst->group_tag, inst->tag_path, &inst->orig_x, &inst->orig_y, &inst->orig_z);

	return live_idx;
}

int32_t forge_3d_adopt_or_find_object_instance(long object_index)
{
	return adopt_default_scenario_object(object_index);
}

static uint8_t *s_live_sp_buf = NULL;
static struct scenario *s_orig_sp_scen = NULL;
static void *s_orig_sp_addr = NULL;
static long s_orig_sp_count = 0;

static uint8_t *s_live_nf_buf = NULL;
static struct scenario *s_orig_nf_scen = NULL;
static uint8_t *s_orig_nf_addr = NULL;
static int s_orig_nf_count = 0;

/* Syncs all active 'spwn' live_instances into the in-memory scenario->players
 * AND into g_forge.edit_state.player_spawns so Red/Blue/FFA player spawns work live and save cleanly. */
void sync_live_player_spawns_to_scenario(void)
{
	struct scenario *scen = global_scenario_try_and_get();
	int i, out_count = 0;

	g_forge.edit_state.replace_player_spawns = 1;
	for (i = 0; i < g_forge.live_instance_count && out_count < FORGE_MAX_PLAYER_SPAWNS; i++)
	{
		struct forge_live_instance *inst = &g_forge.live_instances[i];
		struct forge_player_spawn_entry *sp;
		int16_t mode_type = 12; /* _game_engine_all */

		if (!inst->active || inst->group_tag != 'spwn')
			continue;

		if (inst->gametype_flags == 0x0001u) mode_type = 1;      /* CTF Only */
		else if (inst->gametype_flags == 0x0002u) mode_type = 2; /* Slayer Only */
		else if (inst->gametype_flags == 0x0004u) mode_type = 3; /* Oddball Only */
		else if (inst->gametype_flags == 0x0008u) mode_type = 4; /* KOTH Only */
		else if (inst->gametype_flags == 0x0010u) mode_type = 5; /* Race Only */

		sp = &g_forge.edit_state.player_spawns[out_count];
		memset(sp, 0, sizeof(*sp));
		sp->x = inst->x;
		sp->y = inst->y;
		sp->z = inst->z;
		sp->yaw = inst->yaw;
		sp->bsp_index = (int16_t)(inst->pad_inst & 0x7Fu);

		if (inst->team_designator == 2 || inst->team_designator < 0 || inst->team_designator > 1)
		{
			/* FFA Spawn (Yellow) */
			inst->team_designator = 2;
			sp->team_index = 0;
			sp->type0 = 13; /* _game_engine_all_non_team (FFA Slayer/Oddball/KOTH/Race) */
			sp->type1 = mode_type;
			snprintf(inst->tag_path, sizeof(inst->tag_path), "spawn\\player_ffa");
		}
		else if (inst->team_designator == 1)
		{
			/* Blue Team Spawn (Blue) */
			sp->team_index = 1;
			sp->type0 = (mode_type != 12) ? mode_type :
				((inst->netgame_flag_type > 0 && inst->netgame_flag_type != 13) ? inst->netgame_flag_type : 12);
			sp->type1 = 12;
			snprintf(inst->tag_path, sizeof(inst->tag_path), "spawn\\player_blue");
		}
		else
		{
			/* Red Team Spawn (Red) */
			inst->team_designator = 0;
			sp->team_index = 0;
			sp->type0 = (mode_type != 12) ? mode_type :
				((inst->netgame_flag_type > 0 && inst->netgame_flag_type != 13) ? inst->netgame_flag_type : 12);
			sp->type1 = 12;
			snprintf(inst->tag_path, sizeof(inst->tag_path), "spawn\\player_red");
		}

		out_count++;
	}
	g_forge.edit_state.player_spawn_count = out_count;

	if (scen && out_count <= FORGE_MAX_PLAYER_SPAWNS)
	{
		if (!s_live_sp_buf)
		{
			s_live_sp_buf = (uint8_t *)calloc(FORGE_MAX_PLAYER_SPAWNS, 52u);
		}
		if (scen->players.address && scen->players.address != s_live_sp_buf)
		{
			s_orig_sp_scen = scen;
			s_orig_sp_addr = scen->players.address;
			s_orig_sp_count = scen->players.count;
		}
		if (s_live_sp_buf)
		{
			memset(s_live_sp_buf, 0, (size_t)out_count * 52u);
			for (i = 0; i < out_count; i++)
			{
				struct forge_player_spawn_entry const *sp = &g_forge.edit_state.player_spawns[i];
				uint8_t *dst = s_live_sp_buf + (size_t)i * 52u;
				*(float *)(dst + 0x00) = sp->x;
				*(float *)(dst + 0x04) = sp->y;
				*(float *)(dst + 0x08) = sp->z;
				*(float *)(dst + 0x0C) = sp->yaw;
				*(int16_t *)(dst + 0x10) = sp->team_index;
				*(int16_t *)(dst + 0x12) = sp->bsp_index;
				*(int16_t *)(dst + 0x14) = sp->type0;
				*(int16_t *)(dst + 0x16) = sp->type1;
				*(int16_t *)(dst + 0x18) = sp->type2;
				*(int16_t *)(dst + 0x1A) = sp->type3;
			}
			scen->players.address = s_live_sp_buf;
			scen->players.count = out_count;
		}
	}
}

/* Syncs all active 'tele' and 'flag' live_instances into the in-memory scenario->netgame_flags
 * AND into g_forge.edit_state.netgame_flags so teleporters and KOTH/Oddball/Race/CTF markers work live and save cleanly. */
void sync_live_netgame_flags_to_scenario(void)
{
	struct scenario *scen = global_scenario_try_and_get();
	int i, out_count = 0;

	g_forge.edit_state.replace_netgame_flags = 1;
	for (i = 0; i < g_forge.live_instance_count && out_count < FORGE_MAX_NETGAME_FLAGS; i++)
	{
		struct forge_live_instance *inst = &g_forge.live_instances[i];
		struct forge_netgame_flag_entry *nf;
		if (!inst->active || (inst->group_tag != 'tele' && inst->group_tag != 'flag'))
			continue;

		if (inst->group_tag == 'flag' && inst->netgame_flag_type == _netgame_flag_ctf_flag)
		{
			inst->team_designator = (inst->team_designator == 1) ? 1 : 0;
			snprintf(inst->tag_path, sizeof(inst->tag_path), "%s",
				inst->team_designator == 1 ? "objective\\ctf_flag_blue" : "objective\\ctf_flag_red");
		}

		nf = &g_forge.edit_state.netgame_flags[out_count];
		memset(nf, 0, sizeof(*nf));
		nf->x = inst->x;
		nf->y = inst->y;
		nf->z = inst->z;
		nf->yaw = inst->yaw;
		nf->type = inst->netgame_flag_type;
		nf->usage_id = inst->team_designator;
		out_count++;
	}
	g_forge.edit_state.netgame_flag_count = out_count;

	if (scen && out_count <= FORGE_MAX_NETGAME_FLAGS)
	{
		/* Reallocate scenario->netgame_flags in memory if needed so newly added teleporters/hills work live */
		if (!s_live_nf_buf)
		{
			s_live_nf_buf = (uint8_t *)calloc(FORGE_MAX_NETGAME_FLAGS, 148u);
		}
		if (scen->netgame_flags.address && scen->netgame_flags.address != s_live_nf_buf)
		{
			s_orig_nf_scen = scen;
			s_orig_nf_addr = (uint8_t *)scen->netgame_flags.address;
			s_orig_nf_count = scen->netgame_flags.count;
		}
		if (s_live_nf_buf)
		{
			memset(s_live_nf_buf, 0, (size_t)out_count * 148u);
			for (i = 0; i < out_count; i++)
			{
				uint8_t *dst = s_live_nf_buf + (size_t)i * 148u;
				*(float *)(dst + 0x00) = g_forge.edit_state.netgame_flags[i].x;
				*(float *)(dst + 0x04) = g_forge.edit_state.netgame_flags[i].y;
				*(float *)(dst + 0x08) = g_forge.edit_state.netgame_flags[i].z;
				*(float *)(dst + 0x0C) = g_forge.edit_state.netgame_flags[i].yaw;
				*(int16_t *)(dst + 0x10) = g_forge.edit_state.netgame_flags[i].type;
				*(int16_t *)(dst + 0x12) = g_forge.edit_state.netgame_flags[i].usage_id;
				*(int32_t *)(dst + 0x1C) = NONE; /* weapon_group.index = NONE */
			}
			/* Also update CTF flag base positions in s_orig_nf_addr if ctf_globals cached a pointer to it */
			if (s_orig_nf_addr && s_orig_nf_count > 0)
			{
				int j;
				for (j = 0; j < s_orig_nf_count; j++)
				{
					uint8_t *orig_f = s_orig_nf_addr + (size_t)j * 148u;
					int16_t otype = *(int16_t const *)(orig_f + 0x10);
					int16_t oteam = *(int16_t const *)(orig_f + 0x12);
					if (otype == _netgame_flag_ctf_flag)
					{
						for (i = 0; i < out_count; i++)
						{
							if (g_forge.edit_state.netgame_flags[i].type == _netgame_flag_ctf_flag &&
								g_forge.edit_state.netgame_flags[i].usage_id == oteam)
							{
								*(float *)(orig_f + 0x00) = g_forge.edit_state.netgame_flags[i].x;
								*(float *)(orig_f + 0x04) = g_forge.edit_state.netgame_flags[i].y;
								*(float *)(orig_f + 0x08) = g_forge.edit_state.netgame_flags[i].z;
								*(float *)(orig_f + 0x0C) = g_forge.edit_state.netgame_flags[i].yaw;
								break;
							}
						}
					}
				}
			}
			scen->netgame_flags.address = s_live_nf_buf;
			scen->netgame_flags.count = out_count;
		}
		if (player_data)
		{
			long p_idx;
			for (p_idx = data_next_index(player_data, NONE); p_idx != NONE; p_idx = data_next_index(player_data, p_idx))
			{
				struct player_datum *pl = player_get(p_idx);
				if (pl && (pl->teleporter_index < 0 || pl->teleporter_index >= out_count))
					pl->teleporter_index = NONE;
			}
		}
	}
}

void forge_sync_live_markers_to_scenario(uint32_t group_tag)
{
	if (group_tag == 'spwn')
		sync_live_player_spawns_to_scenario();
	else if (group_tag == 'tele' || group_tag == 'flag')
		sync_live_netgame_flags_to_scenario();
}

void forge_sync_instance_extended_props_to_edit_state(int32_t live_idx)
{
	struct forge_live_instance *inst;

	if (live_idx < 0 || live_idx >= g_forge.live_instance_count)
		return;
	inst = &g_forge.live_instances[live_idx];
	if (!inst->active)
		return;

	if (inst->group_tag == 'spwn')
	{
		if (inst->is_default_scenario_object && inst->edit_entry_index < 0)
			inst->edit_entry_index = 0;
		sync_live_player_spawns_to_scenario();
		return;
	}
	if (inst->group_tag == 'tele' || inst->group_tag == 'flag')
	{
		if (inst->is_default_scenario_object && inst->edit_entry_index < 0)
			inst->edit_entry_index = 0;
		sync_live_netgame_flags_to_scenario();
		return;
	}

	if (inst->is_default_scenario_object)
	{
		forge_ensure_default_object_edit_entry(inst);
		if (inst->edit_entry_index >= 0 &&
			inst->edit_entry_index < (int32_t)g_forge.edit_state.default_edit_count)
		{
			struct forge_default_object_edit *de = &g_forge.edit_state.default_edits[inst->edit_entry_index];
			de->new_x = inst->x;
			de->new_y = inst->y;
			de->new_z = inst->z;
			de->new_yaw = inst->yaw;
			de->new_pitch = inst->pitch;
			de->new_roll = inst->roll;
			de->gametype_flags = inst->gametype_flags;
			de->respawn_timer_sec = inst->respawn_timer_sec;
			de->physics_mode = inst->physics_mode;
			de->group_id = inst->group_id;
			de->light_color_preset = inst->light_color_preset;
			de->light_intensity_preset = inst->light_intensity_preset;
			de->light_radius_preset = inst->light_radius_preset;
		}
	}
	else if (inst->is_ai_unit)
	{
		if (inst->edit_entry_index >= 0 &&
			inst->edit_entry_index < (int32_t)g_forge.edit_state.ai_spawn_count)
		{
			struct forge_ai_spawn_entry *ai = &g_forge.edit_state.ai_spawns[inst->edit_entry_index];
			ai->x = inst->x;
			ai->y = inst->y;
			ai->z = inst->z;
			ai->yaw = inst->yaw;
			ai->team_designator = inst->team_designator;
			ai->respawn_timer_sec = inst->respawn_timer_sec;
			ai->ai_order = inst->ai_order;
			ai->group_id = inst->group_id;
		}
	}
	else
	{
		if (inst->edit_entry_index >= 0 &&
			inst->edit_entry_index < (int32_t)g_forge.edit_state.placed_object_count)
		{
			struct forge_placed_object_entry *po = &g_forge.edit_state.placed_objects[inst->edit_entry_index];
			po->x = inst->x;
			po->y = inst->y;
			po->z = inst->z;
			po->yaw = inst->yaw;
			po->pitch = inst->pitch;
			po->roll = inst->roll;
			po->team_index = inst->team_designator;
			po->gametype_flags = inst->gametype_flags;
			po->respawn_timer_sec = inst->respawn_timer_sec;
			po->physics_mode = inst->physics_mode;
			po->group_id = inst->group_id;
			po->light_color_preset = inst->light_color_preset;
			po->light_intensity_preset = inst->light_intensity_preset;
			po->light_radius_preset = inst->light_radius_preset;
		}
	}
}

static void forge_ensure_object_connected_to_bsp(long object_index, float x, float y, float z)
{
	struct object_datum *obj = forge_valid_object_get(object_index);
	if (!obj)
		return;

	SET_FLAG(obj->object.flags, _object_cannot_be_garbage_bit, TRUE);
	SET_FLAG(obj->object.flags, _object_deleted_when_deactivated_bit, FALSE);
	SET_FLAG(obj->object.flags, _object_outside_of_map_bit, FALSE);
	if (TEST_FLAG(obj->object.flags, _object_garbage_bit))
		object_set_garbage(object_index, FALSE);

	if (obj->object.type == _object_type_weapon ||
		obj->object.type == _object_type_equipment ||
		obj->object.type == _object_type_garbage)
	{
		struct item_datum *it = (struct item_datum *)obj;
		it->item.detonation_ticks = 0;
		it->item.last_owned_time = game_time_get();
	}

	if (obj->object.parent_object_index == NONE &&
		(obj->object.first_cluster_reference_index == NONE ||
		 obj->object.location.cluster_index == NONE ||
		 TEST_FLAG(obj->object.flags, _object_outside_of_map_bit)))
	{
		static float const s_Offsets[10][3] = {
			{  0.00f,  0.00f,  0.08f },
			{  0.00f,  0.00f,  0.25f },
			{  0.00f,  0.00f,  0.60f },
			{  0.00f,  0.00f,  1.10f },
			{  0.00f,  0.00f, -0.20f },
			{  0.25f,  0.00f,  0.15f },
			{ -0.25f,  0.00f,  0.15f },
			{  0.00f,  0.25f,  0.15f },
			{  0.00f, -0.25f,  0.15f },
			{  0.00f,  0.00f,  2.00f }
		};
		struct location loc;
		real_point3d probe_pt;
		int k;
		int found = 0;

		for (k = 0; k < 10; k++)
		{
			probe_pt.x = x + s_Offsets[k][0];
			probe_pt.y = y + s_Offsets[k][1];
			probe_pt.z = z + s_Offsets[k][2];
			memset(&loc, 0, sizeof(loc));
			loc.cluster_index = NONE;
			scenario_location_from_point(&loc, &probe_pt);
			if (loc.cluster_index != NONE)
			{
				found = 1;
				break;
			}
		}
		if (!found)
		{
			probe_pt.x = g_forge.cam_x;
			probe_pt.y = g_forge.cam_y;
			probe_pt.z = g_forge.cam_z;
			memset(&loc, 0, sizeof(loc));
			loc.cluster_index = NONE;
			scenario_location_from_point(&loc, &probe_pt);
			if (loc.cluster_index != NONE)
				found = 1;
		}
		if (found)
		{
			object_disconnect_from_map(object_index);
			object_reconnect_to_map(object_index, &loc);
			SET_FLAG(obj->object.flags, _object_outside_of_map_bit, FALSE);
			object_set_automatic_deactivation(object_index, FALSE);
			object_activate(object_index);
		}
	}
}

static long forge_create_configured_engine_object(
	long def_idx,
	uint32_t group_tag,
	float x, float y, float z,
	float yaw, float pitch, float roll)
{
	struct object_placement_data placement;
	long obj_idx;
	struct object_datum *obj;

	if (def_idx == NONE)
		return NONE;

	object_placement_data_new(&placement, def_idx, NONE);
	SET_FLAG(placement.flags, _new_object_never_automatically_delete_bit, TRUE);
	placement.position.x = x;
	placement.position.y = y;
	placement.position.z = z;
	euler_to_forward_up(yaw, pitch, roll, &placement.forward, &placement.up);

	obj_idx = object_new(&placement);
	obj = forge_valid_object_get(obj_idx);
	if (!obj)
		return NONE;

	SET_FLAG(obj->object.flags, _object_cannot_be_garbage_bit, TRUE);
	SET_FLAG(obj->object.flags, _object_deleted_when_deactivated_bit, FALSE);
	SET_FLAG(obj->object.flags, _object_outside_of_map_bit, FALSE);
	if (group_tag == 'vehi')
	{
		SET_FLAG(obj->object.flags, _object_at_rest_bit, FALSE);
		unit_set_enterable_by_player(obj_idx, TRUE);
		unit_set_controllable(obj_idx, TRUE);
	}
	else if (obj->object.type == _object_type_machine ||
			 obj->object.type == _object_type_light_fixture ||
			 obj->object.type == _object_type_control)
	{
		struct scenario_device_datum scen_dev;
		memset(&scen_dev, 0, sizeof(scen_dev));
		scen_dev.power_group_index = NONE;
		scen_dev.position_group_index = NONE;
		scen_dev.flags = 0;
		device_add_scenario_information(obj_idx, &scen_dev);
		if (obj->object.type == _object_type_light_fixture)
		{
			struct light_fixture_datum *lf = (struct light_fixture_datum *)obj;
			lf->light_fixture.color.red = 1.0f;
			lf->light_fixture.color.green = 1.0f;
			lf->light_fixture.color.blue = 1.0f;
			lf->light_fixture.intensity = 1.0f;
			lf->light_fixture.falloff_angle = 1.0f;
			lf->light_fixture.cutoff_angle = 1.2f;
		}
	}
	object_set_automatic_deactivation(obj_idx, FALSE);
	object_activate(obj_idx);
	forge_ensure_object_connected_to_bsp(obj_idx, x, y, z);
	return obj_idx;
}

long forge_create_engine_object_for_instance(int32_t live_idx, long def_idx, int is_held)
{
	struct forge_live_instance *inst;
	long obj_idx;

	if (live_idx < 0 || live_idx >= g_forge.live_instance_count || def_idx == NONE)
		return NONE;
	inst = &g_forge.live_instances[live_idx];
	if (!inst->active)
		return NONE;

	if (inst->object_index != NONE)
	{
		if (forge_valid_object_get(inst->object_index))
			object_delete(inst->object_index);
		inst->object_index = NONE;
	}

	obj_idx = forge_create_configured_engine_object(
		def_idx,
		inst->group_tag,
		inst->x, inst->y, inst->z,
		inst->yaw, inst->pitch, inst->roll);
	inst->object_index = obj_idx;
	inst->pad_inst &= (uint8_t)~0x40u;
	if (g_forge.grabbed_live_index == live_idx)
		g_forge.grabbed_object_index = obj_idx;

	if (obj_idx != NONE)
	{
		inst->spawned_once = 1;
		inst->death_tick = -1;
		forge_3d_move_instance_ext(
			live_idx,
			inst->x, inst->y, inst->z,
			inst->yaw, inst->pitch, inst->roll,
			is_held,
			0);
		forge_apply_instance_light_settings(live_idx);
	}
	return obj_idx;
}

int32_t forge_3d_spawn_object_at_slot(
	int32_t forced_live_idx,
	uint32_t group_tag,
	char const *tag_path,
	uint8_t source_cache_id,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int16_t team_designator,
	int broadcast_net)
{
	int32_t live_idx = -1;
	struct forge_live_instance *inst;

	if (!tag_path || !tag_path[0])
		return -1;

	if (forced_live_idx >= 0 && forced_live_idx < FORGE_MAX_LIVE_INSTANCES)
	{
		if (forced_live_idx >= g_forge.live_instance_count)
		{
			memset(
				&g_forge.live_instances[g_forge.live_instance_count],
				0,
				(size_t)(forced_live_idx - g_forge.live_instance_count + 1) * sizeof(struct forge_live_instance));
			g_forge.live_instance_count = forced_live_idx + 1;
		}
		else if (g_forge.live_instances[forced_live_idx].active &&
			g_forge.live_instances[forced_live_idx].object_index != NONE)
		{
			if (forge_valid_object_get(g_forge.live_instances[forced_live_idx].object_index))
				object_delete(g_forge.live_instances[forced_live_idx].object_index);
			g_forge.live_instances[forced_live_idx].object_index = NONE;
		}
		live_idx = forced_live_idx;
	}
	else if (g_forge.live_instance_count < FORGE_MAX_LIVE_INSTANCES)
	{
		live_idx = g_forge.live_instance_count++;
	}
	else
	{
		int32_t s;
		for (s = 0; s < g_forge.live_instance_count; s++)
		{
			if (!g_forge.live_instances[s].active)
			{
				live_idx = s;
				break;
			}
		}
		if (live_idx < 0)
		{
			forge_set_status("Forge Object Budget Reached (%d/%d)!", FORGE_MAX_LIVE_INSTANCES, FORGE_MAX_LIVE_INSTANCES);
			return -1;
		}
	}
	inst = &g_forge.live_instances[live_idx];
	if (inst->active && inst->object_index != NONE && forge_valid_object_get(inst->object_index))
		object_delete(inst->object_index);
	memset(inst, 0, sizeof(*inst));
	inst->active = 1;
	inst->object_index = NONE;
	inst->actor_index = NONE;
	inst->edit_entry_index = -1;
	inst->group_tag = group_tag;
	inst->source_cache_id = source_cache_id;
	inst->gametype_flags = 0; /* 0 = All Modes */
	inst->netgame_flag_type = -1;
	inst->team_designator = team_designator;
	inst->death_tick = -1;
	inst->x = x;
	inst->y = y;
	inst->z = z;
	inst->yaw = yaw;
	inst->pitch = pitch;
	inst->roll = roll;
	snprintf(inst->tag_path, sizeof(inst->tag_path), "%s", tag_path);
	if (game_connection() == _game_connection_network_client && broadcast_net != 0)
		inst->pad_inst |= 0x80u;

	if (group_tag == 'spwn')
	{
		inst->team_designator = strstr(tag_path, "blue") ? 1 : strstr(tag_path, "ffa") ? 2 : 0;
		inst->netgame_flag_type = (inst->team_designator == 2) ? 13 : 12;
		sync_live_player_spawns_to_scenario();
		forge_set_status("Placed %s Player Spawn at (%.1f, %.1f, %.1f) [%s Rotate / %s Options]",
			inst->team_designator == 1 ? "Blue Team" : (inst->team_designator == 2 ? "FFA (Yellow)" : "Red Team"),
			x, y, z,
			forge_action_prompt(FORGE_BIND_ROTATE_HOLD),
			forge_action_prompt(FORGE_BIND_OBJECT_OPTIONS));
	}
	else if (group_tag == 'tele')
	{
		/* _netgame_flag_teleport_from = 6, _netgame_flag_teleport_to = 7 */
		inst->netgame_flag_type = strstr(tag_path, "destination") ? 7 : 6;
		inst->team_designator = (team_designator >= 0 && team_designator < 16) ? team_designator : 0;
		sync_live_netgame_flags_to_scenario();
		forge_set_status("Placed Teleporter %s [Ref #%d] (%s while holding for Options)",
			inst->netgame_flag_type == 6 ? "IN (Origin)" : "OUT (Destination)",
			(int)inst->team_designator,
			forge_action_prompt(FORGE_BIND_OBJECT_OPTIONS));
	}
	else if (group_tag == 'flag')
	{
		/* _netgame_flag_ctf_flag = 0, _netgame_flag_ctf_vehicle = 1, _netgame_flag_oddball_ball_spawn = 2,
		 * _netgame_flag_race_track = 3, _netgame_flag_race_vehicle = 4, _netgame_flag_vegas_bank = 5, _netgame_flag_hill_flag = 8 */
		inst->netgame_flag_type =
			strstr(tag_path, "oddball") ? 2 :
			strstr(tag_path, "koth") ? 8 :
			strstr(tag_path, "race_vehicle") ? 4 :
			strstr(tag_path, "race") ? 3 :
			strstr(tag_path, "vegas") ? 5 :
			strstr(tag_path, "ctf_vehicle") ? 1 : 0;
		if (inst->netgame_flag_type == 0 || inst->netgame_flag_type == 1)
		{
			inst->team_designator = strstr(tag_path, "blue") ? 1 : 0;
		}
		else if (inst->netgame_flag_type == 2 && team_designator == 0)
		{
			/* Auto-assign next unused Oddball spawn ID (0..15) so multiple ball spawns each have a valid ID */
			int cand, j, used;
			int16_t chosen_id = 0;
			for (cand = 0; cand < 16; cand++)
			{
				used = 0;
				for (j = 0; j < live_idx; j++)
				{
					if (g_forge.live_instances[j].active &&
						g_forge.live_instances[j].group_tag == 'flag' &&
						g_forge.live_instances[j].netgame_flag_type == 2 &&
						g_forge.live_instances[j].team_designator == cand)
					{
						used = 1;
						break;
					}
				}
				if (!used)
				{
					chosen_id = (int16_t)cand;
					break;
				}
			}
			inst->team_designator = chosen_id;
		}
		else
		{
			inst->team_designator = (team_designator >= 0) ? team_designator : 0;
		}
		sync_live_netgame_flags_to_scenario();
		forge_set_status("Placed Objective '%s' [Ref/Team #%d] at (%.1f, %.1f, %.1f)",
			tag_path, (int)inst->team_designator, x, y, z);
	}
	else if (group_tag == 'actv')
	{
		int32_t spawned_unit = NONE;
		inst->is_ai_unit = 1;
		inst->spawned_once = 1;
		inst->death_tick = -1;
		if (game_connection() != _game_connection_network_client)
		{
			inst->actor_index = forge_ai_spawn_unit_live(tag_path, x, y, z, yaw, team_designator, &spawned_unit);
			inst->object_index = spawned_unit;
			if (inst->ai_order != 0)
			{
				forge_ai_apply_order(
					inst->actor_index, inst->object_index, inst->ai_order, x, y, z, game_time_get());
			}
		}
		if (g_forge.edit_state.ai_spawn_count < FORGE_MAX_AI_SPAWNS)
		{
			struct forge_ai_spawn_entry *ai = &g_forge.edit_state.ai_spawns[g_forge.edit_state.ai_spawn_count];
			inst->edit_entry_index = g_forge.edit_state.ai_spawn_count++;
			memset(ai, 0, sizeof(*ai));
			snprintf(ai->actor_variant_path, sizeof(ai->actor_variant_path), "%s", tag_path);
			ai->x = x;
			ai->y = y;
			ai->z = z;
			ai->yaw = yaw;
			ai->team_designator = team_designator;
			ai->respawn_timer_sec = inst->respawn_timer_sec;
			ai->ai_order = inst->ai_order;
			ai->group_id = inst->group_id;
		}
		forge_set_status("Placed AI '%s' at (%.1f, %.1f, %.1f)", tag_path, x, y, z);
		if (broadcast_net && forge_is_multiplayer_session())
		{
			forge_network_send_spawn_ext(
				live_idx,
				group_tag,
				tag_path,
				source_cache_id,
				x, y, z,
				yaw, pitch, roll,
				team_designator,
				inst->object_index,
				NONE);
		}
		return live_idx;
	}
	else
	{
		uint32_t actual_grp = group_tag;
		long def_idx = forge_resolve_engine_tag(group_tag, tag_path, &actual_grp);
		int is_net_obj_type = (group_tag == 'vehi' || group_tag == 'weap' ||
							   group_tag == 'eqip' || group_tag == 'bipd' ||
							   actual_grp == 'vehi' || actual_grp == 'weap' ||
							   actual_grp == 'eqip' || actual_grp == 'bipd');
		int skip_local_obj_new_on_client = (game_connection() == _game_connection_network_client && is_net_obj_type);

		if (def_idx != NONE && !skip_local_obj_new_on_client)
		{
			inst->object_index = forge_create_configured_engine_object(
				def_idx, group_tag, x, y, z, yaw, pitch, roll);
			if (inst->object_index != NONE)
			{
				inst->spawned_once = 1;
				if (inst->light_color_preset || inst->light_intensity_preset || inst->light_radius_preset)
					forge_apply_instance_light_settings(live_idx);
			}
		}

		if (g_forge.edit_state.placed_object_count < FORGE_MAX_PLACED_OBJECTS)
		{
			struct forge_placed_object_entry *po = &g_forge.edit_state.placed_objects[g_forge.edit_state.placed_object_count];
			inst->edit_entry_index = g_forge.edit_state.placed_object_count++;
			memset(po, 0, sizeof(*po));
			po->active = 1;
			po->source_cache_id = source_cache_id;
			po->group_tag = group_tag;
			po->gametype_flags = inst->gametype_flags;
			po->respawn_timer_sec = inst->respawn_timer_sec;
			po->physics_mode = inst->physics_mode;
			po->group_id = inst->group_id;
			po->light_color_preset = inst->light_color_preset;
			po->light_intensity_preset = inst->light_intensity_preset;
			po->light_radius_preset = inst->light_radius_preset;
			po->category = (group_tag == 'vehi') ? _forge_object_vehicle :
				(group_tag == 'weap') ? _forge_object_weapon :
				(group_tag == 'bipd') ? _forge_object_biped :
				(group_tag == 'eqip') ? _forge_object_equipment : _forge_object_scenery;
			snprintf(po->tag_path, sizeof(po->tag_path), "%s", tag_path);
			po->x = x;
			po->y = y;
			po->z = z;
			po->yaw = yaw;
			po->pitch = pitch;
			po->roll = roll;
		}
		forge_set_status("Placed '%s' at (%.1f, %.1f, %.1f)", tag_path, x, y, z);

		if (broadcast_net && forge_is_multiplayer_session())
		{
			forge_network_send_spawn_ext(
				live_idx,
				group_tag,
				tag_path,
				source_cache_id,
				x, y, z,
				yaw, pitch, roll,
				team_designator,
				inst->object_index,
				def_idx);
		}
		return live_idx;
	}

	if (broadcast_net && forge_is_multiplayer_session())
	{
		forge_network_send_spawn_ext(
			live_idx,
			group_tag,
			tag_path,
			source_cache_id,
			x, y, z,
			yaw, pitch, roll,
			inst->team_designator,
			inst->object_index,
			NONE);
	}

	return live_idx;
}

int32_t forge_3d_spawn_object_at(
	uint32_t group_tag,
	char const *tag_path,
	uint8_t source_cache_id,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int16_t team_designator,
	int broadcast_net)
{
	return forge_3d_spawn_object_at_slot(
		-1,
		group_tag,
		tag_path,
		source_cache_id,
		x, y, z,
		yaw, pitch, roll,
		team_designator,
		broadcast_net);
}

static void forge_eject_all_passengers_from_vehicle(long vehicle_index)
{
	struct object_iterator u_iter;
	struct unit_datum *u;

	if (vehicle_index == NONE)
		return;

	object_iterator_new(&u_iter, _object_mask_unit, 0);
	while ((u = (struct unit_datum *)object_iterator_next(&u_iter)) != NULL)
	{
		long u_idx = u_iter.index;
		if (u_idx == vehicle_index)
			continue;
		if (u->object.parent_object_index == vehicle_index && u->unit.parent_seat_index != NONE)
		{
			unit_exit_seat_end(u_idx);
			SET_FLAG(u->object.flags, _object_at_rest_bit, FALSE);
			object_activate(u_idx);
		}
	}
}

static int forge_is_vehicle_occupied(long vehicle_index)
{
	struct object_iterator u_iter;
	struct unit_datum *u;

	if (vehicle_index == NONE)
		return 0;

	object_iterator_new(&u_iter, _object_mask_unit, 0);
	while ((u = (struct unit_datum *)object_iterator_next(&u_iter)) != NULL)
	{
		if (u_iter.index != vehicle_index &&
			u->object.parent_object_index == vehicle_index &&
			u->unit.parent_seat_index != NONE)
		{
			return 1;
		}
	}
	return 0;
}

void forge_3d_move_instance_ext(
	int32_t live_idx,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int is_held,
	int broadcast_net)
{
	struct forge_live_instance *inst;

	if (live_idx < 0 || live_idx >= g_forge.live_instance_count)
		return;
	inst = &g_forge.live_instances[live_idx];
	if (!inst->active)
		return;

	inst->x = x;
	inst->y = y;
	inst->z = z;
	inst->yaw = yaw;
	inst->pitch = pitch;
	inst->roll = roll;

	if (inst->object_index != NONE)
	{
		struct object_datum *obj = forge_valid_object_get(inst->object_index);
		if (obj)
		{
			real_point3d pos;
			real_vector3d fwd, up;
			int wake_normal_physics = (!is_held &&
				inst->physics_mode == _forge_physics_normal &&
				(obj->object.type == _object_type_vehicle ||
				 obj->object.type == _object_type_weapon ||
				 obj->object.type == _object_type_equipment ||
				 (obj->object.type == _object_type_biped && !inst->is_ai_unit)));

			pos.x = x;
			pos.y = y;
			pos.z = z;
			euler_to_forward_up(yaw, pitch, roll, &fwd, &up);
			if ((obj->object.type == _object_type_weapon ||
				 obj->object.type == _object_type_equipment ||
				 obj->object.type == _object_type_garbage) &&
				obj->definition_index != NONE)
			{
				struct item_definition *idef = item_definition_get(obj->definition_index);
				if (idef)
					idef->item.flags &= ~1UL; /* Clear _item_definition_always_maintains_z_up_bit so pitched/rolled items stay oriented */
			}
			if (obj->object.type == _object_type_vehicle)
			{
				unit_set_enterable_by_player(inst->object_index, TRUE);
				unit_set_controllable(inst->object_index, TRUE);
				if (is_held)
					forge_eject_all_passengers_from_vehicle(inst->object_index);
			}
			SET_FLAG(obj->object.flags, _object_outside_of_map_bit, FALSE);
			SET_FLAG(obj->object.flags, _object_no_collisions_bit, is_held ? TRUE : FALSE);
			if (wake_normal_physics)
			{
				/* Placed/released Normal-physics object: wake physics on BOTH Host and Clients so it settles naturally! */
				SET_FLAG(obj->object.flags, _object_at_rest_bit, FALSE);
				object_set_position(inst->object_index, &pos, &fwd, &up);
				object_set_automatic_deactivation(inst->object_index, FALSE);
				object_activate(inst->object_index);
				forge_ensure_object_connected_to_bsp(inst->object_index, x, y, z);
				obj->object.translational_velocity.i = 0.0f;
				obj->object.translational_velocity.j = 0.0f;
				obj->object.translational_velocity.k = -0.02f;
				obj->object.angular_velocity.i = 0.0f;
				obj->object.angular_velocity.j = 0.0f;
				obj->object.angular_velocity.k = 0.0f;
			}
			else
			{
				SET_FLAG(obj->object.flags, _object_at_rest_bit, TRUE);
				object_set_position(inst->object_index, &pos, &fwd, &up);
				object_set_automatic_deactivation(inst->object_index, FALSE);
				object_activate(inst->object_index);
				forge_ensure_object_connected_to_bsp(inst->object_index, x, y, z);
				obj->object.translational_velocity.i = 0.0f;
				obj->object.translational_velocity.j = 0.0f;
				obj->object.translational_velocity.k = 0.0f;
				obj->object.angular_velocity.i = 0.0f;
				obj->object.angular_velocity.j = 0.0f;
				obj->object.angular_velocity.k = 0.0f;
			}
		}
		else
		{
			inst->object_index = NONE;
		}
	}

	forge_sync_instance_extended_props_to_edit_state(live_idx);

	if (broadcast_net && forge_is_multiplayer_session())
	{
		forge_network_send_transform_ext(live_idx, inst->object_index, x, y, z, yaw, pitch, roll, is_held ? 1 : 0, 0);
	}
}

void forge_3d_move_instance(
	int32_t live_idx,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int broadcast_net)
{
	int is_held = (live_idx >= 0 && live_idx == g_forge.grabbed_live_index) ? 1 : 0;
	forge_3d_move_instance_ext(live_idx, x, y, z, yaw, pitch, roll, is_held, broadcast_net);
}

void forge_3d_delete_instance(int32_t live_idx, int broadcast_net)
{
	struct forge_live_instance *inst;
	long old_obj_idx;

	if (live_idx < 0 || live_idx >= g_forge.live_instance_count)
		return;
	inst = &g_forge.live_instances[live_idx];
	if (!inst->active)
		return;

	old_obj_idx = inst->object_index;
	if (inst->object_index != NONE)
	{
		if (forge_valid_object_get(inst->object_index))
			object_delete(inst->object_index);
		inst->object_index = NONE;
	}
	inst->active = 0;

	if (inst->group_tag == 'tele' || inst->group_tag == 'flag')
	{
		if (inst->is_default_scenario_object && inst->edit_entry_index < 0)
			inst->edit_entry_index = 0;
		sync_live_netgame_flags_to_scenario();
	}
	else if (inst->group_tag == 'spwn')
	{
		if (inst->is_default_scenario_object && inst->edit_entry_index < 0)
			inst->edit_entry_index = 0;
		sync_live_player_spawns_to_scenario();
	}
	else if (inst->is_default_scenario_object)
	{
		forge_ensure_default_object_edit_entry(inst);
		if (inst->edit_entry_index >= 0 && inst->edit_entry_index < g_forge.edit_state.default_edit_count)
		{
			g_forge.edit_state.default_edits[inst->edit_entry_index].deleted = 1;
		}
	}
	else if (inst->is_ai_unit && inst->edit_entry_index >= 0 &&
		inst->edit_entry_index < g_forge.edit_state.ai_spawn_count)
	{
		g_forge.edit_state.ai_spawns[inst->edit_entry_index].actor_variant_path[0] = 0;
	}
	else if (!inst->is_ai_unit && inst->edit_entry_index >= 0 &&
		inst->edit_entry_index < g_forge.edit_state.placed_object_count)
	{
		g_forge.edit_state.placed_objects[inst->edit_entry_index].active = 0;
		g_forge.edit_state.placed_objects[inst->edit_entry_index].x = -99999.0f;
	}

	forge_set_status("Deleted '%s'", inst->tag_path);

	if (broadcast_net && forge_is_multiplayer_session())
	{
		forge_network_send_delete_ext(live_idx, old_obj_idx);
	}
}

static int get_filtered_catalog_indices_for_cat(enum forge_palette_category cat, int *out_indices, int max_out)
{
	int i, count = 0;
	for (i = 0; i < g_forge.catalog_count && count < max_out; i++)
	{
		if (g_forge.catalog[i].category == (uint8_t)cat &&
			forge_catalog_entry_allowed(&g_forge.catalog[i]))
		{
			if (out_indices)
				out_indices[count] = i;
			count++;
		}
	}
	return count;
}

static int get_filtered_catalog_indices(int *out_indices, int max_out)
{
	return get_filtered_catalog_indices_for_cat(g_forge.current_category, out_indices, max_out);
}

extern void platform_translate_path(char const *xbox_path, char *host_path, unsigned long host_path_size);

static int s_save_modal_just_opened = 0;
static int s_save_confirm_overwrite = 0;
static int s_save_confirm_choice = 0; /* 0 = OVERWRITE, 1 = CHANGE NAME */
static int s_save_confirm_in_maps = 0;
static int s_save_confirm_in_custom_maps = 0;

static int forge_check_map_name_collision(
	char const *map_name,
	int *out_in_maps,
	int *out_in_custom_maps)
{
	static char const *const s_stock_maps[] = {
		"beavercreek", "bloodgulch", "boardingaction", "chillout", "chirp",
		"damnation", "dangercanyon", "deathisland", "gephyrophobia", "hangemhigh",
		"icefields", "infinity", "longest", "prisoner", "putput",
		"ratrace", "sidewinder", "timberland", "wizard", "ui",
		"a10", "a30", "a50", "b30", "b40", "c10", "c20", "c40", "d20", "d40"
	};
	int in_maps = 0;
	int in_custom = 0;
	char xbox_path[256];
	char host_path[512];
	FILE *f;
	size_t i;

	if (out_in_maps)
		*out_in_maps = 0;
	if (out_in_custom_maps)
		*out_in_custom_maps = 0;
	if (!map_name || !map_name[0])
		return 0;

	/* 1. Check standard maps/ folder (and built-in stock map names) */
	for (i = 0; i < sizeof(s_stock_maps) / sizeof(s_stock_maps[0]); i++)
	{
		if (_stricmp(map_name, s_stock_maps[i]) == 0)
		{
			in_maps = 1;
			break;
		}
	}
	if (!in_maps)
	{
		snprintf(xbox_path, sizeof(xbox_path), "d:\\maps\\%s.map", map_name);
		host_path[0] = 0;
		platform_translate_path(xbox_path, host_path, sizeof(host_path));
		f = host_path[0] ? fopen(host_path, "rb") : NULL;
		if (!f)
		{
			snprintf(host_path, sizeof(host_path), "maps/%s.map", map_name);
			f = fopen(host_path, "rb");
		}
		if (!f)
		{
			snprintf(xbox_path, sizeof(xbox_path), "d:\\mcc_maps\\%s.map", map_name);
			host_path[0] = 0;
			platform_translate_path(xbox_path, host_path, sizeof(host_path));
			f = host_path[0] ? fopen(host_path, "rb") : NULL;
		}
		if (f)
		{
			fclose(f);
			in_maps = 1;
		}
	}

	/* 2. Check custom_maps/ folder */
	snprintf(xbox_path, sizeof(xbox_path), "d:\\custom_maps\\%s.map", map_name);
	host_path[0] = 0;
	platform_translate_path(xbox_path, host_path, sizeof(host_path));
	f = host_path[0] ? fopen(host_path, "rb") : NULL;
	if (!f)
	{
		snprintf(host_path, sizeof(host_path), "custom_maps/%s.map", map_name);
		f = fopen(host_path, "rb");
	}
	if (!f)
	{
		snprintf(xbox_path, sizeof(xbox_path), "d:\\custom_maps\\%s.txt", map_name);
		host_path[0] = 0;
		platform_translate_path(xbox_path, host_path, sizeof(host_path));
		f = host_path[0] ? fopen(host_path, "rb") : NULL;
	}
	if (!f)
	{
		snprintf(host_path, sizeof(host_path), "custom_maps/%s.txt", map_name);
		f = fopen(host_path, "rb");
	}
	if (f)
	{
		fclose(f);
		in_custom = 1;
	}

	if (out_in_maps)
		*out_in_maps = in_maps;
	if (out_in_custom_maps)
		*out_in_custom_maps = in_custom;
	return (in_maps || in_custom) ? 1 : 0;
}

static void update_save_modal_input(void)
{
	static int prev_key[256];
	static int prev_gp_a = 0;
	static int prev_gp_b = 0;
	static int prev_gp_left = 0;
	static int prev_gp_right = 0;
	int vk;

	director_inhibit_input(0);
	director_inhibit_facing(0);

	if (s_save_modal_just_opened)
	{
		for (vk = 0; vk < 256; vk++)
			prev_key[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
		prev_gp_a = forge_gp_button_down(FORGE_GP_A);
		prev_gp_b = forge_gp_button_down(FORGE_GP_B);
		prev_gp_left = forge_gp_button_down(FORGE_GP_DPAD_LEFT) || forge_gp_button_down(FORGE_GP_DPAD_UP) || (s_gp_lx < -0.55f);
		prev_gp_right = forge_gp_button_down(FORGE_GP_DPAD_RIGHT) || forge_gp_button_down(FORGE_GP_DPAD_DOWN) || (s_gp_lx > 0.55f);
		s_save_confirm_overwrite = 0;
		s_save_confirm_choice = 0;
		s_save_confirm_in_maps = 0;
		s_save_confirm_in_custom_maps = 0;
		s_save_modal_just_opened = 0;
		return;
	}

	if (s_save_confirm_overwrite)
	{
		int left_down = ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0) ||
						((GetAsyncKeyState(VK_UP) & 0x8000) != 0);
		int right_down = ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0) ||
						 ((GetAsyncKeyState(VK_DOWN) & 0x8000) != 0) ||
						 ((GetAsyncKeyState(VK_TAB) & 0x8000) != 0);
		int gp_left_down = forge_gp_button_down(FORGE_GP_DPAD_LEFT) ||
						   forge_gp_button_down(FORGE_GP_DPAD_UP) ||
						   (s_gp_lx < -0.55f);
		int gp_right_down = forge_gp_button_down(FORGE_GP_DPAD_RIGHT) ||
							forge_gp_button_down(FORGE_GP_DPAD_DOWN) ||
							(s_gp_lx > 0.55f);
		int prev_nav_l = prev_key[VK_LEFT] || prev_key[VK_UP];
		int prev_nav_r = prev_key[VK_RIGHT] || prev_key[VK_DOWN] || prev_key[VK_TAB];
		int esc_down = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
		int bk_down = (GetAsyncKeyState(VK_BACK) & 0x8000) != 0;
		int ent_down = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
		int gp_a_down = forge_gp_button_down(FORGE_GP_A);
		int gp_b_down = forge_gp_button_down(FORGE_GP_B);

		if ((left_down && !prev_nav_l) || (gp_left_down && !prev_gp_left) ||
			(right_down && !prev_nav_r) || (gp_right_down && !prev_gp_right))
		{
			s_save_confirm_choice = 1 - s_save_confirm_choice;
		}

		if ((esc_down && !prev_key[VK_ESCAPE]) ||
			(bk_down && !prev_key[VK_BACK]) ||
			(gp_b_down && !prev_gp_b))
		{
			/* Return to map name input box to change the name */
			s_save_confirm_overwrite = 0;
		}
		else if ((ent_down && !prev_key[VK_RETURN]) || (gp_a_down && !prev_gp_a))
		{
			if (s_save_confirm_choice == 0)
			{
				s_save_confirm_overwrite = 0;
				forge_ui_save_copy_as(g_forge.save_map_name);
				g_forge.active_modal = _forge_modal_none;
			}
			else
			{
				/* Change Name selected: return to editing the map name */
				s_save_confirm_overwrite = 0;
			}
		}

		for (vk = 0; vk < 256; vk++)
			prev_key[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
		prev_gp_a = gp_a_down;
		prev_gp_b = gp_b_down;
		prev_gp_left = gp_left_down;
		prev_gp_right = gp_right_down;
		return;
	}

	for (vk = 'A'; vk <= 'Z'; vk++)
	{
		int down = (GetAsyncKeyState(vk) & 0x8000) != 0;
		if (down && !prev_key[vk] && g_forge.save_map_name_len < 48)
		{
			g_forge.save_map_name[g_forge.save_map_name_len++] = (char)(vk - 'A' + 'a');
			g_forge.save_map_name[g_forge.save_map_name_len] = 0;
		}
		prev_key[vk] = down;
	}
	for (vk = '0'; vk <= '9'; vk++)
	{
		int down = (GetAsyncKeyState(vk) & 0x8000) != 0;
		if (down && !prev_key[vk] && g_forge.save_map_name_len < 48)
		{
			g_forge.save_map_name[g_forge.save_map_name_len++] = (char)vk;
			g_forge.save_map_name[g_forge.save_map_name_len] = 0;
		}
		prev_key[vk] = down;
	}
	{
		int minus_down = (GetAsyncKeyState(VK_OEM_MINUS) & 0x8000) != 0;
		if (minus_down && !prev_key[VK_OEM_MINUS] && g_forge.save_map_name_len < 48)
		{
			g_forge.save_map_name[g_forge.save_map_name_len++] = '_';
			g_forge.save_map_name[g_forge.save_map_name_len] = 0;
		}
		prev_key[VK_OEM_MINUS] = minus_down;
	}
	{
		int bk_down = (GetAsyncKeyState(VK_BACK) & 0x8000) != 0;
		if (bk_down && !prev_key[VK_BACK] && g_forge.save_map_name_len > 0)
		{
			g_forge.save_map_name[--g_forge.save_map_name_len] = 0;
		}
		prev_key[VK_BACK] = bk_down;
	}
	{
		int esc_down = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
		int gp_b_down = forge_gp_button_down(FORGE_GP_B);
		if ((esc_down && !prev_key[VK_ESCAPE]) || (gp_b_down && !prev_gp_b))
		{
			g_forge.active_modal = _forge_modal_none;
			prev_key[VK_ESCAPE] = esc_down;
			prev_gp_b = gp_b_down;
			return;
		}
		prev_key[VK_ESCAPE] = esc_down;
		prev_gp_b = gp_b_down;
	}
	{
		int ent_down = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
		int gp_a_down = forge_gp_button_down(FORGE_GP_A);
		if ((ent_down && !prev_key[VK_RETURN]) || (gp_a_down && !prev_gp_a))
		{
			if (!g_forge.save_map_name[0])
			{
				char const *cur = forge_get_active_map_name();
				char const *slash = cur ? strrchr(cur, '\\') : NULL;
				char const *slash2 = cur ? strrchr(cur, '/') : NULL;
				if (slash2 && (!slash || slash2 > slash))
					slash = slash2;
				snprintf(g_forge.save_map_name, sizeof(g_forge.save_map_name), "%s_forge",
					slash ? (slash + 1) : (cur && cur[0] ? cur : "custom"));
				g_forge.save_map_name_len = (int)strlen(g_forge.save_map_name);
			}
			if (forge_check_map_name_collision(
					g_forge.save_map_name,
					&s_save_confirm_in_maps,
					&s_save_confirm_in_custom_maps))
			{
				s_save_confirm_overwrite = 1;
				s_save_confirm_choice = 0;
			}
			else
			{
				forge_ui_save_copy_as(g_forge.save_map_name);
				g_forge.active_modal = _forge_modal_none;
			}
		}
		prev_key[VK_RETURN] = ent_down;
		prev_gp_a = gp_a_down;
	}
	prev_gp_left = forge_gp_button_down(FORGE_GP_DPAD_LEFT) || forge_gp_button_down(FORGE_GP_DPAD_UP) || (s_gp_lx < -0.55f);
	prev_gp_right = forge_gp_button_down(FORGE_GP_DPAD_RIGHT) || forge_gp_button_down(FORGE_GP_DPAD_DOWN) || (s_gp_lx > 0.55f);
}

static int find_valid_bsp_spawn_point(real_point3d *out_pos, float *out_yaw)
{
	struct scenario *scen = global_scenario_try_and_get();
	long i;

	if (!scen)
		return 0;

	/* 1. Standard scenario player starting locations inside the active BSP */
	for (i = 0; i < scen->players.count; i++)
	{
		struct encounter_player_starting_location *loc = TAG_BLOCK_GET_ELEMENT(
			&scen->players, i, struct encounter_player_starting_location);
		real_point3d test_pt = loc->position;
		test_pt.z += 0.3f;
		if (scenario_leaf_index_from_point(&test_pt) != NONE)
		{
			*out_pos = loc->position;
			*out_yaw = loc->facing;
			return 1;
		}
	}

	/* 2. Scenario cutscene flags inside the active BSP (useful on SP maps whose start BSP has no player spawn) */
	for (i = 0; i < scen->cutscene_flags.count; i++)
	{
		struct scenario_cutscene_flag *flg = TAG_BLOCK_GET_ELEMENT(
			&scen->cutscene_flags, i, struct scenario_cutscene_flag);
		real_point3d test_pt = flg->position;
		test_pt.z += 0.3f;
		if (scenario_leaf_index_from_point(&test_pt) != NONE)
		{
			*out_pos = flg->position;
			*out_yaw = flg->facing.yaw;
			return 1;
		}
	}

	/* 3. Any placed object inside the active BSP */
	{
		struct object_iterator iter;
		struct object_datum *obj;
		object_iterator_new(&iter, (unsigned long)-1, 0);
		while ((obj = object_iterator_next(&iter)) != NULL)
		{
			real_point3d test_pt = obj->object.position;
			test_pt.z += 0.5f;
			if (scenario_leaf_index_from_point(&test_pt) != NONE)
			{
				*out_pos = test_pt;
				*out_yaw = 0.0f;
				return 1;
			}
		}
	}

	/* 3b. Any upward-facing surface centroid inside the active structure_bsp (for multi-BSP campaign levels) */
	{
		struct structure_bsp const *bsp = global_structure_bsp_get();
		if (bsp)
		{
			long lm_idx, mat_idx;
			for (lm_idx = 0; lm_idx < bsp->lightmaps.count; lm_idx++)
			{
				struct structure_lightmap const *lm = TAG_BLOCK_GET_ELEMENT(
					&bsp->lightmaps, lm_idx, struct structure_lightmap);
				for (mat_idx = 0; mat_idx < lm->materials.count; mat_idx++)
				{
					struct structure_material const *mat = TAG_BLOCK_GET_ELEMENT(
						&lm->materials, mat_idx, struct structure_material);
					if (mat->plane.n.k > 0.5f)
					{
						real_point3d test_pt;
						test_pt.x = mat->centroid.x + mat->plane.n.i * 0.45f;
						test_pt.y = mat->centroid.y + mat->plane.n.j * 0.45f;
						test_pt.z = mat->centroid.z + mat->plane.n.k * 0.45f;
						if (scenario_leaf_index_from_point(&test_pt) != NONE)
						{
							*out_pos = test_pt;
							*out_yaw = 0.0f;
							return 1;
						}
					}
				}
			}
		}
	}

	/* 4. Fallback to first player starting location even if leaf test failed */
	if (scen->players.count > 0)
	{
		struct encounter_player_starting_location *loc = TAG_BLOCK_GET_ELEMENT(
			&scen->players, 0, struct encounter_player_starting_location);
		*out_pos = loc->position;
		*out_yaw = loc->facing;
		return 1;
	}
	return 0;
}

void forge_init_map_scenario_entities(void)
{
	struct scenario *scen = global_scenario_try_and_get();
	long i;

	if (!scen)
		return;

	if (g_forge.live_instance_count == 0)
	{
		if (scen == s_orig_sp_scen && scen->players.address == s_live_sp_buf && s_orig_sp_addr)
		{
			scen->players.address = s_orig_sp_addr;
			scen->players.count = s_orig_sp_count;
		}
		if (scen == s_orig_nf_scen && scen->netgame_flags.address == s_live_nf_buf && s_orig_nf_addr)
		{
			scen->netgame_flags.address = s_orig_nf_addr;
			scen->netgame_flags.count = s_orig_nf_count;
		}
	}

	/* 1. Load existing scenario player starting locations (scen->players) into g_forge.live_instances
	 *    so Red Team, Blue Team, and FFA player spawns appear as coloured ground circles + facing arrows
	 *    and can be grabbed, moved, rotated, deleted, or team-edited on any map! */
	for (i = 0; i < scen->players.count && g_forge.live_instance_count < FORGE_MAX_LIVE_INSTANCES; i++)
	{
		struct player_starting_location *loc = TAG_BLOCK_GET_ELEMENT(
			&scen->players, i, struct player_starting_location);
		struct forge_live_instance *inst;
		int16_t t0 = loc->game_types[0];
		int16_t t1 = loc->game_types[1];
		int16_t team_desig;
		char const *label_path;

		if (t0 == 13 /* _game_engine_all_non_team (FFA) */ ||
			(t0 == 0 && t1 == 0) ||
			(loc->team_index < 0 || loc->team_index >= 2))
		{
			team_desig = 2; /* FFA Spawn - Yellow */
			label_path = "spawn\\player_ffa";
		}
		else if (loc->team_index == 1)
		{
			team_desig = 1; /* Blue Team Spawn - Blue */
			label_path = "spawn\\player_blue";
		}
		else
		{
			team_desig = 0; /* Red Team Spawn - Red */
			label_path = "spawn\\player_red";
		}

		inst = &g_forge.live_instances[g_forge.live_instance_count++];
		memset(inst, 0, sizeof(*inst));
		inst->active = 1;
		inst->group_tag = 'spwn';
		inst->source_cache_id = 0;
		inst->is_default_scenario_object = 1;
		inst->pad_inst = (uint8_t)(loc->structure_bsp_reference_index >= 0 ? loc->structure_bsp_reference_index : 0);
		inst->edit_entry_index = -1;
		inst->netgame_flag_type = t0;
		inst->team_designator = team_desig;
		inst->gametype_flags = 0x0FFFu;
		snprintf(inst->tag_path, sizeof(inst->tag_path), "%s", label_path);
		inst->orig_x = loc->position.x;
		inst->orig_y = loc->position.y;
		inst->orig_z = loc->position.z;
		inst->x = loc->position.x;
		inst->y = loc->position.y;
		inst->z = loc->position.z;
		inst->yaw = loc->facing;
		inst->object_index = NONE;
	}

	/* 2. Load existing scenario netgame_flags (Teleporters In/Out, KOTH Hills, Oddballs, CTF Flags, Race Nodes)
	 *    into g_forge.live_instances so they can be grabbed, moved, rotated, deleted, and linked via Reference ID! */
	for (i = 0; i < scen->netgame_flags.count && g_forge.live_instance_count < FORGE_MAX_LIVE_INSTANCES; i++)
	{
		struct scenario_netgame_flag *flg = TAG_BLOCK_GET_ELEMENT(
			&scen->netgame_flags, i, struct scenario_netgame_flag);
		struct forge_live_instance *inst;
		char const *label_path = NULL;
		uint32_t grp = 'flag';

		if (flg->type == _netgame_flag_teleporter_source)
		{
			grp = 'tele';
			label_path = "teleporter\\origin";
		}
		else if (flg->type == _netgame_flag_teleporter_target)
		{
			grp = 'tele';
			label_path = "teleporter\\destination";
		}
		else if (flg->type == _netgame_flag_hill)
		{
			grp = 'flag';
			label_path = "objective\\koth_hill";
		}
		else if (flg->type == _netgame_flag_oddball_ball_spawn)
		{
			grp = 'flag';
			label_path = "objective\\oddball_spawn";
		}
		else if (flg->type == _netgame_flag_ctf_flag)
		{
			grp = 'flag';
			label_path = (flg->team_index == 1) ? "objective\\ctf_flag_blue" : "objective\\ctf_flag_red";
		}
		else if (flg->type == _netgame_flag_ctf_vehicle)
		{
			grp = 'flag';
			label_path = (flg->team_index == 1) ? "objective\\ctf_vehicle_blue" : "objective\\ctf_vehicle_red";
		}
		else if (flg->type == _netgame_flag_race_track)
		{
			grp = 'flag';
			label_path = "objective\\race_track";
		}
		else if (flg->type == _netgame_flag_race_vehicle)
		{
			grp = 'flag';
			label_path = "objective\\race_vehicle";
		}
		else
		{
			grp = 'flag';
			label_path = "objective\\netgame_flag";
		}

		inst = &g_forge.live_instances[g_forge.live_instance_count++];
		memset(inst, 0, sizeof(*inst));
		inst->active = 1;
		inst->group_tag = grp;
		inst->source_cache_id = 0;
		inst->is_default_scenario_object = 1;
		inst->edit_entry_index = -1;
		inst->netgame_flag_type = flg->type;
		inst->team_designator = flg->team_index;
		inst->gametype_flags = 0x0FFFu;
		snprintf(inst->tag_path, sizeof(inst->tag_path), "%s", label_path);
		inst->orig_x = flg->position.x;
		inst->orig_y = flg->position.y;
		inst->orig_z = flg->position.z;
		inst->x = flg->position.x;
		inst->y = flg->position.y;
		inst->z = flg->position.z;
		inst->yaw = flg->facing;
		inst->object_index = NONE;
	}

	/* 3. Deterministically register all default non-networked scenario objects (scenery, machines,
	 *    controls, light fixtures) in object_header_data slot order so Host and Clients share the
	 *    exact same live_idx for every in-map scenery/device object before any player edits occur! */
	if (object_header_data)
	{
		unsigned long default_mask =
			_object_mask_scenery |
			_object_mask_machine |
			_object_mask_control |
			_object_mask_light_fixture;
		long o_idx;
		for (o_idx = data_next_index(object_header_data, NONE);
			 o_idx != NONE && g_forge.live_instance_count < FORGE_MAX_LIVE_INSTANCES;
			 o_idx = data_next_index(object_header_data, o_idx))
		{
			struct object_datum *obj = (struct object_datum *)object_try_and_get_and_verify_type(o_idx, default_mask);
			struct forge_live_instance *inst;
			long def_idx;
			uint32_t grp;
			char const *tname;
			int already = 0, k;

			if (!obj || obj->object.parent_object_index != NONE)
				continue;

			for (k = 0; k < g_forge.live_instance_count; k++)
			{
				if (g_forge.live_instances[k].active && g_forge.live_instances[k].object_index == o_idx)
				{
					already = 1;
					break;
				}
			}
			if (already)
				continue;

			def_idx = obj->definition_index;
			grp = (def_idx != NONE) ? (uint32_t)tag_get_group_tag(def_idx) : 'scen';
			tname = (def_idx != NONE) ? tag_get_name(def_idx) : "scenario_object";
			if (!tname || !tname[0])
				tname = "scenario_object";

			inst = &g_forge.live_instances[g_forge.live_instance_count++];
			memset(inst, 0, sizeof(*inst));
			inst->active = 1;
			inst->object_index = o_idx;
			inst->actor_index = NONE;
			inst->edit_entry_index = -1;
			inst->group_tag = grp;
			inst->source_cache_id = 0;
			inst->is_default_scenario_object = 1;
			inst->spawned_once = 1;
			inst->death_tick = -1;
			inst->orig_x = obj->object.position.x;
			inst->orig_y = obj->object.position.y;
			inst->orig_z = obj->object.position.z;
			inst->x = obj->object.position.x;
			inst->y = obj->object.position.y;
			inst->z = obj->object.position.z;
			forward_up_to_euler(&obj->object.forward, &obj->object.up, &inst->yaw, &inst->pitch, &inst->roll);
			snprintf(inst->tag_path, sizeof(inst->tag_path), "%s", tname);
			forge_snap_to_scenario_placement_origin(inst->group_tag, inst->tag_path, &inst->orig_x, &inst->orig_y, &inst->orig_z);
		}
	}

	/* 2. Points 7 & 13: In Single Player mode (!game_engine_running()), the engine skips spawning
	 *    netgame_equipment (weapons/grenades/powerups) and multiplayer-flagged vehicles.
	 *    Force every weapon, item, and vehicle present on the map to spawn! */
	if (!game_engine_running())
	{
		for (i = 0; i < scen->netgame_equipment.count; i++)
		{
			struct scenario_netgame_equipment *eq = TAG_BLOCK_GET_ELEMENT(
				&scen->netgame_equipment, i, struct scenario_netgame_equipment);
			long item_def_idx = NONE;

			if (eq->item_collection.index != NONE)
			{
				uint8_t const *itmc = (uint8_t const *)tag_get('itmc', eq->item_collection.index);
				if (itmc)
				{
					int32_t perm_count = *(int32_t const *)(itmc + 0x00);
					uint8_t const *perms = *(uint8_t const *const *)(itmc + 0x04);
					if (perm_count > 0 && perms)
					{
						/* item_permutation is 84 bytes; item tag_reference is at +0x24 (index at +0x30) */
						item_def_idx = *(int32_t const *)(perms + 0x30);
					}
				}
			}

			if (item_def_idx != NONE)
			{
				struct object_placement_data pdata;
				long oidx;
				object_placement_data_new(&pdata, item_def_idx, NONE);
				pdata.position = eq->position;
				pdata.position.z += 0.08f;
				euler_to_forward_up(eq->facing, 0.0f, 0.0f, &pdata.forward, &pdata.up);
				oidx = object_new(&pdata);
				if (oidx != NONE)
				{
					object_set_automatic_deactivation(oidx, FALSE);
					object_activate(oidx);
				}
			}
		}

		/* Also ensure all scenario vehicles are spawned if the single-player game skipped any MP-flagged ones */
		for (i = 0; i < scen->vehicles.count; i++)
		{
			struct scenario_object_datum *sv = (struct scenario_object_datum *)tag_block_get_element_fast(
				&scen->vehicles, i, 120);
			if (sv && sv->palette_entry_index >= 0 && sv->palette_entry_index < scen->vehicle_palette.count)
			{
				struct scenario_object_palette_entry *pal = TAG_BLOCK_GET_ELEMENT(
					&scen->vehicle_palette, sv->palette_entry_index, struct scenario_object_palette_entry);
				if (pal->reference.index != NONE)
				{
					/* Check if a vehicle already exists near this scenario spawn (vehicle_new adds +bounding_radius*0.5f to Z) */
					struct object_iterator viter;
					struct object_datum *vobj;
					int already_spawned = 0;
					object_iterator_new(&viter, _object_mask_vehicle, 0);
					while ((vobj = object_iterator_next(&viter)) != NULL)
					{
						float dx = vobj->object.position.x - sv->position.x;
						float dy = vobj->object.position.y - sv->position.y;
						float dz = vobj->object.position.z - sv->position.z;
						float max_dz = (vobj->object.bounding_sphere_radius > 2.0f) ?
							(vobj->object.bounding_sphere_radius + 4.0f) : 6.0f;
						if (dx * dx + dy * dy < 4.0f && (float)fabs((double)dz) < max_dz)
						{
							already_spawned = 1;
							break;
						}
					}
					if (!already_spawned)
					{
						struct object_placement_data pdata;
						long v_idx;
						object_placement_data_new(&pdata, pal->reference.index, NONE);
						pdata.position = sv->position;
						pdata.position.z += 0.12f;
						euler_to_forward_up(sv->rotation.yaw, sv->rotation.pitch, sv->rotation.roll, &pdata.forward, &pdata.up);
						v_idx = object_new(&pdata);
						if (v_idx != NONE)
						{
							object_set_automatic_deactivation(v_idx, FALSE);
							object_activate(v_idx);
						}
					}
				}
			}
		}
	}
}

static int count_active_instances_for_catalog_entry(struct forge_catalog_entry const *ce)
{
	int i, count = 0;
	if (!ce || !ce->tag_path[0])
		return 0;
	for (i = 0; i < g_forge.live_instance_count; i++)
	{
		if (g_forge.live_instances[i].active &&
			g_forge.live_instances[i].group_tag == ce->group_tag &&
			_stricmp(g_forge.live_instances[i].tag_path, ce->tag_path) == 0)
		{
			count++;
		}
	}
	return count;
}

static void format_clean_uppercase_item_name(char const *display_name, char *out_buf, size_t out_sz)
{
	char const *p = display_name ? display_name : "";
	size_t i = 0;
	if (!out_buf || out_sz == 0)
		return;
	/* Preserve "[c] " custom tag cache badge and "[map] " map-local tag badge while stripping generic "[WEAP] ", "[VEHI] ", "[SPAWN] ", "[KOTH] " prefixes */
	if (_strnicmp(p, "[c]", 3) == 0)
	{
		p += 3;
		while (*p == ' ')
			p++;
		if (out_sz >= 5)
		{
			out_buf[0] = '[';
			out_buf[1] = 'c';
			out_buf[2] = ']';
			out_buf[3] = ' ';
			i = 4;
		}
	}
	else if (_strnicmp(p, "[map]", 5) == 0)
	{
		p += 5;
		while (*p == ' ')
			p++;
		if (out_sz >= 7)
		{
			out_buf[0] = '[';
			out_buf[1] = 'M';
			out_buf[2] = 'A';
			out_buf[3] = 'P';
			out_buf[4] = ']';
			out_buf[5] = ' ';
			i = 6;
		}
	}
	else if (p[0] == '[')
	{
		char const *rb = strchr(p, ']');
		if (rb)
		{
			p = rb + 1;
			while (*p == ' ')
				p++;
		}
	}
	while (*p && i + 1 < out_sz)
	{
		char ch = *p++;
		if (ch >= 'a' && ch <= 'z')
			ch = (char)(ch - 'a' + 'A');
		out_buf[i++] = ch;
	}
	out_buf[i] = '\0';
}

static char const *gametype_flags_label(uint16_t flags)
{
	if (flags == 0x0001u) return "CTF Only";
	if (flags == 0x0002u) return "Slayer Only";
	if (flags == 0x0004u) return "Oddball Only";
	if (flags == 0x0008u) return "King of the Hill";
	if (flags == 0x0010u) return "Race Only";
	return "All Modes (Default)";
}

static uint16_t cycle_gametype_flags(uint16_t flags, int dir)
{
	static uint16_t const modes[6] = { 0x0FFFu, 0x0001u, 0x0002u, 0x0004u, 0x0008u, 0x0010u };
	int i, cur = 0;
	for (i = 0; i < 6; i++)
	{
		if (modes[i] == flags)
		{
			cur = i;
			break;
		}
	}
	cur = (cur + 6 + dir) % 6;
	return modes[cur];
}

static void forge_sync_live_instance_from_engine_object(struct forge_live_instance *inst)
{
	struct object_datum *obj;
	real_vector3d efwd, eup;
	float df, du;

	if (!inst || inst->object_index == NONE)
		return;
	obj = forge_valid_object_get(inst->object_index);
	if (!obj)
		return;
	inst->x = obj->object.position.x;
	inst->y = obj->object.position.y;
	inst->z = obj->object.position.z;
	euler_to_forward_up(inst->yaw, inst->pitch, inst->roll, &efwd, &eup);
	df = (efwd.i - obj->object.forward.i) * (efwd.i - obj->object.forward.i) +
		(efwd.j - obj->object.forward.j) * (efwd.j - obj->object.forward.j) +
		(efwd.k - obj->object.forward.k) * (efwd.k - obj->object.forward.k);
	du = (eup.i - obj->object.up.i) * (eup.i - obj->object.up.i) +
		(eup.j - obj->object.up.j) * (eup.j - obj->object.up.j) +
		(eup.k - obj->object.up.k) * (eup.k - obj->object.up.k);
	if (df > 0.0004f || du > 0.0004f)
		forward_up_to_euler(&obj->object.forward, &obj->object.up, &inst->yaw, &inst->pitch, &inst->roll);
}

/* =========================================================================
 * WELD GROUPS (Group 1..16) & TRANSACTION-BASED UNDO / REDO STACK (Ctrl+Z/Y)
 * ========================================================================= */

static void forge_weld_clear_followers(void)
{
	s_weld_follower_count = 0;
}

static int forge_weld_count_group_members(uint8_t gid)
{
	int i, cnt = 0;
	if (gid == 0 || gid > FORGE_MAX_WELD_GROUPS)
		return 0;
	for (i = 0; i < g_forge.live_instance_count; i++)
	{
		if (g_forge.live_instances[i].active && g_forge.live_instances[i].group_id == gid)
			cnt++;
	}
	return cnt;
}

static uint8_t forge_weld_find_unused_group_id(uint8_t fallback_gid)
{
	uint8_t cand;
	for (cand = 1; cand <= FORGE_MAX_WELD_GROUPS; cand++)
	{
		if (forge_weld_count_group_members(cand) == 0)
			return cand;
	}
	return (fallback_gid >= 1 && fallback_gid <= FORGE_MAX_WELD_GROUPS) ? fallback_gid : 1;
}

static void forge_weld_capture_followers(int32_t pivot_live_idx)
{
	struct forge_live_instance *piv;
	real_vector3d pf, pu, pl;
	int i;

	s_weld_follower_count = 0;
	if (pivot_live_idx < 0 || pivot_live_idx >= g_forge.live_instance_count)
		return;
	piv = &g_forge.live_instances[pivot_live_idx];
	if (!piv->active || piv->group_id == 0 || piv->group_id > FORGE_MAX_WELD_GROUPS)
		return;

	euler_to_forward_up(piv->yaw, piv->pitch, piv->roll, &pf, &pu);
	/* Orthonormal Left = Up x Forward */
	pl.i = pu.j * pf.k - pu.k * pf.j;
	pl.j = pu.k * pf.i - pu.i * pf.k;
	pl.k = pu.i * pf.j - pu.j * pf.i;

	for (i = 0; i < g_forge.live_instance_count && s_weld_follower_count < FORGE_MAX_WELD_FOLLOWERS; i++)
	{
		struct forge_live_instance *ch;
		struct forge_weld_follower_state *wf;
		struct object_datum *cobj;
		real_vector3d cf, cu;
		float dx, dy, dz;

		if (i == pivot_live_idx)
			continue;
		ch = &g_forge.live_instances[i];
		if (!ch->active || ch->group_id != piv->group_id)
			continue;

		forge_sync_live_instance_from_engine_object(ch);
		cobj = forge_valid_object_get(ch->object_index);
		if (cobj && cobj->object.type == _object_type_vehicle)
			forge_eject_all_passengers_from_vehicle(ch->object_index);

		euler_to_forward_up(ch->yaw, ch->pitch, ch->roll, &cf, &cu);
		dx = ch->x - piv->x;
		dy = ch->y - piv->y;
		dz = ch->z - piv->z;

		wf = &s_weld_followers[s_weld_follower_count++];
		wf->live_idx = i;
		wf->orig_x = ch->x;
		wf->orig_y = ch->y;
		wf->orig_z = ch->z;
		wf->orig_yaw = ch->yaw;
		wf->orig_pitch = ch->pitch;
		wf->orig_roll = ch->roll;

		wf->rel_pos_f = dx * pf.i + dy * pf.j + dz * pf.k;
		wf->rel_pos_l = dx * pl.i + dy * pl.j + dz * pl.k;
		wf->rel_pos_u = dx * pu.i + dy * pu.j + dz * pu.k;

		wf->rel_fwd_f = cf.i * pf.i + cf.j * pf.j + cf.k * pf.k;
		wf->rel_fwd_l = cf.i * pl.i + cf.j * pl.j + cf.k * pl.k;
		wf->rel_fwd_u = cf.i * pu.i + cf.j * pu.j + cf.k * pu.k;

		wf->rel_up_f = cu.i * pf.i + cu.j * pf.j + cu.k * pf.k;
		wf->rel_up_l = cu.i * pl.i + cu.j * pl.j + cu.k * pl.k;
		wf->rel_up_u = cu.i * pu.i + cu.j * pu.j + cu.k * pu.k;
	}
}

static void forge_weld_update_followers(
	int32_t pivot_live_idx,
	int is_held,
	int broadcast_net,
	int force_immediate_net)
{
	struct forge_live_instance const *piv;
	real_vector3d pf, pu, pl;
	unsigned long now_ms;
	int send_net_now = 0;
	int w;

	if (s_weld_follower_count <= 0 ||
		pivot_live_idx < 0 || pivot_live_idx >= g_forge.live_instance_count)
		return;
	piv = &g_forge.live_instances[pivot_live_idx];
	if (!piv->active)
		return;

	euler_to_forward_up(piv->yaw, piv->pitch, piv->roll, &pf, &pu);
	pl.i = pu.j * pf.k - pu.k * pf.j;
	pl.j = pu.k * pf.i - pu.i * pf.k;
	pl.k = pu.i * pf.j - pu.j * pf.i;

	if (broadcast_net && forge_is_multiplayer_session())
	{
		now_ms = system_milliseconds();
		if (force_immediate_net || !is_held || (now_ms - s_last_weld_net_send_ms >= 100u))
		{
			send_net_now = 1;
			s_last_weld_net_send_ms = now_ms;
		}
	}

	for (w = 0; w < s_weld_follower_count; w++)
	{
		struct forge_weld_follower_state const *wf = &s_weld_followers[w];
		struct forge_live_instance *ch;
		real_vector3d nfwd, nup;
		float nx, ny, nz, nyaw, npitch, nroll;

		if (wf->live_idx < 0 || wf->live_idx >= g_forge.live_instance_count)
			continue;
		ch = &g_forge.live_instances[wf->live_idx];
		if (!ch->active)
			continue;

		nx = piv->x + wf->rel_pos_f * pf.i + wf->rel_pos_l * pl.i + wf->rel_pos_u * pu.i;
		ny = piv->y + wf->rel_pos_f * pf.j + wf->rel_pos_l * pl.j + wf->rel_pos_u * pu.j;
		nz = piv->z + wf->rel_pos_f * pf.k + wf->rel_pos_l * pl.k + wf->rel_pos_u * pu.k;

		nfwd.i = wf->rel_fwd_f * pf.i + wf->rel_fwd_l * pl.i + wf->rel_fwd_u * pu.i;
		nfwd.j = wf->rel_fwd_f * pf.j + wf->rel_fwd_l * pl.j + wf->rel_fwd_u * pu.j;
		nfwd.k = wf->rel_fwd_f * pf.k + wf->rel_fwd_l * pl.k + wf->rel_fwd_u * pu.k;

		nup.i = wf->rel_up_f * pf.i + wf->rel_up_l * pl.i + wf->rel_up_u * pu.i;
		nup.j = wf->rel_up_f * pf.j + wf->rel_up_l * pl.j + wf->rel_up_u * pu.j;
		nup.k = wf->rel_up_f * pf.k + wf->rel_up_l * pl.k + wf->rel_up_u * pu.k;

		forward_up_to_euler(&nfwd, &nup, &nyaw, &npitch, &nroll);
		if (ch->group_tag == 'spwn' || ch->group_tag == 'tele' ||
			ch->group_tag == 'flag' || ch->is_ai_unit)
		{
			npitch = 0.0f;
			nroll = 0.0f;
		}

		forge_3d_move_instance_ext(wf->live_idx, nx, ny, nz, nyaw, npitch, nroll, is_held, 0);
		if (send_net_now)
		{
			forge_network_send_transform_ext(
				wf->live_idx, ch->object_index,
				nx, ny, nz, nyaw, npitch, nroll,
				is_held ? 1 : 0, 1);
		}
	}
}

static void forge_weld_restore_followers_orig(void)
{
	int w;
	int send_net = forge_is_multiplayer_session();
	for (w = 0; w < s_weld_follower_count; w++)
	{
		struct forge_weld_follower_state const *wf = &s_weld_followers[w];
		if (wf->live_idx >= 0 && wf->live_idx < g_forge.live_instance_count &&
			g_forge.live_instances[wf->live_idx].active)
		{
			struct forge_live_instance *ch = &g_forge.live_instances[wf->live_idx];
			forge_3d_move_instance_ext(
				wf->live_idx,
				wf->orig_x, wf->orig_y, wf->orig_z,
				wf->orig_yaw, wf->orig_pitch, wf->orig_roll,
				0, 0);
			if (send_net)
			{
				forge_network_send_transform_ext(
					wf->live_idx, ch->object_index,
					wf->orig_x, wf->orig_y, wf->orig_z,
					wf->orig_yaw, wf->orig_pitch, wf->orig_roll,
					0, 1);
			}
		}
	}
	s_weld_follower_count = 0;
}

#define FORGE_UNDO_MAX_RECORDS 128

enum forge_undo_op_type
{
	_undo_op_spawn = 1,
	_undo_op_delete = 2,
	_undo_op_transform = 3,
	_undo_op_props = 4
};

struct forge_undo_record
{
	uint32_t tx_id;
	uint8_t op_type;
	uint8_t pad[3];
	int32_t slot_idx;
	struct forge_live_instance before_snap;
	struct forge_live_instance after_snap;
};

static struct forge_undo_record s_undo_records[FORGE_UNDO_MAX_RECORDS];
static int s_undo_count = 0;
static int s_undo_top = 0;
static uint32_t s_undo_next_tx = 1;

void forge_undo_clear(void)
{
	s_undo_count = 0;
	s_undo_top = 0;
	s_undo_next_tx = 1;
	s_weld_follower_count = 0;
}

static uint32_t forge_undo_begin_tx(void)
{
	uint32_t id = s_undo_next_tx++;
	if (s_undo_next_tx == 0)
		s_undo_next_tx = 1;
	return id;
}

static void forge_undo_push_record(
	uint32_t tx_id,
	uint8_t op_type,
	int32_t slot_idx,
	struct forge_live_instance const *before_s,
	struct forge_live_instance const *after_s)
{
	struct forge_undo_record *rec;

	if (slot_idx < 0 || slot_idx >= FORGE_MAX_LIVE_INSTANCES)
		return;

	/* Truncate any redo history beyond current undo top */
	if (s_undo_top < s_undo_count)
		s_undo_count = s_undo_top;

	/* If buffer is full, drop the oldest transaction cleanly */
	if (s_undo_count >= FORGE_UNDO_MAX_RECORDS)
	{
		uint32_t drop_tx = s_undo_records[0].tx_id;
		int drop_n = 1;
		while (drop_n < s_undo_count && s_undo_records[drop_n].tx_id == drop_tx)
			drop_n++;
		if (drop_n < s_undo_count)
		{
			memmove(&s_undo_records[0], &s_undo_records[drop_n],
				(size_t)(s_undo_count - drop_n) * sizeof(struct forge_undo_record));
		}
		s_undo_count -= drop_n;
		s_undo_top = s_undo_count;
	}

	rec = &s_undo_records[s_undo_count++];
	s_undo_top = s_undo_count;
	memset(rec, 0, sizeof(*rec));
	rec->tx_id = tx_id;
	rec->op_type = op_type;
	rec->slot_idx = slot_idx;
	if (before_s)
		rec->before_snap = *before_s;
	if (after_s)
		rec->after_snap = *after_s;
}

static void forge_copy_extended_props_between_instances(
	struct forge_live_instance *dst,
	struct forge_live_instance const *src)
{
	if (!dst || !src)
		return;
	dst->gametype_flags = src->gametype_flags;
	dst->team_designator = src->team_designator;
	if (src->netgame_flag_type >= 0)
		dst->netgame_flag_type = src->netgame_flag_type;
	dst->physics_mode = src->physics_mode;
	dst->ai_order = src->ai_order;
	dst->group_id = src->group_id;
	dst->light_color_preset = src->light_color_preset;
	dst->light_intensity_preset = src->light_intensity_preset;
	dst->light_radius_preset = src->light_radius_preset;
	dst->respawn_timer_sec = src->respawn_timer_sec;
}

static void forge_apply_props_and_transform_snapshot(
	int32_t slot_idx,
	struct forge_live_instance const *snap)
{
	struct forge_live_instance *inst;

	if (!snap || slot_idx < 0 || slot_idx >= g_forge.live_instance_count)
		return;
	inst = &g_forge.live_instances[slot_idx];
	if (!inst->active)
		return;

	forge_copy_extended_props_between_instances(inst, snap);
	inst->death_tick = -1;
	forge_3d_move_instance_ext(
		slot_idx,
		snap->x, snap->y, snap->z,
		snap->yaw, snap->pitch, snap->roll,
		0, 0);
	forge_apply_instance_light_settings(slot_idx);
	if (inst->is_ai_unit && game_connection() != _game_connection_network_client)
	{
		forge_ai_update_unit_team(inst->actor_index, inst->object_index, inst->team_designator);
		if (inst->ai_order != 0)
		{
			forge_ai_apply_order(
				inst->actor_index, inst->object_index, inst->ai_order,
				inst->x, inst->y, inst->z, game_time_get());
		}
	}
	forge_sync_instance_extended_props_to_edit_state(slot_idx);
	if (forge_is_multiplayer_session())
	{
		forge_network_send_transform_ext(
			slot_idx, inst->object_index,
			inst->x, inst->y, inst->z,
			inst->yaw, inst->pitch, inst->roll,
			0, 1);
		forge_network_send_object_props(slot_idx);
	}
}

static void forge_restore_snapshot_at_slot(
	int32_t slot_idx,
	struct forge_live_instance const *snap)
{
	struct forge_live_instance *inst;

	if (!snap || slot_idx < 0 || slot_idx >= FORGE_MAX_LIVE_INSTANCES)
		return;

	if (snap->is_default_scenario_object &&
		snap->group_tag != 'spwn' &&
		snap->group_tag != 'tele' &&
		snap->group_tag != 'flag' &&
		snap->edit_entry_index >= 0 &&
		snap->edit_entry_index < (int32_t)g_forge.edit_state.default_edit_count)
	{
		uint32_t actual_grp = snap->group_tag;
		long def_idx = forge_resolve_engine_tag(snap->group_tag, snap->tag_path, &actual_grp);
		int is_net_obj_type = (snap->group_tag == 'vehi' || snap->group_tag == 'weap' ||
							   snap->group_tag == 'eqip' || snap->group_tag == 'bipd' ||
							   actual_grp == 'vehi' || actual_grp == 'weap' ||
							   actual_grp == 'eqip' || actual_grp == 'bipd');
		if (slot_idx >= g_forge.live_instance_count)
		{
			memset(&g_forge.live_instances[g_forge.live_instance_count], 0,
				(size_t)(slot_idx - g_forge.live_instance_count + 1) * sizeof(struct forge_live_instance));
			g_forge.live_instance_count = slot_idx + 1;
		}
		inst = &g_forge.live_instances[slot_idx];
		if (inst->active && inst->object_index != NONE && forge_valid_object_get(inst->object_index))
			object_delete(inst->object_index);
		*inst = *snap;
		inst->active = 1;
		inst->object_index = NONE;
		inst->actor_index = NONE;
		inst->death_tick = -1;
		inst->spawned_once = 0;
		g_forge.edit_state.default_edits[inst->edit_entry_index].deleted = 0;
		if (def_idx != NONE && !(game_connection() == _game_connection_network_client && is_net_obj_type))
		{
			inst->object_index = forge_create_configured_engine_object(
				def_idx, inst->group_tag,
				inst->x, inst->y, inst->z,
				inst->yaw, inst->pitch, inst->roll);
			if (inst->object_index != NONE)
				inst->spawned_once = 1;
		}
		forge_3d_move_instance_ext(
			slot_idx, inst->x, inst->y, inst->z, inst->yaw, inst->pitch, inst->roll, 0, 0);
		forge_apply_instance_light_settings(slot_idx);
		forge_sync_instance_extended_props_to_edit_state(slot_idx);
		if (forge_is_multiplayer_session())
		{
			forge_network_send_spawn_ext(
				slot_idx, inst->group_tag, inst->tag_path, inst->source_cache_id,
				inst->x, inst->y, inst->z, inst->yaw, inst->pitch, inst->roll,
				inst->team_designator, inst->object_index, def_idx);
			forge_network_send_object_props(slot_idx);
		}
		return;
	}

	forge_3d_spawn_object_at_slot(
		slot_idx,
		snap->group_tag,
		snap->tag_path,
		snap->source_cache_id,
		snap->x, snap->y, snap->z,
		snap->yaw, snap->pitch, snap->roll,
		snap->team_designator,
		1);
	if (slot_idx < g_forge.live_instance_count && g_forge.live_instances[slot_idx].active)
	{
		inst = &g_forge.live_instances[slot_idx];
		if (snap->is_default_scenario_object)
		{
			inst->is_default_scenario_object = 1;
			inst->orig_x = snap->orig_x;
			inst->orig_y = snap->orig_y;
			inst->orig_z = snap->orig_z;
		}
		forge_apply_props_and_transform_snapshot(slot_idx, snap);
	}
}

static void forge_undo_step(void)
{
	uint32_t target_tx;
	int steps = 0;

	if (s_undo_top <= 0)
	{
		forge_set_status("Nothing to Undo");
		return;
	}

	target_tx = s_undo_records[s_undo_top - 1].tx_id;
	while (s_undo_top > 0 && s_undo_records[s_undo_top - 1].tx_id == target_tx)
	{
		struct forge_undo_record const *rec = &s_undo_records[--s_undo_top];
		switch (rec->op_type)
		{
			case _undo_op_spawn:
				forge_3d_delete_instance(rec->slot_idx, 1);
				break;
			case _undo_op_delete:
				forge_restore_snapshot_at_slot(rec->slot_idx, &rec->before_snap);
				break;
			case _undo_op_transform:
			case _undo_op_props:
				forge_apply_props_and_transform_snapshot(rec->slot_idx, &rec->before_snap);
				break;
			default:
				break;
		}
		steps++;
	}
	forge_server_mark_dirty();
	forge_set_status("Undo (%d item%s)", steps, steps == 1 ? "" : "s");
}

static void forge_redo_step(void)
{
	uint32_t target_tx;
	int steps = 0;

	if (s_undo_top >= s_undo_count)
	{
		forge_set_status("Nothing to Redo");
		return;
	}

	target_tx = s_undo_records[s_undo_top].tx_id;
	while (s_undo_top < s_undo_count && s_undo_records[s_undo_top].tx_id == target_tx)
	{
		struct forge_undo_record const *rec = &s_undo_records[s_undo_top++];
		switch (rec->op_type)
		{
			case _undo_op_spawn:
				forge_restore_snapshot_at_slot(rec->slot_idx, &rec->after_snap);
				break;
			case _undo_op_delete:
				forge_3d_delete_instance(rec->slot_idx, 1);
				break;
			case _undo_op_transform:
			case _undo_op_props:
				forge_apply_props_and_transform_snapshot(rec->slot_idx, &rec->after_snap);
				break;
			default:
				break;
		}
		steps++;
	}
	forge_server_mark_dirty();
	forge_set_status("Redo (%d item%s)", steps, steps == 1 ? "" : "s");
}

static void forge_prepare_delete_snapshot(int32_t slot_idx, struct forge_live_instance *out_snap)
{
	struct forge_live_instance *inst;
	if (!out_snap || slot_idx < 0 || slot_idx >= g_forge.live_instance_count)
		return;
	inst = &g_forge.live_instances[slot_idx];
	if (inst->is_default_scenario_object &&
		inst->group_tag != 'spwn' &&
		inst->group_tag != 'tele' &&
		inst->group_tag != 'flag')
	{
		forge_ensure_default_object_edit_entry(inst);
	}
	*out_snap = *inst;
}

static void forge_delete_instance_or_weld_group_with_undo(int32_t target_idx, int record_undo)
{
	struct forge_live_instance *inst;
	uint8_t gid;
	uint32_t tx_id = 0;

	if (target_idx < 0 || target_idx >= g_forge.live_instance_count)
		return;
	inst = &g_forge.live_instances[target_idx];
	if (!inst->active)
		return;

	gid = inst->group_id;
	if (record_undo)
		tx_id = forge_undo_begin_tx();

	if (gid >= 1 && gid <= FORGE_MAX_WELD_GROUPS)
	{
		int i, del_cnt = 0;
		for (i = 0; i < g_forge.live_instance_count; i++)
		{
			if (g_forge.live_instances[i].active && g_forge.live_instances[i].group_id == gid)
			{
				if (record_undo)
				{
					struct forge_live_instance snap;
					forge_prepare_delete_snapshot(i, &snap);
					if (i == g_forge.grabbed_live_index && !g_forge.grabbed_is_new_spawn)
					{
						snap.x = g_forge.grab_orig_x;
						snap.y = g_forge.grab_orig_y;
						snap.z = g_forge.grab_orig_z;
						snap.yaw = g_forge.grab_orig_yaw;
						snap.pitch = g_forge.grab_orig_pitch;
						snap.roll = g_forge.grab_orig_roll;
					}
					else if (s_weld_follower_count > 0)
					{
						int w;
						for (w = 0; w < s_weld_follower_count; w++)
						{
							if (s_weld_followers[w].live_idx == i)
							{
								snap.x = s_weld_followers[w].orig_x;
								snap.y = s_weld_followers[w].orig_y;
								snap.z = s_weld_followers[w].orig_z;
								snap.yaw = s_weld_followers[w].orig_yaw;
								snap.pitch = s_weld_followers[w].orig_pitch;
								snap.roll = s_weld_followers[w].orig_roll;
								break;
							}
						}
					}
					forge_undo_push_record(tx_id, _undo_op_delete, i, &snap, NULL);
				}
				forge_3d_delete_instance(i, 1);
				del_cnt++;
			}
		}
		forge_set_status("Deleted Group %d (%d items)", (int)gid, del_cnt);
	}
	else
	{
		if (record_undo)
		{
			struct forge_live_instance snap;
			forge_prepare_delete_snapshot(target_idx, &snap);
			if (target_idx == g_forge.grabbed_live_index && !g_forge.grabbed_is_new_spawn)
			{
				snap.x = g_forge.grab_orig_x;
				snap.y = g_forge.grab_orig_y;
				snap.z = g_forge.grab_orig_z;
				snap.yaw = g_forge.grab_orig_yaw;
				snap.pitch = g_forge.grab_orig_pitch;
				snap.roll = g_forge.grab_orig_roll;
			}
			forge_undo_push_record(tx_id, _undo_op_delete, target_idx, &snap, NULL);
		}
		forge_3d_delete_instance(target_idx, 1);
	}
	forge_server_mark_dirty();
}

static void forge_delete_held_item_or_group(void)
{
	int record_undo = !g_forge.grabbed_is_new_spawn;

	if (g_forge.koth_group_placing)
	{
		int k;
		uint32_t tx_id = record_undo ? forge_undo_begin_tx() : 0;
		for (k = 0; k < 10; k++)
		{
			int32_t nidx = g_forge.koth_group_live_indices[k];
			if (nidx >= 0 && nidx < g_forge.live_instance_count && g_forge.live_instances[nidx].active)
			{
				if (record_undo)
				{
					struct forge_live_instance snap;
					forge_prepare_delete_snapshot(nidx, &snap);
					forge_undo_push_record(tx_id, _undo_op_delete, nidx, &snap, NULL);
				}
				forge_3d_delete_instance(nidx, 1);
			}
		}
		g_forge.koth_group_placing = 0;
		forge_server_mark_dirty();
	}
	else if (g_forge.grabbed_live_index >= 0 && g_forge.grabbed_live_index < g_forge.live_instance_count)
	{
		forge_delete_instance_or_weld_group_with_undo(g_forge.grabbed_live_index, record_undo);
	}
	forge_weld_clear_followers();
	g_forge.grabbed_live_index = -1;
	g_forge.grabbed_object_index = NONE;
	g_forge.grabbed_is_new_spawn = 0;
}

static int32_t forge_pick_aimed_live_instance(real_vector3d const *fwd)
{
	int i, best = -1;
	float best_d2 = 25.0f;

	if (g_forge.hovered_live_index >= 0 &&
		g_forge.hovered_live_index < g_forge.live_instance_count &&
		g_forge.live_instances[g_forge.hovered_live_index].active)
	{
		return g_forge.hovered_live_index;
	}
	if (g_forge.hovered_object_index != NONE)
	{
		best = adopt_default_scenario_object(g_forge.hovered_object_index);
		if (best >= 0)
			return best;
	}
	if (fwd)
	{
		float best_ray_perp = 1.6f;
		for (i = 0; i < g_forge.live_instance_count; i++)
		{
			struct forge_live_instance const *inst = &g_forge.live_instances[i];
			float vx, vy, vz, dist_along, px, py, pz, perp_d2;
			if (!inst->active)
				continue;
			vx = inst->x - g_forge.cam_x;
			vy = inst->y - g_forge.cam_y;
			vz = (inst->z + 0.15f) - g_forge.cam_z;
			if (vx * vx + vy * vy + vz * vz > 400.0f)
				continue;
			dist_along = vx * fwd->i + vy * fwd->j + vz * fwd->k;
			if (dist_along > 0.5f && dist_along <= 20.0f)
			{
				px = vx - fwd->i * dist_along;
				py = vy - fwd->j * dist_along;
				pz = vz - fwd->k * dist_along;
				perp_d2 = px * px + py * py + pz * pz;
				if (perp_d2 < best_ray_perp)
				{
					best_ray_perp = perp_d2;
					best = i;
				}
			}
		}
	}
	if (best < 0)
	{
		for (i = 0; i < g_forge.live_instance_count; i++)
		{
			float cx_d, cy_d, cz_d, dx, dy, dz, d2;
			if (!g_forge.live_instances[i].active)
				continue;
			cx_d = g_forge.live_instances[i].x - g_forge.cam_x;
			cy_d = g_forge.live_instances[i].y - g_forge.cam_y;
			cz_d = g_forge.live_instances[i].z - g_forge.cam_z;
			if (cx_d * cx_d + cy_d * cy_d + cz_d * cz_d > 400.0f)
				continue;
			dx = g_forge.live_instances[i].x - g_forge.ray_hit_x;
			dy = g_forge.live_instances[i].y - g_forge.ray_hit_y;
			dz = g_forge.live_instances[i].z - g_forge.ray_hit_z;
			d2 = dx * dx + dy * dy + dz * dz;
			if (d2 < best_d2)
			{
				best_d2 = d2;
				best = i;
			}
		}
	}
	return best;
}

static void forge_cancel_or_release_held_item(void)
{
	if (g_forge.grabbed_live_index >= 0 && g_forge.grabbed_live_index < g_forge.live_instance_count)
	{
		if (g_forge.grabbed_is_new_spawn)
		{
			forge_delete_held_item_or_group();
		}
		else
		{
			int32_t restore_live = g_forge.grabbed_live_index;
			long restore_obj = g_forge.grabbed_object_index;
			g_forge.grabbed_live_index = -1;
			g_forge.grabbed_object_index = NONE;
			forge_3d_move_instance_ext(
				restore_live,
				g_forge.grab_orig_x,
				g_forge.grab_orig_y,
				g_forge.grab_orig_z,
				g_forge.grab_orig_yaw,
				g_forge.grab_orig_pitch,
				g_forge.grab_orig_roll,
				0,
				0);
			forge_network_send_transform_ext(
				restore_live,
				restore_obj,
				g_forge.grab_orig_x,
				g_forge.grab_orig_y,
				g_forge.grab_orig_z,
				g_forge.grab_orig_yaw,
				g_forge.grab_orig_pitch,
				g_forge.grab_orig_roll,
				0,
				1);
			if (s_weld_follower_count > 0)
				forge_weld_restore_followers_orig();
			forge_set_status("Returned '%s'", g_forge.live_instances[restore_live].tag_path);
		}
	}
	forge_weld_clear_followers();
	g_forge.grabbed_live_index = -1;
	g_forge.grabbed_object_index = NONE;
	g_forge.grabbed_is_new_spawn = 0;
	g_forge.koth_group_placing = 0;
}

static void forge_drop_or_place_held_item(void)
{
	int32_t released_live_idx = g_forge.grabbed_live_index;
	int was_new_spawn = g_forge.grabbed_is_new_spawn;
	struct forge_live_instance *released;
	uint32_t tx_id;

	if (released_live_idx < 0 || released_live_idx >= g_forge.live_instance_count)
		return;
	released = &g_forge.live_instances[released_live_idx];
	g_forge.grabbed_live_index = -1;
	g_forge.grabbed_object_index = NONE;
	g_forge.grabbed_is_new_spawn = 0;

	/* Apply release transform (is_held = 0) so vehicles wake up physics and become drivable immediately on Host & Clients! */
	forge_3d_move_instance_ext(
		released_live_idx,
		released->x,
		released->y,
		released->z,
		released->yaw,
		released->pitch,
		released->roll,
		0,
		0);

	tx_id = forge_undo_begin_tx();

	if (g_forge.koth_group_placing)
	{
		int k;
		for (k = 0; k < 10; k++)
		{
			int32_t nidx = g_forge.koth_group_live_indices[k];
			if (nidx >= 0 && nidx < g_forge.live_instance_count && g_forge.live_instances[nidx].active)
			{
				struct forge_live_instance const *ni = &g_forge.live_instances[nidx];
				forge_3d_move_instance_ext(nidx, ni->x, ni->y, ni->z, ni->yaw, ni->pitch, ni->roll, 0, 0);
				forge_network_send_transform_ext(nidx, ni->object_index, ni->x, ni->y, ni->z, ni->yaw, ni->pitch, ni->roll, 0, 1);
				if (was_new_spawn)
					forge_undo_push_record(tx_id, _undo_op_spawn, nidx, NULL, ni);
			}
		}
		forge_set_status("Placed KOTH Hill #%d", g_forge.koth_group_hill_id);
		g_forge.koth_group_placing = 0;
		forge_weld_clear_followers();
	}
	else
	{
		forge_network_send_transform_ext(
			released_live_idx,
			released->object_index,
			released->x,
			released->y,
			released->z,
			released->yaw,
			released->pitch,
			released->roll,
			0,
			1);

		if (was_new_spawn)
		{
			forge_undo_push_record(tx_id, _undo_op_spawn, released_live_idx, NULL, released);
		}
		else
		{
			struct forge_live_instance before_piv = *released;
			before_piv.x = g_forge.grab_orig_x;
			before_piv.y = g_forge.grab_orig_y;
			before_piv.z = g_forge.grab_orig_z;
			before_piv.yaw = g_forge.grab_orig_yaw;
			before_piv.pitch = g_forge.grab_orig_pitch;
			before_piv.roll = g_forge.grab_orig_roll;
			forge_undo_push_record(tx_id, _undo_op_transform, released_live_idx, &before_piv, released);
		}

		if (s_weld_follower_count > 0)
		{
			int w;
			forge_weld_update_followers(released_live_idx, 0, 1, 1);
			for (w = 0; w < s_weld_follower_count; w++)
			{
				int32_t fli = s_weld_followers[w].live_idx;
				if (fli >= 0 && fli < g_forge.live_instance_count && g_forge.live_instances[fli].active)
				{
					struct forge_live_instance const *fi = &g_forge.live_instances[fli];
					if (was_new_spawn)
					{
						forge_undo_push_record(tx_id, _undo_op_spawn, fli, NULL, fi);
					}
					else
					{
						struct forge_live_instance before_f = *fi;
						before_f.x = s_weld_followers[w].orig_x;
						before_f.y = s_weld_followers[w].orig_y;
						before_f.z = s_weld_followers[w].orig_z;
						before_f.yaw = s_weld_followers[w].orig_yaw;
						before_f.pitch = s_weld_followers[w].orig_pitch;
						before_f.roll = s_weld_followers[w].orig_roll;
						forge_undo_push_record(tx_id, _undo_op_transform, fli, &before_f, fi);
					}
				}
			}
			forge_set_status("Placed Group %d (%d items)", (int)released->group_id, s_weld_follower_count + 1);
			forge_weld_clear_followers();
		}
		else
		{
			forge_set_status("Placed '%s'", released->tag_path);
		}
	}
	forge_server_mark_dirty();
}

static real_point3d s_last_valid_spartan_pos = { 0.0f, 0.0f, 0.0f };
static int s_has_valid_spartan_pos = 0;

/* Computes a guaranteed-safe Spartan feet position from the Monitor camera position:
 * 1. Pushes horizontally clear of any walls within Spartan cylinder radius (0.38u).
 * 2. Pushes below any low ceiling within 0.25u above eye level.
 * 3. Raycasts down 0.75u: if a floor is closer than 0.75u, places Spartan feet 0.06u ABOVE the floor
 *    (never cam_z - 0.65f underneath the floor!).
 * 4. Verifies BSP leaf validity and falls back to the last known valid Spartan position if needed. */
static void forge_compute_safe_spartan_stand_pos(float cam_x, float cam_y, float cam_z, real_point3d *out_pos)
{
	unsigned long both_flags = FLAG(_collision_test_structure_bit) |
							   FLAG(_collision_test_front_facing_surfaces_bit) |
							   FLAG(_collision_test_back_facing_surfaces_bit);
	real_point3d eye_pt, stand_pt;
	struct collision_result coll, vcoll;
	real_vector3d ray_vec, step_vec;
	int k;

	eye_pt.x = cam_x;
	eye_pt.y = cam_y;
	eye_pt.z = cam_z;

	/* 1. Ensure eye_pt starts inside a valid BSP leaf */
	if (scenario_leaf_index_from_point(&eye_pt) == NONE)
	{
		static float const s_SavedOffsets[18][3] = {
			{ 0.0f, 0.0f, 0.30f }, { 0.0f, 0.0f, -0.30f },
			{ 0.30f, 0.0f, 0.0f }, { -0.30f, 0.0f, 0.0f },
			{ 0.0f, 0.30f, 0.0f }, { 0.0f, -0.30f, 0.0f },
			{ 0.0f, 0.0f, 0.65f }, { 0.0f, 0.0f, -0.65f },
			{ 0.60f, 0.0f, 0.20f }, { -0.60f, 0.0f, 0.20f },
			{ 0.0f, 0.60f, 0.20f }, { 0.0f, -0.60f, 0.20f },
			{ 1.00f, 0.0f, 0.35f }, { -1.00f, 0.0f, 0.35f },
			{ 0.0f, 1.00f, 0.35f }, { 0.0f, -1.00f, 0.35f },
			{ 0.0f, 0.0f, 1.20f }, { 0.0f, 0.0f, -1.20f }
		};
		for (k = 0; k < 18; k++)
		{
			real_point3d probe;
			probe.x = cam_x + s_SavedOffsets[k][0];
			probe.y = cam_y + s_SavedOffsets[k][1];
			probe.z = cam_z + s_SavedOffsets[k][2];
			if (scenario_leaf_index_from_point(&probe) != NONE)
			{
				eye_pt = probe;
				break;
			}
		}
		if (scenario_leaf_index_from_point(&eye_pt) == NONE && s_has_valid_spartan_pos)
		{
			*out_pos = s_last_valid_spartan_pos;
			return;
		}
	}

	/* 2. Push horizontally clear of any nearby walls within Spartan cylinder radius (0.38u) */
	for (k = 0; k < 8; k++)
	{
		float ang = (float)k * 0.785398163f;
		float wall_pad = 0.38f;
		ray_vec.i = (float)cos((double)ang) * wall_pad;
		ray_vec.j = (float)sin((double)ang) * wall_pad;
		ray_vec.k = 0.0f;
		memset(&coll, 0, sizeof(coll));
		if (collision_test_vector(both_flags, &eye_pt, &ray_vec, NONE, &coll))
		{
			float d = coll.t * wall_pad;
			if (d < wall_pad)
			{
				float push = (wall_pad - d) + 0.02f;
				real_point3d cand = eye_pt;
				cand.x += coll.plane.n.i * push;
				cand.y += coll.plane.n.j * push;
				step_vec.i = cand.x - eye_pt.x;
				step_vec.j = cand.y - eye_pt.y;
				step_vec.k = 0.0f;
				memset(&vcoll, 0, sizeof(vcoll));
				if (!collision_test_vector(both_flags, &eye_pt, &step_vec, NONE, &vcoll) &&
					scenario_leaf_index_from_point(&cand) != NONE)
				{
					eye_pt = cand;
				}
			}
		}
	}

	/* 3. Push below any low ceiling directly above eye_pt */
	ray_vec.i = 0.0f;
	ray_vec.j = 0.0f;
	ray_vec.k = 0.28f;
	memset(&coll, 0, sizeof(coll));
	if (collision_test_vector(both_flags, &eye_pt, &ray_vec, NONE, &coll))
	{
		float d = coll.t * 0.28f;
		if (d < 0.25f)
		{
			real_point3d cand = eye_pt;
			cand.z -= (0.25f - d);
			step_vec.i = 0.0f;
			step_vec.j = 0.0f;
			step_vec.k = cand.z - eye_pt.z;
			memset(&vcoll, 0, sizeof(vcoll));
			if (!collision_test_vector(both_flags, &eye_pt, &step_vec, NONE, &vcoll) &&
				scenario_leaf_index_from_point(&cand) != NONE)
			{
				eye_pt = cand;
			}
		}
	}

	/* 4. Raycast down to find the floor so Spartan feet are NEVER placed below a floor! */
	ray_vec.i = 0.0f;
	ray_vec.j = 0.0f;
	ray_vec.k = -0.75f;
	memset(&coll, 0, sizeof(coll));
	if (collision_test_vector(both_flags, &eye_pt, &ray_vec, NONE, &coll))
	{
		stand_pt.x = eye_pt.x;
		stand_pt.y = eye_pt.y;
		stand_pt.z = coll.point.z + 0.06f;
	}
	else
	{
		stand_pt.x = eye_pt.x;
		stand_pt.y = eye_pt.y;
		stand_pt.z = eye_pt.z - 0.65f;
	}

	/* 5. Verify eye_pt -> stand_pt segment does not cross any surface and lands in a valid BSP leaf */
	step_vec.i = stand_pt.x - eye_pt.x;
	step_vec.j = stand_pt.y - eye_pt.y;
	step_vec.k = stand_pt.z - eye_pt.z;
	memset(&vcoll, 0, sizeof(vcoll));
	if (collision_test_vector(both_flags, &eye_pt, &step_vec, NONE, &vcoll) ||
		scenario_leaf_index_from_point(&stand_pt) == NONE)
	{
		stand_pt = eye_pt;
	}

	if (scenario_leaf_index_from_point(&stand_pt) != NONE)
	{
		s_last_valid_spartan_pos = stand_pt;
		s_has_valid_spartan_pos = 1;
		*out_pos = stand_pt;
	}
	else if (s_has_valid_spartan_pos)
	{
		*out_pos = s_last_valid_spartan_pos;
	}
	else
	{
		*out_pos = stand_pt;
	}
}

static void forge_sync_unit_to_monitor_pose(
	long unit_idx,
	float cam_x, float cam_y, float cam_z,
	float yaw, float pitch, float roll,
	int is_monitor,
	int reposition,
	float min_reposition_dist_sq)
{
	struct unit_datum *u;
	real_point3d u_pos;
	real_vector3d m_fwd, m_up, h_fwd, h_up;

	if (unit_idx == NONE)
		return;
	u = (struct unit_datum *)object_try_and_get_and_verify_type(unit_idx, _object_mask_unit);
	if (!u)
		return;

	if (is_monitor)
	{
		u_pos.x = cam_x;
		u_pos.y = cam_y;
		u_pos.z = cam_z - 0.65f;
	}
	else
	{
		forge_compute_safe_spartan_stand_pos(cam_x, cam_y, cam_z, &u_pos);
	}
	euler_to_forward_up(yaw, pitch, roll, &m_fwd, &m_up);
	euler_to_forward_up(yaw, 0.0f, 0.0f, &h_fwd, &h_up);

	if (is_monitor)
	{
		SET_FLAG(u->object.flags, _object_no_collisions_bit, TRUE);
		SET_FLAG(u->object.flags, _object_at_rest_bit, TRUE);
		SET_FLAG(u->object.damage_flags, _object_cannot_take_damage_bit, TRUE);
		SET_FLAG(u->object.damage_flags, _object_shield_depleted_bit, FALSE);
		u->object.body_vitality = 1.0f;
		u->object.shield_vitality = 1.0f;
		u->unit.control_flags = 0;
		u->unit.persistent_control_flags = 0;
		u->unit.persistent_control_timer = 0;
		u->unit.primary_trigger = 0.0f;
		u->unit.grenade_throw_state = 0;
		u->unit.melee_attack_state = 0;
		u->object.translational_velocity.i = 0.0f;
		u->object.translational_velocity.j = 0.0f;
		u->object.translational_velocity.k = 0.0f;
		u->object.angular_velocity.i = 0.0f;
		u->object.angular_velocity.j = 0.0f;
		u->object.angular_velocity.k = 0.0f;
		forge_hide_spartan_linked_attachments_and_children(unit_idx, 1);
		object_set_automatic_deactivation(unit_idx, FALSE);
		if (reposition)
		{
			float ux_d = u->object.position.x - u_pos.x;
			float uy_d = u->object.position.y - u_pos.y;
			float uz_d = u->object.position.z - u_pos.z;
			if (min_reposition_dist_sq <= 0.0f ||
				(ux_d * ux_d + uy_d * uy_d + uz_d * uz_d) > min_reposition_dist_sq)
			{
				if (scenario_leaf_index_from_point(&u_pos) != NONE)
					object_set_position(unit_idx, &u_pos, &h_fwd, &h_up);
				else
				{
					u->object.position = u_pos;
					u->object.forward = h_fwd;
					u->object.up = h_up;
				}
			}
		}
	}
	else
	{
		SET_FLAG(u->object.flags, _object_no_collisions_bit, FALSE);
		SET_FLAG(u->object.flags, _object_at_rest_bit, FALSE);
		SET_FLAG(u->object.damage_flags, _object_cannot_take_damage_bit, FALSE);
		forge_hide_spartan_linked_attachments_and_children(unit_idx, 0);
		if (reposition)
		{
			if (scenario_leaf_index_from_point(&u_pos) != NONE)
				object_set_position(unit_idx, &u_pos, &h_fwd, &h_up);
			else
			{
				u->object.position = u_pos;
				u->object.forward = h_fwd;
				u->object.up = h_up;
			}
		}
		object_activate(unit_idx);
	}

	u->unit.desired_facing_vector = is_monitor ? m_fwd : h_fwd;
	u->unit.desired_aiming_vector = is_monitor ? m_fwd : h_fwd;
	u->unit.aiming_vector = is_monitor ? m_fwd : h_fwd;
	u->unit.desired_looking_vector = is_monitor ? m_fwd : h_fwd;
	u->unit.looking_vector = is_monitor ? m_fwd : h_fwd;
	u->object.translational_velocity.i = 0.0f;
	u->object.translational_velocity.j = 0.0f;
	u->object.translational_velocity.k = 0.0f;
	u->object.angular_velocity.i = 0.0f;
	u->object.angular_velocity.j = 0.0f;
	u->object.angular_velocity.k = 0.0f;
}

extern void network_distributed_notify_forge_monitor_teleport(
	long player_index,
	long unit_index,
	real_point3d const *new_position,
	boolean is_monitor);

void forge_apply_host_player_monitor_state(
	int player_abs_idx,
	int is_monitor,
	float x, float y, float z,
	float yaw, float pitch)
{
	long p_idx;
	int was_monitor;

	if (player_abs_idx < 0 || player_abs_idx >= 128)
		return;

	was_monitor = g_forge.player_is_monitor[player_abs_idx];
	g_forge.player_is_monitor[player_abs_idx] = (uint8_t)(is_monitor ? 1 : 0);
	g_forge.player_has_monitor_pos[player_abs_idx] = (uint8_t)(is_monitor ? 1 : 0);
	g_forge.player_monitor_pos[player_abs_idx][0] = x;
	g_forge.player_monitor_pos[player_abs_idx][1] = y;
	g_forge.player_monitor_pos[player_abs_idx][2] = z;
	g_forge.player_monitor_yaw[player_abs_idx] = yaw;
	g_forge.player_monitor_pitch[player_abs_idx] = pitch;

	if (!is_monitor)
	{
		g_forge.player_has_render_pos[player_abs_idx] = 0;
	}
	else if (!g_forge.player_has_render_pos[player_abs_idx] || !was_monitor)
	{
		g_forge.player_has_render_pos[player_abs_idx] = 1;
		g_forge.player_monitor_render_pos[player_abs_idx][0] = x;
		g_forge.player_monitor_render_pos[player_abs_idx][1] = y;
		g_forge.player_monitor_render_pos[player_abs_idx][2] = z;
		g_forge.player_monitor_render_yaw[player_abs_idx] = yaw;
		g_forge.player_monitor_render_pitch[player_abs_idx] = pitch;
	}
	else
	{
		float dx = x - g_forge.player_monitor_render_pos[player_abs_idx][0];
		float dy = y - g_forge.player_monitor_render_pos[player_abs_idx][1];
		float dz = z - g_forge.player_monitor_render_pos[player_abs_idx][2];
		if (dx * dx + dy * dy + dz * dz > 36.0f)
		{
			g_forge.player_monitor_render_pos[player_abs_idx][0] = x;
			g_forge.player_monitor_render_pos[player_abs_idx][1] = y;
			g_forge.player_monitor_render_pos[player_abs_idx][2] = z;
			g_forge.player_monitor_render_yaw[player_abs_idx] = yaw;
			g_forge.player_monitor_render_pitch[player_abs_idx] = pitch;
		}
	}

	if (!player_data)
		return;

	for (p_idx = data_next_index(player_data, NONE); p_idx != NONE; p_idx = data_next_index(player_data, p_idx))
	{
		if ((int)DATUM_INDEX_TO_ABSOLUTE_INDEX(p_idx) == player_abs_idx)
		{
			struct player_datum *pl = player_get(p_idx);
			if (pl && pl->unit_index != NONE)
			{
				struct unit_datum *u_chk = (struct unit_datum *)object_try_and_get_and_verify_type(pl->unit_index, _object_mask_unit);
				if (is_monitor && u_chk && (u_chk->object.parent_object_index != NONE || u_chk->unit.parent_seat_index != NONE))
				{
					/* A player seated inside a vehicle cannot switch into Monitor Mode */
					g_forge.player_is_monitor[player_abs_idx] = 0;
					g_forge.player_has_monitor_pos[player_abs_idx] = 0;
					g_forge.player_has_render_pos[player_abs_idx] = 0;
					forge_hide_spartan_linked_attachments_and_children(pl->unit_index, 0);
					return;
				}
				forge_hide_spartan_linked_attachments_and_children(pl->unit_index, is_monitor ? 1 : 0);
				if (game_connection() == _game_connection_network_server &&
					player_abs_idx != forge_get_local_player_abs_index())
				{
					forge_sync_unit_to_monitor_pose(
						pl->unit_index,
						x, y, z,
						yaw, pitch, 0.0f,
						is_monitor ? 1 : 0,
						1,
						0.0f);
					u_chk = (struct unit_datum *)object_try_and_get_and_verify_type(pl->unit_index, _object_mask_unit);
					if (u_chk)
					{
						network_distributed_notify_forge_monitor_teleport(
							p_idx, pl->unit_index, &u_chk->object.position, is_monitor ? TRUE : FALSE);
					}
				}
			}
			break;
		}
	}
}

static void forge_monitor_enforce_wall_clearance(real_point3d *pt, float keep_dist)
{
	static float const s_ProbeDirs[6][3] = {
		{  1.0f,  0.0f,  0.0f },
		{ -1.0f,  0.0f,  0.0f },
		{  0.0f,  1.0f,  0.0f },
		{  0.0f, -1.0f,  0.0f },
		{  0.0f,  0.0f,  1.0f },
		{  0.0f,  0.0f, -1.0f }
	};
	unsigned long both_flags = FLAG(_collision_test_structure_bit) |
							   FLAG(_collision_test_front_facing_surfaces_bit) |
							   FLAG(_collision_test_back_facing_surfaces_bit);
	int d;

	for (d = 0; d < 6; d++)
	{
		real_vector3d pvec, svec;
		struct collision_result pcoll, vcoll;
		pvec.i = s_ProbeDirs[d][0] * keep_dist;
		pvec.j = s_ProbeDirs[d][1] * keep_dist;
		pvec.k = s_ProbeDirs[d][2] * keep_dist;
		memset(&pcoll, 0, sizeof(pcoll));
		if (collision_test_vector(both_flags, pt, &pvec, NONE, &pcoll))
		{
			float dist = pcoll.t * keep_dist;
			if (dist < keep_dist)
			{
				float push = (keep_dist - dist);
				float dot = s_ProbeDirs[d][0] * pcoll.plane.n.i +
							s_ProbeDirs[d][1] * pcoll.plane.n.j +
							s_ProbeDirs[d][2] * pcoll.plane.n.k;
				real_point3d cand = *pt;
				if (dot < 0.0f)
				{
					cand.x += pcoll.plane.n.i * push;
					cand.y += pcoll.plane.n.j * push;
					cand.z += pcoll.plane.n.k * push;
				}
				else
				{
					cand.x -= s_ProbeDirs[d][0] * push;
					cand.y -= s_ProbeDirs[d][1] * push;
					cand.z -= s_ProbeDirs[d][2] * push;
				}
				svec.i = cand.x - pt->x;
				svec.j = cand.y - pt->y;
				svec.k = cand.z - pt->z;
				memset(&vcoll, 0, sizeof(vcoll));
				if (!collision_test_vector(both_flags, pt, &svec, NONE, &vcoll) &&
					scenario_leaf_index_from_point(&cand) != NONE)
				{
					*pt = cand;
				}
			}
		}
	}
}

static int forge_monitor_try_step(
	real_point3d const *from_pt,
	float dx, float dy, float dz,
	float pad,
	real_point3d *out_pt,
	real_plane3d *out_hit_plane)
{
	unsigned long both_flags = FLAG(_collision_test_structure_bit) |
							   FLAG(_collision_test_front_facing_surfaces_bit) |
							   FLAG(_collision_test_back_facing_surfaces_bit);
	float len = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
	real_vector3d dir, ray_vec, step_vec, perp1, perp2;
	struct collision_result best_coll, fcoll, vcoll;
	int hit = 0;
	int f;

	*out_pt = *from_pt;
	if (len <= 0.0001f)
		return 0;

	dir.i = dx / len;
	dir.j = dy / len;
	dir.k = dz / len;

	/* Build two perpendicular axes for 4 radial feeler rays around the camera cylinder */
	if ((float)fabs((double)dir.k) < 0.85f)
	{
		float plen = (float)sqrt((double)(dir.i * dir.i + dir.j * dir.j));
		perp1.i = -dir.j / plen;
		perp1.j =  dir.i / plen;
		perp1.k =  0.0f;
	}
	else
	{
		float plen = (float)sqrt((double)(dir.i * dir.i + dir.k * dir.k));
		perp1.i = -dir.k / plen;
		perp1.j =  0.0f;
		perp1.k =  dir.i / plen;
	}
	perp2.i = dir.j * perp1.k - dir.k * perp1.j;
	perp2.j = dir.k * perp1.i - dir.i * perp1.k;
	perp2.k = dir.i * perp1.j - dir.j * perp1.i;

	ray_vec.i = dir.i * (len + pad);
	ray_vec.j = dir.j * (len + pad);
	ray_vec.k = dir.k * (len + pad);

	memset(&best_coll, 0, sizeof(best_coll));
	if (collision_test_vector(both_flags, from_pt, &ray_vec, NONE, &best_coll))
		hit = 1;

	/* Cast 4 feeler rays around the Monitor's 0.16u radius so grazing walls/corners never clip */
	for (f = 0; f < 4; f++)
	{
		float s1 = (f == 0) ? 0.16f : ((f == 1) ? -0.16f : 0.0f);
		float s2 = (f == 2) ? 0.16f : ((f == 3) ? -0.16f : 0.0f);
		real_point3d f_orig;
		real_vector3d f_ofs;
		f_ofs.i = perp1.i * s1 + perp2.i * s2;
		f_ofs.j = perp1.j * s1 + perp2.j * s2;
		f_ofs.k = perp1.k * s1 + perp2.k * s2;
		f_orig.x = from_pt->x + f_ofs.i;
		f_orig.y = from_pt->y + f_ofs.j;
		f_orig.z = from_pt->z + f_ofs.k;
		memset(&vcoll, 0, sizeof(vcoll));
		if (scenario_leaf_index_from_point(&f_orig) != NONE &&
			!collision_test_vector(both_flags, from_pt, &f_ofs, NONE, &vcoll))
		{
			memset(&fcoll, 0, sizeof(fcoll));
			if (collision_test_vector(both_flags, &f_orig, &ray_vec, NONE, &fcoll))
			{
				if (!hit || fcoll.t < best_coll.t)
				{
					best_coll = fcoll;
					hit = 1;
				}
			}
		}
	}

	if (hit)
	{
		float hit_dist = best_coll.t * (len + pad);
		float allow = hit_dist - pad;
		if (out_hit_plane)
			*out_hit_plane = best_coll.plane;
		if (allow > 0.0005f)
		{
			real_point3d cand;
			cand.x = from_pt->x + dir.i * allow;
			cand.y = from_pt->y + dir.j * allow;
			cand.z = from_pt->z + dir.k * allow;
			step_vec.i = cand.x - from_pt->x;
			step_vec.j = cand.y - from_pt->y;
			step_vec.k = cand.z - from_pt->z;
			memset(&vcoll, 0, sizeof(vcoll));
			if (!collision_test_vector(both_flags, from_pt, &step_vec, NONE, &vcoll) &&
				scenario_leaf_index_from_point(&cand) != NONE)
			{
				*out_pt = cand;
			}
		}
		return 1;
	}
	else
	{
		real_point3d cand;
		cand.x = from_pt->x + dx;
		cand.y = from_pt->y + dy;
		cand.z = from_pt->z + dz;
		step_vec.i = dx;
		step_vec.j = dy;
		step_vec.k = dz;
		memset(&vcoll, 0, sizeof(vcoll));
		if (!collision_test_vector(both_flags, from_pt, &step_vec, NONE, &vcoll) &&
			scenario_leaf_index_from_point(&cand) != NONE)
		{
			*out_pt = cand;
			return 0;
		}
		return 1;
	}
}

static long find_matching_default_world_object(
	uint32_t group_tag,
	char const *tag_path,
	float ox, float oy, float oz,
	int32_t for_edit_idx)
{
	struct object_iterator iter;
	struct object_datum *dobj;
	long found_oidx = NONE;
	float best_d2 = (group_tag == 'scen' || group_tag == 'mach' ||
					 group_tag == 'lifi' || group_tag == 'ctrl') ? 0.04f : 9.0f;

	object_iterator_new(&iter, (unsigned long)-1, 0);
	while ((dobj = object_iterator_next(&iter)) != NULL)
	{
		float dx = dobj->object.position.x - ox;
		float dy = dobj->object.position.y - oy;
		float dz = dobj->object.position.z - oz;
		float d2 = dx * dx + dy * dy + dz * dz * 0.25f;
		int li, claimed_by_other = 0;

		if (dobj->object.parent_object_index != NONE)
			continue;
		if (dobj->object.type == _object_type_biped &&
			((struct unit_datum *)dobj)->unit.player_index != NONE)
			continue;
		if (tag_path && tag_path[0] && dobj->definition_index != NONE)
		{
			char const *oname = tag_get_name(dobj->definition_index);
			if (oname && oname[0] && _stricmp(oname, tag_path) != 0)
				continue;
		}
		for (li = 0; li < g_forge.live_instance_count; li++)
		{
			if (g_forge.live_instances[li].active &&
				g_forge.live_instances[li].object_index == iter.index)
			{
				if (!g_forge.live_instances[li].is_default_scenario_object ||
					(g_forge.live_instances[li].edit_entry_index >= 0 &&
					 g_forge.live_instances[li].edit_entry_index != for_edit_idx))
				{
					claimed_by_other = 1;
					break;
				}
			}
		}
		if (claimed_by_other)
			continue;
		if (d2 < best_d2)
		{
			best_d2 = d2;
			found_oidx = iter.index;
		}
	}
	return found_oidx;
}

int32_t forge_3d_adopt_default_object_at_slot(
	int32_t forced_live_idx,
	uint32_t group_tag,
	char const *tag_path,
	float orig_x,
	float orig_y,
	float orig_z)
{
	long found_oidx;
	int32_t adopted_idx;

	if (forced_live_idx < 0 || forced_live_idx >= FORGE_MAX_LIVE_INSTANCES)
		return -1;

	if (g_forge.live_instance_count == 0)
		forge_init_map_scenario_entities();

	if (forced_live_idx < g_forge.live_instance_count)
	{
		struct forge_live_instance const *cur = &g_forge.live_instances[forced_live_idx];
		if (cur->is_default_scenario_object &&
			(!cur->active || forge_valid_object_get(cur->object_index) != NULL))
		{
			float dx = cur->orig_x - orig_x;
			float dy = cur->orig_y - orig_y;
			float dz = cur->orig_z - orig_z;
			if (dx * dx + dy * dy + dz * dz < 1.0f)
				return forced_live_idx;
		}
	}

	found_oidx = find_matching_default_world_object(group_tag, tag_path, orig_x, orig_y, orig_z, -1);
	if (found_oidx == NONE)
		return -1;

	adopted_idx = adopt_default_scenario_object(found_oidx);
	if (adopted_idx < 0)
		return -1;

	g_forge.live_instances[adopted_idx].orig_x = orig_x;
	g_forge.live_instances[adopted_idx].orig_y = orig_y;
	g_forge.live_instances[adopted_idx].orig_z = orig_z;
	forge_ensure_default_object_edit_entry(&g_forge.live_instances[adopted_idx]);

	if (adopted_idx != forced_live_idx)
	{
		if (forced_live_idx >= g_forge.live_instance_count)
		{
			memset(
				&g_forge.live_instances[g_forge.live_instance_count],
				0,
				(size_t)(forced_live_idx - g_forge.live_instance_count + 1) * sizeof(struct forge_live_instance));
			g_forge.live_instance_count = forced_live_idx + 1;
		}
		if (!g_forge.live_instances[forced_live_idx].active)
		{
			g_forge.live_instances[forced_live_idx] = g_forge.live_instances[adopted_idx];
			memset(&g_forge.live_instances[adopted_idx], 0, sizeof(struct forge_live_instance));
			g_forge.live_instances[adopted_idx].object_index = NONE;
			g_forge.live_instances[adopted_idx].actor_index = NONE;
			g_forge.live_instances[adopted_idx].edit_entry_index = -1;
			adopted_idx = forced_live_idx;
		}
	}
	return adopted_idx;
}

static long s_last_game_tick = -1;
static struct scenario *s_last_tick_scen = NULL;

static int forge_is_instance_held_by_weld_drag(int32_t live_idx)
{
	int w;
	if (g_forge.grabbed_live_index < 0 || s_weld_follower_count <= 0)
		return 0;
	for (w = 0; w < s_weld_follower_count; w++)
	{
		if (s_weld_followers[w].live_idx == live_idx)
			return 1;
	}
	return 0;
}

static void forge_enforce_runtime_object_properties(void)
{
	long cur_tick = game_time_get();
	int i;

	for (i = 0; i < g_forge.live_instance_count; i++)
	{
		struct forge_live_instance *inst = &g_forge.live_instances[i];
		struct object_datum *obj;

		if (!inst->active || inst->is_ai_unit || inst->object_index == NONE)
			continue;
		if (inst->group_tag == 'spwn' || inst->group_tag == 'tele' || inst->group_tag == 'flag')
			continue;
		if (i == g_forge.grabbed_live_index || forge_is_instance_held_by_weld_drag(i))
			continue;

		obj = forge_valid_object_get(inst->object_index);
		if (!obj)
			continue;

		/* Lock Fixed (1) and Phased (2) objects at their exact authored transform with solid collision */
		if ((inst->physics_mode == _forge_physics_fixed ||
			 inst->physics_mode == _forge_physics_phased) &&
			obj->object.parent_object_index == NONE)
		{
			real_vector3d efwd, eup;
			float dx = obj->object.position.x - inst->x;
			float dy = obj->object.position.y - inst->y;
			float dz = obj->object.position.z - inst->z;
			float df, du;

			euler_to_forward_up(inst->yaw, inst->pitch, inst->roll, &efwd, &eup);
			df = (efwd.i - obj->object.forward.i) * (efwd.i - obj->object.forward.i) +
				 (efwd.j - obj->object.forward.j) * (efwd.j - obj->object.forward.j) +
				 (efwd.k - obj->object.forward.k) * (efwd.k - obj->object.forward.k);
			du = (eup.i - obj->object.up.i) * (eup.i - obj->object.up.i) +
				 (eup.j - obj->object.up.j) * (eup.j - obj->object.up.j) +
				 (eup.k - obj->object.up.k) * (eup.k - obj->object.up.k);

			SET_FLAG(obj->object.flags, _object_no_collisions_bit, FALSE);
			SET_FLAG(obj->object.flags, _object_at_rest_bit, TRUE);
			if (dx * dx + dy * dy + dz * dz > 0.0001f || df > 0.0004f || du > 0.0004f)
			{
				real_point3d pos;
				pos.x = inst->x;
				pos.y = inst->y;
				pos.z = inst->z;
				object_set_position(inst->object_index, &pos, &efwd, &eup);
				object_set_automatic_deactivation(inst->object_index, FALSE);
				object_activate(inst->object_index);
				forge_ensure_object_connected_to_bsp(inst->object_index, inst->x, inst->y, inst->z);
			}
			obj->object.translational_velocity.i = 0.0f;
			obj->object.translational_velocity.j = 0.0f;
			obj->object.translational_velocity.k = 0.0f;
			obj->object.angular_velocity.i = 0.0f;
			obj->object.angular_velocity.j = 0.0f;
			obj->object.angular_velocity.k = 0.0f;
		}

		/* Keep custom dynamic light color/intensity/radius active (including after late MP object link) */
		if ((inst->light_color_preset || inst->light_intensity_preset || inst->light_radius_preset) &&
			((cur_tick + i) % 30L == 0))
		{
			forge_apply_instance_light_settings(i);
		}
	}
}

static void forge_reconcile_live_instances(void)
{
	struct scenario *scen = global_scenario_try_and_get();
	long cur_tick = game_time_get();
	long cur_unit = player_control_get_unit_index(0);
	int reverted = 0;
	int i;

	if (game_connection() == _game_connection_network_client)
		return;

	if (scen != s_last_tick_scen)
	{
		s_last_tick_scen = scen;
		s_last_game_tick = cur_tick;
	}
	else if (s_last_game_tick >= 0 && cur_tick < s_last_game_tick)
	{
		/* Halo's single-player death or checkpoint revert (game_state_revert) rewound the game state! */
		reverted = 1;
	}
	s_last_game_tick = cur_tick;

	/* If the local player died while holding an item, cleanly place/release it so it isn't left mid-grab */
	if (cur_unit == NONE && g_forge.grabbed_live_index >= 0)
	{
		forge_drop_or_place_held_item();
		if (g_forge.active_modal == _forge_modal_object_properties)
			g_forge.active_modal = _forge_modal_none;
	}

	if (reverted || cur_tick < 90)
	{
		if (reverted || g_forge.edit_state.replace_player_spawns)
			sync_live_player_spawns_to_scenario();
		if (reverted || g_forge.edit_state.replace_netgame_flags)
			sync_live_netgame_flags_to_scenario();

		/* Re-apply any default scenario object deletions, moves, or extended properties on startup or after game_state_revert() */
		for (i = 0; i < (int)g_forge.edit_state.default_edit_count; i++)
		{
			struct forge_default_object_edit *de = &g_forge.edit_state.default_edits[i];
			long found_oidx;
			int li, already_handled = 0;

			if (!de->active)
				continue;

			if (!reverted)
			{
				for (li = 0; li < g_forge.live_instance_count; li++)
				{
					if (g_forge.live_instances[li].is_default_scenario_object &&
						g_forge.live_instances[li].edit_entry_index == i)
					{
						if (de->deleted ||
							(g_forge.live_instances[li].active &&
							 forge_valid_object_get(g_forge.live_instances[li].object_index) != NULL))
						{
							already_handled = 1;
							break;
						}
					}
				}
				if (already_handled)
					continue;
			}

			found_oidx = find_matching_default_world_object(
				de->group_tag, de->tag_path, de->orig_x, de->orig_y, de->orig_z, i);
			if (found_oidx != NONE)
			{
				if (de->deleted)
				{
					int has_li = 0;
					for (li = 0; li < g_forge.live_instance_count; li++)
					{
						if (g_forge.live_instances[li].is_default_scenario_object &&
							g_forge.live_instances[li].edit_entry_index == i)
						{
							g_forge.live_instances[li].active = 0;
							g_forge.live_instances[li].object_index = NONE;
							has_li = 1;
							break;
						}
					}
					if (!has_li && g_forge.live_instance_count < FORGE_MAX_LIVE_INSTANCES)
					{
						struct forge_live_instance *dinst = &g_forge.live_instances[g_forge.live_instance_count++];
						memset(dinst, 0, sizeof(*dinst));
						dinst->active = 0;
						dinst->object_index = NONE;
						dinst->actor_index = NONE;
						dinst->edit_entry_index = i;
						dinst->group_tag = de->group_tag;
						dinst->is_default_scenario_object = 1;
						dinst->orig_x = de->orig_x;
						dinst->orig_y = de->orig_y;
						dinst->orig_z = de->orig_z;
						snprintf(dinst->tag_path, sizeof(dinst->tag_path), "%s", de->tag_path);
					}
					object_delete(found_oidx);
				}
				else
				{
					int32_t li_idx = forge_3d_adopt_or_find_object_instance(found_oidx);
					if (li_idx >= 0)
					{
						struct forge_live_instance *linst = &g_forge.live_instances[li_idx];
						linst->edit_entry_index = i;
						linst->gametype_flags = de->gametype_flags;
						linst->respawn_timer_sec = de->respawn_timer_sec;
						linst->physics_mode = de->physics_mode;
						linst->group_id = de->group_id;
						linst->light_color_preset = de->light_color_preset;
						linst->light_intensity_preset = de->light_intensity_preset;
						linst->light_radius_preset = de->light_radius_preset;
						linst->spawned_once = 1;
						linst->death_tick = -1;
						forge_3d_move_instance_ext(
							li_idx,
							de->new_x, de->new_y, de->new_z,
							de->new_yaw, de->new_pitch, de->new_roll,
							0, 0);
						forge_apply_instance_light_settings(li_idx);
					}
				}
			}
		}
	}

	for (i = 0; i < g_forge.live_instance_count; i++)
	{
		struct forge_live_instance *inst = &g_forge.live_instances[i];
		struct object_datum *obj;
		uint32_t actual_grp;
		long def_idx;
		int is_held_item = (i == g_forge.grabbed_live_index || forge_is_instance_held_by_weld_drag(i));

		if (inst->group_tag == 'spwn' || inst->group_tag == 'tele' || inst->group_tag == 'flag')
			continue;

		if (inst->is_default_scenario_object)
		{
			if (reverted)
			{
				obj = forge_valid_object_get(inst->object_index);
				if (!obj && inst->active)
				{
					long found_oidx = find_matching_default_world_object(
						inst->group_tag, inst->tag_path, inst->orig_x, inst->orig_y, inst->orig_z, inst->edit_entry_index);
					if (found_oidx != NONE)
					{
						inst->object_index = found_oidx;
						obj = forge_valid_object_get(inst->object_index);
					}
				}
				if (obj)
				{
					if (!inst->active)
					{
						object_delete(inst->object_index);
						inst->object_index = NONE;
					}
					else
					{
						inst->death_tick = -1;
						inst->spawned_once = 1;
						forge_3d_move_instance_ext(
							i,
							inst->x, inst->y, inst->z,
							inst->yaw, inst->pitch, inst->roll,
							0, 0);
						forge_apply_instance_light_settings(i);
					}
				}
			}
			else if (inst->active && !is_held_item && inst->respawn_timer_sec > 0 &&
				(inst->group_tag == 'weap' || inst->group_tag == 'eqip' || inst->group_tag == 'vehi'))
			{
				/* Default scenario weapon/equipment/vehicle with explicit Forge respawn timer */
				obj = forge_valid_object_get(inst->object_index);
				if (inst->group_tag == 'weap' || inst->group_tag == 'eqip')
				{
					if (obj && obj->object.parent_object_index != NONE)
					{
						SET_FLAG(obj->object.flags, _object_cannot_be_garbage_bit, FALSE);
						inst->object_index = NONE;
						obj = NULL;
					}
					if (!obj)
					{
						if (inst->death_tick < 0)
						{
							inst->death_tick = cur_tick;
						}
						else if (cur_tick - inst->death_tick >= (long)inst->respawn_timer_sec * 30L)
						{
							actual_grp = inst->group_tag;
							def_idx = forge_resolve_engine_tag(inst->group_tag, inst->tag_path, &actual_grp);
							inst->death_tick = -1;
							if (def_idx != NONE)
							{
								inst->object_index = forge_create_configured_engine_object(
									def_idx, inst->group_tag,
									inst->x, inst->y, inst->z,
									inst->yaw, inst->pitch, inst->roll);
								forge_3d_move_instance_ext(
									i, inst->x, inst->y, inst->z, inst->yaw, inst->pitch, inst->roll, 0, 0);
								forge_apply_instance_light_settings(i);
								if (game_connection() == _game_connection_network_server)
								{
									forge_network_send_spawn_ext(
										i, inst->group_tag, inst->tag_path, inst->source_cache_id,
										inst->x, inst->y, inst->z, inst->yaw, inst->pitch, inst->roll,
										inst->team_designator, inst->object_index, def_idx);
									forge_network_send_object_props(i);
								}
							}
						}
					}
					else
					{
						inst->death_tick = -1;
					}
				}
				else if (inst->group_tag == 'vehi')
				{
					int need_timer = 0;
					if (!obj || TEST_FLAG(obj->object.damage_flags, _object_dead_bit))
					{
						need_timer = 1;
					}
					else if (!forge_is_vehicle_occupied(inst->object_index))
					{
						float dx = obj->object.position.x - inst->x;
						float dy = obj->object.position.y - inst->y;
						float dz = obj->object.position.z - inst->z;
						if (dx * dx + dy * dy + dz * dz > 6.25f)
							need_timer = 1;
					}
					if (need_timer)
					{
						if (inst->death_tick < 0)
						{
							inst->death_tick = cur_tick;
						}
						else if (cur_tick - inst->death_tick >= (long)inst->respawn_timer_sec * 30L)
						{
							if (obj)
								object_delete(inst->object_index);
							inst->object_index = NONE;
							inst->death_tick = -1;
							actual_grp = inst->group_tag;
							def_idx = forge_resolve_engine_tag(inst->group_tag, inst->tag_path, &actual_grp);
							if (def_idx != NONE)
							{
								inst->object_index = forge_create_configured_engine_object(
									def_idx, inst->group_tag,
									inst->x, inst->y, inst->z,
									inst->yaw, inst->pitch, inst->roll);
								forge_3d_move_instance_ext(
									i, inst->x, inst->y, inst->z, inst->yaw, inst->pitch, inst->roll, 0, 0);
								forge_apply_instance_light_settings(i);
								if (game_connection() == _game_connection_network_server)
								{
									forge_network_send_spawn_ext(
										i, inst->group_tag, inst->tag_path, inst->source_cache_id,
										inst->x, inst->y, inst->z, inst->yaw, inst->pitch, inst->roll,
										inst->team_designator, inst->object_index, def_idx);
									forge_network_send_object_props(i);
								}
							}
						}
					}
					else
					{
						inst->death_tick = -1;
					}
				}
			}
			else if (inst->active && inst->edit_entry_index >= 0 && inst->object_index != NONE)
			{
				obj = forge_valid_object_get(inst->object_index);
				if (obj && obj->object.parent_object_index == NONE)
				{
					forge_ensure_object_connected_to_bsp(
						inst->object_index,
						obj->object.position.x,
						obj->object.position.y,
						obj->object.position.z);
				}
			}
			continue;
		}

		if (!inst->active)
			continue;

		if (inst->is_ai_unit)
		{
			struct unit_datum *ai_u = (struct unit_datum *)forge_valid_object_get(inst->object_index);
			if (reverted)
			{
				int32_t spawned_unit = NONE;
				inst->death_tick = -1;
				inst->actor_index = forge_ai_spawn_unit_live(
					inst->tag_path, inst->x, inst->y, inst->z, inst->yaw, inst->team_designator, &spawned_unit);
				inst->object_index = spawned_unit;
				if (inst->ai_order != 0 && inst->object_index != NONE)
				{
					forge_ai_apply_order(
						inst->actor_index, inst->object_index, inst->ai_order,
						inst->x, inst->y, inst->z, cur_tick);
				}
			}
			else if (!is_held_item)
			{
				int is_dead = (!ai_u || ai_u->object.body_vitality <= 0.0f ||
							   TEST_FLAG(ai_u->object.damage_flags, _object_dead_bit));
				if (is_dead)
				{
					if (inst->respawn_timer_sec > 0)
					{
						if (inst->death_tick < 0)
						{
							inst->death_tick = cur_tick;
						}
						else if (cur_tick - inst->death_tick >= (long)inst->respawn_timer_sec * 30L)
						{
							int32_t spawned_unit = NONE;
							if (ai_u)
								object_delete(inst->object_index);
							inst->death_tick = -1;
							inst->actor_index = forge_ai_spawn_unit_live(
								inst->tag_path, inst->x, inst->y, inst->z, inst->yaw, inst->team_designator, &spawned_unit);
							inst->object_index = spawned_unit;
							if (inst->ai_order != 0 && inst->object_index != NONE)
							{
								forge_ai_apply_order(
									inst->actor_index, inst->object_index, inst->ai_order,
									inst->x, inst->y, inst->z, cur_tick);
							}
						}
					}
				}
				else
				{
					inst->death_tick = -1;
					if (inst->ai_order != 0 && ((cur_tick % 15L) == (i % 15L)))
					{
						forge_ai_apply_order(
							inst->actor_index, inst->object_index, inst->ai_order,
							inst->x, inst->y, inst->z, cur_tick);
					}
				}
			}
			continue;
		}

		obj = forge_valid_object_get(inst->object_index);

		/* Handle placed Weapon / Equipment / Vehicle Respawn Timers after initial spawn */
		if (!reverted && inst->spawned_once && !is_held_item)
		{
			if (inst->group_tag == 'weap' || inst->group_tag == 'eqip')
			{
				long delay_sec = (inst->respawn_timer_sec > 0) ? (long)inst->respawn_timer_sec : 30L;
				if (obj && obj->object.parent_object_index != NONE)
				{
					/* Picked up into a player's or AI's inventory: release slot and start respawn pad timer */
					SET_FLAG(obj->object.flags, _object_cannot_be_garbage_bit, FALSE);
					inst->object_index = NONE;
					obj = NULL;
				}
				if (!obj)
				{
					if (inst->death_tick < 0)
					{
						inst->death_tick = cur_tick;
						continue;
					}
					if (cur_tick - inst->death_tick < delay_sec * 30L)
						continue;
					inst->death_tick = -1;
				}
				else
				{
					inst->death_tick = -1;
				}
			}
			else if (inst->group_tag == 'vehi')
			{
				if (inst->respawn_timer_sec <= 0)
				{
					/* 0 = Never auto-respawn displaced/destroyed vehicles unless game_state_revert() */
					if (!obj)
						continue;
				}
				else
				{
					int need_timer = 0;
					if (!obj || TEST_FLAG(obj->object.damage_flags, _object_dead_bit))
					{
						need_timer = 1;
					}
					else if (!forge_is_vehicle_occupied(inst->object_index))
					{
						float dx = obj->object.position.x - inst->x;
						float dy = obj->object.position.y - inst->y;
						float dz = obj->object.position.z - inst->z;
						if (dx * dx + dy * dy + dz * dz > 6.25f)
							need_timer = 1;
					}
					if (need_timer)
					{
						if (inst->death_tick < 0)
						{
							inst->death_tick = cur_tick;
							if (!obj)
								continue;
						}
						else if (cur_tick - inst->death_tick >= (long)inst->respawn_timer_sec * 30L)
						{
							if (obj)
								object_delete(inst->object_index);
							inst->object_index = NONE;
							obj = NULL;
							inst->death_tick = -1;
						}
						else if (!obj)
						{
							continue;
						}
					}
					else
					{
						inst->death_tick = -1;
					}
				}
			}
		}

		if (reverted || !obj)
		{
			actual_grp = inst->group_tag;
			def_idx = forge_resolve_engine_tag(inst->group_tag, inst->tag_path, &actual_grp);
			if (obj && def_idx != NONE && obj->definition_index != def_idx)
			{
				char const *oname = (obj->definition_index != NONE) ? tag_get_name(obj->definition_index) : NULL;
				int is_baked_mp_vehi = (inst->group_tag == 'vehi' && obj->object.type == _object_type_vehicle) ||
					(oname && _strnicmp(oname, "forge\\vehicles\\placed_unit_", 27) == 0);
				if (!is_baked_mp_vehi)
				{
					/* Slot was overwritten or reverted to a different object by game_state_revert() */
					obj = NULL;
					inst->object_index = NONE;
				}
			}
			if (!obj && def_idx != NONE)
			{
				inst->object_index = forge_create_configured_engine_object(
					def_idx,
					inst->group_tag,
					inst->x, inst->y, inst->z,
					inst->yaw, inst->pitch, inst->roll);
				obj = forge_valid_object_get(inst->object_index);
				if (obj)
				{
					inst->spawned_once = 1;
					inst->death_tick = -1;
					forge_3d_move_instance_ext(
						i,
						inst->x, inst->y, inst->z,
						inst->yaw, inst->pitch, inst->roll,
						is_held_item ? 1 : 0,
						0);
					forge_apply_instance_light_settings(i);
				}
				if (obj && i == g_forge.grabbed_live_index)
					g_forge.grabbed_object_index = inst->object_index;
				if (obj && game_connection() == _game_connection_network_server && !inst->is_default_scenario_object)
				{
					forge_network_send_spawn_ext(
						i,
						inst->group_tag,
						inst->tag_path,
						inst->source_cache_id,
						inst->x, inst->y, inst->z,
						inst->yaw, inst->pitch, inst->roll,
						inst->team_designator,
						inst->object_index,
						def_idx);
					forge_network_send_object_props(i);
				}
			}
		}

		if (obj)
		{
			inst->spawned_once = 1;
			if (reverted && obj->object.parent_object_index == NONE)
			{
				forge_3d_move_instance_ext(
					i,
					inst->x, inst->y, inst->z,
					inst->yaw, inst->pitch, inst->roll,
					is_held_item ? 1 : 0,
					0);
				forge_apply_instance_light_settings(i);
			}
			else if (obj->object.parent_object_index == NONE)
			{
				forge_ensure_object_connected_to_bsp(
					inst->object_index,
					obj->object.position.x,
					obj->object.position.y,
					obj->object.position.z);
			}
		}
	}
}

static int s_init_wait_ticks = 0;
static struct scenario *s_last_scen = NULL;
static long s_player_last_unit_index[128];
static int s_player_last_unit_init = 0;

void forge_3d_reset_session_tracking(void)
{
	int k;
	s_init_wait_ticks = 0;
	s_last_scen = NULL;
	s_last_game_tick = -1;
	s_last_tick_scen = NULL;
	s_has_valid_spartan_pos = 0;
	s_weld_follower_count = 0;
	for (k = 0; k < 128; k++)
		s_player_last_unit_index[k] = NONE;
	s_player_last_unit_init = 1;
}

#define FORGE_MAX_PROP_ROWS 16

enum forge_prop_row_id
{
	FORGE_PROP_TEAM_OR_CHANNEL = 0,
	FORGE_PROP_PHYSICS_MODE,
	FORGE_PROP_RESPAWN_TIMER,
	FORGE_PROP_AI_ORDER,
	FORGE_PROP_LIGHT_COLOR,
	FORGE_PROP_LIGHT_INTENSITY,
	FORGE_PROP_LIGHT_RADIUS,
	FORGE_PROP_WELD_GROUP,
	FORGE_PROP_GAMETYPE_MODE,
	FORGE_PROP_ROT_SNAP,
	FORGE_PROP_DROP_TO_FLOOR,
	FORGE_PROP_DELETE_OBJECT,
	FORGE_PROP_SAVE_TO_CACHE
};

static int forge_build_object_prop_rows(
	int32_t live_idx,
	uint8_t *out_ids,
	char labels[][40],
	char values[][48])
{
	static char const *const s_ai_teams[4] = { "FRIENDLY", "COVENANT", "FLOOD", "SENTINEL" };
	static char const *const s_phys_names[3] = { "NORMAL", "FIXED", "PHASED" };
	static char const *const s_ai_orders[5] = { "DEFAULT", "GUARD POS", "PATROL", "CHARGE", "ENTER VEHICLE" };
	static char const *const s_light_colors[8] = {
		"DEFAULT", "WHITE", "RED", "BLUE", "GREEN", "AMBER", "PURPLE", "CYAN"
	};
	static char const *const s_light_intensities[6] = {
		"100%", "25%", "50%", "150%", "200%", "300%"
	};
	static char const *const s_light_radii[6] = {
		"DEFAULT", "2U", "4U", "8U", "12U", "20U"
	};
	static char const *const s_snap_names[4] = { "OFF", "15 DEG", "45 DEG", "90 DEG" };
	struct forge_live_instance const *inst;
	int n = 0;

	if (live_idx < 0 || live_idx >= g_forge.live_instance_count)
		return 0;
	inst = &g_forge.live_instances[live_idx];
	if (!inst->active)
		return 0;

	/* 1. Team / Order / Channel */
	out_ids[n] = FORGE_PROP_TEAM_OR_CHANNEL;
	if (inst->is_ai_unit)
	{
		int t_idx = (inst->team_designator >= 0 && inst->team_designator <= 3) ? (int)inst->team_designator : 0;
		snprintf(labels[n], 40, "TEAM:");
		snprintf(values[n], 48, "%s", s_ai_teams[t_idx]);
	}
	else if (inst->group_tag == 'spwn')
	{
		snprintf(labels[n], 40, "TEAM:");
		snprintf(values[n], 48, "%s",
			(inst->team_designator == 1) ? "BLUE TEAM" : (inst->team_designator == 2 ? "FFA" : "RED TEAM"));
	}
	else if (inst->group_tag == 'flag' &&
		(inst->netgame_flag_type == _netgame_flag_ctf_flag || inst->netgame_flag_type == _netgame_flag_ctf_vehicle))
	{
		snprintf(labels[n], 40, "TEAM:");
		snprintf(values[n], 48, "%s", inst->team_designator == 1 ? "BLUE TEAM" : "RED TEAM");
	}
	else if (inst->group_tag == 'flag' &&
		(inst->netgame_flag_type == _netgame_flag_race_track || inst->netgame_flag_type == _netgame_flag_race_vehicle))
	{
		snprintf(labels[n], 40, "ORDER:");
		snprintf(values[n], 48, "#%d", (int)inst->team_designator);
	}
	else
	{
		snprintf(labels[n], 40, "CHANNEL:");
		snprintf(values[n], 48, "#%d", (int)inst->team_designator);
	}
	n++;

	/* 2. Physics Mode (for physical non-AI objects) */
	if (!inst->is_ai_unit &&
		inst->group_tag != 'spwn' &&
		inst->group_tag != 'tele' &&
		inst->group_tag != 'flag')
	{
		int pm = (inst->physics_mode < NUMBER_OF_FORGE_PHYSICS_MODES) ? (int)inst->physics_mode : 0;
		out_ids[n] = FORGE_PROP_PHYSICS_MODE;
		snprintf(labels[n], 40, "PHYSICS:");
		snprintf(values[n], 48, "%s", s_phys_names[pm]);
		n++;
	}

	/* 3. Respawn Timer (AI, Weapons, Equipment, Vehicles) */
	if (inst->is_ai_unit ||
		inst->group_tag == 'weap' ||
		inst->group_tag == 'eqip' ||
		inst->group_tag == 'vehi')
	{
		out_ids[n] = FORGE_PROP_RESPAWN_TIMER;
		snprintf(labels[n], 40, "RESPAWN:");
		if (inst->respawn_timer_sec <= 0)
		{
			snprintf(values[n], 48, "%s",
				(inst->group_tag == 'weap' || inst->group_tag == 'eqip') ? "DEFAULT (30S)" : "NEVER");
		}
		else
		{
			snprintf(values[n], 48, "%d SEC", (int)inst->respawn_timer_sec);
		}
		n++;
	}

	/* 4. AI Order (AI units) */
	if (inst->is_ai_unit)
	{
		int ao = (inst->ai_order < NUMBER_OF_FORGE_AI_ORDERS) ? (int)inst->ai_order : 0;
		out_ids[n] = FORGE_PROP_AI_ORDER;
		snprintf(labels[n], 40, "AI ORDER:");
		snprintf(values[n], 48, "%s", s_ai_orders[ao]);
		n++;
	}

	/* 5. Dynamic Light Properties (lifi, Lights category, or objects with light attachments) */
	if (forge_instance_supports_light_props(live_idx))
	{
		int lc = (inst->light_color_preset < NUMBER_OF_FORGE_LIGHT_COLORS) ? (int)inst->light_color_preset : 0;
		int li = (inst->light_intensity_preset < NUMBER_OF_FORGE_LIGHT_INTENSITIES) ? (int)inst->light_intensity_preset : 0;
		int lr = (inst->light_radius_preset < NUMBER_OF_FORGE_LIGHT_RADII) ? (int)inst->light_radius_preset : 0;

		out_ids[n] = FORGE_PROP_LIGHT_COLOR;
		snprintf(labels[n], 40, "LIGHT COLOR:");
		snprintf(values[n], 48, "%s", s_light_colors[lc]);
		n++;

		out_ids[n] = FORGE_PROP_LIGHT_INTENSITY;
		snprintf(labels[n], 40, "INTENSITY:");
		snprintf(values[n], 48, "%s", s_light_intensities[li]);
		n++;

		out_ids[n] = FORGE_PROP_LIGHT_RADIUS;
		snprintf(labels[n], 40, "LIGHT RADIUS:");
		snprintf(values[n], 48, "%s", s_light_radii[lr]);
		n++;
	}

	/* 6. Weld Group (1..16) */
	if (!g_forge.koth_group_placing)
	{
		out_ids[n] = FORGE_PROP_WELD_GROUP;
		snprintf(labels[n], 40, "WELD GROUP:");
		if (inst->group_id >= 1 && inst->group_id <= FORGE_MAX_WELD_GROUPS)
			snprintf(values[n], 48, "GROUP %d", (int)inst->group_id);
		else
			snprintf(values[n], 48, "NONE");
		n++;
	}

	/* 7. Gametype Spawn Mode */
	if (!inst->is_ai_unit)
	{
		out_ids[n] = FORGE_PROP_GAMETYPE_MODE;
		snprintf(labels[n], 40, "GAME MODE:");
		snprintf(values[n], 48, "%s", gametype_flags_label(inst->gametype_flags));
		n++;
	}

	/* 8. Rotation Snap */
	out_ids[n] = FORGE_PROP_ROT_SNAP;
	snprintf(labels[n], 40, "ROT SNAP:");
	snprintf(values[n], 48, "%s", s_snap_names[g_forge.snap_mode]);
	n++;

	/* 9. Drop to Floor */
	out_ids[n] = FORGE_PROP_DROP_TO_FLOOR;
	snprintf(labels[n], 40, "%s DROP TO FLOOR", forge_action_prompt(FORGE_BIND_SNAP_FLOOR));
	values[n][0] = 0;
	n++;

	/* 10. Delete Object */
	out_ids[n] = FORGE_PROP_DELETE_OBJECT;
	snprintf(labels[n], 40, "%s DELETE OBJECT", forge_action_prompt(FORGE_BIND_DELETE_OBJECT));
	values[n][0] = 0;
	n++;

	/* 11. Save Map-Local Tag to Forge Cache */
	if (forge_is_tag_map_local_only(inst->group_tag, inst->tag_path))
	{
		out_ids[n] = FORGE_PROP_SAVE_TO_CACHE;
		snprintf(labels[n], 40, "SAVE TO FORGE CACHE");
		values[n][0] = 0;
		n++;
	}

	return n;
}

void forge_3d_update(float dt)
{
	static int prev_f1, prev_f2, prev_f4, prev_f5, prev_f7, prev_f8, prev_esc, prev_x, prev_bk, prev_lmb;
	static int prev_up, prev_down, prev_left, prev_right, prev_enter, prev_g, prev_z, prev_f, prev_del, prev_rot, prev_copy;
	static int prev_undo, prev_redo, prev_weld;
	static int32_t s_hover_rot_live_idx = -1;
	static struct forge_live_instance s_hover_rot_orig_piv;
	static struct forge_live_instance s_props_orig_snap;
	static int32_t s_props_orig_live_idx = -1;
	static uint32_t s_last_stick_nav_ms = 0;
	static int s_prev_modal = 0;
	struct scenario *scen;
	int cur_f1, cur_f2, cur_f4, cur_f5, cur_f7, cur_f8, cur_esc, cur_x, cur_bk, cur_lmb, cur_rot;
	int cur_up, cur_down, cur_left, cur_right, cur_enter, cur_g, cur_z, cur_f, cur_del, cur_copy;
	int ctrl_down, cur_undo, cur_redo, cur_weld;
	float gp_lx = 0.0f, gp_ly = 0.0f, gp_rx = 0.0f, gp_ry = 0.0f, gp_lt = 0.0f, gp_rt = 0.0f;

	if (!s_player_last_unit_init)
		forge_3d_reset_session_tracking();

	if (g_forge_session_enabled)
	{
		forge_runtime_init();
		forge_poll_controller_state(&gp_lx, &gp_ly, &gp_rx, &gp_ry, &gp_lt, &gp_rt);
	}

	if (g_forge.active_modal == _forge_modal_save_copy_as && s_prev_modal != _forge_modal_save_copy_as)
		s_save_modal_just_opened = 1;

	/* Record Undo transaction if closing X Object Properties after modifying properties */
	if (s_prev_modal == _forge_modal_object_properties &&
		g_forge.active_modal != _forge_modal_object_properties &&
		s_props_orig_live_idx >= 0 &&
		s_props_orig_live_idx < g_forge.live_instance_count &&
		g_forge.live_instances[s_props_orig_live_idx].active)
	{
		struct forge_live_instance const *cur_p = &g_forge.live_instances[s_props_orig_live_idx];
		if (cur_p->team_designator != s_props_orig_snap.team_designator ||
			cur_p->gametype_flags != s_props_orig_snap.gametype_flags ||
			cur_p->physics_mode != s_props_orig_snap.physics_mode ||
			cur_p->respawn_timer_sec != s_props_orig_snap.respawn_timer_sec ||
			cur_p->ai_order != s_props_orig_snap.ai_order ||
			cur_p->group_id != s_props_orig_snap.group_id ||
			cur_p->light_color_preset != s_props_orig_snap.light_color_preset ||
			cur_p->light_intensity_preset != s_props_orig_snap.light_intensity_preset ||
			cur_p->light_radius_preset != s_props_orig_snap.light_radius_preset)
		{
			uint32_t tx_id = forge_undo_begin_tx();
			forge_undo_push_record(tx_id, _undo_op_props, s_props_orig_live_idx, &s_props_orig_snap, cur_p);
		}
		s_props_orig_live_idx = -1;
	}
	else if (g_forge.active_modal == _forge_modal_object_properties &&
		s_prev_modal != _forge_modal_object_properties &&
		g_forge.grabbed_live_index >= 0 &&
		g_forge.grabbed_live_index < g_forge.live_instance_count)
	{
		s_props_orig_live_idx = g_forge.grabbed_live_index;
		s_props_orig_snap = g_forge.live_instances[g_forge.grabbed_live_index];
	}
	s_prev_modal = g_forge.active_modal;

	if (forge_get_active_map_name() && strcmp(forge_get_active_map_name(), "levels\\ui\\ui") == 0)
	{
		forge_3d_reset_session_tracking();
		return;
	}
	scen = global_scenario_try_and_get();
	if (!scen || scen->type == 2 /* _scenario_type_main_menu */)
	{
		forge_3d_reset_session_tracking();
		return;
	}

	/* Ensure Forge runtime & [C] .fbrtag custom tag catalogs are initialized in BOTH Forge and Standard Gamemodes */
	forge_runtime_init();
	if (g_forge.active_weld_group < 1 || g_forge.active_weld_group > FORGE_MAX_WELD_GROUPS)
		g_forge.active_weld_group = 1;

	if (scen != s_last_scen)
	{
		s_last_scen = scen;
		s_init_wait_ticks = 0;
		s_has_valid_spartan_pos = 0;
		g_forge.monitor_cam_initialized = 0;
		forge_undo_clear();
		if (g_forge.initialized)
			forge_runtime_reload_catalogs();
		if (g_forge.live_instance_count == 0)
			forge_init_map_scenario_entities();
		forge_load_saved_map_state_if_present(forge_get_active_map_name());
	}

	/* Only run Forge editor controls when inside a Forge session; in Standard Mode, still reconcile saved map edits & MP sync */
	if (!g_forge_session_enabled)
	{
		if (g_forge.has_saved_map_once ||
			g_forge.save_map_name[0] != 0 ||
			g_forge.edit_state.placed_object_count > 0 ||
			g_forge.edit_state.default_edit_count > 0 ||
			g_forge.edit_state.ai_spawn_count > 0 ||
			g_forge.edit_state.replace_player_spawns ||
			g_forge.edit_state.replace_netgame_flags ||
			forge_is_multiplayer_session())
		{
			long unit_idx = player_control_get_unit_index(0);
			forge_network_sync_tick();
			forge_reconcile_live_instances();
			forge_enforce_runtime_object_properties();
			if (unit_idx != NONE)
			{
				struct unit_datum *u = (struct unit_datum *)object_try_and_get_and_verify_type(unit_idx, _object_mask_unit);
				if (u && u->object.parent_object_index != NONE)
				{
					struct unit_datum *veh = (struct unit_datum *)object_try_and_get_and_verify_type(
						u->object.parent_object_index, _object_mask_vehicle);
					if (veh)
					{
						veh->unit.seat_power[0] = 1.0f;
						veh->unit.seat_power[1] = 1.0f;
					}
				}
			}
		}
		return;
	}

	if (g_forge.live_instance_count == 0 && (scen->players.count > 0 || scen->netgame_flags.count > 0))
	{
		forge_init_map_scenario_entities();
		forge_load_saved_map_state_if_present(forge_get_active_map_name());
	}

	/* On Singleplayer / Campaign maps (scen->type == 0), ensure all players and their controlled units
	 * are on _game_team_player (1) and allied with _game_team_human (2, Campaign Marines/Keyes) so
	 * friendly AI never treat the player as an enemy when entering or switching Forge mode. */
	if (scen->type == 0 /* _scenario_type_single_player */)
	{
		extern boolean game_team_is_enemy(short team1_index, short team2_index);
		extern void game_allegiance_create(
			short team1_index, boolean team1_suspicious,
			short team2_index, boolean team2_suspicious,
			short incident_threshold, short suspicious_threshold,
			boolean locked);
		if (player_data)
		{
			long p_idx;
			for (p_idx = data_next_index(player_data, NONE); p_idx != NONE; p_idx = data_next_index(player_data, p_idx))
			{
				struct player_datum *pl = player_get(p_idx);
				if (pl)
				{
					pl->team_index = 1; /* _game_team_player */
					if (pl->unit_index != NONE)
					{
						struct unit_datum *pu = (struct unit_datum *)object_try_and_get_and_verify_type(pl->unit_index, _object_mask_unit);
						if (pu)
							pu->object.owner_team_index = 1; /* _game_team_player */
					}
				}
			}
		}
		if (game_team_is_enemy(1 /* _game_team_player */, 2 /* _game_team_human */))
		{
			game_allegiance_create(1, FALSE, 2, FALSE, NONE, NONE, FALSE);
		}
	}

	/* If a campaign cutscene is active, exit Monitor mode cleanly and let the cinematic director run undisturbed */
	{
		extern boolean cinematic_in_progress(void);
		if (cinematic_in_progress())
		{
			if (g_forge.grabbed_live_index >= 0)
				forge_drop_or_place_held_item();
			g_forge.active_modal = _forge_modal_none;
			if (g_forge.mode == _forge_mode_3d_monitor)
			{
				int local_p_abs = forge_get_local_player_abs_index();
				long unit_idx = player_control_get_unit_index(0);
				g_forge.mode = _forge_mode_play;
				if (local_p_abs >= 0 && local_p_abs < 128)
				{
					g_forge.player_is_monitor[local_p_abs] = 0;
					g_forge.player_has_monitor_pos[local_p_abs] = 0;
				}
				if (unit_idx != NONE)
				{
					struct unit_datum *u = (struct unit_datum *)object_try_and_get_and_verify_type(unit_idx, _object_mask_unit);
					if (u)
					{
						SET_FLAG(u->object.flags, _object_no_collisions_bit, FALSE);
						SET_FLAG(u->object.flags, _object_at_rest_bit, FALSE);
						SET_FLAG(u->object.damage_flags, _object_cannot_take_damage_bit, FALSE);
						forge_hide_spartan_linked_attachments_and_children(unit_idx, 0);
					}
				}
			}
			forge_network_sync_tick();
			forge_reconcile_live_instances();
			forge_enforce_runtime_object_properties();
			return;
		}
	}

	/* Point 4: When starting a game, spawn as the Spartan at a default player spawn point */
	if (!g_forge.monitor_cam_initialized)
	{
		long unit_idx = player_control_get_unit_index(0);
		real_point3d spawn_pos;
		float spawn_yaw = 0.0f;
		int ready = 0;

		s_init_wait_ticks++;
		if (unit_idx != NONE)
		{
			struct unit_datum *u = (struct unit_datum *)object_try_and_get_and_verify_type(unit_idx, _object_mask_unit);
			if (u)
			{
				real_point3d test_pt = u->object.position;
				test_pt.z += 0.3f;
				if (scenario_leaf_index_from_point(&test_pt) != NONE)
				{
					spawn_pos = u->object.position;
					spawn_yaw = (float)atan2((double)u->object.forward.j, (double)u->object.forward.i);
					ready = 1;
				}
				else if (find_valid_bsp_spawn_point(&spawn_pos, &spawn_yaw))
				{
					real_vector3d fwd, up;
					euler_to_forward_up(spawn_yaw, 0.0f, 0.0f, &fwd, &up);
					SET_FLAG(u->object.flags, _object_no_collisions_bit, FALSE);
					object_set_position(unit_idx, &spawn_pos, &fwd, &up);
					ready = 1;
				}
			}
		}
		else if (s_init_wait_ticks >= 4)
		{
			extern int halo_server_mode(void);
			if (find_valid_bsp_spawn_point(&spawn_pos, &spawn_yaw))
			{
				ready = 1;
			}
			else if (halo_server_mode())
			{
				spawn_pos.x = 0.0f;
				spawn_pos.y = 0.0f;
				spawn_pos.z = 0.0f;
				spawn_yaw = 0.0f;
				ready = 1;
			}
		}

		if (ready)
		{
			s_init_wait_ticks = 0;
			g_forge.cam_x = spawn_pos.x;
			g_forge.cam_y = spawn_pos.y;
			g_forge.cam_z = spawn_pos.z + 0.65f;
			g_forge.cam_yaw = spawn_yaw;
			g_forge.cam_pitch = 0.0f;
			g_forge.monitor_cam_initialized = 1;
			g_forge.mode = _forge_mode_play; /* Spawn as Spartan at default player spawn */
			if (forge_is_multiplayer_session())
			{
				forge_network_send_monitor_mode(
					0,
					g_forge.cam_x, g_forge.cam_y, g_forge.cam_z,
					g_forge.cam_yaw, g_forge.cam_pitch);
			}
			forge_set_status("%s Forge/Spartan   %s Quick Save",
				forge_action_prompt(FORGE_BIND_TOGGLE_MODE),
				forge_action_prompt(FORGE_BIND_QUICK_SAVE));
		}
		else
		{
			forge_network_sync_tick();
			forge_reconcile_live_instances();
			forge_enforce_runtime_object_properties();
			return;
		}
	}

	/* Run collaborative multiplayer sync (late-join state sync, client object adoption, Monitor state heartbeat) */
	forge_network_sync_tick();

	/* Reconcile live Forge instances so placed items never vanish after player death, checkpoint revert, or BSP surface placement */
	forge_reconcile_live_instances();

	/* Enforce Fixed/Phased physics locks & dynamic light settings on both Host and Clients */
	forge_enforce_runtime_object_properties();

	/* Ensure any vehicle driven by a player has powered seat_power[0] = 1.0f so flyers/hovercraft thrust works */
	{
		long unit_idx = player_control_get_unit_index(0);
		if (unit_idx != NONE)
		{
			struct unit_datum *u = (struct unit_datum *)object_try_and_get_and_verify_type(unit_idx, _object_mask_unit);
			if (u && u->object.parent_object_index != NONE)
			{
				struct unit_datum *veh = (struct unit_datum *)object_try_and_get_and_verify_type(
					u->object.parent_object_index, _object_mask_vehicle);
				if (veh)
				{
					veh->unit.seat_power[0] = 1.0f;
					veh->unit.seat_power[1] = 1.0f;
				}
			}
		}
	}

	/* Point 2 & Point 8: Always keep Monitor floating at cam_x/y/z, invulnerable, zero-velocity, and hide Spartan-linked
	 * green lights/weapons EVEN when F2 Forge Menu, Save Modal, or Escape Pause Menu is open! */
	if (g_forge.mode == _forge_mode_3d_monitor)
	{
		int local_p_abs = forge_get_local_player_abs_index();
		long unit_idx = player_control_get_unit_index(0);
		if (local_p_abs >= 0 && local_p_abs < 128)
		{
			g_forge.player_is_monitor[local_p_abs] = 1;
			g_forge.player_has_monitor_pos[local_p_abs] = 1;
			g_forge.player_monitor_pos[local_p_abs][0] = g_forge.cam_x;
			g_forge.player_monitor_pos[local_p_abs][1] = g_forge.cam_y;
			g_forge.player_monitor_pos[local_p_abs][2] = g_forge.cam_z;
			g_forge.player_monitor_yaw[local_p_abs] = g_forge.cam_yaw;
			g_forge.player_monitor_pitch[local_p_abs] = g_forge.cam_pitch;
		}
		if (unit_idx != NONE)
		{
			forge_sync_unit_to_monitor_pose(
				unit_idx,
				g_forge.cam_x, g_forge.cam_y, g_forge.cam_z,
				g_forge.cam_yaw, g_forge.cam_pitch, g_forge.cam_roll,
				1, 1, 0.0f);
		}
	}
	else
	{
		int local_p_abs = forge_get_local_player_abs_index();
		if (local_p_abs >= 0 && local_p_abs < 128)
		{
			g_forge.player_is_monitor[local_p_abs] = 0;
			g_forge.player_has_monitor_pos[local_p_abs] = 0;
		}
	}

	/* Per-player Monitor vs Spartan visibility, smooth remote Monitor interpolation & Host-side physics protection */
	if (player_data)
	{
		int local_p_abs = forge_get_local_player_abs_index();
		float lerp_t = (dt > 0.0f && dt < 0.25f) ? (1.0f - (float)exp((double)(-18.0f * dt))) : 0.35f;
		uint8_t slot_seen[128];
		int s_idx;
		long p_idx;

		memset(slot_seen, 0, sizeof(slot_seen));
		for (p_idx = data_next_index(player_data, NONE); p_idx != NONE; p_idx = data_next_index(player_data, p_idx))
		{
			struct player_datum *pl = player_get(p_idx);
			int p_abs = (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(p_idx);
			if (p_abs >= 0 && p_abs < 128 && pl && pl->unit_index != NONE)
			{
				slot_seen[p_abs] = 1;
				if (p_abs != local_p_abs && s_player_last_unit_index[p_abs] != pl->unit_index)
				{
					/* Remote player just joined, rejoined, or respawned into a new biped: reset Monitor state so
					 * the Host never teleports a newly rejoined player back to their pre-quit Monitor coordinates! */
					if (s_player_last_unit_index[p_abs] == NONE)
					{
						g_forge.player_is_monitor[p_abs] = 0;
						g_forge.player_has_monitor_pos[p_abs] = 0;
						g_forge.player_has_render_pos[p_abs] = 0;
					}
					s_player_last_unit_index[p_abs] = pl->unit_index;
				}
			}
		}
		for (s_idx = 0; s_idx < 128; s_idx++)
		{
			if (!slot_seen[s_idx] && s_idx != local_p_abs)
			{
				s_player_last_unit_index[s_idx] = NONE;
				g_forge.player_is_monitor[s_idx] = 0;
				g_forge.player_has_monitor_pos[s_idx] = 0;
				g_forge.player_has_render_pos[s_idx] = 0;
			}
		}

		for (p_idx = data_next_index(player_data, NONE); p_idx != NONE; p_idx = data_next_index(player_data, p_idx))
		{
			struct player_datum *pl = player_get(p_idx);
			if (pl && pl->unit_index != NONE)
			{
				int p_abs = (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(p_idx);
				int is_mon = forge_is_unit_in_monitor_mode(pl->unit_index);
				forge_hide_spartan_linked_attachments_and_children(pl->unit_index, is_mon ? 1 : 0);
				if (is_mon && p_abs != local_p_abs && p_abs >= 0 && p_abs < 128 &&
					g_forge.player_has_monitor_pos[p_abs])
				{
					if (!g_forge.player_has_render_pos[p_abs])
					{
						g_forge.player_has_render_pos[p_abs] = 1;
						g_forge.player_monitor_render_pos[p_abs][0] = g_forge.player_monitor_pos[p_abs][0];
						g_forge.player_monitor_render_pos[p_abs][1] = g_forge.player_monitor_pos[p_abs][1];
						g_forge.player_monitor_render_pos[p_abs][2] = g_forge.player_monitor_pos[p_abs][2];
						g_forge.player_monitor_render_yaw[p_abs] = g_forge.player_monitor_yaw[p_abs];
						g_forge.player_monitor_render_pitch[p_abs] = g_forge.player_monitor_pitch[p_abs];
					}
					else
					{
						float dyaw = g_forge.player_monitor_yaw[p_abs] - g_forge.player_monitor_render_yaw[p_abs];
						float dpitch = g_forge.player_monitor_pitch[p_abs] - g_forge.player_monitor_render_pitch[p_abs];
						while (dyaw > 3.14159265f) dyaw -= 6.2831853f;
						while (dyaw < -3.14159265f) dyaw += 6.2831853f;
						g_forge.player_monitor_render_pos[p_abs][0] += (g_forge.player_monitor_pos[p_abs][0] - g_forge.player_monitor_render_pos[p_abs][0]) * lerp_t;
						g_forge.player_monitor_render_pos[p_abs][1] += (g_forge.player_monitor_pos[p_abs][1] - g_forge.player_monitor_render_pos[p_abs][1]) * lerp_t;
						g_forge.player_monitor_render_pos[p_abs][2] += (g_forge.player_monitor_pos[p_abs][2] - g_forge.player_monitor_render_pos[p_abs][2]) * lerp_t;
						g_forge.player_monitor_render_yaw[p_abs] += dyaw * lerp_t;
						g_forge.player_monitor_render_pitch[p_abs] += dpitch * lerp_t;
					}

					/* Sync remote Monitor unit pose every frame on BOTH Host and Client with 0.0f distance threshold
					 * so the hidden biped (and overhead player name tag) never falls under gravity and snaps back! */
					forge_sync_unit_to_monitor_pose(
						pl->unit_index,
						g_forge.player_monitor_render_pos[p_abs][0],
						g_forge.player_monitor_render_pos[p_abs][1],
						g_forge.player_monitor_render_pos[p_abs][2],
						g_forge.player_monitor_render_yaw[p_abs],
						g_forge.player_monitor_render_pitch[p_abs],
						0.0f,
						1, 1, 0.0f);
				}
			}
		}
	}

	/* When the native in-game Escape Pause Menu is open, pause Forge flight/editor input */
	if (ui_widgets_active())
		return;

	/* Validate grabbed_object_index each tick so a despawned/recreated object never leaves a stale handle */
	if (g_forge.grabbed_object_index != NONE && !forge_valid_object_get(g_forge.grabbed_object_index))
	{
		if (g_forge.grabbed_live_index >= 0 && g_forge.grabbed_live_index < g_forge.live_instance_count)
			g_forge.live_instances[g_forge.grabbed_live_index].object_index = NONE;
		g_forge.grabbed_object_index = NONE;
	}

	cur_f1 = forge_action_down(FORGE_BIND_TOGGLE_MODE);
	cur_f2 = forge_action_down(FORGE_BIND_PALETTE);
	cur_f4 = forge_action_down(FORGE_BIND_THIRD_PERSON);
	cur_f5 = forge_action_down(FORGE_BIND_QUICK_SAVE);
	cur_f7 = (GetAsyncKeyState(0x76) & 0x8000) != 0; /* VK_F7: Switch Structure BSP on multi-BSP maps */
	cur_f8 = (GetAsyncKeyState(0x77) & 0x8000) != 0; /* VK_F8 */
	cur_esc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
	cur_x = forge_action_down(FORGE_BIND_OBJECT_OPTIONS);
	cur_bk = ((GetAsyncKeyState(VK_BACK) & 0x8000) != 0) || forge_gp_button_down(FORGE_GP_B);
	cur_lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

	/* Multi-BSP Switcher (F7 in Monitor Mode on campaign/multi-BSP maps) */
	if (cur_f7 && !prev_f7 && g_forge.mode == _forge_mode_3d_monitor &&
		g_forge.active_modal == _forge_modal_none && scen && scen->structure_bsp_references.count > 1)
	{
		short cur_bsp = global_structure_bsp_index_get();
		short next_bsp = (short)((cur_bsp + 1) % scen->structure_bsp_references.count);
		if (g_forge.grabbed_live_index >= 0)
			forge_drop_or_place_held_item();
		if (scenario_switch_structure_bsp(next_bsp))
		{
			real_point3d bsp_pos;
			float bsp_yaw = g_forge.cam_yaw;
			if (find_valid_bsp_spawn_point(&bsp_pos, &bsp_yaw))
			{
				long unit_idx = player_control_get_unit_index(0);
				g_forge.cam_x = bsp_pos.x;
				g_forge.cam_y = bsp_pos.y;
				g_forge.cam_z = bsp_pos.z + 0.65f;
				g_forge.cam_yaw = bsp_yaw;
				s_last_valid_spartan_pos = bsp_pos;
				s_has_valid_spartan_pos = 1;
				if (unit_idx != NONE)
				{
					forge_sync_unit_to_monitor_pose(
						unit_idx,
						g_forge.cam_x, g_forge.cam_y, g_forge.cam_z,
						g_forge.cam_yaw, g_forge.cam_pitch, g_forge.cam_roll,
						1, 1, 0.0f);
				}
			}
			forge_set_status("Switched to Structure BSP %d / %d", (int)(next_bsp + 1), (int)scen->structure_bsp_references.count);
		}
	}

	/* Hot-reload Forge UI XML & PNG backgrounds live in-game with F8 */
	if (cur_f8 && !prev_f8)
	{
		forge_ui_theme_load(1);
		forge_set_status("Reloaded Forge UI Theme");
	}

	{
		int modal_before_hotkeys = g_forge.active_modal;
		int allow_f1 = !(modal_before_hotkeys != _forge_modal_none && g_forge.using_controller &&
						 !(g_forge.bind_vk[FORGE_BIND_TOGGLE_MODE] > 0 &&
						   (GetAsyncKeyState(g_forge.bind_vk[FORGE_BIND_TOGGLE_MODE]) & 0x8000)));
		int allow_f5 = !(modal_before_hotkeys != _forge_modal_none && g_forge.using_controller &&
						 !(g_forge.bind_vk[FORGE_BIND_QUICK_SAVE] > 0 &&
						   (GetAsyncKeyState(g_forge.bind_vk[FORGE_BIND_QUICK_SAVE]) & 0x8000)));

		/* Mode & Modal Hotkeys (Point 2: Seamless location continuity between Spartan and Forge Mode; Point 11: Overhead viewer removed) */
		if (cur_f1 && !prev_f1 && allow_f1)
		{
			int local_p_abs = forge_get_local_player_abs_index();
			long unit_idx = player_control_get_unit_index(0);
			if (g_forge.grabbed_live_index >= 0)
				forge_drop_or_place_held_item();
			g_forge.active_modal = _forge_modal_none;
			if (g_forge.mode == _forge_mode_3d_monitor)
			{
				real_point3d sp_pos;
				sp_pos.x = g_forge.cam_x;
				sp_pos.y = g_forge.cam_y;
				sp_pos.z = g_forge.cam_z - 0.65f;
				g_forge.mode = _forge_mode_play;
				if (local_p_abs >= 0 && local_p_abs < 128)
				{
					g_forge.player_is_monitor[local_p_abs] = 0;
					g_forge.player_has_monitor_pos[local_p_abs] = 0;
				}
				if (unit_idx != NONE)
				{
					struct unit_datum *u = (struct unit_datum *)object_try_and_get_and_verify_type(unit_idx, _object_mask_unit);
					if (u)
					{
						real_vector3d fwd, up;
						forge_compute_safe_spartan_stand_pos(g_forge.cam_x, g_forge.cam_y, g_forge.cam_z, &sp_pos);
						euler_to_forward_up(g_forge.cam_yaw, 0.0f, 0.0f, &fwd, &up);
						SET_FLAG(u->object.flags, _object_no_collisions_bit, FALSE);
						SET_FLAG(u->object.flags, _object_at_rest_bit, FALSE);
						SET_FLAG(u->object.damage_flags, _object_cannot_take_damage_bit, FALSE);
						forge_hide_spartan_linked_attachments_and_children(unit_idx, 0);
						object_set_position(unit_idx, &sp_pos, &fwd, &up);
						u->unit.desired_facing_vector = fwd;
						u->unit.desired_aiming_vector = fwd;
						u->unit.aiming_vector = fwd;
						u->unit.desired_looking_vector = fwd;
						u->unit.looking_vector = fwd;
						u->object.translational_velocity.i = 0.0f;
						u->object.translational_velocity.j = 0.0f;
						u->object.translational_velocity.k = 0.0f;
						u->object.angular_velocity.i = 0.0f;
						u->object.angular_velocity.j = 0.0f;
						u->object.angular_velocity.k = 0.0f;
						object_activate(unit_idx);
						network_distributed_notify_forge_monitor_teleport(
							local_player_get_player_index(0), unit_idx, &sp_pos, FALSE);
					}
				}
				forge_network_send_monitor_mode(0, sp_pos.x, sp_pos.y, sp_pos.z + 0.65f, g_forge.cam_yaw, g_forge.cam_pitch);
				forge_set_status("Spartan Mode   %s Forge", forge_action_prompt(FORGE_BIND_TOGGLE_MODE));
			}
			else
			{
				struct unit_datum *u = (unit_idx != NONE)
					? (struct unit_datum *)object_try_and_get_and_verify_type(unit_idx, _object_mask_unit)
					: NULL;
				if (u && (u->object.parent_object_index != NONE || u->unit.parent_seat_index != NONE))
				{
					forge_set_status("Exit vehicle before entering Monitor Mode");
				}
				else
				{
					if (u)
					{
						if (scenario_leaf_index_from_point(&u->object.position) != NONE)
						{
							s_last_valid_spartan_pos = u->object.position;
							s_has_valid_spartan_pos = 1;
						}
						g_forge.cam_x = u->object.position.x;
						g_forge.cam_y = u->object.position.y;
						g_forge.cam_z = u->object.position.z + 0.65f;
						g_forge.cam_yaw = (float)atan2((double)u->unit.aiming_vector.j, (double)u->unit.aiming_vector.i);
						g_forge.cam_pitch = (float)asin((double)u->unit.aiming_vector.k);
						forge_hide_spartan_linked_attachments_and_children(unit_idx, 1);
						network_distributed_notify_forge_monitor_teleport(
							local_player_get_player_index(0), unit_idx, &u->object.position, TRUE);
					}
					g_forge.mode = _forge_mode_3d_monitor;
					if (local_p_abs >= 0 && local_p_abs < 128)
					{
						g_forge.player_is_monitor[local_p_abs] = 1;
						g_forge.player_has_monitor_pos[local_p_abs] = 1;
						g_forge.player_monitor_pos[local_p_abs][0] = g_forge.cam_x;
						g_forge.player_monitor_pos[local_p_abs][1] = g_forge.cam_y;
						g_forge.player_monitor_pos[local_p_abs][2] = g_forge.cam_z;
						g_forge.player_monitor_yaw[local_p_abs] = g_forge.cam_yaw;
						g_forge.player_monitor_pitch[local_p_abs] = g_forge.cam_pitch;
					}
					forge_network_send_monitor_mode(1, g_forge.cam_x, g_forge.cam_y, g_forge.cam_z, g_forge.cam_yaw, g_forge.cam_pitch);
					forge_set_status("Forge Mode   %s Objects   %s View   %s Save",
						forge_action_prompt(FORGE_BIND_PALETTE),
						forge_action_prompt(FORGE_BIND_THIRD_PERSON),
						forge_action_prompt(FORGE_BIND_QUICK_SAVE));
				}
			}
		}
		/* Point 1: Inventory / Item Options (F2 / X on controller):
		 * On controller, X opens Inventory (Spawn Palette) when not holding an item,
		 * and opens Item Options (Object Properties) when holding an item. */
		if (cur_f2 && !prev_f2)
		{
			if (g_forge.mode == _forge_mode_3d_monitor)
			{
				int shared_gp_x = (g_forge.bind_gp[FORGE_BIND_PALETTE] > 0 &&
					g_forge.bind_gp[FORGE_BIND_PALETTE] == g_forge.bind_gp[FORGE_BIND_OBJECT_OPTIONS] &&
					forge_gp_button_down(g_forge.bind_gp[FORGE_BIND_PALETTE]) &&
					!(g_forge.bind_vk[FORGE_BIND_PALETTE] > 0 &&
					  (GetAsyncKeyState(g_forge.bind_vk[FORGE_BIND_PALETTE]) & 0x8000)));
				if (modal_before_hotkeys == _forge_modal_spawn_palette)
				{
					g_forge.active_modal = _forge_modal_none;
				}
				else if (shared_gp_x && g_forge.grabbed_live_index >= 0)
				{
					/* Handled below by cur_x (Item Options when holding an item) */
				}
				else
				{
					if (g_forge.grabbed_live_index >= 0)
						forge_drop_or_place_held_item();
					g_forge.active_modal = _forge_modal_spawn_palette;
					g_forge.h3_menu_stage = 1; /* Unified single-stage tabbed menu: cycle categories with [<-] / [->] */
					if (g_forge.current_category == _forge_cat_bipeds ||
						g_forge.current_category == _forge_cat_ai_squads)
					{
						g_forge.current_category = _forge_cat_weapons;
					}
				}
			}
		}
		if (cur_f4 && !prev_f4 && g_forge.mode == _forge_mode_3d_monitor)
		{
			g_forge.third_person_view = !g_forge.third_person_view;
			forge_set_status("3rd-Person View: %s", g_forge.third_person_view ? "ON" : "OFF");
		}
		/* F5 in-game is for Quick Save (opens the Save Map prompt if not yet saved in this session) */
		if (cur_f5 && !prev_f5 && allow_f5)
		{
			if (g_forge.has_saved_map_once && g_forge.save_map_name[0])
			{
				forge_ui_save_copy_as(g_forge.save_map_name);
			}
			else
			{
				forge_trigger_save_map_action();
			}
		}
		if (cur_x && !prev_x && g_forge.mode == _forge_mode_3d_monitor && g_forge.grabbed_live_index >= 0 &&
			modal_before_hotkeys != _forge_modal_spawn_palette)
		{
			g_forge.active_modal = (modal_before_hotkeys == _forge_modal_object_properties)
				? _forge_modal_none : _forge_modal_object_properties;
			g_forge.props_cursor = 0;
			g_forge.props_scroll = 0;
		}
		if (((cur_esc && !prev_esc) || (cur_bk && !prev_bk)) &&
			modal_before_hotkeys != _forge_modal_none &&
			modal_before_hotkeys != _forge_modal_save_copy_as)
		{
			g_forge.active_modal = _forge_modal_none;
		}
		else if ((cur_esc && !prev_esc) &&
			modal_before_hotkeys == _forge_modal_none &&
			g_forge.mode == _forge_mode_3d_monitor &&
			g_forge.grabbed_live_index >= 0)
		{
			forge_cancel_or_release_held_item();
		}
	}

	prev_f1 = cur_f1;
	prev_f2 = cur_f2;
	prev_f4 = cur_f4;
	prev_f5 = cur_f5;
	prev_f7 = cur_f7;
	prev_f8 = cur_f8;
	prev_esc = cur_esc;
	prev_x = cur_x;

	if (g_forge.active_modal == _forge_modal_save_copy_as)
	{
		update_save_modal_input();
		prev_bk = 1;
		prev_lmb = 1;
		prev_enter = 1;
		return;
	}

	ctrl_down = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
	cur_undo = (ctrl_down && (GetAsyncKeyState('Z') & 0x8000) != 0) ? 1 : 0;
	cur_redo = (ctrl_down && (GetAsyncKeyState('Y') & 0x8000) != 0) ? 1 : 0;
	cur_weld = (ctrl_down && (GetAsyncKeyState('G') & 0x8000) != 0) ? 1 : 0;

	cur_up = ((GetAsyncKeyState(VK_UP) & 0x8000) != 0) || forge_gp_button_down(FORGE_GP_DPAD_UP);
	cur_down = ((GetAsyncKeyState(VK_DOWN) & 0x8000) != 0) || forge_gp_button_down(FORGE_GP_DPAD_DOWN);
	cur_left = ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0) || forge_gp_button_down(FORGE_GP_DPAD_LEFT);
	cur_right = ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0) || forge_gp_button_down(FORGE_GP_DPAD_RIGHT);
	if (g_forge.active_modal != _forge_modal_none)
	{
		uint32_t now_ms = (uint32_t)system_milliseconds();
		if (now_ms - s_last_stick_nav_ms >= 150)
		{
			if (gp_ly > 0.55f)       { cur_up = 1; prev_up = 0; s_last_stick_nav_ms = now_ms; }
			else if (gp_ly < -0.55f) { cur_down = 1; prev_down = 0; s_last_stick_nav_ms = now_ms; }
			else if (gp_lx < -0.55f) { cur_left = 1; prev_left = 0; s_last_stick_nav_ms = now_ms; }
			else if (gp_lx > 0.55f)  { cur_right = 1; prev_right = 0; s_last_stick_nav_ms = now_ms; }
		}
	}
	cur_enter = ((GetAsyncKeyState(VK_RETURN) & 0x8000) != 0) || forge_gp_button_down(FORGE_GP_A);
	cur_rot = forge_action_down(FORGE_BIND_ROTATE_HOLD);
	cur_g = (!ctrl_down && g_forge.active_modal == _forge_modal_none && !cur_rot) ? forge_action_down(FORGE_BIND_ROT_SNAP) : 0;
	cur_z = !ctrl_down ? forge_action_down(FORGE_BIND_SNAP_FLOOR) : 0;
	cur_f = forge_action_down(FORGE_BIND_SNAP_SURFACE);
	cur_del = ((!cur_rot && forge_action_down(FORGE_BIND_DELETE_OBJECT)) ||
			   (GetAsyncKeyState(VK_BACK) & 0x8000)) != 0;
	cur_copy = (g_forge.active_modal == _forge_modal_none && !cur_rot && g_forge.grabbed_live_index < 0 && !g_forge.koth_group_placing)
		? forge_action_down(FORGE_BIND_COPY_OBJECT) : 0;

	/* Unified Tabbed Spawn Palette Menu: [<-] / [->] switches category, [Up] / [Down] / Scroll selects item */
	if (g_forge.active_modal == _forge_modal_spawn_palette)
	{
		int filtered[512];
		int fcount;
		int const visible_rows = 10;

		g_forge.h3_menu_stage = 1;
		if ((g_forge.current_category == _forge_cat_bipeds ||
			 g_forge.current_category == _forge_cat_ai_squads) &&
			get_filtered_catalog_indices_for_cat(g_forge.current_category, NULL, 1) <= 0)
		{
			g_forge.current_category = _forge_cat_weapons;
		}

		if (cur_left && !prev_left)
		{
			do {
				g_forge.current_category = (enum forge_palette_category)(
					(g_forge.current_category + NUMBER_OF_FORGE_PALETTE_CATEGORIES - 1) % NUMBER_OF_FORGE_PALETTE_CATEGORIES);
			} while ((g_forge.current_category == _forge_cat_bipeds ||
					  g_forge.current_category == _forge_cat_ai_squads) &&
					 get_filtered_catalog_indices_for_cat(g_forge.current_category, NULL, 1) <= 0);
			g_forge.palette_cursor = 0;
			g_forge.palette_scroll = 0;
		}
		if (cur_right && !prev_right)
		{
			do {
				g_forge.current_category = (enum forge_palette_category)(
					(g_forge.current_category + 1) % NUMBER_OF_FORGE_PALETTE_CATEGORIES);
			} while ((g_forge.current_category == _forge_cat_bipeds ||
					  g_forge.current_category == _forge_cat_ai_squads) &&
					 get_filtered_catalog_indices_for_cat(g_forge.current_category, NULL, 1) <= 0);
			g_forge.palette_cursor = 0;
			g_forge.palette_scroll = 0;
		}

		fcount = get_filtered_catalog_indices(filtered, 512);
		if (g_forge.palette_cursor >= fcount)
			g_forge.palette_cursor = (fcount > 0) ? (fcount - 1) : 0;
		if (g_forge.palette_scroll > g_forge.palette_cursor)
			g_forge.palette_scroll = g_forge.palette_cursor;

		{
			static uint32_t s_pal_hold_start_ms = 0;
			static uint32_t s_pal_last_rep_ms = 0;
			static int s_pal_hold_dir = 0;
			uint32_t now_ms = (uint32_t)system_milliseconds();
			int step_up = (cur_up && !prev_up) ? 1 : 0;
			int step_down = (cur_down && !prev_down) ? 1 : 0;
			int cur_dir = cur_up ? -1 : (cur_down ? 1 : 0);
			long wheel_notches = 0;

			if (cur_dir != 0)
			{
				if (cur_dir != s_pal_hold_dir)
				{
					s_pal_hold_dir = cur_dir;
					s_pal_hold_start_ms = now_ms;
					s_pal_last_rep_ms = now_ms;
				}
				else if (now_ms - s_pal_hold_start_ms >= 280u && now_ms - s_pal_last_rep_ms >= 55u)
				{
					if (cur_dir < 0) step_up = 1;
					else step_down = 1;
					s_pal_last_rep_ms = now_ms;
				}
			}
			else
			{
				s_pal_hold_dir = 0;
			}

			platform_scoreboard_scroll(1, &wheel_notches, NULL);
			if (wheel_notches < 0)
				step_up = 1;
			else if (wheel_notches > 0)
				step_down = 1;

			if (step_up && g_forge.palette_cursor > 0)
			{
				g_forge.palette_cursor--;
				if (g_forge.palette_cursor < g_forge.palette_scroll)
					g_forge.palette_scroll = g_forge.palette_cursor;
			}
			if (step_down && g_forge.palette_cursor + 1 < fcount)
			{
				g_forge.palette_cursor++;
				if (g_forge.palette_cursor >= g_forge.palette_scroll + visible_rows)
					g_forge.palette_scroll = g_forge.palette_cursor - (visible_rows - 1);
			}
		}
		{
			static int prev_pal_cache = 0;
			int cur_pal_cache = ((GetAsyncKeyState('C') & 0x8000) != 0) || forge_gp_button_down(FORGE_GP_Y);
			if (cur_pal_cache && !prev_pal_cache && fcount > 0 && g_forge.palette_cursor < fcount)
			{
				struct forge_catalog_entry const *ce = &g_forge.catalog[filtered[g_forge.palette_cursor]];
				if (ce->source_cache_id == 3)
				{
					uint32_t save_grp = ce->group_tag;
					char save_path[256];
					snprintf(save_path, sizeof(save_path), "%s", ce->tag_path);
					if (forge_cache_active_map_tag(save_grp, save_path))
					{
						int k;
						fcount = get_filtered_catalog_indices(filtered, 512);
						for (k = 0; k < fcount; k++)
						{
							struct forge_catalog_entry const *nce = &g_forge.catalog[filtered[k]];
							if (nce->group_tag == save_grp && _stricmp(nce->tag_path, save_path) == 0)
							{
								g_forge.palette_cursor = k;
								if (g_forge.palette_cursor < g_forge.palette_scroll)
									g_forge.palette_scroll = g_forge.palette_cursor;
								else if (g_forge.palette_cursor >= g_forge.palette_scroll + visible_rows)
									g_forge.palette_scroll = g_forge.palette_cursor - (visible_rows - 1);
								break;
							}
						}
						forge_set_status("Saved '%s' to Forge Cache ([C])!", save_path);
					}
					else
					{
						forge_set_status("Failed to cache '%s'", save_path);
					}
					prev_pal_cache = 1;
					goto update_prev_keys;
				}
			}
			prev_pal_cache = cur_pal_cache;
		}
		if (cur_enter && !prev_enter && fcount > 0 && g_forge.palette_cursor < fcount)
		{
			struct forge_catalog_entry const *ce = &g_forge.catalog[filtered[g_forge.palette_cursor]];
			real_vector3d cam_fwd, cam_up;
			real_point3d ray_orig;
			real_vector3d ray_vec;
			struct collision_result coll;
			unsigned long cflags = FLAG(_collision_test_structure_bit) |
								   FLAG(_collision_test_objects_bit) |
								   FLAG(_collision_test_front_facing_surfaces_bit);
			short p_idx = local_player_get_player_index(0);
			long local_unit = (p_idx != NONE && player_get(p_idx)) ? player_get(p_idx)->unit_index : NONE;
			float spawn_dist = 4.5f;
			float sx, sy, sz;
			int32_t spawned_idx = -1;

			if (g_forge.grabbed_live_index >= 0)
				forge_drop_or_place_held_item();

			/* Perform a fresh raycast along the current camera view ray so newly spawned items
			 * stick directly to the placement cursor and camera angle! */
			euler_to_forward_up(g_forge.cam_yaw, g_forge.cam_pitch, g_forge.cam_roll, &cam_fwd, &cam_up);
			ray_orig.x = g_forge.cam_x;
			ray_orig.y = g_forge.cam_y;
			ray_orig.z = g_forge.cam_z;
			ray_vec.i = cam_fwd.i * 20.0f;
			ray_vec.j = cam_fwd.j * 20.0f;
			ray_vec.k = cam_fwd.k * 20.0f;
			memset(&coll, 0, sizeof(coll));
			if (forge_collision_test_vector(cflags, &ray_orig, &ray_vec, local_unit, &coll))
			{
				float dx = coll.point.x - g_forge.cam_x;
				float dy = coll.point.y - g_forge.cam_y;
				float dz = coll.point.z - g_forge.cam_z;
				spawn_dist = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
				if (spawn_dist < 1.5f) spawn_dist = 1.5f;
				if (spawn_dist > 20.0f) spawn_dist = 20.0f;
				g_forge.ray_hit_valid = 1;
				g_forge.ray_hit_x = coll.point.x;
				g_forge.ray_hit_y = coll.point.y;
				g_forge.ray_hit_z = coll.point.z;
				sx = coll.point.x;
				sy = coll.point.y;
				sz = coll.point.z + 0.08f;
			}
			else
			{
				spawn_dist = 4.5f;
				sx = g_forge.cam_x + cam_fwd.i * spawn_dist;
				sy = g_forge.cam_y + cam_fwd.j * spawn_dist;
				sz = g_forge.cam_z + cam_fwd.k * spawn_dist;
			}

			g_forge.koth_group_placing = 0;

			/* Item 4: Spawning [KOTH] Hill Marker drops a 4m diameter circle made of 10 linked points! */
			if (ce->group_tag == 'flag' && strstr(ce->tag_path, "koth") != NULL)
			{
				int cand_id, j, k, used;
				int16_t hill_id = 0;
				float r_init = 2.0f * (1.0f / 3.048f); /* 2.0m radius = 4.0m diameter in Halo world units */

				for (cand_id = 0; cand_id < 16; cand_id++)
				{
					used = 0;
					for (j = 0; j < g_forge.live_instance_count; j++)
					{
						if (g_forge.live_instances[j].active &&
							g_forge.live_instances[j].group_tag == 'flag' &&
							g_forge.live_instances[j].netgame_flag_type == 8 &&
							g_forge.live_instances[j].team_designator == cand_id)
						{
							used = 1;
							break;
						}
					}
					if (!used)
					{
						hill_id = (int16_t)cand_id;
						break;
					}
				}

				g_forge.koth_group_hill_id = hill_id;
				g_forge.koth_group_radius = r_init;
				for (k = 0; k < 10; k++)
				{
					float ang = (float)k * (6.283185307f / 10.0f);
					float nx = sx + r_init * (float)cos((double)ang);
					float ny = sy + r_init * (float)sin((double)ang);
					int32_t nidx = forge_3d_spawn_object_at(
						ce->group_tag,
						ce->tag_path,
						ce->source_cache_id,
						nx, ny, sz,
						ang, 0.0f, 0.0f,
						hill_id,
						1);
					g_forge.koth_group_live_indices[k] = nidx;
					if (k == 0)
						spawned_idx = nidx;
				}
				if (spawned_idx >= 0)
					g_forge.koth_group_placing = 1;
			}
			else
			{
				spawned_idx = forge_3d_spawn_object_at(
					ce->group_tag,
					ce->tag_path,
					ce->source_cache_id,
					sx, sy, sz,
					g_forge.cam_yaw, 0.0f, 0.0f,
					0,
					1);
			}

			/* Hitting Enter confirms the previewed item and attaches it to the placement beam */
			if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
			{
				g_forge.active_modal = _forge_modal_none;
				if (spawned_idx >= 0 && g_forge.mode == _forge_mode_3d_monitor)
				{
					g_forge.grabbed_live_index = spawned_idx;
					g_forge.grabbed_object_index = g_forge.live_instances[spawned_idx].object_index;
					g_forge.grabbed_is_new_spawn = 1;
					g_forge.grab_distance = spawn_dist;
					g_forge.grab_yaw = g_forge.cam_yaw;
					g_forge.grab_pitch = 0.0f;
					g_forge.grab_roll = 0.0f;
					if (g_forge.koth_group_placing)
					{
						forge_set_status("KOTH Hill #%d   [C+Scroll] Radius",
							g_forge.koth_group_hill_id);
					}
					else
					{
						forge_set_status("Holding %s", ce->display_name);
					}
				}
			}
			else if (spawned_idx >= 0)
			{
				/* Shift+Enter quick-places without grabbing: push Undo spawn transaction immediately */
				uint32_t tx_id = forge_undo_begin_tx();
				if (g_forge.koth_group_placing)
				{
					int k;
					for (k = 0; k < 10; k++)
					{
						int32_t nidx = g_forge.koth_group_live_indices[k];
						if (nidx >= 0 && nidx < g_forge.live_instance_count && g_forge.live_instances[nidx].active)
							forge_undo_push_record(tx_id, _undo_op_spawn, nidx, NULL, &g_forge.live_instances[nidx]);
					}
					g_forge.koth_group_placing = 0;
				}
				else
				{
					forge_undo_push_record(tx_id, _undo_op_spawn, spawned_idx, NULL, &g_forge.live_instances[spawned_idx]);
				}
				forge_server_mark_dirty();
			}
			/* Prevent this same Enter/A press from immediately dropping the newly spawned item */
			prev_enter = 1;
			prev_lmb = 1;
			goto update_prev_keys;
		}
		goto update_prev_keys;
	}

	/* Halo 3 / Reach Scrollable Object Properties Modal (Physics, Respawn, AI Order, Dynamic Lights, Weld Group, Gametype, etc.) */
	if (g_forge.active_modal == _forge_modal_object_properties && g_forge.grabbed_live_index >= 0)
	{
		int32_t live_idx = g_forge.grabbed_live_index;
		struct forge_live_instance *inst = &g_forge.live_instances[live_idx];
		uint8_t row_ids[FORGE_MAX_PROP_ROWS];
		char dummy_labels[FORGE_MAX_PROP_ROWS][40];
		char dummy_values[FORGE_MAX_PROP_ROWS][48];
		int row_cnt = forge_build_object_prop_rows(live_idx, row_ids, dummy_labels, dummy_values);
		int const visible_rows = 8;
		int max_props_row = (row_cnt > 0) ? (row_cnt - 1) : 0;
		int dir = (cur_left && !prev_left) ? -1 : ((cur_right && !prev_right) || (cur_enter && !prev_enter) ? 1 : 0);
		int step_up = (cur_up && !prev_up) ? 1 : 0;
		int step_down = (cur_down && !prev_down) ? 1 : 0;
		long wheel_notches = 0;
		int prop_changed = 0;

		platform_scoreboard_scroll(1, &wheel_notches, NULL);
		if (wheel_notches < 0)
			step_up = 1;
		else if (wheel_notches > 0)
			step_down = 1;

		if (g_forge.props_cursor > max_props_row)
			g_forge.props_cursor = max_props_row;
		if (g_forge.props_cursor < 0)
			g_forge.props_cursor = 0;

		if (step_up && g_forge.props_cursor > 0)
			g_forge.props_cursor--;
		if (step_down && g_forge.props_cursor < max_props_row)
			g_forge.props_cursor++;

		if (g_forge.props_scroll > g_forge.props_cursor)
			g_forge.props_scroll = g_forge.props_cursor;
		if (g_forge.props_cursor >= g_forge.props_scroll + visible_rows)
			g_forge.props_scroll = g_forge.props_cursor - (visible_rows - 1);
		if (g_forge.props_scroll < 0)
			g_forge.props_scroll = 0;

		if (dir != 0 && row_cnt > 0)
		{
			uint8_t sel_id = row_ids[g_forge.props_cursor];
			switch (sel_id)
			{
				case FORGE_PROP_TEAM_OR_CHANNEL:
					if (inst->is_ai_unit)
					{
						int cur_t = (inst->team_designator >= 0 && inst->team_designator <= 3) ? (int)inst->team_designator : 0;
						inst->team_designator = (int16_t)((cur_t + 4 + dir) % 4);
						forge_ai_update_unit_team(inst->actor_index, inst->object_index, inst->team_designator);
					}
					else if (inst->group_tag == 'spwn')
					{
						int cur_t = (inst->team_designator >= 0 && inst->team_designator <= 2) ? (int)inst->team_designator : 2;
						inst->team_designator = (int16_t)((cur_t + 3 + dir) % 3);
					}
					else if (inst->group_tag == 'flag' &&
						(inst->netgame_flag_type == _netgame_flag_ctf_flag || inst->netgame_flag_type == _netgame_flag_ctf_vehicle))
					{
						int cur_t = (inst->team_designator == 1) ? 1 : 0;
						inst->team_designator = (int16_t)((cur_t + 2 + dir) % 2);
					}
					else if (inst->group_tag == 'flag' &&
						(inst->netgame_flag_type == _netgame_flag_race_track || inst->netgame_flag_type == _netgame_flag_race_vehicle))
					{
						inst->team_designator = (int16_t)((inst->team_designator + 32 + dir) % 32);
					}
					else
					{
						inst->team_designator = (int16_t)((inst->team_designator + 16 + dir) % 16);
					}
					prop_changed = 1;
					break;

				case FORGE_PROP_PHYSICS_MODE:
					inst->physics_mode = (uint8_t)(((int)inst->physics_mode + NUMBER_OF_FORGE_PHYSICS_MODES + dir) % NUMBER_OF_FORGE_PHYSICS_MODES);
					prop_changed = 1;
					break;

				case FORGE_PROP_RESPAWN_TIMER:
					if (inst->is_ai_unit)
					{
						static int16_t const s_ai_respawn_opts[6] = { 0, 5, 10, 15, 30, 60 };
						int cur_idx = 0, k;
						for (k = 0; k < 6; k++)
						{
							if (s_ai_respawn_opts[k] == inst->respawn_timer_sec)
							{
								cur_idx = k;
								break;
							}
						}
						cur_idx = (cur_idx + 6 + dir) % 6;
						inst->respawn_timer_sec = s_ai_respawn_opts[cur_idx];
					}
					else
					{
						static int16_t const s_obj_respawn_opts[7] = { 0, 15, 30, 60, 90, 120, 180 };
						int cur_idx = 0, k;
						for (k = 0; k < 7; k++)
						{
							if (s_obj_respawn_opts[k] == inst->respawn_timer_sec)
							{
								cur_idx = k;
								break;
							}
						}
						cur_idx = (cur_idx + 7 + dir) % 7;
						inst->respawn_timer_sec = s_obj_respawn_opts[cur_idx];
					}
					inst->death_tick = -1;
					prop_changed = 1;
					break;

				case FORGE_PROP_AI_ORDER:
					inst->ai_order = (uint8_t)(((int)inst->ai_order + NUMBER_OF_FORGE_AI_ORDERS + dir) % NUMBER_OF_FORGE_AI_ORDERS);
					if (game_connection() != _game_connection_network_client && inst->ai_order != 0)
					{
						forge_ai_apply_order(
							inst->actor_index, inst->object_index, inst->ai_order,
							inst->x, inst->y, inst->z, game_time_get());
					}
					prop_changed = 1;
					break;

				case FORGE_PROP_LIGHT_COLOR:
					inst->light_color_preset = (uint8_t)(((int)inst->light_color_preset + NUMBER_OF_FORGE_LIGHT_COLORS + dir) % NUMBER_OF_FORGE_LIGHT_COLORS);
					forge_apply_instance_light_settings(live_idx);
					prop_changed = 1;
					break;

				case FORGE_PROP_LIGHT_INTENSITY:
					inst->light_intensity_preset = (uint8_t)(((int)inst->light_intensity_preset + NUMBER_OF_FORGE_LIGHT_INTENSITIES + dir) % NUMBER_OF_FORGE_LIGHT_INTENSITIES);
					forge_apply_instance_light_settings(live_idx);
					prop_changed = 1;
					break;

				case FORGE_PROP_LIGHT_RADIUS:
					inst->light_radius_preset = (uint8_t)(((int)inst->light_radius_preset + NUMBER_OF_FORGE_LIGHT_RADII + dir) % NUMBER_OF_FORGE_LIGHT_RADII);
					forge_apply_instance_light_settings(live_idx);
					prop_changed = 1;
					break;

				case FORGE_PROP_WELD_GROUP:
					inst->group_id = (uint8_t)(((int)inst->group_id + (FORGE_MAX_WELD_GROUPS + 1) + dir) % (FORGE_MAX_WELD_GROUPS + 1));
					if (inst->group_id >= 1 && inst->group_id <= FORGE_MAX_WELD_GROUPS)
						g_forge.active_weld_group = inst->group_id;
					forge_weld_capture_followers(live_idx);
					prop_changed = 1;
					break;

				case FORGE_PROP_GAMETYPE_MODE:
					inst->gametype_flags = cycle_gametype_flags(inst->gametype_flags, dir);
					prop_changed = 1;
					break;

				case FORGE_PROP_ROT_SNAP:
					g_forge.snap_mode = (enum forge_snap_angle_mode)((g_forge.snap_mode + NUMBER_OF_FORGE_SNAP_MODES + dir) % NUMBER_OF_FORGE_SNAP_MODES);
					break;

				case FORGE_PROP_DROP_TO_FLOOR:
					cur_z = 1;
					break;

				case FORGE_PROP_DELETE_OBJECT:
					if (cur_enter && !prev_enter)
					{
						s_props_orig_live_idx = -1;
						forge_delete_held_item_or_group();
						g_forge.active_modal = _forge_modal_none;
						goto update_prev_keys;
					}
					break;

				case FORGE_PROP_SAVE_TO_CACHE:
					if (cur_enter && !prev_enter)
					{
						if (forge_cache_active_map_tag(inst->group_tag, inst->tag_path))
						{
							inst->source_cache_id = 2;
							if (g_forge.props_cursor > 0)
								g_forge.props_cursor--;
							forge_set_status("Saved '%s' to Forge Cache ([C])!", inst->tag_path);
						}
						else
						{
							forge_set_status("Failed to cache '%s'", inst->tag_path);
						}
						prev_enter = 1;
						goto update_prev_keys;
					}
					break;

				default:
					break;
			}

			if (prop_changed)
			{
				forge_sync_instance_extended_props_to_edit_state(live_idx);
				forge_server_mark_dirty();
				if (forge_is_multiplayer_session())
					forge_network_send_object_props(live_idx);
			}
		}
	}

	if (g_forge.mode == _forge_mode_3d_monitor)
	{
		float boost = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) || forge_gp_button_down(FORGE_GP_LTHUMB)) ? 2.5f : 1.0f;
		float spd = g_forge.cam_speed * g_forge.controller_fly_speed * boost * dt;
		float dyaw = 0.0f, dpitch = 0.0f;
		int rot_holding = 0;
		real_vector3d fwd, up, right_vec;

		/* Undo / Redo & Weld Group Keyboard Shortcuts in Monitor Mode */
		if (g_forge.active_modal == _forge_modal_none)
		{
			if (cur_undo && !prev_undo)
			{
				if (g_forge.grabbed_live_index >= 0)
				{
					forge_cancel_or_release_held_item();
				}
				else
				{
					forge_undo_step();
					forge_server_mark_dirty();
				}
			}
			else if (cur_redo && !prev_redo && g_forge.grabbed_live_index < 0)
			{
				forge_redo_step();
				forge_server_mark_dirty();
			}
			else if (cur_weld && !prev_weld)
			{
				if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
				{
					g_forge.active_weld_group = (uint8_t)((g_forge.active_weld_group % FORGE_MAX_WELD_GROUPS) + 1);
					forge_set_status("Active Weld Group: #%d (%d member%s)",
						(int)g_forge.active_weld_group,
						forge_weld_count_group_members(g_forge.active_weld_group),
						forge_weld_count_group_members(g_forge.active_weld_group) == 1 ? "" : "s");
				}
				else
				{
					int32_t w_idx = (g_forge.grabbed_live_index >= 0)
						? g_forge.grabbed_live_index
						: forge_pick_aimed_live_instance(NULL);
					if (w_idx >= 0 && w_idx < g_forge.live_instance_count &&
						g_forge.live_instances[w_idx].active && !g_forge.koth_group_placing)
					{
						struct forge_live_instance *w_inst = &g_forge.live_instances[w_idx];
						struct forge_live_instance before_w = *w_inst;
						uint32_t tx_id = forge_undo_begin_tx();
						if (w_inst->group_id == g_forge.active_weld_group)
						{
							w_inst->group_id = 0;
							forge_set_status("Unwelded '%s' from Group %d",
								w_inst->tag_path, (int)g_forge.active_weld_group);
						}
						else
						{
							w_inst->group_id = g_forge.active_weld_group;
							forge_set_status("Welded '%s' to Group %d (%d items)",
								w_inst->tag_path,
								(int)g_forge.active_weld_group,
								forge_weld_count_group_members(g_forge.active_weld_group));
						}
						forge_sync_instance_extended_props_to_edit_state(w_idx);
						forge_server_mark_dirty();
						if (forge_is_multiplayer_session())
							forge_network_send_object_props(w_idx);
						if (g_forge.grabbed_live_index == w_idx)
							forge_weld_capture_followers(w_idx);
						forge_undo_push_record(tx_id, _undo_op_props, w_idx, &before_w, w_inst);
					}
				}
			}
		}

		if (g_forge.active_modal == _forge_modal_none && cur_rot)
		{
			if (g_forge.grabbed_live_index >= 0)
			{
				s_hover_rot_live_idx = -1;
				rot_holding = 1;
			}
			else
			{
				if (!prev_rot || s_hover_rot_live_idx < 0 ||
					s_hover_rot_live_idx >= g_forge.live_instance_count ||
					!g_forge.live_instances[s_hover_rot_live_idx].active)
				{
					int32_t cand = forge_pick_aimed_live_instance(NULL);
					s_hover_rot_live_idx = cand;
					if (cand >= 0)
					{
						struct forge_live_instance *inst = &g_forge.live_instances[cand];
						forge_sync_live_instance_from_engine_object(inst);
						s_hover_rot_orig_piv = *inst;
						g_forge.grab_yaw = inst->yaw;
						g_forge.grab_pitch = inst->pitch;
						g_forge.grab_roll = inst->roll;
						forge_weld_capture_followers(cand);
					}
				}
				if (s_hover_rot_live_idx >= 0)
					rot_holding = 1;
			}
		}
		else
		{
			if (s_hover_rot_live_idx >= 0 &&
				s_hover_rot_live_idx < g_forge.live_instance_count &&
				g_forge.live_instances[s_hover_rot_live_idx].active)
			{
				struct forge_live_instance *inst = &g_forge.live_instances[s_hover_rot_live_idx];
				uint32_t tx_id = forge_undo_begin_tx();
				forge_3d_move_instance_ext(
					s_hover_rot_live_idx,
					inst->x, inst->y, inst->z,
					inst->yaw, inst->pitch, inst->roll,
					0, 1);
				forge_undo_push_record(tx_id, _undo_op_transform, s_hover_rot_live_idx, &s_hover_rot_orig_piv, inst);
				if (s_weld_follower_count > 0)
				{
					int w;
					forge_weld_update_followers(s_hover_rot_live_idx, 0, 1, 1);
					for (w = 0; w < s_weld_follower_count; w++)
					{
						int32_t fli = s_weld_followers[w].live_idx;
						if (fli >= 0 && fli < g_forge.live_instance_count && g_forge.live_instances[fli].active)
						{
							struct forge_live_instance const *fi = &g_forge.live_instances[fli];
							struct forge_live_instance before_f = *fi;
							before_f.x = s_weld_followers[w].orig_x;
							before_f.y = s_weld_followers[w].orig_y;
							before_f.z = s_weld_followers[w].orig_z;
							before_f.yaw = s_weld_followers[w].orig_yaw;
							before_f.pitch = s_weld_followers[w].orig_pitch;
							before_f.roll = s_weld_followers[w].orig_roll;
							forge_undo_push_record(tx_id, _undo_op_transform, fli, &before_f, fi);
						}
					}
					forge_weld_clear_followers();
				}
				forge_server_mark_dirty();
			}
			s_hover_rot_live_idx = -1;
		}

		/* Native Mouse Look + Controller Right Stick Look */
		halo_linux_mouse_look(0, &dyaw, &dpitch);
		if ((float)fabs((double)gp_rx) > 0.001f || (float)fabs((double)gp_ry) > 0.001f)
		{
			float pad_sens = 2.6f * g_forge.controller_look_sens;
			dyaw -= gp_rx * pad_sens * dt;
			dpitch += gp_ry * pad_sens * dt;
		}

		if (rot_holding)
		{
			/* Holding Left Alt (or Right Trigger on controller) locks camera look & movement and rotates the item:
			 * Mouse / Right Stick -> Pitch & Yaw, Left Stick -> Roll & Pitch, [Q/E] -> Roll, [C] / R-Thumb -> Reset */
			if (cur_rot && !prev_rot)
			{
				if (g_forge.using_controller)
					forge_set_status("Rotating Item: Left/Right Stick = Rotate  |  {RS} Reset");
				else
					forge_set_status("Rotating Item: Mouse Up/Down = Pitch, Left/Right = Yaw  |  [Q/E] Roll  |  [C] Reset");
			}
			g_forge.grab_yaw += dyaw * 1.4f;
			g_forge.grab_pitch += dpitch * 1.4f;
			if ((float)fabs((double)gp_lx) > 0.001f || (float)fabs((double)gp_ly) > 0.001f)
			{
				float pad_sens = 2.6f * g_forge.controller_look_sens;
				g_forge.grab_roll += gp_lx * pad_sens * 1.4f * dt;
				g_forge.grab_pitch += gp_ly * pad_sens * 1.4f * dt;
			}
			if (GetAsyncKeyState(VK_LEFT) & 0x8000)  g_forge.grab_yaw += 2.0f * dt;
			if (GetAsyncKeyState(VK_RIGHT) & 0x8000) g_forge.grab_yaw -= 2.0f * dt;
			if (GetAsyncKeyState(VK_UP) & 0x8000)    g_forge.grab_pitch += 2.0f * dt;
			if (GetAsyncKeyState(VK_DOWN) & 0x8000)  g_forge.grab_pitch -= 2.0f * dt;
			if (GetAsyncKeyState('Q') & 0x8000)
				g_forge.grab_roll -= 2.0f * dt;
			if (GetAsyncKeyState('E') & 0x8000)
				g_forge.grab_roll += 2.0f * dt;
			if ((GetAsyncKeyState('C') & 0x8000) || forge_gp_button_down(FORGE_GP_RTHUMB))
			{
				g_forge.grab_pitch = 0.0f;
				g_forge.grab_roll = 0.0f;
			}
			while (g_forge.grab_yaw > 3.14159265f)   g_forge.grab_yaw -= 6.283185307f;
			while (g_forge.grab_yaw < -3.14159265f)  g_forge.grab_yaw += 6.283185307f;
			while (g_forge.grab_pitch > 3.14159265f) g_forge.grab_pitch -= 6.283185307f;
			while (g_forge.grab_pitch < -3.14159265f) g_forge.grab_pitch += 6.283185307f;
			while (g_forge.grab_roll > 3.14159265f)  g_forge.grab_roll -= 6.283185307f;
			while (g_forge.grab_roll < -3.14159265f) g_forge.grab_roll += 6.283185307f;

			if (g_forge.grabbed_live_index < 0 && s_hover_rot_live_idx >= 0 &&
				s_hover_rot_live_idx < g_forge.live_instance_count &&
				g_forge.live_instances[s_hover_rot_live_idx].active)
			{
				struct forge_live_instance *inst = &g_forge.live_instances[s_hover_rot_live_idx];
				float syaw = snap_angle_rad(g_forge.grab_yaw, g_forge.snap_mode);
				float spitch = snap_angle_rad(g_forge.grab_pitch, g_forge.snap_mode);
				float sroll = snap_angle_rad(g_forge.grab_roll, g_forge.snap_mode);
				forge_3d_move_instance_ext(
					s_hover_rot_live_idx,
					inst->x, inst->y, inst->z,
					syaw, spitch, sroll,
					1, 1);
				if (s_weld_follower_count > 0)
					forge_weld_update_followers(s_hover_rot_live_idx, 1, 1, 0);
			}
		}
		else
		{
			g_forge.cam_yaw += dyaw;
			g_forge.cam_pitch += dpitch;
			if (g_forge.grabbed_live_index >= 0)
				g_forge.grab_yaw += dyaw;

			/* Optional keyboard fine rotation when modal is closed */
			if (g_forge.active_modal == _forge_modal_none)
			{
				if (GetAsyncKeyState(VK_LEFT) & 0x8000)
				{
					g_forge.cam_yaw += 1.6f * dt;
					if (g_forge.grabbed_live_index >= 0)
						g_forge.grab_yaw += 1.6f * dt;
				}
				if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
				{
					g_forge.cam_yaw -= 1.6f * dt;
					if (g_forge.grabbed_live_index >= 0)
						g_forge.grab_yaw -= 1.6f * dt;
				}
				if (GetAsyncKeyState(VK_UP) & 0x8000) g_forge.cam_pitch += 1.2f * dt;
				if (GetAsyncKeyState(VK_DOWN) & 0x8000) g_forge.cam_pitch -= 1.2f * dt;
			}
		}
		if (g_forge.cam_pitch > 1.48f) g_forge.cam_pitch = 1.48f;
		if (g_forge.cam_pitch < -1.48f) g_forge.cam_pitch = -1.48f;

		euler_to_forward_up(g_forge.cam_yaw, g_forge.cam_pitch, g_forge.cam_roll, &fwd, &up);
		right_vec.i = (float)sin((double)g_forge.cam_yaw);
		right_vec.j = -(float)cos((double)g_forge.cam_yaw);
		right_vec.k = 0.0f;

		/* 6DOF Monitor Flight (WASD / Left Stick + Space/E up + Ctrl/Q down) with non-sticking BSP collision & wall sliding */
		{
			float move_dx = 0.0f, move_dy = 0.0f, move_dz = 0.0f;
			if (GetAsyncKeyState('W') & 0x8000)
			{
				move_dx += fwd.i * spd;
				move_dy += fwd.j * spd;
				move_dz += fwd.k * spd;
			}
			if (GetAsyncKeyState('S') & 0x8000)
			{
				move_dx -= fwd.i * spd;
				move_dy -= fwd.j * spd;
				move_dz -= fwd.k * spd;
			}
			if (GetAsyncKeyState('D') & 0x8000)
			{
				move_dx += right_vec.i * spd;
				move_dy += right_vec.j * spd;
			}
			if (GetAsyncKeyState('A') & 0x8000)
			{
				move_dx -= right_vec.i * spd;
				move_dy -= right_vec.j * spd;
			}
			if (g_forge.active_modal == _forge_modal_none && !rot_holding)
			{
				if ((float)fabs((double)gp_ly) > 0.001f)
				{
					move_dx += fwd.i * (gp_ly * spd);
					move_dy += fwd.j * (gp_ly * spd);
					move_dz += fwd.k * (gp_ly * spd);
				}
				if ((float)fabs((double)gp_lx) > 0.001f)
				{
					move_dx += right_vec.i * (gp_lx * spd);
					move_dy += right_vec.j * (gp_lx * spd);
				}
			}
			/* Move Up: Space / E on keyboard, or Right Bumper (FORGE_GP_BLACK) on controller */
			if ((GetAsyncKeyState(VK_SPACE) & 0x8000) || (!rot_holding && (GetAsyncKeyState('E') & 0x8000)) ||
				(g_forge.active_modal == _forge_modal_none && forge_gp_button_down(FORGE_GP_BLACK)))
			{
				move_dz += spd;
			}
			/* Move Down: Ctrl / Q on keyboard, or Left Bumper (FORGE_GP_WHITE) on controller */
			if (((GetAsyncKeyState(VK_CONTROL) & 0x8000) && !cur_undo && !cur_redo && !cur_weld) ||
				(!rot_holding && (GetAsyncKeyState('Q') & 0x8000)) ||
				(g_forge.active_modal == _forge_modal_none && forge_gp_button_down(FORGE_GP_WHITE)))
			{
				move_dz -= spd;
			}

			if (move_dx != 0.0f || move_dy != 0.0f || move_dz != 0.0f)
			{
				float move_len = (float)sqrt((double)(move_dx * move_dx + move_dy * move_dy + move_dz * move_dz));
				if (move_len > 0.0001f)
				{
					real_point3d cur_pt, pt1, pt2, pt3;
					real_plane3d plane1, plane2;
					float pad = 0.28f;
					struct structure_bsp const *bsp = global_structure_bsp_get();
					int hit1;

					cur_pt.x = g_forge.cam_x;
					cur_pt.y = g_forge.cam_y;
					cur_pt.z = g_forge.cam_z;

					/* Keep camera at least 0.24u clear of any surrounding walls/floors/ceilings before moving */
					forge_monitor_enforce_wall_clearance(&cur_pt, 0.24f);

					memset(&plane1, 0, sizeof(plane1));
					hit1 = forge_monitor_try_step(&cur_pt, move_dx, move_dy, move_dz, pad, &pt1, &plane1);
					if (hit1)
					{
						float step1_dx = pt1.x - cur_pt.x;
						float step1_dy = pt1.y - cur_pt.y;
						float step1_dz = pt1.z - cur_pt.z;
						float moved1 = (float)sqrt((double)(step1_dx * step1_dx + step1_dy * step1_dy + step1_dz * step1_dz));
						float rem_frac = (move_len > moved1) ? ((move_len - moved1) / move_len) : 0.0f;
						float vn1 = move_dx * plane1.n.i + move_dy * plane1.n.j + move_dz * plane1.n.k;

						cur_pt = pt1;
						if (vn1 < 0.0f && rem_frac > 0.01f)
						{
							float slide_dx = (move_dx - vn1 * plane1.n.i) * rem_frac;
							float slide_dy = (move_dy - vn1 * plane1.n.j) * rem_frac;
							float slide_dz = (move_dz - vn1 * plane1.n.k) * rem_frac;
							int hit2;

							memset(&plane2, 0, sizeof(plane2));
							hit2 = forge_monitor_try_step(&cur_pt, slide_dx, slide_dy, slide_dz, pad, &pt2, &plane2);
							cur_pt = pt2;
							if (hit2)
							{
								float vn2 = slide_dx * plane2.n.i + slide_dy * plane2.n.j + slide_dz * plane2.n.k;
								if (vn2 < 0.0f)
								{
									/* Crease slide along intersection of plane1 and plane2 (verified with full collision_test_vector!) */
									float c_dx = slide_dx - vn2 * plane2.n.i;
									float c_dy = slide_dy - vn2 * plane2.n.j;
									float c_dz = slide_dz - vn2 * plane2.n.k;
									float c_dot1 = c_dx * plane1.n.i + c_dy * plane1.n.j + c_dz * plane1.n.k;
									c_dx -= c_dot1 * plane1.n.i;
									c_dy -= c_dot1 * plane1.n.j;
									c_dz -= c_dot1 * plane1.n.k;
									if (c_dx * c_dx + c_dy * c_dy + c_dz * c_dz > 0.00001f)
									{
										forge_monitor_try_step(&cur_pt, c_dx, c_dy, c_dz, pad, &pt3, NULL);
										cur_pt = pt3;
									}
								}
							}
						}

						/* If primary + slide made zero progress in a tight corner, try verified axis-separated steps */
						if ((cur_pt.x - g_forge.cam_x) * (cur_pt.x - g_forge.cam_x) +
							(cur_pt.y - g_forge.cam_y) * (cur_pt.y - g_forge.cam_y) +
							(cur_pt.z - g_forge.cam_z) * (cur_pt.z - g_forge.cam_z) < 0.000004f)
						{
							forge_monitor_try_step(&cur_pt, move_dx, move_dy, 0.0f, pad, &pt2, NULL);
							cur_pt = pt2;
							forge_monitor_try_step(&cur_pt, 0.0f, 0.0f, move_dz, pad, &pt3, NULL);
							cur_pt = pt3;
						}
					}
					else
					{
						cur_pt = pt1;
					}

					/* Post-step 6-axis wall clearance so the camera never ends a frame closer than 0.24u to any wall */
					forge_monitor_enforce_wall_clearance(&cur_pt, 0.24f);

					/* Clamp inside structure BSP world_bounds */
					if (bsp)
					{
						float margin_xy = 0.28f;
						float margin_z_low = 0.28f;
						float margin_z_high = 0.50f;
						if (bsp->world_bounds.x1 - bsp->world_bounds.x0 > 2.0f)
						{
							if (cur_pt.x < bsp->world_bounds.x0 + margin_xy)
								cur_pt.x = bsp->world_bounds.x0 + margin_xy;
							if (cur_pt.x > bsp->world_bounds.x1 - margin_xy)
								cur_pt.x = bsp->world_bounds.x1 - margin_xy;
						}
						if (bsp->world_bounds.y1 - bsp->world_bounds.y0 > 2.0f)
						{
							if (cur_pt.y < bsp->world_bounds.y0 + margin_xy)
								cur_pt.y = bsp->world_bounds.y0 + margin_xy;
							if (cur_pt.y > bsp->world_bounds.y1 - margin_xy)
								cur_pt.y = bsp->world_bounds.y1 - margin_xy;
						}
						if (bsp->world_bounds.z1 - bsp->world_bounds.z0 > 2.0f)
						{
							if (cur_pt.z < bsp->world_bounds.z0 + margin_z_low)
								cur_pt.z = bsp->world_bounds.z0 + margin_z_low;
							if (cur_pt.z > bsp->world_bounds.z1 - margin_z_high)
								cur_pt.z = bsp->world_bounds.z1 - margin_z_high;
						}
					}

					if (scenario_leaf_index_from_point(&cur_pt) != NONE)
					{
						g_forge.cam_x = cur_pt.x;
						g_forge.cam_y = cur_pt.y;
						g_forge.cam_z = cur_pt.z;
						s_last_valid_spartan_pos = cur_pt;
						s_has_valid_spartan_pos = 1;
					}
				}
			}
		}

		/* Points 1, 2 & 8: Sync local player's unit position & facing to Monitor position AND keep Invulnerable! */
		{
			long unit_idx = player_control_get_unit_index(0);
			if (unit_idx != NONE)
			{
				struct unit_datum *u_mon;
				forge_sync_unit_to_monitor_pose(
					unit_idx,
					g_forge.cam_x, g_forge.cam_y, g_forge.cam_z,
					g_forge.cam_yaw, g_forge.cam_pitch, g_forge.cam_roll,
					1, 1, 0.0f);
				u_mon = (struct unit_datum *)object_try_and_get_and_verify_type(unit_idx, _object_mask_unit);
				if (u_mon)
				{
					network_distributed_notify_forge_monitor_teleport(
						local_player_get_player_index(0), unit_idx, &u_mon->object.position, TRUE);
				}
			}
		}

		/* Point 4: Perform Physics Beam Raycast + Ray-Cone / Proximity World Object & Marker Scanner (strictly within 20m!) */
		{
			real_point3d ray_origin;
			real_vector3d ray_vec;
			struct collision_result coll;
			unsigned long cflags = FLAG(_collision_test_structure_bit) |
				FLAG(_collision_test_objects_bit) |
				_collision_test_objects_all_types_flags |
				FLAG(_collision_test_front_facing_surfaces_bit);
			long local_unit = player_control_get_unit_index(0);
			long ignore_obj = g_forge.grabbed_object_index;
			float best_obj_score = 4.5f;
			float best_marker_score = 1.6f;
			int best_marker_idx = -1;
			int direct_obj_hit = 0;

			if (!forge_valid_object_get(ignore_obj))
				ignore_obj = local_unit;
			if (!forge_valid_object_get(ignore_obj))
				ignore_obj = NONE;

			ray_origin.x = g_forge.cam_x;
			ray_origin.y = g_forge.cam_y;
			ray_origin.z = g_forge.cam_z;
			/* Point 4: Limit interaction raycast strictly to 20m */
			ray_vec.i = fwd.i * 20.0f;
			ray_vec.j = fwd.j * 20.0f;
			ray_vec.k = fwd.k * 20.0f;

			g_forge.hovered_live_index = -1;
			memset(&coll, 0, sizeof(coll));
			if (forge_collision_test_vector(cflags, &ray_origin, &ray_vec, ignore_obj, &coll))
			{
				g_forge.ray_hit_valid = 1;
				g_forge.ray_hit_x = coll.point.x;
				g_forge.ray_hit_y = coll.point.y;
				g_forge.ray_hit_z = coll.point.z;
				g_forge.hovered_object_index = (coll.type == _collision_result_object) ? coll.object_index : NONE;
				if (g_forge.hovered_object_index == local_unit)
					g_forge.hovered_object_index = NONE;
				if (g_forge.hovered_object_index != NONE)
				{
					direct_obj_hit = 1;
					best_obj_score = 0.0f;
				}
			}
			else
			{
				g_forge.ray_hit_valid = 1;
				g_forge.ray_hit_x = g_forge.cam_x + fwd.i * 8.0f;
				g_forge.ray_hit_y = g_forge.cam_y + fwd.j * 8.0f;
				g_forge.ray_hit_z = g_forge.cam_z + fwd.k * 8.0f;
				g_forge.hovered_object_index = NONE;
			}

			/* Secondary ray-cone & ray-endpoint scan over all map objects within 20m so weapons, equipment & scenery are selectable! */
			if (g_forge.hovered_object_index == NONE)
			{
				struct object_iterator iter;
				struct object_datum *obj;
				unsigned long edit_mask = _object_mask_scenery | _object_mask_vehicle |
					_object_mask_weapon | _object_mask_equipment | _object_mask_biped | _object_mask_device;
				long best_obj = NONE;

				object_iterator_new(&iter, edit_mask, 0);
				while ((obj = object_iterator_next(&iter)) != NULL)
				{
					long oidx = iter.index;
					struct unit_datum *u;
					float vx, vy, vz, cam_d2, dist_along, px, py, pz, perp_d2, hit_dx, hit_dy, hit_dz, hit_d2;

					if (oidx == local_unit || oidx == g_forge.grabbed_object_index)
						continue;
					if (obj->object.parent_object_index != NONE)
						continue;
					u = (struct unit_datum *)object_try_and_get_and_verify_type(oidx, _object_mask_biped);
					if (u && u->unit.player_index != NONE)
						continue;

					vx = obj->object.position.x - g_forge.cam_x;
					vy = obj->object.position.y - g_forge.cam_y;
					vz = obj->object.position.z - g_forge.cam_z;
					cam_d2 = vx * vx + vy * vy + vz * vz;
					/* Point 4: Only objects within 20m (20^2 = 400.0f) can be interacted with */
					if (cam_d2 > 400.0f)
						continue;

					dist_along = vx * fwd.i + vy * fwd.j + vz * fwd.k;
					if (dist_along > 0.6f && dist_along <= 20.0f)
					{
						px = vx - fwd.i * dist_along;
						py = vy - fwd.j * dist_along;
						pz = vz - fwd.k * dist_along;
						perp_d2 = px * px + py * py + pz * pz;
						if (perp_d2 < 1.8f && perp_d2 < best_obj_score)
						{
							best_obj_score = perp_d2;
							best_obj = oidx;
						}
					}

					hit_dx = obj->object.position.x - g_forge.ray_hit_x;
					hit_dy = obj->object.position.y - g_forge.ray_hit_y;
					hit_dz = obj->object.position.z - g_forge.ray_hit_z;
					hit_d2 = hit_dx * hit_dx + hit_dy * hit_dy + hit_dz * hit_dz;
					if (hit_d2 < 3.2f && hit_d2 < best_obj_score)
					{
						best_obj_score = hit_d2;
						best_obj = oidx;
					}
				}
				g_forge.hovered_object_index = best_obj;
			}

			/* Scan virtual markers (Player Spawns, CTF Flags, Oddballs, KOTH Hills, Race Nodes, Teleporters) within 20m!
			 * Compare marker distance against indirect object scan so nearby rocks/weapons never block selecting spawns/flags! */
			{
				int i;
				for (i = 0; i < g_forge.live_instance_count; i++)
				{
					struct forge_live_instance const *inst = &g_forge.live_instances[i];
					float vx, vy, vz, cam_d2, dist_along, px, py, pz, perp_d2, hit_dx, hit_dy, hit_dz, hit_d2;
					if (!inst->active || inst->object_index != NONE || i == g_forge.grabbed_live_index)
						continue;
					vx = inst->x - g_forge.cam_x;
					vy = inst->y - g_forge.cam_y;
					vz = (inst->z + 0.15f) - g_forge.cam_z;
					cam_d2 = vx * vx + vy * vy + vz * vz;
					if (cam_d2 > 400.0f)
						continue;

					dist_along = vx * fwd.i + vy * fwd.j + vz * fwd.k;
					if (dist_along > 0.5f && dist_along <= 20.0f)
					{
						px = vx - fwd.i * dist_along;
						py = vy - fwd.j * dist_along;
						pz = vz - fwd.k * dist_along;
						perp_d2 = px * px + py * py + pz * pz;
						if (perp_d2 < 1.2f && perp_d2 < best_marker_score)
						{
							best_marker_score = perp_d2;
							best_marker_idx = i;
						}
					}

					hit_dx = inst->x - g_forge.ray_hit_x;
					hit_dy = inst->y - g_forge.ray_hit_y;
					hit_dz = inst->z - g_forge.ray_hit_z;
					hit_d2 = hit_dx * hit_dx + hit_dy * hit_dy + hit_dz * hit_dz;
					if (hit_d2 < 1.2f && hit_d2 < best_marker_score)
					{
						best_marker_score = hit_d2;
						best_marker_idx = i;
					}
				}

				if (best_marker_idx >= 0 && (!direct_obj_hit || best_marker_score < 0.35f) &&
					best_marker_score <= best_obj_score + 0.25f)
				{
					g_forge.hovered_live_index = best_marker_idx;
					g_forge.hovered_object_index = NONE;
				}
			}

			/* Update hovered_object_label for HUD display (including Player Spawns, Teleporters & KOTH/Oddball/CTF/Race markers) */
			g_forge.hovered_object_label[0] = 0;
			if (g_forge.hovered_live_index >= 0 && g_forge.hovered_live_index < g_forge.live_instance_count)
			{
				struct forge_live_instance const *inst = &g_forge.live_instances[g_forge.hovered_live_index];
				if (inst->group_tag == 'spwn')
				{
					char const *tname = (inst->team_designator == 1) ? "Blue Team Spawn" :
						(inst->team_designator == 2 ? "FFA Spawn (Yellow)" : "Red Team Spawn");
					snprintf(g_forge.hovered_object_label, sizeof(g_forge.hovered_object_label), "%s", tname);
				}
				else if (inst->group_tag == 'tele')
				{
					int is_sender = (inst->netgame_flag_type == _netgame_flag_teleporter_source ||
						strstr(inst->tag_path, "sender") != NULL ||
						strstr(inst->tag_path, "origin") != NULL);
					snprintf(g_forge.hovered_object_label, sizeof(g_forge.hovered_object_label),
						"Teleporter %s (Ref #%d)", is_sender ? "In [^]" : "Out [v]", (int)inst->team_designator);
				}
				else if (inst->group_tag == 'flag')
				{
					if (inst->netgame_flag_type == _netgame_flag_ctf_flag)
						snprintf(g_forge.hovered_object_label, sizeof(g_forge.hovered_object_label),
							"%s CTF Flag Point", inst->team_designator == 1 ? "Blue" : "Red");
					else if (inst->netgame_flag_type == _netgame_flag_oddball_ball_spawn)
						snprintf(g_forge.hovered_object_label, sizeof(g_forge.hovered_object_label),
							"Oddball Spawn #%d", (int)inst->team_designator);
					else if (inst->netgame_flag_type == _netgame_flag_hill)
						snprintf(g_forge.hovered_object_label, sizeof(g_forge.hovered_object_label),
							"KOTH Hill Marker #%d", (int)inst->team_designator);
					else if (inst->netgame_flag_type == _netgame_flag_race_track)
						snprintf(g_forge.hovered_object_label, sizeof(g_forge.hovered_object_label),
							"Race Node #%d", (int)inst->team_designator);
					else
					{
						char const *slash = strrchr(inst->tag_path, '\\');
						snprintf(g_forge.hovered_object_label, sizeof(g_forge.hovered_object_label),
							"%s (#%d)", slash ? (slash + 1) : inst->tag_path, (int)inst->team_designator);
					}
				}
				if (inst->group_id >= 1 && inst->group_id <= FORGE_MAX_WELD_GROUPS && g_forge.hovered_object_label[0])
				{
					size_t cur_len = strlen(g_forge.hovered_object_label);
					if (cur_len + 8 < sizeof(g_forge.hovered_object_label))
					{
						snprintf(g_forge.hovered_object_label + cur_len,
							sizeof(g_forge.hovered_object_label) - cur_len,
							" [G%d]", (int)inst->group_id);
					}
				}
			}
			else if (g_forge.hovered_object_index != NONE)
			{
				struct object_datum *hobj = forge_valid_object_get(g_forge.hovered_object_index);
				if (hobj && hobj->definition_index != NONE)
				{
					float vx = hobj->object.position.x - g_forge.cam_x;
					float vy = hobj->object.position.y - g_forge.cam_y;
					float vz = hobj->object.position.z - g_forge.cam_z;
					if (vx * vx + vy * vy + vz * vz > 400.0f)
					{
						g_forge.hovered_object_index = NONE;
					}
					else
					{
						char const *tname = tag_get_name(hobj->definition_index);
						char const *slash = tname ? strrchr(tname, '\\') : NULL;
						int hi;
						snprintf(g_forge.hovered_object_label, sizeof(g_forge.hovered_object_label),
							"%s", slash ? (slash + 1) : (tname ? tname : "map_object"));
						for (hi = 0; hi < g_forge.live_instance_count; hi++)
						{
							if (g_forge.live_instances[hi].active &&
								g_forge.live_instances[hi].object_index == g_forge.hovered_object_index &&
								g_forge.live_instances[hi].group_id >= 1 &&
								g_forge.live_instances[hi].group_id <= FORGE_MAX_WELD_GROUPS)
							{
								size_t cur_len = strlen(g_forge.hovered_object_label);
								if (cur_len + 8 < sizeof(g_forge.hovered_object_label))
								{
									snprintf(g_forge.hovered_object_label + cur_len,
										sizeof(g_forge.hovered_object_label) - cur_len,
										" [G%d]", (int)g_forge.live_instances[hi].group_id);
								}
								break;
							}
						}
					}
				}
				else
				{
					g_forge.hovered_object_index = NONE;
				}
			}
		}

		/* Cycle Rotation Snap Mode (G: Off / 15 deg / 45 deg / 90 deg) */
		if (cur_g && !prev_g)
		{
			static char const *const snap_names[4] = { "Off (Free)", "15 Degrees", "45 Degrees", "90 Degrees" };
			g_forge.snap_mode = (enum forge_snap_angle_mode)((g_forge.snap_mode + 1) % NUMBER_OF_FORGE_SNAP_MODES);
			forge_set_status("Rotation Snapping: %s", snap_names[g_forge.snap_mode]);
		}

		/* Grab / Drop Item on Physics Beam (Left Click on mouse, A on controller, or Enter when holding) */
		if ((g_forge.active_modal == _forge_modal_none && cur_lmb && !prev_lmb) ||
			(g_forge.active_modal == _forge_modal_none && forge_gp_button_down(FORGE_GP_A) && !prev_enter) ||
			(g_forge.active_modal == _forge_modal_none && g_forge.grabbed_live_index >= 0 && cur_enter && !prev_enter))
		{
			if (g_forge.grabbed_live_index >= 0)
			{
				forge_drop_or_place_held_item();
			}
			else
			{
				int32_t best = forge_pick_aimed_live_instance(&fwd);

				if (best >= 0)
				{
					struct forge_live_instance *inst = &g_forge.live_instances[best];
					struct object_datum *obj = forge_valid_object_get(inst->object_index);
					float dx, dy, dz;
					forge_sync_live_instance_from_engine_object(inst);
					dx = inst->x - g_forge.cam_x;
					dy = inst->y - g_forge.cam_y;
					dz = inst->z - g_forge.cam_z;
					g_forge.koth_group_placing = 0;
					g_forge.grabbed_live_index = best;
					g_forge.grabbed_object_index = inst->object_index;
					g_forge.grabbed_is_new_spawn = 0;
					g_forge.grab_orig_x = inst->x;
					g_forge.grab_orig_y = inst->y;
					g_forge.grab_orig_z = inst->z;
					g_forge.grab_orig_yaw = inst->yaw;
					g_forge.grab_orig_pitch = inst->pitch;
					g_forge.grab_orig_roll = inst->roll;
					g_forge.grab_distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
					if (g_forge.grab_distance < 1.0f) g_forge.grab_distance = 1.0f;
					if (g_forge.grab_distance > 20.0f) g_forge.grab_distance = 20.0f;
					g_forge.grab_yaw = inst->yaw;
					g_forge.grab_pitch = inst->pitch;
					g_forge.grab_roll = inst->roll;
					if (obj && obj->object.type == _object_type_vehicle)
					{
						forge_eject_all_passengers_from_vehicle(inst->object_index);
					}
					forge_weld_capture_followers(best);
					if (forge_is_multiplayer_session() &&
						((obj && obj->object.type == _object_type_vehicle) || inst->is_default_scenario_object))
					{
						forge_network_send_transform_ext(
							best,
							inst->object_index,
							inst->x,
							inst->y,
							inst->z,
							inst->yaw,
							inst->pitch,
							inst->roll,
							1,
							1);
					}
					if (s_weld_follower_count > 0)
					{
						forge_set_status("Holding '%s' [Group %d: %d items]",
							inst->tag_path, (int)inst->group_id, s_weld_follower_count + 1);
					}
					else
					{
						forge_set_status("Holding '%s'", inst->tag_path);
					}
				}
			}
		}

		/* Hold to Drop Item: B on Controller */
		{
			static float s_b_hold_sec = 0.0f;
			static int s_b_hold_armed = 0;
			if (g_forge.active_modal != _forge_modal_none)
			{
				s_b_hold_sec = 0.0f;
				s_b_hold_armed = 0;
			}
			else if (!forge_gp_button_down(FORGE_GP_B))
			{
				s_b_hold_sec = 0.0f;
				s_b_hold_armed = 1;
			}
			else if (s_b_hold_armed && g_forge.grabbed_live_index >= 0)
			{
				s_b_hold_sec += dt;
				if (s_b_hold_sec >= 0.22f)
				{
					forge_drop_or_place_held_item();
					s_b_hold_sec = 0.0f;
					s_b_hold_armed = 0;
				}
			}
		}

		/* Copy / Duplicate Hovered Object or Weld Group when NOT holding anything ([C] or Controller D-Pad Right) */
		if (g_forge.active_modal == _forge_modal_none && g_forge.grabbed_live_index < 0 && cur_copy && !prev_copy)
		{
			/* Not holding anything: duplicate the object (or entire Weld Group) under the reticle and grab the new copy! */
			int32_t best = forge_pick_aimed_live_instance(&fwd);

			if (best >= 0 && best < g_forge.live_instance_count)
			{
				struct forge_live_instance src_inst = g_forge.live_instances[best];
				int32_t src_followers[FORGE_MAX_WELD_FOLLOWERS];
				int src_fcount = 0;
				uint8_t new_gid = 0;
				float dx, dy, dz, dist;
				int32_t spawned_idx;

				forge_sync_live_instance_from_engine_object(&src_inst);

				if (src_inst.group_id >= 1 && src_inst.group_id <= FORGE_MAX_WELD_GROUPS)
				{
					int mi;
					uint8_t cand_gid = forge_weld_find_unused_group_id(0);
					if (cand_gid >= 1 && cand_gid <= FORGE_MAX_WELD_GROUPS &&
						forge_weld_count_group_members(cand_gid) == 0)
					{
						new_gid = cand_gid;
					}
					for (mi = 0; mi < g_forge.live_instance_count && src_fcount < FORGE_MAX_WELD_FOLLOWERS; mi++)
					{
						if (mi != best &&
							g_forge.live_instances[mi].active &&
							g_forge.live_instances[mi].group_id == src_inst.group_id)
						{
							src_followers[src_fcount++] = mi;
						}
					}
				}

				dx = src_inst.x - g_forge.cam_x;
				dy = src_inst.y - g_forge.cam_y;
				dz = src_inst.z - g_forge.cam_z;
				dist = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
				if (dist < 1.5f) dist = 1.5f;
				if (dist > 20.0f) dist = 20.0f;

				spawned_idx = forge_3d_spawn_object_at(
					src_inst.group_tag,
					src_inst.tag_path,
					src_inst.source_cache_id,
					src_inst.x,
					src_inst.y,
					src_inst.z,
					src_inst.yaw,
					src_inst.pitch,
					src_inst.roll,
					src_inst.team_designator,
					1);
				if (spawned_idx >= 0 && spawned_idx < g_forge.live_instance_count)
				{
					struct forge_live_instance *new_inst = &g_forge.live_instances[spawned_idx];
					forge_copy_extended_props_between_instances(new_inst, &src_inst);
					new_inst->group_id = (src_fcount > 0) ? new_gid : 0;
					forge_apply_instance_light_settings(spawned_idx);
					if (new_inst->is_ai_unit && game_connection() != _game_connection_network_client && new_inst->ai_order != 0)
					{
						forge_ai_apply_order(
							new_inst->actor_index, new_inst->object_index, new_inst->ai_order,
							new_inst->x, new_inst->y, new_inst->z, game_time_get());
					}
					forge_sync_instance_extended_props_to_edit_state(spawned_idx);
					if (forge_is_multiplayer_session())
						forge_network_send_object_props(spawned_idx);

					if (src_fcount > 0 && new_gid >= 1)
					{
						int fi;
						for (fi = 0; fi < src_fcount; fi++)
						{
							struct forge_live_instance fsrc = g_forge.live_instances[src_followers[fi]];
							int32_t fspawn;
							forge_sync_live_instance_from_engine_object(&fsrc);
							fspawn = forge_3d_spawn_object_at(
								fsrc.group_tag,
								fsrc.tag_path,
								fsrc.source_cache_id,
								fsrc.x, fsrc.y, fsrc.z,
								fsrc.yaw, fsrc.pitch, fsrc.roll,
								fsrc.team_designator,
								1);
							if (fspawn >= 0 && fspawn < g_forge.live_instance_count)
							{
								struct forge_live_instance *fn = &g_forge.live_instances[fspawn];
								forge_copy_extended_props_between_instances(fn, &fsrc);
								fn->group_id = new_gid;
								forge_apply_instance_light_settings(fspawn);
								if (fn->is_ai_unit && game_connection() != _game_connection_network_client && fn->ai_order != 0)
								{
									forge_ai_apply_order(
										fn->actor_index, fn->object_index, fn->ai_order,
										fn->x, fn->y, fn->z, game_time_get());
								}
								forge_sync_instance_extended_props_to_edit_state(fspawn);
								if (forge_is_multiplayer_session())
									forge_network_send_object_props(fspawn);
							}
						}
						g_forge.active_weld_group = new_gid;
					}
					forge_server_mark_dirty();

					g_forge.koth_group_placing = 0;
					g_forge.grabbed_live_index = spawned_idx;
					g_forge.grabbed_object_index = new_inst->object_index;
					g_forge.grabbed_is_new_spawn = 1;
					g_forge.grab_distance = dist;
					g_forge.grab_yaw = new_inst->yaw;
					g_forge.grab_pitch = new_inst->pitch;
					g_forge.grab_roll = new_inst->roll;
					forge_weld_capture_followers(spawned_idx);
					if (s_weld_follower_count > 0)
					{
						forge_set_status("Copied Group -> Group %d (%d items)",
							(int)new_gid, s_weld_follower_count + 1);
					}
					else
					{
						forge_set_status("Copied '%s'", new_inst->tag_path);
					}
				}
			}
		}

		/* Update Grabbed Object Transform */
		if (g_forge.grabbed_live_index >= 0 && g_forge.grabbed_live_index < g_forge.live_instance_count)
		{
			float tx, ty, tz, syaw, spitch, sroll;
			long wheel_notches = 0;
			int radius_mod_held = ((GetAsyncKeyState('C') & 0x8000) || (GetAsyncKeyState(VK_SHIFT) & 0x8000)) != 0;

			/* Scroll wheel moves held item away/towards camera, OR scales KOTH Hill radius (only when no modal is open) */
			if (g_forge.active_modal == _forge_modal_none)
				platform_scoreboard_scroll(1, &wheel_notches, NULL);
			if (g_forge.koth_group_placing && radius_mod_held && wheel_notches != 0)
			{
				g_forge.koth_group_radius += (float)wheel_notches * (0.25f / 3.048f);
				if (g_forge.koth_group_radius < (0.4f / 3.048f)) g_forge.koth_group_radius = (0.4f / 3.048f);
				if (g_forge.koth_group_radius > (15.0f / 3.048f)) g_forge.koth_group_radius = (15.0f / 3.048f);
				forge_set_status("KOTH Hill #%d Diameter: %.1fm (Radius: %.2fm)",
					g_forge.koth_group_hill_id,
					g_forge.koth_group_radius * 2.0f * 3.048f,
					g_forge.koth_group_radius * 3.048f);
			}
			else if (wheel_notches != 0)
			{
				g_forge.grab_distance += (float)wheel_notches * 1.0f;
			}

			if (g_forge.koth_group_placing)
			{
				if ((GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) || (GetAsyncKeyState(VK_OEM_6) & 0x8000))
				{
					g_forge.koth_group_radius += (1.5f / 3.048f) * dt;
					if (g_forge.koth_group_radius > (15.0f / 3.048f)) g_forge.koth_group_radius = (15.0f / 3.048f);
				}
				if ((GetAsyncKeyState(VK_OEM_MINUS) & 0x8000) || (GetAsyncKeyState(VK_OEM_4) & 0x8000))
				{
					g_forge.koth_group_radius -= (1.5f / 3.048f) * dt;
					if (g_forge.koth_group_radius < (0.4f / 3.048f)) g_forge.koth_group_radius = (0.4f / 3.048f);
				}
			}

			if (GetAsyncKeyState(VK_PRIOR) & 0x8000) g_forge.grab_distance += 8.0f * dt;
			if (GetAsyncKeyState(VK_NEXT) & 0x8000) g_forge.grab_distance -= 8.0f * dt;
			if (g_forge.grab_distance < 1.0f) g_forge.grab_distance = 1.0f;
			if (g_forge.grab_distance > 30.0f) g_forge.grab_distance = 30.0f;

			/* Keep the placement cursor and held object locked to the camera crosshair ray
			 * (and clamped against any closer BSP/surface hit along the beam unless Physics Mode is PHASED) */
			{
				int is_phased = (g_forge.live_instances[g_forge.grabbed_live_index].physics_mode == _forge_physics_phased);
				real_point3d beam_orig;
				real_vector3d beam_vec;
				struct collision_result bcoll;
				long ignore_grab = forge_valid_object_get(g_forge.grabbed_object_index) ? g_forge.grabbed_object_index : NONE;
				beam_orig.x = g_forge.cam_x;
				beam_orig.y = g_forge.cam_y;
				beam_orig.z = g_forge.cam_z;
				beam_vec.i = fwd.i * g_forge.grab_distance;
				beam_vec.j = fwd.j * g_forge.grab_distance;
				beam_vec.k = fwd.k * g_forge.grab_distance;
				memset(&bcoll, 0, sizeof(bcoll));
				if (!is_phased && forge_collision_test_vector(
					FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
					&beam_orig, &beam_vec, ignore_grab, &bcoll))
				{
					tx = bcoll.point.x + bcoll.plane.n.i * 0.05f;
					ty = bcoll.point.y + bcoll.plane.n.j * 0.05f;
					tz = bcoll.point.z + bcoll.plane.n.k * 0.05f;
				}
				else
				{
					tx = g_forge.cam_x + fwd.i * g_forge.grab_distance;
					ty = g_forge.cam_y + fwd.j * g_forge.grab_distance;
					tz = g_forge.cam_z + fwd.k * g_forge.grab_distance;
				}
				g_forge.ray_hit_valid = 1;
				g_forge.ray_hit_x = tx;
				g_forge.ray_hit_y = ty;
				g_forge.ray_hit_z = tz;
			}

			/* Hold Z to continuously snap held object to floor, then Click (or Enter) to place on ground! */
			if (cur_z)
			{
				real_point3d down_orig;
				real_vector3d down_vec;
				struct collision_result gcoll;
				long ignore_grab = forge_valid_object_get(g_forge.grabbed_object_index) ? g_forge.grabbed_object_index : NONE;
				down_orig.x = tx;
				down_orig.y = ty;
				down_orig.z = tz + 2.0f;
				down_vec.i = 0.0f;
				down_vec.j = 0.0f;
				down_vec.k = -200.0f;
				memset(&gcoll, 0, sizeof(gcoll));
				if (forge_collision_test_vector(
					FLAG(_collision_test_structure_bit) | FLAG(_collision_test_objects_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
					&down_orig, &down_vec, ignore_grab, &gcoll))
				{
					tz = gcoll.point.z + 0.08f;
					if (!prev_z)
						forge_set_status("Snapping to Floor (Z=%.2f)", tz);
				}
			}

			/* Hold/Press F to Snap to Surface WITHOUT forcing or flipping item orientation */
			if (cur_f)
			{
				real_point3d surf_orig;
				real_vector3d surf_vec;
				struct collision_result scoll;
				long ignore_grab = forge_valid_object_get(g_forge.grabbed_object_index) ? g_forge.grabbed_object_index : NONE;
				surf_orig.x = g_forge.cam_x;
				surf_orig.y = g_forge.cam_y;
				surf_orig.z = g_forge.cam_z;
				surf_vec.i = fwd.i * 30.0f;
				surf_vec.j = fwd.j * 30.0f;
				surf_vec.k = fwd.k * 30.0f;
				memset(&scoll, 0, sizeof(scoll));
				if (forge_collision_test_vector(
					FLAG(_collision_test_structure_bit) | FLAG(_collision_test_objects_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
					&surf_orig, &surf_vec, ignore_grab, &scoll))
				{
					real_vector3d surf_up = scoll.plane.n;
					float sdx, sdy, sdz, sdist;
					tx = scoll.point.x + surf_up.i * 0.05f;
					ty = scoll.point.y + surf_up.j * 0.05f;
					tz = scoll.point.z + surf_up.k * 0.05f;
					g_forge.ray_hit_x = tx;
					g_forge.ray_hit_y = ty;
					g_forge.ray_hit_z = tz;
					sdx = tx - g_forge.cam_x;
					sdy = ty - g_forge.cam_y;
					sdz = tz - g_forge.cam_z;
					sdist = (float)sqrt((double)(sdx * sdx + sdy * sdy + sdz * sdz));
					if (sdist >= 1.0f && sdist <= 30.0f)
						g_forge.grab_distance = sdist;
					if (!prev_f)
						forge_set_status("Snapped to Surface (%.1f, %.1f, %.1f)", tx, ty, tz);
				}
			}

			syaw = snap_angle_rad(g_forge.grab_yaw, g_forge.snap_mode);
			spitch = snap_angle_rad(g_forge.grab_pitch, g_forge.snap_mode);
			sroll = snap_angle_rad(g_forge.grab_roll, g_forge.snap_mode);

			if (g_forge.koth_group_placing)
			{
				int k;
				for (k = 0; k < 10; k++)
				{
					int32_t nidx = g_forge.koth_group_live_indices[k];
					float ang = syaw + (float)k * (6.283185307f / 10.0f);
					float nx = tx + g_forge.koth_group_radius * (float)cos((double)ang);
					float ny = ty + g_forge.koth_group_radius * (float)sin((double)ang);
					float nz = tz;
					if (cur_z)
					{
						real_point3d n_orig;
						real_vector3d n_vec;
						struct collision_result ncoll;
						n_orig.x = nx;
						n_orig.y = ny;
						n_orig.z = tz + 2.0f;
						n_vec.i = 0.0f;
						n_vec.j = 0.0f;
						n_vec.k = -200.0f;
						memset(&ncoll, 0, sizeof(ncoll));
						if (collision_test_vector(
							FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
							&n_orig, &n_vec, NONE, &ncoll))
						{
							nz = ncoll.point.z + 0.04f;
						}
					}
					if (nidx >= 0 && nidx < g_forge.live_instance_count)
						forge_3d_move_instance_ext(nidx, nx, ny, nz, ang, 0.0f, 0.0f, 1, 1);
				}
			}
			else
			{
				forge_3d_move_instance_ext(g_forge.grabbed_live_index, tx, ty, tz, syaw, spitch, sroll, 1, 1);
				if (s_weld_follower_count > 0)
					forge_weld_update_followers(g_forge.grabbed_live_index, 1, 1, 0);
			}
		}
		else if (g_forge.active_modal == _forge_modal_none)
		{
			platform_scoreboard_scroll(0, NULL, NULL);
		}

		/* Delete Item (Delete / Backspace on keyboard, or Y on controller): deletes held item or aimed item (or Weld Group) */
		if (cur_del && !prev_del && g_forge.active_modal == _forge_modal_none)
		{
			if (g_forge.grabbed_live_index >= 0)
			{
				forge_delete_held_item_or_group();
			}
			else
			{
				int32_t aimed = forge_pick_aimed_live_instance(&fwd);
				if (aimed >= 0 && aimed < g_forge.live_instance_count && g_forge.live_instances[aimed].active)
				{
					forge_delete_instance_or_weld_group_with_undo(aimed, 1);
				}
			}
		}
	}
	else if (g_forge.active_modal == _forge_modal_none)
	{
		platform_scoreboard_scroll(0, NULL, NULL);
	}

update_prev_keys:
	prev_up = cur_up;
	prev_down = cur_down;
	prev_left = cur_left;
	prev_right = cur_right;
	prev_enter = cur_enter;
	prev_g = cur_g;
	prev_z = cur_z;
	prev_f = cur_f;
	prev_del = cur_del;
	prev_rot = cur_rot;
	prev_bk = cur_bk;
	prev_lmb = cur_lmb;
	prev_copy = cur_copy;
	prev_undo = cur_undo;
	prev_redo = cur_redo;
	prev_weld = cur_weld;
}

static void forge_compute_live_monitor_camera_pose(
	real_point3d *out_pos,
	real_vector3d *out_fwd,
	real_vector3d *out_up)
{
	euler_to_forward_up(g_forge.cam_yaw, g_forge.cam_pitch, g_forge.cam_roll, out_fwd, out_up);
	if (g_forge.third_person_view)
	{
		/* Pull camera back 2.4 world units behind Monitor, clamped against BSP/skybox walls */
		real_point3d mon_pt, cam_pt;
		real_vector3d pullback_vec;
		struct collision_result cam_coll;
		unsigned long cflags = FLAG(_collision_test_structure_bit) |
							   FLAG(_collision_test_front_facing_surfaces_bit) |
							   FLAG(_collision_test_back_facing_surfaces_bit);
		struct structure_bsp const *bsp = global_structure_bsp_get();

		mon_pt.x = g_forge.cam_x;
		mon_pt.y = g_forge.cam_y;
		mon_pt.z = g_forge.cam_z;

		pullback_vec.i = -out_fwd->i * 2.4f;
		pullback_vec.j = -out_fwd->j * 2.4f;
		pullback_vec.k = -out_fwd->k * 2.4f + 0.25f;

		memset(&cam_coll, 0, sizeof(cam_coll));
		if (collision_test_pill(FLAG(_collision_test_structure_bit), &mon_pt, &pullback_vec, 0.22f, NONE, &cam_coll) ||
			collision_test_vector(cflags, &mon_pt, &pullback_vec, NONE, &cam_coll))
		{
			float frac = cam_coll.t - 0.12f;
			if (frac < 0.0f)
				frac = 0.0f;
			cam_pt.x = mon_pt.x + pullback_vec.i * frac;
			cam_pt.y = mon_pt.y + pullback_vec.j * frac;
			cam_pt.z = mon_pt.z + pullback_vec.k * frac;
		}
		else
		{
			cam_pt.x = mon_pt.x + pullback_vec.i;
			cam_pt.y = mon_pt.y + pullback_vec.j;
			cam_pt.z = mon_pt.z + pullback_vec.k;
		}

		if (bsp)
		{
			if (bsp->world_bounds.x1 - bsp->world_bounds.x0 > 2.0f)
			{
				if (cam_pt.x < bsp->world_bounds.x0 + 0.35f) cam_pt.x = bsp->world_bounds.x0 + 0.35f;
				if (cam_pt.x > bsp->world_bounds.x1 - 0.35f) cam_pt.x = bsp->world_bounds.x1 - 0.35f;
			}
			if (bsp->world_bounds.y1 - bsp->world_bounds.y0 > 2.0f)
			{
				if (cam_pt.y < bsp->world_bounds.y0 + 0.35f) cam_pt.y = bsp->world_bounds.y0 + 0.35f;
				if (cam_pt.y > bsp->world_bounds.y1 - 0.35f) cam_pt.y = bsp->world_bounds.y1 - 0.35f;
			}
			if (bsp->world_bounds.z1 - bsp->world_bounds.z0 > 2.0f)
			{
				if (cam_pt.z < bsp->world_bounds.z0 + 0.35f) cam_pt.z = bsp->world_bounds.z0 + 0.35f;
				if (cam_pt.z > bsp->world_bounds.z1 - 0.45f) cam_pt.z = bsp->world_bounds.z1 - 0.45f;
			}
		}

		if (scenario_leaf_index_from_point(&cam_pt) == NONE)
			cam_pt = mon_pt;

		*out_pos = cam_pt;
	}
	else
	{
		out_pos->x = g_forge.cam_x;
		out_pos->y = g_forge.cam_y;
		out_pos->z = g_forge.cam_z;
	}
}

void forge_3d_apply_monitor_camera(short local_player_index)
{
	/* IMPORTANT: observer_set_camera() stores the pointer in observer->pending_command
	 * and observer_update() reads/writes through that pointer AFTER director_update() returns!
	 * Therefore s_cmd MUST be static (never a stack local). */
	static struct observer_command s_cmd;

	if (!g_forge_session_enabled || local_player_index != 0)
		return;
	if (forge_is_main_menu_active())
		return;
	if (g_forge.mode == _forge_mode_play && g_forge.active_modal == _forge_modal_none)
		return;

	director_inhibit_input(0);
	director_inhibit_facing(0);

	if (g_forge.mode == _forge_mode_3d_monitor)
	{
		memset(&s_cmd, 0, sizeof(s_cmd));
		s_cmd.flags = FLAG(_observer_command_valid_bit) | FLAG(_observer_command_ignore_obstructions_bit);
		forge_compute_live_monitor_camera_pose(&s_cmd.focus_position, &s_cmd.forward, &s_cmd.up);
		s_cmd.focus_distance = 0.0f;
		s_cmd.field_of_view = 1.3962634f; /* 80 degrees */
		observer_set_camera(0, &s_cmd);
	}
}

int forge_3d_fill_live_observer_result(void *out_observer_result)
{
	struct observer_result *obs = (struct observer_result *)out_observer_result;
	struct location loc;

	if (!obs || !g_forge_session_enabled || !g_forge.initialized || forge_is_main_menu_active())
		return 0;
	if (g_forge.mode != _forge_mode_3d_monitor)
		return 0;

	forge_compute_live_monitor_camera_pose(&obs->position, &obs->forward, &obs->up);
	obs->velocity.i = 0.0f;
	obs->velocity.j = 0.0f;
	obs->velocity.k = 0.0f;
	scenario_location_from_point(&loc, &obs->position);
	if (loc.leaf_index != NONE)
	{
		obs->location = loc;
	}
	else
	{
		real_point3d mon_pt = { g_forge.cam_x, g_forge.cam_y, g_forge.cam_z };
		scenario_location_from_point(&loc, &mon_pt);
		if (loc.leaf_index != NONE)
			obs->location = loc;
	}
	return 1;
}

static void draw_teleporter_and_objective_markers(
	short window_x0,
	short window_y0,
	short window_x1,
	short window_y1,
	long font_index)
{
	int i;

	/* Player Spawns, Teleporter Origin/Destination, Race Markers, Oddball Markers,
	 * and CTF Flags now render as true 3D translucent world models in
	 * render_world_objective_and_spawn_markers() (forge_runtime_tags.c).
	 * In 2D HUD space we only draw floating text labels while in Forge Mode.
	 * In Spartan Play mode, hide Forge-only Spawn Point and Objective markers and labels completely! */
	if (g_forge.mode != _forge_mode_3d_monitor)
		return;

	/* Only show floating marker names when the pickup reticle is over the item (or holding it) */
	for (i = 0; i < g_forge.live_instance_count; i++)
	{
		struct forge_live_instance const *inst = &g_forge.live_instances[i];
		union real_point3d wpt;
		union real_point2d spt;
		int is_hi;
		short sx, sy;

		if (!inst->active)
			continue;
		if (inst->group_tag != 'spwn' && inst->group_tag != 'tele' && inst->group_tag != 'flag')
			continue;

		is_hi = (i == g_forge.grabbed_live_index || i == g_forge.hovered_live_index);
		if (!is_hi)
			continue;

		if (inst->group_tag == 'spwn')
		{
			uint32_t ring_col;
			char const *team_lbl;

			if (inst->team_designator == 1)
			{
				ring_col = 0xFF2896FFu;
				team_lbl = "BLUE SPAWN";
			}
			else if (inst->team_designator == 0)
			{
				ring_col = 0xFFFF6868u;
				team_lbl = "RED SPAWN";
			}
			else
			{
				ring_col = 0xFFFFFFFFu;
				team_lbl = "FFA SPAWN";
			}

			wpt.x = inst->x;
			wpt.y = inst->y;
			wpt.z = inst->z + 0.14f;
			if (render_camera_world_to_screen(&render.camera, &render.frustum, &wpt, &spt))
			{
				sx = (short)(window_x0 + (short)(spt.x + 0.5f));
				sy = (short)(window_y0 + (short)(spt.y + 0.5f));
				if (sx >= window_x0 + 20 && sx <= window_x1 - 20 && sy >= window_y0 + 10 && sy <= window_y1 - 10)
				{
					set_forge_hud_ui_scale(FORGE_IN_MATCH_HUD_SCALE, (float)sx, (float)sy);
					draw_ce_menu_str_box(0, font_index, (short)(sx - 80), (short)(sy + 6), (short)(sx + 80), (short)(sy + 32), 2, team_lbl, ring_col);
				}
			}
			continue;
		}

		wpt.x = inst->x;
		wpt.y = inst->y;
		wpt.z = inst->z + 0.28f;
		if (!render_camera_world_to_screen(&render.camera, &render.frustum, &wpt, &spt))
			continue;

		sx = (short)(window_x0 + (short)(spt.x + 0.5f));
		sy = (short)(window_y0 + (short)(spt.y + 0.5f));
		if (sx < window_x0 + 10 || sx > window_x1 - 10 || sy < window_y0 + 10 || sy > window_y1 - 10)
			continue;

		set_forge_hud_ui_scale(FORGE_IN_MATCH_HUD_SCALE, (float)sx, (float)sy);
		if (inst->group_tag == 'tele')
		{
			int is_sender = (inst->netgame_flag_type == _netgame_flag_teleporter_source ||
				strstr(inst->tag_path, "sender") != NULL ||
				strstr(inst->tag_path, "origin") != NULL);
			char lbl[64];

			snprintf(lbl, sizeof(lbl), "%s #%d", is_sender ? "TELE ORIGIN" : "TELE DEST", (int)inst->team_designator);
			draw_ce_menu_str_box(0, font_index, (short)(sx - 90), (short)(sy + 8), (short)(sx + 90), (short)(sy + 34), 2, lbl, 0xFFFFFFFFu);
		}
		else if (inst->group_tag == 'flag')
		{
			char lbl[64];
			char const *kind = "OBJECTIVE";
			uint32_t border_col = 0xFFFFFFFFu;

			if (inst->netgame_flag_type == _netgame_flag_hill || strstr(inst->tag_path, "koth"))
			{
				kind = "KOTH HILL";
				border_col = 0xFFFFFFFFu;
			}
			else if (inst->netgame_flag_type == _netgame_flag_oddball_ball_spawn || strstr(inst->tag_path, "oddball"))
			{
				kind = "ODDBALL";
				border_col = 0xFFFFFFFFu;
			}
			else if (inst->netgame_flag_type == _netgame_flag_ctf_flag ||
					 inst->netgame_flag_type == _netgame_flag_ctf_vehicle ||
					 strstr(inst->tag_path, "ctf"))
			{
				kind = (inst->team_designator == 1) ? "BLUE FLAG" : "RED FLAG";
				border_col = (inst->team_designator == 1) ? 0xFF2896FFu : 0xFFFF6868u;
			}
			else if (inst->netgame_flag_type == _netgame_flag_race_track ||
					 inst->netgame_flag_type == _netgame_flag_race_vehicle ||
					 strstr(inst->tag_path, "race"))
			{
				kind = (inst->netgame_flag_type == _netgame_flag_race_vehicle) ? "RACE VEH" : "RACE MARKER";
				border_col = 0xFFFFFFFFu;
			}

			snprintf(lbl, sizeof(lbl), "%s #%d", kind, (int)inst->team_designator);
			draw_ce_menu_str_box(0, font_index, (short)(sx - 90), (short)(sy + 8), (short)(sx + 90), (short)(sy + 34), 2, lbl, border_col);
		}
	}
	set_forge_hud_ui_scale(1.0f, 0.0f, 0.0f);
}

void forge_3d_draw_hud(
	short local_player_index,
	short window_x0,
	short window_y0,
	short window_x1,
	short window_y1,
	long font_index,
	short line_height)
{
	static char const *const cat_names[NUMBER_OF_FORGE_PALETTE_CATEGORIES] = {
		"WEAPONS",
		"EQUIPMENT",
		"VEHICLES",
		"BIPEDS",
		"SCENERY",
		"DOORS",
		"LIGHTS",
		"SPAWNS",
		"OBJECTIVES",
		"AI SQUADS"
	};
	static char const *const snap_names[4] = { "OFF", "15 DEG", "45 DEG", "90 DEG" };
	char buf[256];
	short w = (short)(window_x1 - window_x0);
	short h = (short)(window_y1 - window_y0);

	(void)line_height;

	/* Never draw any Forge overlay on the main menu (levels\ui\ui) or when not in a Forge session */
	if (!g_forge_session_enabled || local_player_index != 0 || w <= 0 || h <= 0)
		return;
	if (forge_is_main_menu_active())
		return;
	if (ui_widgets_active())
		return;

	/* Ensure customizable UI theme & PNG backgrounds from OpenCE Forge/ui/ are loaded */
	forge_ui_theme_ensure_loaded();

	/* Draw 3D-projected Player Spawns & Teleporter / Objective floating labels */
	draw_teleporter_and_objective_markers(window_x0, window_y0, window_x1, window_y1, font_index);

	{
		short bar_top_y = (short)(window_y1 - 26);
		char const *p_toggle = forge_action_prompt(FORGE_BIND_TOGGLE_MODE);
		char const *p_pal = forge_action_prompt(FORGE_BIND_PALETTE);
		char const *p_view = forge_action_prompt(FORGE_BIND_THIRD_PERSON);
		char const *p_qsave = forge_action_prompt(FORGE_BIND_QUICK_SAVE);
		char const *p_rot = forge_action_prompt(FORGE_BIND_ROTATE_HOLD);
		char const *p_floor = forge_action_prompt(FORGE_BIND_SNAP_FLOOR);
		char const *p_surf = forge_action_prompt(FORGE_BIND_SNAP_SURFACE);
		char const *p_snap = forge_action_prompt(FORGE_BIND_ROT_SNAP);
		char const *p_opts = forge_action_prompt(FORGE_BIND_OBJECT_OPTIONS);
		char const *p_del = forge_action_prompt(FORGE_BIND_DELETE_OBJECT);
		char const *p_copy = forge_action_prompt(FORGE_BIND_COPY_OBJECT);
		char const *p_click = g_forge.using_controller ? "{A}" : "[CLICK]";
		char const *p_dist = g_forge.using_controller ? "{LS}" : "[SCROLL]";
		char const *p_esc_save = g_forge.using_controller ? "{START}" : "[ESC]";

		/* Spartan Play Mode: Bottom bar dynamically sized to fit text pixel width */
		if (g_forge.mode == _forge_mode_play && g_forge.active_modal == _forge_modal_none)
		{
			if (g_forge.has_saved_map_once)
				snprintf(buf, sizeof(buf), "%s FORGE   %s QUICK SAVE", p_toggle, p_qsave);
			else
				snprintf(buf, sizeof(buf), "%s FORGE", p_toggle);
			draw_sized_bottom_pill_bar(
				font_index, window_x0, window_x1, (short)(window_y1 - 2),
				buf, NULL, 0, _forge_ui_area_bottom_bar, 0xFF2896FFu, &bar_top_y);
		}
		else if (g_forge.mode == _forge_mode_3d_monitor)
		{
			/* Clean Unarmed Monitor Reticle at screen center */
			short cx = (short)((window_x0 + window_x1) / 2);
			short cy = (short)((window_y0 + window_y1) / 2);
			uint32_t ret_col = (g_forge.grabbed_live_index >= 0 || g_forge.hovered_live_index >= 0 || g_forge.hovered_object_index != NONE || g_forge.hovered_object_label[0])
				? 0xFFFFFFFFu : 0xFF2896FFu;
			rectangle2d rt;
			short px0, py0, px1, py1;

			set_forge_hud_ui_scale(FORGE_IN_MATCH_HUD_SCALE, (float)cx, (float)cy);

			/* Center dot */
			rt.x0 = (short)(cx - 1); rt.y0 = (short)(cy - 1); rt.x1 = (short)(cx + 2); rt.y1 = (short)(cy + 2);
			draw_quad(&rt, ret_col);
			/* Four unarmed bracket ticks around center */
			rt.x0 = (short)(cx - 11); rt.y0 = (short)(cy - 1); rt.x1 = (short)(cx - 5); rt.y1 = (short)(cy + 2);
			draw_quad(&rt, ret_col);
			rt.x0 = (short)(cx + 6); rt.y0 = (short)(cy - 1); rt.x1 = (short)(cx + 12); rt.y1 = (short)(cy + 2);
			draw_quad(&rt, ret_col);
			rt.x0 = (short)(cx - 1); rt.y0 = (short)(cy - 11); rt.x1 = (short)(cx + 2); rt.y1 = (short)(cy - 5);
			draw_quad(&rt, ret_col);
			rt.x0 = (short)(cx - 1); rt.y0 = (short)(cy + 6); rt.x1 = (short)(cx + 2); rt.y1 = (short)(cy + 12);
			draw_quad(&rt, ret_col);

			/* Show hovered object/marker label right below reticle in base-game UI font */
			if (g_forge.grabbed_live_index < 0 && g_forge.hovered_object_label[0])
			{
				char upper_lbl[96];
				format_clean_uppercase_item_name(g_forge.hovered_object_label, upper_lbl, sizeof(upper_lbl));
				snprintf(buf, sizeof(buf), "%s %s   %s COPY", p_click, upper_lbl, p_copy);
				draw_ce_menu_str_box(0, font_index, (short)(cx - 220), (short)(cy + 16), (short)(cx + 220), (short)(cy + 42), 2, buf, 0xFFFFFFFFu);
			}

			/* Top-Right Forge Mode Status & Budget Box (positioned right below the health/shield HUD,
			 * scaled down an additional 50% and styled with 9-slice rounded Halo CE box & UI fonts) */
			px0 = (short)(window_x1 - 236);
			py0 = (short)(window_y0 + 52);
			px1 = (short)(window_x1 - 12);
			py1 = (short)(py0 + 54);
			set_forge_hud_ui_scale(FORGE_COUNTER_HUD_SCALE, (float)px1, (float)py0);
			draw_ce_pause_box(px0, py0, (short)(px1 - px0), (short)(py1 - py0), 0);

			draw_ce_menu_str(1, font_index, (short)(px0 + 14), (short)(py0 + 6), "FORGE", 0xFFFFFFFFu);
			{
				int active_count = 0;
				int ii;
				for (ii = 0; ii < g_forge.live_instance_count; ii++)
				{
					if (g_forge.live_instances[ii].active)
						active_count++;
				}
				snprintf(buf, sizeof(buf), "%d / %d",
					active_count, FORGE_MAX_LIVE_INSTANCES);
			}
			draw_ce_menu_str_box(1, font_index, (short)(px0 + 84), (short)(py0 + 6), (short)(px1 - 14), (short)(py0 + 30), 1 /* right */,
				buf, 0xFFFFFFFFu);

			{
				struct scenario *hud_scen = global_scenario_try_and_get();
				if (hud_scen && hud_scen->structure_bsp_references.count > 1)
				{
					snprintf(buf, sizeof(buf), "%s SNAP: %s   [F7] BSP %d/%d",
						p_snap, snap_names[g_forge.snap_mode],
						(int)(global_structure_bsp_index_get() + 1),
						(int)hud_scen->structure_bsp_references.count);
				}
				else
				{
					snprintf(buf, sizeof(buf), "%s SNAP: %s",
						p_snap, snap_names[g_forge.snap_mode]);
				}
			}
			draw_ce_menu_str_box(0, font_index, (short)(px0 + 14), (short)(py0 + 28), (short)(px1 - 14), (short)(py1 - 6), 0,
				buf, 0xFF2896FFu);

			/* Bottom Control Strip dynamically sized to fit exact pixel width (and 2 lines when holding an item) */
			if (g_forge.active_modal == _forge_modal_none)
			{
				if (g_forge.grabbed_live_index >= 0)
				{
					char line1[160];
					char line2[160];
					char const *p_drop_cancel = g_forge.using_controller ? "HOLD {B} DROP" : "[ESC] CANCEL";
					snprintf(line1, sizeof(line1),
						"%s PLACE  %s %.0fM  %s ROTATE  %s SURFACE",
						p_click, p_dist, g_forge.grab_distance, p_rot, p_surf);
					snprintf(line2, sizeof(line2),
						"%s FLOOR  %s OPTIONS  %s DELETE  %s",
						p_floor, p_opts, p_del, p_drop_cancel);
					draw_sized_bottom_pill_bar(
						font_index, window_x0, window_x1, (short)(window_y1 - 2),
						line1, line2, 0, _forge_ui_area_bottom_bar, 0xFF2896FFu, &bar_top_y);
				}
				else
				{
					if (g_forge.has_saved_map_once)
					{
						snprintf(buf, sizeof(buf),
							"%s OBJECTS  %s GRAB  %s COPY  %s VIEW  %s SAVE  %s PLAY",
							p_pal, p_click, p_copy, p_view, p_qsave, p_toggle);
					}
					else
					{
						snprintf(buf, sizeof(buf),
							"%s OBJECTS  %s GRAB  %s COPY  %s VIEW  %s SAVE  %s PLAY",
							p_pal, p_click, p_copy, p_view, p_esc_save, p_toggle);
					}
					draw_sized_bottom_pill_bar(
						font_index, window_x0, window_x1, (short)(window_y1 - 2),
						buf, NULL, 0, _forge_ui_area_bottom_bar, 0xFF2896FFu, &bar_top_y);
				}
			}
		}

		/* ========================================================================
		 * UNIFIED TABBED FORGE OBJECT PALETTE MENU (F2) - BASE GAME MENU STYLING
		 * ======================================================================== */
		if (g_forge.active_modal == _forge_modal_spawn_palette)
		{
			int filtered[512];
			int fcount = get_filtered_catalog_indices(filtered, 512);
			int r, max_rows = 10;
			short box_w = 296;
			short box_h = 336;
			short px0 = (short)(window_x0 + 16);
			short py0 = (short)(window_y0 + 34);
			short px1 = (short)(px0 + box_w);
			short py1 = (short)(py0 + box_h);
			short spin_x0 = (short)(px0 + 14);
			short spin_y0 = (short)(py0 + 32);
			short spin_x1 = (short)(px1 - 14);
			short spin_y1 = (short)(spin_y0 + 26);
			short y_cur;
			char clean_name[64];

			set_forge_hud_ui_scale(FORGE_IN_MATCH_HUD_SCALE, (float)px0, (float)py0);

			/* Authentic Halo CE 9-slice Objectives/Pause Box container */
			draw_ce_pause_box(px0, py0, box_w, box_h, 1);

			/* Top Header Bar Text in ui\large_ui */
			draw_ce_menu_str(1, font_index, (short)(px0 + 14), (short)(py0 + 4),
				"FORGE PALETTE", 0xFFFFFFFFu);
			snprintf(buf, sizeof(buf), "%d / %d", g_forge.live_instance_count, FORGE_MAX_LIVE_INSTANCES);
			draw_ce_menu_str_box(0, font_index, (short)(px1 - 110), (short)(py0 + 6), (short)(px1 - 14), (short)(py0 + 28), 1 /* right */,
				buf, 0xFF2896FFu);

			/* Category Spinner Row (3-slice rounded option_bkds_sm + arrow_sm_left + arrow_sm_right + ui\large_ui) */
			draw_ce_rounded_row_bar(1, spin_x0, spin_y0, spin_x1, spin_y1);
			draw_ce_menu_bitmap_cropped("pc\\bitmaps\\arrow_sm_left", 0, (short)(spin_x0 + 6), (short)(spin_y0 + 7), (short)(spin_x0 + 12), (short)(spin_y0 + 19), 6, 12, 0xFFFFFFFFu);
			draw_ce_menu_bitmap_cropped("pc\\bitmaps\\arrow_sm_right", 0, (short)(spin_x1 - 12), (short)(spin_y0 + 7), (short)(spin_x1 - 6), (short)(spin_y0 + 19), 6, 12, 0xFFFFFFFFu);

			snprintf(buf, sizeof(buf), "%s (%d/%d)",
				cat_names[g_forge.current_category],
				fcount > 0 ? (g_forge.palette_cursor + 1) : 0,
				fcount);
			draw_ce_menu_str_box(1, font_index, (short)(spin_x0 + 16), (short)(spin_y0 + 2), (short)(spin_x1 - 16), spin_y1, 2 /* center */,
				buf, 0xFFFFFFFFu);

			/* Horizontal Blue Divider Line (bitmaps/blue) */
			draw_ce_blue_line((short)(px0 + 14), (short)(spin_y1 + 3), (short)(px1 - 14), 2);

			/* 10 Item Rows with rounded highlight on BOTH left and right ends when selected */
			y_cur = (short)(spin_y1 + 8);
			for (r = 0; r < max_rows && (g_forge.palette_scroll + r) < fcount; r++)
			{
				int idx = g_forge.palette_scroll + r;
				struct forge_catalog_entry const *ce = &g_forge.catalog[filtered[idx]];
				int sel = (idx == g_forge.palette_cursor);
				int placed_cnt = count_active_instances_for_catalog_entry(ce);
				uint32_t row_col = sel ? 0xFFFFFFFFu : 0xFF2896FFu;
				short rx0 = (short)(px0 + 14);
				short ry0 = y_cur;
				short rx1 = (short)(px1 - 14);
				short ry1 = (short)(ry0 + 22);

				if (sel)
					draw_ce_rounded_row_bar(1, rx0, ry0, rx1, ry1);

				format_clean_uppercase_item_name(ce->display_name, clean_name, sizeof(clean_name));
				draw_ce_menu_str_box(0, font_index, (short)(rx0 + 10), (short)(ry0 + 1), (short)(rx1 - 38), ry1, 0,
					clean_name, row_col);

				snprintf(buf, sizeof(buf), "%d", placed_cnt);
				draw_ce_menu_str_box(0, font_index, (short)(rx1 - 40), (short)(ry0 + 1), (short)(rx1 - 10), ry1, 1 /* right */,
					buf, row_col);

				y_cur = (short)(y_cur + 23);
			}

			/* Scroll indicator arrows using authentic arrow_sm_up / arrow_sm_down bitmaps */
			if (g_forge.palette_scroll > 0)
			{
				draw_ce_menu_bitmap_cropped("pc\\bitmaps\\arrow_sm_up", 1,
					(short)(px1 - 24), (short)(spin_y1 + 9), (short)(px1 - 12), (short)(spin_y1 + 21), 16, 16, 0xFFFFFFFFu);
			}
			if (g_forge.palette_scroll + max_rows < fcount)
			{
				draw_ce_menu_bitmap_cropped("pc\\bitmaps\\arrow_sm_down", 1,
					(short)(px1 - 24), (short)(py1 - 50), (short)(px1 - 12), (short)(py1 - 38), 16, 16, 0xFFFFFFFFu);
			}

			/* Footer Action Buttons using authentic bitmaps/text_button_background */
			{
				int can_cache_sel = (fcount > 0 && g_forge.palette_cursor >= 0 && g_forge.palette_cursor < fcount &&
					g_forge.catalog[filtered[g_forge.palette_cursor]].source_cache_id == 3);
				if (can_cache_sel)
				{
					draw_ce_text_button(font_index, (short)(px0 + 14), (short)(py1 - 32), 84, 24, 1,
						g_forge.using_controller ? "{A} PLACE" : "[ENT] PLACE");
					draw_ce_text_button(font_index, (short)(px0 + 106), (short)(py1 - 32), 84, 24, 0,
						g_forge.using_controller ? "{Y} CACHE" : "[C] CACHE");
					draw_ce_text_button(font_index, (short)(px1 - 98), (short)(py1 - 32), 84, 24, 0,
						g_forge.using_controller ? "{B} DONE" : "[ESC] DONE");
				}
				else
				{
					draw_ce_text_button(font_index, (short)(px0 + 14), (short)(py1 - 32), 128, 24, 1,
						g_forge.using_controller ? "{A} PLACE" : "[ENTER] PLACE");
					draw_ce_text_button(font_index, (short)(px1 - 142), (short)(py1 - 32), 128, 24, 0,
						g_forge.using_controller ? "{B} DONE" : "[ESC] DONE");
				}
			}
		}

		/* ========================================================================
		 * OBJECT PROPERTIES PANEL (X when holding an object) - BASE GAME MENU STYLING
		 * ======================================================================== */
		if (g_forge.active_modal == _forge_modal_object_properties && g_forge.grabbed_live_index >= 0)
		{
			uint8_t row_ids[FORGE_MAX_PROP_ROWS];
			char labels[FORGE_MAX_PROP_ROWS][40];
			char values[FORGE_MAX_PROP_ROWS][48];
			int row_cnt = forge_build_object_prop_rows(g_forge.grabbed_live_index, row_ids, labels, values);
			int vis_rows = (row_cnt > 8) ? 8 : (row_cnt > 0 ? row_cnt : 1);
			int scroll_off = g_forge.props_scroll;
			short box_w = 328;
			short box_h = (short)(84 + vis_rows * 28);
			short px0 = (short)(window_x0 + 16);
			short py0 = (short)(window_y0 + 34);
			short px1 = (short)(px0 + box_w);
			short py1 = (short)(py0 + box_h);
			int r;

			if (scroll_off + vis_rows > row_cnt)
				scroll_off = row_cnt - vis_rows;
			if (scroll_off < 0)
				scroll_off = 0;

			set_forge_hud_ui_scale(FORGE_IN_MATCH_HUD_SCALE, (float)px0, (float)py0);

			draw_ce_pause_box(px0, py0, box_w, box_h, 1);

			draw_ce_menu_str(1, font_index, (short)(px0 + 14), (short)(py0 + 4),
				"OBJECT PROPERTIES", 0xFFFFFFFFu);
			if (row_cnt > vis_rows)
			{
				snprintf(buf, sizeof(buf), "%d/%d", g_forge.props_cursor + 1, row_cnt);
				draw_ce_menu_str_box(0, font_index, (short)(px1 - 76), (short)(py0 + 6), (short)(px1 - 14), (short)(py0 + 28), 1 /* right */,
					buf, 0xFF2896FFu);
			}

			for (r = 0; r < vis_rows && (scroll_off + r) < row_cnt; r++)
			{
				int idx = scroll_off + r;
				short rx0 = (short)(px0 + 14);
				short ry0 = (short)(py0 + 34 + r * 28);
				short rx1 = (short)(px1 - 14);
				short ry1 = (short)(ry0 + 25);
				int sel = (g_forge.props_cursor == idx);
				uint32_t col = sel ? 0xFFFFFFFFu : 0xFF2896FFu;

				draw_ce_rounded_row_bar((short)(sel ? 1 : 0), rx0, ry0, rx1, ry1);

				draw_ce_menu_str_box(1, font_index, (short)(rx0 + 10), (short)(ry0 + 2), (short)(rx1 - 10), ry1, 0, labels[idx], col);
				if (values[idx][0])
				{
					short vx0 = (short)(rx0 + 128);
					short vx1 = (short)(rx1 - 8);
					draw_ce_menu_bitmap_cropped("pc\\bitmaps\\arrow_sm_left", 0, vx0, (short)(ry0 + 6), (short)(vx0 + 6), (short)(ry0 + 18), 6, 12, 0xFFFFFFFFu);
					draw_ce_menu_bitmap_cropped("pc\\bitmaps\\arrow_sm_right", 0, (short)(vx1 - 6), (short)(ry0 + 6), vx1, (short)(ry0 + 18), 6, 12, 0xFFFFFFFFu);
					draw_ce_menu_str_box(0, font_index, (short)(vx0 + 10), (short)(ry0 + 3), (short)(vx1 - 10), ry1, 2 /* center */, values[idx], col);
				}
			}

			if (scroll_off > 0)
			{
				draw_ce_menu_bitmap_cropped("pc\\bitmaps\\arrow_sm_up", 1,
					(short)(px1 - 24), (short)(py0 + 22), (short)(px1 - 12), (short)(py0 + 34), 16, 16, 0xFFFFFFFFu);
			}
			if (scroll_off + vis_rows < row_cnt)
			{
				draw_ce_menu_bitmap_cropped("pc\\bitmaps\\arrow_sm_down", 1,
					(short)(px1 - 24), (short)(py1 - 46), (short)(px1 - 12), (short)(py1 - 34), 16, 16, 0xFFFFFFFFu);
			}

			draw_ce_text_button(font_index, (short)(px0 + 14), (short)(py1 - 32), 136, 24, 1,
				g_forge.using_controller ? "{A} SELECT" : "[ENTER] SELECT");
			draw_ce_text_button(font_index, (short)(px1 - 150), (short)(py1 - 32), 136, 24, 0,
				g_forge.using_controller ? "{B} DONE" : "[ESC] DONE");
		}

		/* ========================================================================
		 * INITIAL SAVE MAP DIALOG BOX (Opened from Esc -> SAVE MAP..)
		 * ======================================================================== */
		if (g_forge.active_modal == _forge_modal_save_copy_as)
		{
			short cx = (short)((window_x0 + window_x1) / 2);
			short cy = (short)((window_y0 + window_y1) / 2);
			short box_w = 416;
			short box_h = 225;
			short px0 = (short)(cx - box_w / 2);
			short py0 = (short)(cy - box_h / 2 - 8);
			short px1 = (short)(px0 + box_w);
			short py1 = (short)(py0 + box_h);
			short in_x0 = (short)(px0 + 28);
			short in_y0 = (short)(py0 + 74);
			short in_x1 = (short)(px1 - 28);
			short in_y1 = (short)(in_y0 + 28);
			size_t name_len = strlen(g_forge.save_map_name);
			char const *visible_input = g_forge.save_map_name;
			char short_name[32];

			set_forge_hud_ui_scale(FORGE_IN_MATCH_HUD_SCALE, (float)cx, (float)cy);

			if (name_len > 22)
				visible_input = g_forge.save_map_name + (name_len - 22);

			if (name_len > 18)
				snprintf(short_name, sizeof(short_name), "%.15s...", g_forge.save_map_name);
			else
				snprintf(short_name, sizeof(short_name), "%s", g_forge.save_map_name);

			draw_ce_pause_box(px0, py0, box_w, box_h, 1);

			if (s_save_confirm_overwrite)
			{
				char const *save_ext = mcc_cache_tags_loaded() ? "txt" : "map";
				draw_ce_menu_str(1, font_index, (short)(px0 + 18), (short)(py0 + 4),
					"MAP ALREADY EXISTS", 0xFFFFFFFFu);

				if (s_save_confirm_in_maps && s_save_confirm_in_custom_maps)
					snprintf(buf, sizeof(buf), "Map '%s.%s' exists in maps\\ and custom_maps\\!", short_name, save_ext);
				else if (s_save_confirm_in_maps)
					snprintf(buf, sizeof(buf), "Map '%s.%s' already exists in the maps\\ folder!", short_name, save_ext);
				else
					snprintf(buf, sizeof(buf), "Map '%s.%s' already exists in custom_maps\\!", short_name, save_ext);

				draw_ce_menu_str_box(0, font_index, in_x0, (short)(py0 + 44), in_x1, (short)(py0 + 68), 0,
					buf, 0xFFFFB428u);

				if (!draw_ce_rounded_row_bar(1, in_x0, in_y0, in_x1, in_y1))
				{
					rectangle2d ib = { in_y0, in_x0, in_y1, in_x1 };
					draw_quad(&ib, 0xE8040C1Cu);
					draw_box_border(in_x0, in_y0, in_x1, in_y1, 1, 0xFF2896FFu);
				}

				snprintf(buf, sizeof(buf), "custom_maps\\%s.%s", visible_input, save_ext);
				draw_ce_menu_str_box(1, font_index, (short)(in_x0 + 14), (short)(in_y0 + 3), (short)(in_x1 - 14), in_y1, 0,
					buf, 0xFFFFFFFFu);

				draw_ce_blue_line(in_x0, (short)(py0 + 114), in_x1, 2);

				draw_ce_menu_str_box(0, font_index, in_x0, (short)(py0 + 124), in_x1, (short)(py0 + 144), 0,
					"Do you want to overwrite it or change the map name?", 0xFFFFFFFFu);

				draw_ce_menu_str_box(0, font_index, in_x0, (short)(py0 + 146), in_x1, (short)(py0 + 168), 0,
					g_forge.using_controller
						? "{DL}/{DR} Select   {A} Confirm   {B} Change Name"
						: "[LEFT]/[RIGHT] Select   [ENTER] Confirm   [ESC] Change Name",
					0xFF2896FFu);

				draw_ce_text_button(font_index, (short)(cx - 166), (short)(py1 - 34), 156, 24,
					(s_save_confirm_choice == 0) ? 1 : 0,
					g_forge.using_controller ? "{A} OVERWRITE" : "OVERWRITE");
				draw_ce_text_button(font_index, (short)(cx + 10), (short)(py1 - 34), 156, 24,
					(s_save_confirm_choice == 1) ? 1 : 0,
					g_forge.using_controller ? "{B} CHANGE NAME" : "CHANGE NAME");
			}
			else
			{
				char const *save_ext = mcc_cache_tags_loaded() ? "txt" : "map";
				draw_ce_menu_str(1, font_index, (short)(px0 + 18), (short)(py0 + 4),
					"SAVE MAP..", 0xFFFFFFFFu);

				draw_ce_menu_str(1, font_index, in_x0, (short)(py0 + 44),
					"ENTER FORGE MAP NAME:", 0xFF2896FFu);

				if (!draw_ce_rounded_row_bar(1, in_x0, in_y0, in_x1, in_y1))
				{
					rectangle2d ib = { in_y0, in_x0, in_y1, in_x1 };
					draw_quad(&ib, 0xE8040C1Cu);
					draw_box_border(in_x0, in_y0, in_x1, in_y1, 1, 0xFF2896FFu);
				}

				snprintf(buf, sizeof(buf), "%s_", visible_input);
				draw_ce_menu_str_box(1, font_index, (short)(in_x0 + 14), (short)(in_y0 + 3), (short)(in_x1 - 14), in_y1, 0,
					buf, 0xFFFFFFFFu);

				draw_ce_blue_line(in_x0, (short)(py0 + 114), in_x1, 2);

				snprintf(buf, sizeof(buf), "Saves as: custom_maps\\%s.%s", short_name, save_ext);
				draw_ce_menu_str_box(0, font_index, in_x0, (short)(py0 + 124), in_x1, (short)(py0 + 144), 0,
					buf, 0xFF2896FFu);

				snprintf(buf, sizeof(buf), "After initial save, press %s in-game for Quick Save.", p_qsave);
				draw_ce_menu_str_box(0, font_index, in_x0, (short)(py0 + 146), in_x1, (short)(py0 + 168), 0,
					buf, 0xFF2896FFu);

				draw_ce_text_button(font_index, (short)(cx - 138), (short)(py1 - 34), 128, 24, 1,
					g_forge.using_controller ? "{A} SAVE" : "[ENTER] SAVE");
				draw_ce_text_button(font_index, (short)(cx + 10), (short)(py1 - 34), 128, 24, 0,
					g_forge.using_controller ? "{B} CANCEL" : "[ESC] CANCEL");
			}
		}

		/* Status Toast Banner dynamically sized to fit text pixel width */
		{
			char dl_status[128];
			int has_toast = (g_forge.status_message[0] && (uint32_t)system_milliseconds() < g_forge.status_message_until_ms) ? 1 : 0;
			int has_dl = forge_network_get_tag_download_status(dl_status, sizeof(dl_status));
			if (has_toast)
			{
				draw_sized_bottom_pill_bar(
					font_index, window_x0, window_x1, (short)(bar_top_y - 4),
					g_forge.status_message, NULL, 1, _forge_ui_area_toast_bar, 0xFFFFFFFFu, NULL);
			}
			if (has_dl && dl_status[0])
			{
				short dl_bottom_y = (short)(bar_top_y - (has_toast ? 34 : 4));
				draw_sized_bottom_pill_bar(
					font_index, window_x0, window_x1, dl_bottom_y,
					dl_status, NULL, 1, _forge_ui_area_toast_bar, 0xFF50B8FFu, NULL);
			}
		}
	}

	set_forge_hud_ui_scale(1.0f, 0.0f, 0.0f);
}
