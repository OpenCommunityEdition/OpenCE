/*
ANIMATION_BLEND_SPACE.C

When an object changes animation with a blend (object_start_interpolation:
a unit's scripted custom_animation with its interpolate flag, the return
from it to the unit's own animations, an action), the pose it last had is
kept (original_node_orientations) and the new animation's is blended from
it over a few ticks (object_compute_node_matrices). A pose's root is in the
object's own space, or, for an animation flagged world relative (the
cinematics'), in the world's: blended from one into the other, the root's
translation went from the object's origin to the world's, or the other way,
and the body flew across the scene for the blend's ticks. In c10's
flashback (x50) marine_2 and Bisenti start their world-relative animations
as the flood come (x50_4110mar2, x50_4110bisenti) with the blend, standing
at about (-22, -10): each was drawn coming in from the world's origin, 16
units off, at some 4 units a tick for 6 ticks, and Bisenti flew 19 units in
from the other side as he went back to his own animations. On the Xbox's
4:3 picture the flight was mostly out of the frame; on a wider one it
crossed the picture's edge, a marine shown and gone in a moment.

The space of each pose computed is noted (animation_blend_space_computed),
and a blend takes the space of the pose it keeps as it starts
(animation_blend_space_started); a blend into an animation of the other
space is not drawn (animation_blend_space_crossed): the new animation's own
pose is, at once, as a blend of no ticks would. Blends within one space are
as they were. An entry is an object's only while its index is, and none is
kept across a map's start or a game state's load (animation_blend_space_forget),
after which a blend is drawn as the original drew it. Only the drawing of
the pose changes: nothing the machines send or agree on.
*/

/* ---------- headers */

#include "cseries.h"
#include "memory/data.h"
#include "objects/objects.h"

#include "animation_blend_space.h"

/* ---------- globals */

/* by an object's index's absolute part: the object whose pose was last
computed there (NONE: none) and whether in the world's space, and the
object whose blend started there (NONE: none known) and from which */
static struct
{
	long computed_object_index;
	boolean computed_world_relative;
	long blend_object_index;
	boolean blend_world_relative;
} animation_blend_spaces[MAXIMUM_OBJECTS_PER_MAP];

/* ---------- public code */

void animation_blend_space_forget(
	void)
{
	long index;

	for (index = 0; index < MAXIMUM_OBJECTS_PER_MAP; index++)
	{
		animation_blend_spaces[index].computed_object_index = NONE;
		animation_blend_spaces[index].blend_object_index = NONE;
	}

	return;
}

void animation_blend_space_computed(
	long object_index,
	boolean world_relative)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);

	if (object_index == NONE || absolute_index >= MAXIMUM_OBJECTS_PER_MAP)
	{
		return;
	}
	animation_blend_spaces[absolute_index].computed_object_index = object_index;
	animation_blend_spaces[absolute_index].computed_world_relative = world_relative != FALSE;

	return;
}

void animation_blend_space_started(
	long object_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);

	if (object_index == NONE || absolute_index >= MAXIMUM_OBJECTS_PER_MAP)
	{
		return;
	}
	if (animation_blend_spaces[absolute_index].computed_object_index == object_index)
	{
		animation_blend_spaces[absolute_index].blend_object_index = object_index;
		animation_blend_spaces[absolute_index].blend_world_relative =
			animation_blend_spaces[absolute_index].computed_world_relative;
	}
	else
	{
		animation_blend_spaces[absolute_index].blend_object_index = NONE;
	}

	return;
}

boolean animation_blend_space_crossed(
	long object_index,
	boolean world_relative)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);

	return object_index != NONE &&
		absolute_index < MAXIMUM_OBJECTS_PER_MAP &&
		animation_blend_spaces[absolute_index].blend_object_index == object_index &&
		animation_blend_spaces[absolute_index].blend_world_relative != (world_relative != FALSE);
}
