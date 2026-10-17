/*
PROPS.C

The content scenarios place (README.md): each a function that makes its tags
and places them in the scenario. Each value says where it comes from
(README.md, "Values").
*/

/* ---------- headers */

#include "stagekit.h"

/* ---------- constants */

enum
{
	/* (device_machines.c names a door, 0, which opens by itself, and a
	platform, 2, which cycles by itself; any other has no behaviour of its own
	and moves only where it is set) */
	MACHINE_TYPE_SET = 1,
};

/* ---------- structures */

/* (damage.c keeps these to itself: its layout, which its size check fixes) */
struct damage_region
{
	char name[TAG_STRING_LENGTH+1];
	unsigned long flags;
	long unused0;
	real damage_threshold;
	long unused1[3];
	struct tag_reference destroyed_effect;
	struct tag_block permutations;
};

struct damage_permutation
{
	char name[TAG_STRING_LENGTH+1];
};

typedef char damage_region_size_check[sizeof(struct damage_region) == 0x54 ? 1 : -1];

/* (and these, which their files keep to themselves: what is set here, the rest
zero, each its tag's size) */
struct model_geometry
{
	unsigned long flags;
	unsigned char unused[0x20];
	struct tag_block parts;
};
struct animation_node
{
	char name[TAG_STRING_LENGTH+1];
	short next_sibling_node_index;
	short first_child_node_index;
	short parent_node_index;
	unsigned char unused[0x1A];
};
struct device_animations
{
	unsigned char unused[0x54];
	struct tag_block animations;
};
struct scenery_placement
{
	struct scenario_object_datum object;
	unsigned char scenery[0x48 - sizeof(struct scenario_object_datum)];
};
struct machine_placement
{
	struct scenario_machine_datum machine;
	unsigned char unused[0x40 - sizeof(struct scenario_machine_datum)];
};
typedef char model_geometry_size_check[sizeof(struct model_geometry) == 0x30 ? 1 : -1];
typedef char animation_node_size_check[sizeof(struct animation_node) == 0x40 ? 1 : -1];
typedef char device_animations_size_check[sizeof(struct device_animations) == 0x60 ? 1 : -1];
typedef char scenery_placement_size_check[sizeof(struct scenery_placement) == 0x48 ? 1 : -1];
typedef char machine_placement_size_check[sizeof(struct machine_placement) == 0x40 ? 1 : -1];

/* ---------- public code */

void player_start(
	struct stage *stage,
	real_point3d const *position)
{
	struct player_starting_location *start = cache_block_add(&stage->scenario->players, sizeof(*start));

	/* (a little above it: where a player is placed falls onto what is there) */
	start->position = *position;
	start->position.z += 0.1f;
	start->structure_bsp_reference_index = 0;
}

/* a model that draws nothing, made once: an object without a model is
invisible, nothing collides with an invisible object (objects.c, collisions.c),
and a unit's animations read its model unchecked (object_start_interpolation) */
long prop_empty_model(
	void)
{
	static long handle = NONE;
	struct model *render;
	struct model_node *root;
	struct model_region *region;
	struct model_region_permutation *permutation;
	void *data;

	if (handle != NONE)
		return handle;
	handle = cache_tag_new(FOURCC('m', 'o', 'd', '2'), NONE, NONE, "stagekit\\empty", sizeof(struct model), &data);
	render = data;
	root = cache_block(&render->nodes, 1, sizeof(*root));
	strcpy(root->name, "root");
	root->next_sibling_node_index = root->first_child_node_index = root->parent_node_index = NONE;
	root->default_rotation.w = 1.f;
	root->runtime_default_inverse_matrix.scale = 1.f;
	root->runtime_default_inverse_matrix.forward.i = 1.f;
	root->runtime_default_inverse_matrix.left.j = 1.f;
	root->runtime_default_inverse_matrix.up.k = 1.f;
	region = cache_block(&render->regions, 1, sizeof(*region));
	strcpy(region->name, "box");
	permutation = cache_block(&region->permutations, 1, sizeof(*permutation));
	strcpy(permutation->name, "default");
	/* (one geometry of no parts: nothing to draw) */
	cache_block(&render->geometries, 1, sizeof(struct model_geometry));
	return handle;
}

