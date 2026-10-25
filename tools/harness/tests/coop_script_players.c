#include "harness.h"
#include <ctype.h>
typedef unsigned short word;
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))
struct player_datum
{
	long unit_index;
};
struct data_iterator
{
	int slot;
};
static struct player_datum people[128];
static int occupied[128], host = TRUE, calls[128];
static long ref_head;
static struct data_array *player_data, *object_list_data;
static long data_next_index(struct data_array *data, long previous)
{
	int i;
	for (i = previous + 1; i < 128; i++)
		if (occupied[i])
			return i;
	return NONE;
}
static struct player_datum *player_get(long i)
{
	return &people[i];
}
static void data_iterator_new(struct data_iterator *it, struct data_array *data)
{
	it->slot = NONE;
}
static struct player_datum *data_iterator_next(struct data_iterator *it)
{
	it->slot = data_next_index(player_data, it->slot);
	return it->slot == NONE ? NULL : &people[it->slot];
}
static boolean coop_scripts_host(void)
{
	return host;
}
static void reference_list_port_full(struct data_array *data)
{
	CHECK(FALSE, "fixture full");
}
static void data_delete_all(struct data_array *data)
{
	memset(data->data, 0, data->maximum_count * data->size);
	data->count = 0;
}
#include "reference.inc"
static long object_list_new(void)
{
	ref_head = NONE;
	return 0;
}
static void object_list_add(long list, long unit)
{
	reference_list_add(object_list_data, &ref_head, unit);
}
static void object_list_mark_players(long list) {}
static void object_list_delete(long list)
{
	data_delete_all(object_list_data);
}
#include "followers.inc"
#include "players.inc"
static long player_index_from_unit_index(long unit)
{
	int i;
	for (i = 0; i < 128; i++)
		if (occupied[i] && people[i].unit_index == unit)
			return i;
	return NONE;
}
static boolean coop_scripts_unit_near(long unit, real_point3d const *p)
{
	return TRUE;
}
static boolean player_position_switches_bsp(real_point3d const *p)
{
	return FALSE;
}
static real_point3d *object_get_origin(long unit, real_point3d *p)
{
	memset(p, 0, sizeof(*p));
	return p;
}
static boolean coop_scripts_arrive_at(long player, real_point3d const *p, long const *arrived,
									  short count)
{
	calls[player]++;
	return TRUE;
}
static void unit_scripting_suspended(long unit, boolean value)
{
	calls[player_index_from_unit_index(unit)]++;
}
static struct
{
	struct
	{
		long parent_object_index;
	} object;
} objects[128];
#define object_get(unit) (&objects[player_index_from_unit_index(unit)])
static void unit_scripting_exit_vehicle(long unit)
{
	calls[player_index_from_unit_index(unit)]++;
}
static char *strlwr(char *p)
{
	char *s = p;
	while (*s)
	{
		*s = tolower((unsigned char)*s);
		s++;
	}
	return p;
}
static void vehicle_scripting_load_magic(long vehicle, char const *seat, long list)
{
	struct data_reference *r = datum_get(object_list_data, ref_head);
	CHECK(!strcmp(seat, "p-rider"), "seat prefix changed");
	calls[player_index_from_unit_index(r->datum_index)]++;
}
static void player_add_equipment(long unit, short profile, boolean reset)
{
	calls[player_index_from_unit_index(unit)]++;
}
#include "actions.inc"
static int script_units(long *units)
{
	int n = 0;
	long ref;
	hs_players();
	for (ref = ref_head; ref != NONE;)
	{
		struct data_reference *r = datum_get(object_list_data, ref);
		units[n++] = r->datum_index;
		ref = r->next_reference_index;
	}
	object_list_delete(0);
	return n;
}
static void verify(void)
{
	long named[128], followers[130];
	int n = script_units(named), i, j, count;
	for (i = 0; i < n; i++)
	{
		memset(followers, 0x55, sizeof(followers));
		count = players_following(named[i], followers, 128);
		CHECK(count == (i == 0 && n > 2 ? n - 2 : 0),
			  "script player %d, %d live units: %d followers", i, n, count);
		for (j = 0; j < count; j++)
			CHECK(followers[j] == named[j + 2], "follower %d disagrees with hs_players", j);
	}
	CHECK(!players_following(NONE, followers, 128), "NONE has followers");
	CHECK(!players_following(123456, followers, 128), "nonplayer has followers");
}
static void reset(int count)
{
	int i;
	memset(occupied, 0, sizeof(occupied));
	memset(calls, 0, sizeof(calls));
	host = TRUE;
	for (i = 0; i < count; i++)
	{
		occupied[i] = 1;
		people[i].unit_index = 1000 + 13 * i;
		objects[i].object.parent_object_index = 555;
	}
	data_delete_all(object_list_data);
}
int main(int argc, char **argv)
{
	char const *name = argc > 1 ? argv[1] : "";
	int n, i;
	long units[128], out[128];
	real_point3d destination = {1, 2, 3};
	object_list_data = game_state_data_new("test refs", 128, sizeof(struct data_reference));
	if (!strcmp(name, "counts"))
	{
		for (n = 0; n <= 128; n++)
		{
			reset(n);
			verify();
		}
	}
	else if (!strcmp(name, "gaps"))
	{
		reset(128);
		for (i = 0; i < 128; i++)
		{
			if (i % 3 == 0)
				occupied[i] = 0;
			if (i % 5 == 0)
				people[i].unit_index = NONE;
		}
		verify();
	}
	else if (!strcmp(name, "respawn"))
	{
		reset(12);
		for (i = 11; i >= 0; i--)
		{
			people[i].unit_index = NONE;
			verify();
			people[i].unit_index = 9000 + i;
			verify();
			occupied[i] = 0;
			verify();
		}
	}
	else if (!strcmp(name, "bounded"))
	{
		reset(12);
		n = script_units(units);
		for (i = 0; i <= n; i++)
		{
			out[i] = 123456;
			CHECK(players_following(units[0], out, i) == (i < n - 2 ? i : n - 2),
				  "capacity ignored");
			CHECK(out[i] == 123456, "output overrun");
		}
	}
	else if (!strcmp(name, "nonhost"))
	{
		reset(12);
		n = script_units(units);
		host = FALSE;
		CHECK(!players_following(units[0], out, 128), "nonhost followers");
	}
	else
	{
		reset(12);
		n = script_units(units);
		if (!strcmp(name, "teleport"))
			coop_scripts_teleport_followers(units[0], &destination);
		else if (!strcmp(name, "suspend"))
			coop_scripts_suspend_followers(units[0], TRUE);
		else if (!strcmp(name, "exit"))
		{
			objects[player_index_from_unit_index(units[4])].object.parent_object_index = 556;
			coop_scripts_exit_followers(units[0], 555);
		}
		else if (!strcmp(name, "board"))
			coop_scripts_board_followers(units[0], 555, "P-RiderLF");
		else if (!strcmp(name, "equipment"))
			coop_scripts_player_add_equipment(units[0], 2, TRUE);
		else
			CHECK(FALSE, "unknown case");
		for (i = 0; i < n; i++)
			CHECK(calls[player_index_from_unit_index(units[i])] ==
					  ((i >= 2 && (!strcmp(name, "exit") ? i != 4 : TRUE)) ||
					   (i == 0 && !strcmp(name, "equipment"))),
				  "wrong action recipient %d", i);
	}
	return 0;
}
