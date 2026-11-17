/* Adapted from OpenH1/OpenH1, revision 6750db5a6c9716857bd1e63d9ec188a20c49f1d3.
 * Copyright OpenH1 contributors; AGPL-3.0-or-later.
 * Provenance and hardware measurements: docs/reading/xbox_audio.md. */

/* ---------- reverb

The Xbox ran an I3DL2 reverb on its audio DSP (the effects image the game
downloads), fed by every 3D voice's I3DL2 mix bin. The game sets the listener
properties from the sound environment the camera is in (sound_dsound_xbox.c
dsound_set_listener_properties), and each 3D voice's room send from its
reverb attenuation and occlusion (dsound_channel_set_I3DL2_properties). This
is that DSP program's reverb, as decoded from the image and from the XDK
DirectSound's conversion of the listener properties into its parameters (in
every retail default.xbe), redone in floating point:
	- the send (here the mono bus, the room level and the Room HF low pass
	  already on it) is halved (and turned over at a Room HF of 0, as the
	  DSP's filter word did), and the reverb runs at half the output rate,
	  24 kHz: it takes every other frame and plays each of its frames twice,
	  as the DSP did (no filter either way: the reverberation stops at 12
	  kHz, with the images of the repeat above, and the send's top octave
	  is folded onto its bottom one, which is why a voice at the output's
	  rate sends its frames unfiltered, mix_voice);
	- every delay is a whole number of the DSP's 4-sample words, and the
	  input's delay line is REVERB_INPUT_SIZE long, a longer delay wrapping
	  round it as the DSP's did;
	- the reflections: on each of four channels (front left and right, back
	  left and right), the input the reflections delay after the sound, and
	  again a channel's share of the reverb delay after that, through a
	  feedback comb of a few milliseconds (gains -0.4, 0.4, -0.4, -0.4), at
	  the reflections level times a channel's weight (0.57, 0.52, 0.54,
	  0.6);
	- the late reverberation: a feedback delay network of four lines of
	  fixed lengths (58 to 103 ms) mixed by a Householder reflection, each
	  line decaying by 60 dB in the decay time (Jot: a gain of
	  10^(-3 length / (rate decay)), whole millibels), its high frequencies
	  in the decay time times the HF ratio at the HF reference (a one-pole
	  low pass; a ratio over 1 is held to 1, as the DSP's conversion does).
	  Into it go taps of the input the reflections and reverb delays after
	  the sound (and 2.4, 5.6 and 9.7 ms later), and the taps of a chain of
	  four delays (up to a second long, as the decay time and the density
	  make it, each decayed as the lines are), at 0.2 times the diffusion;
	  the chain's taps also go, at 1 minus the diffusion, round two loops as
	  long as the chain. Each channel hears a line, at the reverb level, and
	  a loop at half of it;
	- the four channels play from the front and back pairs on 5.1, and on
	  stereo each side's back channel at -3 dB with its front, as the
	  Xbox's stereo downmix (its encode processor's: left = front left +
	  0.707 center + 0.707 back left) played the back speakers' mix bins.
A change of environment crossfades each delay's read from its old length to
its new one, and each gain and filter, over REVERB_FADE_FRAMES: what is
reverberating carries on, without the click of a jump or the pitch shift of
a sliding delay (the DSP faded each tap out and in again, one by one). The
fade is short, 2 ms: recorded on an Xbox, a click played as the
environment was set, 2 ms into its sound, echoed as the new room alone, in
every environment (over 85 ms, as it was, its echo here was the room before's
for the most part, and 1 to 4 dB under the Xbox's, and the cave's, its
reverberation turned over, faded through nothing). The game moves the
environment a step each tick on the way to another (scenario.c), and each
step waits for the fade to the last to end (mix). audio.reverb = false fades
the reverb out over REVERB_LEVEL_FADE_FRAMES and stops it. Once
nothing has been sent for REVERB_QUIET_FRAMES and the reverberation is 120 dB
down, the reverb stops until something is sent again (outdoors, in the
menus). */

