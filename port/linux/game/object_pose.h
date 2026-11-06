/*
OBJECT_POSE.H

Poses (object_pose.c): nodes of the objects of a group, or of one tag of it,
scaled and moved as their node orientations are made, from a mutator's
[[pose]] (docs/mutators.md).
*/

#ifndef OBJECT_POSE_H
#define OBJECT_POSE_H

/* ---------- constants */

/* the most an offset moves a node, a pose's in thousandths of a world unit on
the wire (mutator_tags.c), and a pose's scale */
#define OBJECT_POSE_MAXIMUM_OFFSET 1.f
#define OBJECT_POSE_MINIMUM_SCALE 0.25f
#define OBJECT_POSE_MAXIMUM_SCALE 4.f

/* ---------- prototypes/OBJECT_POSE.C */

/* the group a pose names ("biped", "unit", "object"), else NONE */
long object_pose_group_tag(char const *name);
/* (mutator_tags.c) none posed: a new game's poses are made again, a new
map's tags are others */
void object_pose_clear(void);
/* (mutator_tags.c) the node named posed in each object of the group (or the
tag of it named): its scale multiplied, its offset (in its parent's frame)
added. FALSE (said into reason) if no object of the map has it */
boolean object_pose_add(long group_tag, char const *tag_name, char const *node_name, real scale,
	real_vector3d const *offset, char *reason, unsigned long size);

/* (object_type_preprocess_node_orientations) an object's nodes posed */
void object_pose_node_orientations(long object_index, struct real_orientation *node_orientations);
/* (object_compute_node_matrices) an object's bounding radius, made to hold
its nodes as posed */
real object_pose_bounding_radius(long object_index, real bounding_radius);
/* (object_get_render_bounding_sphere) the radius an object is culled by,
made to hold its nodes as posed */
real object_pose_render_radius(long object_index, real render_radius);

#endif
