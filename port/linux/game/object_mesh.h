/*
OBJECT_MESH.H

An object's drawn surface, as the game draws it (port/linux/game/object_mesh.c):
the triangles of its model's most detailed geometry, in the permutations the
object shows, in the model's own space, each vertex bound to the nodes that
move it as the game draws it.
*/

#ifndef __OBJECT_MESH_H
#define __OBJECT_MESH_H
#pragma once

enum
{
	OBJECT_MESH_MAXIMUM_NODES = 64,
	OBJECT_MESH_MAXIMUM_VERTICES = 16384,
	OBJECT_MESH_MAXIMUM_TRIANGLES = 24576,

	/* (a triangle no marks go on: a visor, glass) */
	OBJECT_MESH_TRIANGLE_NO_MARKS = 1,
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
	/* which way its triangles' normals (b - a) x (c - a) point: 1 out of the
	model, -1 into it (the sign of the volume they enclose) */
	short outward;
	long vertex_count;
	long triangle_count;
	struct object_mesh_vertex *vertices;
	unsigned short (*triangles)[3];
	byte *triangle_flags; /* OBJECT_MESH_TRIANGLE_... */
	/* each node's default inverse: the model's space to the node's */
	real_matrix4x3 inverse[OBJECT_MESH_MAXIMUM_NODES];
};

/* the object's mesh as it shows now (cached), or NULL (no model, or none
that can be read) */
struct object_mesh const *object_mesh_get(
	long object_index);

/* (for debugging) what of the object's model could be read, and why not */
char const *object_mesh_report(
	long object_index);

/* forget every mesh (a new map) */
void object_mesh_reset(
	void);

#endif // __OBJECT_MESH_H
