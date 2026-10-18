/* OpenAL voices for the Mac view of the DirectSound mixer
   (tools/mac_dsound_view.py).

   With HALO_MAC_AUDIO_OPENAL=1, each stream is played by an OpenAL source
   instead of the port's resampler: its packets are queued as OpenAL buffers
   at their own sample rate, and the gains, pan and pitch the mixer would
   use (voice_gains) are given to the source each frame. Apple's OpenAL
   resamples and mixes them, which on a G4 costs a small part of what the
   scalar resampler does (build/mac-audit/audio-cost). A packet is finished
   when OpenAL has played its buffer, and completed as before by
   DirectSoundDoWork. Included at the start of the view's output section. Voice chat, and any stream OpenAL cannot give a source,
   stay on the software mixer.

   Where Apple's ASA extension is present (Leopard), the reverb and the
   muffling are Apple's: the listener's reverb takes a room type from the
   game's I3DL2 decay time, each source sends to it as much as the software
   mixer's room gain, and the direct path's high frequency loss (obstruction
   and occlusion) is its obstruction. Without ASA the voices play dry. Not
   modelled: the pan of positioned stereo voices, and the room send's own
   high frequency loss.
   OpenAL is loaded at run time, as MacSoft's Halo loads it, so builds need
   no framework and a Mac without it keeps the software mixer. */

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <dlfcn.h>

#define OPENAL_LIBRARY "/System/Library/Frameworks/OpenAL.framework/OpenAL"
/* Apple's ALC_MAC_OSX_SPATIAL_RENDERING_QUALITY_LOW ('rdlo'): an equal
   power pan, as the software mixer's */
#define OPENAL_RENDERING_QUALITY_LOW 0x72646c6fL
/* Apple's ASA properties (MacOSX_OALExtensions.h): 'rvon', 'rvgl', 'rvrt',
   'rvqt', 'rvsl', 'obst'. The listener's on switch takes an ALuint. */
#define OPENAL_ASA_REVERB_ON 0x72766f6eUL
#define OPENAL_ASA_REVERB_GLOBAL_LEVEL 0x7276676cUL
#define OPENAL_ASA_REVERB_ROOM_TYPE 0x72767274UL
#define OPENAL_ASA_REVERB_QUALITY 0x72767174UL
#define OPENAL_ASA_REVERB_SEND_LEVEL 0x7276736cUL
#define OPENAL_ASA_OBSTRUCTION 0x6f627374UL
/* ALC_ASA_REVERB_QUALITY_Low: about 2.5% of a PowerBook G4 for the reverb */
#define OPENAL_ASA_REVERB_QUALITY_LOW 0x20
#define OPENAL_MINIMUM_PITCH 0.5f
#define OPENAL_MAXIMUM_PITCH 2.0f

static struct
{
	void *library;
	ALCdevice *device;
	ALCcontext *context;
	BOOL active;
	unsigned long sources;
	unsigned long fallbacks;
	unsigned long buffers_queued;
	unsigned long buffer_failures;
	/* Apple's ASA reverb, on when the extension and audio.reverb allow, and
	the I3DL2 environment it last took */
	BOOL reverb_on;
	unsigned long environment_serial;
	/* nonzero sends and obstructions given, and the room type last chosen */
	unsigned long sends_set, obstructions_set;
	long room_type;
	ALenum (*ASASetListener)(const ALuint, ALvoid *, ALuint);
	ALenum (*ASASetSource)(const ALuint, ALuint, ALvoid *, ALuint);

