/*
COOP_SOUND_SYNC.H

A co-op client that joins during a looping sound that plays once starts it
where the host has reached (coop_sound_sync.c).
*/
#ifndef __COOP_SOUND_SYNC_H
#define __COOP_SOUND_SYNC_H

struct sound_permutation;

/* which part of a looping sound's track (sound_manager.c's sound types) */
enum coop_sound_sync_part
{
	_coop_sound_sync_start_part,
	_coop_sound_sync_loop_part,
	_coop_sound_sync_stop_part,
};

/* what a part about to play does (coop_sound_sync_place) */
enum coop_sound_sync_placement
{
	/* it plays (from where it was placed) */
	_coop_sound_sync_play,
	/* it has played out on the host: it does not play, and the next part does */
	_coop_sound_sync_skip,
	/* the sound has played out on the host: nothing more of it plays */
	_coop_sound_sync_finished,
};

/* the client (network_coop.c): the host's start of a looping sound that plays
once, which it started elapsed_milliseconds before (0 or less: just now) */
void coop_sound_sync_begin(long looping_definition_index, long elapsed_milliseconds);
/* the most a sound plays, in milliseconds (the longest of its pitch ranges'
first permutations, each with the linked ones that follow it, at its slowest
pitch), an hour at most, or 0 for none */
long coop_sound_sync_sound_milliseconds(long sound_index);
/* the client: the host stopped it */
void coop_sound_sync_end(long looping_definition_index);
/* a new game */
void coop_sound_sync_reset(void);

/* sound_manager.c: a part of a looping sound's track about to play, its pitch
range chosen, its first permutation (which this may move along its linked
chain) and the pitch it plays at (the range's playback rate included) */
short coop_sound_sync_place(long looping_definition_index, long loop_identifier, short track_index,
	short part, long definition_index, short pitch_range_index, short *permutation_index, real played_pitch,
	long sound_index);
/* sound_manager.c: a looping sound's refresh is over; started, it has placed
its parts (a refresh can start nothing: no listener hears it yet) */
void coop_sound_sync_refreshed(long looping_definition_index, long loop_identifier, boolean started);
/* sound_manager.c: how far into its samples a sound's first permutation
starts (a block's start; 0: at its beginning) */
long coop_sound_sync_take_offset(long sound_index, struct sound_permutation const *permutation);

#endif
