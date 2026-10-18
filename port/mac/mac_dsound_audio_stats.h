#ifndef HALO_MAC_DSOUND_AUDIO_STATS_H
#define HALO_MAC_DSOUND_AUDIO_STATS_H
#include "mac_audio_output.h"

/* Read-only snapshot of the real DirectSound mixer output. AudioUnit counters
   describe frames submitted by CoreAudio; silent counters describe frames
   mixed by the owned Mach-clock fallback when audio output is unavailable. */
struct mac_dsound_audio_stats {
    struct mac_audio_output_stats output;
    uint64_t silent_callbacks,silent_frames,silent_nonzero_frames;
    int sound_started,output_started,silent_clock_started;
    /* Time spent in the shared mixer on whichever thread called it. */
    uint64_t mix_calls,mix_frames,mix_nanoseconds,mix_max_nanoseconds;
    /* Voice mixes, and those whose every target gain was zero. */
    uint64_t voice_mixes,voice_zero_gain_mixes;
    /* OpenAL voices (HALO_MAC_AUDIO_OPENAL=1): sources live, streams left to
       the software mixer, buffers queued and refused. */
    int openal_active,openal_reverb;
    unsigned long openal_sources,openal_fallbacks,openal_buffers_queued,openal_buffer_failures;
    /* Nonzero reverb sends and obstructions given, and the last room type. */
    unsigned long openal_sends_set,openal_obstructions_set;
    long openal_room_type;
};

/* Implemented by the generated Mac view of the shared DirectSound mixer.
   Safe to call while audio callbacks run. No playback state is changed. */
int mac_dsound_audio_statistics(struct mac_dsound_audio_stats *);
#endif