	ALCdevice *(*OpenDevice)(const ALCchar *);
	ALCboolean (*CloseDevice)(ALCdevice *);
	ALCcontext *(*CreateContext)(ALCdevice *, const ALCint *);
	ALCboolean (*MakeContextCurrent)(ALCcontext *);
	void (*DestroyContext)(ALCcontext *);
	void *(*GetProcAddress)(ALCdevice *, const ALCchar *);
	ALenum (*GetError)(void);
	void (*GenSources)(ALsizei, ALuint *);
	void (*DeleteSources)(ALsizei, const ALuint *);
	void (*Sourcei)(ALuint, ALenum, ALint);
	void (*Sourcef)(ALuint, ALenum, ALfloat);
	void (*Source3f)(ALuint, ALenum, ALfloat, ALfloat, ALfloat);
	void (*GetSourcei)(ALuint, ALenum, ALint *);
	void (*Getsourcef)(ALuint, ALenum, ALfloat *);
	void (*SourcePlay)(ALuint);
	void (*SourcePause)(ALuint);
	void (*SourceStop)(ALuint);
	void (*GenBuffers)(ALsizei, ALuint *);
	void (*DeleteBuffers)(ALsizei, const ALuint *);
	void (*BufferData)(ALuint, ALenum, const ALvoid *, ALsizei, ALsizei);
	void (*SourceQueueBuffers)(ALuint, ALsizei, const ALuint *);
	void (*SourceUnqueueBuffers)(ALuint, ALsizei, ALuint *);
} openal;

static BOOL openal_symbol(void **target, const char *name)
{
	*target = dlsym(openal.library, name);
	if (!*target)
		platform_log("audio: OpenAL lacks %s", name);
	return *target != NULL;
}

static BOOL openal_load(void)
{
	BOOL ok = TRUE;

	openal.library = dlopen(OPENAL_LIBRARY, RTLD_LAZY | RTLD_LOCAL);
	if (!openal.library)
		return FALSE;
	ok &= openal_symbol((void **)&openal.OpenDevice, "alcOpenDevice");
	ok &= openal_symbol((void **)&openal.CloseDevice, "alcCloseDevice");
	ok &= openal_symbol((void **)&openal.CreateContext, "alcCreateContext");
	ok &= openal_symbol((void **)&openal.MakeContextCurrent, "alcMakeContextCurrent");
	ok &= openal_symbol((void **)&openal.DestroyContext, "alcDestroyContext");
	ok &= openal_symbol((void **)&openal.GetProcAddress, "alcGetProcAddress");
	ok &= openal_symbol((void **)&openal.GetError, "alGetError");
	ok &= openal_symbol((void **)&openal.GenSources, "alGenSources");
	ok &= openal_symbol((void **)&openal.DeleteSources, "alDeleteSources");
	ok &= openal_symbol((void **)&openal.Sourcei, "alSourcei");
	ok &= openal_symbol((void **)&openal.Sourcef, "alSourcef");
	ok &= openal_symbol((void **)&openal.Source3f, "alSource3f");
	ok &= openal_symbol((void **)&openal.GetSourcei, "alGetSourcei");
	ok &= openal_symbol((void **)&openal.Getsourcef, "alGetSourcef");
	ok &= openal_symbol((void **)&openal.SourcePlay, "alSourcePlay");
	ok &= openal_symbol((void **)&openal.SourcePause, "alSourcePause");
	ok &= openal_symbol((void **)&openal.SourceStop, "alSourceStop");
	ok &= openal_symbol((void **)&openal.GenBuffers, "alGenBuffers");
	ok &= openal_symbol((void **)&openal.DeleteBuffers, "alDeleteBuffers");
	ok &= openal_symbol((void **)&openal.BufferData, "alBufferData");
	ok &= openal_symbol((void **)&openal.SourceQueueBuffers, "alSourceQueueBuffers");
	ok &= openal_symbol((void **)&openal.SourceUnqueueBuffers, "alSourceUnqueueBuffers");
	if (!ok)
	{
		dlclose(openal.library);
		openal.library = NULL;
	}
	return ok;
}

