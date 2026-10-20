/*
RAGDOLL_BODIES.C

The bodies of the ragdolls (ragdoll_bodies.h). Every stock biped's
collision model holds, for most nodes of its model, a small convex piece
(a box, mostly) in the node's own frame: the thighs, the calves, the spine,
the head. Each node whose piece is big enough becomes a part, shaped as the
piece's bounds (a capsule along the bone when they are long, else a box),
weighing its share of the biped's mass by volume; the other nodes move with
their nearest ancestor's part. A root node without a piece (the jackal's
and the flood carrier's) gets a sphere between its children.

The joints are at each part's node's origin, twisting about the bone (the
node's x axis) and swinging about its y and z axes, with limits chosen by
the bone's name: knees and elbows bend about z and little else, hips and
shoulders swing widely, the spine stiffly.

port/assets/ragdolls/ragdolls.txt adjusts a model's ragdoll. Its sections
start at a line `model <tag path>`; each line after is one of

	none                    the model has no ragdoll
	mass <kg>               the whole body's mass
	part <node>             the node is a part, though its piece is small
	                        (or missing: then a sphere)
	fold <node>             the node moves with its parent's part
	joint <node>: twist <min> <max> swing <normal> <plane>    in degrees
	sphere <node>: <radius>
	capsule <node>: <radius> <half length>
	box <node>: <x> <y> <z>  half extents; sizes in world units
	break <node>: <damage>  an explosion doing this much damage at the
	                        part tears its joint (game.dismemberment)

`#` starts a comment. Map files are untrusted: every index and count is
checked, and a model whose nodes are out of order has no ragdoll.
*/

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "models/model_definitions.h"
#include "objects/object_definitions.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_model_definitions.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"
#include "ragdoll_bodies.h"

#include <string.h>

/* a part's piece is at least this share of the biped's volume */
#define RAGDOLL_PART_VOLUME_SHARE 0.02f
/* the smallest half extent of a shape (world units) */
#define RAGDOLL_SIZE_MIN 0.01f
/* a piece longer than this many times its width is a capsule */
#define RAGDOLL_CAPSULE_LENGTH 1.5f
/* (a break's override line, among the shapes') */
#define RAGDOLL_OVERRIDE_BREAK (NONE - 1)
/* a body's mass without an override (a marine's, kilograms) */
#define RAGDOLL_MASS_DEFAULT 80.0f
/* the lightest part, a share of the whole */
#define RAGDOLL_PART_MASS_SHARE 0.01f
#define RAGDOLL_OVERRIDE_LINE_MAX 128
#define RAGDOLL_NAME_MAX 32

/* a joint's limits by its bone's name, in degrees (the first match); a
limb's node is a part wherever it has a piece, however small */
struct ragdoll_joint_class
{
	char const *name;
	real twist;
	real swing_normal;
	real swing_plane;
	boolean limb;
};

static struct ragdoll_joint_class const ragdoll_joint_classes[] =
{
	/* hinges: bending about z */
	{"calf", 5.0f, 5.0f, 70.0f, TRUE},
	{"horselink", 5.0f, 5.0f, 70.0f, TRUE},
	{"forearm", 5.0f, 5.0f, 70.0f, TRUE},
	{"thigh", 20.0f, 50.0f, 50.0f, TRUE},
	{"upperarm", 40.0f, 80.0f, 80.0f, TRUE},
	{"clavicle", 10.0f, 20.0f, 20.0f, FALSE},
	{"spine", 15.0f, 20.0f, 20.0f, TRUE},
	{"neck", 40.0f, 40.0f, 40.0f, TRUE},
	{"head", 40.0f, 40.0f, 40.0f, TRUE},
	{"hand", 10.0f, 30.0f, 30.0f, TRUE},
	{"foot", 10.0f, 30.0f, 30.0f, TRUE},
	{"toe", 10.0f, 30.0f, 30.0f, FALSE},
	/* frames, flaps, spikes, sacks */
	{"", 10.0f, 20.0f, 20.0f, FALSE},
};

/* ---------- the overrides */

