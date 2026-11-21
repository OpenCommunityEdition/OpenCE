/*
FIRST_PERSON_SHADOW.C

Your own body's shadow in first person (Video Setup > FOV and Viewmodels >
Shadow, display.first_person_shadow): the body the game animates as others
see it, and the weapon in its hands, cast as their shadows are cast. The
stock game draws neither the body nor its shadow for the player whose view
it is. Only the shadows' pass takes the body (render_objects.c); the view
still draws the first-person weapon and arms alone.
*/

#include "cseries.h"
#include "objects/objects.h"
#include "units/units.h"

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
unsigned long config_changes(void);

static boolean first_person_shadow_enabled(
	void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean enabled = FALSE;

	if (changes != config_changes())
	{
		changes = config_changes();
		enabled = config_boolean("display.first_person_shadow") != 0;
	}
	return enabled;
}

/* whether the object a view is seen from in first person casts its shadow:
a player's unit (not a cinematic's camera object) */
boolean first_person_shadow_wanted(
	long object_index)
{
	struct object_datum *object = object_try_and_get(object_index);

	return first_person_shadow_enabled() && object &&
		TEST_FLAG(_object_mask_unit, object->object.type) &&
		((struct unit_datum *)object)->unit.player_index != NONE;
}
