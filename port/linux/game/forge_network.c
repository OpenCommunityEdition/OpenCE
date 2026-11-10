/*
 * OpenCE : Forge
 * Collaborative P2P Forge Network Synchronization:
 * - Bidirectional Spawn (!FS: / !FH:), Transform (!FM:), and Delete (!FD:) replication
 * - Per-Player Monitor vs. Spartan mode & 3D Monitor flight state synchronization (!FT:)
 * - Late-join / full-session state synchronization (!FQ)
 * - Automatic client-side external tag faulting on object creation
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

#undef malloc
#undef calloc
#undef free
#undef realloc

extern int config_boolean(const char *name);
extern boolean network_distributed_client_send_reliably(void *message, word size);
extern boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
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

/* ---------- Zero-Trust Session-RAM Custom Tag Streaming State */

#define FORGE_TAG_REQUEST_MAGIC               0x51525446u /* 'FTRQ' */
#define FORGE_TAG_STREAM_MAGIC                0x54535446u /* 'FTST' */
#define FORGE_TAG_STREAM_OP_MISS              0u
#define FORGE_TAG_STREAM_OP_OFFER             1u
#define FORGE_TAG_STREAM_OP_CHUNK             2u

#define FORGE_TAG_STREAM_CHUNK_BYTES          1280u
#define FORGE_TAG_STREAM_MAX_COMP_BYTES       (10u * 1024u * 1024u)
#define FORGE_TAG_STREAM_MAX_UNCOMP_BYTES     (14u * 1024u * 1024u)
#define FORGE_TAG_STREAM_MAX_CHUNKS           ((FORGE_TAG_STREAM_MAX_COMP_BYTES + FORGE_TAG_STREAM_CHUNK_BYTES - 1u) / FORGE_TAG_STREAM_CHUNK_BYTES)
#define FORGE_TAG_STREAM_CHUNKS_PER_TICK      6
#define FORGE_TAG_STREAM_STALL_TIMEOUT_MS     15000u
#define FORGE_TAG_STREAM_MAX_SESSION_TAGS     32
#define FORGE_TAG_STREAM_MAX_STRIKES          3
#define FORGE_TAG_STREAM_MAX_CLIENT_REQS      64

struct forge_tag_request_packet
{
	struct distributed_message_header header;
	uint32_t magic;
	uint32_t hash;
	uint32_t transfer_id;
};

struct forge_tag_stream_miss_packet
{
	struct distributed_message_header header;
	uint32_t magic;
	uint8_t op;
	uint8_t reserved[3];
	uint32_t hash;
	uint32_t transfer_id;
};

struct forge_tag_stream_offer_packet
{
	struct distributed_message_header header;
	uint32_t magic;
	uint8_t op;
	uint8_t bundle_format;
	uint16_t chunk_count;
	uint32_t hash;
	uint32_t transfer_id;
	uint32_t root_group;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint8_t blake2b_256[32];
	char tag_path[256];
	char display_name[64];
};

struct forge_tag_stream_chunk_packet
{
	struct distributed_message_header header;
	uint32_t magic;
	uint8_t op;
	uint8_t reserved;
	uint16_t chunk_index;
	uint32_t hash;
	uint32_t transfer_id;
	uint16_t payload_len;
	uint16_t pad;
	uint8_t payload[FORGE_TAG_STREAM_CHUNK_BYTES];
};

struct forge_host_cached_tag_bundle
{
	int valid;
	int missing;
	uint32_t hash;
	uint8_t bundle_format;
	uint32_t root_group;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t chunk_count;
	uint8_t blake2b_256[32];
	char tag_path[256];
	char display_name[64];
	uint8_t *comp_bytes;
};

struct forge_host_client_tag_tx
{
	int active;
	int offer_sent;
	int cache_slot;
	uint32_t hash;
	uint32_t transfer_id;
	uint16_t next_chunk;
	int req_count;
};

enum forge_client_tag_rx_state
{
	_forge_tag_rx_idle = 0,
	_forge_tag_rx_wait_offer = 1,
	_forge_tag_rx_receiving = 2
};

struct forge_client_pending_instance
{
	uint8_t active;
	uint8_t is_held;
	int16_t team_designator;
	uint32_t hash;
	int32_t obj_abs;
	int32_t weap0_abs;
};

static struct forge_host_cached_tag_bundle g_host_tag_cache[FORGE_TAG_STREAM_MAX_SESSION_TAGS];
static struct forge_host_client_tag_tx g_host_tag_tx[FORGE_MAX_CLIENT_MACHINES];

static struct
{
	enum forge_client_tag_rx_state state;
	uint32_t hash;
	uint32_t transfer_id;
	uint32_t next_transfer_seq;
	uint8_t bundle_format;
	uint32_t root_group;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t chunk_count;
	uint16_t chunks_received;
	unsigned long last_progress_ms;
	uint8_t blake2b_256[32];
	char tag_path[256];
	char display_name[64];
	uint8_t *comp_buf;
	uint32_t chunk_seen[(FORGE_TAG_STREAM_MAX_CHUNKS + 31u) / 32u];
	uint32_t queue[FORGE_TAG_STREAM_MAX_SESSION_TAGS];
	int queue_count;
	uint32_t settled_hashes[FORGE_TAG_STREAM_MAX_SESSION_TAGS * 2];
	int settled_count;
	int downloaded_count;
	int strikes;
} g_client_tag_rx;

static struct forge_client_pending_instance g_pending_inst_stream[FORGE_MAX_LIVE_INSTANCES];