/* how far a box's farthest corner is from its object's origin */
static real box_reach(
	real_rectangle3d const *bounds)
{
	real dx = MAX(-bounds->x0, bounds->x1), dy = MAX(-bounds->y0, bounds->y1), dz = MAX(-bounds->z0, bounds->z1);

	return square_root(dx * dx + dy * dy + dz * dz);
}

/* a collision model of one node: a solid box (bounds in its object's space),
whose bsp is the one its region's permutation picks (one region, one permutation) */
static long box_collision_model(
	char const *name,
	real_rectangle3d const *bounds)
{
	struct collision_model *model;
	struct collision_node *node;
	struct collision_bsp *box;
	struct damage_region *region;
	struct damage_permutation *permutation;
	void *data;
	long handle = cache_tag_new(FOURCC('c', 'o', 'l', 'l'), NONE, NONE, name, sizeof(struct collision_model), &data);

	model = data;
	node = cache_block(&model->nodes, 1, sizeof(*node));
	strcpy(node->name, "box");
	region = cache_block(&model->resistance.regions, 1, sizeof(*region));
	permutation = cache_block(&region->permutations, 1, sizeof(*permutation));
	strcpy(region->name, "box");
	strcpy(permutation->name, "default");
	model->resistance.indirect_damage_material_index = NONE;
	node->region_index = 0;
	node->parent_node_index = node->next_sibling_node_index = node->first_child_node_index = NONE;
	box = cache_block(&node->bsps, 1, sizeof(*box));
	box_build(box, cache_block, bounds, TRUE);
	model->pathfinding_box = *bounds;
	return handle;
}

void prop_obstacle(
	struct stage *stage,
	char const *name,
	real_rectangle3d const *bounds,
	real_point3d const *position)
{
	struct object_definition *scenery;
	struct scenario_object_palette_entry *palette;
	struct scenery_placement *placement;
	long model_handle, scenery_handle;
	void *data;

	model_handle = box_collision_model(name, bounds);

	scenery_handle = cache_tag_new(FOURCC('s', 'c', 'e', 'n'), FOURCC('o', 'b', 'j', 'e'), NONE, name,
		sizeof(struct object_definition), &data);
	scenery = data;
	scenery->object.type = _object_type_scenery;
	cache_reference(&scenery->object.model, prop_empty_model());
	cache_reference(&scenery->object.collision_model, model_handle);
	/* (round the box, a little past its farthest corner) */
	scenery->object.bounding_radius = box_reach(bounds) + 0.1f;

	palette = cache_block_add(&stage->scenario->scenery_palette, sizeof(*palette));
	cache_reference(&palette->reference, scenery_handle);
	placement = cache_block_add(&stage->scenario->scenery, sizeof(*placement));
	placement->object.palette_entry_index = (short)(stage->scenario->scenery_palette.count - 1);
	placement->object.name_index = NONE;
	placement->object.position = *position;
}

/* the name a scenario's console lines call an object it places by */
static short object_name_new(
	struct stage *stage,
	char const *name,
	short type,
	short placement_index)
{
	struct scenario_object_name *object_name = cache_block_add(&stage->scenario->object_names, sizeof(*object_name));

	if (strlen(name) >= sizeof(object_name->name))
	{
		fprintf(stderr, "stagekit: the object name %s is longer than %d characters\n", name,
			(int)sizeof(object_name->name) - 1);
		exit(1);
	}
	strcpy(object_name->name, name);
	object_name->runtime_object_type = type;
	object_name->runtime_scenario_datum_index = placement_index;
	return (short)(stage->scenario->object_names.count - 1);
}

