/*
NET_WIRE_GOLDEN.C

What the game puts on the network is the same bytes on every host: this builds
representative messages through the game's own code (taken from its sources by
tools/net_wire_extract.py, unchanged) and compares them with the bytes
tools/net_wire_golden.inc holds, which a little-endian x86 build made.

    python3 tools/net_wire_extract.py build/net-wire/extracted.h
    cc -Ibuild/net-wire -Itools -idirafter port/linux/include tools/net_wire_golden.c -o build/net-wire/golden
    build/net-wire/golden            checks the bytes (exit 1 if one differs)
    cc -DNET_WIRE_DUMP ... ; build/net-wire/dump > tools/net_wire_golden.inc

On a big-endian host (the PowerPC Mac) it is the proof that the swaps are the
right ones. On a little-endian one it checks that the code is what it was, and
(--layouts) that every layout covers its structure.
tools/mac_net_wire_checks.sh builds and runs it for ppc and i386.

Where the game's types are used, this file stands in for them with the same
sizes (the layouts' compile-time checks, which are in the extracted code,
catch a stand-in that is not).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* a 64-bit host's long is not the game's */
#if defined(__LP64__) || defined(_LP64)
#define long int
#endif

#include "halo_endian.h"
#include "halo_port_limits.h"

/* ---------- the game's types, as the sources use them */

typedef unsigned char byte;
typedef unsigned short word;
typedef byte boolean;
typedef float real;
typedef struct { real x, y, z; } real_point3d;
typedef struct { real i, j, k; } real_vector3d;
typedef struct { real yaw, pitch; } real_euler_angles2d;
typedef struct { real i, j; } real_vector2d;
union real_rgb_color { real n[3]; struct { real red, green, blue; }; };
typedef union real_rgb_color real_rgb_color;
typedef word message_header;

#define NONE (-1)
#define TRUE 1
#define FALSE 0
#define FLAG(bit) (1 << (bit))
#define TEST_FLAG(flags, bit) (((flags) & FLAG(bit)) != 0)
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define match_assert(file, line, condition) ((void)0)
#define display_assert(a, b, c, d) abort()
#define system_exit(code) exit(code)
#define csmemcpy memcpy
#define csmemset memset
#define _error_silent 0
static void error(int kind, char const *format, ...) { (void)kind; (void)format; }

/* the constants of the game's enums that the extracted structures use
(players.h, units.h, objects.h, object_definitions.h, player_queues_new.h, network_actors.c) */
enum
{
	NUMBER_OF_PLAYER_POWERUPS = 2,
	NUMBER_OF_UNIT_GRENADE_TYPES = 2,
	MAXIMUM_WEAPONS_PER_UNIT = 4,
	MAXIMUM_REGIONS_PER_OBJECT = 8,
	NUMBER_OF_OBJECT_CHANGE_COLORS = 4,
	DISTRIBUTED_INPUT_HISTORY = 4,
	NUMBER_OF_ANGLES = 2,
	MAXIMUM_MESSAGE_SIZE = 0xFFF,
	NUMBER_OF_MESSAGE_TYPES = 4,
	MESSAGE_FLAG_BITS_MASK = 3,
};
#define GET_MESSAGE_SIZE(message) ((message) >> 4)

/* what the extracted code calls */
static long game_time = 0x01020304;
static long game_time_get(void) { return game_time; }

/* the bytes of each message as the transport gets it (its header word in the network's order, as
network_connection.c puts it) */
static unsigned char sent[8][4096];
static long sent_size[8];
static int sent_count;

static void net_wire_capture(void const *message, word size)
{
	if (sent_count < 8 && size <= sizeof(sent[0]))
	{
		message_header header;

		memcpy(sent[sent_count], message, size);
		memcpy(&header, message, sizeof(header));
		header = halo_be16(header);
		memcpy(sent[sent_count], &header, sizeof(header));
		sent_size[sent_count++] = size;
	}
}

static int network_distributed_client_send(void *message, word size) { net_wire_capture(message, size); return 1; }
static int network_distributed_server_send_to_machine(long machine_index, void *message, word size)
{
	(void)machine_index; net_wire_capture(message, size); return 1;
}
static int network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size)
{
	(void)machine_index; net_wire_capture(message, size); return 1;
}

