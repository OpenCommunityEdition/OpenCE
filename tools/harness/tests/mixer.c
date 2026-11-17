/*
MIXER.C (test)

The real mixer of port/linux/src/dsound_sdl.c (its resampler, voices,
reverb and limiter; test_mixer.py takes them as under_test.inc) on known
signals:
	- the resampler against the same Kaiser-windowed sinc worked out in
	  double precision, tap by tap: noise, sines and an impulse at the
	  game's rates (22 and 44 kHz voices, others pitched up and down, a 48
	  kHz one), so a faster resampler is still the same low pass;
	- sines through it measured as the ear would hear them: the gain across
	  the band, and what else comes out (images and aliases) under the tone;
	- a voice out of earshot, which is not resampled, moves on as one heard
	  does, and is heard the same once it comes into earshot;
	- mono voices heard as stereo ones with both channels the same are;
	- audio.resampler = "linear": each frame between the two around it;
	- overlapping cut fades in stereo and 5.1, and cancellation while the
	  audio thread mixes during a packet completion callback.
"render" writes voices at each rate and a scene's whole mix as raw floats
(test_mixer.py compares the resampler adding four taps at a time with it
adding tap by tap), "metrics" prints the figures and "bench" times a game's
worth of voices.
*/

#include "mixer_fixture.h"

#define PI 3.14159265358979323846
#define CHUNK 512 /* frames a mix, as SDL asks on Linux */

/* ---------- voices */

/* a voice of frames frames of channels channels at rate, played at frequency
(0: its rate), a 2D voice heard at full volume in both speakers */
static struct sdl_stream *voice_new(const short *samples, unsigned long frames, unsigned long channels,
	unsigned long rate, unsigned long frequency)
{
	struct sdl_stream *stream = calloc(1, sizeof(*stream));

	stream->cut_played = CUT_FADE_FRAMES;
	stream->reference_count = 1;
	stream->channels = channels;
	stream->sample_rate = rate;
	stream->frequency = frequency;
	stream->volume = 1.0f;
	stream->mix_left = stream->mix_right = 1.0f;
	stream->surround_bins[0][0] = 1.0f;
	stream->surround_bins[channels - 1][1] = 1.0f;
	stream->room = stream->room_hf = DSBVOLUME_MIN;
	stream->packets[0].samples = malloc(frames * channels * sizeof(short));
	memcpy(stream->packets[0].samples, samples, frames * channels * sizeof(short));
	stream->packets[0].frames = frames;
	stream->packet_count = 1;
	resampler_reset(stream);
	return stream;
}

#include "mixer_signals.h"

/* ---------- the reference: the same low pass in double precision */

static double reference_kernel(double distance)
{
	double edge, angle, sinc;

	distance = fabs(distance);
	if (distance >= RESAMPLER_ZERO_CROSSINGS)
		return 0.0;
	edge = distance / RESAMPLER_ZERO_CROSSINGS;
	angle = PI * RESAMPLER_CUTOFF * distance;
	sinc = distance > 0.0 ? sin(angle) / angle : 1.0;
	return RESAMPLER_CUTOFF * sinc * bessel_i0(RESAMPLER_KAISER_BETA * sqrt(1.0 - edge * edge)) /
		bessel_i0(RESAMPLER_KAISER_BETA);
}

/* output frame n of source (frames, mono, full scale 1) resampled at step:
the frames around source time n step, weighted by the low pass (narrowed for a
step over 1, as the mixer's) */
static double reference_frame(const double *source, long frames, double step, unsigned long n)
{
	double scale = step > 1.0 ? 1.0 / (step < RESAMPLER_MAXIMUM_STRETCH ? step : RESAMPLER_MAXIMUM_STRETCH) : 1.0;
	double time = (double)n * step, sum = 0.0;
	long center = (long)floor(time), width = (long)ceil(RESAMPLER_ZERO_CROSSINGS / scale), k;

	for (k = center + 1 - width; k <= center + width; k++)
	{
		if (k >= 0 && k < frames)
			sum += source[k] * scale * reference_kernel((time - (double)k) * scale);
	}
	return sum;
}

/* ---------- signals */

/* the mixer's output against the reference's for a signal at rate played at
frequency: the error's power relative to the reference's, in dB, and its
largest relative to the largest sample */
static void against_reference(const double *signal, unsigned long frames, unsigned long rate, unsigned long frequency,
	double *error_db, double *peak_error_db)
{
	short *samples = malloc(frames * sizeof(short));
	double *exact = malloc(frames * sizeof(double));
	double step = (double)(frequency ? frequency : rate) / OUTPUT_RATE, error = 0, power = 0, peak = 0, worst = 0;
	unsigned long outputs = (unsigned long)(frames / step), n;
	float *output = calloc(outputs, sizeof(float));
	struct sdl_stream *stream;

	quantize(signal, samples, exact, frames);
	stream = voice_new(samples, frames, 1, rate, frequency);
	render(stream, output, outputs, FALSE);
	for (n = 0; n < outputs; n++)
	{
		double expected = reference_frame(exact, (long)frames, step, n);
		double difference = output[n] - expected;

		error += difference * difference;
		power += expected * expected;
		peak = fabs(expected) > peak ? fabs(expected) : peak;
		worst = fabs(difference) > worst ? fabs(difference) : worst;
	}
	*error_db = 10.0 * log10(error / power + 1e-30);
	*peak_error_db = decibels(worst / peak);
	voice_delete(stream);
	free(output);
	free(exact);
	free(samples);
}

