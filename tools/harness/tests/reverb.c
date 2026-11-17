/* Adapted from OpenH1/OpenH1 6750db5a6c9716857bd1e63d9ec188a20c49f1d3.
 * Copyright OpenH1 contributors; AGPL-3.0-or-later. */
/*
REVERB.C (test)

The real reverb of port/linux/src/dsound_sdl.c (test_reverb.py takes the
mixer as under_test.inc, as test_mixer.py does) against the Xbox's: the
I3DL2 reverb of the DSP program in the effects image the game downloads
(sound_dsound_xbox.c), with the parameters the XDK DirectSound worked out
from the listener properties, for each sound environment of the maps (their
tags' values, turned into I3DL2's as sound_dsound_xbox.c
dsound_set_listener_properties turns them). The figures of the Xbox's below
(xbox_parameters, xbox_figures) were measured by running that program, as
the Xbox's own code, on a DSP56300 interpreter, with the parameters its own
DirectSound code produced (see test_reverb.py on how to measure them again
from a default.xbe):
	- "parameters": the delays the environment sets, sample for sample, and
	  the gains and filters, to the DSP's 24-bit precision;
	- "response": the impulse response of each of the four channels (front
	  left and right, back left and right): the energy of the reflections
	  and of the late reverberation, when each channel's reflections start,
	  and the late reverberation's decay time at low frequencies and at the
	  HF reference;
	- "levels": a 3D voice's direct path, against which the reverb's levels
	  are, has an energy of 1 over the output channels;
	- "transition": the game moves the environment a step at each tick
	  (scenario.c scenario_get_sound_environment) on the way from one to
	  another, and the reverb follows: each fade goes on from where the last
	  got to, never back towards the environment left, and it reaches the
	  new one soon after the game does;
	- "change": a new environment set while the reverb reverberates is
	  the reverb's within 2 ms, as an Xbox's recordings show (a click
	  played 2 ms after the environment was set echoed as the new room's
	  levels);
	- "delays": but its delays only REVERB_DELAY_HOLD_FRAMES later (the
	  test bed's ninth run).
"metrics" prints the figures of each environment, beside the Xbox's;
"measure" prints those of four-channel responses (raw floats) given, in the
form of xbox_figures.
*/

#include "mixer_fixture.h"

#define PI 3.14159265358979323846

/* ---------- the maps' sound environments

Their tags' values (sound_environment_definitions.h: gains, seconds, 0 to 1
for the diffusion and density, hertz), every one the maps have: each is one
of I3DL2's presets of its name */

struct environment_tag
{
	const char *name;
	float room, room_hf, room_rolloff, decay_time, decay_hf_ratio, reflections, reflections_delay,
		reverb, reverb_delay, diffusion, density, hf_reference;
};

static const struct environment_tag environments[] =
{
	{ "alley", 0.316f, 0.72f, 0, 1.49f, 0.86f, 0.079f, 0.007f, 0.0995f, 0.011f, 1, 1, 5000 },
	{ "arena", 0.316f, 0.447f, 0, 7.24f, 0.33f, 0.0825f, 0.02f, 0.102f, 0.03f, 1, 1, 5000 },
	{ "cave", 0.316f, 1.0f, 0, 2.91f, 1.3f, 0.158f, 0.015f, 0.0707f, 0.022f, 1, 1, 5000 },
	{ "concert_hall", 0.316f, 0.5623f, 0, 3.92f, 0.7f, 0.0767f, 0.02f, 0.0997f, 0.029f, 1, 1, 5000 },
	{ "forest", 0.316f, 0.022f, 0, 1.49f, 0.54f, 0.0165f, 0.162f, 0.0492f, 0.088f, 0.79f, 1, 5000 },
	{ "hallway", 0.316f, 0.71f, 0, 1.49f, 0.59f, 0.077f, 0.007f, 0.166f, 0.011f, 1, 1, 5000 },
	{ "hangar", 0.316f, 0.316f, 0, 10.05f, 0.23f, 0.158f, 0.02f, 0.125f, 0.03f, 1, 1, 5000 },
	{ "mountains", 0.316f, 0.0562f, 0, 1.49f, 0.21f, 0.0128f, 0.3f, 0.009f, 0.1f, 0.27f, 1, 5000 },
	{ "quarry", 0.316f, 0.316f, 0, 1.49f, 0.83f, 0.0f, 0.061f, 0.1777f, 0.025f, 1, 1, 5000 },
	{ "stone_room", 0.316f, 0.71f, 0, 2.31f, 0.64f, 0.139f, 0.012f, 0.11f, 0.017f, 1, 1, 5000 },
	{ "underwater", 0.316f, 0.01f, 0, 1.49f, 0.1f, 0.188f, 0.007f, 0.71f, 0.011f, 1, 1, 5000 },
};
#define ENVIRONMENTS (sizeof(environments) / sizeof(environments[0]))

/* sound_dsound_xbox.c dsound_volume_from_gain */
static LONG volume_from_gain(float gain, LONG maximum)
{
	LONG volume;

	if (gain == 0.0f)
		return DSBVOLUME_MIN;
	volume = (LONG)(2000.0 * log10(gain) + maximum);
	return volume < DSBVOLUME_MIN ? DSBVOLUME_MIN : volume > maximum ? maximum : volume;
}