/* (audio_start, on the game's thread) */
static void openal_start(void)
{
	const char *requested = getenv("HALO_MAC_AUDIO_OPENAL");
	void (*rendering_quality)(ALint);

	openal.sources = openal.fallbacks = openal.buffers_queued = openal.buffer_failures = 0;
	/* OpenAL unless HALO_MAC_AUDIO_OPENAL=0 (the software mixer) */
	if (requested && requested[0] == '0' && requested[1] == '\0')
		return;
	if (!openal.library && !openal_load())
	{
		platform_log("audio: OpenAL unavailable; mixing in software");
		return;
	}
	openal.device = openal.OpenDevice(NULL);
	{
		/* HALO_MAC_AUDIO_MIXER_RATE=<hz> (a timing switch, off unless set): Apple's
		spatial mixer renders at this rate rather than the output device's, so a
		lower one costs it less (alcMacOSXMixerOutputRate) */
		const char *rate_text = getenv("HALO_MAC_AUDIO_MIXER_RATE");
		void (*mixer_rate)(const ALdouble) = openal.device && rate_text ?
			(void (*)(const ALdouble))openal.GetProcAddress(openal.device,
			(const ALCchar *)"alcMacOSXMixerOutputRate") : NULL;

		if (mixer_rate && atof(rate_text) >= 8000.0)
			mixer_rate((ALdouble)atof(rate_text));
	}
	openal.context = openal.device ? openal.CreateContext(openal.device, NULL) : NULL;
	if (!openal.context || !openal.MakeContextCurrent(openal.context))
	{
		platform_log("audio: OpenAL could not open the default output; mixing in software");
		if (openal.context)
			openal.DestroyContext(openal.context);
		if (openal.device)
			openal.CloseDevice(openal.device);
		openal.context = NULL;
		openal.device = NULL;
		return;
	}
	/* (Apple's extension, which needs a current context) */
	rendering_quality = (void (*)(ALint))openal.GetProcAddress(openal.device,
		(const ALCchar *)"alcMacOSXRenderingQuality");
	if (rendering_quality)
		rendering_quality((ALint)OPENAL_RENDERING_QUALITY_LOW);
	openal.ASASetListener = (ALenum (*)(const ALuint, ALvoid *, ALuint))openal.GetProcAddress(
		openal.device, (const ALCchar *)"alcASASetListener");
	openal.ASASetSource = (ALenum (*)(const ALuint, ALuint, ALvoid *, ALuint))openal.GetProcAddress(
		openal.device, (const ALCchar *)"alcASASetSource");
	openal.reverb_on = FALSE;
	openal.environment_serial = (unsigned long)-1;
	openal.sends_set = openal.obstructions_set = 0;
	openal.room_type = -1;
	if (openal.ASASetListener && openal.ASASetSource && reverb_enabled)
	{
		ALuint on = 1;
		ALint quality = OPENAL_ASA_REVERB_QUALITY_LOW;
		ALfloat level = 0.0f;

		if (openal.ASASetListener(OPENAL_ASA_REVERB_ON, &on, sizeof(on)) == AL_NO_ERROR)
		{
			openal.ASASetListener(OPENAL_ASA_REVERB_QUALITY, &quality, sizeof(quality));
			openal.ASASetListener(OPENAL_ASA_REVERB_GLOBAL_LEVEL, &level, sizeof(level));
			openal.reverb_on = TRUE;
		}
	}
	openal.GetError();
	openal.active = TRUE;
	platform_log("audio: voices play through OpenAL (%s)",
		openal.reverb_on ? "with Apple's reverb and obstruction" :
		openal.ASASetSource ? "with Apple's obstruction, no reverb" : "dry");
}

/* the ASA room type nearest the I3DL2 environment's decay time (seconds) */
static ALint openal_room_type(float decay_time)
{
	if (decay_time < 0.7f)
		return 0; /* SmallRoom */
	if (decay_time < 1.2f)
		return 1; /* MediumRoom */
	if (decay_time < 1.8f)
		return 2; /* LargeRoom */
	if (decay_time < 2.5f)
		return 3; /* MediumHall */
	if (decay_time < 3.5f)
		return 4; /* LargeHall */
	return 8; /* Cathedral */
}