/* a sine of hertz at rate, played at frequency: its gain through the mixer
(dB) and the power of everything else that comes out, relative to it (dB); a
sine the voice plays above the output's band is all "else", relative to the
sine put in */
static void sine_figures(double hertz, unsigned long rate, unsigned long frequency, double *gain_db, double *rest_db)
{
	unsigned long frames = rate, outputs, n;
	double step = (double)(frequency ? frequency : rate) / OUTPUT_RATE, played = hertz * step * OUTPUT_RATE / rate;
	double *signal = malloc(frames * sizeof(double)), *exact = malloc(frames * sizeof(double)), amplitude;
	short *samples = malloc(frames * sizeof(short));
	float *output;
	struct sdl_stream *stream;

	for (n = 0; n < frames; n++)
		signal[n] = 0.5 * sin(2.0 * PI * hertz * n / rate);
	quantize(signal, samples, exact, frames);
	outputs = (unsigned long)(frames / step);
	output = calloc(outputs, sizeof(float));
	stream = voice_new(samples, frames, 1, rate, frequency);
	render(stream, output, outputs, FALSE);
	if (played < 0.5 * OUTPUT_RATE)
	{
		tone_fit(output, outputs / 4, outputs * 3 / 4, played / OUTPUT_RATE, &amplitude, rest_db);
		*gain_db = decibels(amplitude / 0.5);
	}
	else
	{
		double power = 0;

		for (n = outputs / 4; n < outputs * 3 / 4; n++)
			power += (double)output[n] * output[n];
		*gain_db = -999.0;
		*rest_db = 10.0 * log10(power / (outputs / 2) / (0.5 * 0.5 / 2) + 1e-30);
	}
	voice_delete(stream);
	free(output);
	free(samples);
	free(exact);
	free(signal);
}

/* a logarithmic sweep from 20 Hz to top at rate over the voice: what comes out
against the sweep itself at the output's times (an ideal resampler), in dB */
static double sweep_figure(unsigned long rate, double top)
{
	unsigned long frames = 2 * rate, outputs = (unsigned long)((double)frames * OUTPUT_RATE / rate), n;
	double *signal = malloc(frames * sizeof(double)), *exact = malloc(frames * sizeof(double));
	double duration = (double)frames / rate, k = log(top / 20.0), error = 0, power = 0;
	short *samples = malloc(frames * sizeof(short));
	float *output = calloc(outputs, sizeof(float));
	struct sdl_stream *stream;

	for (n = 0; n < frames; n++)
	{
		double t = (double)n / rate;

		signal[n] = 0.5 * sin(2.0 * PI * 20.0 * duration / k * (exp(t / duration * k) - 1.0));
	}
	quantize(signal, samples, exact, frames);
	stream = voice_new(samples, frames, 1, rate, 0);
	render(stream, output, outputs, FALSE);
	for (n = OUTPUT_RATE / 10; n + OUTPUT_RATE / 10 < outputs; n++)
	{
		double t = (double)n / OUTPUT_RATE;
		double ideal = 0.5 * sin(2.0 * PI * 20.0 * duration / k * (exp(t / duration * k) - 1.0));

		error += (output[n] - ideal) * (output[n] - ideal);
		power += ideal * ideal;
	}
	voice_delete(stream);
	free(output);
	free(samples);
	free(exact);
	free(signal);
	return 10.0 * log10(error / power + 1e-30);
}

/* ---------- a scene: a game's worth of voices, for "render" and "bench" */

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
		stream->next = streams;
		streams = stream;
	}
	free(mono);
	free(stereo);
}

/* ---------- 3D voices: Doppler */

/* a 3D voice of a 1 kHz sine at 48 kHz (as long as four times up needs),
distance units straight ahead of the listener, heard whole and sending to
the room at full level */
static struct sdl_stream *voice_ahead(float distance)
{
	unsigned long frames = 100000, n;
	double *signal = malloc(frames * sizeof(double)), *exact = malloc(frames * sizeof(double));
	short *samples = malloc(frames * sizeof(short));
	struct sdl_stream *stream;

	for (n = 0; n < frames; n++)
		signal[n] = 0.5 * sin(2.0 * PI * 1000.0 * n / 48000.0);
	quantize(signal, samples, exact, frames);
	stream = voice_new(samples, frames, 1, 48000, 0);
	stream->has_3d = TRUE;
	stream->mode = DS3DMODE_NORMAL;
	stream->position[2] = distance;
	stream->minimum_distance = 1.0f;
	stream->maximum_distance = 1000.0f;
	stream->room = stream->room_hf = 0;
	free(signal);
	free(exact);
	free(samples);
	return stream;
}

/* the frequency of signal[first, last) in hertz at the output rate, from its
rising zero crossings (each placed between its two frames) */
static double measured_frequency(const float *signal, unsigned long first, unsigned long last)
{
	double first_crossing = -1.0, last_crossing = 0.0;
	unsigned long n, cycles = 0;

	for (n = first; n + 1 < last; n++)
	{
		if (signal[n] <= 0.0f && signal[n + 1] > 0.0f)
		{
			double crossing = (double)n + signal[n] / (signal[n] - signal[n + 1]);

			if (first_crossing < 0.0)
				first_crossing = crossing;
			else
				cycles++;
			last_crossing = crossing;
		}
	}
	return cycles ? cycles * (double)OUTPUT_RATE / (last_crossing - first_crossing) : 0.0;
}

#define FIGURE_FRAMES 20000

/* the voice mixed alone, CHUNK at a time, past its first mixes: the left
channel's frequency and the sine's amplitude there, and the room send's
level */
static void voice_figures(struct sdl_stream *stream, double *frequency, double *level, double *room_level)
{
	static float output[FIGURE_FRAMES], send_out[FIGURE_FRAMES], buffer[CHUNK * 2], send[CHUNK];
	unsigned long done, frame;
	double rest_db, sum = 0.0;

	for (done = 0; done < FIGURE_FRAMES; done += CHUNK)
	{
		unsigned long count = FIGURE_FRAMES - done < CHUNK ? FIGURE_FRAMES - done : CHUNK;

		memset(buffer, 0, sizeof(buffer));
		memset(send, 0, sizeof(send));
		mix_voice(stream, buffer, send, count);
		for (frame = 0; frame < count; frame++)
		{
			output[done + frame] = buffer[frame * 2];
			send_out[done + frame] = send[frame];
		}
	}
	*frequency = measured_frequency(output, 4000, FIGURE_FRAMES);
	tone_fit(output, 4000, FIGURE_FRAMES, *frequency / OUTPUT_RATE, level, &rest_db);
	for (frame = 4000; frame < FIGURE_FRAMES; frame++)
		sum += (double)send_out[frame] * send_out[frame];
	*room_level = sqrt(sum / (FIGURE_FRAMES - 4000));
}

