/* Adapted from OpenH1/OpenH1 6750db5a6c9716857bd1e63d9ec188a20c49f1d3.
 * Copyright OpenH1 contributors; AGPL-3.0-or-later. */
/*
HRTF.C (test)

The HRTF of port/linux/src/dsound_sdl.c (test_hrtf.py takes the mixer as
under_test.inc, as test_mixer.py does) against the Xbox's, two ways. First,
the filters and delays the XDK DirectSound linked into the game's
default.xbe looks up for each direction once the game asks for its full
HRTF (DirectSoundUseFullHRTF, sound_dsound_xbox.c), as the MCPX's voice
processor plays them. The Xbox's figures below (xbox_figures) were measured
on its own filters (see test_hrtf.py on how to measure them again from a
default.xbe); this holds the figures only. Every figure is taken the same
way from the Xbox's responses and from ours, on a grid of directions every
15 degrees round and every 10 from 40 below to 60 above, from impulse
responses of 512 frames at 48 kHz:
	- "delays": the time between the ears (the lag of their greatest
	  cross-correlation below 1.5 kHz, to a sixteenth of a frame), to the
	  side, ahead of it and behind it; the delays in whole frames, as the
	  Xbox's;
	- "levels": the level between the ears in each octave from 125 Hz to 16
	  kHz, over every direction and to the side;
	- "rear": what a sound behind loses above 2.5 kHz, against its low
	  frequencies, beside the direction mirrored ahead (the Xbox on stereo
	  played its table all round, its front mix bins alone).
Second, "recorded": the Xbox's own output, recorded (a sweep 2 m away played
through Halo's DirectSound calls on stereo, its final mix captured, the
same calls through this mixer with no HRTF beside it): each ear's level in
each octave from 125 Hz to 16 kHz against no HRTF's, and the time between
the ears (below 1.5 kHz), every 30 degrees round at 40, 20 and 0 degrees
below and above (recorded_figures), which the mixer's table was fitted to.
Third, "recorded-behind": on 5.1 (its AC-3 decoded), the back pair's at 135
degrees, 40 below and 40 above, which the mixer plays as the direction
mirrored ahead and then brighter on the far ear (hrtf_behind), the same
way, to 16.9 kHz (the AC-3's bandwidth; recorded_behind).
(The notch's figures, the deepest single frequency from 4 to 16 kHz, are
printed but not judged: the table's bands are a third of an octave and
wider, and the recordings' notches ahead are two, at 6.3 and 10 kHz, as
deep as each other.)
"metrics" prints our figures beside the Xbox's; "measure" prints the
figures of responses given (raw floats: for each direction of the grid,
elevation by elevation from below and round from ahead to the right, 512
of the left ear then 512 of the right) in the form of xbox_figures.
*/

#include "mixer_fixture.h"

#define PI 3.14159265358979323846

/* ---------- the grid and the responses */

#define FRAMES 512
#define BINS (FRAMES / 2 + 1)
#define AZIMUTHS 24
#define ELEVATIONS 19
#define DIRECTIONS (AZIMUTHS * ELEVATIONS)
#define OCTAVES 8

static const double octaves[OCTAVES] = { 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };

/* direction index's azimuth (degrees round to the right from ahead) and
elevation */
static int azimuth_of(int direction) { return direction % AZIMUTHS * 15; }
static int elevation_of(int direction) { return direction / AZIMUTHS * 10 - 90; }
/* the directions of the Xbox's figures (xbox_figures): 40 below to 60 above; the grid's others (to the poles)
are judged by recorded_figures only */
#define FIGURE_DIRECTIONS (AZIMUTHS * 11)
static int in_figures(int direction) { return elevation_of(direction) >= -40 && elevation_of(direction) <= 60; }
static int direction_at(int azimuth, int elevation)
{
	return (elevation + 90) / 10 * AZIMUTHS + ((azimuth % 360 + 360) % 360) / 15;
}

/* whether our_response places its sound as 5.1's back pair plays one
behind (hrtf_place's behind) */
static BOOL placing_behind;

/* our response from direction (both ears, FRAMES each), through the HRTF
as the mixer has it (hrtf_place, hrtf_process), 3 units away; its
parameters in placed */
static void our_response(int direction, float *left, float *right, struct hrtf_parameters *placed)
{
	static float impulse[FRAMES], output[FRAMES * 2];
	struct sdl_stream *stream = calloc(1, sizeof(*stream));
	struct hrtf_parameters parameters;
	double azimuth = azimuth_of(direction) * PI / 180.0, elevation = elevation_of(direction) * PI / 180.0;
	unsigned long n;

	hrtf_initialize();
	memset(&parameters, 0, sizeof(parameters));
	hrtf_place((float)(3.0 * sin(azimuth) * cos(elevation)), (float)(3.0 * sin(elevation)),
		(float)(3.0 * cos(azimuth) * cos(elevation)), 3.0f, placing_behind, &parameters);
	parameters.gain = 1.0f;
	if (placed)
		*placed = parameters;
	stream->hrtf_current = parameters;
	memset(impulse, 0, sizeof(impulse));
	memset(output, 0, sizeof(output));
	impulse[0] = 1.0f;
	hrtf_process(stream, impulse, output, FRAMES, FRAMES, &parameters, 1.0f, 0.0f, 1.0f);
	for (n = 0; n < FRAMES; n++)
	{
		left[n] = output[n * 2];
		right[n] = output[n * 2 + 1];
	}
	free(stream);
}

