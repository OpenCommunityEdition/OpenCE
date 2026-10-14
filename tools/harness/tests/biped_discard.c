#include "harness.h"

#include "config.inc"
enum { MAXIMUM_TRACKED_OBJECTS = 16, _error_silent = 0 };
struct biped_datum
{
	struct { unsigned long flags; struct { short cluster_index; } location; real_point3d position; } object;
	struct { long swarm_actor_index, actor_index; } unit;
	long definition_index;
};
static struct biped_datum biped;
static boolean engine_running, client;
static int errors, deletes;
char temporary[256];
static struct biped_datum *biped_get(long index) { CHECK(index == 3, "wrong biped"); return &biped; }
static boolean game_engine_running(void) { return engine_running; }
static boolean network_game_distributed_client(void) { return client; }
static const char *tag_get_name(long index) { return "characters\\floodcarrier\\floodcarrier"; }
static const char *tag_name_strip_path(const char *name) { return name; }
static const char *ai_debug_describe_actor(long actor, long object, boolean full, char *buffer, int size) { return ""; }
static void error(int priority, const char *format, ...) { errors++; }
static void object_delete(long index) { CHECK(index == 3, "wrong object deleted"); deletes++; }
#include "under_test.inc"

int main(int argc, char **argv)
{
	const char *case_name = argv[1];

	biped.object.flags = FLAG(_object_outside_of_map_bit);
	biped.object.location.cluster_index = NONE;
	biped.object.position.z = -2003.f;
	biped.unit.swarm_actor_index = NONE;
	biped.unit.actor_index = NONE;
	CASE("client")
	{
		/* the host's carrier, fallen out of the map on a client: the host
		erases it, and the client must neither try nor say so each tick */
		int tick;

		client = TRUE;
		objects_client_has[3] = 3;
		for (tick = 0; tick < 30; tick++)
			CHECK(!biped_check_discard(3), "discard returned TRUE");
		CHECK(!errors && !deletes, "client logged %d and deleted %d times the host's biped", errors, deletes);
	}
	CASE("client-own")
	{
		client = TRUE;
		CHECK(!biped_check_discard(3), "discard returned TRUE");
		CHECK(errors == 1 && deletes == 1, "client's own biped not erased");
	}
	CASE("host")
	{
		CHECK(!biped_check_discard(3), "discard returned TRUE");
		CHECK(errors == 1 && deletes == 1, "host's biped not erased");
	}
	CASE("above")
	{
		biped.object.position.z = -1999.f;
		CHECK(!biped_check_discard(3), "discard returned TRUE");
		CHECK(!errors && !deletes, "biped above the limit erased");
	}
	return 0;
}