#define REVERB_RATE (OUTPUT_RATE / 2)
#define REVERB_INPUT_SIZE 6000 /* the input's and the chain's delays, a quarter of a second */
#define REVERB_LOOP_SIZE 24000 /* the loops' */
#define REVERB_CHANNELS 4
#define REVERB_LINES 4
#define REVERB_CHAIN 4
#define REVERB_COMB_SIZE 720
#define REVERB_FADE_FRAMES 96 /* 2 ms */
/* a new environment's delays wait 100 ms (REVERB_DELAY_HOLD_FRAMES): recorded
on an Xbox (the test bed's ninth run), a click played 2 or 25 ms after the
hallway gave way to the cave echoed at the cave's levels but at the
hallway's delays (its reflections from 7 ms, its late reverberation from
18), and one played 100 ms after it as in the cave alone (fitted: a hold of
4800 frames, the click trains' echoes within 4 to 7 dB rms of the
recordings', 15 before; 4200 to 5400 fit alike) */
#ifndef REVERB_DELAY_HOLD_FRAMES
#define REVERB_DELAY_HOLD_FRAMES 4800
#endif
#define REVERB_LEVEL_FADE_FRAMES 4096 /* 85 ms */
/* the input's delay, the chain and a loop, played out */
#define REVERB_QUIET_FRAMES (2 * (REVERB_INPUT_SIZE * REVERB_CHAIN + REVERB_LOOP_SIZE))
#define REVERB_QUIET_PEAK 1.0e-6f

/* each channel's reflections: its weight, its tap's part of the reverb
delay, and its comb's delay (a part of the reverb delay and seconds), gain
and line's length (its longest delay) */
static const float reverb_reflection_weights[REVERB_CHANNELS] = { 0.57f, 0.52f, 0.54f, 0.6f };
static const float reverb_reflection_taps[REVERB_CHANNELS] = { 0.1052f, 0.0673f, 0.1737f, 0.2404f };
static const float reverb_comb_parts[REVERB_CHANNELS] = { 0.2469f, 0.2209f, 0.2311f, 0.2608f };
static const float reverb_comb_seconds[REVERB_CHANNELS] = { 0.003651f, 0.003515f, 0.003673f, 0.003946f };
static const float reverb_comb_gains[REVERB_CHANNELS] = { -0.4f, 0.4f, -0.4f, -0.4f };
static const long reverb_comb_sizes[REVERB_CHANNELS] = { 680, 612, 640, 720 };
/* the lines' lengths (58, 70.3, 85.2 and 103.3 ms) */
static const long reverb_line_lengths[REVERB_LINES] = { 1392, 1684, 2044, 2476 };
/* the taps of the input into the lines, after the reverb delay; which pair
of them goes into each line, with what signs (times the diffusion) */
static const long reverb_onset_taps[REVERB_LINES] = { 0, 58, 134, 232 };
static const float reverb_onset_signs[2 * REVERB_LINES] = { 1, -1, -1, 1, 1, 1, -1, -1 };
/* the chain's two taps on each of its delays, in parts of its length (the
second goes on into the next delay) */
static const float reverb_chain_taps[REVERB_CHAIN][2] =
{
	{ 0.040023f, 0.021224f }, { 0.288186f, 0.37542f }, { 0.603764f, 0.623583f }, { 0.851905f, 0.759229f }
};

/* what an environment sets: delays in the reverb's samples (multiples of
4), gains, and the one-pole filters' coefficients (y = a y' + b x) */
struct reverb_filter
{
	float a, b;
};

struct reverb_parameters
{
	long reflections_delay, reflection_taps[REVERB_CHANNELS], comb_delays[REVERB_CHANNELS];
	long late_delay, chain[REVERB_CHAIN][2], loop_length;
	float reflections_gains[REVERB_CHANNELS], late_gain;
	float input_gain, onset_gains[2 * REVERB_LINES], loop_input, network_input;
	struct reverb_filter lines[REVERB_LINES], chain_filter, loop_filter;
};

