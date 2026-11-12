/* OpenH1 6750db5a6c9716857bd1e63d9ec188a20c49f1d3; AGPL-3.0-or-later.
 * Copyright OpenH1 contributors; measured DSP expectations retained. */
/* Xbox mixer integration tests: measured resampling, Doppler, levels,
 * obstruction, room sends, HRTF routing, surround and callback-sized ramps.
 * The existing mixer.c owns the sinc reference and render diagnostics.
 * "circle" renders a directional listening sample; "bench" times the
 * Xbox path with optional HRTF and surround output. */

#define UNDER_TEST_I3DL2_SOURCE
#include "mixer_fixture.h"

#define PI 3.14159265358979323846
#define CHUNK 512 /* frames a mix, as SDL asks on Linux */

/* ---------- voices */

/* A fixture voice owns one already-decoded packet. */
static void voice_packet_taken(struct sdl_stream *stream) { stream->packet_count = 1; }

/* a voice of frames frames of channels channels at rate, played at frequency
(0: its rate), a 2D voice heard at full volume in both speakers */
static struct sdl_stream *voice_new(const short *samples, unsigned long frames, unsigned long channels,
	unsigned long rate, unsigned long frequency)
{
	struct sdl_stream *stream = calloc(1, sizeof(*stream));

	stream->cut_played = CUT_FADE_FRAMES;
	stream->channels = channels;
	stream->sample_rate = rate;
	stream->frequency = frequency;
	stream->volume = 1.0f;
	stream->mix_left = stream->mix_right = 1.0f;
	stream->room = stream->room_hf = DSBVOLUME_MIN;
	stream->packets[0].samples = malloc(frames * channels * sizeof(short));
	memcpy(stream->packets[0].samples, samples, frames * channels * sizeof(short));
	stream->packets[0].frames = frames;
	voice_packet_taken(stream);
	resampler_reset(stream);
	return stream;
}

#include "mixer_signals.h"

/* ---------- a scene for the Xbox-path benchmark */

/* voices: mono 22 kHz sounds in the world (indoors sending to the reverb,
half muffled, silent ones out of earshot), stereo 44 kHz music, a few
pitched; each plays for seconds */
static void scene(unsigned long voices, unsigned long silent, BOOL indoor, double seconds)
{
	unsigned long frames = 22050 * 4, v, n;
	short *mono = malloc(frames * sizeof(short)), *stereo = malloc(frames * 2 * sizeof(short));

	random_state = 7;
	for (n = 0; n < frames; n++)
	{
		mono[n] = (short)(8000 * random_uniform());
		stereo[n * 2] = (short)(8000 * random_uniform());
		stereo[n * 2 + 1] = (short)(8000 * random_uniform());
	}
	if (indoor)
	{
		DSI3DL2LISTENER room = { -1000, -100, 0.0f, 1.49f, 0.83f, -2602, 0.007f, 200, 0.011f, 100.0f, 100.0f, 5000.0f };

		environment = room;
		environment_serial++;
	}
	resampler_initialize();
	resampler_phases_initialize();
	reverb_initialize();
	for (v = 0; v < voices + silent; v++)
	{
		struct sdl_stream *stream;

		if (v == 0)
			stream = voice_new(stereo, frames, 2, 44100, 0);
		else
			stream = voice_new(mono, frames, 1, 22050, v % 5 == 0 ? 22050 + 2000 * (v % 3) : 0);
		if (v)
		{
			stream->has_3d = TRUE;
			stream->mode = DS3DMODE_NORMAL;
			stream->position[0] = (float)(v % 7) - 3.0f;
			stream->position[2] = v < voices ? 2.0f : 1000.0f;
			stream->minimum_distance = 1.0f;
			stream->maximum_distance = v < voices ? 50.0f : 100.0f;
			stream->direct = v < voices ? 0 : DSBVOLUME_MIN;
			stream->direct_hf = v % 2 ? -500 : 0;
			stream->room = v < voices ? 0 : DSBVOLUME_MIN;
			stream->room_hf = 0;
		}
		/* a packet as long as the scene */
		free(stream->packets[0].samples);
		stream->packets[0].frames = (unsigned long)(seconds * 50000);
		stream->packets[0].samples = malloc(stream->packets[0].frames * stream->channels * sizeof(short));
		for (n = 0; n < stream->packets[0].frames * stream->channels; n++)
			stream->packets[0].samples[n] = (v == 0 ? stereo : mono)[n % frames];
		voice_packet_taken(stream);
		stream->next = streams;
		streams = stream;
	}
	free(mono);
	free(stereo);
}

static void bench(unsigned long voices, unsigned long silent, BOOL indoor, double seconds)
{
	unsigned long chunks = (unsigned long)(seconds * OUTPUT_RATE / CHUNK), chunk;
	static float output[CHUNK * MAXIMUM_OUTPUT_CHANNELS];
	struct timespec start, end;
	double elapsed;

	scene(voices, silent, indoor, seconds);
	clock_gettime(CLOCK_MONOTONIC, &start);
	for (chunk = 0; chunk < chunks; chunk++)
		mix(output, CHUNK);
	clock_gettime(CLOCK_MONOTONIC, &end);
	elapsed = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) * 1e-9;
	printf("%lu voices (%lu out of earshot)%s: %.2f%% of a core, %.1f ns a frame\n", voices, silent,
		indoor ? " indoors" : "", 100.0 * elapsed / seconds, elapsed * 1e9 / ((double)chunks * CHUNK));
}

/* ---------- Xbox ADPCM */

/* two blocks (bytes (73 i + 41) mod 256 under their headers) and their frames
as a decoder written apart from the port's (tools: an IMA ADPCM decoder in
Python from the format, which ffmpeg's adpcm_ima_wav matches in the framing,
nibble order and interleave) decodes them: a stereo block (headers 1000 at
step 30, -20000 at step 60), and a mono one (32000 at step 80) that clips */
static const unsigned char adpcm_stereo_block[72] = {
	0xe8, 0x03, 0x1e, 0x00, 0xe0, 0xb1, 0x3c, 0x00, 0x71, 0xba, 0x03, 0x4c, 0x95, 0xde, 0x27, 0x70, 0xb9, 0x02, 0x4b,
	0x94, 0xdd, 0x26, 0x6f, 0xb8, 0x01, 0x4a, 0x93, 0xdc, 0x25, 0x6e, 0xb7, 0x00, 0x49, 0x92, 0xdb, 0x24, 0x6d, 0xb6,
	0xff, 0x48, 0x91, 0xda, 0x23, 0x6c, 0xb5, 0xfe, 0x47, 0x90, 0xd9, 0x22, 0x6b, 0xb4, 0xfd, 0x46, 0x8f, 0xd8, 0x21,
	0x6a, 0xb3, 0xfc, 0x45, 0x8e, 0xd7, 0x20, 0x69, 0xb2, 0xfb, 0x44, 0x8d, 0xd6, 0x1f, 0x68,
};
static const short adpcm_stereo_frames[2][64] = {
	{ 1000, 1048, 1268, 1111, 911, 1093, 1116, 922, 1157, 1063, 863, 993, 1016, 866, 1042, 1255, 1170, 1248, 1271,
	  1164, 1340, 1505, 1441, 1265, 1005, 902, 1186, 1377, 1274, 1054, 739, 1118, 1373, 1511, 1385, 1194, 812, 1169,
	  1400, 1021, 1684, 1413, 509, 1110, 1657, 961, 2137, 3579, 2221, 2749, 3550, 2822, 4544, 6186, 4694, 2948, -572,
	  -2081, 3866, 7918, 2762, -1925, -11056, 691 },
	{ -20000, -16876, -18122, -23036, -30402, -15694, -5183, -3272, 22787, -18179, -32768, 20477, 32767, -23096,
	  30149, 26054, -15, 32767, 32767, -15648, 32767, 32767, 4098, 7822, 11207, -22648, 30597, 32767, 4098, -32768,
	  -32768, -32768, 750, 32767, 4098, -32768, -32768, 28668, 32767, 32767, 21595, -15647, -32768, 20477, 32767,
	  -28669, -32764, -32768, -32768, 12285, 32767, -20478, -24573, 31290, -13763, -9668, 8953, -28289, -32384, 16031,
	  -29022, -32768, -20482, -24206 },
};
static const short adpcm_mono_frames[64] = {
	32000, 10978, 32767, 32767, 20481, -32768, -32768, -32768, -14147, -3991, 32767, 12289, -13780, 9919, 12996,
	-12187, 18284, 32767, 20481, -27934, -32768, 28668, 32767, 32767, 32767, 20481, -5588, 11340, 14417, -5169, 17724,
	32767, 21595, -15647, -32768, 20477, 32767, -23096, 30149, 26054, -15, 10141, 13218, -772, 22121, 32767, 24373,
	1480, -32375, 12678, 32767, -15648, 32767, 32767, 4098, 7822, 11207, 1975, 27158, 32767, 23535, 3949, -24031, 9487,
};

/* a block decoded a frame at a time from the format, apart from the port's
loops: frame 0 of a channel is its header's sample (the header's third byte
its step index, held to 88); frame n its nibble n - 1, the nibbles in 4-byte
words that take turns between the channels, eight a word, low nibble first
(the 64th pads the block); IMA ADPCM's step and its index's moves */
static void adpcm_expected(const unsigned char *block, unsigned long channels, short *frames)
{
	static const int steps[89] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
		107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
		876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428,
		4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385,
		24623, 27086, 29794, 32767,
	};
	unsigned long channel, n;

	for (channel = 0; channel < channels; channel++)
	{
		long predictor = (short)(block[channel * 4] | (block[channel * 4 + 1] << 8));
		int index = block[channel * 4 + 2] > 88 ? 88 : block[channel * 4 + 2];

		frames[channel] = (short)predictor;
		for (n = 1; n < 64; n++)
		{
			unsigned long nibble_index = n - 1, word = nibble_index / 8, within = nibble_index % 8;
			unsigned char byte = block[4 * channels + (word * channels + channel) * 4 + within / 2];
			int nibble = within & 1 ? byte >> 4 : byte & 15, magnitude = nibble & 7, bit;
			long difference = steps[index] >> 3;

			for (bit = 2; bit >= 0; bit--)
			{
				if (magnitude & (1 << bit))
					difference += steps[index] >> (2 - bit);
			}
			predictor += nibble & 8 ? -difference : difference;
			predictor = predictor > 32767 ? 32767 : predictor < -32768 ? -32768 : predictor;
			index += magnitude < 4 ? -1 : 2 * (magnitude - 3);
			index = index < 0 ? 0 : index > 88 ? 88 : index;
			frames[n * channels + channel] = (short)predictor;
		}
	}
}

/* ---------- Doppler */

/* a 1 kHz sine at 22 kHz from a 3D voice ten units ahead of the listener (at
the origin, facing +z, a unit 3.048 metres as the game sets it), moving along z
at velocity units a second (towards the listener: negative), the listener at
listener_velocity, in mode; the shift the mixer works out, and the power of
what it plays besides a sine at expected times 1 kHz, against the sine (dB) */
static void doppler_figures(float velocity, float listener_velocity, DWORD mode, double expected, float *ratio,
	double *rest_db)
{
	unsigned long frames = 22050, outputs = 30000, played = (unsigned long)(OUTPUT_RATE / expected), n;
	short *samples = malloc(frames * sizeof(short));
	double *signal = malloc(frames * sizeof(double)), *exact = malloc(frames * sizeof(double)), amplitude;
	float *output = calloc(outputs, sizeof(float));
	struct sdl_stream *stream;

	for (n = 0; n < frames; n++)
		signal[n] = 0.5 * sin(2.0 * PI * 1000.0 * n / 22050.0);
	quantize(signal, samples, exact, frames);
	stream = voice_new(samples, frames, 1, 22050, 0);
	stream->has_3d = TRUE;
	stream->mode = mode;
	stream->position[2] = 10.0f;
	stream->velocity[2] = velocity;
	stream->minimum_distance = 1.0f;
	stream->maximum_distance = 1000.0f;
	listener.distance_factor = 3.048f;
	listener.velocity[2] = listener_velocity;
	*ratio = doppler_ratio(stream);
	render(stream, output, outputs, FALSE);
	/* (the frames it plays for, short of where it starts and ends) */
	tone_fit(output, 2000, (played < outputs ? played : outputs) - 2000, 1000.0 * expected / OUTPUT_RATE, &amplitude,
		rest_db);
	listener.velocity[2] = 0.0f;
	voice_delete(stream);
	free(output);
	free(exact);
	free(signal);
	free(samples);
}

/* the amplitude of frequency hz in frames of x, Blackman windowed */
static double window_bin(const float *x, unsigned long frames, double hz)
{
	double re = 0.0, im = 0.0, sum = 0.0;
	unsigned long n;

	for (n = 0; n < frames; n++)
	{
		double w = 0.42 - 0.5 * cos(2.0 * 3.14159265358979 * n / (frames - 1)) +
			0.08 * cos(4.0 * 3.14159265358979 * n / (frames - 1));
		double angle = 2.0 * 3.14159265358979 * hz * n / OUTPUT_RATE;

		re += w * x[n] * cos(angle);
		im -= w * x[n] * sin(angle);
		sum += w;
	}
	return 2.0 * sqrt(re * re + im * im) / sum;
}

/* the amplitude of frequency hz in frames of x, its last 96 faded out (a
click's: analyses.py's kernel_spectrum) */
static double segment_bin(const float *x, unsigned long frames, double hz)
{
	double re = 0.0, im = 0.0;
	unsigned long n;

	for (n = 0; n < frames; n++)
	{
		double w = n + 96 < frames ? 1.0 : 0.5 + 0.5 * cos(3.14159265358979 * (n + 96 - frames) / 96.0);
		double angle = 2.0 * 3.14159265358979 * hz * n / OUTPUT_RATE;

		re += w * x[n] * cos(angle);
		im -= w * x[n] * sin(angle);
	}
	return sqrt(re * re + im * im);
}

/* ---------- 3D audio: the HRTF and the pan */

/* a mono 3D voice of frames frames at rate, right, up and ahead of the
listener (at the origin, facing +z, +y up: +x is its right), its minimum
distance a unit, not sending to the reverb */
static struct sdl_stream *voice_placed(const short *samples, unsigned long frames, unsigned long rate, float right,
	float up, float ahead)
{
	struct sdl_stream *stream = voice_new(samples, frames, 1, rate, 0);

	stream->has_3d = TRUE;
	stream->mode = DS3DMODE_NORMAL;
	stream->position[0] = right;
	stream->position[1] = up;
	stream->position[2] = ahead;
	stream->minimum_distance = 1.0f;
	stream->maximum_distance = 100.0f;
	return stream;
}