/* byte_swapping.c's, which swaps what it is given (-2 for _2byte) */
static void byte_swap_memory(void *memory, long count, long code)
{
	halo_swap_units_always(memory, count * (code == -2 ? 2 : 4), code == -2 ? 2 : 4);
}

#define NET_WIRE_BEFORE_FUNCTIONS \
	static struct distributed_batch distributed_batches[MAXIMUM_SENDERS]; \
	static struct { long sent; long received; } distributed_statistics;

#include "extracted.h"


/* ---------- making values (the same on every host: typed, field by field) */

static unsigned long sequence;

static unsigned short s16(void) { sequence++; return (unsigned short)(0x0A01 + sequence * 0x0113); }
static unsigned long l32(void) { sequence++; return 0x11223344UL + sequence * 0x01010101UL; }
static byte b8(void) { sequence++; return (byte)(0x21 + sequence * 3); }
static real f32(void) { sequence++; return (real)sequence * 0.5f + 1.25f; }

static int failures;
static int layouts;

#ifdef NET_WIRE_DUMP
static int dumping = 1;
#define REPORT(name, got, size) report(#name, got, size, NULL, 0)
#else
static int dumping;
#include "net_wire_golden.inc"
#define REPORT(name, got, size) report(#name, got, size, golden_##name, sizeof(golden_##name))
#endif

static void report(char const *name, void const *got, long size, unsigned char const *golden, long golden_size)
{
	unsigned char const *bytes = (unsigned char const *)got;
	long index;

	if (dumping)
	{
		printf("static const unsigned char golden_%s[] =\n{", name);
		for (index = 0; index < size; index++)
			printf("%s0x%02X,", index % 16 ? "" : "\n\t", bytes[index]);
		printf("\n};\n");
		return;
	}
	if (size != golden_size || memcmp(got, golden, size))
	{
		printf("FAIL %s: %d bytes", name, (int)size);
		for (index = 0; index < size && index < golden_size; index++)
		{
			if (bytes[index] != golden[index])
			{
				printf(", first difference at %d (%02X, wanted %02X)", (int)index, bytes[index], golden[index]);
				break;
			}
		}
		printf("\n");
		failures++;
	}
	else
		printf("ok   %s (%d bytes)\n", name, (int)size);
}

/* ---------- the messages */

static void test_headers(void)
{
	message_header header = 0;
	unsigned long address = 0xC0A80102UL;
	unsigned long address_swapped = halo_be32(address);
	unsigned short port_swapped = halo_be16(0x1F90);
	unsigned char bytes[8];

	build_message_header(&header, 0x123, 2, 0);
	byte_swap_message_header(&header, _byte_order_network);
	memcpy(bytes, &header, 2);
	REPORT(message_header, bytes, 2);

	/* an address in network order, as transport_endpoint_winsock.c puts it in a sockaddr */
	memcpy(bytes, &address_swapped, 4);
	memcpy(bytes + 4, &port_swapped, 2);
	REPORT(sockaddr_bytes, bytes, 6);
	if (halo_be32(address_swapped) != address)
	{
		printf("FAIL halo_be32 is not its own inverse\n");
		failures++;
	}
}

static void test_encoding(void)
{
	struct data_encoding_state state;
	short shorts[2];
	long value = 0x01020304;
	unsigned char buffer[16];

	shorts[0] = 0x1234;
	shorts[1] = 0x5678;
	memset(buffer, 0, sizeof(buffer));
	memset(&state, 0, sizeof(state));
	state.buffer = buffer;
	state.buffer_size = sizeof(buffer);
	data_encode_memory(&state, shorts, 2, _2byte);
	data_encode_memory(&state, &value, 1, _4byte);
	data_encode_memory(&state, "AB", 2, 1);
	REPORT(data_encoding, buffer, state.offset);
}

static void fill_unit_state(struct distributed_unit_state *state)
{
	memset(state, 0, sizeof(*state));
	state->player_index = 3;
	state->flags = FLAG(_distributed_unit_alive_bit) | FLAG(_distributed_unit_placed_bit) |
		FLAG(_distributed_unit_predicted_bit);
	state->unit_flags = FLAG(_distributed_unit_camouflaged_bit);
	state->unit_index = (long)l32();
	state->vehicle_index = (long)l32();
	state->seat_index = (short)s16();
	state->active_camouflage = 200;
	state->position.x = f32();
	state->position.y = f32();
	state->position.z = f32();
	state->velocity.i = (short)s16();
	state->velocity.j = (short)s16();
	state->velocity.k = (short)s16();
	state->forward.i = (short)s16();
	state->forward.j = (short)s16();
	state->forward.k = (short)s16();
	state->up.i = (short)s16();
	state->up.j = (short)s16();
	state->up.k = (short)s16();
	state->predicted_time = (short)s16();
	state->body_vitality = s16();
	state->shield_vitality = s16();
	state->current_body_damage = s16();
	state->recent_body_damage = s16();
	state->current_shield_damage = s16();
	state->recent_shield_damage = s16();
	state->powerup_durations[0] = (short)s16();
	state->powerup_durations[1] = (short)s16();
}

