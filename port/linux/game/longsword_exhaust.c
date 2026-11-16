/*
LONGSWORD_EXHAUST.C

The Longswords' engines (vehicles\fighterbomber\fighterbomber) light a lens
flare at their "exhaust flare" marker (vehicles\fighterbomber\fighterbomber
exhaust), a streak twice the size of a world unit. Its tag fades it by no
angle, so it shines toward every side, hidden only where its occlusion test
falls behind the hull: from ahead or abreast, as in a10's opening, the test
point (the marker, moved a tenth of a unit toward the camera) sits just
over the top of the hull, and the flare was drawn across the black ship as
a pale smear. Its marker points to the ship's right, not back along its
path, so the tag's own fade by the light's direction would not do.

As an Xbox map's tags load (scenario_tags_load), the flare is made to fade
by its light's direction to the camera (rasterizer_lens_flare_face_light_direction):
whole within LONGSWORD_EXHAUST_FALLOFF_ANGLE of straight behind the ship,
none past LONGSWORD_EXHAUST_CUTOFF_ANGLE. Each time it is drawn its direction
is the ship's way back (longsword_exhaust_flare_direction), not the marker's:
an engine's glow, seen from behind, as the Longsword leaving the Maw is. A
map whose flare has a fade of its own, or another tag of that name, is left
as it is. Only the drawing changes, nothing played or sent between machines.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "rasterizer/rasterizer.h"
#include "tag_files/tag_groups.h"

#include "longsword_exhaust.h"

/* ---------- constants */

#define LONGSWORD_EXHAUST_FLARE_NAME "vehicles\\fighterbomber\\fighterbomber exhaust"
#define LONGSWORD_EXHAUST_FLARE_TAG 'lens'
/* the angles from straight behind the ship within which the glow is whole,
and past which it is none */
#define LONGSWORD_EXHAUST_FALLOFF_ANGLE (30.0f * _pi / 180.0f)
#define LONGSWORD_EXHAUST_CUTOFF_ANGLE (60.0f * _pi / 180.0f)

/* ---------- globals */

/* the flare changed, or NONE */
static long longsword_exhaust_flare_index = NONE;

/* ---------- public code */

void longsword_exhaust_tags_loaded(
	void)
{
	long flare_index = tag_loaded(LONGSWORD_EXHAUST_FLARE_TAG, LONGSWORD_EXHAUST_FLARE_NAME);

	longsword_exhaust_flare_index = flare_index != NONE &&
		rasterizer_lens_flare_face_light_direction(
			flare_index,
			LONGSWORD_EXHAUST_FALLOFF_ANGLE,
			LONGSWORD_EXHAUST_CUTOFF_ANGLE) ?
		flare_index :
		NONE;

	return;
}

void longsword_exhaust_tags_unloaded(
	void)
{
	longsword_exhaust_flare_index = NONE;

	return;
}

boolean longsword_exhaust_flare_direction(
	long lens_flare_definition_index,
	long object_index,
	real_vector3d *direction)
{
	struct object_datum *object;

	if (longsword_exhaust_flare_index == NONE ||
		lens_flare_definition_index != longsword_exhaust_flare_index ||
		object_index == NONE)
	{
		return FALSE;
	}
	object = object_try_and_get(object_get_ultimate_parent(object_index));
	if (!object)
	{
		return FALSE;
	}
	direction->i = -object->object.forward.i;
	direction->j = -object->object.forward.j;
	direction->k = -object->object.forward.k;

	return TRUE;
}