/* the listener as voice_placed has it */
static void listener_reset(void)
{
	memset(&listener, 0, sizeof(listener));
	listener.front[2] = 1.0f;
	listener.top[1] = 1.0f;
	listener.rolloff_factor = 1.0f;
	listener.distance_factor = 1.0f;
	listener.doppler_factor = 1.0f;
}

/* a second of noise at 48 kHz, as 16-bit samples */
static short *noise_samples(unsigned long frames)
{
	double *signal = noise(frames), *exact = malloc(frames * sizeof(double));
	short *samples = malloc(frames * sizeof(short));

	quantize(signal, samples, exact, frames);
	free(signal);
	free(exact);
	return samples;
}

/* frames of a mono 3D voice of noise at (right, up, ahead), through the
HRTF or panned: both ears, interleaved, after the first mix (its ramps) */
static float *placed_noise(BOOL through_hrtf, float right, float up, float ahead, unsigned long frames)
{
	short *samples = noise_samples(frames + 4096);
	struct sdl_stream *stream = voice_placed(samples, frames + 4096, 48000, right, up, ahead);
	float *output = malloc((frames + CHUNK) * 2 * sizeof(float));

	hrtf_enabled = through_hrtf;
	render(stream, output, frames + CHUNK, TRUE);
	memmove(output, output + CHUNK * 2, frames * 2 * sizeof(float));
	hrtf_enabled = FALSE;
	voice_delete(stream);
	free(samples);
	return output;
}

/* the level between the ears above a few kHz (the first difference's
power, left over right, dB) */
static double ears_level(const float *output, unsigned long frames)
{
	double left = 0.0, right = 0.0;
	unsigned long n;

	for (n = 1; n < frames; n++)
	{
		double l = output[n * 2] - output[(n - 1) * 2], r = output[n * 2 + 1] - output[(n - 1) * 2 + 1];

		left += l * l;
		right += r * r;
	}
	return 10.0 * log10((left + 1e-30) / (right + 1e-30));
}

/* how much later the right ear hears what the left does (frames, -48 to
48: the lag of the ears' greatest cross-correlation) */
static long ears_delay(const float *output, unsigned long frames)
{
	long lag, best = 0;
	double best_sum = -1e30;

	for (lag = -48; lag <= 48; lag++)
	{
		double sum = 0.0;
		unsigned long n;

		for (n = 64; n + 64 < frames; n++)
			sum += (double)output[n * 2] * output[(n + lag) * 2 + 1];
		if (sum > best_sum)
		{
			best_sum = sum;
			best = lag;
		}
	}
	return best;
}

/* an ear's (0 left, 1 right) power around hertz: a 1024-frame Hann
window's spectrum at the five bins nearest it, over every window, in dB */
static double ear_band(const float *output, unsigned long frames, int ear, double hertz)
{
	unsigned long first, n;
	long bin, center = lround(hertz * 1024.0 / OUTPUT_RATE);
	double power = 0.0;

	for (first = 0; first + 1024 <= frames; first += 512)
	{
		for (bin = center - 2; bin <= center + 2; bin++)
		{
			double re = 0.0, im = 0.0;

			for (n = 0; n < 1024; n++)
			{
				double window = 0.5 - 0.5 * cos(2.0 * PI * n / 1024.0);
				double x = output[(first + n) * 2 + ear] * window;

				re += x * cos(2.0 * PI * bin * n / 1024.0);
				im -= x * sin(2.0 * PI * bin * n / 1024.0);
			}
			power += re * re + im * im;
		}
	}
	return 10.0 * log10(power + 1e-30);
}

/* both ears' power, dB */
static double ears_power(const float *output, unsigned long frames)
{
	double power = 0.0;
	unsigned long n;

	for (n = 0; n < frames * 2; n++)
		power += (double)output[n] * output[n];
	return 10.0 * log10(power + 1e-30);
}

/* the octave bands of the figures the Xbox's were measured in (63 Hz to 16
kHz) */
#define OCTAVES 9
static const double octave_hertz[OCTAVES] = { 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };

/* the power (dB) in each octave band (octave_hertz: from f / sqrt 2 to f
sqrt 2) of frames of a channel of signal (frames of stride floats), from
its spectrum over 4096-frame Hann windows */
static void octave_levels(const float *signal, unsigned long frames, unsigned long stride, unsigned long channel,
	double *levels)
{
	enum { SIZE = 4096 };
	static double re[SIZE], im[SIZE];
	double power[OCTAVES] = { 0 };
	unsigned long first, n, k, size, band;

	for (first = 0; first + SIZE <= frames; first += SIZE / 2)
	{
		for (n = 0; n < SIZE; n++)
		{
			unsigned long reversed = 0, bits = n;

			for (k = 1; k < SIZE; k <<= 1, bits >>= 1)
				reversed = (reversed << 1) | (bits & 1);
			re[reversed] = signal[(first + n) * stride + channel] * (0.5 - 0.5 * cos(2.0 * PI * n / SIZE));
			im[reversed] = 0.0;
		}
		for (size = 2; size <= SIZE; size <<= 1)
		{
			for (n = 0; n < SIZE; n += size)
			{
				for (k = 0; k < size / 2; k++)
				{
					double angle = -2.0 * PI * k / size, wr = cos(angle), wi = sin(angle);
					unsigned long a = n + k, b = n + k + size / 2;
					double tr = re[b] * wr - im[b] * wi, ti = re[b] * wi + im[b] * wr;

					re[b] = re[a] - tr;
					im[b] = im[a] - ti;
					re[a] += tr;
					im[a] += ti;
				}
			}
		}
		for (k = 1; k < SIZE / 2; k++)
		{
			double hertz = (double)k * OUTPUT_RATE / SIZE;

			for (band = 0; band < OCTAVES; band++)
			{
				if (hertz >= octave_hertz[band] / sqrt(2.0) && hertz < octave_hertz[band] * sqrt(2.0))
					power[band] += re[k] * re[k] + im[k] * im[k];
			}
		}
	}
	for (band = 0; band < OCTAVES; band++)
		levels[band] = 10.0 * log10(power[band] + 1e-30);
}

/* frames of a voice mixed alone on 5.1, CHUNK at a time, after its first mix
(its ramps): each speaker's energy (dB; -400 silent), and the frames
themselves in output (frames of 6; NULL: not wanted) */
static void render_surround(struct sdl_stream *stream, unsigned long frames, double *energy_db, float *output)
{
	static float buffer[CHUNK * MAXIMUM_OUTPUT_CHANNELS], send[CHUNK];
	double energy[MAXIMUM_OUTPUT_CHANNELS] = { 0 };
	unsigned long done, frame, speaker, saved = output_channels;

	output_channels = 6;
	for (done = 0; done < frames + CHUNK; done += CHUNK)
	{
		memset(buffer, 0, sizeof(buffer));
		memset(send, 0, sizeof(send));
		mix_voice(stream, buffer, send, CHUNK);
		if (!done)
			continue;
		for (frame = 0; frame < CHUNK && done - CHUNK + frame < frames; frame++)
		{
			for (speaker = 0; speaker < MAXIMUM_OUTPUT_CHANNELS; speaker++)
			{
				float x = buffer[frame * MAXIMUM_OUTPUT_CHANNELS + speaker];

				energy[speaker] += (double)x * x;
				if (output)
					output[(done - CHUNK + frame) * MAXIMUM_OUTPUT_CHANNELS + speaker] = x;
			}
		}
	}
	output_channels = saved;
	for (speaker = 0; speaker < MAXIMUM_OUTPUT_CHANNELS; speaker++)
		energy_db[speaker] = energy[speaker] > 0.0 ? 10.0 * log10(energy[speaker]) : -400.0;
}

/* frames of a voice mixed alone on stereo, after its first mix: its left
channel, and its room send, as frames of mono */
static void render_with_send(struct sdl_stream *stream, unsigned long frames, float *left, float *room)
{
	static float buffer[CHUNK * 2], send[CHUNK];
	unsigned long done, frame;

	for (done = 0; done < frames + CHUNK; done += CHUNK)
	{
		memset(buffer, 0, sizeof(buffer));
		memset(send, 0, sizeof(send));
		mix_voice(stream, buffer, send, CHUNK);
		for (frame = 0; done && frame < CHUNK && done - CHUNK + frame < frames; frame++)
		{
			left[done - CHUNK + frame] = buffer[frame * 2];
			room[done - CHUNK + frame] = send[frame];
		}
	}
}

/* "circle": a sound going round the listener, ear high, 8 seconds from
ahead to the right, behind, the left and ahead again, then 4 over the
head, ahead to above, behind and below, through the HRTF or panned, to a
16-bit WAV file: bursts of noise, four a second, which ears place best */
static int circle(const char *path, BOOL through_hrtf)
{
	unsigned long frames = OUTPUT_RATE * 12, n, done;
	short *samples = malloc((frames + OUTPUT_RATE) * sizeof(short));
	float *output = calloc(frames * 2, sizeof(float));
	struct sdl_stream *stream;
	FILE *file;
	unsigned char header[44];
	unsigned long bytes = frames * 4;

	random_state = 3;
	for (n = 0; n < frames + OUTPUT_RATE; n++)
	{
		double t = (double)(n % (OUTPUT_RATE / 4)) / OUTPUT_RATE;
		double envelope = t < 0.002 ? t / 0.002 : t < 0.12 ? exp(-(t - 0.002) * 18.0) : 0.0;

		samples[n] = (short)(26000.0 * envelope * random_uniform());
	}
	hrtf_initialize();
	listener_reset();
	stream = voice_placed(samples, frames + OUTPUT_RATE, OUTPUT_RATE, 0.0f, 0.0f, 1.5f);
	hrtf_enabled = through_hrtf;
	for (done = 0; done < frames; done += CHUNK)
	{
		static float buffer[CHUNK * 2], send[CHUNK];
		double t = (double)done / OUTPUT_RATE, angle;
		unsigned long count = frames - done < CHUNK ? frames - done : CHUNK, frame;

		if (t < 8.0)
		{
			angle = 2.0 * PI * t / 8.0;
			stream->position[0] = (float)(1.5 * sin(angle));
			stream->position[1] = 0.0f;
			stream->position[2] = (float)(1.5 * cos(angle));
		}
		else
		{
			angle = 2.0 * PI * (t - 8.0) / 4.0;
			stream->position[0] = 0.0f;
			stream->position[1] = (float)(1.5 * sin(angle));
			stream->position[2] = (float)(1.5 * cos(angle));
		}
		memset(buffer, 0, sizeof(buffer));
		memset(send, 0, sizeof(send));
		mix_voice(stream, buffer, send, count);
		for (frame = 0; frame < count * 2; frame++)
			output[done * 2 + frame] = buffer[frame];
	}
	hrtf_enabled = FALSE;
	file = fopen(path, "wb");
	if (!file)
		return 2;
	memcpy(header, "RIFF\0\0\0\0WAVEfmt \x10\0\0\0\x01\0\x02\0\x80\xbb\0\0\0\xee\x02\0\x04\0\x10\0data\0\0\0\0", 44);
	for (n = 0; n < 4; n++)
	{
		header[4 + n] = (unsigned char)((bytes + 36) >> (8 * n));
		header[40 + n] = (unsigned char)(bytes >> (8 * n));
	}
	fwrite(header, 1, 44, file);
	for (n = 0; n < frames * 2; n++)
	{
		long value = lround(output[n] * 32767.0);
		unsigned char pair[2];

		value = value > 32767 ? 32767 : value < -32768 ? -32768 : value;
		pair[0] = (unsigned char)(value & 0xff);
		pair[1] = (unsigned char)((value >> 8) & 0xff);
		fwrite(pair, 1, 2, file);
	}
	fclose(file);
	voice_delete(stream);
	free(samples);
	free(output);
	return 0;
}

/* ---------- the mix's frames a call (block-size)

The same voice changed at the same frames, mixed in calls of different
sizes (an audio device asks for its own, SDL's resampling for any), each call
split where a change falls, so that it reaches the mix at the same frame
whatever the calls: the ramps' times alone can differ */

#define BLOCK_FRAMES 19200
#define BLOCK_CHANGES 5
#define BLOCK_SCENES 10
#define BLOCK_PARTITIONS 9

static const char *const block_scene_names[BLOCK_SCENES] = {
	"volume", "pitch", "direct low pass", "room send", "position, panned", "position, HRTF",
	"position, 5.1", "position, 5.1 HRTF", "Doppler", "HRTF choice",
};
/* (the second change 20 frames after the first, while it is ramping: a new
one taken from where the ramp was) */
static const unsigned long block_change_frames[BLOCK_CHANGES] = { 4800, 4820, 9600, 14400, 16000 };
/* the calls' frames, round and round: 64, 256, 480 (the twin's), 1024 and
4096, irregular ones, 16 and 1 */
static const unsigned long block_partitions[BLOCK_PARTITIONS][10] = {
	{ 64 }, { 256 }, { 480 }, { 1024 }, { 4096 }, { 17, 333, 5, 1024, 61, 2000, 1, 700, 128, 3000 },
	{ 37, 3, 90, 41, 7 }, { 16 }, { 1 },
};
static const char *const block_partition_names[BLOCK_PARTITIONS] = {
	"64", "256", "480", "1024", "4096", "irregular", "irregular small", "16", "1",
};

