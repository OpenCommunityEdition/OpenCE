/*
FORGE_TAG_STREAM.C (test)

Zero-trust session-RAM custom tag streaming (port/linux/game/forge_network.c and
port/linux/game/forge_runtime_tags.c): solicited single-flight requests over the
reliable stream, host bundle caching and chunk pacing, offer/path/group/size
admission, duplicate/out-of-bounds chunk rejection, BLAKE2b-256 verification
prior to tag parsing, base-map anti-shadowing, session RAM caps, 3-strike host
cutoff, opt-out, and proxy-to-live-object materialization.
*/

#ifdef __clang__
#pragma clang diagnostic ignored "-Wmultichar"
#pragma clang diagnostic ignored "-Wfour-char-constants"
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wmultichar"
#endif

#include "harness.h"
#include <stddef.h>
#include <stdint.h>

typedef unsigned short word;

#include "forge_engine.h"

#define _game_connection_local 0
#define _game_connection_network_client 1
#define _game_connection_network_server 2

struct distributed_message_header
{
	word header;
	byte type;
	byte count;
	long game_time;
	long checkpoint_epoch;
};

struct tag_reference
{
	uint32_t group_tag;
	char *name;
	long name_length;
	long index;
};

struct tag_block
{
	long count;
	void *address;
	void *definition;
};

struct unit_definition
{
	struct
	{
		struct tag_block initial_weapons;
	} unit;
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
	float change_colors[4][3];
	byte region_permutations[8];
};

#include "config.inc"

/* ---------- Fake world state */

int g_forge_session_enabled = 1;
struct forge_runtime_state g_forge;

static unsigned long fake_now_ms = 1000;
static int fake_in_progress = 1;
static int fake_connection = _game_connection_network_client;
static int fake_opt_in_download = 1;

static uint32_t s_streamed_phys_bytes = 0;
static struct { int count; } s_dyn_tags;
static struct { int count; } s_forge_phys_allocs;

static long g_net_obj_def_index[FORGE_MAX_TRACKED_NET_OBJECTS];
static int32_t g_net_obj_live_index[FORGE_MAX_TRACKED_NET_OBJECTS];
static uint8_t g_net_obj_is_held[FORGE_MAX_TRACKED_NET_OBJECTS];
static uint8_t g_net_obj_has_deferred_create[FORGE_MAX_TRACKED_NET_OBJECTS];
static struct forge_distributed_object_change g_net_obj_deferred_create[FORGE_MAX_TRACKED_NET_OBJECTS];

/* Captured wire queues (Client -> Host and Host -> Client) */
#define MAX_WIRE_QUEUE 64
struct wire_packet
{
	long machine_index;
	word size;
	uint8_t bytes[2048];
};

static struct wire_packet c2h_queue[MAX_WIRE_QUEUE];
static int c2h_count = 0;
static int c2h_backpressure = 0;

static struct wire_packet h2c_queue[MAX_WIRE_QUEUE];
static int h2c_count = 0;
static int h2c_backpressure = 0;
static long fake_host_backlog = 0;

/* Fake host custom tag vault */
static uint32_t fake_host_hash = 0;
static uint8_t fake_host_format = 1;
static uint32_t fake_host_group = 0;
static char fake_host_path[256];
static char fake_host_disp[64];
static uint8_t fake_host_raw[32768];
static uint32_t fake_host_raw_len = 0;
static int fake_export_calls = 0;

/* Fake client tag table & parser counters */
static int fake_faber_install_calls = 0;
static int fake_xbox_install_calls = 0;
static int fake_parser_reject = 0;
static uint32_t fake_base_map_tag_count = 10;
static long fake_existing_tag_handle = NONE;
static uint32_t fake_existing_tag_group = 0;
static char fake_existing_tag_path[256];
static long fake_next_installed_handle = (long)0xE1230020UL;
static uint32_t fake_installed_actual_group = 0;

/* Fake unit initial weapon & object spawning */
static struct tag_reference fake_weap_ref;
static struct unit_definition fake_unit_def;
static int fake_local_spawn_calls = 0;
static int32_t fake_last_spawned_live_idx = -1;
static long fake_last_spawned_def_idx = NONE;
static int fake_replayed_changes = 0;
static long fake_replayed_defs[4];
static long fake_live_objects_by_abs[64];
static int fake_configured_objects = 0;

static unsigned long system_milliseconds(void) { return fake_now_ms; }
static boolean game_in_progress(void) { return fake_in_progress ? TRUE : FALSE; }
static long game_time_get(void) { return 30; }
static short game_connection(void) { return (short)fake_connection; }

static int config_boolean(const char *name)
{
	if (strcmp(name, "network.forge_tag_download") == 0)
		return fake_opt_in_download;
	return 1;
}

static void build_message_header(word *hdr, word size, int cls, int flags)
{
	(void)cls;
	(void)flags;
	*hdr = size;
}

static boolean network_distributed_client_send_reliably(void *message, word size)
{
	if (c2h_backpressure || c2h_count >= MAX_WIRE_QUEUE || size > sizeof(c2h_queue[0].bytes))
		return FALSE;
	c2h_queue[c2h_count].machine_index = 0;
	c2h_queue[c2h_count].size = size;
	memcpy(c2h_queue[c2h_count].bytes, message, size);
	c2h_count++;
	return TRUE;
}

static boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size)
{
	if (h2c_backpressure || h2c_count >= MAX_WIRE_QUEUE || size > sizeof(h2c_queue[0].bytes))
		return FALSE;
	h2c_queue[h2c_count].machine_index = machine_index;
	h2c_queue[h2c_count].size = size;
	memcpy(h2c_queue[h2c_count].bytes, message, size);
	h2c_count++;
	return TRUE;
}

static long network_distributed_server_machine_reliable_backlog(long machine_index)
{
	(void)machine_index;
	return fake_host_backlog;
}