static struct
{
	/* the bus of the current mix */
	float send[MIX_CHUNK_FRAMES];
	/* the environment serial and properties of the parameters faded to */
	unsigned long serial;
	DSI3DL2LISTENER properties;
	/* the parameters fading from and to, and the frames of the fade left
	(none: from is to) */
	struct reverb_parameters from, to;
	unsigned long fade;
	/* the delays a change leaves in place (delays_from) for delay_hold
	frames before they cross to the new environment's over
	REVERB_FADE_FRAMES (delay_fade): the gains and filters are the new
	environment's at once (fade), its delays only after the hold */
	struct reverb_parameters delays_from;
	unsigned long delay_hold, delay_fade;
	/* the output's level, 1 while audio.reverb is on, moving to 0 when it
	is turned off; whether the buffers are silent; and the frames since the
	send or the output was last over REVERB_QUIET_PEAK */
	float level;
	BOOL silent;
	unsigned long quiet;

	/* whether the next output frame is the second of a reverb frame, and
	that frame's channels */
	BOOL odd;
	float held[REVERB_CHANNELS];

	/* the delays, each written at position (of its size) */
	float input[REVERB_INPUT_SIZE];
	float chain[REVERB_CHAIN - 1][REVERB_INPUT_SIZE];
	float combs[REVERB_CHANNELS][REVERB_COMB_SIZE];
	float lines[REVERB_LINES][2476];
	float loops[2][REVERB_LOOP_SIZE];
	unsigned long position;
	/* the filters' last outputs */
	float line_state[REVERB_LINES], chain_state[REVERB_CHAIN - 1], loop_state[2];
} reverb;

static float clamp_real(float value, float minimum, float maximum)
{
	return value < minimum ? minimum : value > maximum ? maximum : value;
}

/* the DSP's: a float to a whole number, towards 0; a delay up to a whole
word, and down */
static long reverb_whole(double value)
{
	return (long)value;
}

static long reverb_word_up(long samples)
{
	return (samples + 1) & ~3L;
}

static long reverb_word_down(long samples)
{
	return samples & ~3L;
}

/* 10^(millibels / 2000), as the conversion works it out (in single
precision) */
static float reverb_gain(long millibels)
{
	return (float)pow(10.0, (double)((float)millibels * 0.0005f));
}

/* the one-pole filter whose gain is 10^(level / 2000) at low frequencies
and 10^((level + high) / 2000) at hertz, at the reverb's rate; levels at or
under -10000 silence it */
static struct reverb_filter reverb_filter_set(long level, long high, float hertz)
{
	struct reverb_filter filter = { 0.0f, 0.0f };
	double cosine, power, root;

	if (level <= DSBVOLUME_MIN)
		return filter;
	if (!high)
	{
		filter.b = level ? reverb_gain(level) : 1.0f;
		return filter;
	}
	cosine = cos(2.0 * 3.14159274 * hertz / REVERB_RATE);
	power = reverb_gain(2 * high);
	root = sqrt(2.0 * power * (1.0 - cosine) - power * power * (1.0 - cosine * cosine));
	filter.a = (float)((1.0 - power * cosine - root) / (1.0 - power));
	filter.b = (1.0f - filter.a) * reverb_gain(level);
	/* under the DSP's least step, the filter passes nothing (whatever its a,
	which can come out at 1) */
	if (filter.b < 1.0f / 8388608.0f)
		filter.a = filter.b = 0.0f;
	return filter;
}

/* the filter of a delay of samples in the environment of properties: what
decays it 60 dB in the decay time, and its high frequencies in the decay
time times the HF ratio (held to 1) */
static struct reverb_filter reverb_decay_filter(long samples, const DSI3DL2LISTENER *properties)
{
	double loss = (double)samples * 4.16666662e-05f / properties->flDecayTime * -6000.0;
	long level = reverb_whole(loss);