/* sound_dsound_xbox.c dsound_set_listener_properties */
static DSI3DL2LISTENER listener_from_tag(const struct environment_tag *tag)
{
	DSI3DL2LISTENER properties;

	properties.lRoom = volume_from_gain(tag->room, 0);
	properties.lRoomHF = volume_from_gain(tag->room_hf, 0);
	properties.flRoomRolloffFactor = tag->room_rolloff;
	properties.flDecayTime = tag->decay_time;
	properties.flDecayHFRatio = tag->decay_hf_ratio;
	properties.lReflections = volume_from_gain(tag->reflections, 1000);
	properties.flReflectionsDelay = tag->reflections_delay;
	properties.lReverb = volume_from_gain(tag->reverb, 2000);
	properties.flReverbDelay = tag->reverb_delay;
	properties.flDiffusion = tag->diffusion * 100.0f;
	properties.flDensity = tag->density * 100.0f;
	properties.flHFReference = tag->hf_reference;
	return properties;
}

/* ---------- the Xbox's

The parameters the XDK DirectSound downloaded to the DSP for each
environment: the delays in the DSP's samples (24 kHz), the filters' and
gains' 24-bit words (fractions of 2^23; the reflections' and the reverb's
gains are 1/128 of the gain, the DSP shifting its output up 7 bits). The
room level is in the DSP's gains there, where the port's send carries it:
the gains here are without it (the Xbox's word times 10^(-room/2000)). */

struct xbox_parameters
{
	long reflections_delay, reflection_taps[4], late_delay, chain[4][2], comb_delays[4], loop_length;
	/* the one-pole filters (y = a y' + b x): the lines', the chain's, the
	loops' */
	long line_filters[4][2], chain_filter[2], loop_filter[2];
	long onset_gains[8], loop_input, network_input;
	/* the reflections' (each channel's) and the reverb's gains, of 2^16 */
	long reflections_gains[4], late_gain;
};

static const struct xbox_parameters xbox_parameters[] =
{
	{ 168, { 196, 184, 212, 228 }, 428, { { 932, 492 }, { 4520, 6000 }, { 5320, 5780 }, { 5320, 3160 } }, { 152, 140, 148, 164 }, 23292, { { 462025, 6061582 }, { 551832, 5664184 }, { 660933, 5212532 }, { 787540, 4713837 } }, { 1625998, 2195908 }, { 4353618, 44858 }, { 8388607, -8388607, -8388607, 8388607, 8388607, 8388607, -8388607, -8388607 }, 0, 1677721, { 2953, 2694, 2798, 3109 }, 20629 }, /* alley */
	{ 476, { 552, 524, 600, 648 }, 1192, { { 960, 508 }, { 4520, 6000 }, { 5480, 5956 }, { 5480, 3256 } }, { 264, 244, 252, 280 }, 24000, { { 1076149, 6919320 }, { 1271671, 6657223 }, { 1491688, 6362901 }, { 1747725, 6021792 } }, { 3340700, 3977510 }, { 7027834, 524548 }, { 8388607, -8388607, -8388607, 8388607, 8388607, 8388607, -8388607, -8388607 }, 0, 1677721, { 3082, 2811, 2919, 3244 }, 21133 }, /* arena */
	{ 356, { 412, 392, 448, 480 }, 880, { { 960, 508 }, { 4520, 6000 }, { 5480, 5956 }, { 5480, 3256 } }, { 216, 200, 208, 232 }, 24000, { { 0, 7314588 }, { 0, 7107058 }, { 0, 6857880 }, { 0, 6571884 } }, { 0, 4636496 }, { 0, 781969 }, { 8388607, -8388607, -8388607, 8388607, 8388607, 8388607, -8388607, -8388607 }, 0, 1677721, { 5906, 5388, 5595, 6217 }, 14654 }, /* cave */
	{ 476, { 548, 520, 596, 640 }, 1168, { { 960, 508 }, { 4520, 6000 }, { 5480, 5956 }, { 5480, 3256 } }, { 260, 236, 248, 276 }, 24000, { { 462025, 7162852 }, { 551832, 6928471 }, { 660933, 6653479 }, { 787540, 6344166 } }, { 1667009, 4329847 }, { 4435309, 679140 }, { 8388607, -8388607, -8388607, 8388607, 8388607, 8388607, -8388607, -8388607 }, 0, 1677721, { 2866, 2615, 2715, 3017 }, 20676 }, /* concert_hall */
	{ 3888, { 4108, 4028, 4252, 4392 }, 5996, { { 932, 492 }, { 4520, 6000 }, { 5320, 5780 }, { 5320, 3160 } }, { 608, 548, 576, 644 }, 23292, { { 1942364, 4929543 }, { 2248816, 4437656 }, { 2597854, 3906025 }, { 2971797, 3359260 } }, { 5084636, 1072843 }, { 8170313, 2426 }, { 6627000, -6627000, -6627000, 6627000, 6627000, 6627000, -6627000, -6627000 }, 1761608, 1325400, { 616, 562, 583, 648 }, 10197 }, /* forest */
	{ 168, { 196, 184, 212, 228 }, 428, { { 932, 492 }, { 4520, 6000 }, { 5320, 5780 }, { 5320, 3160 } }, { 152, 140, 148, 164 }, 23292, { { 1650658, 5152616 }, { 1919521, 4675660 }, { 2234755, 4150945 }, { 2572068, 3607154 } }, { 4523156, 1255163 }, { 7951620, 4858 }, { 8388607, -8388607, -8388607, 8388607, 8388607, 8388607, -8388607, -8388607 }, 0, 1677721, { 2876, 2624, 2725, 3027 }, 34393 }, /* hallway */
	{ 476, { 552, 524, 600, 648 }, 1192, { { 960, 508 }, { 4520, 6000 }, { 5480, 5956 }, { 5480, 3256 } }, { 264, 244, 252, 280 }, 24000, { { 1253492, 6861215 }, { 1474556, 6595270 }, { 1723689, 6292088 }, { 1995146, 5959860 } }, { 3741908, 3914214 }, { 7420903, 486679 }, { 8388607, -8388607, -8388607, 8388607, 8388607, 8388607, -8388607, -8388607 }, 0, 1677721, { 5906, 5388, 5595, 6217 }, 25880 }, /* hangar */
	{ 7200, { 7452, 7360, 7616, 7776 }, 9600, { { 932, 492 }, { 4520, 6000 }, { 5320, 5780 }, { 5320, 3160 } }, { 680, 612, 640, 720 }, 23292, { { 5239210, 2408393 }, { 5779891, 1885501 }, { 6328549, 1389567 }, { 6841822, 959246 } }, { 8241421, 47793 }, { 0, 0 }, { 2264924, -2264924, -2264924, 2264924, 2264924, 2264924, -2264924, -2264924 }, 6123683, 452984, { 478, 436, 453, 503 }, 1866 }, /* mountains */
	{ 1464, { 1528, 1504, 1568, 1608 }, 2064, { { 932, 492 }, { 4520, 6000 }, { 5320, 5780 }, { 5320, 3160 } }, { 236, 216, 224, 252 }, 23292, { { 573926, 5976009 }, { 682352, 5569848 }, { 818498, 5106251 }, { 959520, 4607183 } }, { 1949948, 2090717 }, { 4975145, 37948 }, { 8388607, -8388607, -8388607, 8388607, 8388607, 8388607, -8388607, -8388607 }, 0, 1677721, { 0, 0, 0, 0 }, 36811 }, /* quarry */
	{ 288, { 328, 316, 356, 384 }, 696, { { 960, 508 }, { 4520, 6000 }, { 5480, 5956 }, { 5480, 3256 } }, { 188, 172, 180, 200 }, 24000, { { 959520, 6250798 }, { 1123659, 5891590 }, { 1325676, 5476272 }, { 1559441, 5021895 } }, { 3035675, 2535664 }, { 6665251, 86671 }, { 8388607, -8388607, -8388607, 8388607, 8388607, 8388607, -8388607, -8388607 }, 0, 1677721, { 5198, 4742, 4924, 5471 }, 22776 }, /* stone_room */
	{ 168, { 196, 184, 212, 228 }, 428, { { 932, 492 }, { 4520, 6000 }, { 5320, 5780 }, { 5320, 3160 } }, { 152, 140, 148, 164 }, 23292, { { 7525033, 660389 }, { 7858795, 382933 }, { 8101346, 193766 }, { 8251792, 84846 } }, { 8388198, 133 }, { 0, 0 }, { 8388607, -8388607, -8388607, 8388607, 8388607, 8388607, -8388607, -8388607 }, 0, 1677721, { 7028, 6411, 6658, 7398 }, 147055 }, /* underwater */
};