/* (a stream's source and buffers, with the lock held or the stream unlisted) */
static void openal_stream_stop(struct sdl_stream *stream)
{
	unsigned long slot;

	if (!stream->openal_source)
		return;
	openal.SourceStop(stream->openal_source);
	/* (a stopped source lets go of all its buffers) */
	openal.Sourcei(stream->openal_source, AL_BUFFER, 0);
	for (slot = 0; slot < MAXIMUM_STREAM_PACKETS; slot++)
	{
		if (stream->openal_buffers[slot])
		{
			openal.DeleteBuffers(1, &stream->openal_buffers[slot]);
			stream->openal_buffers[slot] = 0;
		}
	}
	openal.GetError();
}

static void openal_stream_delete(struct sdl_stream *stream)
{
	if (!stream->openal_source)
		return;
	openal_stream_stop(stream);
	openal.DeleteSources(1, &stream->openal_source);
	openal.GetError();
	stream->openal_source = 0;
	openal.sources--;
}

/* (audio_stop: every stream goes back to the software mixer) */
static void openal_stop(void)
{
	struct sdl_stream *stream;

	if (!openal.active)
		return;
	pthread_mutex_lock(&mixer_lock);
	for (stream = streams; stream; stream = stream->next)
	{
		openal_stream_delete(stream);
		stream->openal_failed = FALSE;
	}
	openal.active = FALSE;
	pthread_mutex_unlock(&mixer_lock);
	openal.MakeContextCurrent(NULL);
	openal.DestroyContext(openal.context);
	openal.CloseDevice(openal.device);
	openal.context = NULL;
	openal.device = NULL;
}

/* gains, pan and pitch, from what the software mixer would use */
static void openal_stream_parameters(struct sdl_stream *stream)
{
	float left, right, room, direct_lowpass, room_lowpass, gain, pitch;
	ALuint source = stream->openal_source;

	voice_gains(stream, &left, &right, &room, &direct_lowpass, &room_lowpass);
	if (stream->channels == 1)
	{
		/* the equal power pan the gains make, from -1 (left) to 1 (right),
		as an azimuth a source at unit distance takes */
		float pan, azimuth;

		gain = sqrtf(left * left + right * right);
		pan = gain > 0.0f ? atan2f(right, left) * (4.0f / 3.14159265f) - 1.0f : 0.0f;
		azimuth = pan * (3.14159265f / 2.0f);
		openal.Source3f(source, AL_POSITION, sinf(azimuth), 0.0f, -cosf(azimuth));
	}
	else
	{
		gain = 0.5f * (left + right);
	}
	openal.Sourcef(source, AL_GAIN, gain);
	if (openal.ASASetSource)
	{
		/* the reverb send: the software mixer's room gain (the
		environment's room level and the source's, rolled off with
		distance) without the volume the gain already has */
		float level = stream->volume * master_volume;
		ALfloat send = openal.reverb_on && room > 0.0f && level > 0.0f ? room / level : 0.0f;
		ALfloat obstruction = 0.0f;

		if (send > 1.0f)
			send = 1.0f;
		/* the direct path's high frequency loss, in decibels, where the
		software mixer muffles it */
		if (((stream->has_3d && stream->mode != DS3DMODE_DISABLE) ||
				(stream->channels == 2 && stream->stereo_positioned)) &&
			stream->direct_hf < stream->direct)
		{
			obstruction = (ALfloat)(stream->direct_hf - stream->direct) / 100.0f;
			if (obstruction < -100.0f)
				obstruction = -100.0f;
		}
		if (openal.reverb_on && send != stream->openal_send)
		{
			openal.ASASetSource(OPENAL_ASA_REVERB_SEND_LEVEL, source, &send, sizeof(send));
			stream->openal_send = send;
			openal.sends_set += send > 0.0f;
		}
		if (obstruction != stream->openal_obstruction)
		{
			openal.ASASetSource(OPENAL_ASA_OBSTRUCTION, source, &obstruction, sizeof(obstruction));
			stream->openal_obstruction = obstruction;
			openal.obstructions_set += obstruction < 0.0f;
		}
	}
	pitch = stream->sample_rate ?
		(float)(stream->frequency ? stream->frequency : stream->sample_rate) / (float)stream->sample_rate : 1.0f;
	if (pitch < OPENAL_MINIMUM_PITCH)
		pitch = OPENAL_MINIMUM_PITCH;
	if (pitch > OPENAL_MAXIMUM_PITCH)
		pitch = OPENAL_MAXIMUM_PITCH;
	openal.Sourcef(source, AL_PITCH, pitch);
}