static void forge_tag_stream_reset_state(void);
static void forge_tag_stream_tick(unsigned long now);
static void forge_client_enqueue_tag_download(uint32_t hash);
static int32_t forge_client_upsert_stream_proxy_instance(
	int host_live_idx,
	uint32_t hash,
	int obj_abs,
	int weap0_abs,
	int host_is_held,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int16_t team_designator);

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
		g_net_obj_has_deferred_create[i] = 0;
	}
	for (i = 0; i < FORGE_MAX_CLIENT_MACHINES; i++)
	{
		g_host_machine_known[i] = 0;
		g_host_machine_sync_cursor[i] = -1;
		g_host_machine_mon_synced[i] = 0;
	}
	forge_tag_stream_reset_state();
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
		if (!g_client_requested_full_sync && g_forge.monitor_cam_initialized && (now - g_client_join_ms >= 250))
		{
			g_client_requested_full_sync = 1;
			forge_send_packet_to_host("!FQ");
			if (g_forge.mode == _forge_mode_3d_monitor)
				forge_network_send_monitor_mode(1, g_forge.cam_x, g_forge.cam_y, g_forge.cam_z, g_forge.cam_yaw, g_forge.cam_pitch);
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
				memset(&g_host_tag_tx[m], 0, sizeof(g_host_tag_tx[m]));
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
			while (g_host_machine_sync_cursor[m] >= 0 &&
				   g_host_machine_sync_cursor[m] < g_forge.live_instance_count &&
				   packets_sent < 6)
			{
				int32_t idx = g_host_machine_sync_cursor[m]++;
				struct forge_live_instance const *inst = &g_forge.live_instances[idx];
				char msg[64];

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
			}

			if (g_host_machine_sync_cursor[m] >= g_forge.live_instance_count)
				g_host_machine_sync_cursor[m] = -1;
		}
	}

	forge_tag_stream_tick(now);
	forge_server_autosave_tick();
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
	if (obj_abs < object_header_data->count)
	{
		long cand;
		for (cand = data_next_index(object_header_data, NONE);
			 cand != NONE;
			 cand = data_next_index(object_header_data, cand))
		{
			if ((int)DATUM_INDEX_TO_ABSOLUTE_INDEX(cand) == obj_abs)
				return cand;
		}
	}
	return NONE;
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
	 * before inserting any host-replicated objects, even if !FH: arrives on Frame 0 before forge_3d_update()! */
	if (g_forge.live_instance_count == 0)
		forge_init_map_scenario_entities();

	if (!ce)
	{
		forge_client_upsert_stream_proxy_instance(
			host_live_idx,
			hash,
			obj_abs,
			weap0_abs,
			host_is_held,
			x, y, z,
			yaw, pitch, roll,
			team_designator);
		forge_client_enqueue_tag_download(hash);
		return;
	}

	actual_grp = ce->group_tag;
	if (ce->group_tag != 'spwn' && ce->group_tag != 'tele' && ce->group_tag != 'flag')
	{
		/* Fault in external Xbox campaign tags or cached tag bundles on the client before object creation! */
		def_idx = forge_resolve_engine_tag(ce->group_tag, ce->tag_path, &actual_grp);
		if (def_idx == NONE && ce->group_tag != 'actv')
		{
			forge_client_upsert_stream_proxy_instance(
				host_live_idx,
				hash,
				obj_abs,
				weap0_abs,
				host_is_held,
				x, y, z,
				yaw, pitch, roll,
				team_designator);
			forge_client_enqueue_tag_download(hash);
			return;
		}
	}

	/* Only units and items share lower-1024 obj_abs indices across the network (network_objects.c).
	 * Non-networked objects (scen, mach, lifi) are spawned locally on each machine at independent indices. */
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

	/* 1. If the Client is currently holding a locally-created unlinked instance (pad_inst & 0x80) of this tag, match it first */
	if (g_forge.grabbed_live_index >= 0 && g_forge.grabbed_live_index < g_forge.live_instance_count)
	{
		struct forge_live_instance const *cand = &g_forge.live_instances[g_forge.grabbed_live_index];
		if (cand->active && !cand->is_default_scenario_object &&
			(cand->pad_inst & 0x80u) != 0 &&
			cand->group_tag == ce->group_tag && _stricmp(cand->tag_path, ce->tag_path) == 0)
		{
			match_idx = g_forge.grabbed_live_index;
			matched_client_unlinked = 1;
		}
	}

	/* 2. Check if host_live_idx points to a locally-created unlinked instance of this tag */
	if (match_idx < 0 && host_live_idx >= 0 && host_live_idx < g_forge.live_instance_count)
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

	/* 3. Scan for any locally-created unlinked Client instance (pad_inst & 0x80) of this tag */
	if (match_idx < 0)
	{
		for (i = g_forge.live_instance_count - 1; i >= 0; i--)
		{
			struct forge_live_instance const *cand = &g_forge.live_instances[i];
			if (!cand->active || cand->is_default_scenario_object ||
				(cand->pad_inst & 0x80u) == 0)
				continue;
			if (cand->group_tag == ce->group_tag && _stricmp(cand->tag_path, ce->tag_path) == 0)
			{
				match_idx = i;
				matched_client_unlinked = 1;
				break;
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

		/* If _distributed_message_object_changes arrived BEFORE !FH:, replay deferred creation now! */
		existing_obj = forge_find_live_object_by_abs_index(obj_abs);
		if (existing_obj == NONE && g_net_obj_has_deferred_create[obj_abs] && def_idx != NONE)
		{
			struct forge_distributed_object_change re_change = g_net_obj_deferred_create[obj_abs];
			g_net_obj_has_deferred_create[obj_abs] = 0;
			re_change.definition_index = def_idx;
			orig_network_objects_handle_changes(&re_change, 1);
			existing_obj = forge_find_live_object_by_abs_index(obj_abs);
		}

		if (weap0_abs >= 0 && weap0_abs < FORGE_MAX_TRACKED_NET_OBJECTS &&
			forge_find_live_object_by_abs_index(weap0_abs) == NONE &&
			g_net_obj_has_deferred_create[weap0_abs] && weap0_def_idx != NONE)
		{
			struct forge_distributed_object_change re_weap = g_net_obj_deferred_create[weap0_abs];
			g_net_obj_has_deferred_create[weap0_abs] = 0;
			re_weap.definition_index = weap0_def_idx;
			orig_network_objects_handle_changes(&re_weap, 1);
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
		 * immediately send Client's current held/placed transform so Host updates from initial spawn position! */
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

	if (!g_forge_session_enabled)
	{
		g_forge_session_enabled = 1;
		forge_runtime_init();
	}

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
				if (ce)
				{
					/* Host spawns authoritatively and broadcasts !FH: to all Clients (broadcast_net = 1) */
					forge_3d_spawn_object_at(
						ce->group_tag,
						ce->tag_path,
						ce->source_cache_id,
						x, y, z,
						(float)yaw_cr / 100.0f,
						(float)pitch_cr / 100.0f,
						(float)roll_cr / 100.0f,
						(int16_t)(team < -1 ? -1 : (team > 8 ? 8 : team)),
						1);
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
						}
					}
					if (host_obj_abs >= 0 && host_obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
					{
						g_net_obj_live_index[host_obj_abs] = -1;
						g_net_obj_is_held[host_obj_abs] = 0;
					}
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
					(int16_t)(team < -1 ? -1 : (team > 8 ? 8 : team)));
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
				if (target_idx >= 0 && target_idx < FORGE_MAX_LIVE_INSTANCES && g_pending_inst_stream[target_idx].active)
				{
					g_pending_inst_stream[target_idx].is_held = (n >= 9 && is_held) ? 1u : 0u;
				}
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
						}
					}
					if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
					{
						g_net_obj_live_index[obj_abs] = -1;
						g_net_obj_is_held[obj_abs] = 0;
					}
					if (target_idx < FORGE_MAX_LIVE_INSTANCES)
						memset(&g_pending_inst_stream[target_idx], 0, sizeof(g_pending_inst_stream[target_idx]));
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

/* ============================================================================
 * ZERO-TRUST SOLICITED SINGLE-FLIGHT CUSTOM TAG STREAMING (SESSION RAM ONLY)
 * ============================================================================ */

static void forge_tag_stream_fill_header(struct distributed_message_header *hdr, byte type, word size)
{
	hdr->header = 0;
	build_message_header(&hdr->header, size, 2, 0);
	hdr->type = type;
	hdr->count = 1;
	hdr->game_time = game_in_progress() ? game_time_get() : 0;
	hdr->checkpoint_epoch = 0;
}

static void forge_tag_stream_reset_state(void)
{
	int i;

	for (i = 0; i < FORGE_TAG_STREAM_MAX_SESSION_TAGS; i++)
	{
		if (g_host_tag_cache[i].comp_bytes)
			free(g_host_tag_cache[i].comp_bytes);
	}
	memset(g_host_tag_cache, 0, sizeof(g_host_tag_cache));
	memset(g_host_tag_tx, 0, sizeof(g_host_tag_tx));

	if (g_client_tag_rx.comp_buf)
		free(g_client_tag_rx.comp_buf);
	memset(&g_client_tag_rx, 0, sizeof(g_client_tag_rx));

	memset(g_pending_inst_stream, 0, sizeof(g_pending_inst_stream));
}

static int forge_client_is_hash_settled(uint32_t hash)
{
	int i;
	if (!hash)
		return 1;
	for (i = 0; i < g_client_tag_rx.settled_count; i++)
	{
		if (g_client_tag_rx.settled_hashes[i] == hash)
			return 1;
	}
	return 0;
}

static void forge_client_mark_hash_settled(uint32_t hash)
{
	if (!hash || forge_client_is_hash_settled(hash))
		return;
	if (g_client_tag_rx.settled_count < (int)(sizeof(g_client_tag_rx.settled_hashes) / sizeof(g_client_tag_rx.settled_hashes[0])))
		g_client_tag_rx.settled_hashes[g_client_tag_rx.settled_count++] = hash;
}

static void forge_client_abort_rx(int add_strike)
{
	if (g_client_tag_rx.comp_buf)
	{
		free(g_client_tag_rx.comp_buf);
		g_client_tag_rx.comp_buf = NULL;
	}
	if (g_client_tag_rx.hash)
		forge_client_mark_hash_settled(g_client_tag_rx.hash);
	g_client_tag_rx.state = _forge_tag_rx_idle;
	g_client_tag_rx.hash = 0;
	g_client_tag_rx.transfer_id = 0;
	g_client_tag_rx.chunk_count = 0;
	g_client_tag_rx.chunks_received = 0;
	g_client_tag_rx.compressed_size = 0;
	g_client_tag_rx.uncompressed_size = 0;
	memset(g_client_tag_rx.chunk_seen, 0, sizeof(g_client_tag_rx.chunk_seen));

	if (add_strike)
	{
		g_client_tag_rx.strikes++;
		if (g_client_tag_rx.strikes >= FORGE_TAG_STREAM_MAX_STRIKES)
			g_client_tag_rx.queue_count = 0;
	}
}

static void forge_client_enqueue_tag_download(uint32_t hash)
{
	int i;

	if (!hash)
		return;
	if (!config_boolean("network.forge_tag_download"))
		return;
	if (g_client_tag_rx.strikes >= FORGE_TAG_STREAM_MAX_STRIKES)
		return;
	if (g_client_tag_rx.downloaded_count >= FORGE_TAG_STREAM_MAX_SESSION_TAGS)
		return;
	if (forge_client_is_hash_settled(hash))
		return;
	if (g_client_tag_rx.state != _forge_tag_rx_idle && g_client_tag_rx.hash == hash)
		return;
	for (i = 0; i < g_client_tag_rx.queue_count; i++)
	{
		if (g_client_tag_rx.queue[i] == hash)
			return;
	}
	if (g_client_tag_rx.queue_count < FORGE_TAG_STREAM_MAX_SESSION_TAGS)
		g_client_tag_rx.queue[g_client_tag_rx.queue_count++] = hash;
}

static int32_t forge_client_upsert_stream_proxy_instance(
	int host_live_idx,
	uint32_t hash,
	int obj_abs,
	int weap0_abs,
	int host_is_held,
	float x, float y, float z,
	float yaw, float pitch, float roll,
	int16_t team_designator)
{
	int32_t slot = -1;
	struct forge_live_instance *inst;

	if (host_live_idx >= 0 && host_live_idx < FORGE_MAX_LIVE_INSTANCES)
	{
		if (host_live_idx >= g_forge.live_instance_count)
		{
			memset(
				&g_forge.live_instances[g_forge.live_instance_count],
				0,
				(size_t)(host_live_idx - g_forge.live_instance_count + 1) * sizeof(struct forge_live_instance));
			g_forge.live_instance_count = host_live_idx + 1;
		}
		slot = host_live_idx;
	}
	else if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS &&
			 g_net_obj_live_index[obj_abs] >= 0 &&
			 g_net_obj_live_index[obj_abs] < g_forge.live_instance_count)
	{
		slot = g_net_obj_live_index[obj_abs];
	}
	else if (g_forge.live_instance_count < FORGE_MAX_LIVE_INSTANCES)
	{
		slot = g_forge.live_instance_count++;
	}
	else
	{
		int32_t s;
		for (s = 0; s < g_forge.live_instance_count; s++)
		{
			if (!g_forge.live_instances[s].active)
			{
				slot = s;
				break;
			}
		}
	}

	if (slot < 0 || slot >= FORGE_MAX_LIVE_INSTANCES)
		return -1;

	inst = &g_forge.live_instances[slot];
	if (!(inst->active && (inst->pad_inst & 0x40u) != 0 && g_pending_inst_stream[slot].hash == hash))
	{
		if (inst->active && inst->object_index != NONE)
		{
			if (object_try_and_get_and_verify_type(inst->object_index, (unsigned long)-1))
				object_delete(inst->object_index);
		}
		memset(inst, 0, sizeof(*inst));
		inst->active = 1;
		inst->object_index = NONE;
		inst->actor_index = NONE;
		inst->edit_entry_index = -1;
		inst->group_tag = 'scen';
		inst->source_cache_id = 4;
		inst->netgame_flag_type = -1;
		inst->pad_inst = 0x40u;
		snprintf(inst->tag_path, sizeof(inst->tag_path), "streaming\\tag_%08X", (unsigned int)hash);
		snprintf(inst->label, sizeof(inst->label), "Streaming Custom Tag...");
	}

	inst->team_designator = team_designator;
	inst->x = x;
	inst->y = y;
	inst->z = z;
	inst->yaw = yaw;
	inst->pitch = pitch;
	inst->roll = roll;

	g_pending_inst_stream[slot].active = 1u;
	g_pending_inst_stream[slot].is_held = host_is_held ? 1u : 0u;
	g_pending_inst_stream[slot].team_designator = team_designator;
	g_pending_inst_stream[slot].hash = hash;
	g_pending_inst_stream[slot].obj_abs = obj_abs;
	g_pending_inst_stream[slot].weap0_abs = weap0_abs;

	if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
	{
		int k;
		for (k = 0; k < FORGE_MAX_TRACKED_NET_OBJECTS; k++)
		{
			if (g_net_obj_live_index[k] == slot)
				g_net_obj_live_index[k] = -1;
		}
		g_net_obj_live_index[obj_abs] = slot;
		g_net_obj_is_held[obj_abs] = host_is_held ? 1u : 0u;
	}

	return slot;
}