/* ---------- the figures */

struct figures
{
	/* the delay between the ears (ms; the left later positive) level, 30
	below and 30 above, at 30, 60, 90, 120 and 150 degrees to the right */
	double delay[3][5];
	/* the level between the ears in each octave (dB, the right over the
	left): its mean over every direction (of its size), and level at 45, 90
	and 135 degrees to the right */
	double level_mean[OCTAVES];
	double level[3][OCTAVES];
	/* the notch ahead: 30 below, level, 30 above (its frequency, Hz, and
	its depth, dB) */
	double notch_hz[3], notch_db[3];
	/* what is lost above 2.5 kHz behind (dB, against the mirrored front):
	level at 135, 150 and 180 degrees, and 30 above and below at 180 */
	double rear[5];
};

struct spectra
{
	double left[BINS], right[BINS]; /* power, dB */
};

static double bin_hertz(int bin) { return bin * (double)OUTPUT_RATE / FRAMES; }

static void spectrum(const float *x, double *power_db, double *re_out, double *im_out)
{
	int k, n;

	for (k = 0; k < BINS; k++)
	{
		double re = 0.0, im = 0.0;

		for (n = 0; n < FRAMES; n++)
		{
			re += x[n] * cos(2.0 * PI * k * n / FRAMES);
			im -= x[n] * sin(2.0 * PI * k * n / FRAMES);
		}
		power_db[k] = 10.0 * log10(re * re + im * im + 1e-20);
		if (re_out)
		{
			re_out[k] = re;
			im_out[k] = im;
		}
	}
}

/* the mean power (dB) of the octave about center, to top Hz */
static double band_db_to(const double *power_db, double center, double top)
{
	double low = center / sqrt(2.0), high = center * sqrt(2.0), sum = 0.0;
	int k, count = 0;

	if (high > top)
		high = top;
	for (k = 0; k < BINS; k++)
	{
		if (bin_hertz(k) >= low && bin_hertz(k) < high)
		{
			sum += pow(10.0, power_db[k] / 10.0);
			count++;
		}
	}
	return 10.0 * log10(sum / count + 1e-20);
}

static double band_db(const double *power_db, double center)
{
	return band_db_to(power_db, center, 22000.0);
}

/* the lag (ms) of the left ear's greatest cross-correlation with the
right's below 1.5 kHz, to a sixteenth of a frame */
static double ears_delay_ms(const double *lre, const double *lim, const double *rre, const double *rim)
{
	double best = -1e300, lag = 0.0;
	int step;

	for (step = -48 * 16; step <= 48 * 16; step++)
	{
		double tau = step / 16.0, sum = 0.0;
		int k;

		for (k = 0; k < BINS && bin_hertz(k) < 1500.0; k++)
		{
			/* L conj(R) e^(j 2 pi k tau / N) */
			double xr = lre[k] * rre[k] + lim[k] * rim[k], xi = lim[k] * rre[k] - lre[k] * rim[k];
			double c = cos(2.0 * PI * k * tau / FRAMES), s = sin(2.0 * PI * k * tau / FRAMES);

			sum += (k ? 2.0 : 1.0) * (xr * c - xi * s);
		}
		if (sum > best)
		{
			best = sum;
			lag = tau;
		}
	}
	return lag * 1000.0 / OUTPUT_RATE;
}

/* the figures of responses (for each direction, FRAMES of the left ear
then FRAMES of the right) */
static struct figures measure(const float *responses)
{
	static struct spectra s[DIRECTIONS];
	static double mono[DIRECTIONS][BINS], ild[DIRECTIONS][OCTAVES], delay[DIRECTIONS], high[DIRECTIONS];
	static double common[BINS];
	double re[2][BINS], im[2][BINS];
	struct figures f;
	int d, k, b, i;