/* plays the source if it has buffers and is not paused, and pauses it if
it is */
static void openal_stream_state(struct sdl_stream *stream)
{
	ALint state = AL_STOPPED, queued = 0;

	openal.GetSourcei(stream->openal_source, AL_SOURCE_STATE, &state);
	openal.GetSourcei(stream->openal_source, AL_BUFFERS_QUEUED, &queued);
	if (stream->paused)
	{
		if (state == AL_PLAYING)
			openal.SourcePause(stream->openal_source);
	}
	else if (queued > 0 && state != AL_PLAYING)
	{
		openal.SourcePlay(stream->openal_source);
	}
}

/* the packets whose buffers OpenAL has played are finished. (Played buffers
stay queued until taken off, and a stopped source played again would play
them over, so this comes before anything is queued.) */
static void openal_stream_reclaim(struct sdl_stream *stream)
{
	ALint processed = 0;

	openal.GetSourcei(stream->openal_source, AL_BUFFERS_PROCESSED, &processed);
	while (processed-- > 0)
	{
		ALuint buffer = 0;
		unsigned long slot;

		openal.SourceUnqueueBuffers(stream->openal_source, 1, &buffer);
		if (!buffer)
			break;
		for (slot = 0; slot < MAXIMUM_STREAM_PACKETS; slot++)
		{
			if (stream->openal_buffers[slot] == buffer)
			{
				stream->openal_buffers[slot] = 0;
				stream->packets[slot].finished = TRUE;
				break;
			}
		}
		openal.DeleteBuffers(1, &buffer);
	}
}