/* scene's change'th change, to stream (or the HRTF's choice) */
static void block_change(int scene, struct sdl_stream *stream, int change)
{
	static const float volumes[BLOCK_CHANGES] = { 0.25f, 0.6f, 1.0f, 0.0f, 1.0f };
	static const DWORD frequencies[BLOCK_CHANGES] = { 33075, 16000, 22050, 44100, 22050 };
	static const LONG direct_hf[BLOCK_CHANGES] = { -2000, -500, 0, -4000, 0 };
	static const LONG room[BLOCK_CHANGES][2] = { { -1000, -2000 }, { 0, -500 }, { -3000, -3000 }, { 0, 0 },
		{ -600, -2500 } };
	static const float positions[BLOCK_CHANGES][3] = { { 3, 0, 0.2f }, { -3, 0, 1 }, { 0.5f, 0, -3 }, { 0, 0, 3 },
		{ 2, 1, 2 } };
	static const float doppler[BLOCK_CHANGES][2] = { { -10, 40 }, { 10, 40 }, { 10, -40 }, { 3, 0 }, { -6, 20 } };

	switch (scene)
	{
	case 0: stream->volume = volumes[change]; break;
	case 1: stream->frequency = frequencies[change]; break;
	case 2: stream->direct_hf = direct_hf[change]; break;
	case 3: stream->room = room[change][0]; stream->room_hf = room[change][1]; break;
	case 8:
		stream->position[0] = doppler[change][0];
		stream->velocity[0] = doppler[change][1];
		break;
	case 9: hrtf_enabled = change == 0 || change == 3; break;
	default: memcpy(stream->position, positions[change], sizeof(stream->position)); break;
	}
}

/* scene mixed in partition's calls: each output channel's frames (frames
of 6 on 5.1, of 2 otherwise) and the room send; the output's channels */
static unsigned long block_render(int scene, int partition, float *output, float *room)
{
	static float buffer[4096 * MAXIMUM_OUTPUT_CHANNELS], send[4096];
	short *samples = noise_samples(BLOCK_FRAMES + 8192);
	BOOL is_3d = scene >= 2, through_hrtf = scene == 5 || scene == 7;
	struct sdl_stream *stream;
	unsigned long done = 0, call = 0, channels, saved = output_channels;
	int change = 0;

	output_channels = scene == 6 || scene == 7 ? 6 : 2;
	channels = output_channels;
	hrtf_initialize();
	listener_reset();
	listener.distance_factor = 3.048f;
	environment.lRoom = scene == 3 ? 0 : DSBVOLUME_MIN;
	environment.lRoomHF = 0;
	hrtf_enabled = through_hrtf;
	if (is_3d)
		stream = voice_placed(samples, BLOCK_FRAMES + 8192, scene == 8 ? 22050 : 48000, 1.0f, 0.0f, 2.0f);
	else
		stream = voice_new(samples, BLOCK_FRAMES + 8192, 1, scene == 1 ? 22050 : 48000, 0);
	if (scene == 2 || scene == 3)
	{
		stream->direct = stream->direct_hf = 0;
		stream->room = stream->room_hf = scene == 3 ? 0 : DSBVOLUME_MIN;
	}
	if (scene == 8)
	{
		stream->position[0] = -10.0f;
		stream->position[2] = 1.0f;
		stream->velocity[0] = 40.0f;
		stream->minimum_distance = 4.0f;
		stream->maximum_distance = 1000.0f;
	}
	while (done < BLOCK_FRAMES)
	{
		const unsigned long *sizes = block_partitions[partition];
		unsigned long count, sizes_count = 0, frame, channel;

		while (sizes_count < 10 && sizes[sizes_count])
			sizes_count++;
		while (change < BLOCK_CHANGES && block_change_frames[change] == done)
			block_change(scene, stream, change++);
		count = sizes[call++ % sizes_count];
		if (count > BLOCK_FRAMES - done)
			count = BLOCK_FRAMES - done;
		if (change < BLOCK_CHANGES && done + count > block_change_frames[change])
			count = block_change_frames[change] - done;
		memset(buffer, 0, count * channels * sizeof(float));
		memset(send, 0, count * sizeof(float));
		mix_voice(stream, buffer, send, count);
		for (frame = 0; frame < count; frame++)
		{
			for (channel = 0; channel < channels; channel++)
				output[(done + frame) * channels + channel] = buffer[frame * channels + channel];
			room[done + frame] = send[frame];
		}
		done += count;
	}
	output_channels = saved;
	hrtf_enabled = FALSE;
	environment.lRoom = DSBVOLUME_MIN;
	voice_delete(stream);
	free(samples);
	return channels;
}

/* how far apart two renders of a scene are: the largest difference of a
frame, against the largest frame, in dB (-400: the same) */
static double block_difference(const float *a, const float *b, unsigned long count)
{
	double largest = 0.0, peak = 0.0;
	unsigned long n;

	for (n = 0; n < count; n++)
	{
		double difference = fabs((double)a[n] - b[n]);

		largest = difference > largest ? difference : largest;
		peak = fabs(a[n]) > peak ? fabs(a[n]) : peak;
	}
	return largest > 0.0 && peak > 0.0 ? 20.0 * log10(largest / peak) : -400.0;
}