	if (properties->flDecayHFRatio < 1.0f)
		loss /= properties->flDecayHFRatio;
	return reverb_filter_set(level, reverb_whole(loss) - level, properties->flHFReference);
}

/* the DSP's parameters of an environment, as DirectSound worked them out */
static void reverb_parameters_set(struct reverb_parameters *parameters, const DSI3DL2LISTENER *properties)
{
	float diffusion = properties->flDiffusion * 0.01f;
	/* how long the chain is: a second, shorter in a room of short decay
	(and more so as the density is less) */
	float length = (float)((2.0 - (double)0.01f * properties->flDensity) * 5.0 * (properties->flDecayTime - 1.0));
	double scale;
	long reverb_delay, previous = 0, chain_length;
	int index;

	length = length < 1.0f ? 1.0f : length > 60.0f ? 60.0f : length;
	scale = length < 2.5f ? ((double)length - (double)0.8f) * (double)0.588235259f : 1.0;

	parameters->reflections_delay = reverb_word_down(reverb_whole(properties->flReflectionsDelay * 24000.0));
	reverb_delay = reverb_word_down(reverb_whole(properties->flReverbDelay * 24000.0));
	parameters->late_delay = reverb_delay + parameters->reflections_delay;
	for (index = 0; index < REVERB_CHANNELS; index++)
	{
		long comb = reverb_word_up(reverb_whole(((double)properties->flReverbDelay * reverb_comb_parts[index] +
			reverb_comb_seconds[index]) * 24000.0));

		parameters->reflection_taps[index] = parameters->reflections_delay +
			reverb_word_up(reverb_whole((double)reverb_delay * reverb_reflection_taps[index]));
		parameters->comb_delays[index] = comb > reverb_comb_sizes[index] ? reverb_comb_sizes[index] : comb;
		parameters->reflections_gains[index] = properties->lReflections <= DSBVOLUME_MIN ? 0.0f :
			reverb_gain(properties->lReflections) * reverb_reflection_weights[index];
	}

	/* the chain: its taps at their parts of its length, each delay's
	measured from the last's second tap and squeezed into the delay */
	chain_length = reverb_word_down(reverb_whole(scale * 24000.0));
	for (index = 0; index < REVERB_CHAIN; index++)
	{
		long first = reverb_word_down(reverb_whole(scale * reverb_chain_taps[index][0] * 24000.0));
		long second = reverb_word_down(reverb_whole(scale * reverb_chain_taps[index][1] * 24000.0));
		long span = (first > second ? first : second) - previous;

		if (span > REVERB_INPUT_SIZE - 1)
		{
			double squeeze = (double)(REVERB_INPUT_SIZE - 1) / span;

			parameters->chain[index][0] = reverb_word_up(reverb_whole((first - previous) * squeeze));
			parameters->chain[index][1] = reverb_word_up(reverb_whole((second - previous) * squeeze));
		}
		else
		{
			parameters->chain[index][0] = first - previous;
			parameters->chain[index][1] = second - previous;
		}
		previous = second;
	}
	parameters->loop_length = chain_length > REVERB_LOOP_SIZE ? REVERB_LOOP_SIZE : chain_length;
	parameters->chain_filter = reverb_decay_filter(chain_length >> 2, properties);
	parameters->loop_filter = reverb_decay_filter(chain_length, properties);
	for (index = 0; index < REVERB_LINES; index++)
		parameters->lines[index] = reverb_decay_filter(reverb_line_lengths[index], properties);

	for (index = 0; index < 2 * REVERB_LINES; index++)
		parameters->onset_gains[index] = clamp_real(reverb_onset_signs[index] * diffusion, -1.0f, 1.0f);
	parameters->loop_input = 1.0f - diffusion;
	parameters->network_input = 0.2f * diffusion;
	parameters->late_gain = properties->lReverb <= DSBVOLUME_MIN ? 0.0f : reverb_gain(properties->lReverb);
	/* the send halved; the DSP's Room HF filter, which the send's low pass
	is, passes it whole at a Room HF of 0 with a gain of 1, which its 24-bit
	word holds as -1: the reverberation upside down (the cave's) */
	parameters->input_gain = properties->lRoomHF ? 0.5f : -0.5f;
}

