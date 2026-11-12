/* Adapted from OpenH1/OpenH1, revision 6750db5a6c9716857bd1e63d9ec188a20c49f1d3.
 * Copyright OpenH1 contributors; AGPL-3.0-or-later.
 * Provenance and hardware measurements: docs/reading/xbox_audio.md. */

/* ---------- the HRTF

On stereo and surround outputs (audio.hrtf), a mono 3D voice reaches each
ear as it would a head's, from its direction relative to the listener
(sound_manager.c refresh_listener), rather than panned between them: a
sound behind or above is told from one ahead or below. The Xbox's
DirectSound put its 3D voices through its own HRTF (DirectSoundUseFullHRTF)
and left the 2D ones (the menus, the music, the HUD) dry; so here.

Each ear hears the voice at its own level in each of HRTF_BANDS bands (to
500 Hz, 1, 2, 3, 4.5, 6.7, 10 and 14 kHz, and above) and after its own
delay, from a table of the directions an Xbox was recorded from (its final
output, a sweep 2 m away through Halo's own DirectSound calls, every 15
degrees round at 40, 20 and 0 degrees below and above, the sweep through
OpenH1's mixer with no HRTF beside it): for each, each ear's level in each
band (dB, least squares over its third octaves and octaves, the bands'
filters as they are here) and the left ear's delay after the right for a
sound to the right (frames: the Xbox's time between the ears, the lag of
their greatest cross-correlation below 1.5 kHz, less what the bands add to
it). A sound to the left is the mirror of one to the right. Between the
directions recorded, the table is interpolated (the levels in dB), round
and up and down; above and below 60 degrees, towards the poles, each
recorded straight up or down, the ears alike.
What the Xbox's recordings showed, and the model this replaces (Brown and
Duda's) did not:
	- the time between the ears hardly falls with the elevation: to the side
	  0.72 ms level, 0.70 at 20 degrees up or down, 0.61 at 40 (a sphere's
	  falls with its cosine, 0.53 at 40);
	- ahead, the ears' high frequencies a few dB up and down by the band
	  (5 kHz down 3 dB, 6.3 kHz down 5, 12.5 kHz level, 16 kHz down 4);
	- behind, a loss that grows with the frequency, 2 dB at 2.5 kHz, 11 at
	  8 kHz, 30 at 16 kHz, not a low pass's from 2 kHz;
	- to the side, the near ear 4 to 5 dB up to 1.5 kHz, level about 3 kHz
	  and 3 to 5 dB up again from 5 to 12 kHz; the far ear 9 to 23 dB down
	  above 3 kHz;
	- below and above, each its own peaks and dips (40 degrees below, 3 dB up
	  at 1.2 to 1.6 kHz; above, 3 to 4 dB down from 5 kHz and 8 dB at 16
	  kHz).
The bands: HRTF_BANDS - 1 low passes (2nd order, Butterworth) of the
voice, each band one's output less the one below's, the top band the voice
less the highest low pass's; so the bands add up to the voice itself, and a
direction whose bands are all at one level is that level and nothing else.
Each ear sums the low passes' outputs and the voice, weighted (the band's
gain less the next's), into its own line, and reads it its delay later
(Catmull-Rom), whole frames for a still sound, as the Xbox's.

A sound close to the head (within HRTF_NEAR) fades to the middle, the ears
alike and at once, as the pan centres one within its minimum distance. The
parameters ramp over PARAMETER_RAMP_FRAMES, as the gains do: a turn of the
head moves the sound smoothly from ear to ear, with no step. Recorded on an
Xbox, a step of the position went from 10% to 90% within 1 ms; ramped over
the whole mix, as they were, 8 ms at 480 frames a mix. Distance, the direct
path's obstruction and occlusion (its low pass), the room send and the
Doppler shift are the pan's, before the head's. When the choice changes
(audio.hrtf set mid-game), each voice crossfades between the pan and the
HRTF over HRTF_FADE_FRAMES, both running meanwhile: no click. "speakers"
keeps the pan. On 5.1 the same, on the front and back pairs, a sound behind
heard as the direction mirrored ahead (surround_split), above and below a
little brighter on the far ear, as the Xbox's back pair (hrtf_behind). */

/* a direction of the table: each ear's level in each band (dB) and the left
ear's delay (frames) after the right, for a sound to the right */
struct hrtf_direction
{
	float left[HRTF_BANDS], right[HRTF_BANDS];
	float delay;
};

static const float hrtf_crossovers[HRTF_BANDS - 1] = { 500, 1000, 2000, 3000, 4500, 6700, 10000, 14000 };

