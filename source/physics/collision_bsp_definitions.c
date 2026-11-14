/* ---------- headers */

#include "cseries/cseries.h"
#include "physics/collision_bsp_definitions.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct collision_bsp_flags_definition
{
	long count;
	char **names;
};

struct collision_bsp_definition_data
{
	struct tag_field bsp3d_node_fields[4];
	struct tag_block_definition bsp3d_node_block;
	struct tag_field plane_fields[2];
	struct tag_block_definition plane_block;
	char *leaf_flag_names[1];
	struct collision_bsp_flags_definition leaf_flags;
	struct tag_field leaf_fields[4];
	struct tag_block_definition leaf_block;
	struct tag_field bsp2d_node_fields[4];
	struct tag_block_definition bsp2d_node_block;
	struct tag_field bsp2d_reference_fields[3];
	struct tag_block_definition bsp2d_reference_block;
	char *surface_flag_names[4];
	struct collision_bsp_flags_definition surface_flags;
	long pad0;
	struct tag_field surface_fields[6];
	struct tag_block_definition surface_block;
	long pad1;
	struct tag_field edge_fields[7];
	struct tag_block_definition edge_block;
	struct tag_field vertex_fields[3];
	struct tag_block_definition vertex_block;
};

typedef char collision_bsp_definition_data_size_assert[
	sizeof(struct collision_bsp_definition_data) == 0x318 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

static struct collision_bsp_definition_data global_collision_bsp_definition_data =
{
	{
		{ _tag_field_long_integer, 0, "plane*", NULL },
		{ _tag_field_long_integer, 0, "back child*", NULL },
		{ _tag_field_long_integer, 0, "front child*", NULL },
		{ _tag_field_terminator, 0, NULL, NULL },
	},
	{
		"bsp3d node", 0, 0x20000, sizeof(struct bsp3d_node), NULL,
		global_collision_bsp_definition_data.bsp3d_node_fields, NULL, NULL, NULL, NULL, NULL,
	},
	{
		{ _tag_field_real_plane3d, 0, "plane*", NULL },
		{ _tag_field_terminator, 0, NULL, NULL },
	},
	{
		"plane", 0, 0x10000, sizeof(real_plane3d), NULL,
		global_collision_bsp_definition_data.plane_fields, NULL, NULL, NULL, NULL, NULL,
	},
	{ "contains double-sided surfaces" },
	{ 1, global_collision_bsp_definition_data.leaf_flag_names },
	{
		{ _tag_field_word_flags, 0, "flags*", &global_collision_bsp_definition_data.leaf_flags },
		{ _tag_field_short_integer, 0, "bsp2d reference count*", NULL },
		{ _tag_field_long_integer, 0, "first bsp2d reference*", NULL },
		{ _tag_field_terminator, 0, NULL, NULL },
	},
	{
		"leaf", 0, 0x10000, 8, NULL,
		global_collision_bsp_definition_data.leaf_fields, NULL, NULL, NULL, NULL, NULL,
	},
	{
		{ _tag_field_real_plane2d, 0, "plane*", NULL },
		{ _tag_field_long_integer, 0, "left child*", NULL },
		{ _tag_field_long_integer, 0, "right child*", NULL },
		{ _tag_field_terminator, 0, NULL, NULL },
	},
	{
		"bsp2d node", 0, 0xFFFF, sizeof(struct bsp2d_node), NULL,
		global_collision_bsp_definition_data.bsp2d_node_fields, NULL, NULL, NULL, NULL, NULL,
	},
	{
		{ _tag_field_long_integer, 0, "plane*", &global_collision_bsp_definition_data.plane_block },
		{ _tag_field_long_integer, 0, "bsp2d node*", &global_collision_bsp_definition_data.bsp2d_node_block },
		{ _tag_field_terminator, 0, NULL, NULL },
	},
	{
		"bsp2d reference", 0, 0x20000, 8, NULL,
		global_collision_bsp_definition_data.bsp2d_reference_fields, NULL, NULL, NULL, NULL, NULL,
	},
	{ "two sided", "invisible", "climbable", "breakable" },
	{ 4, global_collision_bsp_definition_data.surface_flag_names },
	0,
	{
		{ _tag_field_long_integer, 0, "plane*", NULL },
		{ _tag_field_long_integer, 0, "first edge*", NULL },
		{ _tag_field_byte_flags, 0, "flags*", &global_collision_bsp_definition_data.surface_flags },
		{ _tag_field_char_integer, 0, "breakable surface*", NULL },
		{ _tag_field_short_integer, 0, "material*", NULL },
		{ _tag_field_terminator, 0, NULL, NULL },
	},
	{
		"surface", 0, 0x20000, sizeof(struct collision_surface), NULL,
		global_collision_bsp_definition_data.surface_fields, NULL, NULL, NULL, NULL, NULL,
	},
	0,
	{
		{ _tag_field_long_integer, 0, "start vertex*", NULL },
		{ _tag_field_long_integer, 0, "end vertex*", NULL },
		{ _tag_field_long_integer, 0, "forward edge*", NULL },
		{ _tag_field_long_integer, 0, "reverse edge*", NULL },
		{ _tag_field_long_integer, 0, "left surface*", NULL },
		{ _tag_field_long_integer, 0, "right surface*", NULL },
		{ _tag_field_terminator, 0, NULL, NULL },
	},
	{
		"edge", 0, 0x40000, sizeof(struct collision_edge), NULL,
		global_collision_bsp_definition_data.edge_fields, NULL, NULL, NULL, NULL, NULL,
	},
	{
		{ _tag_field_real_point3d, 0, "point*", NULL },
		{ _tag_field_long_integer, 0, "first edge*", NULL },
		{ _tag_field_terminator, 0, NULL, NULL },
	},
	{
		"vertex", 0, 0x20000, sizeof(struct collision_vertex), NULL,
		global_collision_bsp_definition_data.vertex_fields, NULL, NULL, NULL, NULL, NULL,
	},
};

struct tag_field global_collision_bsp_fields[9] =
{
	{ _tag_field_block, 0, "bsp3d nodes*", &global_collision_bsp_definition_data.bsp3d_node_block },
	{ _tag_field_block, 0, "planes*", &global_collision_bsp_definition_data.plane_block },
	{ _tag_field_block, 0, "leaves*", &global_collision_bsp_definition_data.leaf_block },
	{ _tag_field_block, 0, "bsp2d references*", &global_collision_bsp_definition_data.bsp2d_reference_block },
	{ _tag_field_block, 0, "bsp2d nodes*", &global_collision_bsp_definition_data.bsp2d_node_block },
	{ _tag_field_block, 0, "surfaces*", &global_collision_bsp_definition_data.surface_block },
	{ _tag_field_block, 0, "edges*", &global_collision_bsp_definition_data.edge_block },
	{ _tag_field_block, 0, "vertices*", &global_collision_bsp_definition_data.vertex_block },
	{ _tag_field_terminator, 0, NULL, NULL },
};

/* ---------- public code */

/* ---------- private code */
