/*
FORGE_NETWORK.C

Multiplayer Forge synchronization over chat-channel and object-change hooks.
*/

#ifndef HALO_RELEASE
#define HALO_RELEASE
#endif
#include "cseries.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "game/game.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "items/items.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"
#include "networking/network_game_globals.h"
#include "network_distributed.h"
#include "network_chat.h"
#include "../src/p2p.h"

#include "forge_engine.h"

boolean tag_index_is_group(long tag_index, tag group_tag);

#undef malloc
#undef calloc
#undef free
#undef realloc

extern int config_boolean(const char *name);
extern boolean network_distributed_client_send_reliably(void *message, word size);
extern long network_distributed_client_reliable_backlog(void);
extern boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
extern long network_distributed_server_machine_reliable_backlog(long machine_index);
extern unsigned long system_milliseconds(void);
extern void orig_network_objects_handle_changes(void const *entries, short count);
extern void *cache_files_tag_instances(long *count);

struct forge_net_tag_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

struct forge_chat_line_payload
{
	uint8_t player_index;
	uint8_t flags;
	uint8_t pad[2];
	char text[64];
};

struct forge_chat_packet
{
	struct distributed_message_header header;
	struct forge_chat_line_payload line;
};

struct forge_distributed_object_change
{
	byte change;
	byte flags;
	byte owner_player_index;
	byte forced_shader_permutation_index;
	long object_index;
	long definition_index;
	short owner_team_index;
	short variant_number;
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	real_vector3d translational_velocity;
	real_vector3d angular_velocity;
	real_rgb_color change_colors[4];
	byte region_permutations[8];
};

#define FORGE_MAX_TRACKED_NET_OBJECTS 2048
#define FORGE_MAX_CLIENT_MACHINES     32

static unsigned long g_last_transform_send_ms = 0;
static unsigned long g_last_monitor_send_ms = 0;
static float g_last_monitor_sent_x = 0.0f;
static float g_last_monitor_sent_y = 0.0f;
static float g_last_monitor_sent_z = 0.0f;
static float g_last_monitor_sent_yaw = 0.0f;
static float g_last_monitor_sent_pitch = 0.0f;
static int g_client_requested_full_sync = 0;
static unsigned long g_client_join_ms = 0;

/* Maps absolute object index -> resolved local tag definition_index & Forge live_idx */
static long g_net_obj_def_index[FORGE_MAX_TRACKED_NET_OBJECTS];
static int32_t g_net_obj_live_index[FORGE_MAX_TRACKED_NET_OBJECTS];
static uint8_t g_net_obj_is_held[FORGE_MAX_TRACKED_NET_OBJECTS];
static uint8_t g_net_obj_has_deferred_create[FORGE_MAX_TRACKED_NET_OBJECTS];
static struct forge_distributed_object_change g_net_obj_deferred_create[FORGE_MAX_TRACKED_NET_OBJECTS];
static uint8_t g_suppressed_change_slots[256];

/* Host per-machine incremental full-sync cursor (-1 = idle, >= 0 = next live_idx to send) */
static int g_host_machine_known[FORGE_MAX_CLIENT_MACHINES];
static int g_host_machine_sync_cursor[FORGE_MAX_CLIENT_MACHINES];
static int g_host_machine_mon_synced[FORGE_MAX_CLIENT_MACHINES];

static uint32_t forge_catalog_entry_hash(uint32_t group_tag, char const *tag_path)
{
	uint32_t h = 2166136261u;
	char const *p;

	h = (h ^ ((group_tag >> 24) & 0xFF)) * 16777619u;
	h = (h ^ ((group_tag >> 16) & 0xFF)) * 16777619u;
	h = (h ^ ((group_tag >> 8) & 0xFF)) * 16777619u;
	h = (h ^ (group_tag & 0xFF)) * 16777619u;

	if (tag_path)
	{
		for (p = tag_path; *p; p++)
		{
			unsigned char c = (unsigned char)*p;
			if (c == '/')
				c = '\\';
			else if (c >= 'A' && c <= 'Z')
				c = (unsigned char)(c - 'A' + 'a');
			h = (h ^ c) * 16777619u;
		}
	}
	return h ? h : 1u;
}

static struct forge_catalog_entry const *forge_find_catalog_entry_by_hash(uint32_t hash)
{
	static struct forge_catalog_entry s_fallback_entry;
	long tag_count = 0;
	struct forge_net_tag_instance const *instances;
	int i;

	if (!g_forge.initialized)
		forge_runtime_init();
	else if (g_forge.catalog_count == 0)
		forge_runtime_reload_catalogs();

	for (i = 0; i < g_forge.catalog_count; i++)
	{
		if (forge_catalog_entry_hash(g_forge.catalog[i].group_tag, g_forge.catalog[i].tag_path) == hash)
			return &g_forge.catalog[i];
	}
	/* Fallback 1: check active live instances (e.g. copied base-map objects or virtual markers) */
	for (i = 0; i < g_forge.live_instance_count; i++)
	{
		struct forge_live_instance const *inst = &g_forge.live_instances[i];
		if (inst->active && inst->tag_path[0] &&
			forge_catalog_entry_hash(inst->group_tag, inst->tag_path) == hash)
		{
			memset(&s_fallback_entry, 0, sizeof(s_fallback_entry));
			s_fallback_entry.group_tag = inst->group_tag;
			snprintf(s_fallback_entry.tag_path, sizeof(s_fallback_entry.tag_path), "%s", inst->tag_path);
			s_fallback_entry.source_cache_id = inst->source_cache_id;
			return &s_fallback_entry;
		}
	}
	/* Fallback 1b: check all built-in virtual marker paths (spawns, teleporters, objectives) */
	{
		static struct { uint32_t group_tag; char const *tag_path; } const k_virtual_markers[] = {
			{ 'spwn', "spawn\\player_red" },
			{ 'spwn', "spawn\\player_blue" },
			{ 'spwn', "spawn\\player_ffa" },
			{ 'tele', "teleporter\\origin" },
			{ 'tele', "teleporter\\destination" },
			{ 'flag', "objective\\ctf_flag_red" },
			{ 'flag', "objective\\ctf_flag_blue" },
			{ 'flag', "objective\\ctf_vehicle_red" },
			{ 'flag', "objective\\ctf_vehicle_blue" },
			{ 'flag', "objective\\oddball_spawn" },
			{ 'flag', "objective\\koth_hill" },
			{ 'flag', "objective\\race_track" },
			{ 'flag', "objective\\race_checkpoint" },
			{ 'flag', "objective\\race_vehicle" },
			{ 'flag', "objective\\vegas_bank" },
			{ 'flag', "objective\\netgame_flag" }
		};
		for (i = 0; i < (int)(sizeof(k_virtual_markers) / sizeof(k_virtual_markers[0])); i++)
		{
			if (forge_catalog_entry_hash(k_virtual_markers[i].group_tag, k_virtual_markers[i].tag_path) == hash)
			{
				memset(&s_fallback_entry, 0, sizeof(s_fallback_entry));
				s_fallback_entry.group_tag = k_virtual_markers[i].group_tag;
				snprintf(s_fallback_entry.tag_path, sizeof(s_fallback_entry.tag_path), "%s",
					strcmp(k_virtual_markers[i].tag_path, "objective\\race_checkpoint") == 0 ?
						"objective\\race_track" : k_virtual_markers[i].tag_path);
				s_fallback_entry.source_cache_id = 0;
				return &s_fallback_entry;
			}
		}
	}
	/* Fallback 2: check all loaded map tag instances (e.g. map-local mp_warthog/ghost_mp when stock catalog deduplicated them) */
	instances = (struct forge_net_tag_instance const *)cache_files_tag_instances(&tag_count);
	if (instances && tag_count > 0)
	{
		long t;
		for (t = 0; t < tag_count; t++)
		{
			struct forge_net_tag_instance const *ti = &instances[t];
			uint32_t grp;
			if (ti->tag_index == NONE || !ti->name || !ti->name[0] || !ti->base_address)
				continue;
			grp = (uint32_t)ti->group_tag;
			if (grp != 'scen' && grp != 'vehi' && grp != 'weap' &&
				grp != 'eqip' && grp != 'bipd' && grp != 'mach' &&
				grp != 'lifi' && grp != 'actv')
			{
				continue;
			}
			if (forge_catalog_entry_hash(grp, ti->name) == hash)
			{
				memset(&s_fallback_entry, 0, sizeof(s_fallback_entry));
				s_fallback_entry.group_tag = grp;
				snprintf(s_fallback_entry.tag_path, sizeof(s_fallback_entry.tag_path), "%s", ti->name);
				s_fallback_entry.source_cache_id = 3;
				return &s_fallback_entry;
			}
		}
	}
	/* Fallback 3: check stock Xbox donor credits database (OpenCE Forge/credits.md) so late-joiners and peers
	   can resolve any stock Xbox campaign/MP tag (e.g. Pelican from b30.map) directly from their local maps/ folder! */
	{
		uint32_t cred_grp = 0;
		char cred_path[128];
		if (forge_find_credits_tag_by_hash(hash, &cred_grp, cred_path, sizeof(cred_path)) && cred_path[0])
		{
			memset(&s_fallback_entry, 0, sizeof(s_fallback_entry));
			s_fallback_entry.group_tag = cred_grp;
			snprintf(s_fallback_entry.tag_path, sizeof(s_fallback_entry.tag_path), "%s", cred_path);
			s_fallback_entry.source_cache_id = 1;
			if (g_forge.catalog_count < FORGE_MAX_CATALOG_ENTRIES)
			{
				struct forge_catalog_entry *ne = &g_forge.catalog[g_forge.catalog_count++];
				memset(ne, 0, sizeof(*ne));
				ne->group_tag = cred_grp;
				snprintf(ne->tag_path, sizeof(ne->tag_path), "%s", cred_path);
				ne->source_cache_id = 1;
				return ne;
			}
			return &s_fallback_entry;
		}
	}
	return NULL;
}

void forge_send_packet_to_host(char const *text)
{
	struct forge_chat_packet pkt;
	int p_abs;
	if (!text || !text[0])
		return;
	memset(&pkt, 0, sizeof(pkt));
	p_abs = forge_get_local_player_abs_index();
	pkt.line.player_index = (p_abs >= 0 && p_abs < 255) ? (uint8_t)p_abs : 0;
	pkt.line.flags = 0;
	snprintf(pkt.line.text, sizeof(pkt.line.text), "%s", text);
	distributed_send(&pkt, _distributed_message_chat_up, 1, sizeof(pkt), _distributed_to_host_reliably);
}