/* round from ahead (0 degrees) to behind (180) to the right, every 15
degrees; 60, 40, 20 and 0 degrees below, and 20, 40 and 60 above. The 60
degree rows and the poles were recorded on an Xbox in the test bed's tenth
run: before, the table stopped at 40 and the poles were the mean of the 40
degree rows, which left a sound straight below (a Warthog's engine and
wheels, 70 to 80 degrees under its driver) 20 to 30 dB too bright above
4.5 kHz, and one 60 below up to 14 dB in a band */
static const struct hrtf_direction hrtf_table[HRTF_ELEVATIONS][HRTF_AZIMUTHS] =
{
	/* 60 degrees below (the test bed's tenth run) */
	{
		{ { -0.8f, -2.1f, -3.9f, 4.8f, -9.9f, -23.3f, -0.4f, -11.1f, -12.5f },
		  { -0.8f, -2.1f, -3.9f, 4.8f, -9.9f, -23.3f, -0.4f, -11.1f, -12.5f }, 0.00f }, /* 0 */
		{ { -1.3f, -3.1f, -5.5f, 5.1f, -15.8f, -25.9f, -0.2f, -22.8f, -14.6f },
		  { -0.1f, -1.3f, -2.5f, 3.7f, -5.4f, -20.0f, -1.2f, -7.4f, -11.5f }, 2.06f }, /* 15 */
		{ { -1.1f, -3.8f, -8.2f, 4.6f, -18.5f, -25.1f, -2.3f, -24.6f, -17.2f },
		  { 0.0f, -0.5f, -0.8f, 2.1f, -2.6f, -21.1f, -1.1f, -7.3f, -12.4f }, 3.62f }, /* 30 */
		{ { -0.9f, -3.6f, -19.3f, 0.0f, -26.6f, -10.9f, -11.3f, -38.1f, -24.0f },
		  { 0.3f, 0.7f, 2.5f, -1.8f, -0.7f, -21.8f, -0.3f, -11.3f, -17.0f }, 7.56f }, /* 45 */
		{ { -1.1f, -2.4f, -21.5f, -8.5f, -12.7f, -19.1f, -18.0f, -39.1f, -55.5f },
		  { 0.3f, 0.8f, 3.5f, -1.5f, -2.4f, -21.5f, -0.3f, -12.3f, -18.0f }, 8.88f }, /* 60 */
		{ { -1.4f, -1.4f, -13.1f, -21.4f, -19.6f, -16.5f, -32.0f, -46.3f, -66.7f },
		  { 0.4f, 0.9f, 4.0f, 0.6f, -5.2f, -22.4f, 0.0f, -14.4f, -17.4f }, 10.63f }, /* 75 */
		{ { -1.5f, -1.2f, -6.8f, -30.9f, -19.4f, -38.2f, -15.5f, -21.8f, -31.3f },
		  { 0.7f, 0.8f, 4.1f, 1.7f, -4.8f, -25.5f, -2.8f, -5.2f, -13.1f }, 12.81f }, /* 90 */
		{ { -1.5f, -1.3f, -13.4f, -35.2f, -15.7f, -18.2f, -23.4f, -24.2f, -30.9f },
		  { 0.7f, 0.8f, 3.8f, 0.7f, -6.7f, -28.2f, -4.4f, -6.4f, -14.1f }, 10.31f }, /* 105 */
		{ { -1.3f, -2.4f, -19.8f, -7.7f, -18.0f, -32.6f, -15.3f, -22.8f, -39.8f },
		  { 0.5f, 0.6f, 3.6f, -2.0f, -5.3f, -28.6f, -7.3f, -5.4f, -13.3f }, 8.81f }, /* 120 */
		{ { -1.3f, -4.7f, -8.0f, -5.1f, -28.0f, -27.7f, -10.7f, -22.5f, -23.7f },
		  { 0.4f, 0.1f, 3.3f, -5.0f, -3.8f, -26.5f, -7.0f, -5.8f, -12.2f }, 8.12f }, /* 135 */
		{ { -1.9f, -3.8f, -1.9f, -3.7f, -10.6f, -24.6f, -8.9f, -13.0f, -21.5f },
		  { -0.3f, -1.0f, 2.1f, -8.2f, -2.1f, -22.2f, -4.1f, -10.8f, -11.0f }, 4.00f }, /* 150 */
		{ { -1.9f, -2.6f, -0.8f, -3.9f, -7.8f, -25.7f, -5.8f, -14.4f, -21.2f },
		  { -0.7f, -1.4f, 1.4f, -6.9f, -2.8f, -22.9f, -4.2f, -9.5f, -13.0f }, 2.13f }, /* 165 */
		{ { -1.4f, -1.9f, 0.4f, -5.3f, -5.2f, -24.1f, -4.6f, -11.5f, -17.3f },
		  { -1.4f, -1.9f, 0.4f, -5.3f, -5.2f, -24.1f, -4.6f, -11.5f, -17.3f }, 0.00f }, /* 180 */
	},
	/* -40 degrees */
	{
		{ { -0.3f, -1.2f, 4.2f, 0.1f, 0.8f, -12.2f, -4.4f, 3.6f, -2.8f },
		  { -0.3f, -1.2f, 4.2f, 0.1f, 0.8f, -12.2f, -4.4f, 3.6f, -2.8f }, 0.00f }, /* 0 */
		{ { -0.8f, -1.2f, 2.6f, -2.0f, 0.4f, -18.3f, -3.3f, 0.7f, -7.7f },
		  { 0.8f, -0.6f, 5.0f, 1.5f, 1.6f, -7.6f, -3.9f, 4.9f, -0.5f }, 4.00f }, /* 15 */
		{ { -1.1f, -1.9f, 0.6f, -2.2f, -5.1f, -22.9f, -0.8f, -5.6f, -11.2f },
		  { 1.3f, 0.6f, 5.2f, 2.3f, 3.2f, -11.4f, -0.3f, 5.6f, -2.0f }, 7.50f }, /* 30 */
		{ { -1.6f, -2.3f, -5.7f, -1.5f, -9.6f, -26.9f, -7.0f, -12.3f, -16.9f },
		  { 2.0f, 2.2f, 4.9f, 3.9f, 2.7f, -16.1f, 7.5f, -13.1f, -1.8f }, 16.50f }, /* 45 */
		{ { -1.3f, -1.7f, -5.9f, -7.0f, -40.8f, -45.5f, -9.4f, -26.9f, -23.6f },
		  { 2.6f, 2.8f, 5.8f, 1.5f, 3.5f, -14.4f, 7.0f, -5.0f, -2.0f }, 21.50f }, /* 60 */
		{ { -1.2f, -0.4f, -6.4f, -40.6f, -33.9f, -14.1f, -30.1f, -17.1f, -33.5f },
		  { 2.7f, 2.9f, 6.4f, -1.9f, 3.9f, -6.3f, 0.2f, 6.2f, -14.0f }, 22.56f }, /* 75 */
		{ { -1.2f, -1.1f, -2.9f, -29.0f, -14.7f, -16.7f, -16.5f, -18.1f, -22.7f },
		  { 2.9f, 2.9f, 6.7f, -1.0f, 0.8f, 0.5f, -11.9f, 7.1f, -0.2f }, 25.00f }, /* 90 */
		{ { -1.4f, -1.0f, -5.5f, -36.2f, -19.6f, -13.6f, -28.4f, -16.1f, -23.1f },
		  { 2.6f, 2.3f, 6.3f, 0.5f, -3.3f, 0.1f, -12.5f, 6.5f, 0.0f }, 23.00f }, /* 105 */
		{ { -1.4f, -1.7f, -9.1f, -5.6f, -34.7f, -39.8f, -11.9f, -20.8f, -38.9f },
		  { 2.5f, 2.0f, 6.0f, 1.4f, -5.7f, -2.6f, -8.9f, 6.3f, -2.2f }, 21.19f }, /* 120 */
		{ { -1.6f, -3.4f, -4.9f, -3.0f, -12.7f, -20.7f, -6.8f, -19.2f, -22.6f },
		  { 1.3f, 1.1f, 5.7f, 0.5f, -6.7f, -8.3f, -6.7f, 2.1f, -10.6f }, 14.06f }, /* 135 */
		{ { -1.7f, -2.5f, -1.0f, -3.0f, -6.2f, -15.9f, -6.3f, -16.7f, -22.0f },
		  { 0.7f, -0.2f, 5.0f, 0.6f, -6.8f, -15.3f, -4.2f, -4.3f, -16.2f }, 8.12f }, /* 150 */
		{ { -1.0f, -2.0f, 0.8f, -0.1f, -5.4f, -17.6f, -6.1f, -14.5f, -23.6f },
		  { -0.1f, -1.0f, 4.5f, -0.9f, -5.9f, -16.7f, -4.4f, -9.8f, -22.1f }, 4.12f }, /* 165 */
		{ { -0.7f, -1.5f, 2.9f, 0.1f, -6.8f, -16.9f, -5.1f, -13.3f, -23.6f },
		  { -0.7f, -1.5f, 2.9f, 0.1f, -6.8f, -16.9f, -5.1f, -13.3f, -23.6f }, 0.00f }, /* 180 */
	},
	/* -20 degrees */
	{
		{ { 0.1f, 0.3f, 3.3f, -13.8f, 5.8f, -9.8f, -14.3f, 4.8f, -3.8f },
		  { 0.1f, 0.3f, 3.3f, -13.8f, 5.8f, -9.8f, -14.3f, 4.8f, -3.8f }, 0.00f }, /* 0 */
		{ { -0.4f, -0.9f, 1.2f, -14.8f, 4.5f, -15.7f, -17.4f, 2.2f, -8.4f },
		  { 0.4f, 1.0f, 4.7f, -16.6f, 6.5f, -3.5f, -13.5f, 5.8f, -0.5f }, 4.19f }, /* 15 */
		{ { -0.7f, -1.6f, -1.3f, -10.5f, -1.6f, -18.0f, -14.7f, -3.9f, -13.1f },
		  { 1.4f, 1.9f, 6.0f, -14.9f, 7.4f, -1.2f, -4.7f, 5.9f, -1.9f }, 9.94f }, /* 30 */
		{ { -0.8f, -2.0f, -4.7f, -4.4f, -6.3f, -12.3f, -16.5f, -9.1f, -18.5f },
		  { 2.0f, 2.6f, 6.2f, -11.8f, 7.6f, -3.9f, 4.3f, -1.6f, -1.1f }, 18.31f }, /* 45 */
		{ { -0.9f, -1.1f, -4.2f, -13.6f, -14.3f, -12.5f, -15.9f, -11.2f, -21.2f },
		  { 3.1f, 3.1f, 6.6f, -4.9f, 4.4f, 2.1f, 2.3f, 2.5f, -0.4f }, 24.38f }, /* 60 */
		{ { -1.1f, 1.0f, -6.5f, -8.1f, -13.6f, -36.4f, -13.9f, -18.4f, -21.0f },
		  { 3.2f, 3.3f, 6.6f, -0.2f, -3.7f, 5.8f, -3.4f, 5.0f, 1.9f }, 27.19f }, /* 75 */
		{ { -1.2f, 0.6f, -3.6f, -23.7f, -6.0f, -25.1f, -15.5f, -16.2f, -19.9f },
		  { 3.2f, 3.1f, 6.7f, 0.3f, -10.1f, 4.9f, -2.8f, 4.6f, 2.7f }, 28.81f }, /* 90 */
		{ { -1.3f, 1.2f, -8.3f, -31.9f, -9.2f, -37.9f, -17.7f, -15.5f, -21.4f },
		  { 2.8f, 2.7f, 6.7f, 0.1f, -13.2f, 1.8f, 0.1f, 3.5f, 2.5f }, 26.13f }, /* 105 */
		{ { -1.1f, -0.7f, -7.7f, -10.1f, -8.3f, -26.6f, -10.9f, -22.0f, -21.4f },
		  { 2.5f, 2.6f, 6.6f, -0.1f, -12.5f, -1.6f, 0.6f, 2.9f, 0.0f }, 24.56f }, /* 120 */
		{ { -1.2f, -2.3f, -8.2f, -1.7f, -10.8f, -33.4f, -14.4f, -13.5f, -25.0f },
		  { 1.6f, 1.7f, 6.7f, -8.5f, -3.1f, -9.5f, -1.0f, 0.6f, -8.9f }, 18.56f }, /* 135 */
		{ { -1.4f, -2.2f, -2.6f, -4.9f, -5.6f, -26.2f, -12.9f, -18.7f, -27.5f },
		  { 0.6f, 1.0f, 5.6f, -8.4f, -0.5f, -18.5f, -1.8f, -7.1f, -15.1f }, 10.50f }, /* 150 */
		{ { -1.2f, -1.3f, 0.4f, -3.1f, -2.5f, -25.8f, -9.6f, -15.5f, -24.0f },
		  { -0.1f, -0.2f, 4.4f, -9.0f, -1.0f, -16.6f, -6.3f, -17.3f, -20.4f }, 4.81f }, /* 165 */
		{ { -0.7f, -0.8f, 2.5f, -3.4f, -3.4f, -24.6f, -6.6f, -23.7f, -26.0f },
		  { -0.7f, -0.8f, 2.5f, -3.4f, -3.4f, -24.6f, -6.6f, -23.7f, -26.0f }, 0.00f }, /* 180 */
	},
	/* level */
	{
		{ { 1.1f, 0.6f, 0.4f, 0.6f, 2.1f, -1.4f, -13.0f, 2.2f, -4.4f },
		  { 1.1f, 0.6f, 0.4f, 0.6f, 2.1f, -1.4f, -13.0f, 2.2f, -4.4f }, 0.00f }, /* 0 */
		{ { 0.6f, -0.3f, -1.1f, -2.8f, 1.7f, -5.4f, -16.4f, 0.7f, -13.2f },
		  { 1.8f, 1.5f, 2.3f, -1.1f, 5.1f, -0.1f, -7.5f, 2.2f, 0.0f }, 4.69f }, /* 15 */
		{ { 0.4f, -1.7f, -4.4f, -1.8f, -7.8f, -10.9f, -14.3f, -5.8f, -18.1f },
		  { 2.1f, 2.7f, 3.6f, -1.2f, 6.7f, -0.2f, 0.1f, 1.3f, -0.7f }, 10.19f }, /* 30 */
		{ { 0.2f, -0.6f, -10.6f, 0.3f, -10.2f, -24.0f, -14.0f, -10.7f, -20.1f },
		  { 3.0f, 3.6f, 4.7f, -1.3f, 7.8f, -1.6f, 5.0f, -2.9f, -1.2f }, 19.56f }, /* 45 */
		{ { -0.5f, 1.4f, -7.8f, -24.8f, -3.8f, -29.2f, -18.5f, -14.4f, -27.6f },
		  { 3.4f, 4.3f, 5.4f, 0.5f, 5.2f, 1.1f, 5.6f, -0.6f, -0.4f }, 24.81f }, /* 60 */
		{ { -0.9f, 1.4f, -2.6f, -18.8f, -16.7f, -24.6f, -12.0f, -18.3f, -20.9f },
		  { 3.4f, 4.4f, 5.3f, 2.7f, -0.1f, 1.4f, 5.2f, 3.4f, 1.3f }, 29.06f }, /* 75 */
		{ { -0.9f, 0.6f, 0.0f, -11.1f, -9.1f, -23.4f, -11.4f, -15.0f, -20.9f },
		  { 3.6f, 4.5f, 5.5f, 3.6f, -3.7f, -0.3f, 5.0f, 5.3f, 1.3f }, 31.50f }, /* 90 */
		{ { -1.0f, 1.3f, -2.6f, -28.9f, -16.7f, -11.6f, -19.4f, -14.2f, -22.4f },
		  { 3.2f, 4.3f, 5.2f, 3.5f, -5.7f, -5.4f, 5.0f, 5.5f, 0.1f }, 28.44f }, /* 105 */
		{ { -0.7f, 1.3f, -10.4f, -12.8f, -6.2f, -37.7f, -16.6f, -15.7f, -28.3f },
		  { 2.9f, 4.3f, 4.3f, 5.0f, -12.5f, -17.6f, 6.5f, 2.3f, -3.9f }, 25.12f }, /* 120 */
		{ { -0.3f, -1.7f, -8.7f, -1.4f, -12.3f, -25.1f, -11.9f, -21.4f, -25.7f },
		  { 1.9f, 3.7f, 3.6f, 4.2f, -16.1f, -21.8f, 5.6f, -8.7f, -10.6f }, 19.94f }, /* 135 */
		{ { -0.6f, -2.1f, -3.3f, -4.8f, -5.7f, -30.2f, -10.2f, -31.3f, -28.3f },
		  { 1.1f, 2.6f, 2.7f, 1.9f, -6.3f, -20.6f, 0.1f, -20.9f, -18.2f }, 11.00f }, /* 150 */
		{ { -0.5f, -0.9f, -0.6f, -3.4f, -1.8f, -24.1f, -8.4f, -39.3f, -26.6f },
		  { 0.8f, 1.1f, 2.6f, -1.8f, -6.0f, -9.5f, -19.0f, -24.8f, -54.2f }, 5.44f }, /* 165 */
		{ { 0.2f, -0.1f, 1.5f, -2.2f, -3.1f, -13.3f, -9.8f, -31.9f, -26.2f },
		  { 0.2f, -0.1f, 1.5f, -2.2f, -3.1f, -13.3f, -9.8f, -31.9f, -26.2f }, 0.00f }, /* 180 */
	},
	/* +20 degrees */
	{
		{ { 1.5f, 0.2f, -0.6f, 2.8f, 0.3f, -0.4f, -6.3f, -1.3f, -6.8f },
		  { 1.5f, 0.2f, -0.6f, 2.8f, 0.3f, -0.4f, -6.3f, -1.3f, -6.8f }, 0.00f }, /* 0 */
		{ { 1.0f, -0.9f, -1.8f, -0.9f, 1.2f, -3.9f, -9.7f, -5.1f, -11.9f },
		  { 2.2f, 0.9f, 0.7f, 3.7f, 1.4f, 2.3f, -4.7f, 1.1f, -2.6f }, 4.25f }, /* 15 */
		{ { 0.2f, -2.0f, -4.2f, -2.2f, -8.6f, -6.2f, -18.0f, -9.2f, -16.4f },
		  { 3.1f, 1.9f, 1.6f, 4.8f, 1.6f, 4.2f, -0.1f, 1.5f, -2.6f }, 9.94f }, /* 30 */
		{ { 0.6f, -1.5f, -9.4f, 0.0f, -10.5f, -17.9f, -19.5f, -15.1f, -24.7f },
		  { 3.7f, 3.2f, 2.5f, 5.3f, 2.6f, 2.5f, 6.3f, -6.8f, -1.1f }, 18.38f }, /* 45 */
		{ { 0.2f, 0.4f, -5.5f, -17.3f, -6.8f, -18.8f, -19.8f, -16.7f, -26.2f },
		  { 4.1f, 3.9f, 2.9f, 6.3f, -0.3f, 3.4f, 6.2f, -0.4f, -2.3f }, 24.31f }, /* 60 */
		{ { -0.2f, 1.6f, -4.7f, -6.0f, -27.0f, -30.3f, -10.4f, -30.8f, -25.6f },
		  { 4.1f, 3.7f, 2.8f, 7.1f, -6.4f, 2.4f, 5.3f, 4.2f, -10.0f }, 27.94f }, /* 75 */
		{ { -0.2f, 0.4f, -0.2f, -15.0f, -3.0f, -12.7f, -8.3f, -22.4f, -24.8f },
		  { 4.3f, 3.6f, 3.2f, 7.3f, -9.9f, -0.7f, 5.5f, 5.5f, -8.0f }, 30.56f }, /* 90 */
		{ { -0.5f, 1.0f, -2.5f, -15.6f, -10.3f, -17.2f, -15.8f, -16.4f, -25.5f },
		  { 4.4f, 3.4f, 3.7f, 7.4f, -11.8f, -4.9f, 5.6f, 5.6f, -7.2f }, 28.50f }, /* 105 */
		{ { -0.2f, 0.2f, -5.6f, -13.0f, -8.2f, -32.9f, -13.6f, -21.8f, -27.9f },
		  { 4.0f, 2.9f, 3.8f, 6.7f, -14.6f, -15.4f, 7.0f, 1.3f, -10.2f }, 24.88f }, /* 120 */
		{ { 0.0f, -1.7f, -7.2f, -2.2f, -12.0f, -28.0f, -11.3f, -21.0f, -30.0f },
		  { 3.2f, 2.3f, 3.0f, 5.4f, -8.9f, -18.5f, 5.0f, -9.3f, -17.3f }, 19.06f }, /* 135 */
		{ { -0.4f, -2.1f, -3.6f, -4.5f, -10.4f, -8.7f, -16.6f, -22.8f, -29.5f },
		  { 2.6f, 1.2f, 2.3f, 3.7f, -8.3f, -8.3f, -1.8f, -21.7f, -19.2f }, 11.00f }, /* 150 */
		{ { 0.2f, -1.0f, -1.8f, -0.7f, -5.7f, -7.7f, -12.3f, -27.3f, -27.3f },
		  { 1.7f, -0.1f, 1.7f, 1.6f, -12.5f, -1.8f, -19.1f, -25.4f, -25.7f }, 5.19f }, /* 165 */
		{ { 1.0f, -0.6f, 0.2f, 1.5f, -14.0f, -2.2f, -16.3f, -24.9f, -27.5f },
		  { 1.0f, -0.6f, 0.2f, 1.5f, -14.0f, -2.2f, -16.3f, -24.9f, -27.5f }, 0.00f }, /* 180 */
	},
	/* +40 degrees */
	{
		{ { 1.1f, -0.2f, -0.1f, 2.3f, -1.5f, -0.9f, -7.0f, -0.1f, -17.9f },
		  { 1.1f, -0.2f, -0.1f, 2.3f, -1.5f, -0.9f, -7.0f, -0.1f, -17.9f }, 0.00f }, /* 0 */
		{ { 0.6f, -0.8f, -1.2f, -2.2f, 2.5f, -10.9f, -5.4f, -5.6f, -21.5f },
		  { 2.0f, 0.1f, 1.4f, 3.3f, -1.8f, 3.4f, -13.2f, 4.4f, -15.2f }, 3.88f }, /* 15 */
		{ { 0.4f, -1.6f, -2.9f, -4.0f, -0.7f, -12.4f, -4.2f, -12.6f, -20.9f },
		  { 2.4f, 0.7f, 2.2f, 3.6f, -1.8f, 5.6f, -11.7f, 5.4f, -2.3f }, 7.62f }, /* 30 */
		{ { 0.6f, -1.5f, -6.8f, -2.8f, -4.8f, -6.6f, -29.7f, -15.2f, -17.5f },
		  { 2.7f, 2.3f, 3.0f, 2.3f, 2.4f, 2.3f, 4.8f, 0.5f, 0.0f }, 16.81f }, /* 45 */
		{ { 0.4f, -0.7f, -4.4f, -4.7f, -14.4f, -5.3f, -18.9f, -12.0f, -25.6f },
		  { 2.9f, 3.0f, 3.2f, 3.2f, 2.4f, 0.0f, 6.3f, 0.4f, -0.4f }, 22.31f }, /* 60 */
		{ { 0.3f, -0.8f, -1.5f, -15.2f, -8.7f, -9.7f, -9.6f, -17.9f, -21.9f },
		  { 3.5f, 2.9f, 3.1f, 4.4f, 0.4f, 0.4f, 4.2f, 4.3f, -1.9f }, 24.44f }, /* 75 */
		{ { 0.2f, -0.7f, -1.1f, -17.8f, -5.2f, -20.1f, -10.4f, -16.5f, -28.5f },
		  { 3.6f, 2.9f, 3.2f, 4.6f, -0.2f, -1.4f, 3.3f, 5.9f, -5.1f }, 25.31f }, /* 90 */
		{ { 0.2f, -1.1f, -1.3f, -14.6f, -8.5f, -20.2f, -6.5f, -22.8f, -26.0f },
		  { 3.1f, 3.0f, 3.5f, 4.2f, 0.0f, -8.0f, 4.9f, 4.4f, -10.1f }, 24.69f }, /* 105 */
		{ { 0.1f, -1.2f, -2.9f, -8.5f, -12.1f, -14.5f, -6.4f, -36.7f, -26.2f },
		  { 3.0f, 2.4f, 3.2f, 4.1f, -1.5f, -11.6f, 5.0f, 1.7f, -12.3f }, 23.00f }, /* 120 */
		{ { 0.2f, -2.0f, -5.5f, -2.8f, -20.5f, -3.9f, -25.1f, -28.1f, -28.3f },
		  { 2.1f, 1.6f, 3.4f, 2.3f, -3.5f, -7.9f, 2.5f, -16.1f, -21.5f }, 15.44f }, /* 135 */
		{ { 0.1f, -2.0f, -3.5f, -1.7f, -17.3f, -3.5f, -12.8f, -35.4f, -27.1f },
		  { 1.7f, 0.3f, 2.9f, 1.3f, -15.4f, 2.1f, -9.8f, -17.9f, -24.1f }, 8.44f }, /* 150 */
		{ { 0.5f, -1.3f, -2.0f, 0.5f, -11.5f, -3.8f, -10.3f, -31.6f, -26.4f },
		  { 1.4f, -0.6f, 2.0f, 0.7f, -18.4f, 1.6f, -11.5f, -19.2f, -24.0f }, 4.25f }, /* 165 */
		{ { 1.0f, -1.0f, 0.4f, 1.2f, -19.2f, -0.1f, -9.6f, -25.8f, -24.9f },
		  { 1.0f, -1.0f, 0.4f, 1.2f, -19.2f, -0.1f, -9.6f, -25.8f, -24.9f }, 0.00f }, /* 180 */
	},
	/* 60 degrees above (the test bed's tenth run) */
	{
		{ { 0.0f, -1.1f, -1.1f, 1.5f, -6.9f, -1.8f, -5.1f, -0.7f, -9.8f },
		  { 0.0f, -1.1f, -1.1f, 1.5f, -6.9f, -1.8f, -5.1f, -0.7f, -9.8f }, 0.00f }, /* 0 */
		{ { 0.0f, -1.3f, -1.9f, -0.1f, -3.2f, -6.8f, -2.4f, -4.6f, -13.5f },
		  { 0.4f, -0.7f, -0.1f, 1.9f, -7.0f, 0.4f, -6.3f, 1.0f, -5.4f }, 2.12f }, /* 15 */
		{ { -0.1f, -1.5f, -2.5f, -1.7f, -3.1f, -6.2f, -5.4f, -4.8f, -15.4f },
		  { 0.5f, -0.6f, 0.7f, 1.5f, -5.0f, 0.6f, -5.1f, 2.5f, -3.9f }, 4.19f }, /* 30 */
		{ { -0.3f, -1.7f, -3.7f, -4.1f, -6.0f, -14.2f, -2.2f, -12.8f, -21.8f },
		  { 0.8f, -0.3f, 1.6f, 1.0f, -2.5f, 0.2f, -3.6f, 4.5f, -2.7f }, 7.94f }, /* 45 */
		{ { -0.3f, -2.0f, -4.0f, -5.2f, -8.0f, -11.9f, -7.3f, -7.8f, -20.4f },
		  { 0.7f, 0.3f, 1.9f, 0.8f, -1.8f, 0.0f, -2.8f, 5.2f, -3.0f }, 9.31f }, /* 60 */
		{ { -0.3f, -2.1f, -3.9f, -6.6f, -8.7f, -14.3f, -7.6f, -13.4f, -21.8f },
		  { 0.8f, 0.5f, 2.0f, 1.1f, -1.7f, -0.8f, -1.3f, 5.3f, -3.6f }, 10.19f }, /* 75 */
		{ { -0.6f, -2.2f, -3.8f, -8.9f, -9.0f, -20.5f, -6.1f, -20.6f, -28.9f },
		  { 1.0f, 0.5f, 2.1f, 1.1f, -1.4f, -3.8f, 1.5f, 4.0f, -6.1f }, 10.75f }, /* 90 */
		{ { -0.3f, -2.1f, -3.4f, -7.8f, -8.2f, -16.9f, -6.2f, -10.0f, -20.7f },
		  { 0.8f, 0.6f, 2.1f, 0.6f, -1.2f, -6.5f, 2.7f, 2.5f, -7.8f }, 10.12f }, /* 105 */
		{ { -0.3f, -1.9f, -3.3f, -6.6f, -7.7f, -18.6f, -3.7f, -8.1f, -21.4f },
		  { 0.7f, 0.4f, 2.0f, 0.3f, -1.4f, -8.5f, 3.2f, 1.1f, -9.2f }, 9.13f }, /* 120 */
		{ { -0.5f, -1.7f, -3.2f, -5.2f, -7.7f, -17.0f, -0.9f, -13.7f, -20.2f },
		  { 0.6f, -0.1f, 1.6f, 0.0f, -2.2f, -9.0f, 2.7f, 0.2f, -10.6f }, 7.75f }, /* 135 */
		{ { -0.2f, -1.3f, -2.0f, -2.7f, -4.2f, -16.1f, -0.3f, -9.1f, -18.9f },
		  { 0.2f, -0.6f, 0.9f, -0.2f, -4.7f, -9.3f, 1.8f, -1.8f, -12.2f }, 3.94f }, /* 150 */
		{ { -0.2f, -1.2f, -1.6f, -1.1f, -5.2f, -16.2f, 1.0f, -8.7f, -17.4f },
		  { -0.1f, -0.9f, 0.1f, -0.1f, -6.5f, -9.7f, 0.8f, -3.1f, -13.9f }, 2.00f }, /* 165 */
		{ { -0.1f, -0.9f, -0.6f, -0.2f, -6.0f, -13.4f, 1.4f, -5.5f, -15.8f },
		  { -0.1f, -0.9f, -0.6f, -0.2f, -6.0f, -13.4f, 1.4f, -5.5f, -15.8f }, 0.00f }, /* 180 */
	},
};
static const struct hrtf_direction hrtf_poles[2] =
{
	/* straight below and straight above (recorded, the test bed's tenth run) */
	{ { -0.3f, -0.3f, 2.0f, -32.3f, -40.1f, -22.4f, -19.1f, -43.1f, -31.9f },
	  { -0.3f, -0.3f, 2.0f, -32.3f, -40.1f, -22.4f, -19.1f, -43.1f, -31.9f }, 0.0f },
	{ { 0.1f, -0.9f, -0.5f, -0.9f, -4.4f, -8.4f, -1.4f, -0.5f, -13.4f },
	  { 0.1f, -0.9f, -0.5f, -0.9f, -4.4f, -8.4f, -1.4f, -0.5f, -13.4f }, 0.0f },
};