	memset(&f, 0, sizeof(f));
	memset(common, 0, sizeof(common));
	for (d = 0; d < DIRECTIONS; d++)
	{
		if (!in_figures(d))
			continue;
		spectrum(responses + d * FRAMES * 2, s[d].left, re[0], im[0]);
		spectrum(responses + d * FRAMES * 2 + FRAMES, s[d].right, re[1], im[1]);
		delay[d] = ears_delay_ms(re[0], im[0], re[1], im[1]);
		for (b = 0; b < OCTAVES; b++)
			ild[d][b] = band_db(s[d].left, octaves[b]) - band_db(s[d].right, octaves[b]);
		for (k = 0; k < BINS; k++)
			common[k] += (pow(10.0, s[d].left[k] / 10.0) + pow(10.0, s[d].right[k] / 10.0)) / (2.0 * FIGURE_DIRECTIONS);
	}
	for (d = 0; d < DIRECTIONS; d++)
	{
		double low_sum = 0.0, high_sum = 0.0;
		int lows = 0, highs = 0;

		if (!in_figures(d))
			continue;
		for (k = 0; k < BINS; k++)
		{
			double c = 10.0 * log10(common[k]);

			mono[d][k] = 10.0 * log10((pow(10.0, (s[d].left[k] - c) / 10.0) + pow(10.0, (s[d].right[k] - c) / 10.0)) / 2.0);
			if (bin_hertz(k) >= 100.0 && bin_hertz(k) <= 1000.0)
			{
				low_sum += mono[d][k];
				lows++;
			}
			if (bin_hertz(k) >= 2500.0 && bin_hertz(k) <= 16000.0)
			{
				high_sum += mono[d][k];
				highs++;
			}
		}
		high[d] = high_sum / highs - low_sum / lows;
	}
	for (i = 0; i < 3; i++)
	{
		for (b = 0; b < 5; b++)
			f.delay[i][b] = delay[direction_at(30 + 30 * b, i == 0 ? 0 : (i == 1 ? -30 : 30))];
		for (b = 0; b < OCTAVES; b++)
			f.level[i][b] = -ild[direction_at(45 + 45 * i, 0)][b];
	}
	for (b = 0; b < OCTAVES; b++)
	{
		for (d = 0; d < DIRECTIONS; d++)
			f.level_mean[b] += in_figures(d) ? fabs(ild[d][b]) / FIGURE_DIRECTIONS : 0.0;
	}
	for (i = 0; i < 3; i++)
	{
		int at = direction_at(0, -30 + 30 * i), lowest = -1, count = 0;
		double sum = 0.0;

		for (k = 0; k < BINS; k++)
		{
			double hz = bin_hertz(k);

			if (hz >= 4000.0 && hz <= 16000.0 && (lowest < 0 || mono[at][k] < mono[at][lowest]))
				lowest = k;
			if (hz >= 2000.0 && hz <= 16000.0)
			{
				sum += mono[at][k];
				count++;
			}
		}
		f.notch_hz[i] = bin_hertz(lowest);
		f.notch_db[i] = mono[at][lowest] - sum / count;
	}
	{
		static const int rear_at[5][2] = { { 135, 0 }, { 150, 0 }, { 180, 0 }, { 180, -30 }, { 180, 30 } };

		for (i = 0; i < 5; i++)
		{
			f.rear[i] = high[direction_at(rear_at[i][0], rear_at[i][1])] -
				high[direction_at(180 - rear_at[i][0], rear_at[i][1])];
		}
	}
	return f;
}

static float *our_responses(void)
{
	float *responses = malloc(sizeof(float) * FRAMES * 2 * DIRECTIONS);
	int d;

	for (d = 0; d < DIRECTIONS; d++)
		our_response(d, responses + d * FRAMES * 2, responses + d * FRAMES * 2 + FRAMES, NULL);
	return responses;
}

/* the Xbox's figures, measured on its own filters and delays (the table
DirectSoundUseFullHRTF has DirectSound look up), all round (its table's
own, which the Xbox on stereo played) */
static const struct figures xbox_figures =
{
	{ { 0.275, 0.651, 0.724, 0.643, 0.255 }, { 0.241, 0.574, 0.673, 0.574, 0.229 }, { 0.260, 0.613, 0.684, 0.604, 0.263 } },
	{ 2.1, 2.2, 2.9, 5.3, 6.7, 8.5, 12.9, 14.9 },
	{ { 2.5, 2.9, 4.4, 9.5, 9.6, 14.4, 16.8, 17.4 },
	  { 3.9, 4.0, 4.6, 5.6, 6.2, 14.8, 23.1, 24.7 },
	  { 2.1, 2.6, 4.4, 9.3, 6.8, 5.4, 18.0, 16.5 } },
	{ 6094, 9938, 15094 }, { -5.6, -6.3, -13.8 },
	{ -4.5, -9.7, -12.0, -10.2, -7.5 }
};

