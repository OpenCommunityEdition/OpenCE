/*
FORGE_SIDECAR.C

Read-only runtime playback of OpenCE Forge .txt sidecar maps using active-map
[MAP] tags only (no Forge editor, no network sync, and no external tag cache).
*/

/* ---------- headers */

#ifndef HALO_RELEASE
#define HALO_RELEASE
#endif

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "main/main.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"
#include "units/units.h"

#include "custom_edition_maps.h"
#include "forge_cache_builder.h"
#include "forge_sidecar.h"

#undef fopen
#undef fclose
#undef fread
#undef fseek
#undef malloc
#undef free

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <strings.h>
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#endif

/* ---------- externs */

extern void platform_translate_path(char const *xbox_path, char *out, unsigned long out_size);
extern void *cache_files_tag_instances(long *count);
extern short ui_widget_port_multiplayer_maps(char const *const **names, short *last_used);
extern boolean ui_widget_port_multiplayer_map_choose(short level_index);
extern boolean main_menu_is_active(void);

/* ---------- constants */

#define FORGE_SIDECAR_MAX_INSTANCES 512
#define FORGE_SIDECAR_PHYSICS_COUNT 3
#define FORGE_SIDECAR_MAX_SPAWNS 256
#define FORGE_SIDECAR_MAX_FLAGS 128

enum
{
	FORGE_SIDECAR_PHYSICS_NORMAL = 0,
	FORGE_SIDECAR_PHYSICS_FIXED = 1,
	FORGE_SIDECAR_PHYSICS_PHASED = 2
};

/* ---------- structures */

struct forge_sidecar_tag_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char const *name;
	void *base_address;
	unsigned long unused[2];
};

typedef char forge_sidecar_tag_instance_size_assert[
	sizeof(struct forge_sidecar_tag_instance) == 32 ? 1 : -1];

struct forge_sidecar_deleted_eq
{
	float orig_x;
	float orig_y;
	float orig_z;
};

struct forge_sidecar_instance
{
	uint8_t active;
	uint8_t is_default_scenario_object;
	uint8_t deleted_default;
	uint8_t hidden_by_gametype;
	uint8_t physics_mode;
	uint16_t gametype_flags;
	int16_t team_designator;
	int16_t respawn_timer_sec;
	int32_t death_tick;
	long object_index;
	long definition_index;
	uint32_t group_tag;
	float x;
	float y;
	float z;
	float yaw;
	float pitch;
	float roll;
	char tag_path[256];
};

struct forge_sidecar_spawn
{
	float x;
	float y;
	float z;
	float yaw;
	int16_t team_index;
	uint16_t gametype_flags;
};

struct forge_sidecar_netgame_flag
{
	float x;
	float y;
	float z;
	float yaw;
	int16_t type;
	int16_t team_designator;
};

/* ---------- globals */

static char s_active_map_name[256];
static char s_pending_sidecar_leaf[128];
static int s_sidecar_applied = 0;

static struct forge_sidecar_instance s_instances[FORGE_SIDECAR_MAX_INSTANCES];
static int s_instance_count = 0;

static struct forge_sidecar_deleted_eq s_deleted_eq[FORGE_MAX_DEFAULT_OBJECT_EDITS];
static int s_deleted_eq_count = 0;

static struct forge_sidecar_spawn s_spawns[FORGE_SIDECAR_MAX_SPAWNS];
static int s_spawn_count = 0;
static int s_replace_spawns = 0;

static struct forge_sidecar_netgame_flag s_flags[FORGE_SIDECAR_MAX_FLAGS];
static int s_flag_count = 0;
static int s_replace_flags = 0;

/* ---------- helpers */

static char const *leaf_from_path(char const *path)
{
	char const *bs;
	char const *fs;

	if (!path)
		return "";
	bs = strrchr(path, '\\');
	fs = strrchr(path, '/');
	if (bs && (!fs || bs > fs))
		return bs + 1;
	return fs ? (fs + 1) : path;
}

static void reset_sidecar_runtime(void)
{
	s_sidecar_applied = 0;
	s_instance_count = 0;
	s_deleted_eq_count = 0;
	s_spawn_count = 0;
	s_replace_spawns = 0;
	s_flag_count = 0;
	s_replace_flags = 0;
}