/* ---------- impulse responses */

static unsigned long serial;

/* the reverb's four channels' response to a unit impulse sent to it, frames
long, in the environment of properties (a new environment, the reverb
silent): channel by channel, from the 5.1 output's front and back pairs */
static float *response(const DSI3DL2LISTENER *properties, unsigned long frames)
{
	float *output = calloc(frames * 6, sizeof(float)), *channels = calloc(frames * 4, sizeof(float));
	static float send[MIX_CHUNK_FRAMES];
	unsigned long frame, saved_channels = output_channels;

	output_channels = 6;
	reverb_clear();
	reverb.level = 1.0f;
	reverb_update(properties, ++serial);
	for (frame = 0; frame < frames; frame += MIX_CHUNK_FRAMES)
	{
		unsigned long count = frames - frame < MIX_CHUNK_FRAMES ? frames - frame : MIX_CHUNK_FRAMES;

		memset(send, 0, sizeof(send));
		if (!frame)
			send[0] = 1.0f;
		reverb_process(send, output + frame * 6, count, 1.0f);
	}
	for (frame = 0; frame < frames; frame++)
	{
		channels[frame * 4] = output[frame * 6];
		channels[frame * 4 + 1] = output[frame * 6 + 1];
		channels[frame * 4 + 2] = output[frame * 6 + 4];
		channels[frame * 4 + 3] = output[frame * 6 + 5];
	}
	free(output);
	output_channels = saved_channels;
	return channels;
}

/* the response with only the reflections (!late), or only the late
reverberation (late) */
static float *part_response(DSI3DL2LISTENER properties, BOOL late, unsigned long frames)
{
	if (late)
		properties.lReflections = DSBVOLUME_MIN;
	else
		properties.lReverb = DSBVOLUME_MIN;
	return response(&properties, frames);
}

static unsigned long response_frames(const DSI3DL2LISTENER *properties)
{
	return (unsigned long)((properties->flDecayTime * 1.5f + 1.5f) * OUTPUT_RATE);
}

/* the four channels through a band pass about hertz an octave wide (RBJ's,
0 dB at its center, twice over), summed in energy: the band's energy at each
frame */
static double *band_energy(const float *output, unsigned long frames, double hertz)
{
	double w = 2.0 * PI * hertz / OUTPUT_RATE;
	double alpha = sin(w) * sinh(log(2.0) / 2.0 * w / sin(w));
	double b0 = alpha / (1.0 + alpha), b2 = -alpha / (1.0 + alpha);
	double a1 = -2.0 * cos(w) / (1.0 + alpha), a2 = (1.0 - alpha) / (1.0 + alpha);
	double *energy = calloc(frames, sizeof(double));
	int channel;

	for (channel = 0; channel < 4; channel++)
	{
		double x1[2] = { 0, 0 }, x2[2] = { 0, 0 }, y1[2] = { 0, 0 }, y2[2] = { 0, 0 };
		unsigned long frame;

		for (frame = 0; frame < frames; frame++)
		{
			double x = output[frame * 4 + channel];
			int stage;

			for (stage = 0; stage < 2; stage++)
			{
				double y = b0 * x + b2 * x2[stage] - a1 * y1[stage] - a2 * y2[stage];

				x2[stage] = x1[stage];
				x1[stage] = x;
				y2[stage] = y1[stage];
				y1[stage] = y;
				x = y;
			}
			energy[frame] += x * x;
		}
	}
	return energy;
}