/* the next line of [*cursor, end), trimmed and without its comment, in
line; FALSE at the end. Lines too long are cut. */
static boolean ragdoll_override_line(
	char const **cursor,
	char const *end,
	char *line)
{
	char const *at = *cursor;
	long length = 0;
	long start = 0;

	if (at >= end)
		return FALSE;
	while (at < end && *at != '\n')
	{
		if (length < RAGDOLL_OVERRIDE_LINE_MAX - 1)
			line[length++] = *at;
		at++;
	}
	*cursor = at < end ? at + 1 : at;
	line[length] = 0;
	{
		char *comment = strchr(line, '#');

		if (comment)
			*comment = 0;
	}
	length = (long)strlen(line);
	while (length > 0 && (line[length - 1] == ' ' || line[length - 1] == '\t' || line[length - 1] == '\r'))
		line[--length] = 0;
	while (line[start] == ' ' || line[start] == '\t')
		start++;
	if (start)
		memmove(line, line + start, (size_t)(length - start + 1));
	return TRUE;
}

/* the line's word, and what follows it; FALSE if it does not start with it */
static boolean ragdoll_override_word(
	char const *line,
	char const *word,
	char const **rest)
{
	long length = (long)strlen(word);

	if (strncmp(line, word, (size_t)length) != 0 || (line[length] != ' ' && line[length] != '\t' && line[length] != 0))
		return FALSE;
	line += length;
	while (*line == ' ' || *line == '\t')
		line++;
	*rest = line;
	return TRUE;
}

/* a number (an optional sign, digits, an optional fraction), skipping the
spaces before it; FALSE if there is none */
static boolean ragdoll_override_number(
	char const **text,
	real *value)
{
	char const *at = *text;
	boolean negative = FALSE;
	boolean digits = FALSE;
	real result = 0.0f;
	real scale = 1.0f;

	while (*at == ' ' || *at == '\t')
		at++;
	if (*at == '-' || *at == '+')
		negative = *at++ == '-';
	while (*at >= '0' && *at <= '9')
	{
		result = 10.0f * result + (real)(*at++ - '0');
		digits = TRUE;
	}
	if (*at == '.')
	{
		at++;
		while (*at >= '0' && *at <= '9')
		{
			scale /= 10.0f;
			result += scale * (real)(*at++ - '0');
			digits = TRUE;
		}
	}
	if (!digits)
		return FALSE;
	*value = negative ? -result : result;
	*text = at;
	return TRUE;
}

/* a line's node name, up to its ':' (or the line's end); the node's index,
or NONE */
static short ragdoll_override_node(
	struct tag_block const *model_nodes,
	char const *text,
	char const **rest)
{
	char name[RAGDOLL_NAME_MAX];
	long length = 0;
	short node_index;

	while (text[length] && text[length] != ':')
		length++;
	*rest = text[length] == ':' ? text + length + 1 : text + length;
	while (length > 0 && (text[length - 1] == ' ' || text[length - 1] == '\t'))
		length--;
	if (length <= 0 || length >= RAGDOLL_NAME_MAX)
		return NONE;
	memcpy(name, text, (size_t)length);
	name[length] = 0;
	for (node_index = 0; node_index < model_nodes->count && node_index < RAGDOLL_BODY_NODES_MAX; node_index++)
	{
		struct model_node const *node = TAG_BLOCK_GET_ELEMENT(model_nodes, node_index, struct model_node);
		char node_name[RAGDOLL_NAME_MAX];

		memcpy(node_name, node->name, RAGDOLL_NAME_MAX - 1);
		node_name[RAGDOLL_NAME_MAX - 1] = 0;
		if (!csstrcasecmp(node_name, name))
			return node_index;
	}
	return NONE;
}

/* the model's section of the overrides: [*start, *end); FALSE if it has none */
static boolean ragdoll_override_section(
	char const *overrides,
	long overrides_size,
	char const *model_name,
	char const **start,
	char const **end)
{
	char const *cursor = overrides;
	char const *limit = overrides + (overrides_size > 0 ? overrides_size : 0);
	char line[RAGDOLL_OVERRIDE_LINE_MAX];
	boolean inside = FALSE;

	if (!overrides || !model_name)
		return FALSE;
	for (;;)
	{
		char const *at = cursor;
		char const *rest;

		if (!ragdoll_override_line(&cursor, limit, line))
		{
			if (inside)
				*end = limit;
			return inside;
		}
		if (ragdoll_override_word(line, "model", &rest))
		{
			if (inside)
			{
				*end = at;
				return TRUE;
			}
			if (!csstrcasecmp(rest, model_name))
			{
				inside = TRUE;
				*start = cursor;
			}
		}
	}
}