static void test_unit_states(void)
{
	struct distributed_unit_state state;
	struct distributed_unit_state back;
	byte buffer[DISTRIBUTED_UNIT_STATE_MAXIMUM_SIZE];
	word size;

	fill_unit_state(&state);
	size = distributed_unit_state_write(&state, TRUE, buffer);
	REPORT(unit_state_alive, buffer, size);
	if (distributed_unit_state_read(buffer, buffer + size, &back) != size || back.unit_index != state.unit_index ||
		back.vehicle_index != state.vehicle_index || back.seat_index != state.seat_index ||
		back.position.x != state.position.x || back.position.z != state.position.z ||
		back.velocity.j != state.velocity.j || back.up.k != state.up.k || back.predicted_time != state.predicted_time ||
		back.body_vitality != state.body_vitality || back.recent_shield_damage != state.recent_shield_damage ||
		back.powerup_durations[1] != state.powerup_durations[1] || back.active_camouflage != state.active_camouflage)
	{
		printf("FAIL unit state read does not give what was written\n");
		failures++;
	}
	memset(&state, 0, sizeof(state));
	state.player_index = 5;
	state.killing_player_index = 2;
	state.flags = FLAG(_distributed_unit_friendly_fire_bit);
	size = distributed_unit_state_write(&state, FALSE, buffer);
	REPORT(unit_state_dead, buffer, size);
}

static void test_relayed_actions(void)
{
	struct distributed_relayed_action action;
	struct distributed_relayed_action back;
	byte buffer[DISTRIBUTED_RELAYED_ACTION_MAXIMUM_SIZE];
	word size;

	memset(&action, 0, sizeof(action));
	action.player_index = 1;
	action.desired_weapon_index = 2;
	action.desired_grenade_index = 1;
	action.desired_zoom_level = (signed char)NONE;
	action.control_flags[0] = s16();
	action.control_flags[1] = action.control_flags[0];
	action.control_flags[2] = s16();
	action.control_flags[3] = s16();
	action.yaw = (short)s16();
	action.pitch = (short)s16();
	action.throttle_i = -100;
	action.throttle_j = 77;
	action.primary_trigger = 250;
	size = distributed_relayed_action_write(&action, buffer);
	REPORT(relayed_action, buffer, size);
	if (distributed_relayed_action_read(buffer, buffer + size, &back) != size || back.yaw != action.yaw ||
		back.pitch != action.pitch || memcmp(back.control_flags, action.control_flags, sizeof(back.control_flags)))
	{
		printf("FAIL relayed action read does not give what was written\n");
		failures++;
	}
}

/* a message of count entries, sent to machine 0 and gathered with others into the machine's batch */
#define MESSAGE(type_name, entries_type, entries_count) \
	struct { struct distributed_message_header header; entries_type entries[entries_count]; }