/* Deterministic 32-byte digest over every byte + length for wire integrity checks */
static void p2p_blake2b_256(void const *data, unsigned long len, uint8_t out[32])
{
	uint8_t const *p = (uint8_t const *)data;
	uint32_t s[8] = {
		0x6A09E667u ^ (uint32_t)len, 0xBB67AE85u, 0x3C6EF372u, 0xA54FF53Au,
		0x510E527Fu, 0x9B05688Cu, 0x1F83D9ABu, 0x5BE0CD19u
	};
	unsigned long i;
	int k;

	for (i = 0; i < len; i++)
	{
		uint32_t v = p[i];
		int lane = (int)(i & 7u);
		s[lane] = ((s[lane] << 5) | (s[lane] >> 27)) ^ (v * 2246822519u + (uint32_t)i);
		s[(lane + 3) & 7] += s[lane] ^ 0x9E3779B9u;
	}
	for (k = 0; k < 8; k++)
		memcpy(out + k * 4, &s[k], 4);
}

/* Framed codec ("FZLB" + u32 exact_len + inverted payload) exercising exact-size inflate */
int forge_zlib_compress_buffer(uint8_t const *src, uint32_t src_len, uint8_t **out_dst, uint32_t *out_dst_len)
{
	uint8_t *dst;
	uint32_t i;

	if (!src || src_len == 0 || !out_dst || !out_dst_len)
		return 0;
	dst = (uint8_t *)malloc((size_t)src_len + 8u);
	if (!dst)
		return 0;
	memcpy(dst, "FZLB", 4);
	memcpy(dst + 4, &src_len, 4);
	for (i = 0; i < src_len; i++)
		dst[8u + i] = (uint8_t)(src[i] ^ 0x5Au);
	*out_dst = dst;
	*out_dst_len = src_len + 8u;
	return 1;
}

int forge_zlib_uncompress_exact(uint8_t const *src, uint32_t src_len, uint8_t *dst, uint32_t exact_dst_len)
{
	uint32_t encoded_len = 0;
	uint32_t i;

	if (!src || src_len <= 8u || !dst || exact_dst_len == 0)
		return 0;
	if (memcmp(src, "FZLB", 4) != 0)
		return 0;
	memcpy(&encoded_len, src + 4, 4);
	if (encoded_len != exact_dst_len || src_len != encoded_len + 8u)
		return 0;
	for (i = 0; i < exact_dst_len; i++)
		dst[i] = (uint8_t)(src[8u + i] ^ 0x5Au);
	return 1;
}

int forge_export_custom_tag_bundle(
	uint32_t hash,
	uint8_t *out_format,
	uint32_t *out_group_tag,
	char *out_tag_path,
	size_t path_sz,
	char *out_display_name,
	size_t disp_sz,
	uint8_t **out_raw_bytes,
	uint32_t *out_raw_size)
{
	uint8_t *buf;
	fake_export_calls++;
	if (!fake_host_hash || hash != fake_host_hash || fake_host_raw_len == 0)
		return 0;
	buf = (uint8_t *)malloc(fake_host_raw_len);
	if (!buf)
		return 0;
	memcpy(buf, fake_host_raw, fake_host_raw_len);
	*out_format = fake_host_format;
	*out_group_tag = fake_host_group;
	snprintf(out_tag_path, path_sz, "%s", fake_host_path);
	snprintf(out_display_name, disp_sz, "%s", fake_host_disp);
	*out_raw_bytes = buf;
	*out_raw_size = fake_host_raw_len;
	return 1;
}

static long tag_loaded(long group_tag, char const *name)
{
	if (fake_existing_tag_handle != NONE &&
		(uint32_t)group_tag == fake_existing_tag_group &&
		_stricmp(name, fake_existing_tag_path) == 0)
	{
		return fake_existing_tag_handle;
	}
	return NONE;
}

uint32_t forge_get_base_map_tag_count(void) { return fake_base_map_tag_count; }
static long tag_get_group_tag(long hdl) { (void)hdl; return (long)fake_existing_tag_group; }

static long install_faber_bundle_from_source(
	long file_handle,
	uint8_t const *mem_data,
	unsigned long bundle_sz,
	uint32_t req_group,
	char const *req_path,
	uint32_t *out_actual_group,
	int strict_untrusted)
{
	(void)file_handle;
	CHECK(strict_untrusted == 1, "streamed FABB bundle must use strict_untrusted=1");
	CHECK(mem_data != NULL && bundle_sz >= TAG_BUNDLE_HEADER_BYTES, "streamed FABB bundle must be in RAM");
	fake_faber_install_calls++;
	if (fake_parser_reject)
		return NONE;
	if (out_actual_group)
		*out_actual_group = fake_installed_actual_group ? fake_installed_actual_group : req_group;
	(void)req_path;
	return fake_next_installed_handle;
}

static long load_native_xbox_tag_from_source(
	char const *cache_map_path,
	uint8_t const *mem_image,
	uint32_t mem_image_size,
	uint32_t req_group,
	char const *req_path,
	uint32_t *out_actual_group,
	int strict_untrusted)
{
	CHECK(cache_map_path == NULL, "streamed Xbox bundle must never pass a disk path");
	CHECK(strict_untrusted == 1, "streamed Xbox bundle must use strict_untrusted=1");
	CHECK(mem_image != NULL && mem_image_size >= TAG_BUNDLE_HEADER_BYTES, "streamed Xbox bundle must be in RAM");
	fake_xbox_install_calls++;
	if (fake_parser_reject)
		return NONE;
	if (out_actual_group)
		*out_actual_group = fake_installed_actual_group ? fake_installed_actual_group : req_group;
	(void)req_path;
	return fake_next_installed_handle;
}

static int is_forge_door_tag(uint32_t grp, char const *path) { (void)grp; (void)path; return 0; }
static int is_forge_light_beacon_emitter_tag(uint32_t grp, char const *path) { (void)grp; (void)path; return 0; }

static void add_catalog_entry(uint32_t group_tag, uint8_t category, uint8_t source_cache_id, char const *tag_path, char const *label)
{
	struct forge_catalog_entry *e;
	if (g_forge.catalog_count >= FORGE_MAX_CATALOG_ENTRIES)
		return;
	e = &g_forge.catalog[g_forge.catalog_count++];
	memset(e, 0, sizeof(*e));
	e->group_tag = group_tag;
	e->category = category;
	e->source_cache_id = source_cache_id;
	snprintf(e->tag_path, sizeof(e->tag_path), "%s", tag_path);
	snprintf(e->display_name, sizeof(e->display_name), "%s", label);
}

static void *object_try_and_get_and_verify_type(long obj_idx, unsigned long mask)
{
	(void)obj_idx;
	(void)mask;
	return NULL;
}

static void object_delete(long obj_idx) { (void)obj_idx; }

