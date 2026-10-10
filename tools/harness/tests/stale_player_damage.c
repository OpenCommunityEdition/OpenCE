#include "harness.h"

typedef unsigned short word;
#include "config.inc"
enum { _object_dead_bit, _unit_record_damage_driver_seat_type = 0 };
enum { _game_connection_local, _game_connection_network_server, _game_connection_network_client };
enum { MAXIMUM_HIT_REPORTS_PER_TICK = 256 };
struct object_fields { long owner_player_index; short type, owner_team_index; unsigned long damage_flags; };
struct object_datum { struct object_fields object; };
struct unit_datum
{
	struct object_fields object;
	struct { long player_index, driver_object_index, gunner_object_index, killing_spree_last_time;
		short killing_spree_count; struct unit_attacker attackers[MAXIMUM_ATTACKERS_PER_UNIT]; } unit;
};
struct player_datum { short identifier, local_player_index; long unit_index; boolean quit_out_of_game; };
struct object_iterator { int absolute_index; unsigned long mask; };
struct distributed_hit_report
{
	long object_index, host_time; struct damage_data damage;
	real_point3d target_position; short node_index, region_index, material_index;
	real_vector3d object_normal; boolean has_normal;
};
static struct data_array *player_data;
static struct unit_datum objects[12];
static long old_player, other_player;
static int ai_calls, ai_unit, connection, damage_report_count, material_calls;
static boolean damage_dealing_report;
static struct distributed_hit_report damage_reports[MAXIMUM_HIT_REPORTS_PER_TICK];

static struct player_datum *player_try_and_get(long index)
{
	unsigned long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	struct player_datum *player;
	if (index == NONE || slot >= (unsigned long)player_data->maximum_count) return NULL;
	player = datum_get(player_data, index);
	return player->identifier && (unsigned short)player->identifier == (unsigned long)index >> 16 ? player : NULL;
}
static struct player_datum *player_get(long index) { return player_try_and_get(index); }
static struct unit_datum *unit_get(long index) { CHECK(index >= 0 && index < 12, "bad unit"); return &objects[index]; }
static struct unit_datum *unit_try_and_get(long index)
{
	return index >= 0 && index < 12 && TEST_FLAG(_object_mask_unit, objects[index].object.type) ? &objects[index] : NULL;
}
static void object_iterator_new(struct object_iterator *iterator, unsigned long mask, byte flags)
{
	CHECK(!flags, "retirement must include inactive objects"); iterator->absolute_index = 0; iterator->mask = mask;
}
static void *object_iterator_next(struct object_iterator *iterator)
{
	while (iterator->absolute_index < 12)
	{
		struct unit_datum *unit = &objects[iterator->absolute_index++];
		if (TEST_FLAG(iterator->mask, unit->object.type)) return unit;
	}
	return NULL;
}
static long game_time_get(void) { return 100; }
static boolean game_team_is_enemy(short a, short b) { return a != b; }
static boolean ai_handle_killing_spree(long index, short count) { ai_calls++; ai_unit = index; return FALSE; }
static int game_connection(void) { return connection; }
static boolean distributed_player_is_local(long index)
{
	struct player_datum *player = player_try_and_get(index);
	return player && player->local_player_index != NONE;
}
static long distributed_player_machine(short index) { return index == DATUM_INDEX_TO_ABSOLUTE_INDEX(old_player) ? 9 : NONE; }
static long distributed_living_unit(struct player_datum *player) { return player ? player->unit_index : NONE; }
static void *object_try_and_get_and_verify_type(long index, unsigned long mask) { return unit_try_and_get(index); }
static boolean distributed_damage_is_recoil(struct damage_data const *damage, long index) { return FALSE; }
static void damage_replay_player_effect(long index, struct damage_data *damage, real amount) { }
static boolean network_objects_client_has(long index) { return TRUE; }
static boolean static_target(long index) { return FALSE; }
static void distributed_damage_from_data(struct damage_data const *damage, struct damage_data *result) { *result = *damage; }
static void object_get_origin(long index, real_point3d *point) { memset(point, 0, sizeof(*point)); }
static long distributed_latest_host_time(void) { return 10; }
static void distributed_damage_material(struct damage_data *damage, long index, short material) { material_calls++; }
#define csmemset memset
#include "under_test.inc"