static void euler_to_forward_up(
	float yaw,
	float pitch,
	float roll,
	real_vector3d *forward,
	real_vector3d *up)
{
	real_euler_angles3d angles;

	angles.yaw = yaw;
	angles.pitch = pitch;
	angles.roll = roll;
	vectors3d_from_euler_angles3d(forward, up, &angles);
}

static char *read_entire_text_file(char const *host_path, size_t *out_size)
{
	FILE *f;
	long fsz;
	char *buf;
	size_t nread;

	f = fopen(host_path, "rb");
	if (!f)
		return NULL;
	if (fseek(f, 0, SEEK_END) != 0)
	{
		fclose(f);
		return NULL;
	}
	fsz = ftell(f);
	if (fsz <= 0 || fsz > 1024 * 1024)
	{
		fclose(f);
		return NULL;
	}
	fseek(f, 0, SEEK_SET);
	buf = (char *)malloc((size_t)fsz + 1);
	if (!buf)
	{
		fclose(f);
		return NULL;
	}
	nread = fread(buf, 1, (size_t)fsz, f);
	fclose(f);
	buf[nread] = '\0';
	if (out_size)
		*out_size = nread;
	return buf;
}

static int sidecar_read_base_map(char const *text_buf, char *out_base, size_t out_sz)
{
	char const *cur = text_buf;
	char line[512];
	int is_forge = 0;

	if (!text_buf || !out_base || out_sz == 0)
		return 0;
	out_base[0] = '\0';

	while (*cur)
	{
		size_t len = 0;
		while (cur[len] && cur[len] != '\r' && cur[len] != '\n')
			len++;
		if (len >= sizeof(line))
			len = sizeof(line) - 1;
		memcpy(line, cur, len);
		line[len] = '\0';
		cur += len;
		while (*cur == '\r' || *cur == '\n')
			cur++;

		if (strcmp(line, "[OpenCE Forge Map]") == 0 || strcmp(line, "[ForgeState]") == 0)
		{
			is_forge = 1;
		}
		else if (_strnicmp(line, "Base Map:", 9) == 0)
		{
			char const *p = line + 9;
			while (*p == ' ' || *p == '\t')
				p++;
			snprintf(out_base, out_sz, "%s", p);
		}
	}
	return is_forge && out_base[0] != '\0';
}

static short find_mp_level_index(
	char const *const *names,
	short total,
	short xbox_cnt,
	short skip_index,
	char const *base_map)
{
	char const *bm_leaf;
	int prefer_ce = 0;
	short k;

	if (!names || total <= 0 || !base_map || !base_map[0])
		return NONE;
	if (_strnicmp(base_map, "custom_maps\\", 12) == 0 ||
		_strnicmp(base_map, "custom_maps/", 12) == 0)
	{
		prefer_ce = 1;
	}
	bm_leaf = leaf_from_path(base_map);

	for (k = 0; k < total; k++)
	{
		if (k != skip_index && names[k] && _stricmp(names[k], base_map) == 0)
			return k;
	}
	if (!prefer_ce)
	{
		for (k = 0; k < xbox_cnt && k < total; k++)
		{
			if (k != skip_index && names[k] && _stricmp(leaf_from_path(names[k]), bm_leaf) == 0)
				return k;
		}
	}
	for (k = 0; k < total; k++)
	{
		if (k != skip_index && names[k] && _stricmp(leaf_from_path(names[k]), bm_leaf) == 0)
			return k;
	}
	return NONE;
}

static int is_object_group_tag(uint32_t group_tag)
{
	return group_tag == 'scen' || group_tag == 'vehi' || group_tag == 'weap' ||
		group_tag == 'eqip' || group_tag == 'mach' || group_tag == 'ctrl' ||
		group_tag == 'bloc' || group_tag == 'bipd';
}