static void test_batch(void)
{
	MESSAGE(pickups, struct distributed_pickup, 2) pickups;
	MESSAGE(pings, struct distributed_player_ping, 2) pings;
	MESSAGE(statistics, struct distributed_player_statistics, 1) statistics;
	MESSAGE(inputs, struct distributed_player_input, 1) inputs;
	MESSAGE(states, struct distributed_object_state, 2) states;
	MESSAGE(bsp, struct distributed_structure_bsp, 1) bsp;
	int index;

	memset(&pickups, 0, sizeof(pickups));
	memset(&pings, 0, sizeof(pings));
	memset(&statistics, 0, sizeof(statistics));
	memset(&inputs, 0, sizeof(inputs));
	memset(&states, 0, sizeof(states));
	memset(&bsp, 0, sizeof(bsp));
	for (index = 0; index < 2; index++)
	{
		pickups.entries[index].player_index = b8();
		pickups.entries[index].kind = b8();
		pickups.entries[index].count = (short)s16();
		pickups.entries[index].definition_index = (long)l32();
		pings.entries[index].player_index = b8();
		pings.entries[index].milliseconds = s16();
		states.entries[index].object_index = (long)l32();
		states.entries[index].flags = b8();
		states.entries[index].time = (short)s16();
		states.entries[index].position.x = f32();
		states.entries[index].position.y = f32();
		states.entries[index].position.z = f32();
		states.entries[index].forward.i = (short)s16();
		states.entries[index].forward.j = (short)s16();
		states.entries[index].forward.k = (short)s16();
		states.entries[index].up.i = (short)s16();
		states.entries[index].up.j = (short)s16();
		states.entries[index].up.k = (short)s16();
		states.entries[index].translational_velocity.i = (short)s16();
		states.entries[index].translational_velocity.j = (short)s16();
		states.entries[index].translational_velocity.k = (short)s16();
		states.entries[index].angular_velocity.i = (short)s16();
		states.entries[index].angular_velocity.j = (short)s16();
		states.entries[index].angular_velocity.k = (short)s16();
	}
	{
		struct game_statistics *stats = &statistics.entries[0].statistics;

		statistics.entries[0].player_index = (short)s16();
		stats->sort_key = (short)s16();
		stats->games_played = (short)s16();
		stats->games_won = (short)s16();
		stats->kills_in_a_row = (short)s16();
		stats->multiple_kills = (short)s16();
		stats->last_kill_time = (short)s16();
		for (index = 0; index < 4; index++)
		{
			stats->kills[index] = (short)s16();
			stats->assists[index] = (short)s16();
		}
		stats->friendly_fire_kills = (short)s16();
		stats->deaths = (short)s16();
		stats->suicides = (short)s16();
		stats->shots_fired = (long)l32();
		stats->shots_hit = (long)l32();
		stats->seconds_online = (long)l32();
		stats->killed_teammate_since_last_death = (short)s16();
		stats->custom_data_size = (short)s16();
		stats->multiplayer_statistics.ctf_statistics.flag_grabs = (short)s16();
		stats->multiplayer_statistics.ctf_statistics.flag_returns = (short)s16();
		stats->multiplayer_statistics.ctf_statistics.flag_scores = (short)s16();
	}
	{
		struct distributed_player_input *input = &inputs.entries[0];

		input->player_index = b8();
		input->structure_bsp_index = b8();
		input->tick = (long)l32();
		input->host_time = (long)l32();
		input->action.control_flags = l32();
		input->action.desired_facing.yaw = f32();
		input->action.desired_facing.pitch = f32();
		input->action.throttle.i = f32();
		input->action.throttle.j = f32();
		input->action.primary_trigger = f32();
		input->action.desired_weapon_index = (short)s16();
		input->action.desired_grenade_index = (short)s16();
		input->action.desired_zoom_level = (short)s16();
		for (index = 0; index < DISTRIBUTED_INPUT_HISTORY; index++)
			input->control_flags[index] = s16();
	}
	bsp.entries[0].structure_bsp_index = (short)s16();

	sent_count = 0;
	distributed_send_to_machine(0, &pickups, _distributed_message_pickups, 2, sizeof(pickups));
	distributed_send_to_machine(0, &pings, _distributed_message_pings, 2, sizeof(pings));
	distributed_send_to_machine(0, &statistics, _distributed_message_player_statistics, 1, sizeof(statistics));
	distributed_send_to_machine(0, &inputs, _distributed_message_player_inputs, 1, sizeof(inputs));
	distributed_send_to_machine(0, &states, _distributed_message_object_states, 2, sizeof(states));
	distributed_send_to_machine(0, &bsp, _distributed_message_structure_bsp, 1, sizeof(bsp));
	distributed_batch_flush(0);
	if (sent_count != 1)
	{
		printf("FAIL the batch went as %d datagrams\n", sent_count);
		failures++;
		return;
	}
	REPORT(batch, sent[0], sent_size[0]);
}

/* a reliable message of one kind: its bytes as sent */
#define RELIABLE(name, type, entry_type, fill) \
	do \
	{ \
		struct { struct distributed_message_header header; entry_type entry; } message; \
		memset(&message, 0, sizeof(message)); \
		fill(&message.entry); \
		sent_count = 0; \
		distributed_send_to_machine_reliably(0, &message, type, 1, sizeof(message)); \
		REPORT(name, sent[0], sent_size[0]); \
	} while (0)