/* the Xbox's output recorded from each direction (see "recorded"): the
azimuth and elevation, the time between the ears (ms, the left later
positive), each ear's level in each octave (dB, left, then right) */
static const struct
{
	int azimuth, elevation;
	double delay;
	double left[OCTAVES], right[OCTAVES];
} recorded_figures[] =
{
	/* (the poles and 60 degrees below and above: the test bed's tenth run) */
	{   0, -90, 0.000, { -0.3, -0.2, 0.1, 0.5, -1.7, -12.1, -23.9, -34.6 }, { -0.3, -0.2, 0.1, 0.5, -1.7, -12.1, -23.9, -34.6 } },
	{   0, -60, 0.000, { -0.9, -1.1, -2.0, -2.1, -0.5, -5.9, -5.7, -12.6 }, { -0.9, -1.1, -2.0, -2.1, -0.5, -5.9, -5.7, -12.6 } },
	{  30, -60, 0.066, { -1.2, -1.7, -3.6, -4.7, -1.6, -7.1, -8.8, -18.3 }, { -0.1, -0.2, -0.5, -0.6, -0.7, -4.9, -5.3, -11.2 } },
	{  60, -60, 0.345, { -1.1, -1.4, -2.6, -7.4, -14.4, -13.8, -17.8, -27.6 }, { 0.4, 0.7, 1.4, 2.2, 0.2, -5.4, -5.0, -13.4 } },
	{  90, -60, 0.389, { -1.4, -1.6, -2.1, -3.8, -9.0, -22.4, -20.7, -25.9 }, { 0.7, 1.0, 1.7, 2.8, 1.6, -5.8, -5.3, -10.9 } },
	{ 120, -60, 0.328, { -1.3, -1.6, -2.8, -7.1, -14.3, -16.4, -20.2, -25.0 }, { 0.5, 0.8, 1.3, 2.1, 0.2, -7.9, -7.8, -11.7 } },
	{ 150, -60, 0.087, { -2.1, -2.3, -3.0, -3.3, -4.1, -10.6, -13.3, -18.0 }, { -0.3, -0.3, -0.2, 0.1, -2.1, -7.9, -8.6, -11.7 } },
	{ 180, -60, 0.000, { -1.5, -1.4, -1.4, -1.0, -3.2, -8.9, -9.2, -15.3 }, { -1.5, -1.4, -1.4, -1.0, -3.2, -8.9, -9.2, -15.3 } },
	{   0, +60, 0.000, { -0.1, -0.3, -0.9, -1.2, -1.2, -3.7, -3.6, -6.7 }, { -0.1, -0.3, -0.9, -1.2, -1.2, -3.7, -3.6, -6.7 } },
	{  30, +60, 0.116, { -0.2, -0.5, -1.4, -2.8, -3.0, -4.6, -6.3, -10.6 }, { 0.4, 0.3, -0.1, 0.1, -0.3, -1.8, -1.0, -2.3 } },
	{  60, +60, 0.264, { -0.5, -0.7, -1.8, -4.4, -6.2, -9.6, -9.2, -13.4 }, { 0.7, 0.7, 0.8, 1.2, 0.2, -0.7, 1.5, -0.4 } },
	{  90, +60, 0.307, { -0.7, -1.0, -1.9, -4.4, -7.0, -11.9, -11.4, -18.1 }, { 1.0, 1.0, 1.0, 1.3, 0.3, -1.2, 1.8, -2.1 } },
	{ 120, +60, 0.251, { -0.5, -0.7, -1.7, -4.0, -6.4, -10.2, -7.1, -12.3 }, { 0.7, 0.7, 0.8, 1.1, -0.2, -2.2, 1.0, -4.2 } },
	{ 150, +60, 0.104, { -0.3, -0.5, -1.3, -2.6, -3.6, -6.4, -4.8, -11.2 }, { 0.1, 0.1, -0.1, -0.2, -1.3, -4.2, -1.0, -6.6 } },
	{ 180, +60, 0.000, { -0.1, -0.3, -0.7, -1.2, -2.1, -5.7, -2.6, -8.8 }, { -0.1, -0.3, -0.7, -1.2, -2.1, -5.7, -2.6, -8.8 } },
	{   0, +90, 0.000, { 0.0, -0.2, -0.7, -1.3, -2.2, -5.0, -2.4, -6.5 }, { 0.0, -0.2, -0.7, -1.3, -2.2, -5.0, -2.4, -6.5 } },
	{   0, -40, 0.000, { -0.3, -0.1, 0.8, 2.6, 1.4, -4.1, 0.0, -1.2 }, { -0.3, -0.1, 0.8, 2.6, 1.4, -4.1, 0.0, -1.2 } },
	{  30, -40, 0.186, { -1.2, -1.2, -1.2, -0.7, -2.4, -7.9, -4.0, -9.7 }, { 1.3, 1.5, 2.3, 4.0, 2.9, -1.4, 2.3, 0.1 } },
	{  60, -40, 0.555, { -1.3, -1.5, -2.1, -4.4, -8.0, -17.1, -15.3, -24.4 }, { 2.7, 3.0, 3.6, 4.4, 2.9, 0.2, 2.8, -3.1 } },
	{  90, -40, 0.617, { -1.2, -1.2, -1.5, -2.8, -7.0, -17.9, -19.0, -22.6 }, { 3.0, 3.3, 4.0, 4.6, 2.5, -1.1, 3.0, 1.8 } },
	{ 120, -40, 0.551, { -1.4, -1.6, -2.4, -5.3, -9.2, -15.7, -16.8, -22.5 }, { 2.5, 2.7, 3.2, 4.1, 2.2, -4.2, 2.6, 0.3 } },
	{ 150, -40, 0.181, { -1.7, -1.8, -2.0, -2.0, -3.0, -7.8, -10.6, -18.8 }, { 0.6, 0.8, 1.5, 3.1, 2.0, -6.3, -6.9, -10.5 } },
	{ 180, -40, 0.000, { -0.7, -0.6, -0.1, 1.3, 0.7, -6.1, -9.8, -16.0 }, { -0.7, -0.6, -0.1, 1.3, 0.7, -6.1, -9.8, -16.0 } },
	{   0, -20, 0.000, { 0.1, 0.3, 1.0, 1.7, -0.8, -1.6, -1.2, -0.7 }, { 0.1, 0.3, 1.0, 1.7, -0.8, -1.6, -1.2, -0.7 } },
	{  30, -20, 0.260, { -0.8, -1.0, -1.5, -2.6, -5.1, -8.5, -10.0, -10.1 }, { 1.6, 1.9, 3.0, 4.3, 1.4, 2.2, 1.3, 0.3 } },
	{  60, -20, 0.609, { -0.9, -1.0, -1.5, -3.6, -8.4, -15.5, -14.3, -17.9 }, { 3.1, 3.4, 4.0, 4.6, 2.3, 2.7, 2.6, -0.2 } },
	{  90, -20, 0.697, { -1.1, -1.0, -0.8, -1.6, -8.0, -13.0, -18.5, -20.4 }, { 3.2, 3.5, 4.1, 4.6, 1.9, 0.7, 3.0, 3.2 } },
	{ 120, -20, 0.612, { -1.1, -1.2, -1.7, -4.4, -9.6, -13.3, -16.2, -22.9 }, { 2.5, 2.9, 3.7, 4.4, 1.8, -2.9, 1.9, 0.6 } },
	{ 150, -20, 0.243, { -1.5, -1.7, -2.2, -3.1, -4.5, -8.2, -15.9, -23.2 }, { 0.7, 1.1, 2.2, 3.6, 0.8, -5.8, -6.2, -11.4 } },
	{ 180, -20, 0.000, { -0.7, -0.5, 0.0, 1.1, -0.6, -5.7, -11.9, -19.4 }, { -0.7, -0.5, 0.0, 1.1, -0.6, -5.7, -11.9, -19.4 } },
	{   0,  +0, 0.000, { 1.0, 0.9, 0.6, 0.3, 0.4, -1.2, -3.7, -2.2 }, { 1.0, 0.9, 0.6, 0.3, 0.4, -1.2, -3.7, -2.2 } },
	{  30,  +0, 0.279, { 0.2, -0.1, -1.4, -4.1, -4.7, -8.7, -11.8, -12.1 }, { 2.2, 2.4, 2.9, 3.1, 2.7, 2.5, -0.2, -0.6 } },
	{  60,  +0, 0.655, { -0.3, -0.3, -0.5, -2.3, -10.5, -11.2, -18.4, -21.5 }, { 3.5, 3.9, 4.5, 4.6, 3.1, 3.3, 2.5, -1.2 } },
	{  90,  +0, 0.723, { -0.8, -0.6, -0.1, 0.0, -4.4, -12.1, -16.2, -19.5 }, { 3.7, 4.1, 4.6, 4.6, 2.2, 1.3, 4.4, 2.3 } },
	{ 120,  +0, 0.648, { -0.5, -0.6, -0.8, -2.8, -11.1, -11.9, -18.7, -21.9 }, { 3.0, 3.4, 4.1, 4.2, 1.4, -2.2, 3.3, -1.5 } },
	{ 150,  +0, 0.257, { -0.7, -1.0, -1.9, -3.7, -4.8, -8.2, -15.9, -22.9 }, { 1.3, 1.6, 2.3, 2.8, 0.2, -5.3, -6.2, -14.5 } },
	{ 180,  +0, 0.000, { 0.2, 0.2, 0.3, 0.5, -0.8, -4.9, -11.4, -22.0 }, { 0.2, 0.2, 0.3, 0.5, -0.8, -4.9, -11.4, -22.0 } },
	{   0, +20, 0.000, { 1.4, 1.1, 0.3, -0.1, 0.9, -0.7, -4.3, -6.0 }, { 1.4, 1.1, 0.3, -0.1, 0.9, -0.7, -4.3, -6.0 } },
	{  30, +20, 0.276, { 0.0, -0.3, -1.6, -4.3, -4.6, -7.7, -13.0, -14.9 }, { 3.0, 2.8, 2.1, 2.2, 3.2, 2.8, 0.5, -2.4 } },
	{  60, +20, 0.638, { 0.2, 0.1, -0.4, -2.8, -8.8, -12.6, -18.9, -23.7 }, { 4.0, 4.0, 3.8, 3.4, 3.8, 3.2, 3.3, -2.6 } },
	{  90, +20, 0.703, { -0.2, -0.1, 0.0, -0.6, -4.9, -8.0, -11.0, -20.8 }, { 4.2, 4.2, 3.7, 3.3, 3.5, 0.5, 4.3, -0.2 } },
	{ 120, +20, 0.625, { -0.1, -0.2, -0.7, -2.9, -8.4, -13.0, -17.9, -25.2 }, { 3.8, 3.8, 3.4, 3.1, 3.3, -3.1, 3.7, -2.8 } },
	{ 150, +20, 0.267, { -0.5, -0.8, -1.8, -4.0, -5.1, -8.8, -13.8, -24.3 }, { 2.4, 2.3, 1.8, 1.5, 1.6, -4.1, -6.4, -15.9 } },
	{ 180, +20, 0.000, { 0.9, 0.7, 0.0, -0.7, -0.3, -4.9, -9.0, -21.6 }, { 0.9, 0.7, 0.0, -0.7, -0.3, -4.9, -9.0, -21.6 } },
	{   0, +40, 0.000, { 1.0, 0.8, 0.1, -0.1, 0.4, -1.5, -3.9, -6.6 }, { 1.0, 0.8, 0.1, -0.1, 0.4, -1.5, -3.9, -6.6 } },
	{  30, +40, 0.212, { 0.2, -0.1, -1.3, -3.3, -3.7, -4.7, -8.2, -14.4 }, { 2.3, 2.1, 1.4, 1.9, 2.1, 1.7, 0.4, 0.0 } },
	{  60, +40, 0.560, { 0.3, 0.1, -0.8, -3.7, -6.5, -9.8, -12.4, -18.4 }, { 2.9, 3.0, 3.1, 3.0, 2.7, 2.7, 3.3, -0.8 } },
	{  90, +40, 0.612, { 0.1, 0.0, -0.5, -2.2, -5.4, -10.6, -14.0, -19.5 }, { 3.5, 3.5, 3.1, 2.9, 2.6, 0.8, 3.6, -0.4 } },
	{ 120, +40, 0.556, { 0.0, -0.2, -1.0, -3.4, -6.0, -12.2, -11.8, -20.3 }, { 2.9, 2.9, 2.7, 2.7, 2.0, -1.7, 2.1, -2.5 } },
	{ 150, +40, 0.206, { -0.1, -0.4, -1.5, -3.8, -4.2, -7.6, -9.6, -22.4 }, { 1.6, 1.5, 1.2, 1.4, 0.7, -3.5, -4.5, -16.9 } },
	{ 180, +40, 0.000, { 0.8, 0.6, -0.2, -0.8, -0.7, -5.1, -6.2, -19.1 }, { 0.8, 0.6, -0.2, -0.8, -0.7, -5.1, -6.2, -19.1 } },
};