/* ---------- the body */

/* the bounds of a collision node's first bsp's vertices; FALSE if it has
none */
static boolean ragdoll_node_bounds(
	struct collision_node const *collision_node,
	real_point3d *minimum,
	real_point3d *maximum)
{
	struct collision_bsp const *bsp;
	long vertex_index;

	if (collision_node->bsps.count <= 0)
		return FALSE;
	bsp = TAG_BLOCK_GET_ELEMENT(&collision_node->bsps, 0, struct collision_bsp);
	if (bsp->vertices.count <= 0)
		return FALSE;
	for (vertex_index = 0; vertex_index < bsp->vertices.count; vertex_index++)
	{
		struct collision_vertex const *vertex =
			TAG_BLOCK_GET_ELEMENT(&bsp->vertices, vertex_index, struct collision_vertex);
		real_point3d const *point = &vertex->point;

		if (point->x != point->x || point->y != point->y || point->z != point->z ||
			point->x < -100.0f || point->x > 100.0f || point->y < -100.0f || point->y > 100.0f ||
			point->z < -100.0f || point->z > 100.0f)
			return FALSE;
		if (vertex_index == 0)
		{
			*minimum = *maximum = *point;
			continue;
		}
		minimum->x = point->x < minimum->x ? point->x : minimum->x;
		minimum->y = point->y < minimum->y ? point->y : minimum->y;
		minimum->z = point->z < minimum->z ? point->z : minimum->z;
		maximum->x = point->x > maximum->x ? point->x : maximum->x;
		maximum->y = point->y > maximum->y ? point->y : maximum->y;
		maximum->z = point->z > maximum->z ? point->z : maximum->z;
	}
	return TRUE;
}

/* the collision node of a model node's name, or NULL */
static struct collision_node const *ragdoll_collision_node(
	struct tag_block const *collision_nodes,
	struct model_node const *node)
{
	char name[RAGDOLL_NAME_MAX];
	long index;

	if (!collision_nodes)
		return NULL;
	memcpy(name, node->name, RAGDOLL_NAME_MAX - 1);
	name[RAGDOLL_NAME_MAX - 1] = 0;
	for (index = 0; index < collision_nodes->count; index++)
	{
		struct collision_node const *collision_node =
			TAG_BLOCK_GET_ELEMENT(collision_nodes, index, struct collision_node);
		char collision_name[RAGDOLL_NAME_MAX];

		memcpy(collision_name, collision_node->name, RAGDOLL_NAME_MAX - 1);
		collision_name[RAGDOLL_NAME_MAX - 1] = 0;
		if (!csstrcasecmp(collision_name, name))
			return collision_node;
	}
	return NULL;
}

static real ragdoll_extent(
	real minimum,
	real maximum)
{
	real half = 0.5f * (maximum - minimum);

	return half > RAGDOLL_SIZE_MIN ? half : RAGDOLL_SIZE_MIN;
}

/* a part's shape from its piece's bounds */
static void ragdoll_part_shape_from_bounds(
	struct ragdoll_part *part,
	real_point3d const *minimum,
	real_point3d const *maximum)
{
	real x = ragdoll_extent(minimum->x, maximum->x);
	real y = ragdoll_extent(minimum->y, maximum->y);
	real z = ragdoll_extent(minimum->z, maximum->z);
	real width = y > z ? y : z;

	part->center.x = 0.5f * (minimum->x + maximum->x);
	part->center.y = 0.5f * (minimum->y + maximum->y);
	part->center.z = 0.5f * (minimum->z + maximum->z);
	if (x > RAGDOLL_CAPSULE_LENGTH * width)
	{
		real radius = 0.5f * (y + z);

		part->shape = _ragdoll_shape_capsule;
		part->size[0] = radius;
		part->size[1] = x - radius > 0.0f ? x - radius : 0.0f;
		part->size[2] = 0.0f;
	}
	else
	{
		part->shape = _ragdoll_shape_box;
		part->size[0] = x;
		part->size[1] = y;
		part->size[2] = z;
	}
}