static long resolve_active_map_object_tag(uint32_t group_tag, char const *tag_path)
{
	long def_idx;
	long count = 0;
	long i;
	struct forge_sidecar_tag_instance *instances;

	if (!tag_path || !tag_path[0] || !is_object_group_tag(group_tag))
		return NONE;

	def_idx = tag_loaded((tag)group_tag, tag_path);
	if (def_idx != NONE)
		return def_idx;

	instances = (struct forge_sidecar_tag_instance *)cache_files_tag_instances(&count);
	if (!instances || count <= 0)
		return NONE;

	for (i = 0; i < count; i++)
	{
		if (instances[i].tag_index == NONE || !instances[i].base_address || !instances[i].name)
			continue;
		if ((uint32_t)instances[i].group_tag == group_tag &&
			_stricmp(instances[i].name, tag_path) == 0)
		{
			return instances[i].tag_index;
		}
	}
	for (i = 0; i < count; i++)
	{
		uint32_t g;
		if (instances[i].tag_index == NONE || !instances[i].base_address || !instances[i].name)
			continue;
		g = (uint32_t)instances[i].group_tag;
		if (is_object_group_tag(g) && _stricmp(instances[i].name, tag_path) == 0)
			return instances[i].tag_index;
	}
	return NONE;
}

static int gametype_allows_flags(uint16_t flags)
{
	struct game_variant *variant;
	long engine_index;

	if (flags == 0 || flags >= 0x0FFFu)
		return 1;
	if (!game_engine_running())
		return 1;
	if (flags == 0x0020u)
		return 0;

	variant = game_engine_get_variant();
	if (!variant)
		return 1;
	engine_index = variant->game_engine_index;
	if (engine_index >= 1 && engine_index <= 5)
		return (flags & (uint16_t)(1u << (engine_index - 1))) != 0;
	return 1;
}

static long spawn_object_at(
	long definition_index,
	float x,
	float y,
	float z,
	float yaw,
	float pitch,
	float roll,
	int16_t team_designator)
{
	struct object_placement_data placement;
	long object_index;

	if (definition_index == NONE)
		return NONE;

	object_placement_data_new(&placement, definition_index, NONE);
	placement.position.x = x;
	placement.position.y = y;
	placement.position.z = z;
	euler_to_forward_up(yaw, pitch, roll, &placement.forward, &placement.up);
	placement.owner_team_index = (team_designator >= 0 && team_designator < 8) ? team_designator : NONE;
	object_index = object_new(&placement);
	return object_index;
}

static void move_existing_object(
	long object_index,
	float x,
	float y,
	float z,
	float yaw,
	float pitch,
	float roll,
	uint8_t physics_mode)
{
	real_point3d pos;
	real_vector3d fwd;
	real_vector3d up;
	struct object_datum *obj;

	if (object_index == NONE)
		return;
	obj = (struct object_datum *)object_try_and_get(object_index);
	if (!obj)
		return;

	pos.x = x;
	pos.y = y;
	pos.z = z;
	euler_to_forward_up(yaw, pitch, roll, &fwd, &up);
	object_set_position(object_index, &pos, &fwd, &up);

	if (physics_mode == FORGE_SIDECAR_PHYSICS_FIXED ||
		physics_mode == FORGE_SIDECAR_PHYSICS_PHASED)
	{
		obj->object.translational_velocity.i = 0.0f;
		obj->object.translational_velocity.j = 0.0f;
		obj->object.translational_velocity.k = 0.0f;
		obj->object.angular_velocity.i = 0.0f;
		obj->object.angular_velocity.j = 0.0f;
		obj->object.angular_velocity.k = 0.0f;
	}
}

static long find_default_object_near(
	float ox,
	float oy,
	float oz,
	char const *tag_path)
{
	struct object_iterator iter;
	struct object_datum *obj;
	long best_oidx = NONE;
	float best_d2 = 36.0f;
	int i;

	object_iterator_new(&iter, (unsigned long)-1, 0);
	while ((obj = object_iterator_next(&iter)) != NULL)
	{
		float dx = obj->object.position.x - ox;
		float dy = obj->object.position.y - oy;
		float dz = obj->object.position.z - oz;
		float d2 = dx * dx + dy * dy + dz * dz * 0.25f;
		int claimed = 0;

		if (obj->object.parent_object_index != NONE)
			continue;
		if (obj->object.type == _object_type_biped &&
			((struct unit_datum *)obj)->unit.player_index != NONE)
		{
			continue;
		}
		if (obj->definition_index != NONE && tag_path && tag_path[0])
		{
			char const *oname = tag_get_name(obj->definition_index);
			if (oname && oname[0] && _stricmp(oname, tag_path) != 0)
				continue;
		}
		for (i = 0; i < s_instance_count; i++)
		{
			if (s_instances[i].active && s_instances[i].object_index == iter.index)
			{
				claimed = 1;
				break;
			}
		}
		if (claimed)
			continue;
		if (dx * dx + dy * dy <= 36.0f && (float)fabs((double)dz) <= 12.0f && d2 < best_d2)
		{
			best_d2 = d2;
			best_oidx = iter.index;
		}
	}
	return best_oidx;
}