static void *tag_get(long group_tag, long def_idx)
{
	if (group_tag == 'unit' && def_idx == fake_next_installed_handle)
		return &fake_unit_def;
	return NULL;
}

static long forge_find_live_object_by_abs_index(int obj_abs)
{
	if (obj_abs >= 0 && obj_abs < 64)
		return fake_live_objects_by_abs[obj_abs];
	return NONE;
}

static void orig_network_objects_handle_changes(void const *entries, short count)
{
	struct forge_distributed_object_change const *ch = (struct forge_distributed_object_change const *)entries;
	if (count == 1 && ch)
	{
		int abs_i = (int)DATUM_INDEX_TO_ABSOLUTE_INDEX(ch->object_index);
		if (fake_replayed_changes < 4)
			fake_replayed_defs[fake_replayed_changes] = ch->definition_index;
		fake_replayed_changes++;
		if (abs_i >= 0 && abs_i < 64)
			fake_live_objects_by_abs[abs_i] = ((long)0x80010000UL) | abs_i;
	}
}

static void forge_client_configure_linked_object(int32_t live_idx, long obj_idx, int is_held)
{
	(void)is_held;
	fake_configured_objects++;
	if (live_idx >= 0 && live_idx < g_forge.live_instance_count)
		g_forge.live_instances[live_idx].object_index = obj_idx;
}

void forge_3d_move_instance_ext(int32_t idx, float x, float y, float z, float yaw, float pitch, float roll, int is_held, int sync)
{
	(void)idx; (void)x; (void)y; (void)z; (void)yaw; (void)pitch; (void)roll; (void)is_held; (void)sync;
}

long forge_create_engine_object_for_instance(int32_t live_idx, long def_idx, int is_held)
{
	(void)is_held;
	fake_local_spawn_calls++;
	fake_last_spawned_live_idx = live_idx;
	fake_last_spawned_def_idx = def_idx;
	if (live_idx >= 0 && live_idx < g_forge.live_instance_count)
		g_forge.live_instances[live_idx].object_index = (long)0x80020010UL;
	return (long)0x80020010UL;
}

#include "under_test.inc"

/* ---------- Test helpers */

static void reset_world(void)
{
	int i;
	memset(&g_forge, 0, sizeof(g_forge));
	g_forge.initialized = 1;
	g_forge_session_enabled = 1;
	fake_now_ms = 1000;
	fake_in_progress = 1;
	fake_connection = _game_connection_network_client;
	fake_opt_in_download = 1;

	s_streamed_phys_bytes = 0;
	s_dyn_tags.count = 0;
	s_forge_phys_allocs.count = 0;

	for (i = 0; i < FORGE_MAX_TRACKED_NET_OBJECTS; i++)
	{
		g_net_obj_def_index[i] = NONE;
		g_net_obj_live_index[i] = -1;
		g_net_obj_is_held[i] = 0;
		g_net_obj_has_deferred_create[i] = 0;
	}
	for (i = 0; i < 64; i++)
		fake_live_objects_by_abs[i] = NONE;

	c2h_count = 0;
	c2h_backpressure = 0;
	h2c_count = 0;
	h2c_backpressure = 0;
	fake_host_backlog = 0;

	fake_host_hash = 0;
	fake_host_format = 1;
	fake_host_group = 0;
	fake_host_path[0] = '\0';
	fake_host_disp[0] = '\0';
	fake_host_raw_len = 0;
	fake_export_calls = 0;

	fake_faber_install_calls = 0;
	fake_xbox_install_calls = 0;
	fake_parser_reject = 0;
	fake_base_map_tag_count = 10;
	fake_existing_tag_handle = NONE;
	fake_existing_tag_group = 0;
	fake_existing_tag_path[0] = '\0';
	fake_next_installed_handle = (long)0xE1230020UL;
	fake_installed_actual_group = 0;

	memset(&fake_weap_ref, 0, sizeof(fake_weap_ref));
	memset(&fake_unit_def, 0, sizeof(fake_unit_def));
	fake_local_spawn_calls = 0;
	fake_last_spawned_live_idx = -1;
	fake_last_spawned_def_idx = NONE;
	fake_replayed_changes = 0;
	fake_configured_objects = 0;

	forge_tag_stream_reset_state();
}

static uint32_t stage_host_bundle(uint8_t fmt, uint32_t grp, char const *path, char const *disp, uint32_t raw_len)
{
	uint32_t i;
	fake_host_format = fmt;
	fake_host_group = grp;
	snprintf(fake_host_path, sizeof(fake_host_path), "%s", path);
	snprintf(fake_host_disp, sizeof(fake_host_disp), "%s", disp);
	fake_host_raw_len = raw_len;
	for (i = 0; i < raw_len; i++)
		fake_host_raw[i] = (uint8_t)((i * 31u + 7u) & 0xFFu);
	fake_host_hash = forge_compute_catalog_hash(grp, path);
	return fake_host_hash;
}

static void pump_client_to_host_stream(long client_machine_index)
{
	int i;
	int prev_conn = fake_connection;
	fake_connection = _game_connection_network_server;
	for (i = 0; i < c2h_count; i++)
	{
		CHECK(forge_tag_stream_handles_message(c2h_queue[i].bytes, c2h_queue[i].size), "host must recognize request");
		forge_tag_stream_handle_message(client_machine_index, c2h_queue[i].bytes, c2h_queue[i].size, 1);
	}
	c2h_count = 0;
	forge_tag_stream_tick(fake_now_ms);
	fake_connection = prev_conn;
}

static void pump_host_to_client_stream(void)
{
	int i;
	int prev_conn = fake_connection;
	fake_connection = _game_connection_network_client;
	for (i = 0; i < h2c_count; i++)
	{
		CHECK(forge_tag_stream_handles_message(h2c_queue[i].bytes, h2c_queue[i].size), "client must recognize stream");
		forge_tag_stream_handle_message(NONE, h2c_queue[i].bytes, h2c_queue[i].size, 1);
	}
	h2c_count = 0;
	fake_connection = prev_conn;
}

static struct forge_tag_stream_offer_packet make_valid_offer(uint32_t hash, uint32_t tid, uint32_t grp, char const *path)
{
	struct forge_tag_stream_offer_packet offer;
	uint8_t dummy[64];
	uint8_t *comp = NULL;
	uint32_t comp_sz = 0;