/* a sphere at a node's origin, half as far across as its furthest child */
static void ragdoll_part_shape_between_children(
	struct ragdoll_part *part,
	struct tag_block const *model_nodes,
	short node_index)
{
	real radius = 0.0f;
	short child_index;

	for (child_index = node_index + 1; child_index < model_nodes->count && child_index < RAGDOLL_BODY_NODES_MAX;
		child_index++)
	{
		struct model_node const *child = TAG_BLOCK_GET_ELEMENT(model_nodes, child_index, struct model_node);
		real distance;

		if (child->parent_node_index != node_index)
			continue;
		distance = square_root(child->default_translation.x * child->default_translation.x +
			child->default_translation.y * child->default_translation.y +
			child->default_translation.z * child->default_translation.z);
		radius = 0.5f * distance > radius ? 0.5f * distance : radius;
	}
	part->shape = _ragdoll_shape_sphere;
	part->size[0] = radius > 2.0f * RAGDOLL_SIZE_MIN ? radius : 2.0f * RAGDOLL_SIZE_MIN;
	part->size[1] = part->size[2] = 0.0f;
	part->center.x = part->center.y = part->center.z = 0.0f;
}

static real ragdoll_part_volume(
	struct ragdoll_part const *part)
{
	real r = part->size[0];

	switch (part->shape)
	{
	case _ragdoll_shape_sphere:
		return (4.0f / 3.0f) * _pi * r * r * r;
	case _ragdoll_shape_capsule:
		return _pi * r * r * ((4.0f / 3.0f) * r + 2.0f * part->size[1]);
	default:
		return 8.0f * part->size[0] * part->size[1] * part->size[2];
	}
}

/* the joint class of a bone's name */
static struct ragdoll_joint_class const *ragdoll_joint_class_get(
	char const *node_name)
{
	char name[RAGDOLL_NAME_MAX];
	short index;
	short class_index;

	for (index = 0; index < RAGDOLL_NAME_MAX - 1 && node_name[index]; index++)
		name[index] = node_name[index] >= 'A' && node_name[index] <= 'Z' ? node_name[index] - 'A' + 'a' : node_name[index];
	name[index] = 0;
	for (class_index = 0; ragdoll_joint_classes[class_index].name[0]; class_index++)
	{
		if (strstr(name, ragdoll_joint_classes[class_index].name))
			break;
	}
	return &ragdoll_joint_classes[class_index];
}

static void ragdoll_part_limits(
	struct ragdoll_part *part,
	struct ragdoll_joint_class const *joint_class)
{
	part->twist_min = DEGREES_TO_RADIANS(-joint_class->twist);
	part->twist_max = DEGREES_TO_RADIANS(joint_class->twist);
	part->swing_normal = DEGREES_TO_RADIANS(joint_class->swing_normal);
	part->swing_plane = DEGREES_TO_RADIANS(joint_class->swing_plane);
}

/* an override's angle in degrees, as radians within [low, high] */
static boolean ragdoll_override_angle(
	char const **text,
	real low,
	real high,
	real *angle)
{
	real degrees;

	if (!ragdoll_override_number(text, &degrees) || degrees < low || degrees > high)
		return FALSE;
	*angle = DEGREES_TO_RADIANS(degrees);
	return TRUE;
}

