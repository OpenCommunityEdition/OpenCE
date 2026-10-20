/* Presentation provenance lives outside the serialized game and tag data. */
#ifndef __PERFORMANCE_SOUND_H
#define __PERFORMANCE_SOUND_H

enum performance_sound_role
{
	_performance_sound_normal = 0,
	_performance_sound_movement = 1,
	_performance_sound_weapon_ready = 2,
};

enum performance_sound_owner
{
	_performance_sound_voice,
	_performance_sound_effect,
	_performance_sound_particle,
};

struct performance_sound_statistics
{
	/* Role indices: normal 0, movement 1, weapon-ready 2. */
	unsigned long voices_by_role[3];
	unsigned long muted_dispatches_by_role[3];
	unsigned long tagged_effects;
	unsigned long tagged_particles;
	long last_voice_index;
	long last_definition_index;
	unsigned last_voice_role;
};

/* These scopes run on the game thread, alongside effect/sound creation.
 * Save and restore the returned value, including when scopes are nested. */
unsigned performance_sound_push(unsigned role);
void performance_sound_pop(unsigned previous);
unsigned performance_sound_current(void);
void performance_sound_reset(unsigned owner);
void performance_sound_capture(unsigned owner, long datum_index);
void performance_sound_capture_voice(long sound_index, long definition_index);
void performance_sound_record(unsigned owner, long datum_index, unsigned role);
void performance_sound_forget(unsigned owner, long datum_index);
unsigned performance_sound_role(unsigned owner, long datum_index);
float performance_sound_gain(long sound_index, unsigned silent_roles);
void performance_sound_get_statistics(struct performance_sound_statistics *statistics);

#endif