static void update_scenario_equipment_edit(
	float ox,
	float oy,
	float oz,
	float nx,
	float ny,
	float nz,
	float nyaw,
	int deleted)
{
	struct scenario *scen = global_scenario_try_and_get();
	long si;

	if (!scen)
		return;
	for (si = 0; si < scen->netgame_equipment.count; si++)
	{
		struct scenario_netgame_equipment *eq = TAG_BLOCK_GET_ELEMENT(
			&scen->netgame_equipment, si, struct scenario_netgame_equipment);
		float dx = eq->position.x - ox;
		float dy = eq->position.y - oy;
		float dz = eq->position.z - oz;
		if (dx * dx + dy * dy + dz * dz < 16.0f)
		{
			if (deleted)
			{
				eq->item_collection.index = NONE;
				eq->position.x = -99999.0f;
				eq->position.y = -99999.0f;
				eq->position.z = -99999.0f;
			}
			else
			{
				eq->position.x = nx;
				eq->position.y = ny;
				eq->position.z = nz;
				eq->facing = nyaw;
			}
			break;
		}
	}
}

static void apply_scenario_spawns(void)
{
	struct scenario *scen = global_scenario_try_and_get();
	int count = s_spawn_count;
	static struct encounter_player_starting_location s_sp_buf[FORGE_SIDECAR_MAX_SPAWNS];
	int i;

	if (!scen || !s_replace_spawns || count <= 0)
		return;
	if (count > FORGE_SIDECAR_MAX_SPAWNS)
		count = FORGE_SIDECAR_MAX_SPAWNS;

	memset(s_sp_buf, 0, sizeof(s_sp_buf));
	for (i = 0; i < count; i++)
	{
		int16_t td = s_spawns[i].team_index;
		s_sp_buf[i].position.x = s_spawns[i].x;
		s_sp_buf[i].position.y = s_spawns[i].y;
		s_sp_buf[i].position.z = s_spawns[i].z;
		s_sp_buf[i].facing = s_spawns[i].yaw;
		s_sp_buf[i].team_index = (td >= 0 && td < 2) ? td : 0;
		if (s_spawns[i].gametype_flags == 0x0001u)
			s_sp_buf[i].game_types[0] = 1;
		else if (s_spawns[i].gametype_flags == 0x0002u)
			s_sp_buf[i].game_types[0] = 2;
		else if (s_spawns[i].gametype_flags == 0x0004u)
			s_sp_buf[i].game_types[0] = 3;
		else if (s_spawns[i].gametype_flags == 0x0008u)
			s_sp_buf[i].game_types[0] = 4;
		else if (s_spawns[i].gametype_flags == 0x0010u)
			s_sp_buf[i].game_types[0] = 5;
		else
			s_sp_buf[i].game_types[0] = 12;
	}
	scen->players.address = s_sp_buf;
	scen->players.count = count;

	/* Relocate already-spawned players onto initial custom spawns */
	if (game_connection() != _game_connection_network_client && player_data)
	{
		long p_idx;
		int sp_cursor = 0;
		for (p_idx = data_next_index(player_data, NONE);
			 p_idx != NONE;
			 p_idx = data_next_index(player_data, p_idx))
		{
			struct player_datum *player = (struct player_datum *)datum_get(player_data, p_idx);
			if (player && player->unit_index != NONE)
			{
				struct forge_sidecar_spawn const *sp = &s_spawns[sp_cursor % count];
				real_point3d pos;
				real_vector3d fwd;
				real_vector3d up;
				pos.x = sp->x;
				pos.y = sp->y;
				pos.z = sp->z + 0.05f;
				euler_to_forward_up(sp->yaw, 0.0f, 0.0f, &fwd, &up);
				object_set_position(player->unit_index, &pos, &fwd, &up);
				sp_cursor++;
			}
		}
	}
}

