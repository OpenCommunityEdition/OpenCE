/*
OBJECT_MESH.H

An object's drawn surface, as the game draws it (port/linux/game/object_mesh.c):
the triangles of its model's most detailed opaque geometry, in the
permutations the object shows, in the model's own space, each vertex bound
to the nodes that move it as the game draws it. first_person_legs.c finds
the waist in it.
*/

#ifndef __OBJECT_MESH_H
#define __OBJECT_MESH_H
#pragma once

enum
{
	OBJECT_MESH_MAXIMUM_NODES = 64,
	OBJECT_MESH_MAXIMUM_VERTICES = 16384,
	OBJECT_MESH_MAXIMUM_TRIANGLES = 24576,
};

struct object_mesh_vertex
{
	real_point3d position; /* the model's space (its default pose) */
	byte nodes[2];
	short weight; /* of nodes[0], of 32767 (the rest nodes[1]'s) */
};

struct object_mesh
{
	long key; /* NONE: none */
	long model_index;
	short node_count;
	long vertex_count;
	long triangle_count;
	struct object_mesh_vertex *vertices;
	unsigned short (*triangles)[3];
	/* each node's default inverse: the model's space to the node's */
	real_matrix4x3 inverse[OBJECT_MESH_MAXIMUM_NODES];
};

/* the object's mesh as it shows now, or NULL (no model, or none that can be
read). The last one read is kept, until another is asked for: its memory is
taken the first time one is */
struct object_mesh const *object_mesh_get(
	long object_index);

/* forget the mesh kept (a new map) */
void object_mesh_reset(
	void);

#endif // __OBJECT_MESH_H
