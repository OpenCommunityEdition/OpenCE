/*
SOUND_MANAGER.H

header included in hcex build.
*/

#ifndef __SOUND_MANAGER_H
#define __SOUND_MANAGER_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/SOUND_MANAGER.C */

boolean sound_valid_for_channel(
	short compression,
	short encoding,
	short sample_rate,
	short spatialization_mode,
	short channel_type_flags);

void sound_dispose(
	void);

void sound_initialize(
	void);
void sound_initialize_for_new_map(
	void);
void sound_dispose_from_old_map(
	void);
void sound_reconnect_to_structure_bsp(
	void);

long sound_render_time(
	void);

void sound_stop_all(
	void);
void sound_stop_impulse(
	long sound_index);
void sound_stop_impulse_by_source_and_definition(
	long source_identifier,
	long definition_index);
void sound_pause(
	boolean paused);
void sound_idle(
	void);

/* ---------- globals */

extern boolean debug_sound;

/* ---------- public code */

#endif // __SOUND_MANAGER_H
