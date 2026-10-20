/*
RAGDOLL_BODIES.H

A dead biped's ragdoll as the host's Jolt world builds it
(port/macos/include/host_physics.h): its parts (one body each, following a
node of the model), their shapes, masses and joints. Made from the model's
nodes and the collision model's geometry for each node, and adjusted by
port/assets/ragdolls/ragdolls.txt.
*/

#ifndef __RAGDOLL_BODIES_H
#define __RAGDOLL_BODIES_H

struct tag_block;

enum
{
	RAGDOLL_BODY_PARTS_MAX = 32,
	RAGDOLL_BODY_NODES_MAX = 64,
};

/* (the host's shape types) */
enum
{
	_ragdoll_shape_sphere,
	/* along the node's x axis */
	_ragdoll_shape_capsule,
	_ragdoll_shape_box,
};

struct ragdoll_part
{
	/* the node whose frame the part's body is; the joint to its parent part
	is at the node's origin */
	short node_index;
	short parent_part_index;
	short shape;
	short pad;
	/* a sphere's radius; a capsule's radius and half its cylinder's length;
	a box's half extents (world units) */
	real size[3];
	/* the shape's centre, in the node's frame */
	real_point3d center;
	/* kilograms */
	real mass;
	/* radians: the twist about the node's x axis, and half the swing's cone
	about the normal (y) and about the plane axis (z) */
	real twist_min;
	real twist_max;
	real swing_normal;
	real swing_plane;
};

struct ragdoll_body
{
	short part_count;
	short node_count;
	/* kilograms, all the parts */
	real mass;
	/* the part each node moves with: its own, or the nearest ancestor's */
	signed char node_parts[RAGDOLL_BODY_NODES_MAX];
	struct ragdoll_part parts[RAGDOLL_BODY_PARTS_MAX];
};

/* The ragdoll of a model (its tag path, its nodes, and its collision model's
nodes, matched by name), with the overrides of its section of the text
given; FALSE if it has none: too few nodes with geometry, nodes out of
order, or an override that says so. */
boolean ragdoll_body_build(char const *model_name, struct tag_block const *model_nodes,
	struct tag_block const *collision_nodes, char const *overrides, long overrides_size,
	struct ragdoll_body *body);

/* the ragdoll of a biped's definition (made once a map, with the embedded
overrides), or NULL */
struct ragdoll_body const *ragdoll_body_get(long definition_index);
/* A vehicle's hull, for the ragdolls to meet: the box of its collision
model's first node's geometry (its centre and half extents, in the first
node's frame), made once a map; FALSE if it has none. */
boolean ragdoll_hull_get(long definition_index, real_point3d *center, real_vector3d *half_extents);
void ragdoll_bodies_dispose_from_old_map(void);

#endif
