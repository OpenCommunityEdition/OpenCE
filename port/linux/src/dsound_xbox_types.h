/* Adapted from OpenH1/OpenH1, revision 6750db5a6c9716857bd1e63d9ec188a20c49f1d3.
 * Copyright OpenH1 contributors; AGPL-3.0-or-later.
 * Provenance and hardware measurements: docs/reading/xbox_audio.md. */

#ifndef DSOUND_XBOX_TYPES_H
#define DSOUND_XBOX_TYPES_H

/* the frames a voice's volume, pan, pitch, low passes and way through the
HRTF take to move to new ones: 64 (1.3 ms), however many frames the mixes
have, a ramp going on from one mix to the next (mix_voice). An Xbox's take 1
to 2 ms (recorded: a step of each settles from 10% to 90% within them, a
step of the position through its HRTF too) */
#define PARAMETER_RAMP_FRAMES 64

#define XBOX_ADPCM_BLOCK_BYTES 36
#define XBOX_ADPCM_BLOCK_SAMPLES 64

/* the resampler (resampling) */
#define RESAMPLER_ZERO_CROSSINGS 24
#define RESAMPLER_TABLE_STEPS 256
#define RESAMPLER_CUTOFF 0.96
#define RESAMPLER_KAISER_BETA 6.5
#define RESAMPLER_MAXIMUM_STRETCH 4
#define RESAMPLER_HISTORY 256
/* the Xbox's resampler, which a voice's reverb send goes through
(send_phases): a low pass of SEND_ZERO_CROSSINGS source frames each side, in
SEND_TAPS taps (whole vectors: those past it are 0) */
#define SEND_ZERO_CROSSINGS 3
#define SEND_TAPS 8
/* the Xbox's resampler on a voice's direct path (audio.resampler "xbox":
direct_phases): its low pass as recorded (direct_kernel), DIRECT_ZERO_CROSSINGS
source frames each side, in DIRECT_TAPS taps, a polynomial of DIRECT_KERNEL_ORDER
in each frame */
#define DIRECT_ZERO_CROSSINGS 4
#define DIRECT_KERNEL_ORDER 5
#define DIRECT_TAPS 8

/* the Xbox's quietest levels (voice_level): below VOICE_LEVEL_KNEE (a gain,
-43.84 dB) a level is VOICE_LEVEL_STEP of full scale a decibel above
VOICE_LEVEL_SILENT (-57 dB), where it is silent */
#define VOICE_LEVEL_KNEE 0.0064242f
#define VOICE_LEVEL_STEP (1.0f / 2048.0f)
#define VOICE_LEVEL_SILENT 57.0f
/* a 3D voice's level under voice_level: its volume and distance rolloff at
VOICE_LEVEL_3D times (its direct path's with its direct level), the rest of
its gains after it (voice_gains); its reverb send is VOICE_LEVEL_3D times its
room level and distance rolloff */
#define VOICE_LEVEL_3D 2.0f

/* the Doppler shift (doppler): the speed of sound (metres a second), the
Doppler factor (DirectSound's default, which the game keeps), and the most a
voice is shifted either way (an octave) */
#define DOPPLER_SPEED_OF_SOUND 343.0f
#define DOPPLER_MAXIMUM_RATIO 2.0f

/* the HRTF (hrtf): the frames each ear keeps for its delay, the distance
within which a sound's direction fades out (world units), the bands each
ear's level is set in (HRTF_BANDS, split at hrtf_crossovers), and the
directions the table (hrtf_table) holds: every 15 degrees round from ahead
to behind, at 60, 40, 20 and 0 degrees below and above, with separate pole responses. Measured on an Xbox
(see the HRTF's comment below) */
#define HRTF_FRAMES 64
#define HRTF_NEAR 0.25f
#define HRTF_BANDS 9
#define HRTF_AZIMUTHS 13
#define HRTF_ELEVATIONS 7
/* the crossfade between the pan and the HRTF when the choice changes
(audio.hrtf): 100 ms */
#define HRTF_FADE_FRAMES 4800
#if defined(__GNUC__) || defined(__clang__)
#define RESAMPLER_ALIGNED __attribute__((aligned(16)))
#else
#define RESAMPLER_ALIGNED
#endif


#define VOICE_FREQUENCY_MINIMUM 188.0
#define VOICE_FREQUENCY_MAXIMUM 191983.0
struct hrtf_parameters
{
	/* each ear's delay (frames; left, right), and its weights of the
	bands' low passes and of the sound itself (hrtf_process: each band at
	its level, HRTF_BANDS of them told apart by HRTF_BANDS - 1 low passes) */
	float delay[2];
	float weights[2][HRTF_BANDS];
	/* (each ear's level in each band, dB: for the log, not ramped) */
	float decibels[2][HRTF_BANDS];
	/* each ear's gain: the pan's in the middle (-3 dB), with the voice's
	distance and direct path, the head's filters doing the rest */
	float gain;
	/* the gains of the front pair of speakers and the back pair (on
	stereo 1 and 0; on 5.1 DirectSound's split, surround_split) */
	float pair[2];
};

#endif