/* a joint's, a shape's or a break's override line, applied to the part of
its node */
static void ragdoll_override_part(
	struct ragdoll_body *body,
	struct tag_block const *model_nodes,
	char const *line)
{
	char const *rest;
	char const *text;
	short node_index;
	struct ragdoll_part *part;
	struct ragdoll_part changed;
	short shape;
	short sizes;
	short size_index;

	if (ragdoll_override_word(line, "joint", &rest))
		shape = NONE;
	else if (ragdoll_override_word(line, "sphere", &rest))
		shape = _ragdoll_shape_sphere;
	else if (ragdoll_override_word(line, "capsule", &rest))
		shape = _ragdoll_shape_capsule;
	else if (ragdoll_override_word(line, "box", &rest))
		shape = _ragdoll_shape_box;
	else if (ragdoll_override_word(line, "break", &rest))
		shape = RAGDOLL_OVERRIDE_BREAK;
	else
		return;
	node_index = ragdoll_override_node(model_nodes, rest, &text);
	if (node_index == NONE || body->node_parts[node_index] < 0 ||
		body->parts[body->node_parts[node_index]].node_index != node_index)
		return;
	part = &body->parts[body->node_parts[node_index]];
	changed = *part;
	if (shape == RAGDOLL_OVERRIDE_BREAK)
	{
		/* (the root has no joint to tear) */
		if (part->parent_part_index == NONE || !ragdoll_override_number(&text, &changed.break_damage) ||
			changed.break_damage < 1.0f || changed.break_damage > 100000.0f)
			return;
	}
	else if (shape == NONE)
	{
		while (*text == ' ' || *text == '\t')
			text++;
		if (!ragdoll_override_word(text, "twist", &text) ||
			!ragdoll_override_angle(&text, -180.0f, 180.0f, &changed.twist_min) ||
			!ragdoll_override_angle(&text, -180.0f, 180.0f, &changed.twist_max) ||
			changed.twist_min > changed.twist_max)
			return;
		while (*text == ' ' || *text == '\t')
			text++;
		if (!ragdoll_override_word(text, "swing", &text) ||
			!ragdoll_override_angle(&text, 0.0f, 180.0f, &changed.swing_normal) ||
			!ragdoll_override_angle(&text, 0.0f, 180.0f, &changed.swing_plane))
			return;
	}
	else
	{
		sizes = shape == _ragdoll_shape_sphere ? 1 : shape == _ragdoll_shape_capsule ? 2 : 3;
		changed.shape = shape;
		changed.size[0] = changed.size[1] = changed.size[2] = 0.0f;
		for (size_index = 0; size_index < sizes; size_index++)
		{
			if (!ragdoll_override_number(&text, &changed.size[size_index]) ||
				changed.size[size_index] < (shape == _ragdoll_shape_capsule && size_index == 1 ? 0.0f : RAGDOLL_SIZE_MIN) ||
				changed.size[size_index] > 10.0f)
				return;
		}
	}
	*part = changed;
}

boolean ragdoll_body_build(
	char const *model_name,
	struct tag_block const *model_nodes,
	struct tag_block const *collision_nodes,
	char const *overrides,
	long overrides_size,
	struct ragdoll_body *body)
{
	char const *section = NULL;
	char const *section_end = NULL;
	boolean has_section;
	char const *cursor;
	char line[RAGDOLL_OVERRIDE_LINE_MAX];
	boolean has_bounds[RAGDOLL_BODY_NODES_MAX];
	real_point3d minimums[RAGDOLL_BODY_NODES_MAX];
	real_point3d maximums[RAGDOLL_BODY_NODES_MAX];
	real volumes[RAGDOLL_BODY_NODES_MAX];
	/* 1 a part, -1 folded, 0 as its piece says */
	signed char forced[RAGDOLL_BODY_NODES_MAX];
	real mass_volumes[RAGDOLL_BODY_PARTS_MAX];
	real total_volume = 0.0f;
	real total_mass_volume = 0.0f;
	short node_index;
	short part_index;

	memset(body, 0, sizeof(*body));
	if (!model_nodes || model_nodes->count < 2 || model_nodes->count > RAGDOLL_BODY_NODES_MAX)
		return FALSE;
	body->node_count = (short)model_nodes->count;
	body->mass = RAGDOLL_MASS_DEFAULT;
	memset(forced, 0, sizeof(forced));

	/* the nodes, each after its parent */
	for (node_index = 0; node_index < body->node_count; node_index++)
	{
		struct model_node const *node = TAG_BLOCK_GET_ELEMENT(model_nodes, node_index, struct model_node);
		struct collision_node const *collision_node = ragdoll_collision_node(collision_nodes, node);

		if (node_index == 0 ? node->parent_node_index != NONE :
			node->parent_node_index < 0 || node->parent_node_index >= node_index)
			return FALSE;
		has_bounds[node_index] = collision_node &&
			ragdoll_node_bounds(collision_node, &minimums[node_index], &maximums[node_index]);
		volumes[node_index] = 0.0f;
		if (has_bounds[node_index])
		{
			volumes[node_index] = 8.0f * ragdoll_extent(minimums[node_index].x, maximums[node_index].x) *
				ragdoll_extent(minimums[node_index].y, maximums[node_index].y) *
				ragdoll_extent(minimums[node_index].z, maximums[node_index].z);
			total_volume += volumes[node_index];
		}
	}
	if (total_volume <= 0.0f)
		return FALSE;