static void forge_client_materialize_streamed_hash(
	uint32_t hash,
	uint32_t root_group,
	char const *tag_path,
	long def_idx,
	uint32_t actual_grp)
{
	int32_t idx;
	int is_net_obj = (actual_grp == 'vehi' || actual_grp == 'weap' ||
					  actual_grp == 'eqip' || actual_grp == 'bipd');
	long weap0_def_idx = NONE;

	if (def_idx == NONE || !tag_path || !tag_path[0])
		return;

	if (is_net_obj && actual_grp == 'vehi')
	{
		struct unit_definition const *udef = (struct unit_definition const *)tag_get('unit', def_idx);
		if (udef && udef->unit.initial_weapons.count > 0 && udef->unit.initial_weapons.address)
		{
			struct tag_reference const *iw_ref = (struct tag_reference const *)udef->unit.initial_weapons.address;
			if (iw_ref->index != NONE)
				weap0_def_idx = iw_ref->index;
		}
	}

	for (idx = 0; idx < g_forge.live_instance_count && idx < FORGE_MAX_LIVE_INSTANCES; idx++)
	{
		struct forge_live_instance *inst = &g_forge.live_instances[idx];
		struct forge_client_pending_instance pend = g_pending_inst_stream[idx];

		if (!pend.active || pend.hash != hash)
			continue;
		memset(&g_pending_inst_stream[idx], 0, sizeof(g_pending_inst_stream[idx]));
		if (!inst->active)
			continue;

		inst->group_tag = root_group;
		inst->source_cache_id = 4;
		inst->pad_inst &= (uint8_t)~0x40u;
		inst->label[0] = '\0';
		snprintf(inst->tag_path, sizeof(inst->tag_path), "%s", tag_path);

		if (inst->edit_entry_index < 0 && g_forge.edit_state.placed_object_count < FORGE_MAX_PLACED_OBJECTS)
		{
			struct forge_placed_object_entry *po =
				&g_forge.edit_state.placed_objects[g_forge.edit_state.placed_object_count];
			inst->edit_entry_index = g_forge.edit_state.placed_object_count++;
			memset(po, 0, sizeof(*po));
			po->active = 1;
			po->source_cache_id = 4;
			po->group_tag = root_group;
			po->gametype_flags = inst->gametype_flags;
			po->category = (root_group == 'vehi') ? _forge_object_vehicle :
				(root_group == 'weap') ? _forge_object_weapon :
				(root_group == 'bipd') ? _forge_object_biped :
				(root_group == 'eqip') ? _forge_object_equipment : _forge_object_scenery;
			snprintf(po->tag_path, sizeof(po->tag_path), "%s", tag_path);
			po->x = inst->x;
			po->y = inst->y;
			po->z = inst->z;
			po->yaw = inst->yaw;
			po->pitch = inst->pitch;
			po->roll = inst->roll;
		}

		if (is_net_obj)
		{
			int obj_abs = pend.obj_abs;
			int weap0_abs = pend.weap0_abs;
			int eff_held = pend.is_held ? 1 : 0;
			long existing_obj;

			if (obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS)
			{
				g_net_obj_def_index[obj_abs] = def_idx;
				g_net_obj_live_index[obj_abs] = idx;
				eff_held = g_net_obj_is_held[obj_abs] ? 1 : eff_held;
			}
			if (weap0_abs >= 0 && weap0_abs < FORGE_MAX_TRACKED_NET_OBJECTS && weap0_def_idx != NONE)
			{
				g_net_obj_def_index[weap0_abs] = weap0_def_idx;
			}

			existing_obj = (obj_abs >= 0) ? forge_find_live_object_by_abs_index(obj_abs) : NONE;
			if (existing_obj == NONE && obj_abs >= 0 && obj_abs < FORGE_MAX_TRACKED_NET_OBJECTS &&
				g_net_obj_has_deferred_create[obj_abs])
			{
				struct forge_distributed_object_change re_change = g_net_obj_deferred_create[obj_abs];
				g_net_obj_has_deferred_create[obj_abs] = 0;
				re_change.definition_index = def_idx;
				orig_network_objects_handle_changes(&re_change, 1);
				existing_obj = forge_find_live_object_by_abs_index(obj_abs);
			}
			if (weap0_abs >= 0 && weap0_abs < FORGE_MAX_TRACKED_NET_OBJECTS &&
				forge_find_live_object_by_abs_index(weap0_abs) == NONE &&
				g_net_obj_has_deferred_create[weap0_abs] && weap0_def_idx != NONE)
			{
				struct forge_distributed_object_change re_weap = g_net_obj_deferred_create[weap0_abs];
				g_net_obj_has_deferred_create[weap0_abs] = 0;
				re_weap.definition_index = weap0_def_idx;
				orig_network_objects_handle_changes(&re_weap, 1);
			}
			if (existing_obj != NONE)
			{
				forge_client_configure_linked_object(idx, existing_obj, eff_held);
				forge_3d_move_instance_ext(
					idx,
					inst->x, inst->y, inst->z,
					inst->yaw, inst->pitch, inst->roll,
					eff_held,
					0);
			}
		}
		else
		{
			forge_create_engine_object_for_instance(idx, def_idx, pend.is_held ? 1 : 0);
		}
	}
}