/* On 5.1 the Xbox's back pair played a sound behind as the direction
mirrored ahead (surround_split), but not quite: recorded there (its AC-3
decoded), at 135 degrees, 40 below and 40 above, the far ear 3 to 4 dB
brighter at 8 and 16 kHz than the 45 degrees it mirrors, and the near ear
within a dB of it, where at 120 and 150 degrees, level, the mirror is the
Xbox's to 0.6 dB. What the back pair adds to each ear's levels in each band
(dB, for a sound to the right; least squares as the table's, held near 0)
at 135 degrees, 40 below and 40 above; it fades out towards 90 and 180
degrees round (sin 2 azimuth), level and the poles (hrtf_place) */
static const struct
{
	float left[HRTF_BANDS], right[HRTF_BANDS];
} hrtf_behind[2] =
{
	{ { -0.1f, 0.1f, 1.6f, 0.2f, -1.6f, -1.6f, 1.4f, 4.2f, -1.5f },
	  { 0.2f, 0.1f, 0.3f, 0.1f, -0.1f, -1.2f, 0.0f, 4.2f, -2.2f } },
	{ { -0.1f, -0.4f, 0.5f, 0.5f, -4.2f, 4.3f, 3.3f, 4.9f, -5.4f },
	  { 0.1f, -0.2f, 0.1f, 0.3f, 0.1f, 0.3f, -0.8f, 0.6f, 1.1f } },
};