	memset(dummy, 0x42, sizeof(dummy));
	CHECK(forge_zlib_compress_buffer(dummy, sizeof(dummy), &comp, &comp_sz) == 1, "compress dummy");

	memset(&offer, 0, sizeof(offer));
	forge_tag_stream_fill_header(&offer.header, _distributed_message_forge_tag_stream, (word)sizeof(offer));
	offer.magic = FORGE_TAG_STREAM_MAGIC;
	offer.op = FORGE_TAG_STREAM_OP_OFFER;
	offer.bundle_format = 1u;
	offer.hash = hash;
	offer.transfer_id = tid;
	offer.root_group = grp;
	offer.compressed_size = comp_sz;
	offer.uncompressed_size = sizeof(dummy);
	offer.chunk_count = (uint16_t)((comp_sz + FORGE_TAG_STREAM_CHUNK_BYTES - 1u) / FORGE_TAG_STREAM_CHUNK_BYTES);
	p2p_blake2b_256(dummy, sizeof(dummy), offer.blake2b_256);
	snprintf(offer.tag_path, sizeof(offer.tag_path), "%s", path);
	snprintf(offer.display_name, sizeof(offer.display_name), "Test Crate");
	free(comp);
	return offer;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	/* 1. Happy-path multi-chunk scenery transfer + host bundle cache reuse */
	CASE("happy-scenery-transfer")
	{
		uint32_t h;
		int32_t slot;
		char hud[128];

		reset_world();
		h = stage_host_bundle(1, 'scen', "custom\\crates\\heavy_crate", "[C] Heavy Crate", 4000u);

		slot = forge_client_upsert_stream_proxy_instance(2, h, -1, -1, 0, 12.5f, -4.0f, 3.0f, 1.2f, 0.0f, 0.0f, -1);
		CHECK(slot == 2, "proxy placed at host_live_idx 2");
		CHECK((g_forge.live_instances[2].pad_inst & 0x40u) != 0, "proxy marker bit 0x40 set");
		CHECK(g_forge.live_instances[2].object_index == NONE, "proxy has no engine object yet");

		forge_client_enqueue_tag_download(h);
		CHECK(g_client_tag_rx.queue_count == 1, "hash queued once");
		forge_client_enqueue_tag_download(h);
		CHECK(g_client_tag_rx.queue_count == 1, "duplicate hash deduplicated in queue");

		/* Client tick sends solicited FTRQ */
		forge_tag_stream_tick(fake_now_ms);
		CHECK(c2h_count == 1, "client sent 1 FTRQ packet");
		CHECK(g_client_tag_rx.state == _forge_tag_rx_wait_offer, "client waiting for offer");
		CHECK(forge_network_get_tag_download_status(hud, sizeof(hud)) == 1, "HUD status active while requesting");

		/* Host processes FTRQ and sends OFFER + 4 chunks (4008 compressed bytes / 1280 = 4 chunks) */
		pump_client_to_host_stream(0);
		CHECK(fake_export_calls == 1, "host exported bundle once");
		CHECK(h2c_count == 5, "host sent 1 offer + 4 chunks in single paced tick (<= 6 chunks/tick)");

		/* Deliver only OFFER + first 2 chunks first to check mid-stream HUD status */
		CHECK(forge_tag_stream_handles_message(h2c_queue[0].bytes, h2c_queue[0].size), "offer recognized");
		forge_tag_stream_handle_message(NONE, h2c_queue[0].bytes, h2c_queue[0].size, 1);
		forge_tag_stream_handle_message(NONE, h2c_queue[1].bytes, h2c_queue[1].size, 1);
		forge_tag_stream_handle_message(NONE, h2c_queue[2].bytes, h2c_queue[2].size, 1);
		CHECK(g_client_tag_rx.state == _forge_tag_rx_receiving, "client receiving chunks");
		CHECK(forge_network_get_tag_download_status(hud, sizeof(hud)) == 1 && strstr(hud, "50%") != NULL,
			"HUD status reports 50%% progress: %s", hud);

		/* Deliver final 2 chunks -> inflates, verifies BLAKE2b-256, installs in RAM, hot-swaps proxy */
		forge_tag_stream_handle_message(NONE, h2c_queue[3].bytes, h2c_queue[3].size, 1);
		forge_tag_stream_handle_message(NONE, h2c_queue[4].bytes, h2c_queue[4].size, 1);
		h2c_count = 0;

		CHECK(g_client_tag_rx.state == _forge_tag_rx_idle, "client returned to idle");
		CHECK(g_client_tag_rx.strikes == 0, "zero strikes on valid transfer");
		CHECK(g_client_tag_rx.downloaded_count == 1, "downloaded_count incremented");
		CHECK(fake_faber_install_calls == 1, "FABB bundle installed in RAM");
		CHECK(forge_get_streamed_tag_ram_used() == 4000u, "session RAM accounting updated");
		CHECK((g_forge.live_instances[2].pad_inst & 0x40u) == 0, "proxy flag cleared");
		CHECK(strcmp(g_forge.live_instances[2].tag_path, "custom\\crates\\heavy_crate") == 0, "instance path updated");
		CHECK(g_forge.live_instances[2].source_cache_id == 4, "instance marked session-streamed (4)");
		CHECK(fake_local_spawn_calls == 1 && fake_last_spawned_live_idx == 2 &&
			fake_last_spawned_def_idx == fake_next_installed_handle, "live engine object spawned");
		CHECK(g_forge.catalog_count == 1 && g_forge.catalog[0].source_cache_id == 4, "catalog registered as session-streamed");
		CHECK(strcmp(g_forge.catalog[0].display_name, "[C] Heavy Crate") == 0, "clean single [C] prefix in catalog");

		/* Second client machine requesting the same hash reuses host's cached compressed bundle */
		{
			struct forge_tag_request_packet req2;
			memset(&req2, 0, sizeof(req2));
			forge_tag_stream_fill_header(&req2.header, _distributed_message_forge_tag_request, (word)sizeof(req2));
			req2.magic = FORGE_TAG_REQUEST_MAGIC;
			req2.hash = h;
			req2.transfer_id = 0xCAFEBABEu;
			fake_connection = _game_connection_network_server;
			forge_tag_stream_handle_message(1, &req2, (word)sizeof(req2), 1);
			forge_tag_stream_tick(fake_now_ms);
			CHECK(fake_export_calls == 1, "host reused cached compressed bundle without re-reading disk");
			CHECK(h2c_count == 5, "second client received cached offer + chunks");
		}
		return 0;
	}

