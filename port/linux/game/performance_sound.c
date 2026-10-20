#include "performance_sound.h"
#include "halo_port_capacity.h"
#include <stddef.h>
#include <string.h>

struct performance_sound_provenance
{
	long datum_index;
	unsigned role;
};

/* Sound manager's fixed pool is 0x200. Effects and particles use the same
 * capacities as their native game pools. A full salted handle distinguishes
 * reused slots; every successful allocation writes even the normal role. */
static struct performance_sound_provenance voices[0x200];
static struct performance_sound_provenance effects[HALO_PORT_MAXIMUM_EFFECTS];
static struct performance_sound_provenance particles[HALO_PORT_MAXIMUM_PARTICLES];
static unsigned current_role;
static struct performance_sound_statistics statistics;

static struct performance_sound_provenance *provenance_table(unsigned owner, size_t *count)
{
	switch (owner)
	{
		case _performance_sound_voice: *count = sizeof(voices) / sizeof(*voices); return voices;
		case _performance_sound_effect: *count = sizeof(effects) / sizeof(*effects); return effects;
		case _performance_sound_particle: *count = sizeof(particles) / sizeof(*particles); return particles;
		default: *count = 0; return NULL;
	}
}

static struct performance_sound_provenance *provenance_get(unsigned owner, long datum_index)
{
	size_t count;
	struct performance_sound_provenance *table = provenance_table(owner, &count);
	unsigned slot = (unsigned long)datum_index & 0xffffu;
	return datum_index != -1 && slot < count ? &table[slot] : NULL;
}

unsigned performance_sound_push(unsigned role)
{
	unsigned previous = current_role;
	current_role = role & (_performance_sound_movement | _performance_sound_weapon_ready);
	return previous;
}

void performance_sound_pop(unsigned previous)
{
	current_role = previous & (_performance_sound_movement | _performance_sound_weapon_ready);
}

unsigned performance_sound_current(void)
{
	return current_role;
}

void performance_sound_reset(unsigned owner)
{
	size_t count;
	struct performance_sound_provenance *table = provenance_table(owner, &count);
	if (table)
		memset(table, 0, count * sizeof(*table));
	if (owner == _performance_sound_voice)
	{
		memset(&statistics, 0, sizeof(statistics));
		statistics.last_voice_index = statistics.last_definition_index = -1;
	}
}

void performance_sound_record(unsigned owner, long datum_index, unsigned role)
{
	struct performance_sound_provenance *record = provenance_get(owner, datum_index);
	if (record)
	{
		record->datum_index = datum_index;
		record->role = role & (_performance_sound_movement | _performance_sound_weapon_ready);
		if (owner == _performance_sound_voice)
		{
			if (!record->role) ++statistics.voices_by_role[_performance_sound_normal];
			if (record->role & _performance_sound_movement) ++statistics.voices_by_role[_performance_sound_movement];
			if (record->role & _performance_sound_weapon_ready) ++statistics.voices_by_role[_performance_sound_weapon_ready];
			statistics.last_voice_index = datum_index;
			statistics.last_definition_index = -1;
			statistics.last_voice_role = record->role;
		}
		else if (record->role && owner == _performance_sound_effect) ++statistics.tagged_effects;
		else if (record->role && owner == _performance_sound_particle) ++statistics.tagged_particles;
	}
}

void performance_sound_capture(unsigned owner, long datum_index)
{
	performance_sound_record(owner, datum_index, current_role);
}

void performance_sound_capture_voice(long sound_index, long definition_index)
{
	performance_sound_capture(_performance_sound_voice, sound_index);
	if (provenance_get(_performance_sound_voice, sound_index))
		statistics.last_definition_index = definition_index;
}

void performance_sound_forget(unsigned owner, long datum_index)
{
	struct performance_sound_provenance *record = provenance_get(owner, datum_index);
	if (record && record->datum_index == datum_index)
		record->role = _performance_sound_normal;
}

unsigned performance_sound_role(unsigned owner, long datum_index)
{
	struct performance_sound_provenance *record = provenance_get(owner, datum_index);
	return record && record->datum_index == datum_index ? record->role : _performance_sound_normal;
}

float performance_sound_gain(long sound_index, unsigned silent_roles)
{
	unsigned muted = performance_sound_role(_performance_sound_voice, sound_index) & silent_roles;
	if (muted & _performance_sound_movement) ++statistics.muted_dispatches_by_role[_performance_sound_movement];
	if (muted & _performance_sound_weapon_ready) ++statistics.muted_dispatches_by_role[_performance_sound_weapon_ready];
	return muted ? 0.0f : 1.0f;
}

void performance_sound_get_statistics(struct performance_sound_statistics *result)
{
	if (result) *result = statistics;
}