static struct
{
	/* each low pass's b0 (and b2), b1, a1 and a2 (a0 1) */
	float lowpass[HRTF_BANDS - 1][4];
} hrtf;

static void hrtf_initialize(void)
{
	int band;

	/* (RBJ's low pass, Q 1 / sqrt(2)) */
	for (band = 0; band < HRTF_BANDS - 1; band++)
	{
		float w = 2.0f * 3.14159265f * hrtf_crossovers[band] / OUTPUT_RATE;
		float alpha = sinf(w) / (2.0f * 0.70710678f), cosine = cosf(w), a0 = 1.0f + alpha;

		hrtf.lowpass[band][0] = 0.5f * (1.0f - cosine) / a0;
		hrtf.lowpass[band][1] = (1.0f - cosine) / a0;
		hrtf.lowpass[band][2] = -2.0f * cosine / a0;
		hrtf.lowpass[band][3] = (1.0f - alpha) / a0;
	}
}

/* DirectSound's azimuth (degrees, -180 to 180, the right positive) and
elevation (-90 to 90) of a direction right, up and ahead: in each octant
45 degrees times the lesser side over the greater, and 90 less the
greater's turn past 45 (surround_split's) */
static void hrtf_angles(float right, float up, float ahead, float *azimuth, float *elevation)
{
	float horizontal = sqrtf(right * right + ahead * ahead), height = fabsf(up);
	float front = fabsf(ahead), side = fabsf(right);

	if (height < horizontal)
		*elevation = height / horizontal * 45.0f;
	else
		*elevation = height > 0.0f ? 90.0f - horizontal / height * 45.0f : 0.0f;
	if (up < 0.0f)
		*elevation = -*elevation;
	if (front > side)
		*azimuth = side / front * 45.0f;
	else
		*azimuth = side > 0.0f ? 90.0f - front / side * 45.0f : 0.0f;
	if (ahead < 0.0f)
		*azimuth = 180.0f - *azimuth;
	if (right < 0.0f)
		*azimuth = -*azimuth;
}