/* the Xbox's back pair on 5.1 recorded at 135 degrees (see
"recorded-behind"): the elevation, each ear's level in each octave (dB, to
16.9 kHz; left, then right). Its front pair at 45 degrees, the direction
mirrored, was 3.2 and 3.6 dB duller on the far ear at 8 kHz, below and
above: -11.5 and -14.0 dB */
static const struct
{
	int elevation;
	double left[OCTAVES], right[OCTAVES];
} recorded_behind[] =
{
	{ -40, { -1.8, -1.9, -2.5, -3.7, -4.7, -10.9, -8.3, -12.5 }, { 2.3, 2.5, 3.2, 4.4, 3.4, -0.1, 2.1, -5.0 } },
	{ +40, { 0.3, -0.1, -1.6, -5.4, -5.3, -6.3, -10.4, -16.4 }, { 2.8, 2.7, 2.5, 2.7, 2.6, 2.9, 2.4, 0.9 } },
};

static void print_figures(const char *name, const struct figures *f)
{
	int i, b;

	printf("%s\n  delay ms (30 60 90 120 150 degrees):", name);
	for (i = 0; i < 3; i++)
	{
		printf(i ? " |" : "");
		for (b = 0; b < 5; b++)
			printf(" %.3f", f->delay[i][b]);
	}
	printf("\n  level dB (125 Hz to 16 kHz) mean:");
	for (b = 0; b < OCTAVES; b++)
		printf(" %.1f", f->level_mean[b]);
	for (i = 0; i < 3; i++)
	{
		printf("\n    at %d:", 45 + 45 * i);
		for (b = 0; b < OCTAVES; b++)
			printf(" %.1f", f->level[i][b]);
	}
	printf("\n  notch (30 below, level, 30 above):");
	for (i = 0; i < 3; i++)
		printf(" %.0f Hz %.1f dB", f->notch_hz[i], f->notch_db[i]);
	printf("\n  rear dB (135, 150, 180; 180 30 below, above):");
	for (i = 0; i < 5; i++)
		printf(" %.1f", f->rear[i]);
	printf("\n");
}