static void fill_object_change(struct distributed_object_change *change)
{
	int index;

	change->change = b8();
	change->flags = b8();
	change->owner_player_index = b8();
	change->forced_shader_permutation_index = b8();
	change->object_index = (long)l32();
	change->definition_index = (long)l32();
	change->owner_team_index = (short)s16();
	change->variant_number = (short)s16();
	change->position.x = f32(); change->position.y = f32(); change->position.z = f32();
	change->forward.i = f32(); change->forward.j = f32(); change->forward.k = f32();
	change->up.i = f32(); change->up.j = f32(); change->up.k = f32();
	change->translational_velocity.i = f32(); change->translational_velocity.j = f32();
	change->translational_velocity.k = f32();
	change->angular_velocity.i = f32(); change->angular_velocity.j = f32(); change->angular_velocity.k = f32();
	for (index = 0; index < NUMBER_OF_OBJECT_CHANGE_COLORS; index++)
	{
		change->change_colors[index].red = f32();
		change->change_colors[index].green = f32();
		change->change_colors[index].blue = f32();
	}
	for (index = 0; index < MAXIMUM_REGIONS_PER_OBJECT; index++)
		change->region_permutations[index] = b8();
}

static void fill_inventory(struct distributed_inventory *inventory)
{
	int index;

	inventory->unit_index = (long)l32();
	inventory->grenade_counts[0] = 2;
	inventory->grenade_counts[1] = 1;
	inventory->current_weapon_index = 1;
	for (index = 0; index < MAXIMUM_WEAPONS_PER_UNIT; index++)
	{
		inventory->weapon_indices[index] = (long)l32();
		inventory->rounds_total[index][0] = (short)s16();
		inventory->rounds_total[index][1] = (short)s16();
		inventory->rounds_loaded[index][0] = (short)s16();
		inventory->rounds_loaded[index][1] = (short)s16();
		inventory->age[index] = s16();
	}
}

static void fill_damage(struct distributed_damage *damage)
{
	damage->definition_index = (long)l32();
	damage->flags = l32();
	damage->owner_player_index = b8();
	damage->owner_team_index = (short)s16();
	damage->owner_object_index = (long)l32();
	damage->origin.x = f32(); damage->origin.y = f32(); damage->origin.z = f32();
	damage->epicenter.x = f32(); damage->epicenter.y = f32(); damage->epicenter.z = f32();
	damage->direction.i = f32(); damage->direction.j = f32(); damage->direction.k = f32();
	damage->scale = f32();
	damage->multiplier = f32();
	damage->material_effect_scale = f32();
	damage->material_type = (short)s16();
}

static void fill_damage_event(struct distributed_damage_event *event)
{
	event->kind = b8();
	event->player_index = b8();
	event->kill_flags = b8();
	event->object_index = (long)l32();
	fill_damage(&event->damage);
	event->being_damaged_flags = l32();
	event->shield_damage = f32();
	event->body_damage = f32();
	event->body_damage_multiplier = f32();
	event->total_damage = f32();
	event->body_part = (short)s16();
	event->node_index = (short)s16();
	event->region_index = (short)s16();
	event->material_index = (short)s16();
	event->killer_score = (long)l32();
}

static void fill_hit_report(struct distributed_hit_report *report)
{
	report->object_index = (long)l32();
	fill_damage(&report->damage);
	report->target_position.x = f32(); report->target_position.y = f32(); report->target_position.z = f32();
	report->object_normal.i = f32(); report->object_normal.j = f32(); report->object_normal.k = f32();
	report->node_index = (short)s16();
	report->region_index = (short)s16();
	report->material_index = (short)s16();
	report->has_normal = 1;
	report->host_time = (long)l32();
}

static void fill_damage_animation(struct distributed_damage_animation *animation)
{
	animation->unit_index = (long)l32();
	animation->animation_index = (short)s16();
	animation->type = b8();
	animation->number = b8();
}

static void fill_actor_damage(struct distributed_actor_damage *damage)
{
	damage->unit_index = (long)l32();
	damage->current_shield_damage = s16();
	damage->recent_shield_damage = s16();
	damage->current_body_damage = s16();
	damage->recent_body_damage = s16();
}

