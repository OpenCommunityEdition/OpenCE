#include "harness.h"
#include <limits.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CEILING(a, b) ((a) < (b) ? (b) : (a))
#define SHORT_MAX SHRT_MAX
#define _pi 3.14159265f
#define MAXIMUM_CHEATS 16
#define WEAPON_DEFINITION_TAG 1
#define NUMBEROF(array) (sizeof(array) / sizeof((array)[0]))
#include "config.inc"

struct tag_reference { long index; };
struct tag_block { long count; void *address; };
struct game_globals { struct tag_block weapon_list; };
struct player_datum { long unit_index; };
struct unit_datum { int unused; };
struct tag_iterator { long next; };
struct weapon_definition { struct { short type; } object; struct { unsigned long flags; } weapon; };
struct object_placement_data
{
	long definition_index, owner_team_index;
	real_point3d position;
	real_vector3d forward, up;
};

static struct weapon_definition definitions[6];
static struct game_globals globals;
static struct player_datum player = { 7 };
static struct unit_datum unit;
static short connection;
static long player_index, made[6], made_count;

#define TAG_BLOCK_GET_ELEMENT(block, index, type) (&((type *)(block)->address)[index])
#define TAG_BLOCK_TRY_AND_GET_ELEMENT(block, index, type) ((block)->count ? TAG_BLOCK_GET_ELEMENT(block, index, type) : NULL)
#define object_definition_get(index) (&definitions[index])
#define weapon_definition_get(index) (&definitions[index])
static short game_connection(void) { return connection; }
static long cheat_player_index(void) { return player_index; }
static struct player_datum *player_get(long index) { (void)index; return &player; }
static struct unit_datum *unit_get(long index) { (void)index; return &unit; }
static struct game_globals *scenario_get_game_globals(void) { return &globals; }
static void object_get_origin(long index, real_point3d *origin) { (void)index; memset(origin, 0, sizeof(*origin)); }
static void object_get_orientation(long index, real_vector3d *forward, real_vector3d *up)
{ (void)index; memset(forward, 0, sizeof(*forward)); memset(up, 0, sizeof(*up)); }
/* Placement maths is not under test; the real spawn loop and tag selection are. */
static real arctangent(real a, real b) { (void)a; (void)b; return 0; }
static real cosine(real a) { (void)a; return 1; }
static real sine(real a) { (void)a; return 0; }
static void object_placement_data_new(struct object_placement_data *data, long definition, long owner)
{ memset(data, 0, sizeof(*data)); data->definition_index = definition; data->owner_team_index = owner; }
static long object_new(struct object_placement_data *data)
{
	CHECK(made_count < 6, "too many spawns");
	CHECK(data->owner_team_index == NONE, "cheat assigned an objective owner");
	made[made_count++] = data->definition_index;
	return made_count;
}
static void tag_iterator_new(struct tag_iterator *iterator, long group) { (void)group; iterator->next = 0; }
static long tag_iterator_next(struct tag_iterator *iterator) { return iterator->next < 4 ? iterator->next++ : NONE; }

#include "under_test.inc"

int main(int argc, char **argv)
{
	char const *case_name = argv[1];
	struct tag_reference references[] = { { 0 }, { 1 }, { NONE }, { 2 }, { 3 } };
	struct tag_reference other[] = { { 4 }, { 5 } };
	struct tag_block other_block = { 2, other };
	short index;
	(void)argc;
	connection = _game_connection_network_server;
	globals.weapon_list.count = 5;
	globals.weapon_list.address = references;
	for (index = 0; index < 6; index++) definitions[index].object.type = _object_type_weapon;
	definitions[1].weapon.flags = FLAG(_weapon_must_be_readied_bit);
	definitions[2].weapon.flags = FLAG(_weapon_must_be_readied_bit) | FLAG(_weapon_doesnt_count_toward_maximum_bit);
	definitions[3].weapon.flags = FLAG(_weapon_must_be_picked_up_bit);
	definitions[4].object.type = _object_type_vehicle;
	definitions[5].object.type = _object_type_equipment;
	definitions[4].weapon.flags = definitions[5].weapon.flags = FLAG(_weapon_must_be_readied_bit);
	if (strstr(case_name, "offline")) connection = _game_connection_local;
	if (strstr(case_name, "fallback")) globals.weapon_list.count = 0;
	if (!strcmp(case_name, "no-player")) player_index = NONE;
	if (!strcmp(case_name, "other-objects"))
	{
		cheat_objects_from_block(&other_block);
		CHECK(made_count == 2 && made[0] == 4 && made[1] == 5, "non-weapons changed");
	}
	else
	{
		cheat_all_weapons();
		if (player_index == NONE) CHECK(made_count == 0, "spawned without a player");
		else if (connection == _game_connection_local)
			CHECK(made_count == 4 && made[0] == 0 && made[1] == 1 && made[2] == 2 && made[3] == 3, "offline behavior changed");
		else CHECK(made_count == 2 && made[0] == 0 && made[1] == 3, "host spawned unowned objectives or lost a normal weapon");
	}
	return 0;
}

