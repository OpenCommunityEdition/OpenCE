/*
OBJECT_DEATH.C

What a unit of a group does as it dies, from a mutator's [death]
(docs/mutators.md): an effect played at it, and its body no longer drawn, as
if it were taken away (a paintball game's player teleported out). It comes
as a tag edit of its own kind (mutator_tags.c), so that the host's is played
on every machine; each machine kills the units the host says died
(unit_died), so each plays the effect and hides the body itself.

The body is only hidden: it is still there for what follows a death (the
dead player's camera, the weapons dropped by it, the garbage collection that
takes it away later), as the stock game has it.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "cache/cache_files.h"
#include "effects/effects.h"
#include "objects/objects.h"
#include "tag_files/tag_files.h"

#include "object_death.h"

#include <stdio.h>

/* ---------- constants */

enum
{
	/* the unit definitions a death is made for (a map has a few) */
	MAXIMUM_DEATH_DEFINITIONS = 64,
};

/* ---------- structures */

struct object_death_definition
{
	long definition_index;
	long effect_index;
	boolean vanish;
};

/* ---------- globals */

short object_death_count;

static struct object_death_definition object_deaths[MAXIMUM_DEATH_DEFINITIONS];

/* ---------- public code */

void object_death_clear(
	void)
{
	object_death_count = 0;
}

boolean object_death_add(
	long group_tag,
	long effect_index,
	boolean vanish,
	char *reason,
	unsigned long size)
{
	struct tag_iterator iterator;
	long definition_index;
	short found = 0;

	tag_iterator_new(&iterator, group_tag);
	while ((definition_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct object_death_definition *death = NULL;
		short index;

		for (index = 0; index < object_death_count; index++)
		{
			if (object_deaths[index].definition_index == definition_index)
				death = &object_deaths[index];
		}
		if (!death)
		{
			if (object_death_count == MAXIMUM_DEATH_DEFINITIONS)
			{
				snprintf(reason, size, "more than %d kinds of unit die so", MAXIMUM_DEATH_DEFINITIONS);
				return FALSE;
			}
			death = &object_deaths[object_death_count++];
			death->definition_index = definition_index;
		}
		death->effect_index = effect_index;
		death->vanish = vanish;
		found++;
	}
	if (!found)
	{
		snprintf(reason, size, "the map has no object of the group");
		return FALSE;
	}
	return TRUE;
}

void object_death_found(
	long unit_index)
{
	struct object_datum *unit = object_get(unit_index);
	short index;

	for (index = 0; index < object_death_count; index++)
	{
		struct object_death_definition const *death = &object_deaths[index];

		if (death->definition_index != unit->definition_index)
			continue;
		if (death->effect_index != NONE)
			effect_new_from_object(death->effect_index, NONE, unit_index, NONE, 1.f, 1.f, NULL, NULL);
		if (death->vanish)
			object_set_visibility(unit_index, FALSE);
		return;
	}
}