	/* 2. Deferred network vehicle + initial weapon replay upon tag completion */
	CASE("happy-deferred-vehicle-transfer")
	{
		uint32_t h;
		reset_world();
		h = stage_host_bundle(2, 'vehi', "vehicles\\pelican\\pelican_mp", "Pelican", 1500u);

		fake_weap_ref.group_tag = 'weap';
		fake_weap_ref.index = (long)0xE1240021UL;
		fake_unit_def.unit.initial_weapons.count = 1;
		fake_unit_def.unit.initial_weapons.address = &fake_weap_ref;

		/* Simulate deferred network object creation for vehicle (abs 12) and turret weapon (abs 13) */
		g_net_obj_has_deferred_create[12] = 1;
		g_net_obj_deferred_create[12].object_index = ((long)0x80010000UL) | 12;
		g_net_obj_has_deferred_create[13] = 1;
		g_net_obj_deferred_create[13].object_index = ((long)0x80010000UL) | 13;

		forge_client_upsert_stream_proxy_instance(5, h, 12, 13, 1, 1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0);
		forge_client_enqueue_tag_download(h);
		forge_tag_stream_tick(fake_now_ms);
		pump_client_to_host_stream(0);
		pump_host_to_client_stream();

		CHECK(fake_xbox_install_calls == 1, "FXT1 native Xbox bundle installed in RAM without temp file");
		CHECK(fake_replayed_changes == 2, "replayed deferred vehicle + initial weapon creations");
		CHECK(fake_replayed_defs[0] == fake_next_installed_handle, "vehicle got streamed def_idx");
		CHECK(fake_replayed_defs[1] == fake_weap_ref.index, "turret got unit initial_weapons[0] def_idx");
		CHECK(fake_configured_objects == 1, "linked vehicle configured");
		CHECK(g_forge.live_instances[5].object_index == (((long)0x80010000UL) | 12), "live instance linked to network object");
		return 0;
	}

	/* 3. Datagram / batch spoof (from_stream == 0) must be completely ignored */
	CASE("datagram-spoof-ignored")
	{
		uint32_t h;
		struct forge_tag_request_packet req;
		struct forge_tag_stream_offer_packet offer;

		reset_world();
		h = stage_host_bundle(1, 'scen', "custom\\wall\\wall_a", "Wall A", 512u);

		/* Spoofed unreliable FTRQ to Host */
		memset(&req, 0, sizeof(req));
		forge_tag_stream_fill_header(&req.header, _distributed_message_forge_tag_request, (word)sizeof(req));
		req.magic = FORGE_TAG_REQUEST_MAGIC;
		req.hash = h;
		req.transfer_id = 12345u;

		fake_connection = _game_connection_network_server;
		forge_tag_stream_handle_message(0, &req, (word)sizeof(req), 0);
		forge_tag_stream_tick(fake_now_ms);
		CHECK(fake_export_calls == 0 && h2c_count == 0, "unreliable datagram FTRQ ignored by host");

		/* Spoofed unreliable OFFER to Client while waiting for a real offer */
		fake_connection = _game_connection_network_client;
		forge_client_enqueue_tag_download(h);
		forge_tag_stream_tick(fake_now_ms);
		CHECK(g_client_tag_rx.state == _forge_tag_rx_wait_offer, "waiting for solicited offer");

		offer = make_valid_offer(h, g_client_tag_rx.transfer_id, 'scen', "custom\\wall\\wall_a");
		forge_tag_stream_handle_message(NONE, &offer, (word)sizeof(offer), 0);
		CHECK(g_client_tag_rx.state == _forge_tag_rx_wait_offer, "unreliable datagram OFFER ignored");
		CHECK(g_client_tag_rx.comp_buf == NULL && g_client_tag_rx.strikes == 0, "no allocation or strike from datagram");
		return 0;
	}

	/* 4. Unsolicited packets and mismatched transfer_id rejection */
	CASE("unsolicited-and-wrong-token")
	{
		uint32_t h;
		struct forge_tag_stream_offer_packet offer;

		reset_world();
		h = forge_compute_catalog_hash('scen', "custom\\prop\\pillar");
		offer = make_valid_offer(h, 999u, 'scen', "custom\\prop\\pillar");

		/* Unsolicited OFFER when client is idle */
		forge_tag_stream_handle_message(NONE, &offer, (word)sizeof(offer), 1);
		CHECK(g_client_tag_rx.strikes == 1, "unsolicited offer adds strike");
		CHECK(g_client_tag_rx.state == _forge_tag_rx_idle && g_client_tag_rx.comp_buf == NULL, "stays idle");

		/* Solicited request, but Host replies with wrong transfer_id */
		forge_client_enqueue_tag_download(h);
		forge_tag_stream_tick(fake_now_ms);
		CHECK(g_client_tag_rx.state == _forge_tag_rx_wait_offer, "waiting for offer");

		offer = make_valid_offer(h, g_client_tag_rx.transfer_id ^ 0x55AAu, 'scen', "custom\\prop\\pillar");
		forge_tag_stream_handle_message(NONE, &offer, (word)sizeof(offer), 1);
		CHECK(g_client_tag_rx.strikes == 2, "wrong transfer_id adds strike");
		CHECK(g_client_tag_rx.state == _forge_tag_rx_idle && g_client_tag_rx.comp_buf == NULL, "aborted cleanly");
		return 0;
	}