static float *read_floats(const char *path, unsigned long count)
{
	FILE *file = fopen(path, "rb");
	float *values = malloc(count * sizeof(float));

	if (!file || fread(values, sizeof(float), count, file) != count)
	{
		fprintf(stderr, "could not read %lu floats from %s\n", count, path);
		exit(2);
	}
	fclose(file);
	return values;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	static struct figures ours;
	int i, b;

	if (strcmp(case_name, "measure"))
	{
		float *responses = our_responses();

		ours = measure(responses);
		free(responses);
	}
	CASE("invalid-angles")
	{
		struct hrtf_direction actual, expected;
		float invalid = sqrtf(-1.0f);
		int band;

		hrtf_lookup(0.0f, 0.0f, &expected);
		hrtf_lookup(invalid, invalid, &actual);
		CHECK(actual.delay == expected.delay, "non-finite angles changed the fallback delay");
		for (band = 0; band < HRTF_BANDS; band++)
		{
			CHECK(actual.left[band] == expected.left[band] && actual.right[band] == expected.right[band],
				"non-finite angles escaped the HRTF table at band %d", band);
		}
		return 0;
	}
	CASE("metrics")
	{
		print_figures("ours", &ours);
		print_figures("the Xbox's", &xbox_figures);
		return 0;
	}
	CASE("measure")
	{
		float *responses = read_floats(argv[2], (unsigned long)FRAMES * 2 * DIRECTIONS);
		struct figures f = measure(responses);

		printf("{\n\t{");
		for (i = 0; i < 3; i++)
			printf("%s{ %.3f, %.3f, %.3f, %.3f, %.3f }", i ? ", " : " ", f.delay[i][0], f.delay[i][1], f.delay[i][2],
				f.delay[i][3], f.delay[i][4]);
		printf(" },\n\t{");
		for (b = 0; b < OCTAVES; b++)
			printf("%s%.1f", b ? ", " : " ", f.level_mean[b]);
		printf(" },\n\t{");
		for (i = 0; i < 3; i++)
		{
			printf("%s{", i ? ",\n\t  " : " ");
			for (b = 0; b < OCTAVES; b++)
				printf("%s%.1f", b ? ", " : " ", f.level[i][b]);
			printf(" }");
		}
		printf(" },\n\t{ %.0f, %.0f, %.0f }, { %.1f, %.1f, %.1f },\n\t{ %.1f, %.1f, %.1f, %.1f, %.1f }\n}\n",
			f.notch_hz[0], f.notch_hz[1], f.notch_hz[2], f.notch_db[0], f.notch_db[1], f.notch_db[2], f.rear[0],
			f.rear[1], f.rear[2], f.rear[3], f.rear[4]);
		return 0;
	}

	/* the time between the ears within 0.06 ms of the Xbox's (its filters
	add a little to its whole frames, as our bands add to ours); every delay
	a whole frame, as the Xbox's, and to the side 32 to 34 frames (the
	recordings' 0.72 ms less what the bands add) */
	CASE("delays")
	{
		int d;

		for (i = 0; i < 3; i++)
		{
			for (b = 0; b < 5; b++)
			{
				CHECK(fabs(ours.delay[i][b] - xbox_figures.delay[i][b]) < 0.06, "the delay between the ears at %d "
					"degrees round, %d up: %.3f ms, the Xbox's %.3f", 30 + 30 * b, i == 0 ? 0 : (i == 1 ? -30 : 30),
					ours.delay[i][b], xbox_figures.delay[i][b]);
			}
		}
		for (d = 0; d < DIRECTIONS; d++)
		{
			static float left[FRAMES], right[FRAMES];
			struct hrtf_parameters placed;

			our_response(d, left, right, &placed);
			CHECK(placed.delay[0] == floorf(placed.delay[0]) && placed.delay[1] == floorf(placed.delay[1]),
				"at %d degrees round, %d up, the ears' delays %g and %g frames, not whole", azimuth_of(d),
				elevation_of(d), placed.delay[0], placed.delay[1]);
			if (azimuth_of(d) == 90 && elevation_of(d) == 0)
			{
				CHECK(placed.delay[0] >= 32.0f && placed.delay[0] <= 34.0f && placed.delay[1] == 0.0f,
					"to the right, the left ear %g frames later, not 32 to 34", placed.delay[0]);
			}
		}
		return 0;
	}
	/* the level between the ears: over every direction within 1.5 dB of the
	Xbox's in each octave, and at 45, 90 and 135 degrees within 2 dB to 500
	Hz and 3 dB above */
	CASE("levels")
	{
		for (b = 0; b < OCTAVES; b++)
		{
			CHECK(fabs(ours.level_mean[b] - xbox_figures.level_mean[b]) < 1.5, "the level between the ears at %.0f Hz "
				"over every direction: %.1f dB, the Xbox's %.1f", octaves[b], ours.level_mean[b],
				xbox_figures.level_mean[b]);
			for (i = 0; i < 3; i++)
			{
				CHECK(fabs(ours.level[i][b] - xbox_figures.level[i][b]) < (octaves[b] <= 500 ? 2.0 : 3.0),
					"the level between the ears at %.0f Hz, %d degrees round: %.1f dB, the Xbox's %.1f", octaves[b],
					45 + 45 * i, ours.level[i][b], xbox_figures.level[i][b]);
			}
		}
		return 0;
	}
	/* each ear's level in each octave within 1 dB of the Xbox's recorded to
	2 kHz, 2 dB at 4 and 8 kHz and 3 dB at 16 kHz (the table's bands are
	wider than the recordings' peaks and dips up there), and the time
	between the ears within 0.025 ms, from every direction recorded */
	CASE("recorded")
	{
		int d;

		for (d = 0; d < (int)(sizeof(recorded_figures) / sizeof(recorded_figures[0])); d++)
		{
			static float left[FRAMES], right[FRAMES];
			double re[2][BINS], im[2][BINS], power[2][BINS], delay;
			int azimuth = recorded_figures[d].azimuth, elevation = recorded_figures[d].elevation, ear;

			our_response(direction_at(azimuth, elevation), left, right, NULL);
			spectrum(left, power[0], re[0], im[0]);
			spectrum(right, power[1], re[1], im[1]);
			delay = ears_delay_ms(re[0], im[0], re[1], im[1]);
			CHECK(fabs(delay - recorded_figures[d].delay) < 0.025, "%d degrees round, %d up: the left ear %.3f ms "
				"later, the Xbox's %.3f", azimuth, elevation, delay, recorded_figures[d].delay);
			for (ear = 0; ear < 2; ear++)
			{
				for (b = 0; b < OCTAVES; b++)
				{
					double level = band_db(power[ear], octaves[b]);
					double xbox = ear ? recorded_figures[d].right[b] : recorded_figures[d].left[b];

					CHECK(fabs(level - xbox) < (octaves[b] <= 2000 ? 1.0 : octaves[b] <= 8000 ? 2.0 : 3.0),
						"%d degrees round, %d up: the %s ear "
						"%+.1f dB at %.0f Hz, the Xbox's %+.1f", azimuth, elevation, ear ? "right" : "left", level,
						octaves[b], xbox);
				}
			}
		}
		return 0;
	}
	/* on 5.1, the back pair at 135 degrees, 40 below and above: each ear's
	level in each octave as recorded's tolerances */
	CASE("recorded-behind")
	{
		int d;

		placing_behind = TRUE;
		for (d = 0; d < (int)(sizeof(recorded_behind) / sizeof(recorded_behind[0])); d++)
		{
			static float left[FRAMES], right[FRAMES];
			double re[2][BINS], im[2][BINS], power[2][BINS];
			int elevation = recorded_behind[d].elevation, ear;

			/* (behind, as the direction mirrored ahead) */
			our_response(direction_at(45, elevation), left, right, NULL);
			spectrum(left, power[0], re[0], im[0]);
			spectrum(right, power[1], re[1], im[1]);
			for (ear = 0; ear < 2; ear++)
			{
				for (b = 0; b < OCTAVES; b++)
				{
					double level = band_db_to(power[ear], octaves[b], 16900.0);
					double xbox = ear ? recorded_behind[d].right[b] : recorded_behind[d].left[b];

					CHECK(fabs(level - xbox) < (octaves[b] <= 2000 ? 1.0 : octaves[b] <= 8000 ? 2.0 : 3.0),
						"5.1's back pair, 135 degrees round, %d up: the %s ear %+.1f dB at %.0f Hz, the Xbox's %+.1f",
						elevation, ear ? "right" : "left", level, octaves[b], xbox);
				}
			}
		}
		placing_behind = FALSE;
		return 0;
	}
	/* behind, the high frequencies within 3 dB of the Xbox's loss */
	CASE("rear")
	{
		static const char *at[5] = { "135 degrees", "150 degrees", "behind", "behind, 30 below", "behind, 30 above" };

		for (i = 0; i < 5; i++)
		{
			CHECK(fabs(ours.rear[i] - xbox_figures.rear[i]) < 3.0, "%s, above 2.5 kHz %+.1f dB of the front's, the "
				"Xbox's %+.1f", at[i], ours.rear[i], xbox_figures.rear[i]);
		}
		return 0;
	}
	fprintf(stderr, "no case %s\n", case_name);
	return 2;
}