void prop_elevator(
	struct stage *stage,
	char const *name,
	real_rectangle3d const *platform,
	real_point3d const *position,
	real rise,
	real seconds)
{
	enum { QUATERNION_ONE = 32767 };
	long frames = MAX(1, (long)(seconds * TICKS_PER_SECOND)) + 1;
	struct animation_graph *graph;
	struct device_animations *device;
	struct animation_graph_animation_index *slots;
	struct animation_node *node;
	struct animation *animation;
	struct machine_definition *machine;
	struct scenario_object_palette_entry *palette;
	struct machine_placement *placement;
	short *rotation;
	real *scale;
	real_point3d *translations;
	long graph_handle, machine_handle, frame;
	void *data;

	/* its rise: one animation of its one node, a frame a tick, which a device
	plays by its position (devices.c: the position slot) */
	graph_handle = cache_tag_new(FOURCC('a', 'n', 't', 'r'), NONE, NONE, name, sizeof(struct animation_graph), &data);
	graph = data;
	device = cache_block(&graph->device_animations, 1, sizeof(*device));
	slots = cache_block(&device->animations, 1, sizeof(*slots));
	slots[0].animation_index = 0;
	node = cache_block(&graph->nodes, 1, sizeof(*node));
	strcpy(node->name, "root");
	node->next_sibling_node_index = node->first_child_node_index = node->parent_node_index = NONE;
	animation = cache_block(&graph->animations, 1, sizeof(*animation));
	strcpy(animation->name, "position");
	/* (a device plays its animations over its nodes at rest: overlays, devices.c) */
	animation->type = _animation_overlay;
	animation->frame_count = (short)frames;
	animation->frame_size = sizeof(real_point3d);
	animation->node_count = 1;
	animation->next_animation_index = NONE;
	animation->sound_index = NONE;
	animation->nodes_with_translation_flags[0] = 1;
	/* (its rotation and scale, the same every frame: the identity) */
	animation->default_data.size = 4 * sizeof(short) + sizeof(real);
	rotation = animation->default_data.address = cache_allocate(animation->default_data.size);
	rotation[3] = QUATERNION_ONE;
	scale = (real *)(rotation + 4);
	*scale = 1.f;
	animation->data.size = frames * sizeof(real_point3d);
	translations = animation->data.address = cache_allocate(animation->data.size);
	for (frame = 0; frame < frames; frame++)
		translations[frame].z = rise * frame / (frames - 1);

	machine_handle = cache_tag_new(FOURCC('m', 'a', 'c', 'h'), FOURCC('d', 'e', 'v', 'i'), FOURCC('o', 'b', 'j', 'e'),
		name, sizeof(struct machine_definition), &data);
	machine = data;
	machine->object.type = _object_type_machine;
	/* (round the platform, wherever it is on its way) */
	machine->object.bounding_radius = box_reach(platform) + rise + 0.1f;
	cache_reference(&machine->object.model, prop_empty_model());
	cache_reference(&machine->object.collision_model, box_collision_model(name, platform));
	cache_reference(&machine->object.animation_graph, graph_handle);
	/* (moved only where it is set, a frame a tick: devices.c moves it by its
	runtime speeds, in position a tick, which a map carries made from its
	times, and reads nothing else of them) */
	machine->machine.type = MACHINE_TYPE_SET;
	machine->device.powered_position_transition_time = seconds;
	machine->device.depowered_position_transition_time = seconds;
	machine->device.runtime_maximum_powered_position_velocity = 1.f / (frames - 1);
	machine->device.runtime_maximum_powered_position_acceleration = 1.f / (frames - 1);
	machine->device.runtime_maximum_depowered_position_velocity = 1.f / (frames - 1);
	machine->device.runtime_maximum_depowered_position_acceleration = 1.f / (frames - 1);
	machine->machine.flags = FLAG(_machine_is_elevator_bit);
	machine->machine.elevator_node_index = 0;

	palette = cache_block_add(&stage->scenario->machine_palette, sizeof(*palette));
	cache_reference(&palette->reference, machine_handle);
	placement = cache_block_add(&stage->scenario->machines, sizeof(*placement));
	placement->machine.object.palette_entry_index = (short)(stage->scenario->machine_palette.count - 1);
	placement->machine.object.name_index = object_name_new(stage, name, _object_type_machine,
		(short)(stage->scenario->machines.count - 1));
	placement->machine.object.position = *position;
	placement->machine.device.power_group_index = NONE;
	placement->machine.device.position_group_index = NONE;
}