	/* 5. Hostile offer metadata: sizes, chunk count, forbidden groups, path traversal, hash bait-and-switch */
	CASE("hostile-offer-fields")
	{
		uint32_t h;
		uint32_t tid = 0x11223344u;
		struct forge_tag_stream_offer_packet base_offer, bad;
		uint32_t dummy_actual = 0;
		uint8_t dummy_bytes[64];

		reset_world();
		memset(dummy_bytes, 0x11, sizeof(dummy_bytes));
		h = forge_compute_catalog_hash('scen', "custom\\rock\\boulder");
		base_offer = make_valid_offer(h, tid, 'scen', "custom\\rock\\boulder");
		CHECK(forge_stream_offer_is_valid(&base_offer, h, tid) == 1, "baseline offer is valid");

		/* Oversized compressed & uncompressed bounds */
		bad = base_offer;
		bad.compressed_size = FORGE_TAG_STREAM_MAX_COMP_BYTES + 1u;
		bad.chunk_count = (uint16_t)((bad.compressed_size + FORGE_TAG_STREAM_CHUNK_BYTES - 1u) / FORGE_TAG_STREAM_CHUNK_BYTES);
		CHECK(forge_stream_offer_is_valid(&bad, h, tid) == 0, "oversized compressed_size rejected");

		bad = base_offer;
		bad.uncompressed_size = FORGE_TAG_STREAM_MAX_UNCOMP_BYTES + 1u;
		CHECK(forge_stream_offer_is_valid(&bad, h, tid) == 0, "oversized uncompressed_size rejected");

		bad = base_offer;
		bad.chunk_count = base_offer.chunk_count + 1u;
		CHECK(forge_stream_offer_is_valid(&bad, h, tid) == 0, "mismatched chunk_count rejected");

		/* Disallowed root tag groups (scenario, BSP, globals) */
		bad = make_valid_offer(forge_compute_catalog_hash('scnr', "levels\\test\\bloodgulch\\bloodgulch"), tid,
			'scnr', "levels\\test\\bloodgulch\\bloodgulch");
		CHECK(forge_stream_offer_is_valid(&bad, bad.hash, tid) == 0, "scnr root group rejected in offer");
		CHECK(forge_install_ephemeral_tag_bundle(bad.hash, 1, 'scnr', "levels\\test\\bloodgulch\\bloodgulch",
			"BG", dummy_bytes, sizeof(dummy_bytes), &dummy_actual) == NONE, "scnr root group rejected in installer");

		/* Path traversal, drive letters, leading/trailing/double slashes, non-ASCII */
		{
			char const *bad_paths[] = {
				"..\\secret\\tag",
				"custom\\..\\secret",
				"c:\\windows\\win",
				"\\leading\\slash",
				"trailing\\slash\\",
				"double\\\\slash",
				"non_ascii_\x80_tag"
			};
			size_t pi;
			for (pi = 0; pi < sizeof(bad_paths) / sizeof(bad_paths[0]); pi++)
			{
				uint32_t bh = forge_compute_catalog_hash('scen', bad_paths[pi]);
				bad = make_valid_offer(bh, tid, 'scen', bad_paths[pi]);
				CHECK(forge_stream_offer_is_valid(&bad, bh, tid) == 0, "bad path rejected in offer: %s", bad_paths[pi]);
				CHECK(forge_install_ephemeral_tag_bundle(bh, 1, 'scen', bad_paths[pi], "Bad",
					dummy_bytes, sizeof(dummy_bytes), &dummy_actual) == NONE, "bad path rejected in installer");
			}
		}

		/* Bait-and-switch: offer claims requested hash h, but tag_path hashes to another tag */
		bad = base_offer;
		snprintf(bad.tag_path, sizeof(bad.tag_path), "weapons\\pistol\\pistol");
		CHECK(forge_stream_offer_is_valid(&bad, h, tid) == 0, "bait-and-switch tag_path hash mismatch rejected");
		CHECK(forge_install_ephemeral_tag_bundle(h, 1, 'scen', "weapons\\pistol\\pistol", "Pistol",
			dummy_bytes, sizeof(dummy_bytes), &dummy_actual) == NONE, "installer rejects hash mismatch");
		return 0;
	}

	/* 6. Duplicate chunk, out-of-range chunk_index, and wrong payload_len rejection */
	CASE("hostile-chunks")
	{
		uint32_t h;
		struct forge_tag_stream_chunk_packet bad_chunk;

		/* 6a: Duplicate chunk 0 */
		reset_world();
		h = stage_host_bundle(1, 'scen', "custom\\dup\\test", "Dup", 3000u);
		forge_client_enqueue_tag_download(h);
		forge_tag_stream_tick(fake_now_ms);
		pump_client_to_host_stream(0);
		CHECK(h2c_count >= 3, "offer + multiple chunks");

		forge_tag_stream_handle_message(NONE, h2c_queue[0].bytes, h2c_queue[0].size, 1); /* OFFER */
		forge_tag_stream_handle_message(NONE, h2c_queue[1].bytes, h2c_queue[1].size, 1); /* CHUNK 0 */
		CHECK(g_client_tag_rx.state == _forge_tag_rx_receiving, "receiving after chunk 0");
		forge_tag_stream_handle_message(NONE, h2c_queue[1].bytes, h2c_queue[1].size, 1); /* DUP CHUNK 0 */
		CHECK(g_client_tag_rx.state == _forge_tag_rx_idle && g_client_tag_rx.strikes == 1, "duplicate chunk rejected with strike");
		CHECK(g_client_tag_rx.comp_buf == NULL, "comp_buf freed on abort");

		/* 6b: Out-of-bounds chunk_index and truncated payload_len */
		reset_world();
		h = stage_host_bundle(1, 'scen', "custom\\oob\\test", "OOB", 3000u);
		forge_client_enqueue_tag_download(h);
		forge_tag_stream_tick(fake_now_ms);
		pump_client_to_host_stream(0);
		forge_tag_stream_handle_message(NONE, h2c_queue[0].bytes, h2c_queue[0].size, 1); /* OFFER */

		memset(&bad_chunk, 0, sizeof(bad_chunk));
		memcpy(&bad_chunk, h2c_queue[1].bytes, h2c_queue[1].size);
		bad_chunk.chunk_index = 99u;
		forge_tag_stream_handle_message(NONE, &bad_chunk, h2c_queue[1].size, 1);
		CHECK(g_client_tag_rx.state == _forge_tag_rx_idle && g_client_tag_rx.strikes == 1, "OOB chunk_index rejected");
		return 0;
	}

