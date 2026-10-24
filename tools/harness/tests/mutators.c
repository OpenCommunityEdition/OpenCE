/*
The mutators' game side (port/linux/game/mutators.c, network_mutators.c,
mutator_big_head.c) in a fake world: the keys checked, a game's own mutators
and the host's, the host's message admitted only whole and from the host, and
the big head's node and spheres.
*/

#include "harness.h"
#include <math.h>

typedef unsigned short word;
typedef short message_header;
#define csmemset memset
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define VALID_INDEX(index, count) ((index) >= 0 && (index) < (count))
#define square_root sqrtf
enum { _error_log };
enum { _game_connection_local, _game_connection_network_client, _game_connection_network_server };
enum { _object_type_biped = 0, _object_type_vehicle = 1 };
#define MAXIMUM_NODES_PER_MODEL 64
#define MAXIMUM_MESSAGE_SIZE 0xFFF
#define GET_MESSAGE_TYPE(message) (((message) >> 2) & 3)
#include "message_ids.inc"

struct distributed_message_header
{
	message_header header;
	byte type;
	byte count;
	long game_time;
};
#define RELIABLE_ENTRIES(type) \
	((short)MIN(255, (MAXIMUM_MESSAGE_SIZE - sizeof(struct distributed_message_header)) / sizeof(type)))