static int forge_stream_path_is_valid_ascii(char const *s, size_t buf_cap)
{
	size_t i;
	int prev_sep = 1;

	if (!s)
		return 0;
	for (i = 0; i < buf_cap; i++)
	{
		unsigned char c = (unsigned char)s[i];
		if (c == '\0')
			break;
		if (i >= 240u || c < 0x20u || c > 0x7Eu)
			return 0;
		if (c == ':' || c == '|' || c == '"' || c == '*' || c == '?' || c == '<' || c == '>')
			return 0;
		if (c == '\\' || c == '/')
		{
			if (prev_sep)
				return 0;
			prev_sep = 1;
		}
		else
		{
			if (c == '.' && i + 1u < buf_cap && s[i + 1u] == '.')
				return 0;
			prev_sep = 0;
		}
	}
	if (i == 0 || i >= buf_cap || prev_sep)
		return 0;
	return 1;
}

static int forge_stream_offer_is_valid(
	struct forge_tag_stream_offer_packet const *offer,
	uint32_t expected_hash,
	uint32_t expected_tid)
{
	uint32_t expected_chunks;
	uint32_t grp;
	size_t di;
	int disp_terminated = 0;

	if (!offer || !expected_hash || !expected_tid)
		return 0;
	if (offer->magic != FORGE_TAG_STREAM_MAGIC || offer->op != FORGE_TAG_STREAM_OP_OFFER)
		return 0;
	if (offer->hash != expected_hash || offer->transfer_id != expected_tid)
		return 0;
	if (offer->bundle_format != 1u && offer->bundle_format != 2u)
		return 0;

	grp = offer->root_group;
	if (grp != 'scen' && grp != 'vehi' && grp != 'weap' && grp != 'eqip' &&
		grp != 'mach' && grp != 'lifi' && grp != 'ctrl' && grp != 'bipd')
	{
		return 0;
	}
	if (offer->compressed_size == 0u || offer->compressed_size > FORGE_TAG_STREAM_MAX_COMP_BYTES)
		return 0;
	if (offer->uncompressed_size < 36u || offer->uncompressed_size > FORGE_TAG_STREAM_MAX_UNCOMP_BYTES)
		return 0;

	expected_chunks = (offer->compressed_size + FORGE_TAG_STREAM_CHUNK_BYTES - 1u) / FORGE_TAG_STREAM_CHUNK_BYTES;
	if (expected_chunks == 0u || expected_chunks > FORGE_TAG_STREAM_MAX_CHUNKS ||
		(uint32_t)offer->chunk_count != expected_chunks)
	{
		return 0;
	}
	if (!forge_stream_path_is_valid_ascii(offer->tag_path, sizeof(offer->tag_path)))
		return 0;
	for (di = 0; di < sizeof(offer->display_name); di++)
	{
		unsigned char dc = (unsigned char)offer->display_name[di];
		if (dc == '\0')
		{
			disp_terminated = 1;
			break;
		}
		if (dc < 0x20u || dc > 0x7Eu)
			return 0;
	}
	if (!disp_terminated)
		return 0;
	if (forge_compute_catalog_hash(offer->root_group, offer->tag_path) != expected_hash)
		return 0;
	return 1;
}