/* a new environment, faded to unless the reverb is silent. The game sets the
same one again several times a second in some places (Derelict), which
changes nothing */
static void reverb_update(const DSI3DL2LISTENER *properties, unsigned long serial)
{
	BOOL reflections_off, held;

	reverb.serial = serial;
	if (!reverb.silent && !memcmp(properties, &reverb.properties, sizeof(*properties)))
		return;
	reflections_off = reverb.properties.lReflections <= DSBVOLUME_MIN;
	reverb.properties = *properties;
	/* the delays in use held, unless a hold is running already: then they
	cross to whichever environment is the last when it ends (the game moves
	from one to another a step each tick, scenario_get_sound_environment) */
	held = reverb.delay_hold || reverb.delay_fade;
	if (!held)
	{
		reverb.delays_from = reverb.to;
		reverb.delay_hold = REVERB_DELAY_HOLD_FRAMES;
		reverb.delay_fade = REVERB_FADE_FRAMES;
	}
	reverb_parameters_set(&reverb.to, properties);
	/* (from a room with no reflections, Halo's outdoors, the reflections'
	delays are the new room's at once: recorded, the cave's first
	reflections came at its 15 ms while its late reverberation waited) */
	if (!held && reflections_off)
	{
		int index;

		reverb.delays_from.reflections_delay = reverb.to.reflections_delay;
		for (index = 0; index < REVERB_CHANNELS; index++)
		{
			reverb.delays_from.reflection_taps[index] = reverb.to.reflection_taps[index];
			reverb.delays_from.comb_delays[index] = reverb.to.comb_delays[index];
		}
	}
	if (reverb.silent)
		reverb.from = reverb.to;
	reverb.fade = reverb.silent ? 0 : REVERB_FADE_FRAMES;
}

static void reverb_clear(void)
{
	memset(reverb.input, 0, sizeof(reverb.input));
	memset(reverb.chain, 0, sizeof(reverb.chain));
	memset(reverb.combs, 0, sizeof(reverb.combs));
	memset(reverb.lines, 0, sizeof(reverb.lines));
	memset(reverb.loops, 0, sizeof(reverb.loops));
	memset(reverb.line_state, 0, sizeof(reverb.line_state));
	memset(reverb.chain_state, 0, sizeof(reverb.chain_state));
	memset(reverb.loop_state, 0, sizeof(reverb.loop_state));
	memset(reverb.held, 0, sizeof(reverb.held));
	reverb.odd = FALSE;
	reverb.from = reverb.to;
	reverb.fade = 0;
	reverb.silent = TRUE;
	reverb.quiet = 0;
}

static void reverb_initialize(void)
{
	reverb.silent = TRUE;
	reverb.level = reverb_enabled ? 1.0f : 0.0f;
	reverb_update(&environment, environment_serial);
	reverb.delays_from = reverb.to;
	reverb.delay_hold = reverb.delay_fade = 0;
}

/* the delays' hold and crossfade moved on frames (output frames) */
static void reverb_delays_step(unsigned long frames)
{
	unsigned long step;

	step = frames < reverb.delay_hold ? frames : reverb.delay_hold;
	reverb.delay_hold -= step;
	frames -= step;
	if (!reverb.delay_hold && reverb.delay_fade)
	{
		reverb.delay_fade = frames < reverb.delay_fade ? reverb.delay_fade - frames : 0;
		if (frames && !reverb.delay_fade)
			reverb.delays_from = reverb.to;
	}
}

static float reverb_blend(float from, float to, float fade)
{
	return from + (to - from) * fade;
}

/* the sample of a delay line size long that went in delay samples ago: as
the DSP's, a delay of a block (16 samples) or more, and one longer than the
line wrapping round it */
static float reverb_tap(const float *line, long size, long delay)
{
	long position = (long)(reverb.position % (unsigned long)size);

	delay = delay < 16 ? 16 : (delay - 16) % size + 16;
	/* The block-minimum wrap can leave delay up to size + 15. Reduce it
	before subtracting so the ring index cannot become negative. */
	return line[(position - delay % size + size) % size];
}