/* out blended from a to b by t (0 to 1) */
static void hrtf_blend(const struct hrtf_direction *a, const struct hrtf_direction *b, float t,
	struct hrtf_direction *out)
{
	int band;

	for (band = 0; band < HRTF_BANDS; band++)
	{
		out->left[band] = a->left[band] + (b->left[band] - a->left[band]) * t;
		out->right[band] = a->right[band] + (b->right[band] - a->right[band]) * t;
	}
	out->delay = a->delay + (b->delay - a->delay) * t;
}

/* the table at azimuth (degrees, 0 ahead to 180 behind, to the right) and
elevation (-90 to 90), interpolated round, and up and down between its rows
and the poles */
static void hrtf_lookup(float azimuth, float elevation, struct hrtf_direction *out)
{
	static const float rows[HRTF_ELEVATIONS + 2] = { -90.0f, -60.0f, -40.0f, -20.0f, 0.0f, 20.0f, 40.0f, 60.0f, 90.0f };
	float column = azimuth / 15.0f, round;
	struct hrtf_direction at[2];
	int first, row, side;

	/* A non-finite angle must not become an out-of-bounds table index. */
	column = !(column >= 0.0f) ? 0.0f : column > HRTF_AZIMUTHS - 1 ? (float)(HRTF_AZIMUTHS - 1) : column;
	first = (int)column < HRTF_AZIMUTHS - 1 ? (int)column : HRTF_AZIMUTHS - 2;
	round = column - (float)first;
	if (elevation != elevation) elevation = 0.0f;
	elevation = elevation < -90.0f ? -90.0f : elevation > 90.0f ? 90.0f : elevation;
	for (row = 0; row < HRTF_ELEVATIONS && elevation > rows[row + 1]; row++)
		;
	for (side = 0; side < 2; side++)
	{
		int at_row = row + side;

		if (at_row == 0)
			at[side] = hrtf_poles[0];
		else if (at_row == HRTF_ELEVATIONS + 1)
			at[side] = hrtf_poles[1];
		else
			hrtf_blend(&hrtf_table[at_row - 1][first], &hrtf_table[at_row - 1][first + 1], round, &at[side]);
	}
	hrtf_blend(&at[0], &at[1], (elevation - rows[row]) / (rows[row + 1] - rows[row]), out);
}