static int forge_host_get_or_build_cached_bundle(uint32_t hash)
{
	int i, free_slot = -1;
	uint8_t fmt = 0;
	uint32_t grp = 0;
	char tpath[256];
	char dname[64];
	uint8_t *raw_bytes = NULL;
	uint32_t raw_size = 0;
	uint8_t *comp_bytes = NULL;
	uint32_t comp_size = 0;
	struct forge_host_cached_tag_bundle *entry;

	for (i = 0; i < FORGE_TAG_STREAM_MAX_SESSION_TAGS; i++)
	{
		if (g_host_tag_cache[i].valid && g_host_tag_cache[i].hash == hash)
			return i;
		if (!g_host_tag_cache[i].valid && free_slot < 0)
			free_slot = i;
	}
	if (free_slot < 0)
		return -1;

	entry = &g_host_tag_cache[free_slot];
	memset(entry, 0, sizeof(*entry));
	entry->valid = 1;
	entry->hash = hash;

	if (!forge_export_custom_tag_bundle(
			hash, &fmt, &grp, tpath, sizeof(tpath), dname, sizeof(dname), &raw_bytes, &raw_size) ||
		!raw_bytes || raw_size < 36u || raw_size > FORGE_TAG_STREAM_MAX_UNCOMP_BYTES)
	{
		free(raw_bytes);
		entry->missing = 1;
		return free_slot;
	}

	if (!forge_zlib_compress_buffer(raw_bytes, raw_size, &comp_bytes, &comp_size) ||
		!comp_bytes || comp_size == 0u || comp_size > FORGE_TAG_STREAM_MAX_COMP_BYTES)
	{
		free(raw_bytes);
		free(comp_bytes);
		entry->missing = 1;
		return free_slot;
	}

	p2p_blake2b_256(raw_bytes, (unsigned long)raw_size, entry->blake2b_256);
	free(raw_bytes);

	entry->missing = 0;
	entry->bundle_format = fmt;
	entry->root_group = grp;
	entry->compressed_size = comp_size;
	entry->uncompressed_size = raw_size;
	entry->chunk_count = (uint16_t)((comp_size + FORGE_TAG_STREAM_CHUNK_BYTES - 1u) / FORGE_TAG_STREAM_CHUNK_BYTES);
	snprintf(entry->tag_path, sizeof(entry->tag_path), "%s", tpath);
	snprintf(entry->display_name, sizeof(entry->display_name), "%s", dname);
	entry->comp_bytes = comp_bytes;
	return free_slot;
}

unsigned char forge_tag_stream_handles_message(void const *message, unsigned short size)
{
	struct distributed_message_header hdr;

	if (!message || size < sizeof(hdr))
		return FALSE;
	memcpy(&hdr, message, sizeof(hdr));
	return (hdr.type == _distributed_message_forge_tag_request ||
			hdr.type == _distributed_message_forge_tag_stream) ? TRUE : FALSE;
}

