/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "sound_classes.h"
#include "game_state.h"

/* ---------- constants */

enum
{
	NUMBER_OF_SOUND_CLASSES = 51
};

/* ---------- macros */

#define SOUND_CLASS_DEFINITION(maximum_per_definition, maximum_per_object, preemption_time, speech, priority, cache_miss_mode, wet_gain, minimum_distance, maximum_distance, unknown_gain, unknown_scale, disabled) \
	{ maximum_per_definition, maximum_per_object, preemption_time, speech, 0, priority, cache_miss_mode, 0, wet_gain, 0.f, minimum_distance, maximum_distance, unknown_gain, unknown_scale, disabled, { 0, 0, 0 } }

/* ---------- structures */

struct sound_class_datum
{
	real desired_gain;
	real gain;
	short ticks;
};

typedef char verify_sound_class_definition_size[
	sizeof(struct sound_class_definition) == 0x2C ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

struct sound_class_datum *sound_class_data;
struct sound_class_definition sound_classes[NUMBER_OF_SOUND_CLASSES] =
{
	SOUND_CLASS_DEFINITION(6, 4, 100, 0, 4, 0, 0.5f, 1.4f, 8.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 200, 0, 5, 1, 0.5f, 8.f, 120.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 0, 0, 4, 1, 0.5f, 4.f, 70.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 500, 0, 4, 1, 0.5f, 1.f, 9.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 500, 0, 4, 1, 0.5f, 1.f, 9.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 60, 0, 4, 1, 0.5f, 1.f, 9.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 500, 0, 4, 1, 0.5f, 1.f, 9.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 500, 0, 4, 1, 0.5f, 1.f, 9.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 500, 0, 4, 1, 0.5f, 1.f, 9.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 3, 1, 0.5f, 0.5f, 3.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 3, 0, 0.5f, 0.5f, 3.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 1000, 0, 3, 0, 0.5f, 0.5f, 3.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 200, 0, 3, 0, 0.5f, 0.9f, 10.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 1, 3, 1, 0.8f, 3.f, 20.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(4, 2, 400, 0, 3, 0, 0.5f, 1.4f, 8.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 2, 100, 0, 3, 1, 0.9f, 1.4f, 8.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 2, 1, 0.5f, 0.9f, 5.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 2, 1, 0.5f, 0.9f, 5.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 1, 1, 0.5f, 0.9f, 5.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 1, 1, 0.5f, 0.9f, 5.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 1, 1, 0.5f, 0.5f, 3.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(4, 4, 100, 0, 2, 1, 1.f, 0.9f, 5.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 1, 1, 1.f, 0.9f, 5.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 1, 1, 1.f, 0.9f, 5.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 1, 1, 1.f, 0.5f, 3.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 4, 1, 1.f, 0.5f, 3.f, 1.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(4, 4, 100, 1, 6, 1, 0.8f, 3.f, 20.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 4, 100, 0, 3, 1, 0.8f, 2.f, 5.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 4, 100, 1, 5, 1, 0.8f, 3.f, 20.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(4, 4, 100, 1, 6, 1, 0.8f, 3.f, 20.f, 0.f, 1.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(0, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0),
	SOUND_CLASS_DEFINITION(4, 1, 100, 0, 5, 1, 1.f, 3.f, 20.f, 1.f, 1.f, 0),
};

char const *sound_class_names[NUMBER_OF_SOUND_CLASSES] =
{
	"projectile_impact", "projectile_detonation", "", "",
	"weapon_fire", "weapon_ready", "weapon_reload", "weapon_empty",
	"weapon_charge", "weapon_overheat", "weapon_idle", "", "",
	"object_impacts", "particle_impacts", "slow_particle_impacts", "", "",
	"unit_footsteps", "unit_dialog", "", "", "vehicle_collision",
	"vehicle_engine", "", "", "device_door", "device_force_field",
	"device_machinery", "device_nature", "device_computers", "", "music",
	"ambient_nature", "ambient_machinery", "ambient_computers", "", "", "",
	"first_person_damage", "", "", "", "", "scripted_dialog_player",
	"scripted_effect", "scripted_dialog_other",
	"scripted_dialog_force_unspatialized", "", "", "game_event",
};

/* ---------- private code */

/* port: a class the engine has (a sound's class is the map's value, asked
on every play). One out of range is the nearest class, said once a run. */
static short sound_class_index_valid(
	short class_index)
{
	static boolean complained = FALSE;

	if (!VALID_INDEX(class_index, NUMBER_OF_SOUND_CLASSES))
	{
		if (!complained)
		{
			error(
				_error_silent,
				"sound class %d is not one of the %d sound classes",
				class_index,
				NUMBER_OF_SOUND_CLASSES);
			complained = TRUE;
		}
		class_index = (short)PIN(class_index, 0, NUMBER_OF_SOUND_CLASSES - 1);
	}

	return class_index;
}

/* ---------- public code */

struct sound_class_definition *sound_class_get(
	short class_index)
{
	struct sound_class_definition *definition;

	class_index = sound_class_index_valid(class_index);
	definition = &sound_classes[class_index];
	match_assert(
		"c:\\halo\\source\\sound\\sound_classes.h",
		131,
		class_index>=0 && class_index<NUMBER_OF_SOUND_CLASSES);
	match_assert(
		"c:\\halo\\source\\sound\\sound_classes.h",
		132,
		sound_class_names[class_index][0]);
	match_assert(
		"c:\\halo\\source\\sound\\sound_classes.h",
		133,
		definition->maximum_number_per_definition<=MAXIMUM_SOUND_INSTANCES_PER_DEFINITION);
	match_assert(
		"c:\\halo\\source\\sound\\sound_classes.h",
		134,
		definition->maximum_number_per_object<=MAXIMUM_SOUND_INSTANCES_PER_OBJECT_PER_DEFINITION);

	return &sound_classes[class_index];
}

void sound_classes_initialize(
	void)
{
	sound_class_data = game_state_malloc(
		"sound classes",
		NULL,
		0x264);

	return;
}

void sound_classes_dispose_from_old_map(
	void)
{
	return;
}

void sound_classes_dispose(
	void)
{
	sound_class_data = NULL;

	return;
}

static struct sound_class_datum *sound_class_datum_get(
	short index)
{
	index = sound_class_index_valid(index);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_classes.c",
		288,
		index>=0 && index<NUMBER_OF_SOUND_CLASSES);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_classes.c",
		289,
		sound_class_data);

	return &sound_class_data[index];
}

real sound_class_get_gain(
	short index)
{
	return sound_class_datum_get(index)->gain;
}

void debug_sound_classes_enable(
	char const *name,
	boolean enable)
{
	short class_index;
	char const **sound_class_name;

	class_index = 0;
	sound_class_name = sound_class_names;
	do
	{
		if ((*sound_class_name)[0] && strstr(*sound_class_name, name))
		{
			sound_class_get(class_index)->disabled = !enable;
		}
		class_index++;
		sound_class_name++;
	}
	while (class_index < NUMBER_OF_SOUND_CLASSES);

	return;
}

void debug_sound_classes_set_distances(
	char const *name,
	real minimum_distance,
	real maximum_distance)
{
	short class_index;
	char const **sound_class_name;

	class_index = 0;
	sound_class_name = sound_class_names;
	do
	{
		if ((*sound_class_name)[0] && strstr(*sound_class_name, name))
		{
			sound_class_get(class_index)->minimum_distance = minimum_distance;
			sound_class_get(class_index)->maximum_distance = maximum_distance;
		}
		class_index++;
		sound_class_name++;
	}
	while (class_index < NUMBER_OF_SOUND_CLASSES);

	return;
}

void sound_classes_initialize_for_new_map(
	void)
{
	short class_index;

	for (class_index = 0; class_index < NUMBER_OF_SOUND_CLASSES; class_index++)
	{
		struct sound_class_datum *sound_class = sound_class_datum_get(class_index);

		sound_class->gain = 1.f;
		sound_class->desired_gain = 1.f;
		sound_class->ticks = 0;
	}

	return;
}

void sound_classes_update(
	long ticks)
{
	if (ticks > 0)
	{
		short class_index;

		for (class_index = 0; class_index < NUMBER_OF_SOUND_CLASSES; class_index++)
		{
			struct sound_class_datum *sound_class = sound_class_datum_get(class_index);
			if (sound_class->ticks > ticks)
			{
				sound_class->gain =
					(real)ticks / sound_class->ticks *
					(sound_class->desired_gain - sound_class->gain) +
					sound_class->gain;
				sound_class->ticks -= ticks;
			}
			else
			{
				sound_class->gain = sound_class->desired_gain;
				sound_class->ticks = 0;
			}
		}
	}

	return;
}

void debug_sound_classes_set_wet(
	char const *name,
	real wet)
{
	short class_index;
	char const **sound_class_name;

	class_index = 0;
	sound_class_name = sound_class_names;
	do
	{
		if ((*sound_class_name)[0] && strstr(*sound_class_name, name))
		{
			real wet_gain = PIN(1.f - wet, 0.f, 1.f);

			sound_class_get(class_index)->wet_gain = wet_gain;
		}
		class_index++;
		sound_class_name++;
	}
	while (class_index < NUMBER_OF_SOUND_CLASSES);

	return;
}

void sound_class_set_gain(
	char const *name,
	real gain,
	short interpolation_ticks)
{
	short class_index;
	char const **sound_class_name;

	class_index = 0;
	sound_class_name = sound_class_names;
	do
	{
		if ((*sound_class_name)[0] && strstr(*sound_class_name, name))
		{
			struct sound_class_datum *sound_class = sound_class_datum_get(class_index);

			sound_class->desired_gain = PIN(gain, 0.f, 1.f);
			sound_class->ticks =
				interpolation_ticks < 0 ? 0 : interpolation_ticks;
		}
		class_index++;
		sound_class_name++;
	}
	while (class_index < NUMBER_OF_SOUND_CLASSES);

	return;
}

/* ---------- private code */