/* the decay time (seconds to fall 60 dB) of the band's energy: Schroeder's
backward integral, a line fitted to it from 5 to 25 dB down */
static double decay_time(const double *energy, unsigned long frames)
{
	double *integral = malloc(frames * sizeof(double));
	double sum = 0.0, sx = 0, sy = 0, sxx = 0, sxy = 0, n = 0;
	unsigned long frame;

	for (frame = frames; frame-- > 0;)
	{
		sum += energy[frame];
		integral[frame] = sum;
	}
	for (frame = 0; frame < frames; frame++)
	{
		double level = 10.0 * log10(integral[frame] / integral[0] + 1e-300);

		if (level <= -5.0 && level >= -25.0)
		{
			double t = (double)frame / OUTPUT_RATE;

			sx += t;
			sy += level;
			sxx += t * t;
			sxy += t * level;
			n++;
		}
	}
	free(integral);
	if (n < 2)
		return 0.0;
	return -60.0 / ((n * sxy - sx * sy) / (n * sxx - sx * sx));
}

/* the figures of an environment: each channel's energy of the reflections
and of the late reverberation (decibels of the impulse sent), the frame its
reflections start and their sign, and the late reverberation's decay times
at 500 Hz and at the HF reference */
struct figures
{
	double reflections_db[4], late_db[4];
	long first[4];
	/* the sign of the first reflection (front left's) */
	int polarity;
	double decay_low, decay_high;
};

/* the Xbox's figures of each environment */
static const struct figures xbox_figures[] =
{
	{ { -16.27, -17.07, -16.74, -15.82 }, { 2.89, 2.61, 2.29, 1.95 }, { 336, 336, 336, 336 }, -1, 1.528, 1.304 }, /* alley */
	{ { -15.89, -16.70, -16.37, -15.44 }, { 5.82, 5.67, 5.32, 5.02 }, { 952, 952, 952, 952 }, -1, 7.032, 2.900 }, /* arena */
	{ { -10.20, -11.00, -10.67, -9.75 }, { 2.64, 2.44, 2.13, 1.78 }, { 712, 712, 712, 712 }, -1, 2.941, 2.961 }, /* cave */
	{ { -16.53, -17.34, -17.01, -16.08 }, { 5.34, 5.16, 4.85, 4.57 }, { 952, 952, 952, 952 }, -1, 3.922, 2.847 }, /* concert_hall */
	{ { -30.17, -30.96, -30.61, -29.72 }, { -6.19, -6.50, -6.79, -7.04 }, { 7776, 7776, 7776, 7776 }, -1, 1.480, 1.052 }, /* forest */
	{ { -16.50, -17.31, -16.97, -16.05 }, { 6.50, 6.23, 5.93, 5.64 }, { 336, 336, 336, 336 }, -1, 1.511, 0.942 }, /* hallway */
	{ { -10.20, -11.00, -10.67, -9.75 }, { 7.97, 7.85, 7.48, 7.20 }, { 952, 952, 952, 952 }, -1, 9.660, 3.056 }, /* hangar */
	{ { -31.99, -32.80, -32.44, -31.55 }, { -29.06, -29.16, -29.25, -29.33 }, { 2400, 2400, 2400, 2400 }, -1, 1.116, 0.509 }, /* mountains */
	{ { -300.00, -300.00, -300.00, -300.00 }, { 7.82, 7.54, 7.21, 6.88 }, { -1, -1, -1, -1 }, 0, 1.498, 1.249 }, /* quarry */
	{ { -11.31, -12.11, -11.79, -10.86 }, { 4.24, 3.99, 3.68, 3.35 }, { 576, 576, 576, 576 }, -1, 2.323, 1.555 }, /* stone_room */
	{ { -8.68, -9.48, -9.15, -8.23 }, { 17.17, 17.12, 17.08, 17.04 }, { 336, 336, 336, 336 }, -1, 0.481, 0.059 }, /* underwater */
};

static double channel_db(const float *output, unsigned long frames, int channel)
{
	double sum = 0.0;
	unsigned long frame;

	for (frame = 0; frame < frames; frame++)
		sum += (double)output[frame * 4 + channel] * output[frame * 4 + channel];
	return 10.0 * log10(sum + 1e-30);
}

static struct figures measure_responses(const float *early, unsigned long early_frames, const float *late,
	unsigned long late_frames, float hf_reference)
{
	struct figures figures;
	double *energy;
	int channel;

	for (channel = 0; channel < 4; channel++)
	{
		unsigned long frame;

		figures.reflections_db[channel] = channel_db(early, early_frames, channel);
		figures.late_db[channel] = channel_db(late, late_frames, channel);
		figures.first[channel] = -1;
		for (frame = 0; frame < early_frames && figures.first[channel] < 0; frame++)
		{
			if (fabsf(early[frame * 4 + channel]) > 1.0e-3f)
				figures.first[channel] = (long)frame;
		}
	}
	figures.polarity = figures.first[0] < 0 ? 0 : early[figures.first[0] * 4] < 0.0f ? -1 : 1;
	energy = band_energy(late, late_frames, 500.0);
	figures.decay_low = decay_time(energy, late_frames);
	free(energy);
	energy = band_energy(late, late_frames, hf_reference);
	figures.decay_high = decay_time(energy, late_frames);
	free(energy);
	return figures;
}