static void apply_scenario_netgame_flags(void)
{
	struct scenario *scen = global_scenario_try_and_get();
	int count = s_flag_count;
	static struct scenario_netgame_flag s_nf_buf[FORGE_SIDECAR_MAX_FLAGS];
	int i;

	if (!scen || !s_replace_flags)
		return;
	if (count > FORGE_SIDECAR_MAX_FLAGS)
		count = FORGE_SIDECAR_MAX_FLAGS;

	memset(s_nf_buf, 0, sizeof(s_nf_buf));
	for (i = 0; i < count; i++)
	{
		s_nf_buf[i].position.x = s_flags[i].x;
		s_nf_buf[i].position.y = s_flags[i].y;
		s_nf_buf[i].position.z = s_flags[i].z;
		s_nf_buf[i].facing = s_flags[i].yaw;
		s_nf_buf[i].type = s_flags[i].type;
		s_nf_buf[i].team_index = s_flags[i].team_designator;
	}
	scen->netgame_flags.address = s_nf_buf;
	scen->netgame_flags.count = count;
}

static void parse_and_apply_sidecar(char const *text_buf)
{
	char const *cur = text_buf;
	char line[512];
	int in_forge_state = 0;
	int last_obj_idx = -1;
	int last_def_idx = -1;

	while (*cur)
	{
		size_t len = 0;
		while (cur[len] && cur[len] != '\r' && cur[len] != '\n')
			len++;
		if (len >= sizeof(line))
			len = sizeof(line) - 1;
		memcpy(line, cur, len);
		line[len] = '\0';
		cur += len;
		while (*cur == '\r' || *cur == '\n')
			cur++;

		if (strcmp(line, "[ForgeState]") == 0)
		{
			in_forge_state = 1;
		}
		else if (!in_forge_state)
		{
			continue;
		}
		else if (strncmp(line, "OBJECT|", 7) == 0)
		{
			unsigned int grp = 0;
			int src_cache = 0;
			int baked = 0;
			int team = 0;
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;
			float yaw = 0.0f;
			float pitch = 0.0f;
			float roll = 0.0f;
			char tpath[256];

			tpath[0] = '\0';
			(void)src_cache;
			(void)baked;
			if (sscanf(line + 7, "%u|%d|%d|%f|%f|%f|%f|%f|%f|%d|%255[^\r\n]",
					&grp, &src_cache, &baked, &x, &y, &z, &yaw, &pitch, &roll, &team, tpath) == 11 &&
				tpath[0] && s_instance_count < FORGE_SIDECAR_MAX_INSTANCES)
			{
				long def_idx = resolve_active_map_object_tag((uint32_t)grp, tpath);
				if (def_idx != NONE)
				{
					int idx = s_instance_count++;
					struct forge_sidecar_instance *inst = &s_instances[idx];
					memset(inst, 0, sizeof(*inst));
					inst->active = 1;
					inst->object_index = spawn_object_at(
						def_idx, x, y, z, yaw, pitch, roll, (int16_t)team);
					inst->definition_index = def_idx;
					inst->group_tag = (uint32_t)grp;
					inst->team_designator = (int16_t)team;
					inst->death_tick = -1;
					inst->x = x;
					inst->y = y;
					inst->z = z;
					inst->yaw = yaw;
					inst->pitch = pitch;
					inst->roll = roll;
					snprintf(inst->tag_path, sizeof(inst->tag_path), "%s", tpath);
					last_obj_idx = idx;
				}
			}
		}
		else if (strncmp(line, "OBJECT_GAMETYPE|", 16) == 0)
		{
			unsigned int gflags = 0;
			if (sscanf(line + 16, "%u", &gflags) == 1 && last_obj_idx >= 0)
				s_instances[last_obj_idx].gametype_flags = (uint16_t)gflags;
		}
		else if (strncmp(line, "OBJECT_PROPS|", 13) == 0)
		{
			int pidx = -1;
			int rsec = 0;
			unsigned int pmode = 0;
			unsigned int gid = 0;
			int parsed = 0;

			if (sscanf(line + 13, "%d|%u|%d|%u", &pidx, &pmode, &rsec, &gid) == 4)
				parsed = 1;
			else if (sscanf(line + 13, "%u|%d|%u", &pmode, &rsec, &gid) >= 3)
				parsed = 1;
			(void)pidx;
			(void)gid;
			if (parsed && last_obj_idx >= 0)
			{
				if (pmode >= FORGE_SIDECAR_PHYSICS_COUNT)
					pmode = 0;
				if (rsec < 0)
					rsec = 0;
				s_instances[last_obj_idx].physics_mode = (uint8_t)pmode;
				s_instances[last_obj_idx].respawn_timer_sec = (int16_t)rsec;
			}
		}
		else if (strncmp(line, "DEFAULT_EDIT|", 13) == 0)
		{
			unsigned int grp = 0;
			int deleted = 0;
			float ox = 0.0f;
			float oy = 0.0f;
			float oz = 0.0f;
			float nx = 0.0f;
			float ny = 0.0f;
			float nz = 0.0f;
			float nyaw = 0.0f;
			float npitch = 0.0f;
			float nroll = 0.0f;
			char tpath[256];

			tpath[0] = '\0';
			if (sscanf(line + 13, "%u|%d|%f|%f|%f|%f|%f|%f|%f|%f|%f|%255[^\r\n]",
					&grp, &deleted, &ox, &oy, &oz, &nx, &ny, &nz, &nyaw, &npitch, &nroll, tpath) == 12)
			{
				long found_oidx;

				update_scenario_equipment_edit(ox, oy, oz, nx, ny, nz, nyaw, deleted);
				if (deleted && s_deleted_eq_count < FORGE_MAX_DEFAULT_OBJECT_EDITS)
				{
					s_deleted_eq[s_deleted_eq_count].orig_x = ox;
					s_deleted_eq[s_deleted_eq_count].orig_y = oy;
					s_deleted_eq[s_deleted_eq_count].orig_z = oz;
					s_deleted_eq_count++;
				}
				found_oidx = find_default_object_near(ox, oy, oz, tpath);
				if (found_oidx != NONE)
				{
					if (deleted)
					{
						object_delete(found_oidx);
					}
					else if (s_instance_count < FORGE_SIDECAR_MAX_INSTANCES)
					{
						int idx = s_instance_count++;
						struct forge_sidecar_instance *inst = &s_instances[idx];
						struct object_datum *obj = (struct object_datum *)object_try_and_get(found_oidx);
						memset(inst, 0, sizeof(*inst));
						inst->active = 1;
						inst->is_default_scenario_object = 1;
						inst->object_index = found_oidx;
						inst->definition_index = obj ? obj->definition_index : NONE;
						inst->group_tag = (uint32_t)grp;
						inst->death_tick = -1;
						inst->x = nx;
						inst->y = ny;
						inst->z = nz;
						inst->yaw = nyaw;
						inst->pitch = npitch;
						inst->roll = nroll;
						snprintf(inst->tag_path, sizeof(inst->tag_path), "%s", tpath);
						move_existing_object(found_oidx, nx, ny, nz, nyaw, npitch, nroll, 0);
						last_def_idx = idx;
					}
				}
			}
		}
		else if (strncmp(line, "DEFAULT_GAMETYPE|", 17) == 0)
		{
			int didx = -1;
			unsigned int gflags = 0;
			int nread = sscanf(line + 17, "%d|%u", &didx, &gflags);
			if (nread == 1)
				gflags = (unsigned int)didx;
			if (nread >= 1 && last_def_idx >= 0)
				s_instances[last_def_idx].gametype_flags = (uint16_t)gflags;
		}
		else if (strncmp(line, "DEFAULT_PROPS|", 14) == 0)
		{
			int didx = -1;
			int rsec = 0;
			unsigned int pmode = 0;
			unsigned int gid = 0;

			(void)didx;
			(void)gid;
			if (sscanf(line + 14, "%d|%u|%d|%u", &didx, &pmode, &rsec, &gid) >= 4 && last_def_idx >= 0)
			{
				if (pmode >= FORGE_SIDECAR_PHYSICS_COUNT)
					pmode = 0;
				if (rsec < 0)
					rsec = 0;
				s_instances[last_def_idx].physics_mode = (uint8_t)pmode;
				s_instances[last_def_idx].respawn_timer_sec = (int16_t)rsec;
			}
		}
		else if (strncmp(line, "SPAWN|", 6) == 0)
		{
			float sx = 0.0f;
			float sy = 0.0f;
			float sz = 0.0f;
			float syaw = 0.0f;
			int steam = 0;
			unsigned int sgflags = 0;

			if (sscanf(line + 6, "%f|%f|%f|%f|%d|%u", &sx, &sy, &sz, &syaw, &steam, &sgflags) >= 5 &&
				s_spawn_count < FORGE_SIDECAR_MAX_SPAWNS)
			{
				s_replace_spawns = 1;
				s_spawns[s_spawn_count].x = sx;
				s_spawns[s_spawn_count].y = sy;
				s_spawns[s_spawn_count].z = sz;
				s_spawns[s_spawn_count].yaw = syaw;
				s_spawns[s_spawn_count].team_index = (int16_t)steam;
				s_spawns[s_spawn_count].gametype_flags = (uint16_t)sgflags;
				s_spawn_count++;
			}
		}
		else if (strncmp(line, "NETGAME_FLAG|", 13) == 0)
		{
			float fx = 0.0f;
			float fy = 0.0f;
			float fz = 0.0f;
			float fyaw = 0.0f;
			int ftype = 0;
			int fusage = 0;

			if (sscanf(line + 13, "%f|%f|%f|%f|%d|%d", &fx, &fy, &fz, &fyaw, &ftype, &fusage) >= 6 &&
				s_flag_count < FORGE_SIDECAR_MAX_FLAGS)
			{
				s_replace_flags = 1;
				s_flags[s_flag_count].x = fx;
				s_flags[s_flag_count].y = fy;
				s_flags[s_flag_count].z = fz;
				s_flags[s_flag_count].yaw = fyaw;
				s_flags[s_flag_count].type = (int16_t)ftype;
				s_flags[s_flag_count].team_designator = (int16_t)fusage;
				s_flag_count++;
			}
		}
	}

	apply_scenario_spawns();
	apply_scenario_netgame_flags();
}