/* the HRTF's parameters for a sound right, up and ahead of the head, as
far away (world units); behind: on 5.1, a sound behind mirrored ahead, as
the back pair plays it (hrtf_behind) */
static void hrtf_place(float right, float up, float ahead, float distance, BOOL behind,
	struct hrtf_parameters *parameters)
{
	float strength = distance < HRTF_NEAR ? distance / HRTF_NEAR : 1.0f, azimuth = 0.0f, elevation = 0.0f;
	struct hrtf_direction direction;
	int mirrored = 0, ear, band;

	if (distance > 1.0e-4f)
	{
		azimuth = atan2f(fabsf(right), ahead) * (180.0f / 3.14159265f);
		elevation = atan2f(up, sqrtf(right * right + ahead * ahead)) * (180.0f / 3.14159265f);
		mirrored = right < 0.0f;
	}
	hrtf_lookup(azimuth, elevation, &direction);
	if (behind && elevation != 0.0f)
	{
		/* (all of it at 45 degrees mirrored, 40 up or down; none ahead or to
		the side, level or at the poles) */
		float height = fabsf(elevation);
		float share = sinf(2.0f * azimuth * (3.14159265f / 180.0f)) *
			(height < 40.0f ? height / 40.0f : (90.0f - height) / 50.0f);

		for (band = 0; band < HRTF_BANDS; band++)
		{
			direction.left[band] += share * hrtf_behind[elevation > 0.0f].left[band];
			direction.right[band] += share * hrtf_behind[elevation > 0.0f].right[band];
		}
	}
	for (ear = 0; ear < 2; ear++)
	{
		/* (a sound to the left: the right's ears swapped) */
		const float *decibels = ear ^ mirrored ? direction.right : direction.left;
		float gain[HRTF_BANDS];

		for (band = 0; band < HRTF_BANDS; band++)
		{
			parameters->decibels[ear][band] = strength * decibels[band];
			gain[band] = powf(10.0f, parameters->decibels[ear][band] / 20.0f);
		}
		/* (each band a low pass less the one below: each low pass's weight
		its band's gain less the next's) */
		for (band = 0; band < HRTF_BANDS - 1; band++)
			parameters->weights[ear][band] = gain[band] - gain[band + 1];
		parameters->weights[ear][HRTF_BANDS - 1] = gain[HRTF_BANDS - 1];
	}
	/* (the nearer ear at no delay; whole frames, as the Xbox's) */
	parameters->delay[mirrored] = floorf(strength * direction.delay + 0.5f);
	parameters->delay[!mirrored] = 0.0f;
}