void forge_tag_stream_handle_message(
	long machine_index,
	void const *message,
	unsigned short size,
	unsigned char from_stream)
{
	struct distributed_message_header hdr;

	/* Reject any datagram / batch spoof: tag streaming runs strictly on the reliable stream */
	if (!from_stream || !message || size < sizeof(hdr) || !game_in_progress() || !g_forge_session_enabled)
		return;

	memcpy(&hdr, message, sizeof(hdr));

	/* 1. Host handling a Client's solicited tag request */
	if (hdr.type == _distributed_message_forge_tag_request)
	{
		struct forge_tag_request_packet req;
		struct forge_host_client_tag_tx *tx;
		int slot;

		if (game_connection() != _game_connection_network_server ||
			machine_index < 0 || machine_index >= FORGE_MAX_CLIENT_MACHINES ||
			size != sizeof(req))
		{
			return;
		}
		memcpy(&req, message, sizeof(req));
		if (req.magic != FORGE_TAG_REQUEST_MAGIC || req.hash == 0u || req.transfer_id == 0u)
			return;

		tx = &g_host_tag_tx[machine_index];
		if (tx->req_count >= FORGE_TAG_STREAM_MAX_CLIENT_REQS)
			return;
		if (tx->active && tx->transfer_id == req.transfer_id && tx->hash == req.hash)
			return;
		tx->req_count++;

		slot = forge_host_get_or_build_cached_bundle(req.hash);
		if (slot < 0 || g_host_tag_cache[slot].missing || !g_host_tag_cache[slot].comp_bytes)
		{
			struct forge_tag_stream_miss_packet miss;
			memset(&miss, 0, sizeof(miss));
			forge_tag_stream_fill_header(&miss.header, _distributed_message_forge_tag_stream, (word)sizeof(miss));
			miss.magic = FORGE_TAG_STREAM_MAGIC;
			miss.op = FORGE_TAG_STREAM_OP_MISS;
			miss.hash = req.hash;
			miss.transfer_id = req.transfer_id;
			network_distributed_server_send_to_machine_reliably(machine_index, &miss, (word)sizeof(miss));
			return;
		}

		tx->active = 1;
		tx->offer_sent = 0;
		tx->cache_slot = slot;
		tx->hash = req.hash;
		tx->transfer_id = req.transfer_id;
		tx->next_chunk = 0;
		return;
	}

	/* 2. Client handling the Host's tag stream response */
	if (hdr.type == _distributed_message_forge_tag_stream)
	{
		uint32_t magic = 0;
		uint8_t op = 0xFFu;

		if (game_connection() != _game_connection_network_client ||
			machine_index != NONE ||
			!config_boolean("network.forge_tag_download") ||
			g_client_tag_rx.strikes >= FORGE_TAG_STREAM_MAX_STRIKES)
		{
			return;
		}
		if (size < sizeof(struct forge_tag_stream_miss_packet))
		{
			forge_client_abort_rx(1);
			return;
		}
		memcpy(&magic, (uint8_t const *)message + sizeof(hdr), sizeof(magic));
		op = *((uint8_t const *)message + sizeof(hdr) + 4u);
		if (magic != FORGE_TAG_STREAM_MAGIC)
		{
			forge_client_abort_rx(1);
			return;
		}

		if (op == FORGE_TAG_STREAM_OP_MISS)
		{
			struct forge_tag_stream_miss_packet miss;
			if (size != sizeof(miss) || g_client_tag_rx.state != _forge_tag_rx_wait_offer)
			{
				forge_client_abort_rx(1);
				return;
			}
			memcpy(&miss, message, sizeof(miss));
			if (miss.hash != g_client_tag_rx.hash || miss.transfer_id != g_client_tag_rx.transfer_id)
			{
				forge_client_abort_rx(1);
				return;
			}
			forge_client_abort_rx(0);
			return;
		}
		else if (op == FORGE_TAG_STREAM_OP_OFFER)
		{
			struct forge_tag_stream_offer_packet offer;
			uint32_t ram_used;

			if (size != sizeof(offer) || g_client_tag_rx.state != _forge_tag_rx_wait_offer)
			{
				forge_client_abort_rx(1);
				return;
			}
			memcpy(&offer, message, sizeof(offer));
			if (!forge_stream_offer_is_valid(&offer, g_client_tag_rx.hash, g_client_tag_rx.transfer_id))
			{
				forge_client_abort_rx(1);
				return;
			}

			ram_used = forge_get_streamed_tag_ram_used();
			if (ram_used > (24u * 1024u * 1024u) ||
				offer.uncompressed_size > (24u * 1024u * 1024u) - ram_used)
			{
				forge_client_abort_rx(0);
				return;
			}

			g_client_tag_rx.comp_buf = (uint8_t *)malloc((size_t)offer.compressed_size);
			if (!g_client_tag_rx.comp_buf)
			{
				forge_client_abort_rx(0);
				return;
			}

			g_client_tag_rx.bundle_format = offer.bundle_format;
			g_client_tag_rx.root_group = offer.root_group;
			g_client_tag_rx.compressed_size = offer.compressed_size;
			g_client_tag_rx.uncompressed_size = offer.uncompressed_size;
			g_client_tag_rx.chunk_count = offer.chunk_count;
			g_client_tag_rx.chunks_received = 0;
			g_client_tag_rx.last_progress_ms = system_milliseconds();
			memcpy(g_client_tag_rx.blake2b_256, offer.blake2b_256, 32u);
			snprintf(g_client_tag_rx.tag_path, sizeof(g_client_tag_rx.tag_path), "%s", offer.tag_path);
			snprintf(g_client_tag_rx.display_name, sizeof(g_client_tag_rx.display_name), "%s", offer.display_name);
			memset(g_client_tag_rx.chunk_seen, 0, sizeof(g_client_tag_rx.chunk_seen));
			g_client_tag_rx.state = _forge_tag_rx_receiving;
			return;
		}
		else if (op == FORGE_TAG_STREAM_OP_CHUNK)
		{
			size_t hdr_prefix = offsetof(struct forge_tag_stream_chunk_packet, payload);
			uint16_t chunk_idx;
			uint32_t pkt_hash, pkt_tid;
			uint16_t payload_len;
			uint32_t dst_off, expected_len;
			uint32_t word_i, bit_m;

			if (g_client_tag_rx.state != _forge_tag_rx_receiving || !g_client_tag_rx.comp_buf ||
				size <= hdr_prefix || size > sizeof(struct forge_tag_stream_chunk_packet))
			{
				forge_client_abort_rx(1);
				return;
			}

			memcpy(&chunk_idx, (uint8_t const *)message + offsetof(struct forge_tag_stream_chunk_packet, chunk_index), sizeof(chunk_idx));
			memcpy(&pkt_hash, (uint8_t const *)message + offsetof(struct forge_tag_stream_chunk_packet, hash), sizeof(pkt_hash));
			memcpy(&pkt_tid, (uint8_t const *)message + offsetof(struct forge_tag_stream_chunk_packet, transfer_id), sizeof(pkt_tid));
			memcpy(&payload_len, (uint8_t const *)message + offsetof(struct forge_tag_stream_chunk_packet, payload_len), sizeof(payload_len));

			if (pkt_hash != g_client_tag_rx.hash ||
				pkt_tid != g_client_tag_rx.transfer_id ||
				chunk_idx >= g_client_tag_rx.chunk_count ||
				payload_len == 0u ||
				payload_len > FORGE_TAG_STREAM_CHUNK_BYTES ||
				(size_t)size != hdr_prefix + (size_t)payload_len)
			{
				forge_client_abort_rx(1);
				return;
			}

			dst_off = (uint32_t)chunk_idx * FORGE_TAG_STREAM_CHUNK_BYTES;
			expected_len = (chunk_idx + 1u == g_client_tag_rx.chunk_count)
				? (g_client_tag_rx.compressed_size - dst_off)
				: FORGE_TAG_STREAM_CHUNK_BYTES;
			if (payload_len != expected_len || dst_off + (uint32_t)payload_len > g_client_tag_rx.compressed_size)
			{
				forge_client_abort_rx(1);
				return;
			}

			word_i = (uint32_t)chunk_idx >> 5;
			bit_m = 1u << ((uint32_t)chunk_idx & 31u);
			if ((g_client_tag_rx.chunk_seen[word_i] & bit_m) != 0u)
			{
				/* Duplicate chunk on a reliable stream is protocol-invalid */
				forge_client_abort_rx(1);
				return;
			}

			g_client_tag_rx.chunk_seen[word_i] |= bit_m;
			memcpy(
				g_client_tag_rx.comp_buf + dst_off,
				(uint8_t const *)message + hdr_prefix,
				(size_t)payload_len);
			g_client_tag_rx.chunks_received++;
			g_client_tag_rx.last_progress_ms = system_milliseconds();

			/* Complete transfer: inflate in RAM, verify BLAKE2b-256, validate schema, hot-swap live objects! */
			if (g_client_tag_rx.chunks_received == g_client_tag_rx.chunk_count)
			{
				uint8_t *uncomp_buf = (uint8_t *)malloc((size_t)g_client_tag_rx.uncompressed_size);
				uint8_t calc_digest[32];
				uint32_t fin_hash = g_client_tag_rx.hash;
				uint8_t fin_fmt = g_client_tag_rx.bundle_format;
				uint32_t fin_grp = g_client_tag_rx.root_group;
				uint32_t fin_uncomp_sz = g_client_tag_rx.uncompressed_size;
				char fin_path[256];
				char fin_disp[64];
				uint32_t actual_grp = fin_grp;
				long installed_def = NONE;
				int diff_bits = 0;
				int bi;

				if (!uncomp_buf)
				{
					forge_client_abort_rx(0);
					return;
				}
				if (!forge_zlib_uncompress_exact(
						g_client_tag_rx.comp_buf,
						g_client_tag_rx.compressed_size,
						uncomp_buf,
						fin_uncomp_sz))
				{
					free(uncomp_buf);
					forge_client_abort_rx(1);
					return;
				}

				p2p_blake2b_256(uncomp_buf, (unsigned long)fin_uncomp_sz, calc_digest);
				for (bi = 0; bi < 32; bi++)
					diff_bits |= (int)(calc_digest[bi] ^ g_client_tag_rx.blake2b_256[bi]);
				if (diff_bits != 0)
				{
					free(uncomp_buf);
					forge_client_abort_rx(1);
					return;
				}

				snprintf(fin_path, sizeof(fin_path), "%s", g_client_tag_rx.tag_path);
				snprintf(fin_disp, sizeof(fin_disp), "%s", g_client_tag_rx.display_name);

				/* Release compressed buffer before installing uncompressed tag structures into session RAM */
				free(g_client_tag_rx.comp_buf);
				g_client_tag_rx.comp_buf = NULL;
				forge_client_mark_hash_settled(fin_hash);
				g_client_tag_rx.state = _forge_tag_rx_idle;
				g_client_tag_rx.hash = 0;
				g_client_tag_rx.transfer_id = 0;

				installed_def = forge_install_ephemeral_tag_bundle(
					fin_hash,
					fin_fmt,
					fin_grp,
					fin_path,
					fin_disp,
					uncomp_buf,
					fin_uncomp_sz,
					&actual_grp);
				free(uncomp_buf);

				if (installed_def == NONE)
				{
					g_client_tag_rx.strikes++;
					if (g_client_tag_rx.strikes >= FORGE_TAG_STREAM_MAX_STRIKES)
						g_client_tag_rx.queue_count = 0;
					return;
				}

				g_client_tag_rx.downloaded_count++;
				forge_client_materialize_streamed_hash(fin_hash, fin_grp, fin_path, installed_def, actual_grp);
			}
			return;
		}
		else
		{
			forge_client_abort_rx(1);
			return;
		}
	}
}