static void try_load_active_sidecar(void)
{
	char const *leaf;
	char xbox_path[256];
	char host_path[512];
	char *buf = NULL;
	static char const *const folders[2] = {"custom_maps\\", "maps\\"};
	int f_i;

	if (s_pending_sidecar_leaf[0])
		leaf = s_pending_sidecar_leaf;
	else
		leaf = leaf_from_path(s_active_map_name);

	if (!leaf || !leaf[0] || _stricmp(leaf, "ui") == 0)
		return;

	for (f_i = 0; f_i < 2 && !buf; f_i++)
	{
		snprintf(xbox_path, sizeof(xbox_path), "%s%s.txt", folders[f_i], leaf);
		platform_translate_path(xbox_path, host_path, sizeof(host_path));
		buf = read_entire_text_file(host_path, NULL);
	}
	s_pending_sidecar_leaf[0] = '\0';
	if (!buf)
		return;

	if (strstr(buf, "[ForgeState]") != NULL)
		parse_and_apply_sidecar(buf);
	free(buf);
}

static void step_sidecar_instances(void)
{
	long now = game_time_get();
	int i;

	for (i = 0; i < s_instance_count; i++)
	{
		struct forge_sidecar_instance *inst = &s_instances[i];
		int allowed;

		if (!inst->active)
			continue;

		allowed = gametype_allows_flags(inst->gametype_flags);
		if (!allowed)
		{
			if (!inst->hidden_by_gametype && inst->object_index != NONE)
			{
				if (object_try_and_get(inst->object_index))
					object_delete(inst->object_index);
				inst->object_index = NONE;
			}
			inst->hidden_by_gametype = 1;
			continue;
		}
		if (inst->hidden_by_gametype)
		{
			inst->hidden_by_gametype = 0;
			if (inst->object_index == NONE && inst->definition_index != NONE)
			{
				inst->object_index = spawn_object_at(
					inst->definition_index,
					inst->x, inst->y, inst->z,
					inst->yaw, inst->pitch, inst->roll,
					inst->team_designator);
			}
		}

		if (inst->object_index != NONE)
		{
			struct object_datum *obj = (struct object_datum *)object_try_and_get(inst->object_index);
			if (!obj)
			{
				inst->object_index = NONE;
				inst->death_tick = now;
			}
			else if (inst->physics_mode == FORGE_SIDECAR_PHYSICS_FIXED ||
					 inst->physics_mode == FORGE_SIDECAR_PHYSICS_PHASED)
			{
				move_existing_object(
					inst->object_index,
					inst->x, inst->y, inst->z,
					inst->yaw, inst->pitch, inst->roll,
					inst->physics_mode);
			}
		}
		else if (inst->respawn_timer_sec > 0 && inst->death_tick >= 0)
		{
			long wait_ticks = (long)inst->respawn_timer_sec * TICKS_PER_SECOND;
			if (now - inst->death_tick >= wait_ticks && inst->definition_index != NONE)
			{
				inst->object_index = spawn_object_at(
					inst->definition_index,
					inst->x, inst->y, inst->z,
					inst->yaw, inst->pitch, inst->roll,
					inst->team_designator);
				inst->death_tick = -1;
			}
		}
	}
}