/* the frame delay frames before newest (at least 1: one frame each side of
it is read; at most HRTF_FRAMES - 2), interpolated (Catmull-Rom) */
static float hrtf_read(const float *newest, float delay)
{
	unsigned long whole = (unsigned long)delay;
	const float *at = newest - whole;
	float t = delay - (float)whole;
	float newer = at[1];
	float x0 = at[0];
	float x1 = at[-1];
	float older = at[-2];

	return x0 + 0.5f * t * (x1 - newer + t * (2.0f * newer - 5.0f * x0 + 4.0f * x1 - older +
		t * (3.0f * (x0 - x1) + older - newer)));
}

/* parameters moved on by step (a frame's share of a ramp) */
static void hrtf_step(struct hrtf_parameters *parameters, const struct hrtf_parameters *step)
{
	int ear, band;

	for (ear = 0; ear < 2; ear++)
	{
		parameters->delay[ear] += step->delay[ear];
		for (band = 0; band < HRTF_BANDS; band++)
			parameters->weights[ear][band] += step->weights[ear][band];
	}
	parameters->gain += step->gain;
	parameters->pair[0] += step->pair[0];
	parameters->pair[1] += step->pair[1];
}

/* the crossfade between the pan and the HRTF a frame on: mix moved by
ramp_mix (a frame's share of HRTF_FADE_FRAMES), no further than end */
static float hrtf_mix_step(float mix, float ramp_mix, float end)
{
	mix += ramp_mix;
	return (ramp_mix > 0.0f && mix > end) || (ramp_mix < 0.0f && mix < end) ? end : mix;
}