static void forge_send_packet_to_machine(long machine_index, char const *text)
{
	struct forge_chat_packet pkt;
	if (!text || !text[0] || machine_index < 0)
		return;
	memset(&pkt, 0, sizeof(pkt));
	pkt.line.player_index = 0;
	pkt.line.flags = 0;
	snprintf(pkt.line.text, sizeof(pkt.line.text), "%s", text);
	distributed_send_to_machine_reliably(machine_index, &pkt, _distributed_message_chat_down, 1, sizeof(pkt));
}

static void forge_broadcast_packet_to_clients(char const *text, long exclude_machine_index)
{
	long machines[FORGE_MAX_CLIENT_MACHINES];
	short count, i;

	if (!text || !text[0])
		return;
	count = distributed_client_machines(machines, FORGE_MAX_CLIENT_MACHINES);
	for (i = 0; i < count; i++)
	{
		if (machines[i] != exclude_machine_index)
			forge_send_packet_to_machine(machines[i], text);
	}
}

struct forge_pending_slot_props
{
	uint8_t valid;
	uint8_t physics_mode;
	uint8_t ai_order;
	uint8_t group_id;
	uint8_t light_color_preset;
	uint8_t light_intensity_preset;
	uint8_t light_radius_preset;
	int16_t team_designator;
	uint16_t gametype_flags;
	int16_t respawn_timer_sec;
};

static struct forge_pending_slot_props g_pending_slot_props[FORGE_MAX_LIVE_INSTANCES];

static int forge_instance_has_extended_props(struct forge_live_instance const *inst)
{
	if (!inst || !inst->active)
		return 0;
	if (inst->is_default_scenario_object && inst->edit_entry_index >= 0)
		return 1;
	if (inst->gametype_flags != 0 && inst->gametype_flags != 0x0FFFu)
		return 1;
	if (inst->physics_mode != 0 || inst->respawn_timer_sec > 0 ||
		inst->ai_order != 0 || inst->group_id != 0 ||
		inst->light_color_preset != 0 || inst->light_intensity_preset != 0 ||
		inst->light_radius_preset != 0)
		return 1;
	return 0;
}

static void forge_format_props_packet(char *out_buf, size_t out_sz, int32_t live_idx, struct forge_live_instance const *inst)
{
	snprintf(
		out_buf,
		out_sz,
		"!FP:%X:%d:%X:%u:%d:%u:%u:%u:%u:%u",
		(unsigned int)live_idx,
		(int)inst->team_designator,
		(unsigned int)inst->gametype_flags,
		(unsigned int)inst->physics_mode,
		(int)inst->respawn_timer_sec,
		(unsigned int)inst->ai_order,
		(unsigned int)inst->group_id,
		(unsigned int)inst->light_color_preset,
		(unsigned int)inst->light_intensity_preset,
		(unsigned int)inst->light_radius_preset);
}

static void forge_apply_props_to_live_slot(
	int32_t live_idx,
	int16_t team_designator,
	uint16_t gametype_flags,
	uint8_t physics_mode,
	int16_t respawn_timer_sec,
	uint8_t ai_order,
	uint8_t group_id,
	uint8_t light_color_preset,
	uint8_t light_intensity_preset,
	uint8_t light_radius_preset)
{
	struct forge_live_instance *inst;
	int16_t prev_team;

	if (live_idx < 0 || live_idx >= g_forge.live_instance_count)
		return;
	inst = &g_forge.live_instances[live_idx];
	if (!inst->active)
		return;

	prev_team = inst->team_designator;
	inst->team_designator = team_designator;
	inst->gametype_flags = gametype_flags;
	inst->physics_mode = physics_mode;
	inst->respawn_timer_sec = respawn_timer_sec;
	inst->ai_order = ai_order;
	inst->group_id = group_id;
	inst->light_color_preset = light_color_preset;
	inst->light_intensity_preset = light_intensity_preset;
	inst->light_radius_preset = light_radius_preset;

	if (inst->is_ai_unit && inst->team_designator != prev_team)
		forge_ai_update_unit_team(inst->actor_index, inst->object_index, inst->team_designator);

	forge_sync_instance_extended_props_to_edit_state(live_idx);
	forge_apply_instance_light_settings(live_idx);

	if (inst->is_ai_unit && game_connection() != _game_connection_network_client)
	{
		forge_ai_apply_order(inst->actor_index, inst->object_index, inst->ai_order,
			inst->x, inst->y, inst->z, game_time_get());
	}
	else if (inst->object_index != NONE && live_idx != g_forge.grabbed_live_index)
	{
		forge_3d_move_instance_ext(live_idx, inst->x, inst->y, inst->z,
			inst->yaw, inst->pitch, inst->roll, 0, 0);
	}
}

void forge_network_reset_session(void)
{
	int i;
	g_last_transform_send_ms = 0;
	g_last_monitor_send_ms = 0;
	g_last_monitor_sent_x = 0.0f;
	g_last_monitor_sent_y = 0.0f;
	g_last_monitor_sent_z = 0.0f;
	g_last_monitor_sent_yaw = 0.0f;
	g_last_monitor_sent_pitch = 0.0f;
	g_client_requested_full_sync = 0;
	g_client_join_ms = system_milliseconds();

	memset(g_forge.player_is_monitor, 0, sizeof(g_forge.player_is_monitor));
	memset(g_forge.player_has_monitor_pos, 0, sizeof(g_forge.player_has_monitor_pos));
	memset(g_forge.player_monitor_pos, 0, sizeof(g_forge.player_monitor_pos));
	memset(g_forge.player_monitor_yaw, 0, sizeof(g_forge.player_monitor_yaw));
	memset(g_forge.player_monitor_pitch, 0, sizeof(g_forge.player_monitor_pitch));
	memset(g_forge.player_has_render_pos, 0, sizeof(g_forge.player_has_render_pos));
	memset(g_forge.player_monitor_render_pos, 0, sizeof(g_forge.player_monitor_render_pos));
	memset(g_forge.player_monitor_render_yaw, 0, sizeof(g_forge.player_monitor_render_yaw));
	memset(g_forge.player_monitor_render_pitch, 0, sizeof(g_forge.player_monitor_render_pitch));

	for (i = 0; i < FORGE_MAX_TRACKED_NET_OBJECTS; i++)
	{
		g_net_obj_def_index[i] = NONE;
		g_net_obj_live_index[i] = -1;
		g_net_obj_is_held[i] = 0;
		/* Note: do not clear g_net_obj_has_deferred_create[i] here because on client join,
		   _distributed_message_object_changes can arrive before forge_runtime_init() runs on tick 0. */
	}
	for (i = 0; i < FORGE_MAX_CLIENT_MACHINES; i++)
	{
		g_host_machine_known[i] = 0;
		g_host_machine_sync_cursor[i] = -1;
		g_host_machine_mon_synced[i] = 0;
	}
	memset(g_pending_slot_props, 0, sizeof(g_pending_slot_props));
}

static int forge_object_is_network_shared(long obj_idx)
{
	struct object_datum *obj;
	if (obj_idx == NONE)
		return 0;
	obj = (struct object_datum *)object_try_and_get_and_verify_type(obj_idx, (unsigned long)-1);
	if (!obj)
		return 0;
	return TEST_FLAG(_object_mask_unit | _object_mask_item, obj->object.type) ? 1 : 0;
}

static void forge_format_host_spawn_packet(
	char *out_buf,
	size_t out_size,
	int32_t live_idx,
	uint32_t group_tag,
	char const *tag_path,
	long obj_idx,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int16_t team_designator)
{
	uint32_t hash = forge_catalog_entry_hash(group_tag, tag_path);
	unsigned int obj_enc = 0u;
	int yaw_cr = (int)(yaw * 100.0f);
	int pitch_cr = (int)(pitch * 100.0f);
	int roll_cr = (int)(roll * 100.0f);

	if (obj_idx != NONE && forge_object_is_network_shared(obj_idx))
	{
		struct object_datum *obj = (struct object_datum *)object_try_and_get_and_verify_type(obj_idx, (unsigned long)-1);
		if (obj)
		{
			unsigned int base_enc = (unsigned int)((DATUM_INDEX_TO_ABSOLUTE_INDEX(obj_idx) + 1) & 0xFFFu);
			unsigned int weap0_enc = 0u;
			if (TEST_FLAG(_object_mask_unit, obj->object.type))
			{
				struct unit_datum *unit = (struct unit_datum *)obj;
				if (unit->unit.weapon_object_indices[0] != NONE &&
					forge_object_is_network_shared(unit->unit.weapon_object_indices[0]))
				{
					weap0_enc = (unsigned int)((DATUM_INDEX_TO_ABSOLUTE_INDEX(unit->unit.weapon_object_indices[0]) + 1) & 0xFFFu);
				}
			}
			obj_enc = base_enc | (weap0_enc << 12);
		}
	}
	if (live_idx >= 0 && g_forge.grabbed_live_index == live_idx)
	{
		obj_enc |= (1u << 24);
	}

	snprintf(
		out_buf,
		out_size,
		"!FH:%X:%08X:%X:%.2f:%.2f:%.2f:%d:%d:%d:%d",
		(unsigned int)live_idx,
		(unsigned int)hash,
		obj_enc,
		x, y, z,
		yaw_cr, pitch_cr, roll_cr,
		(int)team_designator);
}

void forge_network_send_monitor_mode(int is_monitor, float x, float y, float z, float yaw, float pitch)
{
	char msg[64];
	int local_p_abs;
	int yaw_cr = (int)(yaw * 100.0f);
	int pitch_cr = (int)(pitch * 100.0f);

	if (!forge_is_multiplayer_session())
		return;

	local_p_abs = forge_get_local_player_abs_index();
	if (local_p_abs < 0 || local_p_abs >= 128)
		local_p_abs = 0;

	g_last_monitor_send_ms = system_milliseconds();
	g_last_monitor_sent_x = x;
	g_last_monitor_sent_y = y;
	g_last_monitor_sent_z = z;
	g_last_monitor_sent_yaw = yaw;
	g_last_monitor_sent_pitch = pitch;

	snprintf(
		msg,
		sizeof(msg),
		"!FT:%d:%d:%.2f:%.2f:%.2f:%d:%d",
		local_p_abs,
		is_monitor ? 1 : 0,
		x, y, z,
		yaw_cr, pitch_cr);

	if (game_connection() == _game_connection_network_client)
	{
		forge_send_packet_to_host(msg);
	}
	else if (game_connection() == _game_connection_network_server)
	{
		forge_broadcast_packet_to_clients(msg, -1);
	}
}