static void fill_actor_state(struct distributed_actor_state *state)
{
	state->unit_index = (long)l32();
	state->vehicle_index = (long)l32();
	state->seat_index = (short)s16();
	state->flags = b8();
	state->impulse_number = b8();
	state->impulse = b8();
	state->animation_state = b8();
	state->aiming_speed = b8();
	state->primary_trigger = b8();
	state->control_flags = s16();
	state->throttle[0] = (signed char)b8();
	state->throttle[1] = (signed char)b8();
	state->throttle[2] = (signed char)b8();
	state->active_camouflage = b8();
	state->speech_number = b8();
	state->team = b8();
	state->facing[0] = (short)s16(); state->facing[1] = (short)s16();
	state->aiming[0] = (short)s16(); state->aiming[1] = (short)s16();
	state->looking[0] = (short)s16(); state->looking[1] = (short)s16();
	state->impulse_alignment = (short)s16();
	state->run_blindly_angle = (short)s16();
	state->body_vitality = s16();
	state->shield_vitality = s16();
	state->speech_sound = (long)l32();
	state->user_animation_graph = (long)l32();
	state->user_animation = (short)s16();
	state->user_animation_number = b8();
	state->user_animation_interpolate = b8();
	state->position.x = f32(); state->position.y = f32(); state->position.z = f32();
	state->velocity.i = (short)s16(); state->velocity.j = (short)s16(); state->velocity.k = (short)s16();
	state->forward.i = (short)s16(); state->forward.j = (short)s16(); state->forward.k = (short)s16();
	state->up.i = (short)s16(); state->up.j = (short)s16(); state->up.k = (short)s16();
}

static void fill_coop_event(struct distributed_coop_event *event)
{
	event->kind = b8();
	event->type = b8();
	event->number = s16();
	event->value = (short)s16();
	event->frame = (short)s16();
	event->name_index = (short)s16();
	event->interpolate = b8();
	event->object_index = (long)l32();
	event->target = (long)l32();
	event->tag_index = (long)l32();
	event->definition_index = (long)l32();
	event->reals[0] = f32(); event->reals[1] = f32(); event->reals[2] = f32();
}

static void fill_device_group(struct distributed_coop_device_group *group)
{
	group->group_index = (short)s16();
	group->name_index = (short)s16();
	group->object_index = (long)l32();
	group->definition_index = (long)l32();
	group->role = b8(); group->flags = b8(); group->snaps = b8(); group->changed = b8();
	group->value = f32();
}

static void fill_object_transform(struct distributed_coop_object_transform *transform)
{
	transform->name_index = (short)s16();
	transform->object_index = (long)l32();
	transform->definition_index = (long)l32();
	transform->position.x = f32(); transform->position.y = f32(); transform->position.z = f32();
	transform->forward.i = (short)s16(); transform->forward.j = (short)s16(); transform->forward.k = (short)s16();
	transform->up.i = (short)s16(); transform->up.j = (short)s16(); transform->up.k = (short)s16();
}

static void fill_object_look(struct distributed_coop_object_look *look)
{
	int index;

	look->name_index = (short)s16();
	look->object_index = (long)l32();
	look->definition_index = (long)l32();
	for (index = 0; index < MAXIMUM_REGIONS_PER_OBJECT; index++)
		look->region_permutations[index] = b8();
	look->scale = f32();
}

static void fill_device_state(struct distributed_coop_device_state *state)
{
	state->name_index = (short)s16();
	state->moving = b8();
	state->object_index = (long)l32();
	state->definition_index = (long)l32();
	state->position = f32();
	state->power = f32();
}

static void test_reliable_messages(void)
{
	RELIABLE(object_changes, _distributed_message_object_changes, struct distributed_object_change, fill_object_change);
	RELIABLE(inventories, _distributed_message_inventories, struct distributed_inventory, fill_inventory);
	RELIABLE(damage_events, _distributed_message_damage_events, struct distributed_damage_event, fill_damage_event);
	RELIABLE(hit_reports, _distributed_message_hit_reports, struct distributed_hit_report, fill_hit_report);
	RELIABLE(damage_animations, _distributed_message_damage_animations, struct distributed_damage_animation,
		fill_damage_animation);
	RELIABLE(actor_damage, _distributed_message_actor_damage, struct distributed_actor_damage, fill_actor_damage);
	RELIABLE(actor_states, _distributed_message_actor_states, struct distributed_actor_state, fill_actor_state);
	RELIABLE(coop_events, _distributed_message_coop_events, struct distributed_coop_event, fill_coop_event);
	RELIABLE(coop_device_groups, _distributed_message_coop_device_groups, struct distributed_coop_device_group,
		fill_device_group);
	RELIABLE(coop_object_transforms, _distributed_message_coop_object_transforms,
		struct distributed_coop_object_transform, fill_object_transform);
	RELIABLE(coop_object_looks, _distributed_message_coop_object_looks, struct distributed_coop_object_look,
		fill_object_look);
	RELIABLE(coop_device_states, _distributed_message_coop_device_states, struct distributed_coop_device_state,
		fill_device_state);
}