/* frames of a mono voice's direct path (direct) through the HRTF to each
ear, added to output (frames of output_channels: on 5.1 to the front pair
and the back pair, at their gains) at mix of the voice (the crossfade with
the pan, which the voice's mix added: hrtf_mix_step to mix_end), from the
voice's parameters (stream->hrtf_current) to target, ramped over the
ramp_frames left (more than frames: on in the next mix); a pass of its own
over the mix, each ear's line one run of frames (the voice's last and
these), so that it keeps to registers */
static void hrtf_process(struct sdl_stream *stream, const float *direct, float *output, unsigned long frames,
	unsigned long ramp_frames, const struct hrtf_parameters *target, float mix, float ramp_mix, float mix_end)
{
	float line[2][HRTF_FRAMES + MIX_CHUNK_FRAMES];
	struct hrtf_parameters now = stream->hrtf_current, ramp;
	float state[HRTF_BANDS - 1][2];
	float share = ramp_frames ? 1.0f / (float)ramp_frames : 0.0f;
	unsigned long frame;
	int ear, band;

	for (ear = 0; ear < 2; ear++)
	{
		ramp.delay[ear] = (target->delay[ear] - now.delay[ear]) * share;
		for (band = 0; band < HRTF_BANDS; band++)
			ramp.weights[ear][band] = (target->weights[ear][band] - now.weights[ear][band]) * share;
		memcpy(line[ear], stream->hrtf_frames[ear], sizeof(stream->hrtf_frames[ear]));
	}
	ramp.gain = (target->gain - now.gain) * share;
	ramp.pair[0] = (target->pair[0] - now.pair[0]) * share;
	ramp.pair[1] = (target->pair[1] - now.pair[1]) * share;
	memcpy(state, stream->hrtf_bands, sizeof(state));
	for (frame = 0; frame < frames; frame++)
	{
		/* the low passes (transposed direct form II), then each ear its
		bands' levels and its delay */
		float x = direct[frame], low[HRTF_BANDS - 1];

		for (band = 0; band < HRTF_BANDS - 1; band++)
		{
			const float *c = hrtf.lowpass[band];
			float y = c[0] * x + state[band][0];

			state[band][0] = flush_denormal(c[1] * x - c[2] * y + state[band][1]);
			state[band][1] = flush_denormal(c[0] * x - c[3] * y);
			low[band] = y;
		}
		for (ear = 0; ear < 2; ear++)
		{
			const float *weights = now.weights[ear];
			float heard = weights[HRTF_BANDS - 1] * x;

			for (band = 0; band < HRTF_BANDS - 1; band++)
				heard += weights[band] * low[band];
			line[ear][HRTF_FRAMES + frame] = heard;
			heard = hrtf_read(line[ear] + HRTF_FRAMES + frame, 1.0f + now.delay[ear]) * now.gain * mix;
			if (output_channels == 6)
			{
				output[frame * 6 + ear] += heard * now.pair[0];
				output[frame * 6 + 4 + ear] += heard * now.pair[1];
			}
			else
				output[frame * 2 + ear] += heard;
		}
		/* (at the ramp's end, exactly the new ones, held) */
		if (frame + 1 == ramp_frames)
		{
			memcpy(now.delay, target->delay, sizeof(now.delay));
			memcpy(now.weights, target->weights, sizeof(now.weights));
			now.gain = target->gain;
			now.pair[0] = target->pair[0];
			now.pair[1] = target->pair[1];
			memset(&ramp, 0, sizeof(ramp));
		}
		else
			hrtf_step(&now, &ramp);
		mix = hrtf_mix_step(mix, ramp_mix, mix_end);
	}
	for (ear = 0; ear < 2; ear++)
		memcpy(stream->hrtf_frames[ear], line[ear] + frames, sizeof(stream->hrtf_frames[ear]));
	memcpy(now.decibels, target->decibels, sizeof(now.decibels));
	stream->hrtf_current = now;
	memcpy(stream->hrtf_bands, state, sizeof(state));
}