/* the tap crossfaded from the from delay to the to delay by fade (0 to 1) */
static float reverb_read(const float *line, long size, long from, long to, float fade)
{
	float sample = reverb_tap(line, size, from);

	return from == to ? sample : sample + (reverb_tap(line, size, to) - sample) * fade;
}

static void reverb_write(float *line, long size, float value)
{
	line[reverb.position % (unsigned long)size] = flush_denormal(value);
}

static float reverb_filter(struct reverb_filter from, struct reverb_filter to, float fade, float *state, float x)
{
	*state = flush_denormal(reverb_blend(from.a, to.a, fade) * *state + reverb_blend(from.b, to.b, fade) * x);
	return *state;
}

/* a frame of the reverb (at REVERB_RATE) of input, its four channels into
channels */
static void reverb_frame(float input, float fade, float *channels)
{
	const struct reverb_parameters *from = &reverb.from, *to = &reverb.to, *dfrom = &reverb.delays_from;
	/* (the delays' own crossfade: none while they are held) */
	float dfade = reverb.delay_hold ? 0.0f : 1.0f - (float)reverb.delay_fade / REVERB_FADE_FRAMES;
	float onset[2 * REVERB_LINES], taps[REVERB_LINES], chain[REVERB_CHAIN][2], sums[2], loops[2];
	float reflections, network_input, loop_input, mean = 0.0f, late_gain;
	int index;

	/* the reflections: each channel's comb */
	reflections = reverb_read(reverb.input, REVERB_INPUT_SIZE, dfrom->reflections_delay, to->reflections_delay, dfade);
	for (index = 0; index < REVERB_CHANNELS; index++)
	{
		float value = reflections + reverb_read(reverb.input, REVERB_INPUT_SIZE,
			dfrom->reflection_taps[index], to->reflection_taps[index], dfade) +
			reverb_comb_gains[index] * reverb_read(reverb.combs[index], reverb_comb_sizes[index],
			dfrom->comb_delays[index], to->comb_delays[index], dfade);

		reverb_write(reverb.combs[index], reverb_comb_sizes[index], value);
		channels[index] = value * reverb_blend(from->reflections_gains[index], to->reflections_gains[index], fade);
	}

	/* the lines' ends, with the input's onset taps in pairs */
	for (index = 0; index < 2 * REVERB_LINES; index++)
	{
		long tap = reverb_onset_taps[index % REVERB_LINES];

		onset[index] = reverb_read(reverb.input, REVERB_INPUT_SIZE, dfrom->late_delay + tap, to->late_delay + tap, dfade);
	}
	for (index = 0; index < REVERB_LINES; index++)
	{
		taps[index] = reverb_tap(reverb.lines[index], reverb_line_lengths[index], reverb_line_lengths[index]) +
			reverb_blend(from->onset_gains[2 * index], to->onset_gains[2 * index], fade) * onset[2 * index] +
			reverb_blend(from->onset_gains[2 * index + 1], to->onset_gains[2 * index + 1], fade) * onset[2 * index + 1];
		mean += 0.5f * taps[index];
	}

	/* the chain's taps, summed in two, round the loops and into the lines */
	for (index = 0; index < REVERB_CHAIN; index++)
	{
		const float *line = index ? reverb.chain[index - 1] : reverb.input;

		chain[index][0] = reverb_read(line, REVERB_INPUT_SIZE, dfrom->chain[index][0], to->chain[index][0], dfade);
		chain[index][1] = reverb_read(line, REVERB_INPUT_SIZE, dfrom->chain[index][1], to->chain[index][1], dfade);
	}
	sums[0] = chain[0][1] + chain[1][0] + chain[2][0] + chain[3][1];
	sums[1] = chain[0][0] + chain[1][1] + chain[2][1] + chain[3][0];
	network_input = reverb_blend(from->network_input, to->network_input, fade) * (sums[0] + sums[1]);
	loop_input = reverb_blend(from->loop_input, to->loop_input, fade);
	for (index = 0; index < 2; index++)
	{
		loops[index] = loop_input * sums[index] +
			reverb_read(reverb.loops[index], REVERB_LOOP_SIZE, dfrom->loop_length, to->loop_length, dfade);
		reverb_write(reverb.loops[index], REVERB_LOOP_SIZE,
			reverb_filter(from->loop_filter, to->loop_filter, fade, &reverb.loop_state[index], loops[index]));
	}
	for (index = 0; index < REVERB_CHAIN - 1; index++)
	{
		reverb_write(reverb.chain[index], REVERB_INPUT_SIZE,
			reverb_filter(from->chain_filter, to->chain_filter, fade, &reverb.chain_state[index], chain[index][1]));
	}

	/* the network: a Householder reflection of the lines' ends, decayed */
	for (index = 0; index < REVERB_LINES; index++)
	{
		reverb_write(reverb.lines[index], reverb_line_lengths[index],
			reverb_filter(from->lines[index], to->lines[index], fade, &reverb.line_state[index],
			taps[index] - mean + network_input));
	}
	reverb_write(reverb.input, REVERB_INPUT_SIZE, input);
	reverb.position++;

	late_gain = reverb_blend(from->late_gain, to->late_gain, fade);
	for (index = 0; index < REVERB_CHANNELS; index++)
		channels[index] += late_gain * (taps[index] + 0.5f * loops[index / 2]);
}