/* network_voice.c's: a message made in the host's order and put in the wire's just before it is sent */
static void test_voice(void)
{
	struct { struct distributed_message_header header; struct distributed_voice_up up; } up;
	struct { struct distributed_message_header header; struct distributed_voice_down down; } down;
	struct { struct distributed_message_header header; struct distributed_voice_config config; } config;

	memset(&up, 0, sizeof(up));
	build_message_header(&up.header.header, sizeof(up), 2, 0);
	up.header.type = _distributed_message_voice_up;
	up.header.count = 1;
	up.header.game_time = game_time_get();
	up.up.key = l32();
	up.up.sequence = s16();
	up.up.length = 40;
	VOICE_TO_WIRE(&up.header, &up.up, VOICE_UP_LAYOUT);
	sent_count = 0;
	network_distributed_client_send(&up, sizeof(up));
	REPORT(voice_up, sent[0], sent_size[0]);

	memset(&down, 0, sizeof(down));
	build_message_header(&down.header.header, sizeof(down), 2, 0);
	down.header.type = _distributed_message_voice_down;
	down.header.count = 1;
	down.header.game_time = game_time_get();
	down.down.key = l32();
	down.down.sequence = s16();
	down.down.machine_index = 2;
	down.down.player_index = 6;
	down.down.route = 1;
	down.down.length = 40;
	VOICE_TO_WIRE(&down.header, &down.down, VOICE_DOWN_LAYOUT);
	sent_count = 0;
	network_distributed_client_send(&down, sizeof(down));
	REPORT(voice_down, sent[0], sent_size[0]);

	memset(&config, 0, sizeof(config));
	build_message_header(&config.header.header, sizeof(config), 2, 0);
	config.header.type = _distributed_message_voice_config;
	config.header.count = 1;
	config.header.game_time = game_time_get();
	config.config.key = l32();
	config.config.flags = 1;
	config.config.mode = 2;
	config.config.kbps = 24;
	config.config.proximity_tenths = s16();
	VOICE_TO_WIRE(&config.header, &config.config, VOICE_CONFIG_LAYOUT);
	sent_count = 0;
	network_distributed_client_send(&config, sizeof(config));
	REPORT(voice_config, sent[0], sent_size[0]);
}

/* every layout, applied on any host, swaps each field of a structure whose fields have a known value: the
bytes of a field are reversed, and nothing else moves (--layouts) */
static int swapped_correctly(char const *name, void *structure, long size, char const *layout)
{
	unsigned char before[512];
	unsigned char after[512];
	long offset = 0;
	char const *item = layout;

	memcpy(before, structure, size);
	memcpy(after, structure, size);
	halo_wire_swap_always(after, layout, 1);
	while (*item)
	{
		long repeat = 0;
		long unit;

		while (*item >= '0' && *item <= '9')
			repeat = repeat * 10 + (*item++ - '0');
		unit = *item == 'l' ? 4 : *item == 's' ? 2 : 1;
		item++;
		for (; repeat > 0; repeat--, offset += unit)
		{
			long byte_index;

			for (byte_index = 0; byte_index < unit; byte_index++)
			{
				if (after[offset + byte_index] != before[offset + unit - 1 - byte_index])
				{
					printf("FAIL layout of %s at %d\n", name, (int)offset);
					return 0;
				}
			}
		}
	}
	if (offset != size)
	{
		printf("FAIL layout of %s is %d bytes, the structure %d\n", name, (int)offset, (int)size);
		return 0;
	}
	return 1;
}

