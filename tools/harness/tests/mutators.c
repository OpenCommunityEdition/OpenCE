/*
The mutators' game side (port/linux/game/mutators.c, network_mutators.c,
object_pose.c) in a fake world: the keys checked, a game's own mutators and
the host's, the host's message admitted only whole and from the host, the
host's tag edits sent in pieces and taken only in order, and poses: the nodes
scaled and moved, and the spheres grown to hold them and what hangs from them.
*/

#include "harness.h"
#include <math.h>
#include <stddef.h>

typedef unsigned short word;
typedef short message_header;
#define csmemset memset
#define csmemcpy memcpy
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define VALID_INDEX(index, count) ((index) >= 0 && (index) < (count))
#define square_root sqrtf
#define NUMBEROF(array) ((int)(sizeof(array) / sizeof((array)[0])))
#define _stricmp strcasecmp
enum { _error_log };
enum { _game_connection_local, _game_connection_network_client, _game_connection_network_server };
enum { _object_type_biped = 0, _object_type_vehicle = 1 };
enum { POSED_SCALE_NODE = 1 };
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

/* the world: two objects (a biped and a vehicle), their definitions, a model
and a collision model. The model's nodes: the pelvis, and under it an upper
arm with a hand under it, and beside the arm the head */
struct tag_block { long count; void *address; };
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (&((type *)(block)->address)[index])
struct collision_vertex { real_point3d point; long first_edge_index; };
struct collision_bsp { struct tag_block vertices; };
struct collision_node { struct tag_block bsps; };
struct collision_model { struct tag_block nodes; };
struct model_node { char name[32]; short next_sibling_node_index, first_child_node_index, parent_node_index; };
struct model { struct tag_block nodes; };
struct tag_reference { long index; };
struct _object_definition { short type; struct tag_reference model, collision_model; };
struct _biped_definition { short runtime_head_node_index; };
struct biped_definition { struct _object_definition object; struct _biped_definition biped; };
struct object_definition { struct _object_definition object; };
struct header_block { long size; };
struct _object_datum { short type; struct header_block node_matrices; real_point3d bounding_sphere_center; };
struct object_datum { long definition_index; struct _object_datum object; };
enum { _node_pelvis, _node_upper_arm, _node_head, _node_hand, NUMBER_OF_NODES };

/* the tags: the biped's and the vehicle's definitions */
static struct { long group_tags[3]; char const *name; } const fake_tags[2] =
{
	{ { 'bipd', 'unit', 'obje' }, "characters\\cyborg\\cyborg" },
	{ { 'vehi', 'unit', 'obje' }, "vehicles\\warthog\\warthog" },
};
struct tag_iterator { long absolute_index; long group_tag; };
static void tag_iterator_new(struct tag_iterator *iterator, long group_tag)
{
	iterator->absolute_index = 0;
	iterator->group_tag = group_tag;
}
static long tag_iterator_next(struct tag_iterator *iterator)
{
	while (iterator->absolute_index < 2)
	{
		long index = iterator->absolute_index++;

		if (fake_tags[index].group_tags[0] == iterator->group_tag || fake_tags[index].group_tags[1] ==
			iterator->group_tag || fake_tags[index].group_tags[2] == iterator->group_tag)
		{
			return index;
		}
	}
	return NONE;
}
static char *tag_get_name(long index) { return (char *)fake_tags[index].name; }

static struct biped_definition definitions[2];
static struct model fake_model;
static struct model_node model_nodes[NUMBER_OF_NODES];
static struct collision_model collision;
static struct collision_node collision_nodes[NUMBER_OF_NODES];
static struct collision_bsp head_bsp, hand_bsp;
static struct collision_vertex head_vertices[2], hand_vertices[1];
static struct object_datum the_object;
static real_matrix4x3 matrices[NUMBER_OF_NODES];
static int reach_reads;

static struct object_datum *object_get(long index) { (void)index; return &the_object; }
#define object_definition_get(index) ((struct object_definition *)&definitions[index])
#define biped_definition_get(index) (&definitions[index])
#define model_definition_get(index) (&fake_model)
static struct collision_model *collision_model_definition_get(long index) { (void)index; reach_reads++; return &collision; }
static real_matrix4x3 *object_get_node_matrix(long object_index, short node) { (void)object_index; return &matrices[node]; }
static real distance3d(real_point3d const *a, real_point3d const *b)
{
	return sqrtf((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y) + (a->z - b->z) * (a->z - b->z));
}