/* each scene in each partition against the twin's (480 frames a call):
differences[scene][partition] for the output and the room send */
static void block_figures(double differences[BLOCK_SCENES][BLOCK_PARTITIONS][2])
{
	float *output[2], *room[2];
	int scene, partition;

	output[0] = malloc(BLOCK_FRAMES * MAXIMUM_OUTPUT_CHANNELS * sizeof(float));
	output[1] = malloc(BLOCK_FRAMES * MAXIMUM_OUTPUT_CHANNELS * sizeof(float));
	room[0] = malloc(BLOCK_FRAMES * sizeof(float));
	room[1] = malloc(BLOCK_FRAMES * sizeof(float));
	resampler_mode = RESAMPLER_SINC;
	for (scene = 0; scene < BLOCK_SCENES; scene++)
	{
		unsigned long channels = block_render(scene, 2, output[0], room[0]);

		for (partition = 0; partition < BLOCK_PARTITIONS; partition++)
		{
			block_render(scene, partition, output[1], room[1]);
			differences[scene][partition][0] = block_difference(output[0], output[1], BLOCK_FRAMES * channels);
			differences[scene][partition][1] = scene == 3 ? block_difference(room[0], room[1], BLOCK_FRAMES) : -400.0;
		}
	}
	free(output[0]);
	free(output[1]);
	free(room[0]);
	free(room[1]);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	unsigned long index;

	hrtf_enabled = FALSE;
	resampler_initialize();
	resampler_phases_initialize();
	/* (the cases are of the mixer's own low pass but where they say) */
	resampler_mode = RESAMPLER_SINC;

	/* audio.resampler = "xbox", the default: the direct path through the
	Xbox's low pass, as recorded (issue 714's seventh run): a click at 32 kHz
	played at 24 kHz, half the output's rate, is the click itself on the
	frames and between them the Xbox's weights halfway, 0.598, -0.120, 0.024
	and -0.003, both channels of a stereo voice alike; nothing further than
	four frames */
	CASE("resampler-xbox")
	{
		static const double halfway[4] = { 0.598, -0.120, 0.024, -0.003 };
		short samples[64 * 2];
		float output[160 * 2];
		unsigned long channels, n;

		resampler_setting = "xbox";
		CHECK(resampler_mode_read() == RESAMPLER_XBOX, "audio.resampler = \"xbox\" is not read as xbox");
		resampler_setting = "";
		CHECK(resampler_mode_read() == RESAMPLER_XBOX, "audio.resampler = \"\" is not read as xbox");
		resampler_setting = NULL;
		resampler_mode = resampler_mode_read();
		CHECK(resampler_mode == RESAMPLER_XBOX, "audio.resampler unset is not read as xbox");
		for (channels = 1; channels <= 2; channels++)
		{
			struct sdl_stream *stream;

			memset(samples, 0, sizeof(samples));
			samples[20 * channels] = 16384;
			if (channels == 2)
				samples[20 * channels + 1] = -16384;
			stream = voice_new(samples, 64, channels, 32000, 24000);
			render(stream, output, 100, TRUE);
			for (n = 0; n < 100; n++)
			{
				long offset = (long)n - 40;
				double expected = offset == 0 ? 0.5 : (offset & 1) && labs(offset) < 8 ?
					0.5 * halfway[(labs(offset) - 1) / 2] : 0.0;

				CHECK(fabs(output[2 * n] - expected) < 0.5 * 0.003, "%lu channels: frame %ld from the click is "
					"%.4f of it, not %.4f", channels, offset, output[2 * n] / 0.5, expected / 0.5);
				CHECK(output[2 * n + 1] == (channels == 2 ? -output[2 * n] : output[2 * n]),
					"%lu channels: the right channel's frame %ld is %f, the left's %f", channels, offset,
					output[2 * n + 1], output[2 * n]);
			}
			voice_delete(stream);
		}
		resampler_mode = RESAMPLER_SINC;
		return 0;
	}
	/* audio.resampler = "xbox" at 22.05 kHz, as recorded (issue 792's
	eighth run): a 4003 Hz sine's images at 0.82, 1.18 and 1.82 of the
	source's rate (18047, 21947 and 7903 Hz) -63, -71 and -79 dB under it;
	and sixteen one-sample clicks 1103 frames apart, each on another phase,
	whose 2 kHz bands of 16 to 22 kHz average -34, -41 and -50 dB under
	their passband (0.5 kHz to 0.7 of the source's Nyquist rate), where the
	low pass's lobe from 1.3 to 1.6 of the source's rate comes down */
	CASE("resampler-xbox-images")
	{
		static const double images[3][3] = { { 18047.0, -63.0, 2.0 }, { 21947.0, -71.0, 2.0 },
			{ 7903.0, -79.0, 6.0 } };
		static const double bands[3][2] = { { 16000.0, -34.0 }, { 18000.0, -41.0 }, { 20000.0, -50.0 } };
		unsigned long frames = 30000, n, k, click;
		short *samples = calloc(frames, sizeof(short));
		float *output = calloc(48000, sizeof(float));
		double band_sum[3] = { 0 };
		struct sdl_stream *stream;

		resampler_mode = RESAMPLER_XBOX;
		for (n = 0; n < frames; n++)
			samples[n] = (short)floor(16384.0 * sin(2.0 * 3.14159265358979 * 4003.0 * n / 22050.0) + 0.5);
		stream = voice_new(samples, frames, 1, 22050, 0);
		render(stream, output, 48000, FALSE);
		voice_delete(stream);
		{
			/* (32768 frames from 0.1 s, Blackman windowed: each image's
			level under the tone's) */
			double tone = window_bin(output + 4800, 32768, 4003.0);

			for (k = 0; k < 3; k++)
			{
				double level = 20.0 * log10(window_bin(output + 4800, 32768, images[k][0]) / tone);

				CHECK(fabs(level - images[k][1]) <= images[k][2], "a 4003 Hz sine at 22.05 kHz: its image at %.0f Hz "
					"is %+.1f dB under it (the Xbox's: %g)", images[k][0], level, images[k][1]);
			}
		}
		memset(samples, 0, frames * sizeof(short));
		for (click = 0; click < 16; click++)
			samples[551 + 1103 * click] = 16384;
		stream = voice_new(samples, frames, 1, 22050, 0);
		memset(output, 0, 48000 * sizeof(float));
		render(stream, output, 40000, FALSE);
		voice_delete(stream);
		for (click = 0; click < 16; click++)
		{
			/* (from 2 ms before the click's largest frame to 8 ms after) */
			unsigned long start = (unsigned long)((551.0 + 1103.0 * click) * OUTPUT_RATE / 22050.0) - 40, peak = start;
			double passband = 0.0, f;
			unsigned long count = 0;

			for (n = start; n < start + 80; n++)
				peak = fabsf(output[n]) > fabsf(output[peak]) ? n : peak;
			for (f = 500.0; f <= 0.7 * 11025.0; f += 50.0, count++)
				passband += pow(segment_bin(output + peak - 96, 480, f), 2);
			passband /= count;
			for (k = 0; k < 3; k++)
			{
				double power = 0.0;

				for (f = bands[k][0]; f < bands[k][0] + 2000.0; f += 50.0)
					power += pow(segment_bin(output + peak - 96, 480, f), 2) / 40.0;
				band_sum[k] += power / passband / 16.0;
			}
		}
		for (k = 0; k < 3; k++)
			CHECK(fabs(10.0 * log10(band_sum[k]) - bands[k][1]) <= 2.0, "sixteen clicks at 22.05 kHz: %.0f to %.0f kHz "
				"averages %+.1f dB under their passband (the Xbox's: %g)", bands[k][0] / 1000.0,
				bands[k][0] / 1000.0 + 2.0, 10.0 * log10(band_sum[k]), bands[k][1]);
		resampler_mode = RESAMPLER_SINC;
		free(output);
		free(samples);
		return 0;
	}

	/* Xbox ADPCM decoded as the reference decoder decodes the two blocks, and
	as the format reads a frame at a time for blocks of every header (step
	indices past 88, predictors at full scale, nibbles that clip) */
	CASE("adpcm")
	{
		static unsigned char blocks[400 * 72];
		short expected[64 * 2];
		unsigned long frames, channels, block, n;
		short *decoded = decode_adpcm(adpcm_stereo_block, 72, 2, &frames);
		unsigned char mono[36];

		CHECK(decoded && frames == 64, "a stereo block is %lu frames, not 64", frames);
		for (n = 0; n < 64 * 2; n++)
			CHECK(decoded[n] == adpcm_stereo_frames[n % 2][n / 2], "stereo frame %lu, channel %lu: %d, not %d", n / 2,
				n % 2, decoded[n], adpcm_stereo_frames[n % 2][n / 2]);
		free(decoded);
		mono[0] = 0x00;
		mono[1] = 0x7d;
		mono[2] = 80;
		mono[3] = 0;
		for (n = 4; n < 36; n++)
			mono[n] = (unsigned char)((n * 73 + 41) & 0xff);
		decoded = decode_adpcm(mono, 36, 1, &frames);
		CHECK(decoded && frames == 64, "a mono block is %lu frames, not 64", frames);
		for (n = 0; n < 64; n++)
			CHECK(decoded[n] == adpcm_mono_frames[n], "mono frame %lu: %d, not %d", n, decoded[n], adpcm_mono_frames[n]);
		free(decoded);
		random_state = 3;
		for (n = 0; n < sizeof(blocks); n++)
			blocks[n] = (unsigned char)((random_uniform() + 1.0) * 128.0);
		for (channels = 1; channels <= 2; channels++)
		{
			unsigned long count = sizeof(blocks) / (36 * channels);

			/* (and a part block at the end, which is not played) */
			decoded = decode_adpcm(blocks, sizeof(blocks) - 7, channels, &frames);
			CHECK(frames == (count - 1) * 64, "%lu channels: %lu frames of %lu blocks and a part", channels, frames,
				count);
			for (block = 0; block + 1 < count; block++)
			{
				adpcm_expected(blocks + block * 36 * channels, channels, expected);
				for (n = 0; n < 64 * channels; n++)
					CHECK(decoded[block * 64 * channels + n] == expected[n], "%lu channels, block %lu (step index %d), "
						"frame %lu: %d, not %d", channels, block, blocks[block * 36 * channels + 4 * (n % channels) + 2],
						n / channels, decoded[block * 64 * channels + n], expected[n]);
			}
			free(decoded);
		}
		return 0;
	}
	/* the Doppler shift, the Xbox's first order one: a sound coming at 10
	units a second (30.5 m/s) is heard 1 + 30.5 / 343 times as high, going
	away 1 - 30.5 / 343; the listener coming at it 1 + 30.5 / 343; and both
	coming, the two added; head-relative, the voice's own velocity alone; to
	an octave at most; none with Doppler factor zero, nor for a 2D voice.
	Recorded on an Xbox, a source passing at 20 and 40 units a second was
	shifted 1.178 and 1.356 coming, 0.822 and 0.644 going */
	CASE("doppler")
	{
		static const struct { float velocity, listener_velocity; DWORD mode; double expected; } cases[] = {
			{ 0.0f, 0.0f, DS3DMODE_NORMAL, 1.0 },
			{ -10.0f, 0.0f, DS3DMODE_NORMAL, 1.0 + 30.48 / 343.0 },
			{ 10.0f, 0.0f, DS3DMODE_NORMAL, 1.0 - 30.48 / 343.0 },
			{ 0.0f, 10.0f, DS3DMODE_NORMAL, 1.0 + 30.48 / 343.0 },
			{ -20.0f, 0.0f, DS3DMODE_NORMAL, 1.0 + 60.96 / 343.0 },
			{ -40.0f, 0.0f, DS3DMODE_NORMAL, 1.0 + 121.92 / 343.0 },
			{ -10.0f, 10.0f, DS3DMODE_NORMAL, 1.0 + 60.96 / 343.0 },
			{ -10.0f, 10.0f, DS3DMODE_HEADRELATIVE, 1.0 + 30.48 / 343.0 },
			{ 1000.0f, 0.0f, DS3DMODE_NORMAL, 0.5 },
			{ -1000.0f, 0.0f, DS3DMODE_NORMAL, 2.0 },
			{ -10.0f, 0.0f, DS3DMODE_DISABLE, 1.0 },
		};
		float ratio;
		double rest_db;

		resampler_mode = RESAMPLER_SINC;
		for (index = 0; index < sizeof(cases) / sizeof(cases[0]); index++)
		{
			doppler_figures(cases[index].velocity, cases[index].listener_velocity, cases[index].mode,
				cases[index].expected, &ratio, &rest_db);
			CHECK(fabs(ratio - cases[index].expected) < 1.0e-5 && rest_db < -60.0, "at %.0f units a second, the "
				"listener at %.0f (mode %lu): shifted %.5f, not %.5f; the rest %.1f dB", cases[index].velocity,
				cases[index].listener_velocity, cases[index].mode, ratio, cases[index].expected, rest_db);
		}
		listener.doppler_factor = 0.0f;
		doppler_figures(-10.0f, 0.0f, DS3DMODE_NORMAL, 1.0, &ratio, &rest_db);
		CHECK(ratio == 1.0f && rest_db < -60.0, "Doppler factor zero: shifted %.5f, the rest %.1f dB", ratio, rest_db);
		listener.doppler_factor = 1.0f;
		return 0;
	}
	/* a voice's volume changed between mixes moves to the new one over
	PARAMETER_RAMP_FRAMES (1.3 ms) and holds it, however many frames the mix
	has, as an Xbox's settled from 10% to 90% within 1 to 2 ms (recorded: a
	step of SetVolume, SetFrequency or the position); the mix of 512 frames
	it took before (8 ms) */
	CASE("parameter-ramp")
	{
		static short samples[48000];
		static float output[CHUNK * 2], send[CHUNK];
		struct sdl_stream *stream;
		float before, ratio;
		unsigned long n;

		for (n = 0; n < 48000; n++)
			samples[n] = 8000;
		resampler_mode = RESAMPLER_SINC;
		stream = voice_new(samples, 48000, 1, 48000, 0);
		for (n = 0; n < 2; n++)
		{
			memset(output, 0, sizeof(output));
			mix_voice(stream, output, send, CHUNK);
		}
		before = output[(CHUNK - 1) * 2];
		stream->volume = 0.25f;
		memset(output, 0, sizeof(output));
		mix_voice(stream, output, send, CHUNK);
		ratio = output[16 * 2] / before;
		CHECK(before > 0.0f && ratio < 0.95f && ratio > 0.3f, "16 frames after the change at %.3f of the volume before",
			ratio);
		for (n = 64; n < CHUNK; n++)
		{
			ratio = output[n * 2] / before;
			CHECK(fabsf(ratio - 0.25f) < 1.0e-4f, "%lu frames after the change at %.4f of the volume before, not "
				"0.25", n, ratio);
		}
		voice_delete(stream);
		return 0;
	}
	/* the Doppler shift steps with the voice's position and velocity, as a
	change of its pitch does (over PARAMETER_RAMP_FRAMES): recorded on an
	Xbox, a source passing at 40 units a second, its position set every
	34 ms, stepped its pitch at each set, from 1.356 coming to 0.644 going,
	and settled within 2 ms. A voice coming at 40 units a second ten units to
	the left of ahead, then going ten to the right, plays the second shift
	from PARAMETER_RAMP_FRAMES into the mix after the change */
	CASE("doppler-step")
	{
		unsigned long frames = 22050, n;
		short *samples = malloc(frames * sizeof(short));
		double *signal = malloc(frames * sizeof(double)), *exact = malloc(frames * sizeof(double)), amplitude,
			rest_db;
		float *output = calloc(4 * CHUNK, sizeof(float)), coming, going;
		struct sdl_stream *stream;

		for (n = 0; n < frames; n++)
			signal[n] = 0.5 * sin(2.0 * PI * 1000.0 * n / 22050.0);
		quantize(signal, samples, exact, frames);
		resampler_mode = RESAMPLER_SINC;
		listener.distance_factor = 3.048f;
		stream = voice_new(samples, frames, 1, 22050, 0);
		stream->has_3d = TRUE;
		stream->mode = DS3DMODE_NORMAL;
		stream->position[0] = -10.0f;
		stream->position[2] = 1.0f;
		stream->velocity[0] = 40.0f;
		stream->minimum_distance = 4.0f;
		stream->maximum_distance = 1000.0f;
		render(stream, output, 2 * CHUNK, FALSE);
		coming = stream->current_doppler;
		stream->position[0] = 10.0f;
		render(stream, output + 2 * CHUNK, 2 * CHUNK, FALSE);
		going = stream->current_doppler;
		tone_fit(output, 2 * CHUNK + PARAMETER_RAMP_FRAMES, 4 * CHUNK, 1000.0 * going / OUTPUT_RATE, &amplitude,
			&rest_db);
		CHECK(fabsf(coming - 1.356f) < 0.01f && fabsf(going - 0.644f) < 0.01f && rest_db < -40.0,
			"shifted x%.4f coming, x%.4f going (an Xbox: 1.356, 0.644); the mix after the change, from %d frames "
			"in, %.1f dB besides the second", coming, going, PARAMETER_RAMP_FRAMES, rest_db);
		voice_delete(stream);
		free(output);
		free(exact);
		free(signal);
		free(samples);
		return 0;
	}
	/* the mix at the Xbox's output level, half (-6.02 dB): recorded on an
	Xbox, a voice through its front left mix bin alone at 0.5 of full scale
	came out at 0.2505, every other sound at half too, and a sum of 1.40 came
	out whole at 0.70. So a voice at full scale comes out at half, two such
	voices' sum whole (not limited), and the limiter holds three's to full
	scale */

	CASE("output-level")
	{
		unsigned long frames = 48000 * 2, n, chunk;
		short *samples = malloc(frames * 2 * sizeof(short));
		double *signal = malloc(frames * sizeof(double));
		static float mixed[CHUNK * 2];
		struct sdl_stream *voices[3];
		float peaks[3] = { 0.0f, 0.0f, 0.0f };
		unsigned long count, voice;

		for (n = 0; n < frames; n++)
			signal[n] = 0.999 * sin(2.0 * PI * 997.0 * n / 48000.0);
		for (n = 0; n < frames; n++)
			samples[n * 2] = samples[n * 2 + 1] = (short)lround(signal[n] * 32768.0);
		reverb_initialize();
		/* (one voice, then two, then three, each lot started together) */
		for (count = 1; count <= 3; count++)
		{
			streams = NULL;
			for (voice = 0; voice < count; voice++)
			{
				voices[voice] = voice_new(samples, frames, 2, 48000, 0);
				voices[voice]->next = streams;
				streams = voices[voice];
			}
			/* Measure steady levels after the limiter settles. */
			for (chunk = 0; chunk < OUTPUT_RATE / 2 / CHUNK; chunk++)
			{
				mix(mixed, CHUNK);
				for (n = 0; chunk >= OUTPUT_RATE / 4 / CHUNK && n < CHUNK * 2; n++)
					peaks[count - 1] = fabsf(mixed[n]) > peaks[count - 1] ? fabsf(mixed[n]) : peaks[count - 1];
			}
			streams = NULL;
			/* (the last of the mix played out) */
			for (chunk = 0; chunk < 4; chunk++)
				mix(mixed, CHUNK);
			for (voice = 0; voice < count; voice++)
				voice_delete(voices[voice]);
		}
		CHECK(peaks[0] > 0.4985f && peaks[0] < 0.5f, "a voice at -0.01 dBFS comes out at %.2f dBFS (the Xbox's -6.03)",
			decibels(peaks[0]));
		CHECK(peaks[1] > 0.998f && peaks[1] < 1.0f, "two voices at -0.01 dBFS come out at %.2f dBFS (the Xbox's "
			"-0.01: not limited)", decibels(peaks[1]));
		CHECK(peaks[2] <= 1.0f + 1.0e-6f && peaks[2] > 0.99f, "three voices at -0.01 dBFS come out at %.2f dBFS",
			decibels(peaks[2]));
		free(signal);
		free(samples);
		return 0;
	}

	/* obstruction and occlusion, as the Xbox recorded them: a voice of noise
	2 units ahead in a hall (the reverb on), its direct path's octave levels
	(the recording's impulse response, the direct sound's 9 ms) against the
	voice unobstructed, within 2 dB of the Xbox's at each, for obstruction
	alone (its LF ratio 0 and 0.5) and with occlusion. The Xbox muffles the
	reverb's send as the direct path for both: a voice obstructed by -1200
	(LF ratio 0) was recorded the same as one occluded by -1200, sample for
	sample; so the send of an obstructed voice is muffled as its direct path,
	and obstruction's LF ratio lowers the direct path alone (its recording
	4.5 dB louder than occlusion's, the reverb's part) */
	CASE("obstruction")
	{
		static const struct { const char *name; LONG obstruction; float ratio; LONG occlusion; double xbox[OCTAVES]; }
			cases[] = {
			{ "obstructed by -1200", -1200, 0.0f, 0, { -0.6, -0.3, -0.6, -0.9, -2.3, -5.6, -10.0, -15.7, -19.4 } },
			{ "obstructed by -1200, LF ratio 0.5", -1200, 0.5f, 0,
				{ -6.5, -6.1, -6.4, -6.4, -6.7, -8.0, -10.6, -15.2, -18.3 } },
			{ "obstructed by -4000", -4000, 0.0f, 0, { -4.2, -9.4, -14.8, -20.5, -26.2, -32.1, -37.7, -43.7, -47.5 } },
			{ "obstructed and occluded by -1500", -1500, 0.0f, -1500,
				{ -1.3, -2.7, -6.2, -10.8, -16.4, -22.2, -27.8, -33.8, -37.6 } },
		};
		unsigned long frames = 24000, band;
		short *samples = noise_samples(frames + 8192);
		float *left = malloc(frames * sizeof(float)), *room = malloc(frames * sizeof(float));
		double open_direct[OCTAVES], open_room[OCTAVES], direct[OCTAVES], send[OCTAVES];
		int which;

		hrtf_initialize();
		listener_reset();
		environment.lRoom = 0;
		environment.lRoomHF = 0;
		for (which = -1; which < (int)(sizeof(cases) / sizeof(cases[0])); which++)
		{
			DSI3DL2BUFFER source;
			struct sdl_stream *stream = voice_placed(samples, frames + 8192, 48000, 0.0f, 0.0f, 2.0f);

			memset(&source, 0, sizeof(source));
			if (which >= 0)
			{
				source.Obstruction.lHFLevel = cases[which].obstruction;
				source.Obstruction.flLFRatio = cases[which].ratio;
				source.Occlusion.lHFLevel = cases[which].occlusion;
			}
			IDirectSoundStream_SetI3DL2Source(&stream->object, &source, 0);
			render_with_send(stream, frames, left, room);
			voice_delete(stream);
			octave_levels(left, frames, 1, 0, which < 0 ? open_direct : direct);
			octave_levels(room, frames, 1, 0, which < 0 ? open_room : send);
			if (which < 0)
				continue;
			for (band = 0; band < OCTAVES; band++)
			{
				CHECK(fabs(direct[band] - open_direct[band] - cases[which].xbox[band]) < 2.0,
					"%s: the direct path at %.0f Hz %+.1f dB (the Xbox's %+.1f)", cases[which].name, octave_hertz[band],
					direct[band] - open_direct[band], cases[which].xbox[band]);
				/* (the send muffled as the direct path; not lowered by
				obstruction's LF ratio) */
				CHECK(fabs((send[band] - open_room[band]) - (direct[band] - open_direct[band]) +
					(cases[which].obstruction * cases[which].ratio / 100.0)) < 0.5,
					"%s: the room send at %.0f Hz %+.1f dB, the direct path %+.1f dB", cases[which].name, octave_hertz[band],
					send[band] - open_room[band], direct[band] - open_direct[band]);
			}
		}
		/* (obstructed and occluded alike, LF ratio 0) */
		{
			DSI3DL2BUFFER obstructed, occluded;
			struct sdl_stream *first = voice_placed(samples, 64, 48000, 0.0f, 0.0f, 2.0f);
			struct sdl_stream *second = voice_placed(samples, 64, 48000, 0.0f, 0.0f, 2.0f);

			memset(&obstructed, 0, sizeof(obstructed));
			memset(&occluded, 0, sizeof(occluded));
			obstructed.Obstruction.lHFLevel = -1200;
			occluded.Occlusion.lHFLevel = -1200;
			IDirectSoundStream_SetI3DL2Source(&first->object, &obstructed, 0);
			IDirectSoundStream_SetI3DL2Source(&second->object, &occluded, 0);
			CHECK(first->direct == second->direct && first->direct_hf == second->direct_hf &&
				first->room == second->room && first->room_hf == second->room_hf,
				"obstructed by -1200: %ld %ld, room %ld %ld; occluded: %ld %ld, room %ld %ld", first->direct,
				first->direct_hf, first->room, first->room_hf, second->direct,
				second->direct_hf, second->room, second->room_hf);
			voice_delete(first);
			voice_delete(second);
		}
		environment.lRoom = DSBVOLUME_MIN;
		free(left);
		free(room);
		free(samples);
		return 0;
	}
	/* the room send of a voice at the output's rate, as the reverb takes it
	(every other frame): a click sends a single pulse, as the Xbox's did.
	Recorded on an Xbox, the first reflection of a 48 kHz click in the cave
	(Room HF 0) was one pulse with nothing either side, and the cave's
	reverberation below 500 Hz was 4.1 dB louder than the mixer's (equal
	above 2 kHz), as was each room's by as much of the send's top octave
	as its Room HF low pass left: the resampler's low pass took the send's
	last 23 to 24 kHz out, which the reverb's frames fold onto its low end.
	Here the frames the reverb takes about the click add up to the click
	within 0.2 dB (they added up to 3.9 dB under it), and none but the
	click's is over 1% of it */
	CASE("room-send")
	{
		unsigned long frames = 4096, at = CHUNK + 1000, peak = 0, n;
		short *samples = calloc(frames + 8192, sizeof(short));
		float *left = malloc(frames * sizeof(float)), *room = malloc(frames * sizeof(float));
		double sum = 0.0, largest = 0.0;
		DSI3DL2BUFFER source;
		struct sdl_stream *stream;

		hrtf_initialize();
		listener_reset();
		environment.lRoom = 0;
		environment.lRoomHF = 0;
		samples[at] = 16384;
		stream = voice_placed(samples, frames + 8192, 48000, 0.0f, 0.0f, 2.0f);
		memset(&source, 0, sizeof(source));
		IDirectSoundStream_SetI3DL2Source(&stream->object, &source, 0);
		render_with_send(stream, frames, left, room);
		voice_delete(stream);
		for (n = 0; n < frames; n++)
			peak = fabsf(room[n]) > fabsf(room[peak]) ? n : peak;
		CHECK(room[peak] > 0.0f && peak > 64 && peak + 64 < frames, "the click's send at frame %lu, %g", peak,
			room[peak]);
		for (n = peak - 64; n <= peak + 64; n += 2)
		{
			sum += room[n];
			if (n != peak && fabs(room[n]) > largest)
				largest = fabs(room[n]);
		}
		CHECK(fabs(20.0 * log10(sum / room[peak])) < 0.2,
			"the reverb's frames about the click add up to %+.2f dB of it (the Xbox's: the click alone)",
			20.0 * log10(fabs(sum) / room[peak] + 1e-30));
		CHECK(largest < 0.01 * room[peak], "a frame the reverb takes beside the click at %.1f%% of it", 100.0 * largest /
			room[peak]);
		/* the source's Room HF does nothing (the test bed's ninth run: the
		cave's reverb of a voice at a Room and Room HF of -1204, as the game
		sets most of its 3D voices, 12 dB down at every frequency); the
		listener's and occlusion's still muffle the send */
		stream = voice_placed(samples, frames + 8192, 48000, 0.0f, 0.0f, 2.0f);
		source.lRoom = -1204;
		source.lRoomHF = -1204;
		IDirectSoundStream_SetI3DL2Source(&stream->object, &source, 0);
		CHECK(stream->room == -1204 && stream->room_hf == -1204,
			"a Room and Room HF of -1204 send at %ld and %ld mB at high frequencies (the Xbox's: -1204 at both)",
			(long)stream->room, (long)stream->room_hf);
		source.Occlusion.lHFLevel = -2000;
		source.Occlusion.flLFRatio = 0.2f;
		IDirectSoundStream_SetI3DL2Source(&stream->object, &source, 0);
		CHECK(stream->room == -1604 && stream->room_hf == -3204,
			"occluded by -2000 (LF ratio 0.2), the send at %ld and %ld mB (the Xbox's: -1604 and -3204)",
			(long)stream->room, (long)stream->room_hf);
		voice_delete(stream);
		environment.lRoom = DSBVOLUME_MIN;
		free(left);
		free(room);
		free(samples);
		return 0;
	}
	/* a voice's room send through the Xbox's resampler, as recorded (issue
	714's sixth run): the frames around its moment weighted by a sinc at the
	source's Nyquist rate, Hann windowed three frames each side, worked out
	here in double precision; never narrowed for a voice pitched up (44.1
	kHz at 1.5), so its images and aliases near 24 kHz, which the reverb
	folds onto its low end, are the Xbox's. Noise at 44.1 kHz at pitch 1,
	1.5 and 0.75, at 22.05 kHz, and at 48 kHz at 0.75, each at the same
	level */
	CASE("room-send-images")
	{
		static const struct { unsigned long rate, frequency; } voices[] = {
			{ 44100, 0 }, { 44100, 66150 }, { 44100, 33075 }, { 22050, 0 }, { 48000, 36000 },
		};
		unsigned long frames = 8192, source_frames = 32768, n;
		double *signal = noise(source_frames), *exact = malloc(source_frames * sizeof(double)), first_gain = 0.0;
		short *samples = malloc(source_frames * sizeof(short));
		float *left = malloc(frames * sizeof(float)), *room = malloc(frames * sizeof(float));

		quantize(signal, samples, exact, source_frames);
		hrtf_initialize();
		listener_reset();
		environment.lRoom = 0;
		environment.lRoomHF = 0;
		for (index = 0; index < sizeof(voices) / sizeof(voices[0]); index++)
		{
			double step = (double)(voices[index].frequency ? voices[index].frequency : voices[index].rate) / 48000.0;
			double along = 0.0, power = 0.0, rest = 0.0, gain;
			DSI3DL2BUFFER source;
			struct sdl_stream *stream = voice_placed(samples, source_frames, voices[index].rate, 0.0f, 0.0f, 2.0f);
			double *expected = malloc(frames * sizeof(double));

			stream->frequency = voices[index].frequency;
			memset(&source, 0, sizeof(source));
			IDirectSoundStream_SetI3DL2Source(&stream->object, &source, 0);
			render_with_send(stream, frames, left, room);
			voice_delete(stream);
			/* (render_with_send leaves out the first mix) */
			for (n = 0; n < frames; n++)
			{
				double time = (double)(CHUNK + n) * step, sum = 0.0;
				long center = (long)floor(time), k;

				for (k = center - 3; k <= center + 3; k++)
				{
					double distance = fabs(time - (double)k);

					if (k < 0 || distance >= 3.0)
						continue;
					sum += exact[k] * (distance == 0.0 ? 1.0 : sin(PI * distance) / (PI * distance) *
						(0.5 + 0.5 * cos(PI * distance / 3.0)));
				}
				expected[n] = sum;
				along += room[n] * sum;
				power += sum * sum;
			}
			gain = along / power;
			for (n = 0; n < frames; n++)
				rest += (room[n] - gain * expected[n]) * (room[n] - gain * expected[n]);
			CHECK(gain > 0.0 && 10.0 * log10(rest / (gain * gain * power) + 1e-30) < -80.0,
				"%lu Hz at %g: the room send differs from the Xbox's resampler by %.1f dB", voices[index].rate, step *
				48000.0 / voices[index].rate, 10.0 * log10(rest / (gain * gain * power) + 1e-30));
			if (!index)
				first_gain = gain;
			CHECK(fabs(20.0 * log10(gain / first_gain)) < 0.01, "%lu Hz at %g: the room send %+.3f dB from the first's",
				voices[index].rate, step * 48000.0 / voices[index].rate, 20.0 * log10(gain / first_gain));
			free(expected);
		}
		environment.lRoom = DSBVOLUME_MIN;
		free(left);
		free(room);
		free(samples);
		free(exact);
		free(signal);
		return 0;
	}
	/* the Xbox's quietest levels (voice_level), as recorded (issue 714's
	sixth run: a 2D voice stepped down from -40 dB): each step's level, and
	silence from -58 dB; louder never quieter; through the mixer a 2D voice,
	a 3D one through the HRTF and its room send silent under the floor, and
	audio.volume, which the Xbox did not have, under it, not in it */
	CASE("quiet-levels")
	{
		static const double recorded[][2] = {
			{ -40.0, -39.8 }, { -42.0, -42.1 }, { -44.0, -44.0 }, { -46.0, -45.4 }, { -48.0, -47.2 }, { -50.0, -49.4 },
			{ -52.0, -52.3 }, { -54.0, -56.7 }, { -56.0, -66.2 },
		};
		static const double silent[] = { -58.0, -60.0, -70.0, -80.0, -100.0 };
		unsigned long frames = 24000, n, hundredths;
		short *samples = malloc((frames + 4096) * 2 * sizeof(short));
		float *output = malloc(frames * 2 * sizeof(float)), *room = malloc(frames * sizeof(float));
		double amplitude[3], rest_db, previous = 0.0;
		int pass;

		for (index = 0; index < sizeof(recorded) / sizeof(recorded[0]); index++)
		{
			double level = voice_level((float)pow(10.0, recorded[index][0] / 20.0));

			CHECK(level > 0.0 && fabs(20.0 * log10(level) - recorded[index][1]) < 0.25,
				"%g dB plays at %+.2f dB (the Xbox's: %g)", recorded[index][0], level > 0.0 ? 20.0 * log10(level) :
				-999.0, recorded[index][1]);
		}
		for (index = 0; index < sizeof(silent) / sizeof(silent[0]); index++)
		{
			CHECK(voice_level((float)pow(10.0, silent[index] / 20.0)) == 0.0f, "%g dB is heard (the Xbox's: silent)",
				silent[index]);
		}
		for (hundredths = 6000; hundredths >= 2000; hundredths--)
		{
			double level = voice_level((float)pow(10.0, -(double)hundredths / 2000.0));

			CHECK(level >= previous, "-%.2f dB is quieter than -%.2f dB", hundredths / 100.0, (hundredths + 1) / 100.0);
			CHECK(previous <= 0.0 || hundredths > 5500 || 20.0 * log10(level / previous) < 0.1,
				"the level jumps %+.2f dB at -%.2f dB", 20.0 * log10(level / previous), hundredths / 100.0);
			CHECK(voice_level((float)-level) == -voice_level((float)level), "-%.2f dB's sign not kept",
				hundredths / 100.0);
			previous = level;
		}
		/* through the mixer: a 1 kHz tone as a 2D stereo voice at 0 dB, -54
		dB, and -40 dB under an audio.volume of -40 dB */
		for (n = 0; n < frames + 4096; n++)
			samples[2 * n] = samples[2 * n + 1] = (short)lround(16384.0 * sin(2.0 * PI * n / 48.0));
		for (pass = 0; pass < 3; pass++)
		{
			struct sdl_stream *stream = voice_new(samples, frames + 4096, 2, 48000, 0);

			stream->volume = pass == 1 ? (float)pow(10.0, -54.0 / 20.0) : pass == 2 ? 0.01f : 1.0f;
			master_volume = pass == 2 ? 0.01f : 1.0f;
			render(stream, output, frames, FALSE);
			tone_fit(output, CHUNK, frames, 1.0 / 48.0, &amplitude[pass], &rest_db);
			voice_delete(stream);
		}
		master_volume = 1.0f;
		CHECK(fabs(20.0 * log10(amplitude[1] / amplitude[0]) + 56.7) < 0.25 &&
			fabs(20.0 * log10(amplitude[2] / amplitude[0]) + 80.0) < 0.1,
			"a 2D voice at -54 dB plays at %+.2f dB (the Xbox's: -56.7), at -40 dB under an audio.volume of -40 dB "
			"at %+.2f dB", 20.0 * log10(amplitude[1] / amplitude[0] + 1e-30),
			20.0 * log10(amplitude[2] / amplitude[0] + 1e-30));
		/* a 3D voice through the HRTF 1 m ahead (each ear at -3 dB) at -52
		dB, at the level of a 2D voice of -46 dB (VOICE_LEVEL_3D: its volume
		and rolloff twice over); at -64 dB, sending to the room, silent */
		hrtf_initialize();
		listener_reset();
		environment.lRoom = 0;
		environment.lRoomHF = 0;
		hrtf_enabled = TRUE;
		{
			short *mono = noise_samples(frames + 4096);
			double power_db[2], expected;

			for (pass = 0; pass < 3; pass++)
			{
				DSI3DL2BUFFER source;
				struct sdl_stream *stream = voice_placed(mono, frames + 4096, 48000, 0.0f, 0.0f, 1.0f);

				memset(&source, 0, sizeof(source));
				IDirectSoundStream_SetI3DL2Source(&stream->object, &source, 0);
				stream->volume = (float)pow(10.0, (pass == 0 ? 0.0 : pass == 1 ? -52.0 : -64.0) / 20.0);
				render_with_send(stream, frames, output, room);
				if (pass < 2)
				{
					/* (render_with_send's left channel only) */
					power_db[pass] = ears_power(output, frames / 2);
				}
				for (n = 0; pass == 2 && n < frames; n++)
				{
					CHECK(output[n] == 0.0f && room[n] == 0.0f, "a 3D voice at -64 dB is heard at frame %lu (%g, "
						"room %g)", n, output[n], room[n]);
				}
				voice_delete(stream);
			}
			expected = 20.0 * log10(voice_level((float)(2.0 * pow(10.0, -52.0 / 20.0))) / 2.0);
			CHECK(fabs(power_db[1] - power_db[0] - expected) < 0.1 && expected > -52.0,
				"through the HRTF a voice at -52 dB plays at %+.2f dB (a 2D voice's of -46 dB, %+.2f)",
				power_db[1] - power_db[0], expected);
			free(mono);
		}
		hrtf_enabled = FALSE;
		environment.lRoom = DSBVOLUME_MIN;
		free(samples);
		free(output);
		free(room);
		return 0;
	}
	/* a 3D voice's quietest levels, as recorded (issue 714's seventh run: a
	voice 10 m away, -20 dB of rolloff, stepped down from -20 dB): those of
	its volume and rolloff together at twice their level, the pan's, the
	HRTF's and the room's gains after them; each step's level under its
	level at 0 dB, the same through the pan, the HRTF and to the room, and
	silent from -44 dB. Its room send twice its room level and rolloff
	(issue 792's eighth run: noise bursts in the cave, a room of -10 dB):
	-4 dB within its minimum distance, -10 dB 2 m away, 14 dB under that 10
	m away. With a direct level of -10 dB its direct path's steps are those
	of a voice 10 dB quieter (recorded 10 m away: -25 dB at -24.2, -27 at
	-26.6, -29 at -30.1, -31 at -36.0, silent from -33), its room send's
	those of the voice without it */
	CASE("quiet-levels-3d")
	{
		static const double recorded[][2] = {
			{ -20.0, -20.0 }, { -30.0, -30.0 }, { -32.0, -31.5 }, { -34.0, -33.2 }, { -36.0, -35.4 },
			{ -38.0, -38.3 }, { -40.0, -42.8 }, { -42.0, -52.1 },
		};
		static const double silent[] = { -44.0, -46.0, -50.0, -60.0 };
		static const float aheads[] = { 2.0f, 10.0f, 0.5f };
		static const double direct_recorded[][2] = { { -25.0, -24.2 }, { -27.0, -26.6 }, { -29.0, -30.1 },
			{ -31.0, -36.0 }, { -33.0, -999.0 }, { -35.0, -999.0 } };
		short samples[64] = { 0 };
		float reference[3][4] = { { 0 } };
		int pass, hrtf;

		hrtf_initialize();
		listener_reset();
		environment.lRoom = -1000;
		environment.lRoomHF = 0;
		environment.flRoomRolloffFactor = 0.0f;
		for (hrtf = 0; hrtf < 2; hrtf++)
		{
			hrtf_enabled = hrtf;
			for (pass = -1; pass < (int)(sizeof(recorded) / sizeof(recorded[0]) + sizeof(silent) / sizeof(silent[0]));
				pass++)
			{
				int heard = pass < (int)(sizeof(recorded) / sizeof(recorded[0]));
				double asked = pass < 0 ? 0.0 : heard ? recorded[pass][0] :
					silent[pass - sizeof(recorded) / sizeof(recorded[0])];
				unsigned long place;

				for (place = 0; place < 3; place++)
				{
					DSI3DL2BUFFER source;
					struct sdl_stream *stream = voice_placed(samples, 64, 48000, 0.0f, 0.0f, aheads[place]);
					float left, right, pair[2], room, direct_lowpass, room_lowpass, gains[4];
					BOOL hrtf_on;
					struct hrtf_parameters way;
					unsigned long kind;

					memset(&source, 0, sizeof(source));
					IDirectSoundStream_SetI3DL2Source(&stream->object, &source, 0);
					stream->volume = (float)pow(10.0, asked / 20.0);
					memset(&way, 0, sizeof(way));
					voice_gains(stream, &left, &right, pair, &room, &direct_lowpass, &room_lowpass, &hrtf_on, &way);
					gains[0] = left;
					gains[1] = right;
					gains[2] = room;
					gains[3] = hrtf_on ? way.gain : 0.0f;
					CHECK(!hrtf_on == !hrtf, "a 3D voice %s the HRTF", hrtf ? "not through" : "through");
					if (pass < 0)
					{
						memcpy(reference[place], gains, sizeof(gains));
						if (place == 0)
							CHECK(fabs(20.0 * log10(room) + 10.0) < 0.1, "2 m away its room send is at %+.2f dB "
								"(the Xbox's: -10, the room's -10, the rolloff's -6 and 6 over)", 20.0 * log10(room));
						else if (place == 1)
							CHECK(fabs(20.0 * log10(room / reference[0][2]) + 14.0) < 0.1,
								"10 m away its room send is %+.2f dB of 2 m away's (the Xbox's: -14.0)",
								20.0 * log10(room / reference[0][2]));
						else
							CHECK(fabs(20.0 * log10(room / reference[0][2]) - 6.0) < 0.1,
								"0.5 m away its room send is %+.2f dB of 2 m away's (the Xbox's: +6.0)",
								20.0 * log10(room / reference[0][2]));
					}
					for (kind = 0; pass >= 0 && place == 1 && kind < 4; kind++)
					{
						double level;

						if (reference[place][kind] == 0.0f)
							continue;
						level = gains[kind] / reference[place][kind];
						if (heard)
							CHECK(level > 0.0 && fabs(20.0 * log10(level) - recorded[pass][1]) < 0.4,
								"10 m away at %g dB its %s plays at %+.2f dB (the Xbox's: %g)", asked,
								kind == 0 ? "left" : kind == 1 ? "right" : kind == 2 ? "room send" : "HRTF's gain",
								level > 0.0 ? 20.0 * log10(level) : -999.0, recorded[pass][1]);
						else
							CHECK(level == 0.0, "10 m away at %g dB its %s is heard (the Xbox's: silent)", asked,
								kind == 0 ? "left" : kind == 1 ? "right" : kind == 2 ? "room send" : "HRTF's gain");
					}
					voice_delete(stream);
				}
			}
		}
		hrtf_enabled = FALSE;
		for (pass = -1; pass < (int)(sizeof(direct_recorded) / sizeof(direct_recorded[0])); pass++)
		{
			double asked = pass < 0 ? 0.0 : direct_recorded[pass][0];
			float gains[2][2];
			int direct;

			for (direct = 0; direct < 2; direct++)
			{
				DSI3DL2BUFFER source;
				struct sdl_stream *stream = voice_placed(samples, 64, 48000, 0.0f, 0.0f, 10.0f);
				float left, right, pair[2], room, direct_lowpass, room_lowpass;
				BOOL hrtf_on;
				struct hrtf_parameters way;

				memset(&source, 0, sizeof(source));
				source.lDirect = direct ? -1000 : 0;
				IDirectSoundStream_SetI3DL2Source(&stream->object, &source, 0);
				stream->volume = (float)pow(10.0, asked / 20.0);
				voice_gains(stream, &left, &right, pair, &room, &direct_lowpass, &room_lowpass, &hrtf_on, &way);
				gains[direct][0] = left;
				gains[direct][1] = room;
				voice_delete(stream);
			}
			if (pass < 0)
			{
				memcpy(reference[0], gains[1], sizeof(gains[1]));
				continue;
			}
			{
				double level = gains[1][0] / reference[0][0];

				if (direct_recorded[pass][1] > -900.0)
					CHECK(level > 0.0 && fabs(20.0 * log10(level) - direct_recorded[pass][1]) < 0.5,
						"10 m away, its direct level -10 dB, at %g dB its direct path plays at %+.2f dB (the Xbox's: "
						"%g)", asked, level > 0.0 ? 20.0 * log10(level) : -999.0, direct_recorded[pass][1]);
				else
					CHECK(level < 0.001, "10 m away, its direct level -10 dB, at %g dB its direct path plays at %+.1f "
						"dB (the Xbox's: silent, under the recording's noise 60 dB down)", asked,
						20.0 * log10(level + 1e-30));
				CHECK(gains[1][1] > 0.0f && fabs(gains[1][1] / gains[0][1] - 1.0) < 1.0e-4, "10 m away, its direct "
					"level -10 dB, at %g dB its room send is %+.2f dB of the voice's without it (the Xbox's: the same)",
					asked, 20.0 * log10(gains[1][1] / gains[0][1] + 1e-30));
			}
		}
		environment.lRoom = DSBVOLUME_MIN;
		return 0;
	}
	/* audio.channels and the device's channels: "auto" is 5.1 on six or
	more, stereo on fewer and when the device cannot say; "5.1" and "stereo"
	whatever the device; anything else stereo */

	/* the 5.1 pan (VBAP) of a stereo voice positioned in the world: a sound
	at each speaker's angle plays from that speaker alone; one at any angle
	from the two either side of it, their power summing to 1, their gains'
	vector sum pointing its way (where the tangent law puts the image); never
	from the LFE. A 3D voice on 5.1 as the Xbox's AC-3, recorded: from the
	front pair and the back pair, never the center or the LFE, its power
	split between them by its direction as measured, through the HRTF and
	panned alike; the same sound on both pairs, the near side the louder to
	the side, a sound behind the sound ahead mirrored. 2D voices by the mix
	bins the game gives them on AC-3, and a voice the center, a back or the
	LFE alone plays from it */
	CASE("surround")
	{
		/* each speaker's angle (degrees, clockwise from ahead) and its channel
		in SDL's 5.1 order (FL, FR, FC, LFE, BL, BR) */
		static const struct { double degrees; unsigned long channel; } speakers[] = {
			{ 0.0, 2 }, { 30.0, 1 }, { 110.0, 5 }, { 250.0, 4 }, { 330.0, 0 },
		};
		static const double channel_degrees[MAXIMUM_OUTPUT_CHANNELS] = { 330.0, 30.0, 0.0, -1.0, 250.0, 110.0 };
		float gains[MAXIMUM_OUTPUT_CHANNELS];
		unsigned long speaker, degree;

		for (index = 0; index < sizeof(speakers) / sizeof(speakers[0]); index++)
		{
			double radians = speakers[index].degrees * PI / 180.0;

			surround_pan((float)sin(radians), (float)cos(radians), 0.0f, gains);
			for (speaker = 0; speaker < MAXIMUM_OUTPUT_CHANNELS; speaker++)
			{
				CHECK(fabsf(gains[speaker] - (speaker == speakers[index].channel ? 1.0f : 0.0f)) < 1.0e-4f,
					"a sound at %.0f degrees: channel %lu at %f (its speaker is channel %lu)", speakers[index].degrees,
					speaker, gains[speaker], speakers[index].channel);
			}
		}
		for (degree = 0; degree < 3600; degree++)
		{
			double radians = degree * PI / 1800.0, power = 0.0, x = 0.0, y = 0.0, heard;
			unsigned long sounding = 0;

			surround_pan((float)sin(radians), (float)cos(radians), 0.0f, gains);
			for (speaker = 0; speaker < MAXIMUM_OUTPUT_CHANNELS; speaker++)
			{
				power += (double)gains[speaker] * gains[speaker];
				if (gains[speaker] > 1.0e-4f)
				{
					sounding++;
					x += gains[speaker] * sin(channel_degrees[speaker] * PI / 180.0);
					y += gains[speaker] * cos(channel_degrees[speaker] * PI / 180.0);
				}
			}
			heard = atan2(x, y) * 180.0 / PI;
			heard = fmod(heard - degree / 10.0 + 540.0, 360.0) - 180.0;
			CHECK(gains[3] == 0.0f, "a sound at %.1f degrees: the LFE at %f", degree / 10.0, gains[3]);
			CHECK(fabs(power - 1.0) < 1.0e-4 && sounding <= 2 && fabs(heard) < 0.05,
				"a sound at %.1f degrees: power %f, %lu speakers, heard %.2f degrees off", degree / 10.0, power,
				sounding, heard);
		}
		/* through the mixer: a mono 3D voice of noise 2 units away, as the
		Xbox recordings' sweeps were, around and above and below; the back
		pair's share of the power the recordings' (their AC-3 decoded), to
		the third place */
		{
			static const struct { double azimuth, elevation, back; } xbox[] = {
				{ 0, 0, 0.000 }, { 30, 0, 0.000 }, { 60, 0, 0.212 }, { 90, 0, 0.500 }, { 120, 0, 0.789 },
				{ 150, 0, 1.000 }, { 180, 0, 1.000 }, { 210, 0, 1.000 }, { 240, 0, 0.788 }, { 270, 0, 0.500 },
				{ 300, 0, 0.211 }, { 330, 0, 0.000 }, { 45, -40, 0.211 }, { 135, -40, 0.791 }, { 45, 40, 0.209 },
				{ 135, 40, 0.791 },
			};
			unsigned long frames = 24000;
			short *samples = noise_samples(frames + 8192);
			double ahead_db[2][MAXIMUM_OUTPUT_CHANNELS], behind_db[2][MAXIMUM_OUTPUT_CHANNELS];
			int through;

			hrtf_initialize();
			listener_reset();
			for (through = 0; through < 2; through++)
			{
				hrtf_enabled = through;
				for (index = 0; index < sizeof(xbox) / sizeof(xbox[0]); index++)
				{
					double azimuth = xbox[index].azimuth * PI / 180.0, elevation = xbox[index].elevation * PI / 180.0;
					double energy_db[MAXIMUM_OUTPUT_CHANNELS], front, back, share;
					struct sdl_stream *stream = voice_placed(samples, frames + 8192, 48000,
						(float)(2.0 * sin(azimuth) * cos(elevation)), (float)(2.0 * sin(elevation)),
						(float)(2.0 * cos(azimuth) * cos(elevation)));

					render_surround(stream, frames, energy_db, NULL);
					voice_delete(stream);
					front = pow(10.0, energy_db[0] / 10.0) + pow(10.0, energy_db[1] / 10.0);
					back = pow(10.0, energy_db[4] / 10.0) + pow(10.0, energy_db[5] / 10.0);
					share = back / (front + back);
					CHECK(energy_db[2] == -400.0 && energy_db[3] == -400.0 && fabs(share - xbox[index].back) < 0.005,
						"%s, a 3D voice at %.0f degrees, %+.0f up on 5.1: the back pair's share %.3f (the Xbox's %.3f), "
						"the center %.1f dB, the LFE %.1f dB", through ? "through the HRTF" : "panned",
						xbox[index].azimuth, xbox[index].elevation, share, xbox[index].back, energy_db[2], energy_db[3]);
					/* (the pairs the same sound: the speakers as far apart in
					each, the near one the louder to the side; through the HRTF
					by more than the pan's) */
					if (xbox[index].back > 0.0 && xbox[index].back < 1.0)
					{
						double front_apart = energy_db[1] - energy_db[0], back_apart = energy_db[5] - energy_db[4];

						CHECK(fabs(front_apart - back_apart) < 0.05 &&
							(xbox[index].azimuth < 180.0 ? front_apart : -front_apart) > (through ? 3.0 : 1.0),
							"%s, a 3D voice at %.0f degrees, %+.0f up on 5.1: the front pair %+.2f dB apart, the back "
							"pair %+.2f dB", through ? "through the HRTF" : "panned", xbox[index].azimuth,
							xbox[index].elevation, front_apart, back_apart);
					}
					if (xbox[index].azimuth == 0.0 && xbox[index].elevation == 0.0)
						memcpy(ahead_db[through], energy_db, sizeof(energy_db));
					if (xbox[index].azimuth == 180.0)
						memcpy(behind_db[through], energy_db, sizeof(energy_db));
				}
				/* a sound behind from the back pair as a sound ahead from the
				front pair: the same direction mirrored, not duller (the
				HRTF's behind) */
				CHECK(fabs(ahead_db[through][0] - ahead_db[through][1]) < 0.05 &&
					fabs(behind_db[through][4] - behind_db[through][5]) < 0.05 &&
					fabs(behind_db[through][4] - ahead_db[through][0]) < 0.05,
					"%s on 5.1: ahead the front pair at %.2f and %.2f dB, behind the back pair at %.2f and %.2f dB",
					through ? "through the HRTF" : "panned", ahead_db[through][0], ahead_db[through][1],
					behind_db[through][4], behind_db[through][5]);
			}
			hrtf_enabled = FALSE;
			free(samples);
		}
		/* 2D voices: one at full volume to the center alone, a back alone or
		the LFE alone plays from that speaker alone, at its level (a voice
		whose left and right gains are 0 was skipped) */
		{
			static const unsigned long alone[] = { 2, 4, 5, 3 };
			unsigned long frames = 24000, speaker;
			short *samples = noise_samples(frames + 8192);
			double energy_db[MAXIMUM_OUTPUT_CHANNELS], reference_db[MAXIMUM_OUTPUT_CHANNELS];
			struct sdl_stream *stream;

			stream = voice_new(samples, frames + 8192, 1, 48000, 0);
			memset(stream->surround_bins, 0, sizeof(stream->surround_bins));
			stream->surround_bins[0][0] = 1.0f;
			render_surround(stream, frames, reference_db, NULL);
			voice_delete(stream);
			for (index = 0; index < sizeof(alone) / sizeof(alone[0]); index++)
			{
				stream = voice_new(samples, frames + 8192, 1, 48000, 0);
				stream->mix_left = stream->mix_right = 0.0f;
				memset(stream->surround_bins, 0, sizeof(stream->surround_bins));
				stream->surround_bins[0][alone[index]] = 1.0f;
				render_surround(stream, frames, energy_db, NULL);
				voice_delete(stream);
				for (speaker = 0; speaker < MAXIMUM_OUTPUT_CHANNELS; speaker++)
				{
					CHECK(speaker == alone[index] ? fabs(energy_db[speaker] - reference_db[0]) < 0.01 :
						energy_db[speaker] == -400.0, "a 2D voice to channel %lu alone on 5.1: channel %lu at %.2f dB "
						"(the front left's alone %.2f dB)", alone[index], speaker, energy_db[speaker], reference_db[0]);
				}
			}
			free(samples);
		}
		output_channels = 6;
		/* a mono 2D voice at half to the front three; a stereo one's left
		whole to the front left and at half to the back left, its right
		likewise; none to the center or the LFE */
		{
			unsigned long frames = 4800, count = 4096, n;
			short *samples = malloc(frames * 2 * sizeof(short));
			float *output = calloc(count * MAXIMUM_OUTPUT_CHANNELS, sizeof(float));
			static float send[4096];
			float surround[2][MAXIMUM_OUTPUT_CHANNELS];
			struct sdl_stream *stream;

			for (n = 0; n < frames * 2; n++)
				samples[n] = (short)(n & 1 ? -8000 : 8000);
			stream = voice_new(samples, frames, 1, 48000, 0);
			memset(stream->surround_bins, 0, sizeof(stream->surround_bins));
			stream->surround_bins[0][0] = stream->surround_bins[0][1] = stream->surround_bins[0][2] = 0.5f;
			voice_surround_gains(stream, 1.0f, 1.0f, NULL, surround);
			CHECK(surround[0][0] == 0.5f && surround[0][1] == 0.5f && surround[0][2] == 0.5f && surround[0][3] == 0.0f &&
				surround[0][4] == 0.0f && surround[0][5] == 0.0f, "a mono 2D voice: %g %g %g %g %g %g", surround[0][0],
				surround[0][1], surround[0][2], surround[0][3], surround[0][4], surround[0][5]);
			voice_delete(stream);
			stream = voice_new(samples, frames, 2, 48000, 0);
			memset(stream->surround_bins, 0, sizeof(stream->surround_bins));
			stream->surround_bins[0][0] = 1.0f;
			stream->surround_bins[1][1] = 1.0f;
			stream->surround_bins[0][4] = 0.5f;
			stream->surround_bins[1][5] = 0.5f;
			mix_voice(stream, output, send, count);
			/* (the left +8000, the right -8000: each speaker its side's sign) */
			n = 2048 * MAXIMUM_OUTPUT_CHANNELS;
			CHECK(output[n] > 0.2f && output[n + 1] < -0.2f && fabsf(output[n + 4] - 0.5f * output[n]) < 1.0e-3f &&
				fabsf(output[n + 5] - 0.5f * output[n + 1]) < 1.0e-3f && output[n + 2] == 0.0f && output[n + 3] == 0.0f,
				"a stereo 2D voice on 5.1: %f %f %f %f %f %f", output[n], output[n + 1], output[n + 2], output[n + 3],
				output[n + 4], output[n + 5]);
			voice_delete(stream);
			free(output);
			free(samples);
		}
		output_channels = 2;
		return 0;
	}
	/* the LFE on 5.1, through the whole mix: a 2D voice to the LFE alone,
	a sine at each frequency, its level from the LFE against its level
	from the front left (the same voice to it alone, which the low pass
	leaves as it is): the bass whole, 86 Hz at -3 dB, and above it falling
	24 dB an octave, as an Xbox's did (recorded: a sweep to the LFE alone,
	-3 dB at 86 Hz, a 4th order's slope); its AC-3 carried a 1 kHz tone to
	the LFE more than 84 dB down */
	CASE("lfe")
	{
		static const struct { double hertz, lowest, highest; } expected[] = {
			{ 30.0, -0.1, 0.1 }, { 60.0, -0.4, -0.1 }, { 86.0, -3.5, -2.5 }, { 172.0, -26.0, -22.0 },
			{ 1000.0, -400.0, -84.0 },
		};
		unsigned long frames = 48000, n, chunk;
		short *samples = malloc(frames * sizeof(short));
		static float mixed[CHUNK * MAXIMUM_OUTPUT_CHANNELS];

		output_channels = 6;
		for (index = 0; index < sizeof(expected) / sizeof(expected[0]); index++)
		{
			double energy[2] = { 0.0, 0.0 }, level;
			int speaker;

			for (n = 0; n < frames; n++)
				samples[n] = (short)(16000.0 * sin(2.0 * PI * expected[index].hertz * n / 48000.0));
			for (speaker = 0; speaker < 2; speaker++)
			{
				struct sdl_stream *stream = voice_new(samples, frames, 1, 48000, 0);
				unsigned long channel = speaker ? 3 : 0;

				stream->mix_left = stream->mix_right = 0.0f;
				memset(stream->surround_bins, 0, sizeof(stream->surround_bins));
				stream->surround_bins[0][channel] = 1.0f;
				streams = stream;
				/* (its second quarter second to its fourth: the filter
				settled, before the voice ends) */
				for (chunk = 0; chunk < frames / CHUNK - 4; chunk++)
				{
					mix(mixed, CHUNK);
					for (n = 0; chunk * CHUNK >= 12000 && n < CHUNK; n++)
						energy[speaker] += (double)mixed[n * 6 + channel] * mixed[n * 6 + channel];
				}
				streams = NULL;
				for (chunk = 0; chunk < 16; chunk++)
					mix(mixed, CHUNK);
				voice_delete(stream);
			}
			level = energy[1] > 0.0 ? 10.0 * log10(energy[1] / energy[0]) : -400.0;
			CHECK(energy[0] > 0.0 && level >= expected[index].lowest && level <= expected[index].highest,
				"a %.0f Hz tone to the LFE on 5.1: %.1f dB of the front left's (%.1f to %.1f)", expected[index].hertz,
				level, expected[index].lowest, expected[index].highest);
		}
		output_channels = 2;
		free(samples);
		return 0;
	}
	/* a 3D voice of noise through the HRTF: to the left (90 degrees), the
	right ear 0.69 ms later (33 frames, an Xbox's recorded 0.72 ms less
	what the bands add) and its high frequencies well down; ahead, behind,
	straight above and straight below, the ears alike, and behind duller
	above a few kHz (an Xbox's recorded 30 dB at 16 kHz); 40 degrees below
	ahead brighter at 8 kHz than 40 above, and louder at 1.4 kHz (the
	recordings' 5.4 dB and 2.7 dB). Panned: to the left, louder in the left
	ear but at the same time; above, below and behind exactly as ahead (no
	cue of them) */
	CASE("hrtf-cues")
	{
		unsigned long frames = 48000;
		float *left_hrtf, *ahead_hrtf, *behind_hrtf, *above_hrtf, *below_hrtf, *up_hrtf, *down_hrtf, *left_pan,
			*ahead_pan, *above_pan, *below_pan, *behind_pan;
		double rear_db, low_db, bright_db, middle_db;
		long delay;
		int ear;

		hrtf_initialize();
		listener_reset();
		left_hrtf = placed_noise(TRUE, -3.0f, 0.0f, 0.0f, frames);
		ahead_hrtf = placed_noise(TRUE, 0.0f, 0.0f, 3.0f, frames);
		behind_hrtf = placed_noise(TRUE, 0.0f, 0.0f, -3.0f, frames);
		above_hrtf = placed_noise(TRUE, 0.0f, 3.0f, 0.0f, frames);
		below_hrtf = placed_noise(TRUE, 0.0f, -3.0f, 0.0f, frames);
		/* (40 degrees up and down, ahead) */
		up_hrtf = placed_noise(TRUE, 0.0f, 1.92836f, 2.29813f, frames);
		down_hrtf = placed_noise(TRUE, 0.0f, -1.92836f, 2.29813f, frames);
		left_pan = placed_noise(FALSE, -3.0f, 0.0f, 0.0f, frames);
		ahead_pan = placed_noise(FALSE, 0.0f, 0.0f, 3.0f, frames);
		behind_pan = placed_noise(FALSE, 0.0f, 0.0f, -3.0f, frames);
		above_pan = placed_noise(FALSE, 0.0f, 3.0f, 0.0f, frames);
		below_pan = placed_noise(FALSE, 0.0f, -3.0f, 0.0f, frames);

		delay = ears_delay(left_hrtf, frames);
		CHECK(delay >= 31 && delay <= 35, "to the left through the HRTF, the right ear %ld frames later, not 33", delay);
		CHECK(ears_level(left_hrtf, frames) > 10.0, "to the left through the HRTF, the left ear's high frequencies "
			"%+.1f dB over the right's", ears_level(left_hrtf, frames));
		delay = ears_delay(left_pan, frames);
		CHECK(delay == 0, "to the left panned, the right ear %ld frames later, not at once", delay);
		CHECK(ears_level(left_pan, frames) > 6.0, "to the left panned, the left ear %+.1f dB over the right",
			ears_level(left_pan, frames));
		CHECK(labs(ears_delay(ahead_hrtf, frames)) <= 1 && fabs(ears_level(ahead_hrtf, frames)) < 0.5 &&
			labs(ears_delay(behind_hrtf, frames)) <= 1 && fabs(ears_level(behind_hrtf, frames)) < 0.5,
			"ahead and behind through the HRTF, the ears differ: %ld and %ld frames, %+.2f and %+.2f dB",
			ears_delay(ahead_hrtf, frames), ears_delay(behind_hrtf, frames), ears_level(ahead_hrtf, frames),
			ears_level(behind_hrtf, frames));
		CHECK(labs(ears_delay(above_hrtf, frames)) <= 1 && fabs(ears_level(above_hrtf, frames)) < 0.5 &&
			labs(ears_delay(below_hrtf, frames)) <= 1 && fabs(ears_level(below_hrtf, frames)) < 0.5,
			"straight above and below through the HRTF, the ears differ: %ld and %ld frames, %+.2f and %+.2f dB",
			ears_delay(above_hrtf, frames), ears_delay(below_hrtf, frames), ears_level(above_hrtf, frames),
			ears_level(below_hrtf, frames));
		for (ear = 0; ear < 2; ear++)
		{
			rear_db = ear_band(behind_hrtf, frames, ear, 15000.0) - ear_band(ahead_hrtf, frames, ear, 15000.0);
			low_db = ear_band(behind_hrtf, frames, ear, 300.0) - ear_band(ahead_hrtf, frames, ear, 300.0);
			CHECK(rear_db < -15.0 && fabs(low_db) < 1.0, "behind through the HRTF, 15 kHz %+.1f dB and 300 Hz %+.1f dB "
				"of ahead's (ear %d)", rear_db, low_db, ear);
			bright_db = ear_band(down_hrtf, frames, ear, 8000.0) - ear_band(up_hrtf, frames, ear, 8000.0);
			middle_db = ear_band(down_hrtf, frames, ear, 1400.0) - ear_band(up_hrtf, frames, ear, 1400.0);
			CHECK(bright_db > 3.0 && middle_db > 1.5, "40 degrees below against 40 above, through the HRTF: 8 kHz %+.1f "
				"dB, 1.4 kHz %+.1f dB (ear %d)", bright_db, middle_db, ear);
		}
		for (index = 0; index < frames * 2; index++)
		{
			CHECK(above_pan[index] == ahead_pan[index] && below_pan[index] == ahead_pan[index] &&
				behind_pan[index] == ahead_pan[index], "panned, above, below or behind differs from ahead at %lu",
				index);
		}
		free(left_hrtf);
		free(ahead_hrtf);
		free(behind_hrtf);
		free(above_hrtf);
		free(below_hrtf);
		free(up_hrtf);
		free(down_hrtf);
		free(left_pan);
		free(ahead_pan);
		free(behind_pan);
		free(above_pan);
		free(below_pan);
		return 0;
	}
	/* through the HRTF, a 3D voice keeps the pan's distance, the direct
	path's occlusion (its low pass), the room send and the Doppler shift:
	8 units away 12 dB under 2 (its minimum distance 1), as panned; the
	high frequencies of an occluded voice as far down as panned; the same
	send to the reverb; a 1 kHz sine coming at 10 units a second heard
	343 / (343 - 30.5) times as high */
	CASE("hrtf-paths")
	{
		unsigned long frames = 24000;
		short *samples = noise_samples(frames + 4096);
		float *output = malloc((frames + CHUNK) * 2 * sizeof(float));
		double near_db[2], far_db[2], open_db[2], muffled_db[2], send_power[2];
		int through;

		hrtf_initialize();
		listener_reset();
		for (through = 0; through < 2; through++)
		{
			struct sdl_stream *stream;
			static float buffer[CHUNK * 2], send[CHUNK];
			unsigned long chunk, n;

			hrtf_enabled = through;
			stream = voice_placed(samples, frames + 4096, 48000, -2.0f, 0.0f, 0.0f);
			render(stream, output, frames, TRUE);
			near_db[through] = ears_power(output + CHUNK * 2, frames - CHUNK);
			voice_delete(stream);
			stream = voice_placed(samples, frames + 4096, 48000, -8.0f, 0.0f, 0.0f);
			render(stream, output, frames, TRUE);
			far_db[through] = ears_power(output + CHUNK * 2, frames - CHUNK);
			open_db[through] = ear_band(output + CHUNK * 2, frames - CHUNK, 0, 12000.0) -
				ear_band(output + CHUNK * 2, frames - CHUNK, 0, 500.0);
			voice_delete(stream);
			stream = voice_placed(samples, frames + 4096, 48000, -8.0f, 0.0f, 0.0f);
			stream->direct_hf = -2000;
			render(stream, output, frames, TRUE);
			muffled_db[through] = ear_band(output + CHUNK * 2, frames - CHUNK, 0, 12000.0) -
				ear_band(output + CHUNK * 2, frames - CHUNK, 0, 500.0);
			voice_delete(stream);
			/* (the send, indoors) */
			environment.lRoom = -1000;
			stream = voice_placed(samples, frames + 4096, 48000, -3.0f, 0.0f, 1.0f);
			stream->room = 0;
			stream->room_hf = -300;
			send_power[through] = 0.0;
			for (chunk = 0; chunk < 20; chunk++)
			{
				memset(buffer, 0, sizeof(buffer));
				memset(send, 0, sizeof(send));
				mix_voice(stream, buffer, send, CHUNK);
				for (n = 0; n < CHUNK; n++)
					send_power[through] += (double)send[n] * send[n];
			}
			environment.lRoom = DSBVOLUME_MIN;
			voice_delete(stream);
		}
		hrtf_enabled = FALSE;
		for (through = 0; through < 2; through++)
		{
			CHECK(fabs(far_db[through] - near_db[through] + 12.04) < 0.3, "%s, 8 units away %+.2f dB of 2", through ?
				"through the HRTF" : "panned", far_db[through] - near_db[through]);
		}
		CHECK(fabs((muffled_db[1] - open_db[1]) - (muffled_db[0] - open_db[0])) < 1.0 && muffled_db[1] - open_db[1] < -6.0,
			"occluded, the high frequencies %+.1f dB through the HRTF, %+.1f dB panned", muffled_db[1] - open_db[1],
			muffled_db[0] - open_db[0]);
		CHECK(send_power[0] > 0.0 && send_power[1] == send_power[0], "the room send through the HRTF %g, panned %g",
			send_power[1], send_power[0]);
		free(output);
		free(samples);
		{
			float ratio;
			double rest_db;

			resampler_mode = RESAMPLER_SINC;
			hrtf_enabled = TRUE;
			doppler_figures(-10.0f, 0.0f, DS3DMODE_NORMAL, 1.0 + 30.48 / 343.0, &ratio, &rest_db);
			hrtf_enabled = FALSE;
			listener_reset();
			CHECK(fabs(ratio - (1.0 + 30.48 / 343.0)) < 1.0e-5 && rest_db < -40.0, "through the HRTF, coming at 10 "
				"units a second: shifted %.5f, the rest %.1f dB", ratio, rest_db);
		}
		return 0;
	}
	/* the choice changing as a 3D voice plays (headphones plugged in, then
	out): a 100 Hz sine to the left crossfades from the pan to the HRTF and
	back, with no step: no frame moves further from the last than the
	sine does at the louder of the two (no click) */
	CASE("hrtf-switch")
	{
		unsigned long frames = 48000 * 3, n, chunk;
		short *samples = malloc((frames + 4096) * sizeof(short));
		/* (whole mixes of CHUNK: the last runs past frames) */
		float *output = malloc((frames + CHUNK) * 2 * sizeof(float));
		struct sdl_stream *stream;
		double steady[2] = { 0.0, 0.0 }, switching[2] = { 0.0, 0.0 };
		int ear;

		for (n = 0; n < frames + 4096; n++)
			samples[n] = (short)lround(16000.0 * sin(2.0 * PI * 100.0 * n / 48000.0));
		hrtf_initialize();
		listener_reset();
		stream = voice_placed(samples, frames + 4096, 48000, -3.0f, 0.0f, 0.5f);
		/* (each switch a quarter of a cycle past a zero crossing) */
		for (chunk = 0; chunk * CHUNK < frames; chunk++)
		{
			static float buffer[CHUNK * 2], send[CHUNK];

			hrtf_enabled = chunk >= 94 && chunk < 188;
			memset(buffer, 0, sizeof(buffer));
			memset(send, 0, sizeof(send));
			mix_voice(stream, buffer, send, CHUNK);
			memcpy(output + chunk * CHUNK * 2, buffer, sizeof(buffer));
		}
		hrtf_enabled = FALSE;
		for (n = CHUNK + 1; n < frames; n++)
		{
			/* (the switches and their crossfades, and where each is still) */
			BOOL near_switch = (n >= 94 * CHUNK - 16 && n < 94 * CHUNK + HRTF_FADE_FRAMES + 2 * CHUNK) ||
				(n >= 188 * CHUNK - 16 && n < 188 * CHUNK + HRTF_FADE_FRAMES + 2 * CHUNK);

			for (ear = 0; ear < 2; ear++)
			{
				double step = fabs(output[n * 2 + ear] - output[(n - 1) * 2 + ear]);

				if (near_switch)
					switching[ear] = step > switching[ear] ? step : switching[ear];
				else
					steady[ear] = step > steady[ear] ? step : steady[ear];
			}
		}
		for (ear = 0; ear < 2; ear++)
		{
			CHECK(steady[ear] > 0.0 && switching[ear] < 1.05 * steady[ear], "the %s ear: a step of %.5f while the choice "
				"changes, %.5f at most otherwise (a click)", ear ? "right" : "left", switching[ear], steady[ear]);
		}
		voice_delete(stream);
		free(output);
		free(samples);
		return 0;
	}
	/* 2D voices (the menus, the music, the HUD), stereo voices placed in
	the world and 3D voices with 3D off are as they were through the HRTF:
	the same to the bit; a mono 3D voice is not (the HRTF is on) */
	CASE("dry-2d")
	{
		unsigned long frames = 24000, kind;
		short *samples = noise_samples(frames * 2 + 4096);
		float *panned = malloc(frames * 2 * sizeof(float)), *through = malloc(frames * 2 * sizeof(float));

		hrtf_initialize();
		listener_reset();
		for (kind = 0; kind < 5; kind++)
		{
			int pass;
			BOOL same = TRUE;

			for (pass = 0; pass < 2; pass++)
			{
				struct sdl_stream *stream;

				if (kind == 0)
				{
					/* a 2D stereo voice (music) */
					stream = voice_new(samples, frames + 2048, 2, 44100, 0);
				}
				else if (kind == 1)
				{
					/* a 2D mono voice (the HUD's, a menu's) */
					stream = voice_new(samples, frames + 2048, 1, 22050, 0);
					stream->mix_left = 0.8f;
					stream->mix_right = 0.5f;
				}
				else if (kind == 2)
				{
					/* a stereo voice placed in the world */
					stream = voice_new(samples, frames + 2048, 2, 22050, 0);
					stream->stereo_positioned = TRUE;
					stream->stereo_pan = -0.6f;
				}
				else if (kind == 3)
				{
					/* a 3D voice with 3D off */
					stream = voice_placed(samples, frames + 2048, 22050, -3.0f, 1.0f, 0.0f);
					stream->mode = DS3DMODE_DISABLE;
				}
				else
				{
					/* a mono 3D voice: the HRTF's */
					stream = voice_placed(samples, frames + 2048, 22050, -3.0f, 1.0f, 0.0f);
				}
				hrtf_enabled = pass;
				render(stream, pass ? through : panned, frames, TRUE);
				hrtf_enabled = FALSE;
				voice_delete(stream);
			}
			for (index = 0; index < frames * 2; index++)
				same = same && through[index] == panned[index];
			CHECK(same == (kind != 4), "voice %lu (0 2D stereo, 1 2D mono, 2 stereo placed, 3 3D off, 4 mono 3D) "
				"%s through the HRTF", kind, same ? "the same" : "changed");
		}
		free(samples);
		free(panned);
		free(through);
		return 0;
	}
	/* audio.volume and a voice's own volume (SetVolume) scale every voice
	alike, the 3D voices through the HRTF as well as the 2D ones: each
	voice at a fifth of both is 2 x 13.98 dB down */
	/* Settings > Audio's SOUND (audio.enabled) as the game plays: off fades
	the output to silence over SOUND_FADE_FRAMES and keeps it silent, on
	fades it back and then leaves it as mixed; a device is opened for it
	once a change of the settings turns it on with none open */

	CASE("master-volume")
	{
		unsigned long frames = 24000, kind;
		short *samples = noise_samples(frames * 2 + 4096);
		float *output = malloc(frames * 2 * sizeof(float));

		hrtf_initialize();
		listener_reset();
		for (kind = 0; kind < 5; kind++)
		{
			double power_db[2];
			int pass;

			for (pass = 0; pass < 2; pass++)
			{
				struct sdl_stream *stream;

				if (kind == 0)
					stream = voice_new(samples, frames + 2048, 2, 44100, 0);
				else if (kind == 1)
					stream = voice_new(samples, frames + 2048, 1, 22050, 0);
				else if (kind == 2)
				{
					stream = voice_new(samples, frames + 2048, 2, 22050, 0);
					stream->stereo_positioned = TRUE;
					stream->stereo_pan = -0.6f;
				}
				else
					stream = voice_placed(samples, frames + 2048, 22050, -3.0f, 1.0f, 0.0f);
				hrtf_enabled = kind == 4;
				master_volume = pass ? 0.2f : 1.0f;
				stream->volume = pass ? 0.2f : 1.0f;
				render(stream, output, frames, TRUE);
				power_db[pass] = ears_power(output + CHUNK * 2, frames - CHUNK);
				voice_delete(stream);
			}
			hrtf_enabled = FALSE;
			master_volume = 1.0f;
			CHECK(fabs(power_db[1] - power_db[0] + 27.96) < 0.05, "voice %lu (0 2D stereo, 1 2D mono, 2 stereo placed, "
				"3 3D panned, 4 3D through the HRTF) at a fifth of the master volume and its own: %+.2f dB", kind,
				power_db[1] - power_db[0]);
		}
		free(samples);
		free(output);
		return 0;
	}

	/* "circle <file.wav> hrtf|pan": a sound going round the listener, to
	listen to (circle) */
	CASE("circle")
	{
		if (argc < 4)
		{
			fprintf(stderr, "circle <file.wav> hrtf|pan\n");
			return 2;
		}
		return circle(argv[2], !strcmp(argv[3], "hrtf"));
	}

	/* a voice's changes take the same frames however many a mix has (an
	audio device asks for its own, SDL's resampling for any): each scene of
	changes (block_render) mixed in calls of 64 to 4096 frames, irregular
	ones and single frames, is the one mixed 480 at a time to float's
	rounding, in every output channel and the room send. Before, a change
	ramped over a call of fewer than 64 frames, and the HRTF's over the whole
	call (8 ms at 480 frames, where an Xbox's settled within 1 ms) */
	CASE("block-size")
	{
		static double differences[BLOCK_SCENES][BLOCK_PARTITIONS][2];
		int scene, partition;

		block_figures(differences);
		for (scene = 0; scene < BLOCK_SCENES; scene++)
		{
			for (partition = 0; partition < BLOCK_PARTITIONS; partition++)
			{
				CHECK(differences[scene][partition][0] < -100.0 && differences[scene][partition][1] < -100.0,
					"%s mixed %s frames a call: %.1f dB from 480 a call (the room send %.1f dB)",
					block_scene_names[scene], block_partition_names[partition], differences[scene][partition][0],
					differences[scene][partition][1]);
			}
		}
		return 0;
	}
	/* "block-figures": block-size's differences, each scene's against 480
	frames a call, in dB of its largest frame */
	CASE("block-figures")
	{
		static double differences[BLOCK_SCENES][BLOCK_PARTITIONS][2];
		int scene, partition;

		block_figures(differences);
		printf("%-20s", "dB from 480");
		for (partition = 0; partition < BLOCK_PARTITIONS; partition++)
			printf(" %9s", block_partition_names[partition][0] == 'i' ? (partition == 5 ? "irreg" : "irr-small") :
				block_partition_names[partition]);
		printf("\n");
		for (scene = 0; scene < BLOCK_SCENES; scene++)
		{
			printf("%-20s", block_scene_names[scene]);
			for (partition = 0; partition < BLOCK_PARTITIONS; partition++)
				printf(" %9.1f", differences[scene][partition][0]);
			printf("\n");
			if (scene == 3)
			{
				printf("%-20s", "  (its send)");
				for (partition = 0; partition < BLOCK_PARTITIONS; partition++)
					printf(" %9.1f", differences[scene][partition][1]);
				printf("\n");
			}
		}
		return 0;
	}
	/* "bench [voices] [out of earshot] [indoor|outdoor] [hrtf]": through the
	HRTF, the 3D voices' */
	CASE("bench")
	{
		hrtf_initialize();
		hrtf_enabled = argc > 5 && !strcmp(argv[5], "hrtf");
		resampler_mode = RESAMPLER_XBOX;
		output_channels = argc > 6 && !strcmp(argv[6], "5.1") ? 6 : 2;
		bench(argc > 2 ? strtoul(argv[2], NULL, 0) : 24, argc > 3 ? strtoul(argv[3], NULL, 0) : 16,
			argc > 4 && !strcmp(argv[4], "indoor"), 10.0);
		return 0;
	}
	fprintf(stderr, "no case %s\n", case_name);
	return 2;
}