static void setup(int players)
{
	int i, j;
	player_data = game_state_data_new("players", 128, sizeof(struct player_datum));
	for (i = 0; i < players; i++)
	{
		long index = datum_new(player_data);
		struct player_datum *player = player_get(index);
		player->unit_index = 1; player->local_player_index = NONE;
		if (i == 1) old_player = index;
		if (i == 0) other_player = index;
	}
	memset(objects, 0, sizeof(objects));
	for (i = 0; i < 12; i++)
	{
		objects[i].object.type = i;
		objects[i].object.owner_team_index = 1;
		objects[i].object.owner_player_index = old_player;
		objects[i].unit.player_index = old_player;
		objects[i].unit.driver_object_index = objects[i].unit.gunner_object_index = NONE;
		objects[i].unit.killing_spree_last_time = NONE;
		for (j = 0; j < MAXIMUM_ATTACKERS_PER_UNIT; j++)
		{
			objects[i].unit.attackers[j].player_index = NONE;
			objects[i].unit.attackers[j].object_index = NONE;
			objects[i].unit.attackers[j].game_time_stamp = NONE;
		}
	}
	ai_calls = damage_report_count = material_calls = 0; ai_unit = NONE;
	connection = _game_connection_local; damage_dealing_report = FALSE;
}
static void release_world(void) { free(player_data->data); free(player_data); }
static struct damage_data payload(long owner)
{
	struct damage_data damage;
	memset(&damage, 0x5a, sizeof(damage));
	damage.owner_player_index = owner;
	damage.owner_object_index = 1; damage.owner_team_index = 0;
	return damage;
}
int main(int argc, char **argv)
{
	char const *case_name = argv[1]; int players;
	for (players = 2; players <= 128; players++)
	{
		struct player_datum *owner; struct damage_data damage, expected;
		long index; boolean stale = FALSE, notify = TRUE;
		setup(players); owner = player_get(old_player);
		CASE("retire")
		{
			struct unit_datum expected_objects[12]; int i;
			objects[0].unit.attackers[0] = (struct unit_attacker){99, 7, 1, old_player};
			objects[1].unit.attackers[1] = (struct unit_attacker){99, 3, 1, other_player};
			objects[2].object.owner_player_index = old_player ^ 0x10000;
			objects[3].object.owner_player_index = other_player;
			memcpy(expected_objects, objects, sizeof(objects));
			for (i = 0; i < 12; i++)
			{
				if (i != 2 && i != 3) expected_objects[i].object.owner_player_index = NONE;
				if (i < 2) expected_objects[i].unit.player_index = NONE;
			}
			expected_objects[0].unit.attackers[0].player_index = NONE;
			player_delete(old_player);
			CHECK(!player_try_and_get(old_player), "player datum not deleted");
			CHECK(!memcmp(expected_objects, objects, sizeof(objects)), "retirement changed teams/damage or left stale references");
			index = datum_new(player_data);
			CHECK(DATUM_INDEX_TO_ABSOLUTE_INDEX(index) == DATUM_INDEX_TO_ABSOLUTE_INDEX(old_player) && index != old_player,
				"slot was not reused with a fresh generation");
			release_world(); continue;
		}
		if (!strcmp(case_name, "deleted") || !strcmp(case_name, "reused") ||
			!strcmp(case_name, "payload") || !strcmp(case_name, "nonlethal") || !strcmp(case_name, "stale-item"))
		{
			/* Simulate an already stale checkpoint/delayed damage event, bypassing cleanup. */
			datum_delete(player_data, old_player); stale = TRUE;
			if (!strcmp(case_name, "reused"))
			{
				index = datum_new(player_data); player_get(index)->unit_index = 0;
			}
		}
		CASE("dead") { objects[1].object.damage_flags = FLAG(_object_dead_bit); }
		CASE("quit") { owner->quit_out_of_game = TRUE; }
		CASE("unspawned") { owner->unit_index = NONE; }
		CASE("nonlethal") { notify = FALSE; }
		CASE("friendly") { objects[0].object.owner_team_index = 0; }
		CASE("driver") { objects[1].unit.driver_object_index = 0; }
		CASE("gunner") { objects[1].unit.gunner_object_index = 0; }
		damage = payload(!strcmp(case_name, "unowned") || !strcmp(case_name, "ai-item") ? NONE : old_player);
		if (!strcmp(case_name, "stale-item") || !strcmp(case_name, "ai-item")) damage.owner_object_index = 3;
		expected = damage; if (stale) expected.owner_player_index = NONE;
		CASE("payload")
		{
			damage_data_validate_owner(&damage);
			CHECK(!memcmp(&damage, &expected, sizeof(damage)), "stale owner or changed damage payload");
			/* NONE and a live (including quit/dead) owner remain byte-identical. */
			damage = payload(other_player); expected = damage; damage_data_validate_owner(&damage);
			CHECK(!memcmp(&damage, &expected, sizeof(damage)), "valid identity changed");
			damage = payload(NONE); expected = damage; damage_data_validate_owner(&damage);
			CHECK(!memcmp(&damage, &expected, sizeof(damage)), "unowned damage changed");
			release_world(); continue;
		}
		CASE("authority")
		{
			connection = _game_connection_network_server;
			CHECK(!network_damage_deals(&damage, 0, NONE, NONE, NONE, NULL, FALSE), "host duplicated connected remote damage");
			CHECK(damage.owner_player_index == old_player, "admission changed ownership");
			owner->quit_out_of_game = TRUE;
			CHECK(network_damage_deals(&damage, 0, NONE, NONE, NONE, NULL, FALSE), "departed player's grenade lost damage");
			datum_delete(player_data, old_player);
			index = datum_new(player_data); player_get(index)->local_player_index = 0;
			CHECK(network_damage_deals(&damage, 0, NONE, NONE, NONE, NULL, FALSE), "stale host damage rejected");
			connection = _game_connection_network_client;
			CHECK(!network_damage_deals(&damage, 0, NONE, NONE, NONE, NULL, FALSE) && !damage_report_count,
				"client applied/reported a stale owner as replacement player");
			CHECK(network_damage_deals(&damage, 0, NONE, NONE, NONE, NULL, TRUE), "authorized replay rejected");
			damage.owner_player_index = index;
			CHECK(!network_damage_deals(&damage, 0, NONE, NONE, NONE, NULL, FALSE) && damage_report_count == 1,
				"live local client did not report its hit");
			CHECK(damage_reports[0].damage.owner_player_index == index, "report identity changed");
			release_world(); continue;
		}
		unit_record_damage(0, 10, !strcmp(case_name, "gunner") ? 1 : 0, notify,
			damage.owner_player_index, damage.owner_team_index, damage.owner_object_index);
		CHECK(objects[0].unit.attackers[0].player_index == expected.owner_player_index, "stale/wrong damage attribution");
		CHECK(objects[0].unit.attackers[0].object_index == damage.owner_object_index && objects[0].unit.attackers[0].damage_inflicted == 10,
			"damage amount/source changed");
		CHECK(ai_calls == (notify && strcmp(case_name, "dead") && strcmp(case_name, "friendly") &&
			strcmp(case_name, "stale-item") && strcmp(case_name, "ai-item")), "AI kill notification changed");
		if (ai_calls) CHECK(ai_unit == (!strcmp(case_name, "driver") || !strcmp(case_name, "gunner") ? 0 : 1), "wrong AI killer");
		release_world();
	}
	return 0;
}
