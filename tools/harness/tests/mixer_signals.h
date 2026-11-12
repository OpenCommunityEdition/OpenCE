/* Shared voice rendering and signal measurements for mixer tests.
 * Adapted in part from OpenH1/OpenH1 6750db5a6c9716857bd1e63d9ec188a20c49f1d3.
 * Copyright OpenH1 contributors; AGPL-3.0-or-later. */

static unsigned long random_state = 1;

static void voice_delete(struct sdl_stream *stream)
{
	free(stream->packets[0].samples);
	free(stream);
}

/* frames of the voice mixed alone, CHUNK at a time: the left channel, or
both (stereo: interleaved) */
static void render(struct sdl_stream *stream, float *output, unsigned long frames, BOOL stereo)
{
	static float buffer[CHUNK * 2], send[CHUNK];
	unsigned long done, frame;

	for (done = 0; done < frames; done += CHUNK)
	{
		unsigned long count = frames - done < CHUNK ? frames - done : CHUNK;

		memset(buffer, 0, sizeof(buffer));
		memset(send, 0, sizeof(send));
		mix_voice(stream, buffer, send, count);
		for (frame = 0; frame < count; frame++)
		{
			if (stereo)
			{
				output[(done + frame) * 2] = buffer[frame * 2];
				output[(done + frame) * 2 + 1] = buffer[frame * 2 + 1];
			}
			else
			{
				output[done + frame] = buffer[frame * 2];
			}
		}
	}
}

static double random_uniform(void)
{
	random_state = random_state * 1103515245UL + 12345UL;
	return (double)((random_state >> 8) & 0xffffff) / 16777216.0 * 2.0 - 1.0;
}

/* frames of a signal as 16-bit samples (and as the doubles they are) */
static void quantize(const double *signal, short *samples, double *exact, unsigned long frames)
{
	unsigned long frame;

	for (frame = 0; frame < frames; frame++)
	{
		long value = lround(signal[frame] * 32767.0);

		samples[frame] = (short)(value > 32767 ? 32767 : value < -32768 ? -32768 : value);
		exact[frame] = samples[frame] / 32768.0;
	}
}

/* the amplitude of a sine of frequency cycles a frame in signal[first, last),
and the power of the rest relative to the sine's, in dB */
static void tone_fit(const float *signal, unsigned long first, unsigned long last, double frequency,
	double *amplitude, double *rest_db)
{
	double ss = 0, sc = 0, cc = 0, ys = 0, yc = 0, a, b, determinant, rest = 0, tone = 0;
	unsigned long n;

	for (n = first; n < last; n++)
	{
		double s = sin(2.0 * PI * frequency * n), c = cos(2.0 * PI * frequency * n);

		ss += s * s;
		sc += s * c;
		cc += c * c;
		ys += signal[n] * s;
		yc += signal[n] * c;
	}
	determinant = ss * cc - sc * sc;
	a = (ys * cc - yc * sc) / determinant;
	b = (yc * ss - ys * sc) / determinant;
	for (n = first; n < last; n++)
	{
		double fit = a * sin(2.0 * PI * frequency * n) + b * cos(2.0 * PI * frequency * n);

		rest += (signal[n] - fit) * (signal[n] - fit);
		tone += fit * fit;
	}
	*amplitude = sqrt(a * a + b * b);
	*rest_db = 10.0 * log10(rest / tone + 1e-30);
}

static double decibels(double ratio)
{
	return 20.0 * log10(ratio + 1e-30);
}

static double *noise(unsigned long frames)
{
	double *signal = malloc(frames * sizeof(double));
	unsigned long n;

	random_state = 1;
	for (n = 0; n < frames; n++)
		signal[n] = 0.5 * random_uniform();
	return signal;
}

/* the scene's voices out of earshot come into it */
static void scene_approach(void)
{
	struct sdl_stream *stream;

	for (stream = streams; stream; stream = stream->next)
	{
		if (stream->direct == DSBVOLUME_MIN)
		{
			stream->position[2] = 3.0f;
			stream->direct = stream->room = 0;
		}
	}
}