/* ---------- public API */

void forge_sidecar_menu_loaded(char const *map_name)
{
	char const *leaf;

	reset_sidecar_runtime();
	if (!map_name)
	{
		s_active_map_name[0] = '\0';
		return;
	}
	snprintf(s_active_map_name, sizeof(s_active_map_name), "%s", map_name);
	leaf = leaf_from_path(map_name);
	if (_stricmp(leaf, "ui") == 0)
		s_pending_sidecar_leaf[0] = '\0';
}

void forge_sidecar_menu_unloaded(void)
{
	reset_sidecar_runtime();
}

boolean forge_sidecar_choose_multiplayer_map(short level_index)
{
	short display_index = custom_edition_maps_level_display_index(level_index);
	char const *level_name = custom_edition_maps_level_name(display_index);

	s_pending_sidecar_leaf[0] = '\0';
	if (display_index >= 0x4000 && level_name && level_name[0])
	{
		char xbox_txt[512];
		char host_txt[512];
		char *buf;

		snprintf(xbox_txt, sizeof(xbox_txt), "%s.txt", level_name);
		platform_translate_path(xbox_txt, host_txt, sizeof(host_txt));
		buf = read_entire_text_file(host_txt, NULL);
		if (buf)
		{
			char base_map[256];
			if (sidecar_read_base_map(buf, base_map, sizeof(base_map)))
			{
				char const *const *names = NULL;
				short last_used = 0;
				short total = ui_widget_port_multiplayer_maps(&names, &last_used);
				short xbox_cnt = (short)(total - custom_edition_maps_count(FALSE));
				short base_idx = find_mp_level_index(names, total, xbox_cnt, level_index, base_map);

				free(buf);
				if (base_idx != NONE)
				{
					snprintf(s_pending_sidecar_leaf, sizeof(s_pending_sidecar_leaf), "%s", leaf_from_path(level_name));
					return ui_widget_port_multiplayer_map_choose(base_idx);
				}
			}
			else
			{
				free(buf);
			}
		}
	}
	return ui_widget_port_multiplayer_map_choose(level_index);
}

void forge_sidecar_tick(void)
{
	if (!global_scenario_try_and_get() || main_menu_is_active())
		return;
	if (game_connection() == _game_connection_network_client)
		return;

	if (!s_sidecar_applied)
	{
		if (game_time_get() < 1)
			return;
		s_sidecar_applied = 1;
		try_load_active_sidecar();
		return;
	}
	step_sidecar_instances();
}

int forge_sidecar_should_skip_netgame_equipment_spawn(long definition_index, void const *position)
{
	real_point3d const *pos = (real_point3d const *)position;
	int i;

	(void)definition_index;
	if (!pos)
		return 0;

	for (i = 0; i < s_deleted_eq_count; i++)
	{
		float dx = pos->x - s_deleted_eq[i].orig_x;
		float dy = pos->y - s_deleted_eq[i].orig_y;
		float dz = pos->z - s_deleted_eq[i].orig_z;
		if (dx * dx + dy * dy + dz * dz < 16.0f)
			return 1;
	}
	return 0;
}