/* the game's: the connection, the console, the sender */
static int connection = _game_connection_local, playing = 1, menu = 0, unreliable = 0, warnings = 0, sends = 0;
static byte wire[4096];
static word wire_size;
/* each message sent, in order */
static byte sent[8][4096];
static word sent_sizes[8];
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
	if (sends < 8)
	{
		memcpy(sent[sends], wire, size);
		sent_sizes[sends] = size;
	}
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
/* the scripts of a game (mutator_scripts.c): the keys given it */
static char script_keys[8][32];
static short script_key_count, script_commits;
void mutator_scripts_begin(void)
{
	script_key_count = 0;
}
void mutator_scripts_add(char const *key)
{
	if (script_key_count < 8)
		snprintf(script_keys[script_key_count++], 32, "%s", key);
}
void mutator_scripts_commit(void)
{
	script_commits++;
}
short mutator_scripts_keys(char keys[][32], short maximum)
{
	short index;

	for (index = 0; index < script_key_count && index < maximum; index++)
		memcpy(keys[index], script_keys[index], 32);
	return index;
}

#include "mutators.inc"
#include "network_mutators.inc"
#include "object_pose.inc"

/* port_mutators.c's tag edits of the mutators on, and mutator_tags.c's
playing them (test_mutator_tags.py tests both): what was played */
static byte local_edits[MAXIMUM_MUTATOR_TAG_EDIT_BYTES], played[MAXIMUM_MUTATOR_TAG_EDIT_BYTES];
static long local_edit_size, played_size;
static int plays;
long port_mutators_tag_edits(unsigned char *edits, long maximum)
{
	CHECK(local_edit_size <= maximum, "more edits than asked for");
	memcpy(edits, local_edits, local_edit_size);
	return local_edit_size;
}
boolean mutator_tags_play(byte const *edits, long size)
{
	/* (edits this build does not make) */
	if (size > 0 && edits[0] == 0xEE)
		return FALSE;
	memcpy(played, edits, size);
	played_size = size;
	plays++;
	return TRUE;
}
void mutator_tags_undo(void)
{
	played_size = 0;
}


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

static void node(int index, char const *name, short sibling, short child)
{
	snprintf(model_nodes[index].name, sizeof(model_nodes[index].name), "%s", name);
	model_nodes[index].next_sibling_node_index = sibling;
	model_nodes[index].first_child_node_index = child;
}

static void world(void)
{
	int index;

	memset(definitions, 0, sizeof(definitions));
	for (index = 0; index < 2; index++)
	{
		definitions[index].object.model.index = 1;
		definitions[index].object.collision_model.index = 7;
		definitions[index].biped.runtime_head_node_index = NONE;
	}
	definitions[0].object.type = _object_type_biped;
	definitions[0].biped.runtime_head_node_index = _node_head;
	definitions[1].object.type = _object_type_vehicle;
	node(_node_pelvis, "bip01 pelvis", NONE, _node_upper_arm);
	node(_node_upper_arm, "bip01 l upperarm", _node_head, _node_hand);
	node(_node_head, "bip01 head", NONE, NONE);
	node(_node_hand, "bip01 l hand", NONE, NONE);
	fake_model.nodes.count = NUMBER_OF_NODES;
	fake_model.nodes.address = model_nodes;
	collision.nodes.count = NUMBER_OF_NODES;
	collision.nodes.address = collision_nodes;
	memset(collision_nodes, 0, sizeof(collision_nodes));
	collision_nodes[_node_head].bsps.count = 1;
	collision_nodes[_node_head].bsps.address = &head_bsp;
	head_bsp.vertices.count = 2;
	head_bsp.vertices.address = head_vertices;
	head_vertices[0].point.x = 0.1f;
	head_vertices[1].point.z = 0.2f;
	collision_nodes[_node_hand].bsps.count = 1;
	collision_nodes[_node_hand].bsps.address = &hand_bsp;
	hand_bsp.vertices.count = 1;
	hand_bsp.vertices.address = hand_vertices;
	hand_vertices[0].point.y = 0.05f;
	memset(&the_object, 0, sizeof(the_object));
	the_object.definition_index = 0;
	the_object.object.type = _object_type_biped;
	the_object.object.node_matrices.size = NUMBER_OF_NODES * sizeof(real_matrix4x3);
	memset(matrices, 0, sizeof(matrices));
	for (index = 0; index < NUMBER_OF_NODES; index++)
		matrices[index].scale = 1.f;
	/* (the head 0.5 above the sphere's center, at scale 3; the hand 0.6 out) */
	matrices[_node_head].position.z = 0.5f;
	matrices[_node_head].scale = 3.f;
	matrices[_node_hand].position.x = 0.6f;
}