/* adds the reverberation of send to output, its level moving to
target_level */
static void reverb_process(const float *send, float *output, unsigned long frames, float target_level)
{
	float peak = 0.0f;
	unsigned long frame;

	reverb.silent = FALSE;
	for (frame = 0; frame < frames; frame++)
	{
		const float *held = reverb.held;
		float level = reverb.level;

		if (!reverb.odd)
		{
			float fade = 1.0f - (float)reverb.fade / REVERB_FADE_FRAMES;

			reverb_frame(reverb_blend(reverb.from.input_gain, reverb.to.input_gain, fade) * send[frame], fade, reverb.held);
		}
		reverb.odd = !reverb.odd;
		if (output_channels == 6)
		{
			output[frame * 6] += held[0] * level;
			output[frame * 6 + 1] += held[1] * level;
			output[frame * 6 + 4] += held[2] * level;
			output[frame * 6 + 5] += held[3] * level;
		}
		else
		{
			output[frame * 2] += (held[0] + 0.70710678f * held[2]) * level;
			output[frame * 2 + 1] += (held[1] + 0.70710678f * held[3]) * level;
		}
		peak = fabsf(held[0]) > peak ? fabsf(held[0]) : peak;
		peak = fabsf(held[1]) > peak ? fabsf(held[1]) : peak;
		peak = fabsf(held[2]) > peak ? fabsf(held[2]) : peak;
		peak = fabsf(held[3]) > peak ? fabsf(held[3]) : peak;
		peak = fabsf(send[frame]) > peak ? fabsf(send[frame]) : peak;
		if (reverb.fade && !--reverb.fade)
			reverb.from = reverb.to;
		reverb_delays_step(1);
		reverb.level = clamp_real(target_level, reverb.level - 1.0f / REVERB_LEVEL_FADE_FRAMES,
			reverb.level + 1.0f / REVERB_LEVEL_FADE_FRAMES);
	}
	reverb.quiet = peak > REVERB_QUIET_PEAK ? 0 : reverb.quiet + frames;
}

/* whether anything is sent to the reverb in send */
static BOOL reverb_sent(const float *send, unsigned long frames)
{
	unsigned long frame;

	for (frame = 0; frame < frames; frame++)
	{
		if (fabsf(send[frame]) > REVERB_QUIET_PEAK)
			return TRUE;
	}
	return FALSE;
}