static void forge_tag_stream_tick(unsigned long now)
{
	if (game_connection() == _game_connection_network_client)
	{
		if (!config_boolean("network.forge_tag_download") ||
			g_client_tag_rx.strikes >= FORGE_TAG_STREAM_MAX_STRIKES)
		{
			if (g_client_tag_rx.state != _forge_tag_rx_idle)
				forge_client_abort_rx(0);
			g_client_tag_rx.queue_count = 0;
			return;
		}

		if (g_client_tag_rx.state != _forge_tag_rx_idle)
		{
			if (now - g_client_tag_rx.last_progress_ms > FORGE_TAG_STREAM_STALL_TIMEOUT_MS)
				forge_client_abort_rx(0);
			else
				return;
		}

		while (g_client_tag_rx.queue_count > 0 &&
			   g_client_tag_rx.downloaded_count < FORGE_TAG_STREAM_MAX_SESSION_TAGS)
		{
			uint32_t next_hash = g_client_tag_rx.queue[0];
			struct forge_tag_request_packet req;
			uint32_t tid;
			int q;

			for (q = 1; q < g_client_tag_rx.queue_count; q++)
				g_client_tag_rx.queue[q - 1] = g_client_tag_rx.queue[q];
			g_client_tag_rx.queue_count--;

			if (forge_client_is_hash_settled(next_hash))
				continue;

			g_client_tag_rx.next_transfer_seq++;
			tid = (g_client_tag_rx.next_transfer_seq * 2654435761u) ^ (uint32_t)now ^ next_hash;
			if (tid == 0u)
				tid = 1u;

			memset(&req, 0, sizeof(req));
			forge_tag_stream_fill_header(&req.header, _distributed_message_forge_tag_request, (word)sizeof(req));
			req.magic = FORGE_TAG_REQUEST_MAGIC;
			req.hash = next_hash;
			req.transfer_id = tid;

			if (!network_distributed_client_send_reliably(&req, (word)sizeof(req)))
			{
				/* Reliable stream busy: put back at head of queue and retry next tick */
				for (q = g_client_tag_rx.queue_count; q > 0; q--)
					g_client_tag_rx.queue[q] = g_client_tag_rx.queue[q - 1];
				g_client_tag_rx.queue[0] = next_hash;
				g_client_tag_rx.queue_count++;
				break;
			}

			g_client_tag_rx.state = _forge_tag_rx_wait_offer;
			g_client_tag_rx.hash = next_hash;
			g_client_tag_rx.transfer_id = tid;
			g_client_tag_rx.last_progress_ms = now;
			break;
		}
	}
	else if (game_connection() == _game_connection_network_server)
	{
		int m;
		for (m = 0; m < FORGE_MAX_CLIENT_MACHINES; m++)
		{
			struct forge_host_client_tag_tx *tx = &g_host_tag_tx[m];
			struct forge_host_cached_tag_bundle const *cb;
			int sent_this_tick = 0;

			if (!tx->active)
				continue;
			if (tx->cache_slot < 0 || tx->cache_slot >= FORGE_TAG_STREAM_MAX_SESSION_TAGS)
			{
				tx->active = 0;
				continue;
			}
			cb = &g_host_tag_cache[tx->cache_slot];
			if (!cb->valid || cb->missing || !cb->comp_bytes)
			{
				tx->active = 0;
				continue;
			}

			if (!tx->offer_sent)
			{
				struct forge_tag_stream_offer_packet offer;
				memset(&offer, 0, sizeof(offer));
				forge_tag_stream_fill_header(&offer.header, _distributed_message_forge_tag_stream, (word)sizeof(offer));
				offer.magic = FORGE_TAG_STREAM_MAGIC;
				offer.op = FORGE_TAG_STREAM_OP_OFFER;
				offer.bundle_format = cb->bundle_format;
				offer.chunk_count = cb->chunk_count;
				offer.hash = tx->hash;
				offer.transfer_id = tx->transfer_id;
				offer.root_group = cb->root_group;
				offer.compressed_size = cb->compressed_size;
				offer.uncompressed_size = cb->uncompressed_size;
				memcpy(offer.blake2b_256, cb->blake2b_256, 32u);
				snprintf(offer.tag_path, sizeof(offer.tag_path), "%s", cb->tag_path);
				snprintf(offer.display_name, sizeof(offer.display_name), "%s", cb->display_name);

				if (!network_distributed_server_send_to_machine_reliably(m, &offer, (word)sizeof(offer)))
					continue;
				tx->offer_sent = 1;
			}

			while (tx->next_chunk < cb->chunk_count && sent_this_tick < FORGE_TAG_STREAM_CHUNKS_PER_TICK)
			{
				struct forge_tag_stream_chunk_packet chunk;
				uint32_t src_off = (uint32_t)tx->next_chunk * FORGE_TAG_STREAM_CHUNK_BYTES;
				uint32_t rem = cb->compressed_size - src_off;
				uint16_t plen = (rem > FORGE_TAG_STREAM_CHUNK_BYTES) ? (uint16_t)FORGE_TAG_STREAM_CHUNK_BYTES : (uint16_t)rem;
				word wire_sz = (word)(offsetof(struct forge_tag_stream_chunk_packet, payload) + (size_t)plen);

				memset(&chunk, 0, offsetof(struct forge_tag_stream_chunk_packet, payload));
				forge_tag_stream_fill_header(&chunk.header, _distributed_message_forge_tag_stream, wire_sz);
				chunk.magic = FORGE_TAG_STREAM_MAGIC;
				chunk.op = FORGE_TAG_STREAM_OP_CHUNK;
				chunk.chunk_index = tx->next_chunk;
				chunk.hash = tx->hash;
				chunk.transfer_id = tx->transfer_id;
				chunk.payload_len = plen;
				memcpy(chunk.payload, cb->comp_bytes + src_off, (size_t)plen);

				if (!network_distributed_server_send_to_machine_reliably(m, &chunk, wire_sz))
					break;
				tx->next_chunk++;
				sent_this_tick++;
			}

			if (tx->next_chunk >= cb->chunk_count)
				tx->active = 0;
		}
	}
}

