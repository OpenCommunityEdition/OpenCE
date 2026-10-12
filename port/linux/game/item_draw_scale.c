/*
ITEM_DRAW_SCALE.C

display.big_multiplayer_guns (port/linux/src/port_config.c): in multiplayer
the game gives weapons and equipment lying about their item's multiplayer
scale (game_engine.c's game_engine_update_weapons), drawn larger than in the
campaign. "classic" draws them so; "enhanced" draws them at their own size.

Only the drawing changes: the item's scale in the game (its pickup reach,
what the host and clients agree on) is the game's. Its node matrices are
drawn shrunk about its root by its scale (render_objects.c), as the first
person legs are drawn with nodes of their own.
*/

#include "cseries.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "objects/object_types.h"
#include "items/items.h"

#include <string.h>

/* port_config.c's */
const char *config_string(const char *name);
unsigned long config_changes(void);

enum
{
	/* an item's nodes, at most, drawn shrunk (an item has a few) */
	ITEM_DRAW_SCALE_MAXIMUM_NODES = 64,
};

static boolean item_draw_scale_enhanced(
	void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean enhanced = FALSE;

	if (changes != config_changes())
	{
		const char *value = config_string("display.big_multiplayer_guns");

		changes = config_changes();
		enhanced = value && !strcmp(value, "enhanced");
	}
	return enhanced;
}

/* the node matrices an object is drawn with: an item lying about at a scale
not its own, with "enhanced", its nodes at their own size; else the nodes */
real_matrix4x3 const *item_draw_scale_node_matrices(
	long object_index,
	real_matrix4x3 const *nodes)
{
	static real_matrix4x3 shrunk[ITEM_DRAW_SCALE_MAXIMUM_NODES];
	struct item_datum *item;
	long count;
	long index;
	real inverse;
	real_point3d root;

	if (!nodes || !item_draw_scale_enhanced())
		return nodes;
	item = (struct item_datum *)object_try_and_get_and_verify_type(object_index, _object_mask_item);
	if (!item || TEST_FLAG(item->item.flags, _item_attached_to_unit_bit) ||
		!(item->object.scale > 0.f) || item->object.scale == 1.f)
	{
		return nodes;
	}
	count = item->object.node_matrices.size / (long)sizeof(real_matrix4x3);
	if (count <= 0 || count > ITEM_DRAW_SCALE_MAXIMUM_NODES)
		return nodes;
	inverse = 1.f / item->object.scale;
	root = nodes[0].position;
	for (index = 0; index < count; index++)
	{
		shrunk[index] = nodes[index];
		shrunk[index].scale *= inverse;
		shrunk[index].position.x = root.x + (nodes[index].position.x - root.x) * inverse;
		shrunk[index].position.y = root.y + (nodes[index].position.y - root.y) * inverse;
		shrunk[index].position.z = root.z + (nodes[index].position.z - root.z) * inverse;
	}
	return shrunk;
}