	/* 7. Corrupted BLAKE2b-256 digest & zip-bomb inflate size mismatch never reach tag parser */
	CASE("bad-digest-and-inflate-mismatch")
	{
		uint32_t h;

		/* 7a: Valid compressed stream, but BLAKE2b-256 does not match uncompressed bytes */
		reset_world();
		h = stage_host_bundle(1, 'scen', "custom\\hash\\tampered", "Tampered", 800u);
		forge_client_enqueue_tag_download(h);
		forge_tag_stream_tick(fake_now_ms);
		pump_client_to_host_stream(0);
		CHECK(h2c_count == 2, "1 offer + 1 chunk");

		/* Flip 1 bit in offer's blake2b_256 */
		{
			struct forge_tag_stream_offer_packet *of = (struct forge_tag_stream_offer_packet *)h2c_queue[0].bytes;
			of->blake2b_256[31] ^= 0x01u;
		}
		pump_host_to_client_stream();
		CHECK(g_client_tag_rx.strikes == 1, "BLAKE2b mismatch recorded strike");
		CHECK(fake_faber_install_calls == 0 && fake_xbox_install_calls == 0, "tag parser NEVER reached on digest mismatch");

		/* 7b: Inflate size mismatch (corrupted compressed header) */
		reset_world();
		h = stage_host_bundle(1, 'scen', "custom\\bomb\\mismatch", "Bomb", 800u);
		forge_client_enqueue_tag_download(h);
		forge_tag_stream_tick(fake_now_ms);
		pump_client_to_host_stream(0);
		/* Corrupt compressed magic inside chunk 0 payload */
		h2c_queue[1].bytes[offsetof(struct forge_tag_stream_chunk_packet, payload)] ^= 0xFFu;
		pump_host_to_client_stream();
		CHECK(g_client_tag_rx.strikes == 1, "inflate failure recorded strike");
		CHECK(fake_faber_install_calls == 0, "tag parser NEVER reached on inflate failure");
		return 0;
	}

	/* 8. Base-map tag anti-shadowing and 24 MB session RAM cap */
	CASE("base-map-shadow-and-ram-cap")
	{
		uint32_t h;
		uint32_t actual_grp = 0;
		uint8_t raw[128];

		reset_world();
		memset(raw, 0x33, sizeof(raw));

		/* Attempt to overwrite base-map tag index 3 (< fake_base_map_tag_count = 10) */
		fake_base_map_tag_count = 10;
		fake_existing_tag_handle = ((long)0xE0030003UL);
		fake_existing_tag_group = 'weap';
		snprintf(fake_existing_tag_path, sizeof(fake_existing_tag_path), "weapons\\pistol\\pistol");
		h = forge_compute_catalog_hash('weap', "weapons\\pistol\\pistol");

		CHECK(forge_install_ephemeral_tag_bundle(
			h, 1, 'weap', "weapons\\pistol\\pistol", "Fake Pistol", raw, sizeof(raw), &actual_grp) == NONE,
			"streamed bundle must never shadow an existing base-map tag");
		CHECK(fake_faber_install_calls == 0, "installer refused before parsing");

		/* Enforce 24 MB session RAM cap */
		reset_world();
		s_streamed_phys_bytes = FORGE_TAG_STREAM_MAX_SESSION_BYTES - 64u;
		h = forge_compute_catalog_hash('scen', "custom\\huge\\statue");
		CHECK(forge_install_ephemeral_tag_bundle(
			h, 1, 'scen', "custom\\huge\\statue", "Statue", raw, sizeof(raw), &actual_grp) == NONE,
			"streamed bundle exceeding 24 MB session RAM cap must be refused");
		CHECK(fake_faber_install_calls == 0, "RAM cap refused before parsing");
		return 0;
	}

	/* 9. 3-strike malicious host cutoff and session reset recovery */
	CASE("three-strikes-cutoff-and-reset")
	{
		uint32_t h1, h2, h3, h4;
		struct forge_tag_stream_miss_packet bad_pkt;

		reset_world();
		h1 = forge_compute_catalog_hash('scen', "custom\\set\\one");
		h2 = forge_compute_catalog_hash('scen', "custom\\set\\two");
		h3 = forge_compute_catalog_hash('scen', "custom\\set\\three");
		h4 = forge_compute_catalog_hash('scen', "custom\\set\\four");

		forge_client_enqueue_tag_download(h1);
		forge_client_enqueue_tag_download(h2);
		forge_client_enqueue_tag_download(h3);
		forge_client_enqueue_tag_download(h4);
		CHECK(g_client_tag_rx.queue_count == 4, "4 tags queued");

		memset(&bad_pkt, 0, sizeof(bad_pkt));
		forge_tag_stream_fill_header(&bad_pkt.header, _distributed_message_forge_tag_stream, (word)sizeof(bad_pkt));
		bad_pkt.magic = 0xDEADBEEFu;

		forge_tag_stream_handle_message(NONE, &bad_pkt, (word)sizeof(bad_pkt), 1);
		forge_tag_stream_handle_message(NONE, &bad_pkt, (word)sizeof(bad_pkt), 1);
		CHECK(g_client_tag_rx.strikes == 2 && g_client_tag_rx.queue_count == 4, "2 strikes so far");

		/* 3rd strike wipes queue and locks out streaming for the session */
		forge_tag_stream_handle_message(NONE, &bad_pkt, (word)sizeof(bad_pkt), 1);
		CHECK(g_client_tag_rx.strikes == 3, "reached 3 strikes");
		CHECK(g_client_tag_rx.queue_count == 0, "queue cleared at 3 strikes");

		/* Further incoming packets from the host are ignored completely */
		forge_tag_stream_handle_message(NONE, &bad_pkt, (word)sizeof(bad_pkt), 1);
		CHECK(g_client_tag_rx.strikes == 3, "further stream packets ignored after 3 strikes");

		forge_client_enqueue_tag_download(h4);
		forge_tag_stream_tick(fake_now_ms);
		CHECK(g_client_tag_rx.queue_count == 0 && c2h_count == 0, "no further requests after 3 strikes");

		/* Resetting session restores clean operation */
		forge_tag_stream_reset_state();
		CHECK(g_client_tag_rx.strikes == 0, "strikes cleared on session reset");
		forge_client_enqueue_tag_download(h4);
		CHECK(g_client_tag_rx.queue_count == 1, "downloads work again in new session");
		return 0;
	}