int forge_network_get_tag_download_status(char *out_text, size_t out_sz)
{
	if (!out_text || out_sz < 8)
		return 0;
	out_text[0] = '\0';

	if (game_connection() != _game_connection_network_client)
		return 0;

	if (g_client_tag_rx.state == _forge_tag_rx_receiving && g_client_tag_rx.chunk_count > 0)
	{
		int pct = (int)(((uint32_t)g_client_tag_rx.chunks_received * 100u) / (uint32_t)g_client_tag_rx.chunk_count);
		char const *label = g_client_tag_rx.display_name[0] ? g_client_tag_rx.display_name : g_client_tag_rx.tag_path;
		if (g_client_tag_rx.queue_count > 0)
		{
			snprintf(out_text, out_sz, "STREAMING CUSTOM TAG: %s (%d%%, +%d queued)",
				label, pct, g_client_tag_rx.queue_count);
		}
		else
		{
			snprintf(out_text, out_sz, "STREAMING CUSTOM TAG: %s (%d%%)", label, pct);
		}
		return 1;
	}
	if (g_client_tag_rx.state == _forge_tag_rx_wait_offer || g_client_tag_rx.queue_count > 0)
	{
		snprintf(out_text, out_sz, "REQUESTING CUSTOM TAG FROM HOST... (%d remaining)",
			g_client_tag_rx.queue_count + (g_client_tag_rx.state == _forge_tag_rx_wait_offer ? 1 : 0));
		return 1;
	}
	return 0;
}