/* the object's nodes, unposed, then posed */
static void posed(struct real_orientation *orientations)
{
	int index;

	memset(orientations, 0, NUMBER_OF_NODES * sizeof(*orientations));
	for (index = 0; index < NUMBER_OF_NODES; index++)
		orientations[index].scale = 1.f;
	object_pose_node_orientations(0, orientations);
}

static boolean near(real a, real b)
{
	return fabsf(a - b) < 1e-5f;
}

int main(int argc, char **argv)
{
	char const *case_name = argv[1];
	long index_unused;

	CHECK(argc == 2, "case");
	world();
	mutators_new_game();
	if (!strcmp(case_name, "keys"))
	{
		char reason[160];

		/* (the game has no effects built in: every one refused, and why) */
		CHECK(!mutators_key_check("big_head", 1.0, reason, sizeof(reason)) && strstr(reason, "[[pose]]"),
			"the big head's effect taken: %s", reason);
		CHECK(!mutators_key_check("big_head.scale", 2.5, NULL, 0), "the big head's parameter taken");
		CHECK(!mutators_key_check("moon_gravity", 1.0, reason, sizeof(reason)) &&
			strstr(reason, "no effect moon_gravity") && !strstr(reason, "pose"), "an effect taken: %s", reason);
		CHECK(!mutators_key_check("Big_Head", 1.0, reason, sizeof(reason)) && strstr(reason, "not an effect's name") &&
			!mutators_key_check("big_head..scale", 2.0, NULL, 0) && !mutators_key_check("", 1.0, NULL, 0) &&
			!mutators_key_check(NULL, 1.0, NULL, 0), "a key of other text taken");
	}
	else if (!strcmp(case_name, "local"))
	{
		CHECK(!mutators_active() && script_key_count == 0, "stock game not stock");
		local("script.222817e03a3c03c96ad58429", 1.0);
		mutators_new_game();
		CHECK(mutators_active() && script_key_count == 1 && !strcmp(script_keys[0], "script.222817e03a3c03c96ad58429"), "the script not run");
		connection = _game_connection_network_client;
		mutators_new_game();
		CHECK(!mutators_active() && script_key_count == 0, "a client played its own mutators");
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
		char keys[2][MUTATOR_KEY_LENGTH] = { "script.222817e03a3c03c96ad58429", "moon_gravity" };
		real values[2] = { 1.f, 1.f };

		connection = _game_connection_network_client;
		CHECK(mutators_set_host(keys, values, 2), "the host's keys refused");
		CHECK(mutators_active() && script_key_count == 1, "the host's not played");
		CHECK(warnings == 1, "a key this build lacks not said");
		values[1] = NAN;
		CHECK(!mutators_set_host(keys, values, 2), "NaN taken");
		CHECK(script_key_count == 1, "a refused message changed what is played");
		values[1] = 1.f;
		memcpy(keys[1], "MOON gravity", 13);
		CHECK(!mutators_set_host(keys, values, 2), "a key of other text taken");
		memset(keys[1], 'a', MUTATOR_KEY_LENGTH);
		CHECK(!mutators_set_host(keys, values, 2), "a key not ended taken");
	}
	else if (!strcmp(case_name, "wire"))
	{
		word size = (word)(sizeof(struct distributed_message_header) + sizeof(struct distributed_mutator_entry));

		connection = _game_connection_network_server;
		network_mutators_send(3);
		CHECK(sends == 0, "a host without mutators sent some");
		local("script.222817e03a3c03c96ad58429", 1.0);
		local("script.0000", 1.0);
		mutators_new_game();
		network_mutators_send(3);
		CHECK(sends == 1 && wire_size == 8 + 2 * 36 && wire[3] == 2, "the wire's layout");
		connection = _game_connection_network_client;
		mutators_new_game();
		CHECK(!mutators_active(), "setup");
		network_mutators_handle_message(NONE, (word *)wire, wire_size);
		CHECK(script_key_count == 2 && !strcmp(script_keys[1], "script.0000"), "the host's message not played");
		mutators_new_game();
		network_mutators_handle_message(4, (word *)wire, message(1, "script.222817e03a3c03c96ad58429", 1.f, size));
		CHECK(!mutators_active(), "a client's message taken");
		unreliable = 1;
		network_mutators_handle_message(NONE, (word *)wire, message(1, "script.222817e03a3c03c96ad58429", 1.f, size));
		unreliable = 0;
		CHECK(!mutators_active(), "a datagram taken");
		network_mutators_handle_message(NONE, (word *)wire, message(1, "script.222817e03a3c03c96ad58429", 1.f, (word)(size + 1)));
		CHECK(!mutators_active(), "trailing bytes taken");
		network_mutators_handle_message(NONE, (word *)wire, message(1, "script.222817e03a3c03c96ad58429", 1.f, (word)(size - 1)));
		CHECK(!mutators_active(), "a cut entry taken");
		network_mutators_handle_message(NONE, (word *)wire, message(2, "script.222817e03a3c03c96ad58429", 1.f, size));
		CHECK(!mutators_active(), "a count past the size taken");
		network_mutators_handle_message(NONE, (word *)wire, message(0, "script.222817e03a3c03c96ad58429", 1.f, 8));
		CHECK(!mutators_active(), "a count of 0 taken");
		network_mutators_handle_message(NONE, (word *)wire, message(MAXIMUM_MUTATOR_KEYS + 1, "script.222817e03a3c03c96ad58429", 1.f,
			(word)(8 + (MAXIMUM_MUTATOR_KEYS + 1) * 36)));
		CHECK(!mutators_active(), "more entries than a message holds taken");
		network_mutators_handle_message(NONE, (word *)wire, message(1, "script.222817e03a3c03c96ad58429", NAN, size));
		CHECK(!mutators_active(), "NaN taken");
		connection = _game_connection_network_server;
		network_mutators_handle_message(NONE, (word *)wire, message(1, "script.222817e03a3c03c96ad58429", 1.f, size));
		connection = _game_connection_network_client;
		CHECK(!mutators_active(), "the host took a message");
		network_mutators_handle_message(NONE, (word *)wire, message(1, "script.222817e03a3c03c96ad58429", 1.f, size));
		CHECK(mutators_active(), "a whole message refused");
	}
	else if (!strcmp(case_name, "scripts"))
	{
		/* a host's script: run by its key, and sent on; the old big head's
		key beside it passed over */
		char keys[2][MUTATOR_KEY_LENGTH] = { "script.222817e03a3c03c96ad58429", "big_head" };
		char sent[4][MUTATOR_KEY_LENGTH];
		real values[2] = { 1.f, 1.f };
		real sent_values[4];
		short commits = script_commits;

		connection = _game_connection_network_client;
		CHECK(mutators_set_host(keys, values, 2), "the host's keys refused");
		CHECK(warnings == 1, "the big head's key not said");
		CHECK(script_key_count == 1 && !strcmp(script_keys[0], keys[0]) && script_commits == commits + 1,
			"the script not run");
		CHECK(mutators_keys(sent, sent_values, 4) == 1 && !strcmp(sent[0], keys[0]), "the script not sent on");
		/* the next game's without it */
		CHECK(mutators_set_host(keys + 1, values + 1, 1) && script_key_count == 0 && script_commits == commits + 2,
			"still run");
	}
	else if (!strcmp(case_name, "tag_edits"))
	{
		byte const *edits;
		int piece;

		for (index_unused = 0; index_unused < 9000; index_unused++)
			local_edits[index_unused] = (byte)(index_unused * 7 + 1);
		local_edit_size = 9000;
		connection = _game_connection_network_server;
		plays = 0;
		mutators_new_game();
		CHECK(plays == 1 && played_size == 9000 && !memcmp(played, local_edits, 9000), "the host's own not played");
		CHECK(mutators_tag_edits(&edits) == 9000 && mutators_active(), "the host's kept");
		network_mutators_send(3);
		/* (no keys: no message of them) */
		CHECK(sends == 3, "the pieces (%d)", sends);
		for (piece = 0; piece < 3; piece++)
		{
			struct distributed_mutator_tag_edits_message const *message = (void *)sent[piece];

			CHECK(message->header.type == _distributed_message_mutator_tag_edits && message->header.count == 1 &&
				message->size == 9000 && message->offset == piece * 4000 &&
				sent_sizes[piece] == 12 + (piece < 2 ? 4000 : 1000), "piece %d's layout", piece);
		}
		connection = _game_connection_network_client;
		mutators_new_game();
		CHECK(played_size == 0 && mutators_tag_edits(&edits) == 0, "a client played its own edits");
		/* (out of order: the rest refused until a first piece) */
		network_mutators_handle_message(NONE, (word *)sent[0], sent_sizes[0]);
		network_mutators_handle_message(NONE, (word *)sent[2], sent_sizes[2]);
		network_mutators_handle_message(NONE, (word *)sent[1], sent_sizes[1]);
		CHECK(played_size == 0, "pieces out of order played");
		network_mutators_handle_message(4, (word *)sent[0], sent_sizes[0]);
		network_mutators_handle_message(4, (word *)sent[1], sent_sizes[1]);
		network_mutators_handle_message(4, (word *)sent[2], sent_sizes[2]);
		CHECK(played_size == 0, "a client's pieces played");
		unreliable = 1;
		for (piece = 0; piece < 3; piece++)
			network_mutators_handle_message(NONE, (word *)sent[piece], sent_sizes[piece]);
		unreliable = 0;
		CHECK(played_size == 0, "datagrams played");
		for (piece = 0; piece < 3; piece++)
			network_mutators_handle_message(NONE, (word *)sent[piece], sent_sizes[piece]);
		CHECK(played_size == 9000 && !memcmp(played, local_edits, 9000) && mutators_tag_edits(&edits) == 9000,
			"the host's edits not played");
		/* (a piece past the edits' size, a size past the most) */
		mutators_new_game();
		((struct distributed_mutator_tag_edits_message *)sent[2])->size = 4500;
		for (piece = 0; piece < 3; piece++)
			network_mutators_handle_message(NONE, (word *)sent[piece], sent_sizes[piece]);
		CHECK(played_size == 0, "a piece past the size played");
		((struct distributed_mutator_tag_edits_message *)sent[0])->size = MAXIMUM_MUTATOR_TAG_EDIT_BYTES + 1;
		((struct distributed_mutator_tag_edits_message *)sent[0])->offset = 0;
		network_mutators_handle_message(NONE, (word *)sent[0], sent_sizes[0]);
		CHECK(played_size == 0, "a size past the most taken");
		/* (edits this build does not make: none played, none kept) */
		local_edits[0] = 0xEE;
		local_edit_size = 100;
		connection = _game_connection_network_server;
		mutators_new_game();
		CHECK(mutators_tag_edits(&edits) == 0, "edits refused, kept");
		CHECK(!mutators_set_host_tag_edits(local_edits, 100) && mutators_tag_edits(&edits) == 0,
			"the host's refused edits kept");
		CHECK(!mutators_set_host_tag_edits(local_edits, MAXIMUM_MUTATOR_TAG_EDIT_BYTES + 1), "too many bytes taken");
	}
	else if (!strcmp(case_name, "pose"))
	{
		struct real_orientation orientations[NUMBER_OF_NODES];
		real_vector3d none = { 0.f, 0.f, 0.f };
		real_vector3d down = { 0.f, 0.f, -0.08f };
		char reason[160];
		int index;

		/* (none: nothing posed, nothing grown) */
		posed(orientations);
		for (index = 0; index < NUMBER_OF_NODES; index++)
			CHECK(orientations[index].scale == 1.f, "a node posed with no pose");
		CHECK(object_pose_bounding_radius(0, 0.7f) == 0.7f && object_pose_render_radius(0, 0.9f) == 0.9f,
			"a sphere grown with no pose");
		/* (the big head: every biped's head) */
		CHECK(object_pose_add('bipd', "", "head", 3.f, &none, reason, sizeof(reason)), "the head refused: %s", reason);
		posed(orientations);
		CHECK(orientations[_node_head].scale == 3.f && orientations[_node_pelvis].scale == 1.f &&
			orientations[_node_upper_arm].scale == 1.f && orientations[_node_hand].scale == 1.f, "only the head scaled");
		/* (0.5 to the head, and its geometry's reach of 0.2 at the node's scale of 3) */
		CHECK(near(object_pose_bounding_radius(0, 0.7f), 1.1f), "the head outside the sphere");
		CHECK(object_pose_bounding_radius(0, 2.f) == 2.f, "a larger sphere shrunk");
		CHECK(near(object_pose_render_radius(0, 0.7f), 0.5f + 0.2f * 3.f * 1.25f), "the head culled");
		the_object.definition_index = 1;
		posed(orientations);
		CHECK(orientations[_node_head].scale == 1.f && object_pose_bounding_radius(0, 0.7f) == 0.7f,
			"the vehicle posed as a biped");
		the_object.definition_index = 0;
		the_object.object.node_matrices.size = _node_head * sizeof(real_matrix4x3);
		CHECK(object_pose_bounding_radius(0, 0.7f) == 0.7f, "a node past the object's matrices used");
		the_object.object.node_matrices.size = NUMBER_OF_NODES * sizeof(real_matrix4x3);
		/* (a node by name, in one tag of a parent group: what hangs from it held
		by the sphere, what is beside it not) */
		object_pose_clear();
		CHECK(object_pose_add('unit', "characters\\cyborg\\cyborg", "BIP01 L UPPERARM", 1.35f, &down, reason,
			sizeof(reason)), "the arm refused: %s", reason);
		posed(orientations);
		CHECK(orientations[_node_upper_arm].scale == 1.35f && orientations[_node_upper_arm].translation.z == -0.08f &&
			orientations[_node_hand].scale == 1.f && orientations[_node_head].scale == 1.f, "the arm's pose");
		CHECK(near(object_pose_bounding_radius(0, 0.1f), 0.6f + 0.05f), "the hand outside the sphere");
		/* (the same node posed again, by another mutator: both made) */
		CHECK(object_pose_add('bipd', "", "bip01 l upperarm", 2.f, &down, reason, sizeof(reason)), "again: %s", reason);
		posed(orientations);
		CHECK(near(orientations[_node_upper_arm].scale, 2.7f) && near(orientations[_node_upper_arm].translation.z,
			-0.16f), "both poses not made");
		/* (what no object has: refused, and why) */
		CHECK(!object_pose_add('bipd', "", "tail", 2.f, &none, reason, sizeof(reason)) && strstr(reason, "tail"),
			"a node no model has taken");
		CHECK(!object_pose_add('bipd', "characters\\elite\\elite", "head", 2.f, &none, reason, sizeof(reason)) &&
			strstr(reason, "no such tag"), "a tag the map has not taken");
		CHECK(!object_pose_add('vehi', "", "head", 2.f, &none, reason, sizeof(reason)), "a vehicle's head taken");
		/* (a biped's head is its own, whatever its model names it, and not past
		its nodes) */
		object_pose_clear();
		definitions[0].biped.runtime_head_node_index = NUMBER_OF_NODES;
		CHECK(!object_pose_add('bipd', "", "head", 2.f, &none, reason, sizeof(reason)), "a head past the nodes taken");
		/* (a model's tree that loops: each node once) */
		model_nodes[_node_hand].first_child_node_index = _node_upper_arm;
		CHECK(object_pose_add('bipd', "", "bip01 l upperarm", 2.f, &none, reason, sizeof(reason)), "a loop: %s",
			reason);
		posed(orientations);
		CHECK(orientations[_node_upper_arm].scale == 2.f, "a loop's pose");
		object_pose_clear();
		posed(orientations);
		CHECK(orientations[_node_upper_arm].scale == 1.f && object_pose_bounding_radius(0, 0.1f) == 0.1f,
			"posed after the poses were cleared");
		(void)reach_reads;
	}
	else
	{
		CHECK(0, "no case %s", case_name);
	}
	return 0;
}
