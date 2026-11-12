/* Adapted from OpenH1/OpenH1, revision 6750db5a6c9716857bd1e63d9ec188a20c49f1d3.
 * Copyright OpenH1 contributors; AGPL-3.0-or-later.
 * Provenance and hardware measurements: docs/reading/xbox_audio.md. */

/* ---------- the LFE

An Xbox set to Dolby Digital plays its LFE mix bin from the AC-3's LFE
channel, which carries only the lowest frequencies: recorded (its AC-3
decoded), a 1 kHz tone sent to the LFE alone came out at nothing over the
capture's last bit, more than 84 dB down, where it played here whole. So on
5.1 the LFE channel goes through a low pass, the Xbox's own: a sweep sent to
the LFE alone, recorded, fell 3 dB by 86 Hz and then as a 4th order filter
does (a fit of 4.7), not as the Dolby Digital LFE's 120 Hz 8th order would.
So a 4th order Butterworth at 86 Hz (24 dB an octave, 85 dB down at 1 kHz,
as the tone was), as two biquads (RBJ's, in double
precision: their poles are close to 1). The
game itself sends nothing to the LFE bin; a sound given it (a mod's) is
heard there as an Xbox played it. */

#define LFE_CUTOFF 86.0
#define LFE_SECTIONS 2

static struct
{
	/* each biquad's b0 (and b2), b1, a1 and a2 (a0 1), and its state
	(transposed direct form II) */
	double coefficients[LFE_SECTIONS][4];
	double state[LFE_SECTIONS][2];
	BOOL ready;
} lfe;

static void lfe_initialize(void)
{
	int section;

	for (section = 0; section < LFE_SECTIONS; section++)
	{
		/* (the Butterworth's Q of each pair of poles) */
		double q = 1.0 / (2.0 * cos((2 * section + 1) * 3.14159265358979 / (4 * LFE_SECTIONS)));
		double w = 2.0 * 3.14159265358979 * LFE_CUTOFF / OUTPUT_RATE;
		double alpha = sin(w) / (2.0 * q), cosine = cos(w), a0 = 1.0 + alpha;

		lfe.coefficients[section][0] = 0.5 * (1.0 - cosine) / a0;
		lfe.coefficients[section][1] = (1.0 - cosine) / a0;
		lfe.coefficients[section][2] = -2.0 * cosine / a0;
		lfe.coefficients[section][3] = (1.0 - alpha) / a0;
	}
	memset(lfe.state, 0, sizeof(lfe.state));
	lfe.ready = TRUE;
}

/* the LFE channel of frames of output (frames of 6) low passed; on stereo
nothing, the filters emptied */
static void lfe_lowpass(float *output, unsigned long frames)
{
	unsigned long frame;
	int section;

	if (!lfe.ready)
		lfe_initialize();
	if (output_channels != 6)
	{
		memset(lfe.state, 0, sizeof(lfe.state));
		return;
	}
	/* (nothing sent and nothing ringing, as nearly always: nothing to do) */
	for (section = 0; section < LFE_SECTIONS; section++)
	{
		if (lfe.state[section][0] || lfe.state[section][1])
			break;
	}
	if (section == LFE_SECTIONS)
	{
		for (frame = 0; frame < frames && !output[frame * 6 + 3]; frame++)
			;
		if (frame == frames)
			return;
	}
	for (frame = 0; frame < frames; frame++)
	{
		double x = output[frame * 6 + 3];

		for (section = 0; section < LFE_SECTIONS; section++)
		{
			const double *c = lfe.coefficients[section];
			double *state = lfe.state[section];
			double y = c[0] * x + state[0];

			state[0] = c[1] * x - c[2] * y + state[1];
			state[1] = c[0] * x - c[3] * y;
			x = y;
		}
		output[frame * 6 + 3] = flush_denormal((float)x);
	}
	/* (fed silence, the state decays into the denormals within a second or
	two: flush_denormal's) */
	for (section = 0; section < LFE_SECTIONS; section++)
	{
		if (fabs(lfe.state[section][0]) < 1.0e-20 && fabs(lfe.state[section][1]) < 1.0e-20)
			lfe.state[section][0] = lfe.state[section][1] = 0.0;
	}
}