/* (stream_process, with the lock held) a new packet: an idle stream takes a
source, and the packet's frames are queued on it */
static void openal_stream_queue(struct sdl_stream *stream, struct voice_packet *entry)
{
	unsigned long slot = (unsigned long)(entry - stream->packets);
	ALuint buffer = 0;

	if (!openal.active)
		return;
	if (!stream->openal_source)
	{
		/* (only between sounds: frames the software mixer has begun stay
		with it) */
		if (stream->openal_failed || stream->packet_count || !stream->sample_rate)
			return;
		openal.GetError();
		openal.GenSources(1, &stream->openal_source);
		if (openal.GetError() != AL_NO_ERROR || !stream->openal_source)
		{
			stream->openal_source = 0;
			stream->openal_failed = TRUE;
			openal.fallbacks++;
			return;
		}
		openal.sources++;
		openal.Sourcei(stream->openal_source, AL_SOURCE_RELATIVE, AL_TRUE);
		openal.Sourcef(stream->openal_source, AL_ROLLOFF_FACTOR, 0.0f);
		openal.Source3f(stream->openal_source, AL_POSITION, 0.0f, 0.0f, -1.0f);
		/* (ASA's defaults: dry, unobstructed) */
		stream->openal_send = 0.0f;
		stream->openal_obstruction = 0.0f;
	}
	openal_stream_reclaim(stream);
	if (!entry->frames)
	{
		/* (nothing to play: finished, completed in turn) */
		entry->finished = TRUE;
		return;
	}
	/* HALO_MAC_AUDIO_DUMP=<file> (diagnostic): the 16-bit samples given to
	OpenAL, in this host's byte order, appended (channels and rate logged once) */
	{
		static FILE *dump;
		static int dump_checked, dump_logged;
		if (!dump_checked)
		{
			const char *path = getenv("HALO_MAC_AUDIO_DUMP");
			dump_checked = 1;
			if (path)
				dump = fopen(path, "wb");
		}
		if (dump)
		{
			if (dump_logged < 4)
			{
				fprintf(stderr, "audio dump: %lu channels at %lu Hz, %lu frames\n",
					(unsigned long)stream->channels, (unsigned long)stream->sample_rate,
					(unsigned long)entry->frames);
				dump_logged++;
			}
			fwrite(entry->samples, sizeof(short), (size_t)(entry->frames * stream->channels), dump);
		}
	}
	openal.GetError();
	openal.GenBuffers(1, &buffer);
	if (openal.GetError() == AL_NO_ERROR && buffer)
	{
		openal.BufferData(buffer, stream->channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
			entry->samples, (ALsizei)(entry->frames * stream->channels * sizeof(short)),
			(ALsizei)stream->sample_rate);
		if (openal.GetError() == AL_NO_ERROR)
		{
			openal.SourceQueueBuffers(stream->openal_source, 1, &buffer);
			if (openal.GetError() == AL_NO_ERROR)
			{
				stream->openal_buffers[slot] = buffer;
				openal.buffers_queued++;
				openal_stream_parameters(stream);
				{
					/* HALO_MAC_AUDIO_TRACE=1 (diagnostic): each packet queued, with
					the source's gain, pitch and reverb send as set */
					static int trace_checked, trace_on;
					if (!trace_checked)
					{
						trace_on = getenv("HALO_MAC_AUDIO_TRACE") != NULL;
						trace_checked = 1;
					}
					if (trace_on)
					{
						ALfloat g = -1.0f, p = -1.0f;
						ALint st = -1, q = -1;
						openal.Getsourcef(stream->openal_source, AL_GAIN, &g);
						openal.Getsourcef(stream->openal_source, AL_PITCH, &p);
						openal.GetSourcei(stream->openal_source, AL_SOURCE_STATE, &st);
						openal.GetSourcei(stream->openal_source, AL_BUFFERS_QUEUED, &q);
						fprintf(stderr, "audio pkt src=%u ch=%lu rate=%lu frames=%lu gain=%.4f pitch=%.3f vol=%.3f send=%.3f state=%x queued=%d\n",
							(unsigned)stream->openal_source, (unsigned long)stream->channels,
							(unsigned long)stream->sample_rate, (unsigned long)entry->frames,
							g, p, stream->volume, stream->openal_send, (unsigned)st, (int)q);
					}
				}
				openal_stream_state(stream);
				return;
			}
		}
		openal.DeleteBuffers(1, &buffer);
	}
	/* (a buffer OpenAL would not take is skipped, as a failed decode is) */
	openal.buffer_failures++;
	entry->finished = TRUE;
}

/* (DirectSoundDoWork, each frame) the packets OpenAL has played are
finished, and every source takes its stream's parameters */
static void openal_update(void)
{
	struct sdl_stream *stream;

	if (!openal.active)
		return;
	pthread_mutex_lock(&mixer_lock);
	if (openal.reverb_on && openal.environment_serial != environment_serial)
	{
		ALint room_type = openal_room_type(environment.flDecayTime);

		openal.ASASetListener(OPENAL_ASA_REVERB_ROOM_TYPE, &room_type, sizeof(room_type));
		openal.environment_serial = environment_serial;
		openal.room_type = room_type;
	}
	for (stream = streams; stream; stream = stream->next)
	{
		if (!stream->openal_source)
			continue;
		openal_stream_reclaim(stream);
		if (stream->packet_count)
		{
			openal_stream_parameters(stream);
			openal_stream_state(stream);
		}
	}
	openal.GetError();
	pthread_mutex_unlock(&mixer_lock);
}
