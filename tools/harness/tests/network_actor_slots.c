#include "harness.h"

/* The AI units' tables of network_actors.c (the host's and a client's),
found by a hint per unit's absolute index: over random lookups, removals and
new games, each lookup finds or adds the same entry a search of the table
finds or adds, the table holds the same units in the same order, a unit's
hint points at its entry once it is looked up, and a removal's moved entry's
hint follows it. Units of the same absolute index with different salts,
units past the hints' range and NONE are among them, and the table fills. */
#define MAXIMUM_NETWORK_ACTORS 40
#define MAXIMUM_OBJECTS_PER_MAP 64
#define ABSOLUTE_INDICES 70
#define csmemset memset

struct host_actor { long unit_index; long payload; short impulse; long speech_sound; long user_animation_graph; };
struct client_actor { long unit_index; short impulse; long speech_sound; long user_animation_graph; long payload; };

static struct host_actor host_actors[MAXIMUM_NETWORK_ACTORS];
static short host_actor_count;
static struct client_actor client_actors[MAXIMUM_NETWORK_ACTORS];
static short client_actor_count;
static short host_actor_slots[MAXIMUM_OBJECTS_PER_MAP];
static short client_actor_slots[MAXIMUM_OBJECTS_PER_MAP];

#include "under_test.inc"

/* the tables as a search finds them: the units in order */
static long model_units[MAXIMUM_NETWORK_ACTORS];
static short model_count;

static short model_for(long unit_index)
{
	short index;

	for (index = 0; index < model_count; index++)
	{
		if (model_units[index] == unit_index)
			return index;
	}
	if (model_count >= MAXIMUM_NETWORK_ACTORS)
		return NONE;
	model_units[model_count] = unit_index;
	return model_count++;
}

static void model_remove(short index)
{
	model_units[index] = model_units[--model_count];
}

static unsigned long random_seed = 12345;

static unsigned long random_next(void)
{
	random_seed = random_seed * 1103515245UL + 12345UL;
	return (random_seed >> 8) & 0xFFFFFF;
}

static long random_unit(void)
{
	if (random_next() % 50 == 0)
		return NONE;
	return (long)((((random_next() % 3) + 1) << 16) | (random_next() % ABSOLUTE_INDICES));
}

/* the hint of the unit at the index points at it */
static void check_hint(short const *slots, long unit_index, short index, long operation)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_index);

	if (absolute_index < MAXIMUM_OBJECTS_PER_MAP)
	{
		CHECK(slots[absolute_index] == index, "operation %ld: unit %lx at %d, its hint %d",
			operation, unit_index, index, slots[absolute_index]);
	}
}

/* the table's units */
static void check_table(long const *units, short count, long operation)
{
	short index;

	CHECK(count == model_count, "operation %ld: %d entries, %d searched", operation, count, model_count);
	for (index = 0; index < count; index++)
	{
		CHECK(units[index] == model_units[index], "operation %ld: entry %d is unit %lx, searched %lx",
			operation, index, units[index], model_units[index]);
	}
}

static void run_host(void)
{
	long operation;
	long units[MAXIMUM_NETWORK_ACTORS];

	for (operation = 0; operation < 200000; operation++)
	{
		unsigned long choice = random_next() % 100;
		short index;

		if (choice < 65)
		{
			long unit_index = random_unit();
			struct host_actor *actor = host_actor_for(unit_index);
			short searched = model_for(unit_index);

			CHECK((actor ? (short)(actor - host_actors) : NONE) == searched, "operation %ld: unit %lx found at %d, searched %d",
				operation, unit_index, actor ? (int)(actor - host_actors) : NONE, searched);
			if (actor)
			{
				actor->payload = unit_index;
				check_hint(host_actor_slots, unit_index, searched, operation);
			}
		}
		else if (choice < 99)
		{
			if (host_actor_count > 0)
			{
				index = (short)(random_next() % host_actor_count);
				host_actor_remove(index);
				model_remove(index);
				if (index < host_actor_count)
					check_hint(host_actor_slots, host_actors[index].unit_index, index, operation);
			}
		}
		else
		{
			host_actor_count = 0;
			model_count = 0;
		}
		for (index = 0; index < host_actor_count; index++)
		{
			units[index] = host_actors[index].unit_index;
			CHECK(host_actors[index].payload == host_actors[index].unit_index, "operation %ld: entry %d's own data", operation, index);
		}
		check_table(units, host_actor_count, operation);
	}
}

static void run_client(void)
{
	long operation;
	long units[MAXIMUM_NETWORK_ACTORS];

	for (operation = 0; operation < 200000; operation++)
	{
		unsigned long choice = random_next() % 100;
		short index;

		if (choice < 65)
		{
			long unit_index = random_unit();
			struct client_actor *actor = client_actor_for(unit_index);
			short searched = model_for(unit_index);

			CHECK((actor ? (short)(actor - client_actors) : NONE) == searched, "operation %ld: unit %lx found at %d, searched %d",
				operation, unit_index, actor ? (int)(actor - client_actors) : NONE, searched);
			if (actor)
			{
				actor->payload = unit_index;
				check_hint(client_actor_slots, unit_index, searched, operation);
			}
		}
		else if (choice < 99)
		{
			if (client_actor_count > 0)
			{
				index = (short)(random_next() % client_actor_count);
				client_actor_remove(index);
				model_remove(index);
				if (index < client_actor_count)
					check_hint(client_actor_slots, client_actors[index].unit_index, index, operation);
			}
		}
		else
		{
			client_actor_count = 0;
			model_count = 0;
		}
		for (index = 0; index < client_actor_count; index++)
		{
			units[index] = client_actors[index].unit_index;
			CHECK(client_actors[index].payload == client_actors[index].unit_index, "operation %ld: entry %d's own data", operation, index);
		}
		check_table(units, client_actor_count, operation);
	}
}

int main(int argc, char **argv)
{
	if (argc < 2)
		return 2;
	if (!strcmp(argv[1], "host"))
		run_host();
	else if (!strcmp(argv[1], "client"))
		run_client();
	else
		return 2;
	printf("ok\n");
	return 0;
}