/* which fields of a typed structure a layout names: fields of size 2 and 4 bytes are where the typed fill put
nonzero values; a layout that swaps a byte field, or leaves a short alone, shows as a field whose value did not
come out reversed in the golden bytes above. Here the layouts are only checked to cover their structures. */
static void test_layouts(void)
{
	unsigned char block[512];
	long index;

	for (index = 0; index < (long)sizeof(block); index++)
		block[index] = (unsigned char)(index * 7 + 1);
#define LAYOUT_CASE(name, layout, type) \
	do { \
		static char const text[] = layout(HALO_WIRE_STRING); \
		if (swapped_correctly(name, block, sizeof(type), text) && halo_wire_layout_size(text) == (long)sizeof(type)) \
			printf("ok   layout %s (%s)\n", name, text); \
		else failures++; \
	} while (0)
	LAYOUT_CASE("message header", DISTRIBUTED_MESSAGE_HEADER_LAYOUT, struct distributed_message_header);
	LAYOUT_CASE("player statistics", DISTRIBUTED_PLAYER_STATISTICS_LAYOUT, struct distributed_player_statistics);
	LAYOUT_CASE("pickup", DISTRIBUTED_PICKUP_LAYOUT, struct distributed_pickup);
	LAYOUT_CASE("player ping", DISTRIBUTED_PLAYER_PING_LAYOUT, struct distributed_player_ping);
	LAYOUT_CASE("player input", DISTRIBUTED_PLAYER_INPUT_LAYOUT, struct distributed_player_input);
	LAYOUT_CASE("object change", OBJECT_CHANGE_LAYOUT, struct distributed_object_change);
	LAYOUT_CASE("object state", OBJECT_STATE_LAYOUT, struct distributed_object_state);
	LAYOUT_CASE("inventory", INVENTORY_LAYOUT, struct distributed_inventory);
	LAYOUT_CASE("damage animation", DAMAGE_ANIMATION_LAYOUT, struct distributed_damage_animation);
	LAYOUT_CASE("damage event", DISTRIBUTED_DAMAGE_EVENT_LAYOUT, struct distributed_damage_event);
	LAYOUT_CASE("hit report", DISTRIBUTED_HIT_REPORT_LAYOUT, struct distributed_hit_report);
	LAYOUT_CASE("actor state", ACTOR_STATE_LAYOUT, struct distributed_actor_state);
	LAYOUT_CASE("actor damage", ACTOR_DAMAGE_LAYOUT, struct distributed_actor_damage);
	LAYOUT_CASE("coop presentation", COOP_PRESENTATION_LAYOUT, struct distributed_coop_presentation);
	LAYOUT_CASE("coop event", COOP_EVENT_LAYOUT, struct distributed_coop_event);
	LAYOUT_CASE("coop device group", COOP_DEVICE_GROUP_LAYOUT, struct distributed_coop_device_group);
	LAYOUT_CASE("coop object transform", COOP_OBJECT_TRANSFORM_LAYOUT, struct distributed_coop_object_transform);
	LAYOUT_CASE("coop object look", COOP_OBJECT_LOOK_LAYOUT, struct distributed_coop_object_look);
	LAYOUT_CASE("coop device state", COOP_DEVICE_STATE_LAYOUT, struct distributed_coop_device_state);
	LAYOUT_CASE("coop screen effect", COOP_SCREEN_EFFECT_LAYOUT, struct rasterizer_screen_effect_port_state);
	LAYOUT_CASE("voice up", VOICE_UP_LAYOUT, struct distributed_voice_up);
	LAYOUT_CASE("voice down", VOICE_DOWN_LAYOUT, struct distributed_voice_down);
	LAYOUT_CASE("voice config", VOICE_CONFIG_LAYOUT, struct distributed_voice_config);
}

int main(int argc, char **argv)
{
	int argument;

	for (argument = 1; argument < argc; argument++)
	{
		if (!strcmp(argv[argument], "--layouts"))
			layouts = 1;
	}
	printf("%s host, long is %d bytes\n",
#ifdef HALO_BIG_ENDIAN_HOST
		"big-endian",
#else
		"little-endian",
#endif
		(int)sizeof(long));
	if (dumping)
		printf("/* the bytes a little-endian x86 build puts on the wire (tools/net_wire_golden.c, built with -DNET_WIRE_DUMP) */\n");
	test_headers();
	test_encoding();
	test_unit_states();
	test_relayed_actions();
	test_batch();
	test_reliable_messages();
	test_voice();
	if (layouts)
		test_layouts();
	if (!dumping)
		printf("%s (%d failures)\n", failures ? "FAILED" : "all the bytes are the golden ones", failures);
	return failures ? 1 : 0;
}