static void bench(unsigned long voices, unsigned long silent, BOOL indoor, double seconds)
{
	unsigned long chunks = (unsigned long)(seconds * OUTPUT_RATE / CHUNK), chunk;
	static float output[CHUNK * 2];
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

/* Run the audio worker while a real flush completion callback has dropped
the mixer lock. The handshake makes the problematic interleaving repeatable. */
static pthread_mutex_t flush_test_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t flush_test_done = PTHREAD_COND_INITIALIZER;
static struct sdl_stream *flush_test_stream;
static float flush_output[2 * 100 * MAXIMUM_OUTPUT_CHANNELS], flush_send[2 * 100];
static unsigned long flush_callbacks;
static BOOL flush_worker_done;

static void *flush_mix_worker(void *unused)
{
	unsigned long offset = flush_callbacks * 100;

	(void)unused;
	pthread_mutex_lock(&mixer_lock);
	mix_voice(flush_test_stream, flush_output + offset * output_channels, flush_send + offset, 100);
	mix_cut(flush_test_stream, flush_output + offset * output_channels, flush_send + offset, 100);
	pthread_mutex_unlock(&mixer_lock);
	pthread_mutex_lock(&flush_test_lock);
	flush_worker_done = TRUE;
	pthread_cond_signal(&flush_test_done);
	pthread_mutex_unlock(&flush_test_lock);
	return NULL;
}

static void flush_test_callback(void *context, void *packet, DWORD status)
{
	pthread_t worker;

	CHECK(context == flush_test_stream, "wrong stream context");
	CHECK(packet == flush_test_stream, "wrong packet context");
	CHECK(status == XMEDIAPACKET_STATUS_FLUSHED, "flush completed with %lu", status);
	CHECK(flush_callbacks < 2, "duplicate completion callback");
	/* A repeated flush from a completion callback must not restart cancellation. */
	stream_flush(&flush_test_stream->object);
	pthread_mutex_lock(&flush_test_lock);
	flush_worker_done = FALSE;
	CHECK(!pthread_create(&worker, NULL, flush_mix_worker, NULL), "cannot create mixer worker");
	pthread_detach(worker);
	while (!flush_worker_done)
		pthread_cond_wait(&flush_test_done, &flush_test_lock);
	pthread_mutex_unlock(&flush_test_lock);
	flush_callbacks++;
}

/* Exercise mono/stereo sources and both output layouts without game assets. */
static void test_cut_overlap(void)
{
	static short samples[16000];
	static float scratch[MIX_CHUNK_FRAMES * MAXIMUM_OUTPUT_CHANNELS], send[MIX_CHUNK_FRAMES];
	static float old_cut[CUT_FADE_FRAMES * MAXIMUM_OUTPUT_CHANNELS], old_send[CUT_FADE_FRAMES];
	static float next[CUT_FADE_FRAMES * MAXIMUM_OUTPUT_CHANNELS], next_send[CUT_FADE_FRAMES];
	unsigned long layout, channels, mode, spatial, n, frame, speaker;

	for (n = 0; n < 16000; n++)
		samples[n] = (short)(12000 * sin(2.0 * PI * n / 93.0));
	for (layout = 2; layout <= 6; layout += 4)
	for (channels = 1; channels <= 2; channels++)
	for (mode = RESAMPLER_XBOX; mode <= RESAMPLER_LINEAR; mode++)
	for (spatial = 0; spatial < 2; spatial++)
	{
		struct sdl_stream *stream = voice_new(samples, 8000, channels, 44100, 0), reference;

		output_channels = layout;
		hrtf_enabled = spatial;
		resampler_mode = mode;
		stream->has_3d = TRUE;
		stream->position[2] = 2.0f;
		stream->minimum_distance = 1.0f;
		stream->maximum_distance = 100.0f;
		stream->room = 0;
		stream->room_hf = 0;
		memset(scratch, 0, sizeof(scratch));
		memset(send, 0, sizeof(send));
		mix_voice(stream, scratch, send, 512);
		voice_cut(stream);
		memcpy(old_cut, stream->cut, sizeof(old_cut));
		memcpy(old_send, stream->cut_send, sizeof(old_send));
		mix_cut(stream, scratch, send, 100);
		CHECK(stream->cut_played == 100, "first fade already drained");
		/* The next voice on this channel has started before it is cut too. */
		stream->cursor = 0;
		resampler_reset(stream);
		stream->gains_valid = FALSE;
		mix_voice(stream, scratch, send, 100);
		reference = *stream;
		memset(next, 0, sizeof(next));
		memset(next_send, 0, sizeof(next_send));
		mix_voice(&reference, next, next_send, CUT_FADE_FRAMES);
		voice_cut(stream);
		for (frame = 0; frame < CUT_FADE_FRAMES; frame++)
		{
			float fade = 0.5f + 0.5f * (float)cos(3.14159265358979 * (double)(frame + 1) / CUT_FADE_FRAMES);
			float expected_send = next_send[frame] * fade +
				(frame < CUT_FADE_FRAMES - 100 ? old_send[frame + 100] : 0.0f);
			CHECK(fabsf(stream->cut_send[frame] - expected_send) < 1e-6f, "lost overlapping room send");
			for (speaker = 0; speaker < layout; speaker++)
			{
				float expected = next[frame * layout + speaker] * fade +
					(frame < CUT_FADE_FRAMES - 100 ? old_cut[(frame + 100) * layout + speaker] : 0.0f);
				CHECK(fabsf(stream->cut[frame * layout + speaker] - expected) < 1e-6f,
					"lost overlapping output: layout %lu, channel %lu, frame %lu", layout, speaker, frame);
			}
		}
		memset(scratch, 0, sizeof(scratch));
		memset(send, 0, sizeof(send));
		mix_cut(stream, scratch, send, 100);
		mix_cut(stream, scratch + 100 * layout, send + 100, 200);
		for (frame = 0; frame < 300; frame++)
		{
			CHECK(send[frame] == (frame < CUT_FADE_FRAMES ? stream->cut_send[frame] : 0.0f),
				"cut send did not drain exactly");
			for (speaker = 0; speaker < layout; speaker++)
				CHECK(scratch[frame * layout + speaker] ==
					(frame < CUT_FADE_FRAMES ? stream->cut[frame * layout + speaker] : 0.0f),
					"cut output did not drain exactly");
		}
		voice_delete(stream);
	}
}

static void test_flush(void)
{
	static short samples[20000];
	static float warm[512 * MAXIMUM_OUTPUT_CHANNELS], send[512];
	static float expected[CUT_FADE_FRAMES * MAXIMUM_OUTPUT_CHANNELS], expected_send[CUT_FADE_FRAMES];
	unsigned long layout, n, frame;
	DWORD status, completed;
	XMEDIAPACKET packet;

	for (n = 0; n < 20000; n++)
		samples[n] = 16384;
	for (layout = 2; layout <= 6; layout += 4)
	{
		struct sdl_stream *stream = voice_new(samples, 10000, 1, 48000, 0), reference;

		output_channels = layout;
		stream->has_3d = TRUE;
		stream->position[2] = 2.0f;
		stream->minimum_distance = 1.0f;
		stream->maximum_distance = 100.0f;
		stream->room = stream->room_hf = 0;
		stream->packets[0].packet.pContext = stream;
		stream->packets[0].packet.dwMaxSize = 20000;
		memset(&packet, 0, sizeof(packet));
		packet.pvBuffer = samples;
		packet.dwMaxSize = 20000;
		packet.pdwStatus = &status;
		packet.pdwCompletedSize = &completed;
		packet.pContext = stream;
		CHECK(stream_process(&stream->object, &packet, NULL) == S_OK, "cannot queue second packet");
		memset(warm, 0, sizeof(warm));
		memset(send, 0, sizeof(send));
		mix_voice(stream, warm, send, 512);
		reference = *stream;
		voice_cut(&reference);
		memcpy(expected, reference.cut, sizeof(expected));
		memcpy(expected_send, reference.cut_send, sizeof(expected_send));
		flush_test_stream = stream;
		flush_callbacks = 0;
		memset(flush_output, 0, sizeof(flush_output));
		memset(flush_send, 0, sizeof(flush_send));
		stream->context = stream;
		stream->callback = flush_test_callback;
		CHECK(stream_flush(&stream->object) == S_OK, "flush failed");
		CHECK(flush_callbacks == 2 && status == XMEDIAPACKET_STATUS_FLUSHED && completed == 0,
			"packets were not synchronously cancelled");
		CHECK(!stream->packet_count && !stream->packets[0].samples && !stream->packets[1].samples,
			"packet samples retained after flush");
		for (frame = 0; frame < 200; frame++)
		{
			CHECK(fabsf(flush_send[frame] - expected_send[frame]) < 1e-6f, "live room send during flush");
			for (n = 0; n < layout; n++)
				CHECK(fabsf(flush_output[frame * layout + n] - expected[frame * layout + n]) < 1e-6f,
					"live voice mixed over fade during flush at frame %lu", frame);
		}
		/* The empty second flush must not replay or discard the remaining fade. */
		stream_flush(&stream->object);
		memset(warm, 0, sizeof(warm));
		memset(send, 0, sizeof(send));
		mix_cut(stream, warm, send, 512);
		for (frame = 0; frame < 512; frame++)
		{
			CHECK(send[frame] == (frame < 40 ? expected_send[frame + 200] : 0.0f), "wrong final room tail");
			for (n = 0; n < layout; n++)
				CHECK(warm[frame * layout + n] ==
					(frame < 40 ? expected[(frame + 200) * layout + n] : 0.0f), "wrong final output tail");
		}
		stream->callback = NULL;
		CHECK(stream_process(&stream->object, &packet, NULL) == S_OK, "cannot restart after flush");
		memset(warm, 0, sizeof(warm));
		mix_voice(stream, warm, send, 512);
		CHECK(fabsf(warm[100 * layout]) > 0.01f, "flush left the stream muted");
		streams = stream;
		CHECK(stream_release(&stream->object) == 0 && !streams, "released stream retained");
	}
}

static void test_cut_lifecycle(void)
{
	short samples[1000];
	float output[512 * MAXIMUM_OUTPUT_CHANNELS], send[512];
	struct sdl_stream *stream;
	unsigned long n;

	for (n = 0; n < 1000; n++)
		samples[n] = 16384;
	stream = voice_new(samples, 1000, 1, 48000, 0);
	voice_cut(stream);
	CHECK(stream->cut_played == CUT_FADE_FRAMES, "unplayed voice acquired a fade");
	memset(output, 0, sizeof(output));
	memset(send, 0, sizeof(send));
	mix_voice(stream, output, send, 512);
	IDirectSoundStream_Pause(&stream->object, DSSTREAMPAUSE_PAUSE);
	voice_cut(stream);
	CHECK(stream->cut_played == CUT_FADE_FRAMES, "paused voice acquired a fade");
	stream_flush(&stream->object);
	CHECK(stream->paused && !stream->packet_count, "flush changed pause state");
	IDirectSoundStream_Pause(&stream->object, DSSTREAMPAUSE_RESUME);
	CHECK(!stream->paused, "resume failed");
	voice_cut(stream);
	CHECK(stream->cut_played == CUT_FADE_FRAMES, "empty stream acquired a fade");
	streams = stream;
	stream_release(&stream->object);
	CHECK(!streams, "released paused stream retained");
	/* A short voice ends before 5 ms; capture must not read beyond its packet. */
	stream = voice_new(samples, 40, 1, 48000, 0);
	mix_voice(stream, output, send, 10);
	voice_cut(stream);
	memset(output, 0, sizeof(output));
	memset(send, 0, sizeof(send));
	mix_cut(stream, output, send, 512);
	for (n = CUT_FADE_FRAMES; n < 512; n++)
		CHECK(output[n * 2] == 0.0f && send[n] == 0.0f, "short packet leaked beyond its fade");
	voice_delete(stream);
}

/* Reusing a stream after Flush must not carry the previous sound's ear
history into its replacement. Compare with a fresh voice, excluding the
separately owned cut fade. */
static void test_hrtf_restart(void)
{
	short samples[4000];
	float actual[512 * MAXIMUM_OUTPUT_CHANNELS], expected[512 * MAXIMUM_OUTPUT_CHANNELS];
	float send[512], expected_send[512];
	unsigned long layout, mode, n;

	for (n = 0; n < 4000; n++)
		samples[n] = (short)(14000 * sin(2.0 * PI * n / 57.0));
	hrtf_enabled = TRUE;
	for (layout = 2; layout <= 6; layout += 4)
	for (mode = RESAMPLER_XBOX; mode <= RESAMPLER_LINEAR; mode++)
	{
		struct sdl_stream *reused = voice_new(samples, 4000, 1, 44100, 0);
		struct sdl_stream *fresh = voice_new(samples, 4000, 1, 44100, 0);
		XMEDIAPACKET packet;

		output_channels = layout;
		resampler_mode = mode;
		reused->has_3d = fresh->has_3d = TRUE;
		reused->minimum_distance = fresh->minimum_distance = 1.0f;
		reused->maximum_distance = fresh->maximum_distance = 100.0f;
		reused->position[0] = 2.0f;
		reused->position[1] = 1.0f;
		memset(actual, 0, sizeof(actual));
		memset(send, 0, sizeof(send));
		mix_voice(reused, actual, send, 512);
		stream_flush(&reused->object);
		memset(&packet, 0, sizeof(packet));
		packet.pvBuffer = samples;
		packet.dwMaxSize = sizeof(samples);
		CHECK(stream_process(&reused->object, &packet, NULL) == S_OK, "replacement packet failed");
		reused->position[0] = fresh->position[0] = -2.0f;
		reused->position[1] = fresh->position[1] = -1.0f;
		memset(actual, 0, sizeof(actual));
		memset(expected, 0, sizeof(expected));
		memset(send, 0, sizeof(send));
		memset(expected_send, 0, sizeof(expected_send));
		mix_voice(reused, actual, send, 512);
		mix_voice(fresh, expected, expected_send, 512);
		for (n = 0; n < 512 * layout; n++)
			CHECK(fabsf(actual[n] - expected[n]) < 1.0e-6f, "HRTF history leaked after flush at %lu", n);
		for (n = 0; n < 512; n++)
			CHECK(fabsf(send[n] - expected_send[n]) < 1.0e-6f, "room history leaked after flush at %lu", n);
		voice_delete(reused);
		voice_delete(fresh);
	}
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	static const unsigned long rates[][2] = {
		{ 22050, 0 }, { 44100, 0 }, { 48000, 0 }, { 22050, 17000 }, { 22050, 30000 }, { 44100, 60000 },
		{ 44100, 96000 }, { 32000, 0 },
	};
	unsigned long index;

#ifdef _WIN32
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	hrtf_initialize();
	hrtf_enabled = FALSE;
	resampler_mode = RESAMPLER_SINC;
	resampler_initialize();
	resampler_phases_initialize();

	/* the resampler is the low pass worked out exactly: noise at every rate,
	the error well under 16-bit's */
	CASE("cut-overlap") { test_cut_overlap(); return 0; }
	CASE("flush") { test_flush(); return 0; }
	CASE("hrtf-restart") { test_hrtf_restart(); return 0; }
	CASE("cut-lifecycle") { test_cut_lifecycle(); return 0; }
	CASE("reference")
	{
		double *signal = noise(30000);

		for (index = 0; index < sizeof(rates) / sizeof(rates[0]); index++)
		{
			double error_db, peak_db;

			against_reference(signal, 30000, rates[index][0], rates[index][1], &error_db, &peak_db);
			CHECK(error_db < -100.0 && peak_db < -90.0, "%lu Hz played at %lu: the error %.1f dB, its peak %.1f dB",
				rates[index][0], rates[index][1], error_db, peak_db);
		}
		free(signal);
		return 0;
	}
	/* an impulse: the low pass itself */
	CASE("impulse")
	{
		double *signal = calloc(4000, sizeof(double));

		signal[2000] = 1.0;
		for (index = 0; index < sizeof(rates) / sizeof(rates[0]); index++)
		{
			double error_db, peak_db;

			against_reference(signal, 4000, rates[index][0], rates[index][1], &error_db, &peak_db);
			CHECK(peak_db < -90.0, "%lu Hz played at %lu: the impulse's error peaks at %.1f dB", rates[index][0],
				rates[index][1], peak_db);
		}
		free(signal);
		return 0;
	}
	/* sines as heard: a 22 kHz voice flat within half a dB to 10 kHz, a 44
	kHz one to 20 kHz, the rest 70 dB down in the band, more past it */
	CASE("sines")
	{
		static const struct { unsigned long rate, frequency; double hertz, gain_db, rest_db; } sines[] = {
			{ 22050, 0, 100.0, 0.05, -85.0 }, { 22050, 0, 1000.0, 0.05, -85.0 }, { 22050, 0, 5000.0, 0.05, -80.0 },
			{ 22050, 0, 8000.0, 0.1, -75.0 }, { 22050, 0, 10000.0, 0.5, -70.0 },
			{ 44100, 0, 1000.0, 0.05, -85.0 }, { 44100, 0, 10000.0, 0.05, -80.0 },
			{ 44100, 0, 16000.0, 0.1, -75.0 }, { 44100, 0, 20000.0, 0.5, -70.0 },
			{ 48000, 0, 1000.0, 0.05, -85.0 }, { 48000, 0, 20000.0, 0.5, -70.0 },
			{ 22050, 30000, 5000.0, 0.1, -75.0 },
		};

		for (index = 0; index < sizeof(sines) / sizeof(sines[0]); index++)
		{
			double gain_db, rest_db;

			sine_figures(sines[index].hertz, sines[index].rate, sines[index].frequency, &gain_db, &rest_db);
			CHECK(fabs(gain_db) < sines[index].gain_db && rest_db < sines[index].rest_db,
				"%.0f Hz at %lu Hz, played at %lu: gain %.2f dB, the rest %.1f dB", sines[index].hertz,
				sines[index].rate, sines[index].frequency, gain_db, rest_db);
		}
		return 0;
	}
	/* a tone a voice played faster than the output rate takes over the
	output's band does not come out as an alias */
	CASE("aliases")
	{
		double gain_db, rest_db;

		/* 18 kHz at 44.1 kHz, played at 96 kHz: 39 kHz */
		sine_figures(18000.0, 44100, 96000, &gain_db, &rest_db);
		CHECK(rest_db < -60.0, "an alias of 39 kHz %.1f dB", rest_db);
		/* 15 kHz at 44.1 kHz, played at 88.2 kHz: 30 kHz */
		sine_figures(15000.0, 44100, 88200, &gain_db, &rest_db);
		CHECK(rest_db < -60.0, "an alias of 30 kHz %.1f dB", rest_db);
		return 0;
	}
	/* a voice out of earshot moves on as one heard does: once it is audible
	again, both play the same */
	CASE("silent-voice")
	{
		double *signal = noise(60000);
		short *samples = malloc(60000 * sizeof(short));
		double *exact = malloc(60000 * sizeof(double));
		float *heard = calloc(40000 * 2, sizeof(float)), *skipped = calloc(40000 * 2, sizeof(float));
		struct sdl_stream *loud, *quiet;
		unsigned long n, sizes[] = { 1, 7, 512, 333, 2000, 1024 };

		quantize(signal, samples, exact, 60000);
		for (index = 0; index < 3; index++)
		{
			unsigned long frequency = index == 0 ? 0 : index == 1 ? 23456 : 70000, done = 0, size = 0;

			loud = voice_new(samples, 60000, 1, 22050, frequency);
			quiet = voice_new(samples, 60000, 1, 22050, frequency);
			/* the quiet one turned down for a while, at various mix sizes,
			then up */
			quiet->volume = 0.0f;
			while (done < 20000)
			{
				unsigned long count = sizes[size++ % 6];
				static float scratch[2000 * 2], send[2000];

				memset(scratch, 0, sizeof(scratch));
				memset(send, 0, sizeof(send));
				mix_voice(quiet, scratch, send, count);
				for (n = 0; n < count * 2; n++)
					CHECK(scratch[n] == 0.0f, "a voice turned down was heard");
				memset(scratch, 0, sizeof(scratch));
				mix_voice(loud, scratch, send, count);
				done += count;
			}
			CHECK(quiet->center == loud->center && quiet->phase == loud->phase &&
				quiet->history_count == loud->history_count && quiet->cursor == loud->cursor,
				"played at %lu: turned down, the voice is at %lu + %f, heard at %lu + %f", frequency,
				quiet->center, quiet->phase, loud->center, loud->phase);
			/* (it fades in over a mix from 0; the loud one does the same) */
			quiet->volume = 1.0f;
			loud->current_left = loud->current_right = 0.0f;
			loud->ramp_frames = PARAMETER_RAMP_FRAMES;
			render(quiet, skipped, 40000, TRUE);
			render(loud, heard, 40000, TRUE);
			for (n = 0; n < 40000 * 2; n++)
				CHECK(fabsf(heard[n] - skipped[n]) < 1.0e-6f, "played at %lu: frame %lu differs (%f, %f)",
					frequency, n / 2, heard[n], skipped[n]);
			voice_delete(loud);
			voice_delete(quiet);
		}
		/* a voice that runs out of packets while it is turned down ends as
		one heard does */
		loud = voice_new(samples, 3000, 1, 22050, 0);
		quiet = voice_new(samples, 3000, 1, 22050, 0);
		quiet->volume = 0.0f;
		render(loud, heard, 20000, TRUE);
		render(quiet, skipped, 20000, TRUE);
		CHECK(quiet->center == loud->center && quiet->silence == loud->silence &&
			quiet->packets[0].finished == loud->packets[0].finished, "turned down, the voice ends at %lu (%lu), "
			"heard at %lu (%lu)", quiet->center, quiet->silence, loud->center, loud->silence);
		voice_delete(loud);
		voice_delete(quiet);
		free(heard);
		free(skipped);
		free(exact);
		free(samples);
		free(signal);
		return 0;
	}
	/* a mono voice is heard as a stereo one with the same two channels */
	CASE("mono")
	{
		double *signal = noise(30000);
		short *samples = malloc(30000 * sizeof(short)), *pairs = malloc(30000 * 2 * sizeof(short));
		double *exact = malloc(30000 * sizeof(double));
		float *mono = calloc(50000 * 2, sizeof(float)), *stereo = calloc(50000 * 2, sizeof(float));
		unsigned long n;

		quantize(signal, samples, exact, 30000);
		for (n = 0; n < 30000; n++)
			pairs[n * 2] = pairs[n * 2 + 1] = samples[n];
		for (index = 0; index < sizeof(rates) / sizeof(rates[0]); index++)
		{
			struct sdl_stream *one = voice_new(samples, 30000, 1, rates[index][0], rates[index][1]);
			struct sdl_stream *two = voice_new(pairs, 30000, 2, rates[index][0], rates[index][1]);

			render(one, mono, 50000, TRUE);
			render(two, stereo, 50000, TRUE);
			for (n = 0; n < 50000 * 2; n++)
				CHECK(fabsf(mono[n] - stereo[n]) < 1.0e-6f, "%lu Hz played at %lu: frame %lu differs (%f, %f)",
					rates[index][0], rates[index][1], n / 2, mono[n], stereo[n]);
			voice_delete(one);
			voice_delete(two);
		}
		free(mono);
		free(stereo);
		free(exact);
		free(pairs);
		free(samples);
		free(signal);
		return 0;
	}
	/* a voice cut off at the top of its wave (voice_cut, as stream_flush
	does) carries on and fades out over CUT_FADE_FRAMES: no step to silence
	larger than the sine's own, from it at full level to nothing, and nothing
	past it */
	CASE("cut")
	{
		unsigned long frames = 4812, n;
		double *signal = malloc(frames * 2 * sizeof(double)), *exact = malloc(frames * 2 * sizeof(double));
		short *samples = malloc(frames * 2 * sizeof(short));
		float *before = malloc(frames * sizeof(float));
		static float buffer[CHUNK * 2], send[CHUNK];
		struct sdl_stream *stream;
		double largest_step = 0.0, sine_step = 2.0 * PI * 1000.0 / 48000.0 * 0.5;
		float last;

		for (n = 0; n < frames * 2; n++)
			signal[n] = 0.5 * sin(2.0 * PI * 1000.0 * n / 48000.0);
		quantize(signal, samples, exact, frames * 2);
		stream = voice_new(samples, frames * 2, 1, 48000, 0);
		stream->cut_played = CUT_FADE_FRAMES;
		/* 100 cycles and a quarter: at the top */
		render(stream, before, frames, FALSE);
		last = before[frames - 1];
		voice_cut(stream);
		stream->packet_count = 0;
		resampler_reset(stream);
		memset(buffer, 0, sizeof(buffer));
		memset(send, 0, sizeof(send));
		mix_cut(stream, buffer, send, CHUNK);
		for (n = 0; n < CHUNK; n++)
		{
			double step = fabs(buffer[n * 2] - (n ? buffer[(n - 1) * 2] : last));

			if (step > largest_step)
				largest_step = step;
		}
		CHECK(fabs(last) > 0.45, "not cut at the top of the wave: %.3f", last);
		CHECK(largest_step < 1.2 * sine_step, "a step of %.4f cutting off (the sine's %.4f)", largest_step, sine_step);
		CHECK(fabs(buffer[(CUT_FADE_FRAMES / 2) * 2]) > 0.0f, "nothing of the voice after it was cut");
		for (n = CUT_FADE_FRAMES; n < CHUNK; n++)
			CHECK(buffer[n * 2] == 0.0f, "the cut still playing at frame %lu", n);
		voice_delete(stream);
		free(before);
		free(samples);
		free(exact);
		free(signal);
		return 0;
	}
	/* voices of every kind cut off at random (voice_cut, as stream_flush
	does) and started over: nothing but finite samples, the room send's
	included, in the mix of each and of its cut */
	CASE("cuts")
	{
		unsigned long frames = 30000, n, trial, chunk;
		double *signal = noise(frames * 2);
		short *samples = malloc(frames * 2 * sizeof(short));
		double *exact = malloc(frames * 2 * sizeof(double));
		static float buffer[CHUNK * 2], send[CHUNK];
		static const unsigned long rates[] = { 22050, 44100 };

		quantize(signal, samples, exact, frames * 2);
		srand(7);
		for (trial = 0; trial < 1000; trial++)
		{
			struct sdl_stream *stream = voice_new(samples, frames, 1 + rand() % 2, rates[rand() % 2], 0);

			/* (garbage where no cut is playing, which must never be read) */
			for (n = 0; n < CUT_FADE_FRAMES * MAXIMUM_OUTPUT_CHANNELS; n++)
				stream->cut[n] = NAN;
			for (n = 0; n < CUT_FADE_FRAMES; n++)
				stream->cut_send[n] = NAN;
			stream->cut_played = CUT_FADE_FRAMES;
			stream->has_3d = rand() % 4 != 0;
			stream->mode = rand() % 5 == 0 ? DS3DMODE_HEADRELATIVE : DS3DMODE_NORMAL;
			for (n = 0; n < 3; n++)
				stream->position[n] = (float)((rand() % 2000) - 1000) / (rand() % 2 ? 10.0f : 1000.0f);
			stream->minimum_distance = 1.0f;
			stream->maximum_distance = 1000.0f;
			stream->room = rand() % 2 ? 0 : DSBVOLUME_MIN;
			stream->frequency = rand() % 3 ? 0 : (DWORD)(stream->sample_rate * (0.5 + (rand() % 100) / 50.0));
			for (chunk = 0; chunk < 8; chunk++)
			{
				memset(buffer, 0, sizeof(buffer));
				memset(send, 0, sizeof(send));
				/* cut once, and again while that cut plays */
				if (chunk == 3 || (chunk == 4 && rand() % 2))
				{
					voice_cut(stream);
					stream->cursor = 0;
					resampler_reset(stream);
				}
				mix_voice(stream, buffer, send, chunk == 3 ? 100 : CHUNK);
				mix_cut(stream, buffer, send, chunk == 3 ? 100 : CHUNK);
				for (n = 0; n < CHUNK * 2; n++)
				{
					CHECK(fabsf(buffer[n]) < 8.0f && (n >= CHUNK || fabsf(send[n]) < 8.0f),
						"trial %lu, mix %lu, frame %lu: %g, sending %g", trial, chunk, n, buffer[n],
						n < CHUNK ? send[n] : 0.0f);
				}
			}
			voice_delete(stream);
		}
		free(signal);
		free(samples);
		free(exact);
		return 0;
	}
	/* linear interpolation between the surrounding source frames */
	CASE("linear")
	{
		double *signal = noise(30000);
		short *samples = malloc(30000 * sizeof(short));
		double *exact = malloc(30000 * sizeof(double));
		float *output = calloc(50000, sizeof(float));
		unsigned long n;

		resampler_setting = "linear";
		resampler_mode = resampler_mode_read();
		CHECK(resampler_mode == RESAMPLER_LINEAR, "audio.resampler = \"linear\" is not read as linear");
		quantize(signal, samples, exact, 30000);
		for (index = 0; index < sizeof(rates) / sizeof(rates[0]); index++)
		{
			double step = (double)(rates[index][1] ? rates[index][1] : rates[index][0]) / OUTPUT_RATE;
			unsigned long outputs = (unsigned long)((30000 - 1) / step);
			struct sdl_stream *stream = voice_new(samples, 30000, 1, rates[index][0], rates[index][1]);

			if (outputs > 50000)
				outputs = 50000;
			render(stream, output, outputs, FALSE);
			for (n = 0; n < outputs; n++)
			{
				double time = (double)n * step, fraction = time - floor(time);
				long first = (long)floor(time);
				double expected = exact[first] + (first + 1 < 30000 ? exact[first + 1] - exact[first] : -exact[first]) *
					fraction;

				CHECK(fabs(output[n] - expected) < 1.0e-5, "%lu Hz played at %lu: frame %lu is %f, not %f",
					rates[index][0], rates[index][1], n, output[n], expected);
			}
			voice_delete(stream);
		}
		resampler_setting = "sinc";
		CHECK(resampler_mode_read() == RESAMPLER_SINC, "audio.resampler = \"sinc\" is not read as sinc");
		resampler_setting = NULL;
		CHECK(resampler_mode_read() == RESAMPLER_XBOX, "audio.resampler unset is not read as xbox");
		free(output);
		free(exact);
		free(samples);
		free(signal);
		return 0;
	}
	/* A live pitch change keeps both ends of the ramp inside the Xbox
	voice frequency range, including the measured Doppler shift. */
	CASE("doppler-limits")
	{
		unsigned long mode, limit;

		for (mode = 0; mode < 2; mode++)
		for (limit = 0; limit < 2; limit++)
		{
			struct sdl_stream *stream = voice_ahead(10.0f);
			float buffer[CHUNK * 2] = { 0 }, send[CHUNK] = { 0 };
			double phase, rate;
			unsigned long center;

			resampler_mode = mode ? RESAMPLER_LINEAR : RESAMPLER_SINC;
			listener.distance_factor = 3.048f;
			/* Source at supersonic approach, or listener outrunning sound. */
			stream->velocity[2] = limit ? -200.0f : 0.0f;
			listener.velocity[2] = limit ? 0.0f : -200.0f;
			stream->frequency = limit ? 96000 : 100;
			mix_voice(stream, buffer, send, CHUNK);
			stream->frequency = limit ? 192000 : 50;
			center = stream->center;
			phase = stream->phase;
			mix_voice(stream, buffer, send, CHUNK);
			rate = ((double)(stream->center - center) + stream->phase - phase) * OUTPUT_RATE / CHUNK;
			CHECK(fabs(rate - (limit ? 191983.0 : 188.0)) < 0.1,
				"pitch change escaped frequency limit: %.3f Hz (mode %lu)", rate, mode);
			voice_delete(stream);
		}
		listener.velocity[2] = 0.0f;
		return 0;
	}
	CASE("doppler-stereo")
	{
		unsigned long mode, speakers, enabled, frame, chunk;
		short samples[48000 * 2];
		float buffer[CHUNK * 6], send[CHUNK], left[8192];

		/* Two actual source channels: identical frequency, unequal amplitude. */
		for (frame = 0; frame < 48000; frame++)
		{
			samples[frame * 2] = (short)(16000.0 * sin(2.0 * PI * 1000.0 * frame / 48000.0));
			samples[frame * 2 + 1] = samples[frame * 2] / 2;
		}
		CHECK(!stereo_doppler_enabled, "stereo Doppler must initially be off");
		for (mode = 0; mode < 2; mode++)
		for (speakers = 2; speakers <= 6; speakers += 4)
		{
			struct sdl_stream *stream = voice_new(samples, 48000, 2, 48000, 0);

			stream->stereo_positioned = TRUE;
			stream->stereo_approach = 10.0f;
			stream->stereo_distance_fade = 1.0f;
			listener.distance_factor = 3.048f;
			resampler_mode = mode ? RESAMPLER_LINEAR : RESAMPLER_SINC;
			output_channels = speakers;
			/* Enable and then disable on the same playing voice. */
			for (enabled = 0; enabled < 3; enabled++)
			{
				double hertz, wanted = enabled == 1 ? 1000.0 * (1.0 + 30.48 / 343.0) : 1000.0;

				stereo_doppler_enabled = enabled == 1;
				for (chunk = 0; chunk < 8192 / CHUNK; chunk++)
				{
					memset(buffer, 0, sizeof(buffer));
					memset(send, 0, sizeof(send));
					mix_voice(stream, buffer, send, CHUNK);
					for (frame = 0; frame < CHUNK; frame++)
						left[chunk * CHUNK + frame] = buffer[frame * speakers];
				}
				hertz = measured_frequency(left, 2048, 8192);
				CHECK(fabs(hertz - wanted) < 0.2, "stereo setting %lu, speakers %lu, mode %lu: %.2f Hz expected %.2f",
					enabled, speakers, mode, hertz, wanted);
			}
			voice_delete(stream);
		}
		return 0;
	}
	CASE("metrics")
	{
		static const double tones[] = { 100, 1000, 5000, 8000, 10000, 10500, 15000, 18000, 20000, 21000 };
		double *signal = noise(30000), *impulse = calloc(4000, sizeof(double));

		impulse[2000] = 1.0;
		for (index = 0; index < sizeof(rates) / sizeof(rates[0]); index++)
		{
			unsigned long rate = rates[index][0], frequency = rates[index][1], tone;
			double error_db, peak_db, impulse_db, impulse_peak_db;

			against_reference(signal, 30000, rate, frequency, &error_db, &peak_db);
			against_reference(impulse, 4000, rate, frequency, &impulse_db, &impulse_peak_db);
			printf("%lu Hz played at %lu: against the exact low pass, noise %.1f dB (peak %.1f), impulse peak %.1f dB\n",
				rate, frequency ? frequency : rate, error_db, peak_db, impulse_peak_db);
			for (tone = 0; tone < sizeof(tones) / sizeof(tones[0]); tone++)
			{
				double gain_db, rest_db;

				if (tones[tone] >= 0.5 * rate)
					continue;
				sine_figures(tones[tone], rate, frequency, &gain_db, &rest_db);
				printf("  %6.0f Hz: gain %+.3f dB, the rest %.1f dB\n", tones[tone], gain_db, rest_db);
			}
			if (!frequency)
				printf("  sweep to 0.9 of its band: %.1f dB from ideal\n", sweep_figure(rate, 0.45 * rate));
		}
		free(signal);
		free(impulse);
		return 0;
	}
	/* noise, mono and stereo, at each rate through the mixer: the output as
	raw floats on stdout, to compare one mixer with another */
	CASE("render")
	{
		static float mixed[CHUNK * 2];
		double *signal = noise(60000);
		short *samples = malloc(30000 * 2 * sizeof(short));
		double *exact = malloc(60000 * sizeof(double));
		float *output = calloc(40000 * 2, sizeof(float));

		quantize(signal, samples, exact, 60000);
		for (index = 0; index < sizeof(rates) / sizeof(rates[0]); index++)
		{
			unsigned long channels;

			for (channels = 1; channels <= 2; channels++)
			{
				struct sdl_stream *stream = voice_new(samples, 30000, channels, rates[index][0], rates[index][1]);

				render(stream, output, 40000, TRUE);
				fwrite(output, sizeof(float), 40000 * 2, stdout);
				voice_delete(stream);
			}
		}
		/* and a scene's whole mix, reverb and limiter, its voices out of
		earshot coming into it halfway */
		scene(24, 8, TRUE, 3.0);
		for (index = 0; index < 2 * OUTPUT_RATE / CHUNK; index++)
		{
			if (index == OUTPUT_RATE / CHUNK)
				scene_approach();
			mix(mixed, CHUNK);
			fwrite(mixed, sizeof(float), CHUNK * 2, stdout);
		}
		return 0;
	}
	CASE("bench")
	{
		bench(argc > 2 ? strtoul(argv[2], NULL, 0) : 24, argc > 3 ? strtoul(argv[3], NULL, 0) : 16,
			argc > 4 && !strcmp(argv[4], "indoor"), 10.0);
		return 0;
	}
	fprintf(stderr, "no case %s\n", case_name);
	return 2;
}