/* the environment's figures, with the room level (the send's) out, and its
Room HF flat */
static struct figures measure(DSI3DL2LISTENER properties)
{
	unsigned long frames = response_frames(&properties);
	float *early, *late;
	struct figures figures;

	properties.lRoom = 0;
	properties.lRoomHF = 0;
	early = part_response(properties, FALSE, OUTPUT_RATE / 2);
	late = part_response(properties, TRUE, frames);
	figures = measure_responses(early, OUTPUT_RATE / 2, late, frames, properties.flHFReference);
	free(early);
	free(late);
	return figures;
}

static float *read_floats(const char *path, unsigned long *frames)
{
	FILE *file = fopen(path, "rb");
	float *data;
	long size;

	if (!file)
		return NULL;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	data = malloc(size);
	*frames = fread(data, sizeof(float), size / sizeof(float), file) / 4;
	fclose(file);
	return data;
}

/* ---------- transitions */

/* the late gain the reverb plays at, between its fade's ends */
static float late_gain_now(void)
{
	float fade = 1.0f - (float)reverb.fade / REVERB_FADE_FRAMES;

	return reverb_blend(reverb.from.late_gain, reverb.to.late_gain, fade);
}

static double word(float value)
{
	return floor(value * 8388608.0 + 0.5);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	unsigned long index;

	reverb_initialize();
	CASE("ring-wrap")
	{
		float guarded[96], *line = guarded + 16;
		long position, delay, step, expected;

		for (step = 0; step < 96; step++) guarded[step] = -1000.0f;
		for (step = 0; step < 64; step++) line[step] = (float)step;
		for (position = 0; position < 64; position++)
		for (delay = 0; delay < 192; delay++)
		{
			expected = position;
			for (step = 0; step < (delay < 16 ? 16 : delay); step++)
			{
				if (--expected < 0) expected = 63;
			}
			reverb.position = position;
			CHECK(reverb_tap(line, 64, delay) == line[expected],
				"ring wrap: position %ld, delay %ld read outside the expected slot", position, delay);
		}
		return 0;
	}
	CASE("metrics")
	{
		printf("%-13s %-27s %-27s %-11s %-23s %s\n", "environment", "reflections dB FL FR BL BR", "late dB FL FR BL BR",
			"RT60 500Hz", "first reflection, ms", "(Xbox's under)");
		for (index = 0; index < ENVIRONMENTS; index++)
		{
			DSI3DL2LISTENER properties = listener_from_tag(&environments[index]);
			struct figures ours = measure(properties);
			const struct figures *xbox = &xbox_figures[index];
			const struct figures *both[2] = { &ours, xbox };
			int which, channel;

			for (which = 0; which < 2; which++)
			{
				const struct figures *f = both[which];

				printf("%-13s", which ? "  (Xbox)" : environments[index].name);
				for (channel = 0; channel < 4; channel++)
					printf(" %6.2f", f->reflections_db[channel]);
				for (channel = 0; channel < 4; channel++)
					printf(" %6.2f", f->late_db[channel]);
				printf("  %5.2f %5.2f ", f->decay_low, f->decay_high);
				for (channel = 0; channel < 4; channel++)
					printf(" %5.1f", f->first[channel] * 1000.0 / OUTPUT_RATE);
				printf("\n");
			}
		}
		return 0;
	}

	/* the figures of the four-channel responses in argv[3] (the reflections)
	and argv[4] (the late reverberation), raw floats, of environment argv[2]
	(its index) */
	CASE("measure")
	{
		DSI3DL2LISTENER properties = listener_from_tag(&environments[atoi(argv[2])]);
		unsigned long early_frames, late_frames;
		float *early = read_floats(argv[3], &early_frames), *late = read_floats(argv[4], &late_frames);
		struct figures f;
		int channel;

		if (!early || !late)
			return 2;
		f = measure_responses(early, early_frames < OUTPUT_RATE / 2 ? early_frames : OUTPUT_RATE / 2, late,
			late_frames, properties.flHFReference);
		printf("\t{ {");
		for (channel = 0; channel < 4; channel++)
			printf(" %.2f%s", f.reflections_db[channel], channel < 3 ? "," : " }, {");
		for (channel = 0; channel < 4; channel++)
			printf(" %.2f%s", f.late_db[channel], channel < 3 ? "," : " }, {");
		for (channel = 0; channel < 4; channel++)
			printf(" %ld%s", f.first[channel], channel < 3 ? "," : " },");
		printf(" %d, %.3f, %.3f }, /* %s */\n", f.polarity, f.decay_low, f.decay_high, environments[atoi(argv[2])].name);
		return 0;
	}

	/* the four-channel response of environment argv[2] (its index), late
	(argv[3] "late") or the reflections, as raw floats (as "measure" takes
	them) */
	CASE("dump")
	{
		DSI3DL2LISTENER properties = listener_from_tag(&environments[atoi(argv[2])]);
		BOOL late = argc > 3 && !strcmp(argv[3], "late");
		unsigned long frames = late ? response_frames(&properties) : OUTPUT_RATE / 2;
		float *output;

		properties.lRoom = 0;
		properties.lRoomHF = 0;
		output = part_response(properties, late, frames);
		fwrite(output, sizeof(float), frames * 4, stdout);
		free(output);
		return 0;
	}

	CASE("parameters")
	{
		for (index = 0; index < ENVIRONMENTS; index++)
		{
			DSI3DL2LISTENER properties = listener_from_tag(&environments[index]);
			const struct xbox_parameters *xbox = &xbox_parameters[index];
			const char *name = environments[index].name;
			struct reverb_parameters p;
			/* the room level, which the Xbox's gains hold, of 2^16 */
			double room = 65536.0 * reverb_gain(properties.lRoom);
			int n;

			reverb_parameters_set(&p, &properties);
			CHECK(p.reflections_delay == xbox->reflections_delay && p.late_delay == xbox->late_delay &&
				p.loop_length == xbox->loop_length, "%s: the reflections, late and loop delays %ld %ld %ld, not %ld %ld %ld",
				name, p.reflections_delay, p.late_delay, p.loop_length, xbox->reflections_delay, xbox->late_delay,
				xbox->loop_length);
			for (n = 0; n < 4; n++)
			{
				CHECK(p.reflection_taps[n] == xbox->reflection_taps[n] && p.comb_delays[n] == xbox->comb_delays[n],
					"%s: channel %d's reflection tap and comb %ld %ld, not %ld %ld", name, n, p.reflection_taps[n],
					p.comb_delays[n], xbox->reflection_taps[n], xbox->comb_delays[n]);
				CHECK(p.chain[n][0] == xbox->chain[n][0] && p.chain[n][1] == xbox->chain[n][1],
					"%s: the chain's delay %d's taps %ld %ld, not %ld %ld", name, n, p.chain[n][0], p.chain[n][1],
					xbox->chain[n][0], xbox->chain[n][1]);
				CHECK(fabs(word(p.lines[n].a) - xbox->line_filters[n][0]) <= 1 &&
					fabs(word(p.lines[n].b) - xbox->line_filters[n][1]) <= 1,
					"%s: line %d's filter %.0f %.0f, not %ld %ld", name, n, word(p.lines[n].a), word(p.lines[n].b),
					xbox->line_filters[n][0], xbox->line_filters[n][1]);
				CHECK(fabs(p.reflections_gains[n] * room - xbox->reflections_gains[n]) <= 1.0 + 1e-3 * xbox->reflections_gains[n],
					"%s: channel %d's reflections gain %.1f, not %ld", name, n, p.reflections_gains[n] * room,
					xbox->reflections_gains[n]);
			}
			CHECK(fabs(word(p.chain_filter.a) - xbox->chain_filter[0]) <= 1 &&
				fabs(word(p.chain_filter.b) - xbox->chain_filter[1]) <= 1 &&
				fabs(word(p.loop_filter.a) - xbox->loop_filter[0]) <= 1 &&
				fabs(word(p.loop_filter.b) - xbox->loop_filter[1]) <= 1,
				"%s: the chain's and loops' filters %.0f %.0f %.0f %.0f, not %ld %ld %ld %ld", name,
				word(p.chain_filter.a), word(p.chain_filter.b), word(p.loop_filter.a), word(p.loop_filter.b),
				xbox->chain_filter[0], xbox->chain_filter[1], xbox->loop_filter[0], xbox->loop_filter[1]);
			for (n = 0; n < 8; n++)
				CHECK(fabs(word(p.onset_gains[n]) - xbox->onset_gains[n]) <= 1, "%s: onset gain %d %.0f, not %ld", name, n,
					word(p.onset_gains[n]), xbox->onset_gains[n]);
			CHECK(fabs(word(p.loop_input) - xbox->loop_input) <= 1 && fabs(word(p.network_input) - xbox->network_input) <= 1,
				"%s: the loops' and network's inputs %.0f %.0f, not %ld %ld", name, word(p.loop_input),
				word(p.network_input), xbox->loop_input, xbox->network_input);
			CHECK(p.input_gain == (properties.lRoomHF ? 0.5f : -0.5f), "%s: the send's gain %.2f", name, p.input_gain);
			CHECK(fabs(p.late_gain * room - xbox->late_gain) <= 1.0 + 1e-3 * xbox->late_gain, "%s: the reverb gain %.1f, not %ld",
				name, p.late_gain * room, xbox->late_gain);
		}
		return 0;
	}

	CASE("response")
	{
		for (index = 0; index < ENVIRONMENTS; index++)
		{
			DSI3DL2LISTENER properties = listener_from_tag(&environments[index]);
			struct figures ours = measure(properties);
			const struct figures *xbox = &xbox_figures[index];
			const char *name = environments[index].name;
			int channel;

			for (channel = 0; channel < 4; channel++)
			{
				/* (the DSP glides its gains to an environment's in fixed point,
				and from below stops short by up to 34 of the word's 2^16 when
				its taps' fades end first: its reflections up to 0.4 dB under
				their gain here) */
				CHECK(fabs(ours.reflections_db[channel] - xbox->reflections_db[channel]) < 0.45 ||
					(ours.reflections_db[channel] < -90.0 && xbox->reflections_db[channel] < -90.0),
					"%s: channel %d's reflections %.2f dB, the Xbox's %.2f dB", name, channel,
					ours.reflections_db[channel], xbox->reflections_db[channel]);
				CHECK(fabs(ours.late_db[channel] - xbox->late_db[channel]) < 0.1,
					"%s: channel %d's late reverberation %.2f dB, the Xbox's %.2f dB", name, channel,
					ours.late_db[channel], xbox->late_db[channel]);
				CHECK(ours.first[channel] == xbox->first[channel] && ours.polarity == xbox->polarity,
					"%s: channel %d's reflections from frame %ld (sign %d), the Xbox's %ld (%d)", name, channel,
					ours.first[channel], ours.polarity, xbox->first[channel], xbox->polarity);
			}
			/* (the mountains' late reverberation, 29 dB down, nears the 24-bit
			output's least step in the 20 dB fitted: their decay times are not
			the Xbox's measure of them) */
			if (xbox->late_db[0] > -20.0)
				CHECK(fabs(ours.decay_low - xbox->decay_low) < 0.02 * xbox->decay_low + 0.005 &&
					fabs(ours.decay_high - xbox->decay_high) < 0.02 * xbox->decay_high + 0.005,
					"%s: the decay times %.3f s and %.3f s, the Xbox's %.3f s and %.3f s", name, ours.decay_low,
					ours.decay_high, xbox->decay_low, xbox->decay_high);
		}
		return 0;
	}

	CASE("levels")
	{
		/* the reference: a 3D voice's direct path, centred, has an energy of
		1 over the two channels (the equal power pan); the reverb's four
		channels play on stereo as the Xbox's stereo downmix played the
		speakers' mix bins (its encode processor's: left = front left + 0.707
		center + 0.707 back left), each side its front and its back at -3 dB */
		struct sdl_stream stream;
		float left, right, pair[2], room, direct_lowpass, room_lowpass, output[2 * 2048], *four;
		static float send[2048];
		BOOL hrtf_on;
		struct hrtf_parameters hrtf;
		DSI3DL2LISTENER properties = listener_from_tag(&environments[0]);
		double stereo = 0.0, quad = 0.0;
		unsigned long frame;
		int channel;

		memset(&stream, 0, sizeof(stream));
		stream.has_3d = TRUE;
		stream.channels = 1;
		stream.volume = 1.0f;
		stream.position[2] = 1.0f;
		stream.minimum_distance = 1.0f;
		stream.maximum_distance = 100.0f;
		stream.room = stream.room_hf = DSBVOLUME_MIN;
		listener.position[0] = listener.position[1] = listener.position[2] = 0.0f;
		voice_gains(&stream, &left, &right, pair, &room, &direct_lowpass, &room_lowpass, &hrtf_on, &hrtf);
		CHECK(fabs(left * left + right * right - 1.0) < 1e-4, "the direct path's energy %.3f", left * left + right * right);

		properties.lReflections = DSBVOLUME_MIN;
		four = response(&properties, 2048);
		for (frame = 0; frame < 2048; frame++)
		{
			for (channel = 0; channel < 2; channel++)
			{
				double side = four[frame * 4 + channel] + 0.70710678 * four[frame * 4 + 2 + channel];

				quad += side * side;
			}
		}
		free(four);
		reverb_clear();
		reverb_update(&properties, ++serial);
		memset(output, 0, sizeof(output));
		send[0] = 1.0f;
		reverb_process(send, output, 2048, 1.0f);
		for (frame = 0; frame < 2 * 2048; frame++)
			stereo += (double)output[frame] * output[frame];
		CHECK(fabs(10.0 * log10(stereo / quad)) < 0.1, "the stereo reverb %.2f dB over the four channels' downmix",
			10.0 * log10(stereo / quad));
		return 0;
	}

	/* from the hallway to the mountains as the game moves it, a step a tick
	(33 ms, 1600 frames), through the mix (SetI3DL2Listener's hand-off), a 3D
	voice of noise sending to the reverb all the while */
	CASE("transition")
	{
		struct environment_tag current = environments[5];
		const struct environment_tag *target = &environments[7];
		DSI3DL2LISTENER final = listener_from_tag(target);
		struct sdl_stream *stream = calloc(1, sizeof(*stream));
		float output[64 * 2], start, end, last;
		unsigned long tick, frame = 0, settled = 0, n;

		resampler_initialize();
		resampler_phases_initialize();
		stream->channels = 1;
		stream->sample_rate = OUTPUT_RATE;
		stream->volume = 1.0f;
		stream->has_3d = TRUE;
		stream->mode = DS3DMODE_NORMAL;
		stream->position[2] = 2.0f;
		stream->minimum_distance = 1.0f;
		stream->maximum_distance = 50.0f;
		stream->packets[0].frames = 3 * OUTPUT_RATE;
		stream->packets[0].samples = malloc(stream->packets[0].frames * sizeof(short));
		for (n = 0; n < stream->packets[0].frames; n++)
			stream->packets[0].samples[n] = (short)(((n * 2654435761u) >> 16) & 0x1fff) - 0x1000;

		stream->packet_count = 1;
		resampler_reset(stream);
		streams = stream;
		{
			struct reverb_parameters parameters;

			reverb_parameters_set(&parameters, &final);
			end = parameters.late_gain;
		}
		environment = listener_from_tag(&current);
		environment_serial++;
		mix(output, 64);
		start = last = late_gain_now();
		for (tick = 0; tick < 60; tick++)
		{
			unsigned long chunk;

			/* scenario.c scenario_get_sound_environment's steps a tick */
#define STEP(field, step) current.field += fmaxf(-(step), fminf((step), target->field - current.field))
			STEP(room, 0.03f);
			STEP(room_hf, 0.03f);
			STEP(room_rolloff, 0.3f);
			STEP(decay_time, 0.1f);
			STEP(decay_hf_ratio, 0.03f);
			STEP(reflections, 0.03f);
			STEP(reflections_delay, 0.09f);
			STEP(reverb, 0.03f);
			STEP(reverb_delay, 0.003f);
			STEP(diffusion, 0.03f);
			STEP(density, 0.03f);
			STEP(hf_reference, 600.0f);
#undef STEP
			environment = listener_from_tag(&current);
			environment_serial++;
			/* 1600 frames a tick, in 64-frame mixes, the late gain watched */
			for (chunk = 0; chunk < 25; chunk++)
			{
				float now;

				mix(output, 64);
				frame += 64;
				now = late_gain_now();
				CHECK((now - last) * (end - start) >= -1e-6f,
					"at %.0f ms the late gain went back towards the hallway's: %.4f after %.4f (%.4f to %.4f)",
					frame * 1000.0 / OUTPUT_RATE, now, last, start, end);
				last = now;
				if (!settled && !reverb.fade && !memcmp(&reverb.properties, &final, sizeof(final)))
					settled = frame;
			}
		}
		/* the game is there after 30 ticks (the reverb delay's, 1 s), the
		reverb within two fades after */
		CHECK(!reverb.silent && settled && settled < 30 * 1600 + 2 * REVERB_FADE_FRAMES + 128,
			"the reverb reached the mountains' at %.0f ms", settled * 1000.0 / OUTPUT_RATE);
		return 0;
	}

	/* the hallway reverberating (a 3D voice of noise sending to it), then
	the cave set at once: within 2 ms of the mix taking it, the reverb is
	the cave's, its fade over */
	CASE("change")
	{
		struct sdl_stream *stream = calloc(1, sizeof(*stream));
		DSI3DL2LISTENER cave = listener_from_tag(&environments[2]);
		float output[32 * 2];
		unsigned long chunk, frame = 0, n;

		resampler_initialize();
		resampler_phases_initialize();
		stream->channels = 1;
		stream->sample_rate = OUTPUT_RATE;
		stream->volume = 1.0f;
		stream->has_3d = TRUE;
		stream->mode = DS3DMODE_NORMAL;
		stream->position[2] = 2.0f;
		stream->minimum_distance = 1.0f;
		stream->maximum_distance = 50.0f;
		stream->packets[0].frames = OUTPUT_RATE;
		stream->packets[0].samples = malloc(stream->packets[0].frames * sizeof(short));
		for (n = 0; n < stream->packets[0].frames; n++)
			stream->packets[0].samples[n] = (short)(((n * 2654435761u) >> 16) & 0x1fff) - 0x1000;

		stream->packet_count = 1;
		resampler_reset(stream);
		streams = stream;
		environment = listener_from_tag(&environments[5]);
		environment_serial++;
		for (chunk = 0; chunk < 100; chunk++)
			mix(output, 32);
		CHECK(!reverb.silent && !reverb.fade, "the hallway's reverb not running, or still fading in");
		environment = cave;
		environment_serial++;
		for (chunk = 0; chunk < 4 && (reverb.fade || memcmp(&reverb.properties, &cave, sizeof(cave))); chunk++)
		{
			mix(output, 32);
			frame += 32;
		}
		CHECK(!reverb.fade && !memcmp(&reverb.properties, &cave, sizeof(cave)) && reverb.from.input_gain == -0.5f,
			"%.1f ms after the cave was set, the reverb still fading to it (%d frames to go)",
			frame * 1000.0 / OUTPUT_RATE, (int)reverb.fade);
		return 0;
	}

	/* "delays": the cave set while the hallway reverberates is the cave's
	at once in its gains, but its delays wait REVERB_DELAY_HOLD_FRAMES (the
	test bed's ninth run: a click 2 or 25 ms after the change echoed at the
	hallway's delays, one 100 ms after it at the cave's); a second change
	during the hold does not start it again */
	CASE("delays")
	{
		struct sdl_stream *stream = calloc(1, sizeof(*stream));
		DSI3DL2LISTENER cave = listener_from_tag(&environments[2]), hallway = listener_from_tag(&environments[5]);
		struct reverb_parameters hall_delays, cave_delays;
		float output[32 * 2];
		unsigned long chunk, n;

		reverb_parameters_set(&hall_delays, &hallway);
		reverb_parameters_set(&cave_delays, &cave);
		resampler_initialize();
		resampler_phases_initialize();
		stream->channels = 1;
		stream->sample_rate = OUTPUT_RATE;
		stream->volume = 1.0f;
		stream->has_3d = TRUE;
		stream->mode = DS3DMODE_NORMAL;
		stream->position[2] = 2.0f;
		stream->minimum_distance = 1.0f;
		stream->maximum_distance = 50.0f;
		stream->packets[0].frames = OUTPUT_RATE;
		stream->packets[0].samples = malloc(stream->packets[0].frames * sizeof(short));
		for (n = 0; n < stream->packets[0].frames; n++)
			stream->packets[0].samples[n] = (short)(((n * 2654435761u) >> 16) & 0x1fff) - 0x1000;

		stream->packet_count = 1;
		resampler_reset(stream);
		streams = stream;
		environment = hallway;
		environment_serial++;
		for (chunk = 0; chunk < 300; chunk++)
			mix(output, 32);
		CHECK(!reverb.silent && !reverb.delay_hold && reverb.delays_from.late_delay == hall_delays.late_delay,
			"the hallway's reverb not running at its own delays");
		environment = cave;
		environment_serial++;
		for (chunk = 0; chunk < 75; chunk++)
			mix(output, 32);
		CHECK(!memcmp(&reverb.properties, &cave, sizeof(cave)) && !reverb.fade &&
			reverb.delays_from.late_delay == hall_delays.late_delay &&
			reverb.delays_from.reflections_delay == hall_delays.reflections_delay,
			"50 ms after the cave was set, its gains not in place, or its delays already (%ld, the hallway's %ld)",
			(long)reverb.delays_from.late_delay, (long)hall_delays.late_delay);
		/* (the hallway again, then the cave, within the hold) */
		environment = hallway;
		environment_serial++;
		mix(output, 32);
		mix(output, 32);
		mix(output, 32);
		mix(output, 32);
		environment = cave;
		environment_serial++;
		for (chunk = 0; chunk < 75; chunk++)
			mix(output, 32);
		CHECK(!reverb.delay_hold && !reverb.delay_fade && reverb.delays_from.late_delay == cave_delays.late_delay,
			"%.0f ms after the first change, the cave's delays not in place (%lu frames of the hold left)",
			(75 + 4 + 75) * 32 * 1000.0 / OUTPUT_RATE, (unsigned long)reverb.delay_hold);
		return 0;
	}

	fprintf(stderr, "no case %s\n", case_name);
	return 2;
}