void forge_network_sync_tick(void)
{
	unsigned long now;

	if (!forge_is_multiplayer_session())
		return;

	now = system_milliseconds();

	/* Broadcast local Monitor flight position/aiming at 20 Hz when moving/aiming, or 2.5 Hz when stationary */
	if (g_forge.mode == _forge_mode_3d_monitor)
	{
		float dx = g_forge.cam_x - g_last_monitor_sent_x;
		float dy = g_forge.cam_y - g_last_monitor_sent_y;
		float dz = g_forge.cam_z - g_last_monitor_sent_z;
		float dyaw = g_forge.cam_yaw - g_last_monitor_sent_yaw;
		float dpitch = g_forge.cam_pitch - g_last_monitor_sent_pitch;
		int moved = (dx * dx + dy * dy + dz * dz > 0.0004f) ||
					(dyaw * dyaw + dpitch * dpitch > 0.0001f);
		unsigned long min_interval = moved ? 50u : 400u;
		if (now - g_last_monitor_send_ms >= min_interval)
		{
			forge_network_send_monitor_mode(1, g_forge.cam_x, g_forge.cam_y, g_forge.cam_z, g_forge.cam_yaw, g_forge.cam_pitch);
		}
	}

	if (game_connection() == _game_connection_network_client)
	{
		if (g_client_requested_full_sync == 0 && (now - g_client_join_ms >= 250))
		{
			g_client_requested_full_sync = 1;
			forge_send_packet_to_host("!FQ");
			if (g_forge.mode == _forge_mode_3d_monitor)
				forge_network_send_monitor_mode(1, g_forge.cam_x, g_forge.cam_y, g_forge.cam_z, g_forge.cam_yaw, g_forge.cam_pitch);
		}
		else if ((g_client_requested_full_sync == 1 && (now - g_client_join_ms >= 1500)) ||
				 (g_client_requested_full_sync == 2 && (now - g_client_join_ms >= 4000)))
		{
			int k, need_retry = (g_forge.live_instance_count == 0) ? 1 : 0;
			g_client_requested_full_sync++;
			for (k = 0; k < FORGE_MAX_TRACKED_NET_OBJECTS && !need_retry; k++)
			{
				if (g_net_obj_has_deferred_create[k] &&
					(g_net_obj_def_index[k] == NONE || g_net_obj_live_index[k] < 0))
				{
					need_retry = 1;
				}
			}
			if (need_retry)
				forge_send_packet_to_host("!FQ");
		}
	}
	else if (game_connection() == _game_connection_network_server)
	{
		long machines[FORGE_MAX_CLIENT_MACHINES];
		uint8_t active_mask[FORGE_MAX_CLIENT_MACHINES];
		short count, i;
		int m;

		memset(active_mask, 0, sizeof(active_mask));
		count = distributed_client_machines(machines, FORGE_MAX_CLIENT_MACHINES);
		for (i = 0; i < count; i++)
		{
			long midx = machines[i];
			if (midx >= 0 && midx < FORGE_MAX_CLIENT_MACHINES)
			{
				active_mask[midx] = 1;
				if (!g_host_machine_known[midx])
				{
					g_host_machine_known[midx] = 1;
					g_host_machine_sync_cursor[midx] = 0;
					g_host_machine_mon_synced[midx] = 0;
				}
			}
		}

		for (m = 0; m < FORGE_MAX_CLIENT_MACHINES; m++)
		{
			int packets_sent = 0;
			if (!active_mask[m])
			{
				g_host_machine_known[m] = 0;
				g_host_machine_sync_cursor[m] = -1;
				g_host_machine_mon_synced[m] = 0;
				continue;
			}

			/* Step 1: Sync all active Monitor players to newly joined client machine */
			if (!g_host_machine_mon_synced[m])
			{
				int p_abs;
				for (p_abs = 0; p_abs < 128; p_abs++)
				{
					if (g_forge.player_is_monitor[p_abs])
					{
						char msg[64];
						snprintf(
							msg,
							sizeof(msg),
							"!FT:%d:1:%.2f:%.2f:%.2f:%d:%d",
							p_abs,
							g_forge.player_monitor_pos[p_abs][0],
							g_forge.player_monitor_pos[p_abs][1],
							g_forge.player_monitor_pos[p_abs][2],
							(int)(g_forge.player_monitor_yaw[p_abs] * 100.0f),
							(int)(g_forge.player_monitor_pitch[p_abs] * 100.0f));
						forge_send_packet_to_machine(m, msg);
						packets_sent++;
					}
				}
				g_host_machine_mon_synced[m] = 1;
			}

			/* Step 2: Drip-feed up to 6 live_instances per tick to newly joined client machine */
			if (g_forge.live_instance_count == 0)
				forge_init_map_scenario_entities();

			while (g_host_machine_sync_cursor[m] >= 0 &&
				   g_host_machine_sync_cursor[m] < g_forge.live_instance_count &&
				   packets_sent < 6)
			{
				int32_t idx = g_host_machine_sync_cursor[m]++;
				struct forge_live_instance const *inst = &g_forge.live_instances[idx];
				char msg[64];

				if (inst->is_default_scenario_object &&
					(!inst->active || inst->edit_entry_index >= 0) &&
					inst->group_tag != 'spwn' && inst->group_tag != 'tele' && inst->group_tag != 'flag')
				{
					uint32_t d_hash = forge_catalog_entry_hash(inst->group_tag, inst->tag_path);
					snprintf(
						msg,
						sizeof(msg),
						"!FXA:%X:%08X:%.2f:%.2f:%.2f",
						(unsigned int)idx,
						(unsigned int)d_hash,
						inst->orig_x,
						inst->orig_y,
						inst->orig_z);
					forge_send_packet_to_machine(m, msg);
					packets_sent++;
				}

				if (!inst->active)
				{
					if (inst->is_default_scenario_object)
					{
						snprintf(msg, sizeof(msg), "!FD:%d:-1", (int)idx);
						forge_send_packet_to_machine(m, msg);
						packets_sent++;
					}
				}
				else if (!inst->is_default_scenario_object)
				{
					forge_format_host_spawn_packet(
						msg,
						sizeof(msg),
						idx,
						inst->group_tag,
						inst->tag_path,
						inst->object_index,
						inst->x, inst->y, inst->z,
						inst->yaw, inst->pitch, inst->roll,
						inst->team_designator);
					forge_send_packet_to_machine(m, msg);
					packets_sent++;
				}
				else if (inst->edit_entry_index >= 0)
				{
					int obj_abs = (inst->object_index != NONE && forge_object_is_network_shared(inst->object_index))
									  ? (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(inst->object_index)
									  : -1;
					int is_held = (g_forge.grabbed_live_index == idx) ? 1 : 0;
					snprintf(
						msg,
						sizeof(msg),
						"!FM:%d:%d:%.2f:%.2f:%.2f:%d:%d:%d:%d",
						(int)idx,
						obj_abs,
						inst->x, inst->y, inst->z,
						(int)(inst->yaw * 100.0f),
						(int)(inst->pitch * 100.0f),
						(int)(inst->roll * 100.0f),
						is_held);
					forge_send_packet_to_machine(m, msg);
					packets_sent++;
				}

				if (inst->active && forge_instance_has_extended_props(inst))
				{
					forge_format_props_packet(msg, sizeof(msg), idx, inst);
					forge_send_packet_to_machine(m, msg);
					packets_sent++;
				}
			}

			if (g_host_machine_sync_cursor[m] >= g_forge.live_instance_count)
				g_host_machine_sync_cursor[m] = -1;
		}
	}

	(void)now;
	forge_server_autosave_tick();
}

void forge_network_register_host_object(
	int32_t live_idx,
	long object_index,
	long definition_index,
	uint32_t group_tag,
	char const *tag_path)
{
	int m;
	(void)group_tag;
	(void)tag_path;
	if (object_index != NONE && forge_object_is_network_shared(object_index))
	{
		int obj_abs = (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);
		if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
		{
			if (definition_index != NONE)
				g_net_obj_def_index[obj_abs] = definition_index;
			if (live_idx >= 0)
				g_net_obj_live_index[obj_abs] = live_idx;
		}
	}
	if (game_connection() == _game_connection_network_server)
	{
		for (m = 0; m < FORGE_MAX_CLIENT_MACHINES; m++)
		{
			if (g_host_machine_known[m])
				g_host_machine_sync_cursor[m] = 0;
		}
	}
}

void forge_network_send_spawn_ext(
	int32_t live_idx,
	uint32_t group_tag,
	char const *tag_path,
	uint8_t source_cache_id,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int16_t team_designator,
	long host_obj_idx,
	long host_def_idx)
{
	char msg[64];
	uint32_t hash;

	(void)source_cache_id;
	if (!forge_is_multiplayer_session() || !tag_path || !tag_path[0])
		return;

	hash = forge_catalog_entry_hash(group_tag, tag_path);

	if (host_obj_idx != NONE && forge_object_is_network_shared(host_obj_idx))
	{
		int obj_abs = (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(host_obj_idx);
		if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
		{
			if (host_def_idx != NONE)
				g_net_obj_def_index[obj_abs] = host_def_idx;
			if (live_idx >= 0)
				g_net_obj_live_index[obj_abs] = live_idx;
		}
	}

	if (game_connection() == _game_connection_network_client)
	{
		snprintf(
			msg,
			sizeof(msg),
			"!FS:%08X:%.2f:%.2f:%.2f:%d:%d:%d:%d",
			(unsigned int)hash,
			x, y, z,
			(int)(yaw * 100.0f),
			(int)(pitch * 100.0f),
			(int)(roll * 100.0f),
			(int)team_designator);
		forge_send_packet_to_host(msg);
	}
	else if (game_connection() == _game_connection_network_server)
	{
		forge_format_host_spawn_packet(
			msg,
			sizeof(msg),
			live_idx,
			group_tag,
			tag_path,
			host_obj_idx,
			x, y, z,
			yaw, pitch, roll,
			team_designator);
		forge_broadcast_packet_to_clients(msg, -1);
	}
}

void forge_network_send_transform_ext(
	int32_t live_idx,
	long obj_idx,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int is_held,
	int immediate)
{
	unsigned long now;
	char msg[64];
	int obj_abs;

	if (!forge_is_multiplayer_session())
		return;

	now = system_milliseconds();
	if (!immediate && (now - g_last_transform_send_ms < 50))
		return;
	g_last_transform_send_ms = now;

	obj_abs = (obj_idx != NONE && forge_object_is_network_shared(obj_idx))
				  ? (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(obj_idx)
				  : -1;
	if (obj_abs < 0 && obj_idx == NONE && live_idx >= 0 && live_idx < g_forge.live_instance_count)
	{
		uint32_t grp = g_forge.live_instances[live_idx].group_tag;
		if (grp == 'vehi' || grp == 'weap' || grp == 'eqip' || grp == 'bipd' || grp == 'actv')
		{
			int k;
			for (k = 0; k < FORGE_MAX_TRACKED_NET_OBJECTS; k++)
			{
				if (g_net_obj_live_index[k] == live_idx)
				{
					obj_abs = k;
					break;
				}
			}
		}
	}
	if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
	{
		g_net_obj_is_held[obj_abs] = is_held ? 1 : 0;
	}

	/* If this client instance has not yet been linked to a Host slot (e.g. while the Host
	   is downloading a custom/MCC tag from this client), send a negative ordinal live_idx
	   (-1 - unlinked_ord) and obj_abs = -1 so the Host updates the matching entry in
	   g_host_pending_client_spawns without colliding when multiple custom tags are queued. */
	if (game_connection() == _game_connection_network_client &&
		live_idx >= 0 && live_idx < g_forge.live_instance_count &&
		(g_forge.live_instances[live_idx].pad_inst & 0x80u) != 0)
	{
		int32_t u_i;
		int unlinked_ord = 0;
		for (u_i = 0; u_i < live_idx; u_i++)
		{
			if (g_forge.live_instances[u_i].active &&
				!g_forge.live_instances[u_i].is_default_scenario_object &&
				(g_forge.live_instances[u_i].pad_inst & 0x80u) != 0)
			{
				unlinked_ord++;
			}
		}
		live_idx = -1 - unlinked_ord;
		obj_abs = -1;
	}

	if (immediate && obj_abs < 0 && live_idx >= 0 && live_idx < g_forge.live_instance_count)
	{
		struct forge_live_instance const *inst = &g_forge.live_instances[live_idx];
		if (inst->is_default_scenario_object &&
			inst->group_tag != 'spwn' && inst->group_tag != 'tele' && inst->group_tag != 'flag')
		{
			char adopt_msg[64];
			uint32_t d_hash = forge_catalog_entry_hash(inst->group_tag, inst->tag_path);
			snprintf(
				adopt_msg,
				sizeof(adopt_msg),
				"!FXA:%X:%08X:%.2f:%.2f:%.2f",
				(unsigned int)live_idx,
				(unsigned int)d_hash,
				inst->orig_x,
				inst->orig_y,
				inst->orig_z);
			if (game_connection() == _game_connection_network_client)
				forge_send_packet_to_host(adopt_msg);
			else if (game_connection() == _game_connection_network_server)
				forge_broadcast_packet_to_clients(adopt_msg, -1);
		}
	}

	snprintf(
		msg,
		sizeof(msg),
		"!FM:%d:%d:%.2f:%.2f:%.2f:%d:%d:%d:%d",
		(int)live_idx,
		obj_abs,
		x, y, z,
		(int)(yaw * 100.0f),
		(int)(pitch * 100.0f),
		(int)(roll * 100.0f),
		is_held ? 1 : 0);

	if (game_connection() == _game_connection_network_client)
	{
		forge_send_packet_to_host(msg);
	}
	else if (game_connection() == _game_connection_network_server)
	{
		forge_broadcast_packet_to_clients(msg, -1);
	}
}

void forge_network_send_object_props(int32_t live_idx)
{
	char msg[64];
	struct forge_live_instance const *inst;

	if (!forge_is_multiplayer_session())
		return;
	if (live_idx < 0 || live_idx >= g_forge.live_instance_count)
		return;
	inst = &g_forge.live_instances[live_idx];
	if (!inst->active)
		return;

	if (inst->is_default_scenario_object &&
		inst->group_tag != 'spwn' && inst->group_tag != 'tele' && inst->group_tag != 'flag')
	{
		char adopt_msg[64];
		uint32_t d_hash = forge_catalog_entry_hash(inst->group_tag, inst->tag_path);
		snprintf(
			adopt_msg,
			sizeof(adopt_msg),
			"!FXA:%X:%08X:%.2f:%.2f:%.2f",
			(unsigned int)live_idx,
			(unsigned int)d_hash,
			inst->orig_x,
			inst->orig_y,
			inst->orig_z);
		if (game_connection() == _game_connection_network_client)
			forge_send_packet_to_host(adopt_msg);
		else if (game_connection() == _game_connection_network_server)
			forge_broadcast_packet_to_clients(adopt_msg, -1);
	}

	forge_format_props_packet(msg, sizeof(msg), live_idx, inst);
	if (game_connection() == _game_connection_network_client)
		forge_send_packet_to_host(msg);
	else if (game_connection() == _game_connection_network_server)
		forge_broadcast_packet_to_clients(msg, -1);
}

void forge_network_send_delete_ext(int32_t live_idx, long obj_idx)
{
	char msg[64];
	int obj_abs;
	int k;

	if (!forge_is_multiplayer_session())
		return;

	obj_abs = (obj_idx != NONE && forge_object_is_network_shared(obj_idx))
				  ? (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(obj_idx)
				  : -1;
	for (k = 0; k < FORGE_MAX_TRACKED_NET_OBJECTS; k++)
	{
		if (g_net_obj_live_index[k] == live_idx)
		{
			if (obj_abs < 0)
				obj_abs = k;
			g_net_obj_live_index[k] = -1;
			g_net_obj_is_held[k] = 0;
		}
	}
	if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
	{
		g_net_obj_live_index[obj_abs] = -1;
		g_net_obj_is_held[obj_abs] = 0;
	}
	if (obj_abs < 0 && live_idx >= 0 && live_idx < g_forge.live_instance_count)
	{
		struct forge_live_instance const *inst = &g_forge.live_instances[live_idx];
		if (inst->is_default_scenario_object &&
			inst->group_tag != 'spwn' && inst->group_tag != 'tele' && inst->group_tag != 'flag')
		{
			char adopt_msg[64];
			uint32_t d_hash = forge_catalog_entry_hash(inst->group_tag, inst->tag_path);
			snprintf(
				adopt_msg,
				sizeof(adopt_msg),
				"!FXA:%X:%08X:%.2f:%.2f:%.2f",
				(unsigned int)live_idx,
				(unsigned int)d_hash,
				inst->orig_x,
				inst->orig_y,
				inst->orig_z);
			if (game_connection() == _game_connection_network_client)
				forge_send_packet_to_host(adopt_msg);
			else if (game_connection() == _game_connection_network_server)
				forge_broadcast_packet_to_clients(adopt_msg, -1);
		}
	}
	snprintf(msg, sizeof(msg), "!FD:%d:%d", (int)live_idx, obj_abs);

	if (game_connection() == _game_connection_network_client)
	{
		forge_send_packet_to_host(msg);
	}
	else if (game_connection() == _game_connection_network_server)
	{
		forge_broadcast_packet_to_clients(msg, -1);
	}
}

static long forge_find_live_object_by_abs_index(int obj_abs)
{
	if (obj_abs < 0 || !object_header_data)
		return NONE;
	if (obj_abs < object_header_data->maximum_count)
	{
		struct datum_header const *hdr = (struct datum_header const *)(
			(byte const *)object_header_data->data + obj_abs * object_header_data->size);
		if (hdr->identifier != 0)
		{
			long cand = ((long)(unsigned short)hdr->identifier << 16) | (obj_abs & 0xFFFF);
			if (object_try_and_get_and_verify_type(cand, (unsigned long)-1) != NULL)
				return cand;
		}
	}
	return NONE;
}

static void forge_net_euler_to_vectors(float yaw, float pitch, float roll, real_vector3d *out_fwd, real_vector3d *out_up)
{
	float cy = (float)cos((double)yaw);
	float sy = (float)sin((double)yaw);
	float cp = (float)cos((double)pitch);
	float sp = (float)sin((double)pitch);
	float cr = (float)cos((double)roll);
	float sr = (float)sin((double)roll);
	out_fwd->i = cy * cp;
	out_fwd->j = sy * cp;
	out_fwd->k = sp;
	out_up->i = -cy * sp * cr + sy * sr;
	out_up->j = -sy * sp * cr - cy * sr;
	out_up->k = cp * cr;
}

static int32_t forge_resolve_target_live_idx(int live_idx, int obj_abs)
{
	if (g_forge.live_instance_count == 0)
		forge_init_map_scenario_entities();

	if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
	{
		int32_t mapped_idx = g_net_obj_live_index[obj_abs];
		if (mapped_idx >= 0 && mapped_idx < g_forge.live_instance_count && g_forge.live_instances[mapped_idx].active)
			return mapped_idx;
	}
	if (live_idx >= 0 && live_idx < g_forge.live_instance_count && g_forge.live_instances[live_idx].active)
		return live_idx;
	if (obj_abs >= 0)
	{
		long obj_idx = forge_find_live_object_by_abs_index(obj_abs);
		if (obj_idx != NONE && forge_object_is_network_shared(obj_idx))
		{
			int32_t adopted_idx = forge_3d_adopt_or_find_object_instance(obj_idx);
			if (adopted_idx >= 0)
			{
				if (live_idx >= 0 && live_idx < FORGE_MAX_LIVE_INSTANCES && adopted_idx != live_idx &&
					(live_idx >= g_forge.live_instance_count || !g_forge.live_instances[live_idx].active))
				{
					if (live_idx >= g_forge.live_instance_count)
					{
						memset(
							&g_forge.live_instances[g_forge.live_instance_count],
							0,
							(size_t)(live_idx - g_forge.live_instance_count + 1) * sizeof(struct forge_live_instance));
						g_forge.live_instance_count = live_idx + 1;
					}
					g_forge.live_instances[live_idx] = g_forge.live_instances[adopted_idx];
					memset(&g_forge.live_instances[adopted_idx], 0, sizeof(struct forge_live_instance));
					g_forge.live_instances[adopted_idx].object_index = NONE;
					g_forge.live_instances[adopted_idx].actor_index = NONE;
					g_forge.live_instances[adopted_idx].edit_entry_index = -1;
					adopted_idx = live_idx;
				}
				if (obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
					g_net_obj_live_index[obj_abs] = adopted_idx;
				return adopted_idx;
			}
		}
	}
	if (live_idx >= 0 && live_idx < g_forge.live_instance_count)
		return live_idx;
	return -1;
}

static void forge_client_configure_linked_object(int32_t live_idx, long obj_idx, int is_held)
{
	struct object_datum *obj;

	if (obj_idx == NONE)
		return;

	object_set_automatic_deactivation(obj_idx, FALSE);
	object_activate(obj_idx);

	obj = object_get(obj_idx);
	if (obj)
	{
		SET_FLAG(obj->object.flags, _object_no_collisions_bit, is_held ? TRUE : FALSE);
		if (TEST_FLAG(_object_mask_unit, obj->object.type))
		{
			if (is_held)
				SET_FLAG(obj->object.flags, _object_at_rest_bit, TRUE);
			else
				SET_FLAG(obj->object.flags, _object_at_rest_bit, FALSE);
			unit_set_enterable_by_player(obj_idx, TRUE);
			unit_set_controllable(obj_idx, TRUE);
		}
		else if (!is_held)
		{
			SET_FLAG(obj->object.flags, _object_at_rest_bit, FALSE);
		}
	}

	if (live_idx >= 0 && live_idx < g_forge.live_instance_count && g_forge.live_instances[live_idx].active)
	{
		long prev_obj = g_forge.live_instances[live_idx].object_index;
		if (prev_obj != NONE && prev_obj != obj_idx)
		{
			if (object_try_and_get_and_verify_type(prev_obj, (unsigned long)-1))
				object_delete(prev_obj);
		}
		g_forge.live_instances[live_idx].object_index = obj_idx;
		if (g_forge.grabbed_live_index == live_idx)
			g_forge.grabbed_object_index = obj_idx;
	}
}

static void forge_client_handle_host_spawn(
	int host_live_idx,
	uint32_t hash,
	int obj_abs,
	int weap0_abs,
	int host_is_held,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int16_t team_designator)
{
	struct forge_catalog_entry const *ce = forge_find_catalog_entry_by_hash(hash);
	uint32_t actual_grp;
	long def_idx = NONE;
	long weap0_def_idx = NONE;
	int32_t match_idx = -1;
	int matched_client_unlinked = 0;
	int matched_existing = 0;
	int is_net_obj;
	int i;

	/* Ensure default scenario entities (spawns, flags, teleporters) are initialized at indices 0..N-1
	   before inserting any host-replicated objects, even if !FH: arrives on Frame 0 before forge_3d_update()! */
	if (g_forge.live_instance_count == 0)
		forge_init_map_scenario_entities();

	if (!ce)
		return;

	actual_grp = ce->group_tag;
	if (ce->group_tag != 'spwn' && ce->group_tag != 'tele' && ce->group_tag != 'flag')
	{
		/* Fault in external Xbox campaign tags or resolve active map tags before object creation */
		def_idx = forge_resolve_engine_tag(ce->group_tag, ce->tag_path, &actual_grp);
		if (def_idx == NONE && ce->group_tag != 'actv')
			return;
	}

	/* Only units and items share lower-1024 obj_abs indices across the network (network_objects.c).
	   Non-networked objects (scen, mach, lifi) are spawned locally on each machine at independent indices. */
	is_net_obj = (actual_grp == 'vehi' || actual_grp == 'weap' || actual_grp == 'eqip' ||
				  actual_grp == 'bipd' || ce->group_tag == 'actv');
	if (!is_net_obj)
	{
		obj_abs = -1;
		weap0_abs = -1;
	}

	if (is_net_obj && obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
	{
		if (def_idx != NONE)
			g_net_obj_def_index[obj_abs] = def_idx;
		g_net_obj_is_held[obj_abs] = host_is_held ? 1 : 0;
	}

	if (is_net_obj && weap0_abs >= 0 && weap0_abs < FORGE_MAX_TRACKED_NET_OBJECTS && def_idx != NONE && actual_grp == 'vehi')
	{
		struct unit_definition const *udef = (struct unit_definition const *)tag_get('unit', def_idx);
		if (udef && udef->unit.initial_weapons.count > 0 && udef->unit.initial_weapons.address)
		{
			struct tag_reference const *iw_ref = (struct tag_reference const *)udef->unit.initial_weapons.address;
			if (iw_ref->index != NONE)
			{
				weap0_def_idx = iw_ref->index;
				g_net_obj_def_index[weap0_abs] = weap0_def_idx;
			}
		}
	}

	/* 1. Check if host_live_idx already points to a locally-created unlinked instance of this tag */
	if (host_live_idx >= 0 && host_live_idx < g_forge.live_instance_count)
	{
		struct forge_live_instance const *cand = &g_forge.live_instances[host_live_idx];
		if (cand->active && !cand->is_default_scenario_object &&
			(cand->pad_inst & 0x80u) != 0 &&
			cand->group_tag == ce->group_tag && _stricmp(cand->tag_path, ce->tag_path) == 0)
		{
			match_idx = host_live_idx;
			matched_client_unlinked = 1;
		}
	}

	/* 2. Scan locally-created unlinked Client instances (pad_inst & 0x80) of this tag in FIFO order,
	   preferring the one closest to the confirmed spawn coordinates so multiple queued spawns never collide. */
	if (match_idx < 0)
	{
		float best_dist_sq = 1.0e30f;
		for (i = 0; i < g_forge.live_instance_count; i++)
		{
			struct forge_live_instance const *cand = &g_forge.live_instances[i];
			float dx, dy, dz, d2;
			if (!cand->active || cand->is_default_scenario_object ||
				(cand->pad_inst & 0x80u) == 0)
				continue;
			if (cand->group_tag == ce->group_tag && _stricmp(cand->tag_path, ce->tag_path) == 0)
			{
				dx = cand->x - x;
				dy = cand->y - y;
				dz = cand->z - z;
				d2 = dx * dx + dy * dy + dz * dz;
				if (match_idx < 0 || d2 + 0.01f < best_dist_sq)
				{
					best_dist_sq = d2;
					match_idx = i;
					matched_client_unlinked = 1;
				}
			}
		}
	}

	if (matched_client_unlinked && match_idx >= 0 && match_idx < g_forge.live_instance_count)
	{
		g_forge.live_instances[match_idx].pad_inst &= (uint8_t)~0x80u;
		if (host_live_idx >= 0 && host_live_idx < FORGE_MAX_LIVE_INSTANCES && match_idx != host_live_idx)
		{
			if (host_live_idx >= g_forge.live_instance_count)
			{
				memset(
					&g_forge.live_instances[g_forge.live_instance_count],
					0,
					(size_t)(host_live_idx - g_forge.live_instance_count + 1) * sizeof(struct forge_live_instance));
				g_forge.live_instance_count = host_live_idx + 1;
			}
			if (!g_forge.live_instances[host_live_idx].active)
			{
				g_forge.live_instances[host_live_idx] = g_forge.live_instances[match_idx];
				memset(&g_forge.live_instances[match_idx], 0, sizeof(struct forge_live_instance));
				g_forge.live_instances[match_idx].object_index = NONE;
				g_forge.live_instances[match_idx].actor_index = NONE;
				g_forge.live_instances[match_idx].edit_entry_index = -1;
				if (g_forge.grabbed_live_index == match_idx)
					g_forge.grabbed_live_index = host_live_idx;
				if (g_forge.hovered_live_index == match_idx)
					g_forge.hovered_live_index = host_live_idx;
				match_idx = host_live_idx;
			}
			else
			{
				struct forge_live_instance tmp_inst = g_forge.live_instances[host_live_idx];
				int k_map;
				g_forge.live_instances[host_live_idx] = g_forge.live_instances[match_idx];
				g_forge.live_instances[match_idx] = tmp_inst;
				for (k_map = 0; k_map < FORGE_MAX_TRACKED_NET_OBJECTS; k_map++)
				{
					if (g_net_obj_live_index[k_map] == host_live_idx)
						g_net_obj_live_index[k_map] = match_idx;
					else if (g_net_obj_live_index[k_map] == match_idx)
						g_net_obj_live_index[k_map] = host_live_idx;
				}
				if (g_forge.grabbed_live_index == match_idx)
					g_forge.grabbed_live_index = host_live_idx;
				else if (g_forge.grabbed_live_index == host_live_idx)
					g_forge.grabbed_live_index = match_idx;
				if (g_forge.hovered_live_index == match_idx)
					g_forge.hovered_live_index = host_live_idx;
				else if (g_forge.hovered_live_index == host_live_idx)
					g_forge.hovered_live_index = match_idx;
				match_idx = host_live_idx;
			}
		}
	}

	/* 4. Check if already linked to obj_abs, at host_live_idx, or at the exact position */
	if (match_idx < 0)
	{
		long existing_obj = is_net_obj ? forge_find_live_object_by_abs_index(obj_abs) : NONE;
		if (is_net_obj && obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
		{
			int32_t mapped = g_net_obj_live_index[obj_abs];
			if (mapped >= 0 && mapped < g_forge.live_instance_count)
			{
				struct forge_live_instance const *cand = &g_forge.live_instances[mapped];
				if (cand->active && !cand->is_default_scenario_object &&
					cand->group_tag == ce->group_tag && _stricmp(cand->tag_path, ce->tag_path) == 0)
				{
					match_idx = mapped;
					matched_existing = 1;
				}
			}
		}
		if (match_idx < 0 && existing_obj != NONE)
		{
			for (i = g_forge.live_instance_count - 1; i >= 0; i--)
			{
				struct forge_live_instance const *cand = &g_forge.live_instances[i];
				if (cand->active && !cand->is_default_scenario_object && cand->object_index == existing_obj)
				{
					match_idx = i;
					matched_existing = 1;
					break;
				}
			}
		}
		if (match_idx < 0 && host_live_idx >= 0 && host_live_idx < g_forge.live_instance_count)
		{
			struct forge_live_instance const *cand = &g_forge.live_instances[host_live_idx];
			if (cand->active && !cand->is_default_scenario_object &&
				cand->group_tag == ce->group_tag && _stricmp(cand->tag_path, ce->tag_path) == 0)
			{
				match_idx = host_live_idx;
				matched_existing = 1;
			}
		}
		if (match_idx < 0)
		{
			for (i = g_forge.live_instance_count - 1; i >= 0; i--)
			{
				struct forge_live_instance const *cand = &g_forge.live_instances[i];
				float dx, dy, dz;
				if (!cand->active || cand->is_default_scenario_object)
					continue;
				if (is_net_obj && cand->object_index != NONE)
					continue;
				if (cand->group_tag != ce->group_tag || _stricmp(cand->tag_path, ce->tag_path) != 0)
					continue;
				dx = cand->x - x;
				dy = cand->y - y;
				dz = cand->z - z;
				if (dx * dx + dy * dy + dz * dz < 0.04f)
				{
					match_idx = i;
					matched_existing = 1;
					break;
				}
			}
		}
	}

	if (match_idx < 0)
	{
		match_idx = forge_3d_spawn_object_at_slot(
			host_live_idx,
			ce->group_tag,
			ce->tag_path,
			ce->source_cache_id,
			x, y, z,
			yaw, pitch, roll,
			team_designator,
			0);
	}
	else if (matched_existing && match_idx != g_forge.grabbed_live_index)
	{
		forge_3d_move_instance_ext(
			match_idx,
			x, y, z,
			yaw, pitch, roll,
			host_is_held,
			0);
	}

	if (match_idx >= 0 && host_live_idx >= 0 && host_live_idx < FORGE_MAX_LIVE_INSTANCES &&
		g_pending_slot_props[host_live_idx].valid)
	{
		struct forge_pending_slot_props const *pp = &g_pending_slot_props[host_live_idx];
		forge_apply_props_to_live_slot(
			match_idx,
			pp->team_designator,
			pp->gametype_flags,
			pp->physics_mode,
			pp->respawn_timer_sec,
			pp->ai_order,
			pp->group_id,
			pp->light_color_preset,
			pp->light_intensity_preset,
			pp->light_radius_preset);
		g_pending_slot_props[host_live_idx].valid = 0;
	}

	if (is_net_obj && obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
	{
		long existing_obj;
		int effective_held = host_is_held;

		if (match_idx >= 0)
		{
			int k;
			for (k = 0; k < FORGE_MAX_TRACKED_NET_OBJECTS; k++)
			{
				if (g_net_obj_live_index[k] == match_idx)
					g_net_obj_live_index[k] = -1;
			}
			g_net_obj_live_index[obj_abs] = match_idx;
			if (matched_client_unlinked)
			{
				effective_held = (g_forge.grabbed_live_index == match_idx) ? 1 : 0;
				g_net_obj_is_held[obj_abs] = (uint8_t)effective_held;
			}
		}

		/* If _distributed_message_object_changes arrived BEFORE !FH: (or was missed on late-join), create/replay now! */
		existing_obj = forge_find_live_object_by_abs_index(obj_abs);
		if (existing_obj == NONE && g_net_obj_has_deferred_create[obj_abs] && def_idx != NONE)
		{
			struct forge_distributed_object_change re_change = g_net_obj_deferred_create[obj_abs];
			g_net_obj_has_deferred_create[obj_abs] = 0;
			re_change.definition_index = def_idx;
			orig_network_objects_handle_changes(&re_change, 1);
			existing_obj = forge_find_live_object_by_abs_index(obj_abs);
		}
		if (existing_obj == NONE && def_idx != NONE && ce->group_tag != 'actv')
		{
			struct forge_distributed_object_change synth;
			memset(&synth, 0, sizeof(synth));
			synth.change = 0; /* _object_change_create */
			synth.owner_player_index = (byte)NONE;
			synth.forced_shader_permutation_index = (byte)NONE;
			synth.object_index = (long)((0x8001u << 16) | (uint32_t)(obj_abs & 0xFFFF));
			synth.definition_index = def_idx;
			synth.owner_team_index = team_designator;
			synth.variant_number = NONE;
			synth.position.x = x;
			synth.position.y = y;
			synth.position.z = z;
			forge_net_euler_to_vectors(yaw, pitch, roll, &synth.forward, &synth.up);
			orig_network_objects_handle_changes(&synth, 1);
			existing_obj = forge_find_live_object_by_abs_index(obj_abs);
		}

		if (weap0_abs >= 0 && weap0_abs < FORGE_MAX_TRACKED_NET_OBJECTS &&
			forge_find_live_object_by_abs_index(weap0_abs) == NONE &&
			weap0_def_idx != NONE)
		{
			if (g_net_obj_has_deferred_create[weap0_abs])
			{
				struct forge_distributed_object_change re_weap = g_net_obj_deferred_create[weap0_abs];
				g_net_obj_has_deferred_create[weap0_abs] = 0;
				re_weap.definition_index = weap0_def_idx;
				orig_network_objects_handle_changes(&re_weap, 1);
			}
			else
			{
				struct forge_distributed_object_change synth_w;
				memset(&synth_w, 0, sizeof(synth_w));
				synth_w.change = 0;
				synth_w.owner_player_index = (byte)NONE;
				synth_w.forced_shader_permutation_index = (byte)NONE;
				synth_w.object_index = (long)((0x8001u << 16) | (uint32_t)(weap0_abs & 0xFFFF));
				synth_w.definition_index = weap0_def_idx;
				synth_w.owner_team_index = team_designator;
				synth_w.variant_number = NONE;
				synth_w.position.x = x;
				synth_w.position.y = y;
				synth_w.position.z = z;
				forge_net_euler_to_vectors(yaw, pitch, roll, &synth_w.forward, &synth_w.up);
				orig_network_objects_handle_changes(&synth_w, 1);
			}
		}

		if (existing_obj != NONE && match_idx >= 0 && match_idx < g_forge.live_instance_count)
		{
			forge_client_configure_linked_object(match_idx, existing_obj, effective_held);
			if (matched_client_unlinked)
			{
				struct forge_live_instance const *inst = &g_forge.live_instances[match_idx];
				forge_3d_move_instance_ext(
					match_idx,
					inst->x, inst->y, inst->z,
					inst->yaw, inst->pitch, inst->roll,
					effective_held,
					0);
				forge_network_send_transform_ext(
					match_idx,
					existing_obj,
					inst->x, inst->y, inst->z,
					inst->yaw, inst->pitch, inst->roll,
					effective_held,
					1);
			}
		}
	}
	else if (matched_client_unlinked && match_idx >= 0 && match_idx < g_forge.live_instance_count)
	{
		/* Non-networked object (scen, mach, lifi, ctrl, spwn, tele, flag) spawned by Client and now linked to host_live_idx:
		   immediately send Client's current held/placed transform so Host updates from initial spawn position! */
		struct forge_live_instance const *inst = &g_forge.live_instances[match_idx];
		int effective_held = (g_forge.grabbed_live_index == match_idx) ? 1 : 0;
		forge_network_send_transform_ext(
			match_idx,
			inst->object_index,
			inst->x, inst->y, inst->z,
			inst->yaw, inst->pitch, inst->roll,
			effective_held,
			1);
	}
}

static int forge_net_coords_valid(float x, float y, float z, int yaw_cr, int pitch_cr, int roll_cr)
{
	if (!(x == x && y == y && z == z))
		return 0;
	if (x < -50000.0f || x > 50000.0f || y < -50000.0f || y > 50000.0f || z < -50000.0f || z > 50000.0f)
		return 0;
	if (yaw_cr < -100000 || yaw_cr > 100000 || pitch_cr < -100000 || pitch_cr > 100000 || roll_cr < -100000 || roll_cr > 100000)
		return 0;
	return 1;
}

int forge_network_intercept_chat_packet(long machine_index, char const *text, int is_from_client_to_host)
{
	if (!text || text[0] != '!' || text[1] != 'F')
		return 0;

	if (!g_forge.initialized)
		forge_runtime_init();

	/* In Standard Mode (!g_forge_session_enabled), only allow initial map state sync (!FQ from client,
	   and host-to-client state sync packets). Never enable g_forge_session_enabled from network packets,
	   and ignore client-to-host Forge edit commands when the server is not in Forge Mode. */
	if (!g_forge_session_enabled && is_from_client_to_host && strcmp(text, "!FQ") != 0)
		return 1;

	/* 1. Per-Player Monitor Mode & 3D Flight State (!FT:) */
	if (strncmp(text, "!FT:", 4) == 0)
	{
		int p_abs = -1, is_mon = 0, yaw_cr = 0, pitch_cr = 0;
		float x = 0.0f, y = 0.0f, z = 0.0f;
		if (sscanf(text + 4, "%d:%d:%f:%f:%f:%d:%d", &p_abs, &is_mon, &x, &y, &z, &yaw_cr, &pitch_cr) == 7 &&
			forge_net_coords_valid(x, y, z, yaw_cr, pitch_cr, 0))
		{
			if (is_from_client_to_host && game_connection() == _game_connection_network_server)
			{
				/* Verify or resolve the player index belonging to this client machine */
				if (p_abs < 0 || p_abs >= 128 || !distributed_machine_has_player(machine_index, (short)p_abs))
				{
					short cand;
					for (cand = 0; cand < 16; cand++)
					{
						if (distributed_machine_has_player(machine_index, cand))
						{
							p_abs = cand;
							break;
						}
					}
				}
				if (p_abs >= 0 && p_abs < 128)
				{
					char relay[64];
					forge_apply_host_player_monitor_state(
						p_abs,
						is_mon,
						x, y, z,
						(float)yaw_cr / 100.0f,
						(float)pitch_cr / 100.0f);
					snprintf(
						relay,
						sizeof(relay),
						"!FT:%d:%d:%.2f:%.2f:%.2f:%d:%d",
						p_abs,
						is_mon ? 1 : 0,
						x, y, z,
						yaw_cr,
						pitch_cr);
					forge_broadcast_packet_to_clients(relay, machine_index);
				}
			}
			else if (!is_from_client_to_host && game_connection() == _game_connection_network_client)
			{
				if (p_abs >= 0 && p_abs < 128 && p_abs != forge_get_local_player_abs_index())
				{
					forge_apply_host_player_monitor_state(
						p_abs,
						is_mon,
						x, y, z,
						(float)yaw_cr / 100.0f,
						(float)pitch_cr / 100.0f);
				}
			}
		}
		return 1;
	}

	/* 2. Client Full-State Sync Request (!FQ) */
	if (strncmp(text, "!FQ", 3) == 0)
	{
		if (is_from_client_to_host && game_connection() == _game_connection_network_server)
		{
			if (machine_index >= 0 && machine_index < FORGE_MAX_CLIENT_MACHINES)
			{
				g_host_machine_known[machine_index] = 1;
				g_host_machine_sync_cursor[machine_index] = 0;
				g_host_machine_mon_synced[machine_index] = 0;
			}
		}
		return 1;
	}

	/* 2b. Default Scenario Object Adoption by Original Coordinates (!FXA:) */
	if (strncmp(text, "!FXA:", 5) == 0)
	{
		unsigned int live_idx_u = 0, hash = 0;
		float ox = 0.0f, oy = 0.0f, oz = 0.0f;
		if (sscanf(text + 5, "%X:%X:%f:%f:%f", &live_idx_u, &hash, &ox, &oy, &oz) == 5 &&
			(int)live_idx_u >= 0 && (int)live_idx_u < FORGE_MAX_LIVE_INSTANCES &&
			forge_net_coords_valid(ox, oy, oz, 0, 0, 0))
		{
			struct forge_catalog_entry const *ce = forge_find_catalog_entry_by_hash((uint32_t)hash);
			forge_3d_adopt_default_object_at_slot(
				(int32_t)live_idx_u,
				ce ? ce->group_tag : 0,
				ce ? ce->tag_path : NULL,
				ox, oy, oz);
			if (is_from_client_to_host && game_connection() == _game_connection_network_server)
			{
				forge_server_mark_dirty();
				forge_broadcast_packet_to_clients(text, machine_index);
			}
		}
		return 1;
	}

	/* 2c. Extended Object Properties Sync (!FP:<slot>:<team>:<gflags>:<phys>:<respawn>:<ai_order>:<group>:<lcol>:<lint>:<lrad>) */
	if (strncmp(text, "!FP:", 4) == 0)
	{
		unsigned int live_idx_u = 0, gflags = 0, pmode = 0, aorder = 0, gid = 0, lcol = 0, lint = 0, lrad = 0;
		int team = 0, rsec = 0;
		if (sscanf(text + 4, "%X:%d:%X:%u:%d:%u:%u:%u:%u:%u",
				&live_idx_u, &team, &gflags, &pmode, &rsec, &aorder, &gid, &lcol, &lint, &lrad) == 10 &&
			(int)live_idx_u >= 0 && (int)live_idx_u < FORGE_MAX_LIVE_INSTANCES)
		{
			if (team < -1) team = -1;
			if (team > 31) team = 31;
			if (pmode >= NUMBER_OF_FORGE_PHYSICS_MODES) pmode = 0;
			if (aorder >= NUMBER_OF_FORGE_AI_ORDERS) aorder = 0;
			if (gid > FORGE_MAX_WELD_GROUPS) gid = 0;
			if (lcol >= NUMBER_OF_FORGE_LIGHT_COLORS) lcol = 0;
			if (lint >= NUMBER_OF_FORGE_LIGHT_INTENSITIES) lint = 0;
			if (lrad >= NUMBER_OF_FORGE_LIGHT_RADII) lrad = 0;
			if (rsec < 0 || rsec > 3600) rsec = 0;

			if ((int)live_idx_u < g_forge.live_instance_count &&
				g_forge.live_instances[live_idx_u].active)
			{
				forge_apply_props_to_live_slot(
					(int32_t)live_idx_u,
					(int16_t)team,
					(uint16_t)gflags,
					(uint8_t)pmode,
					(int16_t)rsec,
					(uint8_t)aorder,
					(uint8_t)gid,
					(uint8_t)lcol,
					(uint8_t)lint,
					(uint8_t)lrad);
				if (is_from_client_to_host && game_connection() == _game_connection_network_server)
				{
					char relay[64];
					forge_server_mark_dirty();
					forge_format_props_packet(relay, sizeof(relay), (int32_t)live_idx_u, &g_forge.live_instances[live_idx_u]);
					forge_broadcast_packet_to_clients(relay, machine_index);
				}
			}
			else
			{
				/* Buffer early !FP: packet if it arrives before !FS: / !FH: creates the live slot */
				struct forge_pending_slot_props *pp = &g_pending_slot_props[live_idx_u];
				pp->valid = 1;
				pp->team_designator = (int16_t)team;
				pp->gametype_flags = (uint16_t)gflags;
				pp->physics_mode = (uint8_t)pmode;
				pp->respawn_timer_sec = (int16_t)rsec;
				pp->ai_order = (uint8_t)aorder;
				pp->group_id = (uint8_t)gid;
				pp->light_color_preset = (uint8_t)lcol;
				pp->light_intensity_preset = (uint8_t)lint;
				pp->light_radius_preset = (uint8_t)lrad;
			}
		}
		return 1;
	}

	/* 3. Client -> Host Packets (!FS:, !FM:, !FD:, !FW) */
	if (is_from_client_to_host && game_connection() == _game_connection_network_server)
	{
		if (strncmp(text, "!FW", 3) == 0)
		{
			forge_server_mark_dirty();
			forge_server_save_map_if_dirty(1);
			return 1;
		}
		if (strncmp(text, "!FS:", 4) == 0)
		{
			unsigned int hash = 0;
			float x = 0.0f, y = 0.0f, z = 0.0f;
			int yaw_cr = 0, pitch_cr = 0, roll_cr = 0, team = 0;
			if (sscanf(text + 4, "%X:%f:%f:%f:%d:%d:%d:%d", &hash, &x, &y, &z, &yaw_cr, &pitch_cr, &roll_cr, &team) == 8 &&
				forge_net_coords_valid(x, y, z, yaw_cr, pitch_cr, roll_cr))
			{
				struct forge_catalog_entry const *ce = forge_find_catalog_entry_by_hash(hash);
				if (ce && ce->group_tag != 'spwn' && ce->group_tag != 'tele' &&
					ce->group_tag != 'flag' && ce->group_tag != 'actv')
				{
					uint32_t check_grp = ce->group_tag;
					if (forge_resolve_engine_tag(ce->group_tag, ce->tag_path, &check_grp) == NONE)
						ce = NULL;
				}
				if (ce)
				{
					int32_t spawned_idx;
					/* Host spawns authoritatively and broadcasts !FH: to all Clients (broadcast_net = 1) */
					spawned_idx = forge_3d_spawn_object_at(
						ce->group_tag,
						ce->tag_path,
						ce->source_cache_id,
						x, y, z,
						(float)yaw_cr / 100.0f,
						(float)pitch_cr / 100.0f,
						(float)roll_cr / 100.0f,
						(int16_t)(team < -1 ? -1 : (team > 31 ? 31 : team)),
						1);
					if (spawned_idx >= 0 && spawned_idx < FORGE_MAX_LIVE_INSTANCES &&
						g_pending_slot_props[spawned_idx].valid)
					{
						struct forge_pending_slot_props const *pp = &g_pending_slot_props[spawned_idx];
						char relay_props[64];
						forge_apply_props_to_live_slot(
							spawned_idx,
							pp->team_designator,
							pp->gametype_flags,
							pp->physics_mode,
							pp->respawn_timer_sec,
							pp->ai_order,
							pp->group_id,
							pp->light_color_preset,
							pp->light_intensity_preset,
							pp->light_radius_preset);
						g_pending_slot_props[spawned_idx].valid = 0;
						forge_format_props_packet(relay_props, sizeof(relay_props), spawned_idx, &g_forge.live_instances[spawned_idx]);
						forge_broadcast_packet_to_clients(relay_props, -1);
					}
					forge_server_mark_dirty();
				}
			}
			return 1;
		}
		else if (strncmp(text, "!FM:", 4) == 0)
		{
			int live_idx = -1, obj_abs = -1, yaw_cr = 0, pitch_cr = 0, roll_cr = 0, is_held = 0;
			float x = 0.0f, y = 0.0f, z = 0.0f;
			int n = sscanf(text + 4, "%d:%d:%f:%f:%f:%d:%d:%d:%d", &live_idx, &obj_abs, &x, &y, &z, &yaw_cr, &pitch_cr, &roll_cr, &is_held);
			if (n >= 8 && live_idx >= -1 && live_idx < FORGE_MAX_LIVE_INSTANCES &&
				obj_abs >= -1 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS &&
				forge_net_coords_valid(x, y, z, yaw_cr, pitch_cr, roll_cr))
			{
				int32_t target_idx = forge_resolve_target_live_idx(live_idx, obj_abs);
				if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
					g_net_obj_is_held[obj_abs] = (n >= 9 && is_held) ? 1 : 0;
				if (target_idx >= 0)
				{
					char relay[64];
					int host_obj_abs = obj_abs;
					if (host_obj_abs < 0 &&
						g_forge.live_instances[target_idx].object_index != NONE &&
						forge_object_is_network_shared(g_forge.live_instances[target_idx].object_index))
					{
						host_obj_abs = (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(g_forge.live_instances[target_idx].object_index);
					}
					forge_3d_move_instance_ext(
						target_idx,
						x, y, z,
						(float)yaw_cr / 100.0f,
						(float)pitch_cr / 100.0f,
						(float)roll_cr / 100.0f,
						(n >= 9) ? is_held : 0,
						0);
					forge_server_mark_dirty();
					snprintf(
						relay,
						sizeof(relay),
						"!FM:%d:%d:%.2f:%.2f:%.2f:%d:%d:%d:%d",
						(int)target_idx,
						host_obj_abs,
						x, y, z,
						yaw_cr,
						pitch_cr,
						roll_cr,
						(n >= 9 && is_held) ? 1 : 0);
					forge_broadcast_packet_to_clients(relay, machine_index);
				}
			}
			return 1;
		}
		else if (strncmp(text, "!FD:", 4) == 0)
		{
			int live_idx = -1, obj_abs = -1;
			if (sscanf(text + 4, "%d:%d", &live_idx, &obj_abs) >= 1 &&
				live_idx >= -1 && live_idx < FORGE_MAX_LIVE_INSTANCES &&
				obj_abs >= -1 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
			{
				int32_t target_idx = forge_resolve_target_live_idx(live_idx, obj_abs);
				if (target_idx >= 0)
				{
					char relay[64];
					int host_obj_abs = obj_abs;
					int k;
					if (host_obj_abs < 0 &&
						g_forge.live_instances[target_idx].object_index != NONE &&
						forge_object_is_network_shared(g_forge.live_instances[target_idx].object_index))
					{
						host_obj_abs = (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(g_forge.live_instances[target_idx].object_index);
					}
					for (k = 0; k < FORGE_MAX_TRACKED_NET_OBJECTS; k++)
					{
						if (g_net_obj_live_index[k] == target_idx)
						{
							g_net_obj_live_index[k] = -1;
							g_net_obj_is_held[k] = 0;
							g_net_obj_def_index[k] = NONE;
							g_net_obj_has_deferred_create[k] = 0;
						}
					}
					if (host_obj_abs >= 0 && host_obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
					{
						g_net_obj_live_index[host_obj_abs] = -1;
						g_net_obj_is_held[host_obj_abs] = 0;
						g_net_obj_def_index[host_obj_abs] = NONE;
						g_net_obj_has_deferred_create[host_obj_abs] = 0;
					}
					if (target_idx < FORGE_MAX_LIVE_INSTANCES)
						g_pending_slot_props[target_idx].valid = 0;
					forge_3d_delete_instance(target_idx, 0);
					forge_server_mark_dirty();
					snprintf(relay, sizeof(relay), "!FD:%d:%d", (int)target_idx, host_obj_abs);
					forge_broadcast_packet_to_clients(relay, machine_index);
				}
			}
			return 1;
		}
	}
	/* 4. Host -> Client Packets (!FH:, !FM:, !FD:) */
	else if (!is_from_client_to_host && game_connection() == _game_connection_network_client)
	{
		if (strncmp(text, "!FH:", 4) == 0)
		{
			unsigned int live_idx_u = 0, hash = 0, obj_enc = 0;
			float x = 0.0f, y = 0.0f, z = 0.0f;
			int yaw_cr = 0, pitch_cr = 0, roll_cr = 0, team = 0;
			if (sscanf(
					text + 4,
					"%X:%X:%X:%f:%f:%f:%d:%d:%d:%d",
					&live_idx_u,
					&hash,
					&obj_enc,
					&x, &y, &z,
					&yaw_cr, &pitch_cr, &roll_cr,
					&team) == 10 &&
				((int)live_idx_u >= -1 && (int)live_idx_u < FORGE_MAX_LIVE_INSTANCES) &&
				forge_net_coords_valid(x, y, z, yaw_cr, pitch_cr, roll_cr))
			{
				unsigned int base_enc = obj_enc & 0xFFFu;
				unsigned int weap0_enc = (obj_enc >> 12) & 0xFFFu;
				int host_is_held = (obj_enc & (1u << 24)) ? 1 : 0;
				int obj_abs = (base_enc > 0) ? (int)(base_enc - 1) : -1;
				int weap0_abs = (weap0_enc > 0) ? (int)(weap0_enc - 1) : -1;
				forge_client_handle_host_spawn(
					(int)live_idx_u,
					(uint32_t)hash,
					obj_abs,
					weap0_abs,
					host_is_held,
					x, y, z,
					(float)yaw_cr / 100.0f,
					(float)pitch_cr / 100.0f,
					(float)roll_cr / 100.0f,
					(int16_t)(team < -1 ? -1 : (team > 31 ? 31 : team)));
			}
			return 1;
		}
		else if (strncmp(text, "!FM:", 4) == 0)
		{
			int live_idx = -1, obj_abs = -1, yaw_cr = 0, pitch_cr = 0, roll_cr = 0, is_held = 0;
			float x = 0.0f, y = 0.0f, z = 0.0f;
			int n = sscanf(text + 4, "%d:%d:%f:%f:%f:%d:%d:%d:%d", &live_idx, &obj_abs, &x, &y, &z, &yaw_cr, &pitch_cr, &roll_cr, &is_held);
			if (n >= 8 && live_idx >= -1 && live_idx < FORGE_MAX_LIVE_INSTANCES &&
				obj_abs >= -1 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS &&
				forge_net_coords_valid(x, y, z, yaw_cr, pitch_cr, roll_cr))
			{
				int32_t target_idx = forge_resolve_target_live_idx(live_idx, obj_abs);
				if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
					g_net_obj_is_held[obj_abs] = (n >= 9 && is_held) ? 1 : 0;
				if (target_idx >= 0 && target_idx != g_forge.grabbed_live_index)
				{
					forge_3d_move_instance_ext(
						target_idx,
						x, y, z,
						(float)yaw_cr / 100.0f,
						(float)pitch_cr / 100.0f,
						(float)roll_cr / 100.0f,
						(n >= 9) ? is_held : 0,
						0);
				}
			}
			return 1;
		}
		else if (strncmp(text, "!FD:", 4) == 0)
		{
			int live_idx = -1, obj_abs = -1;
			if (sscanf(text + 4, "%d:%d", &live_idx, &obj_abs) >= 1 &&
				live_idx >= -1 && live_idx < FORGE_MAX_LIVE_INSTANCES &&
				obj_abs >= -1 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
			{
				int32_t target_idx = forge_resolve_target_live_idx(live_idx, obj_abs);
				if (target_idx >= 0)
				{
					int k;
					for (k = 0; k < FORGE_MAX_TRACKED_NET_OBJECTS; k++)
					{
						if (g_net_obj_live_index[k] == target_idx)
						{
							g_net_obj_live_index[k] = -1;
							g_net_obj_is_held[k] = 0;
							g_net_obj_def_index[k] = NONE;
							g_net_obj_has_deferred_create[k] = 0;
						}
					}
					if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
					{
						g_net_obj_live_index[obj_abs] = -1;
						g_net_obj_is_held[obj_abs] = 0;
						g_net_obj_def_index[obj_abs] = NONE;
						g_net_obj_has_deferred_create[obj_abs] = 0;
					}
					if (target_idx < FORGE_MAX_LIVE_INSTANCES)
						g_pending_slot_props[target_idx].valid = 0;
					forge_3d_delete_instance(target_idx, 0);
				}
			}
			return 1;
		}
	}

	return 1;
}

void forge_network_pre_handle_object_changes(void const *entries, short count)
{
	struct forge_distributed_object_change *changes = (struct forge_distributed_object_change *)entries;
	uint32_t base_tag_count = forge_get_base_map_tag_count();
	short i;

	memset(g_suppressed_change_slots, 0, sizeof(g_suppressed_change_slots));
	if (!changes || count <= 0)
		return;

	for (i = 0; i < count; i++)
	{
		if (changes[i].change == 0 /* _object_change_create */ && changes[i].object_index != NONE)
		{
			int obj_abs = (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(changes[i].object_index);
			if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
			{
				if (changes[i].owner_player_index != (byte)NONE)
				{
					/* A player-owned unit/biped is never a Forge-spawned prop/weapon:
					   clear any stale tracking on this reused datum slot so we never
					   overwrite the respawned player's biped definition_index. */
					g_net_obj_has_deferred_create[obj_abs] = 0;
					g_net_obj_def_index[obj_abs] = NONE;
					g_net_obj_live_index[obj_abs] = -1;
					g_net_obj_is_held[obj_abs] = 0;
					continue;
				}
				if (g_net_obj_def_index[obj_abs] != NONE)
				{
					int32_t live_idx = g_net_obj_live_index[obj_abs];
					int valid_forge_slot =
						(live_idx >= 0 && live_idx < g_forge.live_instance_count &&
						 g_forge.live_instances[live_idx].active) ||
						(base_tag_count > 0 && changes[i].definition_index != NONE &&
						 (uint32_t)DATUM_INDEX_TO_ABSOLUTE_INDEX(changes[i].definition_index) >= base_tag_count);
					if (!valid_forge_slot)
					{
						g_net_obj_def_index[obj_abs] = NONE;
						g_net_obj_live_index[obj_abs] = -1;
						g_net_obj_is_held[obj_abs] = 0;
					}
				}
				g_net_obj_deferred_create[obj_abs] = changes[i];
				g_net_obj_has_deferred_create[obj_abs] = 1;
				if (g_net_obj_def_index[obj_abs] != NONE)
				{
					changes[i].definition_index = g_net_obj_def_index[obj_abs];
				}
				else if (base_tag_count > 0 &&
						 changes[i].definition_index != NONE &&
						 (uint32_t)DATUM_INDEX_TO_ABSOLUTE_INDEX(changes[i].definition_index) >= base_tag_count)
				{
					/* Dynamic tag from Host not yet resolved on Client via !FH: — defer creation cleanly */
					if (i < (short)sizeof(g_suppressed_change_slots))
					{
						g_suppressed_change_slots[i] = 1;
						changes[i].change = 0xFF;
					}
				}
			}
		}
		else if (changes[i].change == 1 /* _object_change_delete */ && changes[i].object_index != NONE)
		{
			int obj_abs = (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(changes[i].object_index);
			if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
			{
				g_net_obj_has_deferred_create[obj_abs] = 0;
				g_net_obj_def_index[obj_abs] = NONE;
				g_net_obj_live_index[obj_abs] = -1;
				g_net_obj_is_held[obj_abs] = 0;
			}
		}
	}
}

void forge_network_post_handle_object_changes(void const *entries, short count)
{
	struct forge_distributed_object_change *changes = (struct forge_distributed_object_change *)entries;
	short i;

	if (!changes || count <= 0)
		return;

	for (i = 0; i < count; i++)
	{
		if (i < (short)sizeof(g_suppressed_change_slots) && g_suppressed_change_slots[i])
		{
			changes[i].change = 0;
			continue;
		}
		if (changes[i].change == 0 /* _object_change_create */ && changes[i].object_index != NONE)
		{
			int obj_abs = (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(changes[i].object_index);
			if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
			{
				long existing_obj = forge_find_live_object_by_abs_index(obj_abs);
				if (existing_obj != NONE)
				{
					int32_t live_idx = g_net_obj_live_index[obj_abs];
					int is_held = g_net_obj_is_held[obj_abs] ? 1 : 0;
					g_net_obj_has_deferred_create[obj_abs] = 0;
					if (live_idx >= 0 && g_forge.grabbed_live_index == live_idx)
						is_held = 1;
					forge_client_configure_linked_object(live_idx, existing_obj, is_held);
				}
			}
		}
	}
}