	/* the overrides that choose the parts */
	has_section = ragdoll_override_section(overrides, overrides_size, model_name, &section, &section_end);
	cursor = section;
	while (has_section && ragdoll_override_line(&cursor, section_end, line))
	{
		char const *rest;
		char const *after;
		real mass;

		if (!strcmp(line, "none"))
			return FALSE;
		if (ragdoll_override_word(line, "mass", &rest))
		{
			if (ragdoll_override_number(&rest, &mass) && mass >= 1.0f && mass <= 10000.0f)
				body->mass = mass;
		}
		else if (ragdoll_override_word(line, "part", &rest))
		{
			node_index = ragdoll_override_node(model_nodes, rest, &after);
			if (node_index != NONE)
				forced[node_index] = 1;
		}
		else if (ragdoll_override_word(line, "fold", &rest))
		{
			node_index = ragdoll_override_node(model_nodes, rest, &after);
			if (node_index > 0)
				forced[node_index] = -1;
		}
	}

	/* the parts */
	for (node_index = 0; node_index < body->node_count; node_index++)
	{
		struct model_node const *node = TAG_BLOCK_GET_ELEMENT(model_nodes, node_index, struct model_node);
		struct ragdoll_joint_class const *joint_class = ragdoll_joint_class_get(node->name);
		boolean big = has_bounds[node_index] &&
			(joint_class->limb || volumes[node_index] >= RAGDOLL_PART_VOLUME_SHARE * total_volume);
		boolean is_part = node_index == 0 || (forced[node_index] ? forced[node_index] > 0 : big);
		struct ragdoll_part *part;

		if (!is_part || body->part_count >= RAGDOLL_BODY_PARTS_MAX)
		{
			part_index = body->node_parts[node->parent_node_index];
			body->node_parts[node_index] = (signed char)part_index;
			mass_volumes[part_index] += volumes[node_index];
			total_mass_volume += volumes[node_index];
			continue;
		}
		part_index = body->part_count++;
		part = &body->parts[part_index];
		part->node_index = node_index;
		part->parent_part_index = node_index == 0 ? NONE : body->node_parts[node->parent_node_index];
		if (has_bounds[node_index])
			ragdoll_part_shape_from_bounds(part, &minimums[node_index], &maximums[node_index]);
		else
			ragdoll_part_shape_between_children(part, model_nodes, node_index);
		ragdoll_part_limits(part, joint_class);
		body->node_parts[node_index] = (signed char)part_index;
		mass_volumes[part_index] = has_bounds[node_index] ? volumes[node_index] : ragdoll_part_volume(part);
		total_mass_volume += mass_volumes[part_index];
	}
	if (body->part_count < 2)
		return FALSE;

	/* the overrides of the joints and the shapes */
	cursor = section;
	while (has_section && ragdoll_override_line(&cursor, section_end, line))
		ragdoll_override_part(body, model_nodes, line);

	/* the masses, by volume */
	for (part_index = 0; part_index < body->part_count; part_index++)
	{
		real mass = body->mass * mass_volumes[part_index] / total_mass_volume;
		real lightest = RAGDOLL_PART_MASS_SHARE * body->mass;

		body->parts[part_index].mass = mass > lightest ? mass : lightest;
	}
	return TRUE;
}

/* ---------- the bodies of the map's bipeds */

/* port/assets/ragdolls/ragdolls.txt (tools/embed_assets.py) */
extern const unsigned int ragdoll_overrides_embedded[];
extern const unsigned long ragdoll_overrides_embedded_size;

enum
{
	RAGDOLL_BODIES_CACHED = 64,
};

static struct
{
	long count;
	long definition_indices[RAGDOLL_BODIES_CACHED];
	boolean made[RAGDOLL_BODIES_CACHED];
	struct ragdoll_body bodies[RAGDOLL_BODIES_CACHED];
} ragdoll_bodies;