struct real_quaternion { real i, j, k, w; };
struct real_orientation { struct real_quaternion rotation; real_point3d translation; real scale; };
typedef struct { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;

/* the world: one object, its definition, model and collision model */
struct tag_block { long count; void *address; };
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (&((type *)(block)->address)[index])
struct collision_vertex { real_point3d point; long first_edge_index; };
struct collision_bsp { struct tag_block vertices; };
struct collision_node { struct tag_block bsps; };
struct collision_model { struct tag_block nodes; };
struct model { struct tag_block nodes; };
struct tag_reference { long index; };
struct _object_definition { struct tag_reference model, collision_model; };
struct _biped_definition { short runtime_head_node_index; };
struct biped_definition { struct _object_definition object; struct _biped_definition biped; };
struct object_definition { struct _object_definition object; };
struct header_block { long size; };
struct _object_datum { short type; struct header_block node_matrices; real_point3d bounding_sphere_center; };
struct object_datum { long definition_index; struct _object_datum object; };

static struct biped_definition the_definition;
static struct model model;
static struct collision_model collision;
static struct collision_node collision_nodes[3];
static struct collision_bsp head_bsp;
static struct collision_vertex head_vertices[2];
static struct object_datum the_object;
static real_matrix4x3 matrices[3];
static int reach_reads;

static struct object_datum *object_get(long index) { (void)index; return &the_object; }
#define object_definition_get(index) ((struct object_definition *)&the_definition)
#define biped_definition_get(index) (&the_definition)
#define model_definition_get(index) (&model)
static struct collision_model *collision_model_definition_get(long index) { (void)index; reach_reads++; return &collision; }
static real_matrix4x3 *object_get_node_matrix(long object_index, short node) { (void)object_index; return &matrices[node]; }
static real distance3d(real_point3d const *a, real_point3d const *b)
{
	return sqrtf((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y) + (a->z - b->z) * (a->z - b->z));
}

/* the game's: the connection, the console, the sender */
static int connection = _game_connection_local, playing = 1, menu = 0, unreliable = 0, warnings = 0, sends = 0;
static byte wire[2048];
static word wire_size;
static short game_connection(void) { return (short)connection; }
static boolean game_in_progress(void) { return playing; }
static boolean main_menu_is_active(void) { return menu; }
enum { _scenario_type_solo, _scenario_type_multiplayer, _scenario_type_main_menu };
static struct scenario { short type; } scenario = { _scenario_type_multiplayer };
static struct scenario *global_scenario_get(void) { return &scenario; }
static boolean network_connection_last_read_was_unreliable(void) { return unreliable; }
#define error(kind, ...) ((void)(kind))
#define console_printf(clear, ...) ((void)(clear))
#define console_warning(...) ((void)warnings++)
static void distributed_send_to_machine_reliably(long machine, void *data, byte type, int count, word size)
{
	struct distributed_message_header *header = (void *)wire;

	(void)machine;
	memcpy(wire, data, size);
	header->header = 8;
	header->type = type;
	header->count = (byte)count;
	wire_size = size;
	sends++;
}

/* port_mutators.c's: the keys of the mutators turned on */
static char local_keys[8][32];
static double local_values[8];
static long local_count;
long port_mutators_keys(char keys[][32], double *values, long maximum)
{
	long index;

	for (index = 0; index < local_count && index < maximum; index++)
	{
		memcpy(keys[index], local_keys[index], 32);
		values[index] = local_values[index];
	}
	return index;
}

#include "mutators_header.inc"
#include "mutators.inc"
#include "network_mutators.inc"
#include "big_head.inc"

static void local(char const *key, double value)
{
	snprintf(local_keys[local_count], 32, "%s", key);
	local_values[local_count++] = value;
}

/* a host's message of `count` entries, as it would come */
static word message(byte const_count, char const *key, real value, word size)
{
	struct distributed_mutators_message *built = (void *)wire;

	memset(wire, 0, sizeof(wire));
	built->header.header = 8;
	built->header.type = _distributed_message_mutators;
	built->header.count = const_count;
	if (const_count > 0)
	{
		snprintf(built->entries[0].key, MUTATOR_KEY_LENGTH, "%s", key);
		built->entries[0].value = value;
	}
	return size;
}

static void world(void)
{
	memset(&the_definition, 0, sizeof(the_definition));
	the_definition.object.model.index = 1;
	the_definition.object.collision_model.index = 7;
	the_definition.biped.runtime_head_node_index = 2;
	model.nodes.count = 3;
	collision.nodes.count = 3;
	collision.nodes.address = collision_nodes;
	memset(collision_nodes, 0, sizeof(collision_nodes));
	collision_nodes[2].bsps.count = 1;
	collision_nodes[2].bsps.address = &head_bsp;
	head_bsp.vertices.count = 2;
	head_bsp.vertices.address = head_vertices;
	head_vertices[0].point.x = 0.1f;
	head_vertices[1].point.z = 0.2f;
	memset(&the_object, 0, sizeof(the_object));
	the_object.object.type = _object_type_biped;
	the_object.object.node_matrices.size = 3 * sizeof(real_matrix4x3);
	memset(matrices, 0, sizeof(matrices));
	/* (the head 0.5 above the sphere's center, at scale 3) */
	matrices[2].position.z = 0.5f;
	matrices[2].scale = 3.f;
}

int main(int argc, char **argv)
{
	char const *case_name = argv[1];

	CHECK(argc == 2, "case");
	world();
	mutators_new_game();
	if (!strcmp(case_name, "keys"))
	{
		CHECK(mutators_key_check("big_head", 1.0, NULL, 0), "the effect refused");
		CHECK(mutators_key_check("big_head.scale", 2.5, NULL, 0), "a value in range refused");
		CHECK(mutators_key_check("big_head.scale", 4.0, NULL, 0) && mutators_key_check("big_head.scale", 1.0, NULL, 0),
			"the range's ends refused");
		CHECK(!mutators_key_check("big_head.scale", 0.5, NULL, 0), "a scale below 1 taken");
		CHECK(!mutators_key_check("big_head.scale", 4.5, NULL, 0), "a scale above 4 taken");
		CHECK(!mutators_key_check("big_head.scale", NAN, NULL, 0), "NaN taken");
		CHECK(!mutators_key_check("big_head.scale", INFINITY, NULL, 0), "infinity taken");
		CHECK(!mutators_key_check("big_head.scale", 1e300, NULL, 0), "a double past a float's range taken");
		CHECK(!mutators_key_check("tiny_head", 1.0, NULL, 0), "an effect this build lacks taken");
		CHECK(!mutators_key_check("big_head.size", 2.0, NULL, 0), "a parameter this build lacks taken");
		CHECK(!mutators_key_check("Big_Head", 1.0, NULL, 0) && !mutators_key_check("big_head..scale", 2.0, NULL, 0) &&
			!mutators_key_check("big_head.", 2.0, NULL, 0) && !mutators_key_check(".scale", 2.0, NULL, 0) &&
			!mutators_key_check("big_head.scale.x", 2.0, NULL, 0) && !mutators_key_check("", 1.0, NULL, 0) &&
			!mutators_key_check(NULL, 1.0, NULL, 0), "a key of other text taken");
		CHECK(!mutators_key_check("big_head", 2.0, NULL, 0), "an effect's own key with a value other than 1 taken");
	}
	else if (!strcmp(case_name, "local"))
	{
		CHECK(!mutators_active() && mutators_parameter(_mutator_big_head_scale) == 2.5f, "stock game not stock");
		local("big_head", 1.0);
		mutators_new_game();
		CHECK(mutators_effect_active(_mutator_effect_big_head) && mutators_parameter(_mutator_big_head_scale) == 2.5f,
			"the effect's default");
		local("big_head.scale", 3.0);
		mutators_new_game();
		CHECK(mutators_parameter(_mutator_big_head_scale) == 3.f, "the mutator's parameter");
		/* (a later mutator's value wins) */
		local("big_head.scale", 1.5);
		mutators_new_game();
		CHECK(mutators_parameter(_mutator_big_head_scale) == 1.5f, "the later value lost");
		connection = _game_connection_network_client;
		mutators_new_game();
		CHECK(!mutators_active(), "a client played its own mutators");
		connection = _game_connection_network_server;
		mutators_new_game();
		CHECK(mutators_active(), "the host did not play its own");
		scenario.type = _scenario_type_main_menu;
		mutators_new_game();
		CHECK(!mutators_active(), "the main menu's map played them");
		scenario.type = _scenario_type_multiplayer;
		local_count = 0;
		mutators_new_game();
		CHECK(!mutators_active(), "a mutator kept into a game without it");
	}
	else if (!strcmp(case_name, "host"))
	{
		char keys[3][MUTATOR_KEY_LENGTH] = { "big_head", "big_head.scale", "moon_gravity" };
		real values[3] = { 1.f, 3.5f, 1.f };

		connection = _game_connection_network_client;
		CHECK(mutators_set_host(keys, values, 3), "the host's keys refused");
		CHECK(mutators_effect_active(_mutator_effect_big_head) && mutators_parameter(_mutator_big_head_scale) == 3.5f,
			"the host's not played");
		CHECK(warnings == 1, "an effect this build lacks not said");
		values[1] = 9.f;
		CHECK(!mutators_set_host(keys, values, 3), "a value out of range taken");
		CHECK(mutators_parameter(_mutator_big_head_scale) == 3.5f, "a refused message changed what is played");
		values[1] = 2.f;
		memcpy(keys[2], "MOON gravity", 13);
		CHECK(!mutators_set_host(keys, values, 3), "a key of other text taken");
		memset(keys[2], 'a', MUTATOR_KEY_LENGTH);
		CHECK(!mutators_set_host(keys, values, 3), "a key not ended taken");
	}
	else if (!strcmp(case_name, "wire"))
	{
		word size = (word)(sizeof(struct distributed_message_header) + sizeof(struct distributed_mutator_entry));

		connection = _game_connection_network_server;
		network_mutators_send(3);
		CHECK(sends == 0, "a host without mutators sent some");
		local("big_head", 1.0);
		local("big_head.scale", 3.0);
		mutators_new_game();
		network_mutators_send(3);
		CHECK(sends == 1 && wire_size == 8 + 2 * 36 && wire[3] == 2, "the wire's layout");
		connection = _game_connection_network_client;
		mutators_new_game();
		CHECK(!mutators_active(), "setup");
		network_mutators_handle_message(NONE, (word *)wire, wire_size);
		CHECK(mutators_parameter(_mutator_big_head_scale) == 3.f, "the host's message not played");
		mutators_new_game();
		network_mutators_handle_message(4, (word *)wire, message(1, "big_head", 1.f, size));
		CHECK(!mutators_active(), "a client's message taken");
		unreliable = 1;
		network_mutators_handle_message(NONE, (word *)wire, message(1, "big_head", 1.f, size));
		unreliable = 0;
		CHECK(!mutators_active(), "a datagram taken");
		network_mutators_handle_message(NONE, (word *)wire, message(1, "big_head", 1.f, (word)(size + 1)));
		CHECK(!mutators_active(), "trailing bytes taken");
		network_mutators_handle_message(NONE, (word *)wire, message(1, "big_head", 1.f, (word)(size - 1)));
		CHECK(!mutators_active(), "a cut entry taken");
		network_mutators_handle_message(NONE, (word *)wire, message(2, "big_head", 1.f, size));
		CHECK(!mutators_active(), "a count past the size taken");
		network_mutators_handle_message(NONE, (word *)wire, message(0, "big_head", 1.f, 8));
		CHECK(!mutators_active(), "a count of 0 taken");
		network_mutators_handle_message(NONE, (word *)wire, message(MAXIMUM_MUTATOR_KEYS + 1, "big_head", 1.f,
			(word)(8 + (MAXIMUM_MUTATOR_KEYS + 1) * 36)));
		CHECK(!mutators_active(), "more entries than a message holds taken");
		network_mutators_handle_message(NONE, (word *)wire, message(1, "big_head", NAN, size));
		CHECK(!mutators_active(), "NaN taken");
		connection = _game_connection_network_server;
		network_mutators_handle_message(NONE, (word *)wire, message(1, "big_head", 1.f, size));
		connection = _game_connection_network_client;
		CHECK(!mutators_active(), "the host took a message");
		network_mutators_handle_message(NONE, (word *)wire, message(1, "big_head", 1.f, size));
		CHECK(mutators_active(), "a whole message refused");
	}
	else if (!strcmp(case_name, "big_head"))
	{
		struct real_orientation orientations[3];
		int index;

		for (index = 0; index < 3; index++)
			orientations[index].scale = 1.f;
		mutator_big_head_node_orientations(0, orientations);
		CHECK(orientations[2].scale == 1.f, "a head scaled with the mutator off");
		CHECK(mutator_big_head_bounding_radius(0, 0.7f) == 0.7f && mutator_big_head_render_radius(0, 0.9f) == 0.9f,
			"a sphere grown with the mutator off");
		local("big_head", 1.0);
		local("big_head.scale", 3.0);
		mutators_new_game();
		mutator_big_head_node_orientations(0, orientations);
		CHECK(orientations[2].scale == 3.f && orientations[0].scale == 1.f && orientations[1].scale == 1.f,
			"only the head scaled");
		/* (0.5 to the head, and its geometry's reach of 0.2 at the node's scale of 3) */
		CHECK(fabsf(mutator_big_head_bounding_radius(0, 0.7f) - 1.1f) < 1e-5f, "the head outside the sphere");
		CHECK(mutator_big_head_bounding_radius(0, 2.f) == 2.f, "a larger sphere shrunk");
		CHECK(fabsf(mutator_big_head_render_radius(0, 0.7f) - (0.5f + 0.2f * 3.f * 1.25f)) < 1e-5f,
			"the head culled");
		CHECK(reach_reads == 1, "the head's reach not kept");
		the_object.object.type = _object_type_vehicle;
		CHECK(mutator_big_head_bounding_radius(0, 0.7f) == 0.7f, "not a biped, grown");
		the_object.object.type = _object_type_biped;
		the_definition.biped.runtime_head_node_index = 3;
		orientations[2].scale = 1.f;
		mutator_big_head_node_orientations(0, orientations);
		CHECK(mutator_big_head_bounding_radius(0, 0.7f) == 0.7f, "a head past the model's nodes used");
		the_definition.biped.runtime_head_node_index = NONE;
		mutator_big_head_node_orientations(0, orientations);
		CHECK(orientations[2].scale == 1.f, "no head, scaled");
	}
	else
	{
		CHECK(0, "no case %s", case_name);
	}
	return 0;
}