	/* 10. Opt-out setting, stall timeout recovery, and reliable-stream backpressure */
	CASE("opt-out-and-stall-timeout")
	{
		uint32_t h1, h2;

		/* 10a: Opt-out (network.forge_tag_download = false), both before enqueue and toggled mid-queue */
		reset_world();
		fake_opt_in_download = 0;
		h1 = stage_host_bundle(1, 'scen', "custom\\opt\\out", "OptOut", 600u);
		forge_client_upsert_stream_proxy_instance(0, h1, -1, -1, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1);
		forge_client_enqueue_tag_download(h1);
		CHECK(g_client_tag_rx.queue_count == 0, "opt-out refuses enqueue immediately");
		fake_opt_in_download = 1;
		forge_client_enqueue_tag_download(h1);
		CHECK(g_client_tag_rx.queue_count == 1, "queued while enabled");
		fake_opt_in_download = 0;
		forge_tag_stream_tick(fake_now_ms);
		CHECK(g_client_tag_rx.queue_count == 0 && c2h_count == 0, "opt-out clears queue and sends zero requests on tick");

		/* 10b: Backpressure retry + 15s stall timeout recovery */
		reset_world();
		h1 = forge_compute_catalog_hash('scen', "custom\\stall\\first");
		h2 = stage_host_bundle(1, 'scen', "custom\\stall\\second", "Second", 600u);
		forge_client_enqueue_tag_download(h1);
		forge_client_enqueue_tag_download(h2);

		c2h_backpressure = 1;
		forge_tag_stream_tick(fake_now_ms);
		CHECK(g_client_tag_rx.state == _forge_tag_rx_idle && g_client_tag_rx.queue_count == 2,
			"backpressure keeps request at head of queue");

		c2h_backpressure = 0;
		forge_tag_stream_tick(fake_now_ms);
		CHECK(g_client_tag_rx.state == _forge_tag_rx_wait_offer && g_client_tag_rx.hash == h1, "sent h1 after backpressure cleared");
		c2h_count = 0; /* Host stalls and never replies to h1 */

		/* Stall timeout aborts h1 without strike and immediately advances to h2 on the same tick */
		fake_now_ms += FORGE_TAG_STREAM_STALL_TIMEOUT_MS + 100u;
		forge_tag_stream_tick(fake_now_ms);
		CHECK(g_client_tag_rx.strikes == 0, "stall timed out without strike");
		CHECK(g_client_tag_rx.state == _forge_tag_rx_wait_offer && g_client_tag_rx.hash == h2, "immediately advanced to h2");
		pump_client_to_host_stream(0);
		pump_host_to_client_stream();
		CHECK(g_client_tag_rx.downloaded_count == 1, "h2 completed after h1 stall");
		return 0;
	}

	/* 11. Reliable backlog cap, high-FPS wall-clock byte-rate budget, and bounded stall-timeout retry */
	CASE("backlog-pacing-and-timeout-retry")
	{
		uint32_t h, h_retry, tid1, tid2, tid3;
		int f;

		/* 11a: Host pauses streaming completely when per-client reliable backlog exceeds 16 KB */
		reset_world();
		h = stage_host_bundle(1, 'scen', "custom\\paced\\large_mesh", "Large Mesh", 24000u);
		forge_client_upsert_stream_proxy_instance(0, h, -1, -1, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1);
		forge_client_enqueue_tag_download(h);
		forge_tag_stream_tick(fake_now_ms);
		CHECK(c2h_count == 1, "client sent FTRQ");

		fake_host_backlog = FORGE_TAG_STREAM_MAX_BACKLOG + 1L;
		pump_client_to_host_stream(0);
		CHECK(g_host_tag_tx[0].active == 1, "host registered transfer");
		CHECK(h2c_count == 0, "host paused streaming while reliable backlog > 16 KB");

		/* 11b: High-FPS render loop (20 ticks with 0 ms elapsed) is strictly bounded by 16 KB byte_budget */
		fake_host_backlog = 0;
		fake_connection = _game_connection_network_server;
		for (f = 0; f < 20; f++)
			forge_tag_stream_tick(fake_now_ms);
		CHECK(h2c_count > 0 && h2c_count < 20, "high-FPS 0ms loop stopped at 16 KB burst budget (%d pkts)", h2c_count);
		CHECK(g_host_tag_tx[0].active == 1, "remaining chunks still pending budget replenishment");

		/* Advancing wall clock by 100 ms replenishes budget so remaining chunks finish cleanly */
		fake_now_ms += 100u;
		forge_tag_stream_tick(fake_now_ms);
		fake_connection = _game_connection_network_client;
		CHECK(g_host_tag_tx[0].active == 0 && h2c_count == 20, "all 1 offer + 19 chunks sent after 100 ms replenishment");
		pump_host_to_client_stream();
		CHECK(g_client_tag_rx.downloaded_count == 1, "24 KB multi-burst bundle installed cleanly");

		/* 11c: Bounded stall-timeout retries (up to 3 total attempts with fresh transfer_ids before settling) */
		reset_world();
		h_retry = forge_compute_catalog_hash('scen', "custom\\retry\\flaky");
		forge_client_enqueue_tag_download(h_retry);

		forge_tag_stream_tick(fake_now_ms);
		CHECK(c2h_count == 1 && g_client_tag_rx.active_attempts == 1, "attempt 1 sent");
		tid1 = g_client_tag_rx.transfer_id;
		c2h_count = 0;

		fake_now_ms += FORGE_TAG_STREAM_STALL_TIMEOUT_MS + 100u;
		forge_tag_stream_tick(fake_now_ms);
		CHECK(c2h_count == 1 && g_client_tag_rx.active_attempts == 2, "attempt 2 sent after 1st stall");
		tid2 = g_client_tag_rx.transfer_id;
		CHECK(tid2 != tid1, "attempt 2 uses fresh transfer_id");
		c2h_count = 0;

		fake_now_ms += FORGE_TAG_STREAM_STALL_TIMEOUT_MS + 100u;
		forge_tag_stream_tick(fake_now_ms);
		CHECK(c2h_count == 1 && g_client_tag_rx.active_attempts == 3, "attempt 3 sent after 2nd stall");
		tid3 = g_client_tag_rx.transfer_id;
		CHECK(tid3 != tid2, "attempt 3 uses fresh transfer_id");
		c2h_count = 0;

		fake_now_ms += FORGE_TAG_STREAM_STALL_TIMEOUT_MS + 100u;
		forge_tag_stream_tick(fake_now_ms);
		CHECK(c2h_count == 0 && g_client_tag_rx.state == _forge_tag_rx_idle, "settled permanently after 3rd stall");
		CHECK(forge_client_is_hash_settled(h_retry) == 1 && g_client_tag_rx.strikes == 0, "marked settled with 0 strikes");
		return 0;
	}

	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