struct ragdoll_body const *ragdoll_body_get(
	long definition_index)
{
	struct object_definition const *definition;
	long model_index;
	long collision_model_index;
	struct model const *model;
	struct tag_block const *collision_nodes = NULL;
	long index;

	for (index = 0; index < ragdoll_bodies.count; index++)
	{
		if (ragdoll_bodies.definition_indices[index] == definition_index)
			return ragdoll_bodies.made[index] ? &ragdoll_bodies.bodies[index] : NULL;
	}
	if (definition_index == NONE || ragdoll_bodies.count >= RAGDOLL_BODIES_CACHED)
		return NULL;
	index = ragdoll_bodies.count++;
	ragdoll_bodies.definition_indices[index] = definition_index;
	ragdoll_bodies.made[index] = FALSE;
	definition = object_definition_get(definition_index);
	model_index = definition->object.model.index;
	collision_model_index = definition->object.collision_model.index;
	if (model_index == NONE || collision_model_index == NONE)
		return NULL;
	model = model_definition_get(model_index);
	collision_nodes = &collision_model_definition_get(collision_model_index)->nodes;
	ragdoll_bodies.made[index] = ragdoll_body_build(tag_get_name(model_index), &model->nodes, collision_nodes,
		(char const *)ragdoll_overrides_embedded, (long)ragdoll_overrides_embedded_size, &ragdoll_bodies.bodies[index]);
	return ragdoll_bodies.made[index] ? &ragdoll_bodies.bodies[index] : NULL;
}

/* ---------- the hulls of the map's vehicles */

enum
{
	RAGDOLL_HULLS_CACHED = 64,
};

static struct
{
	long count;
	long definition_indices[RAGDOLL_HULLS_CACHED];
	boolean made[RAGDOLL_HULLS_CACHED];
	real_point3d centers[RAGDOLL_HULLS_CACHED];
	real_vector3d half_extents[RAGDOLL_HULLS_CACHED];
} ragdoll_hulls;

boolean ragdoll_hull_get(
	long definition_index,
	real_point3d *center,
	real_vector3d *half_extents)
{
	struct object_definition const *definition;
	struct collision_model const *collision_model;
	real_point3d minimum;
	real_point3d maximum;
	long index;

	for (index = 0; index < ragdoll_hulls.count; index++)
	{
		if (ragdoll_hulls.definition_indices[index] == definition_index)
			break;
	}
	if (index == ragdoll_hulls.count)
	{
		if (definition_index == NONE || ragdoll_hulls.count >= RAGDOLL_HULLS_CACHED)
			return FALSE;
		ragdoll_hulls.count++;
		ragdoll_hulls.definition_indices[index] = definition_index;
		ragdoll_hulls.made[index] = FALSE;
		definition = object_definition_get(definition_index);
		if (definition->object.collision_model.index != NONE)
		{
			collision_model = collision_model_definition_get(definition->object.collision_model.index);
			if (collision_model->nodes.count > 0 &&
				ragdoll_node_bounds(TAG_BLOCK_GET_ELEMENT(&collision_model->nodes, 0, struct collision_node),
					&minimum, &maximum))
			{
				ragdoll_hulls.made[index] = TRUE;
				ragdoll_hulls.centers[index].x = 0.5f * (minimum.x + maximum.x);
				ragdoll_hulls.centers[index].y = 0.5f * (minimum.y + maximum.y);
				ragdoll_hulls.centers[index].z = 0.5f * (minimum.z + maximum.z);
				ragdoll_hulls.half_extents[index].i = 0.5f * (maximum.x - minimum.x);
				ragdoll_hulls.half_extents[index].j = 0.5f * (maximum.y - minimum.y);
				ragdoll_hulls.half_extents[index].k = 0.5f * (maximum.z - minimum.z);
			}
		}
	}
	if (!ragdoll_hulls.made[index])
		return FALSE;
	*center = ragdoll_hulls.centers[index];
	*half_extents = ragdoll_hulls.half_extents[index];
	return TRUE;
}

void ragdoll_bodies_dispose_from_old_map(void)
{
	ragdoll_bodies.count = 0;
	ragdoll_hulls.count = 0;
}
