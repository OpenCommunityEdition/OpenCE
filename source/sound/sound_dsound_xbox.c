/*
SOUND_DSOUND_XBOX.C
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "cache/physical_memory_map.h"
#include "cache/sound_cache.h"
#include "math/real_math.h"
#include "render/render_debug.h"
#include "sound/game_sound.h"
#include "sound/sound_definitions.h"
#include "sound/sound_dsound.h"
#include "sound/sound_environment_definitions.h"
#include "sound/sound_preferences.h"
#include "text/draw_string.h"

#include <xtl.h>

/* ---------- constants */

enum
{
	NUMBER_OF_SOUND_SAMPLE_RATES = 2,
	MAXIMUM_SOUND_CHANNELS = 256,
	NUMBER_OF_SOUND_CHANNEL_TYPES = 4,
	MAXIMUM_DSOUND_ERROR_STRING_LENGTH = 256,
	MAXIMUM_DSOUND_ERROR_MESSAGE_LENGTH = 4096,
	MAXIMUM_SOUND_PACKETS = 4,
	MAXIMUM_SOUND_PACKET_SIZE = 8192,
	MAXIMUM_COMPRESSED_SOUND_PACKET_SIZE = 2304,
	SOUND_CACHE_SIZE = 0x400000,
	SOUND_COMPRESSED_BLOCK_SIZE = 36,
	SOUND_COMPRESSED_SAMPLES_PER_BLOCK = 64,
	MAXIMUM_DSOUND_MIXBINS = 8
};

enum
{
	_sound_channel_3d_bit,
	_sound_channel_stereo_bit,
	_sound_channel_44k_bit,
	_sound_channel_compressed_bit
};

enum sound_channel_state
{
	_sound_channel_idle,
	_sound_channel_playing,
	_sound_channel_queued,
	NUMBER_OF_SOUND_CHANNEL_STATES
};

/* ---------- macros */

#define cache_base_address unknown1

/* ---------- structures */

struct sound_virtual_channel
{
	short channel_index;
	short type_index;
};

struct platform_sound_listener_properties
{
	real_point3d position;
	real_vector3d forward;
	real_vector3d up;
	real_vector3d velocity;
	struct sound_environment_definition const *environment;
};

struct platform_sound_channel_properties
{
	real minimum_distance;
	real maximum_distance;
	real pitch;
	real gain;
	real cone_inside_angle;
	real cone_outside_angle;
	real cone_outside_gain;
	real reverb_attenuation;
};

struct sound_channel
{
	short state;
	short virtual_channel_index;
	boolean spatialized;
	boolean attenuate_direct_path;
	boolean stopping;
	byte reserved007[1];
	short packet_count;
	byte reserved00a[2];
	real_point3d position;
	real_vector3d forward;
	real_vector3d translational_velocity;
	byte reserved030[8];
	short type_flags;
	byte reserved03a[2];
	real gain;
	real pitch;
	real occlusion;
	real obstruction;
	real minimum_distance;
	real maximum_distance;
	real cone_outside_gain;
	real cone_inside_angle;
	real cone_outside_angle;
	real reverb_attenuation;
	long sample_offset;
	struct sound_permutation *playing_permutation;
	struct sound_permutation *queued_permutation;
	LPDIRECTSOUNDSTREAM stream;
};

typedef char sound_channel_gain_offset_assert[
	offsetof(struct sound_channel, gain) == 0x3C ? 1 : -1];
typedef char sound_channel_sample_offset_offset_assert[
	offsetof(struct sound_channel, sample_offset) == 0x64 ? 1 : -1];
typedef char sound_channel_type_flags_offset_assert[
	offsetof(struct sound_channel, type_flags) == 0x38 ? 1 : -1];
typedef char sound_channel_stream_offset_assert[
	offsetof(struct sound_channel, stream) == 0x70 ? 1 : -1];

typedef char sound_channel_size_assert[
	sizeof(struct sound_channel) == 0x74 ? 1 : -1];

struct dsound_globals
{
	boolean initialized;
	short virtual_channel_count;
	struct sound_virtual_channel virtual_channels[MAXIMUM_SOUND_CHANNELS];
	short actual_channel_count;
	byte reserved406[2];
	struct sound_channel channels[MAXIMUM_SOUND_CHANNELS];
	short type_first_channel_index[NUMBER_OF_SOUND_CHANNEL_TYPES];
	real_point3d listener_position;
	real_vector3d listener_forward;
	real_vector3d listener_up;
	real_vector3d listener_velocity;
	byte reserved7840[4];
	struct sound_environment_definition listener_environment;
	DSCAPS caps;
	LPDIRECTSOUND direct_sound;
	LPDIRECTSOUNDBUFFER inanity_buffer;
	short inanity_samples[16];
	boolean paused;
	byte reserved78c5[3];
	real pause_gain;
};

typedef char dsound_globals_type_first_channel_index_offset_assert[
	offsetof(struct dsound_globals, type_first_channel_index) == 0x7808 ? 1 : -1];
typedef char dsound_globals_direct_sound_offset_assert[
	offsetof(struct dsound_globals, direct_sound) == 0x789C ? 1 : -1];
typedef char dsound_globals_paused_offset_assert[
	offsetof(struct dsound_globals, paused) == 0x78C4 ? 1 : -1];
typedef char dsound_globals_pause_gain_offset_assert[
	offsetof(struct dsound_globals, pause_gain) == 0x78C8 ? 1 : -1];

struct sound_platform_definition
{
	short platform_code;
	byte reserved002[2];
	boolean (*initialize)(
		struct sound_preferences *preferences);
	void (*dispose)(
		void);
	void (*set_listener_properties)(
		struct platform_sound_listener_properties const *properties);
	void (*begin_scene)(
		void);
	void (*end_scene)(
		void);
	void (*queue_sound_to_channel)(
		short channel_index,
		struct sound_permutation *sound);
	void (*channel_update)(
		short channel_index);
	void (*stop_channel)(
		short channel_index);
	short (*get_channel_state)(
		short channel_index);
	void (*set_pause)(
		boolean paused);
	void (*flush)(
		void);
	void (*set_channel_location)(
		short channel_index,
		boolean spatialized,
		struct sound_location const *location,
		real occlusion,
		real obstruction,
		boolean attenuate_direct_path);
	void (*set_channel_properties)(
		short channel_index,
		struct platform_sound_channel_properties const *properties,
		boolean gain_only);
	real direct_path_gain;
};

typedef char sound_platform_definition_direct_path_gain_offset_assert[
	offsetof(struct sound_platform_definition, direct_path_gain) == 0x38 ? 1 : -1];

/* ---------- prototypes */

void __stdcall DirectSoundStopStream(
	LPDIRECTSOUNDSTREAM stream);
unsigned long __stdcall DirectSoundGetStreamVoiceStatus(
	LPDIRECTSOUNDSTREAM stream);

static void dsound_error(
	HRESULT result,
	char const *format,
	...);
static void channel_stop(
	short index);
static boolean channel_finish_stopping(
	short index);
static void dsound_record_error(
	HRESULT *result,
	char const *message);
static boolean channel_queue_packet(
	short index);
static void channel_submit_packets(
	short index);
static void channel_queue_sound(
	short index,
	struct sound_permutation *sound);
static void channel_set_i3dl2_source(
	short index);
static void channel_set_properties(
	short channel_index,
	struct platform_sound_channel_properties const *properties,
	boolean gain_only);
static void channel_set_location(
	short channel_index,
	boolean spatialized,
	struct sound_location const *location,
	real occlusion,
	real obstruction,
	boolean attenuate_direct_path);
static boolean channel_new(
	short channel_index,
	short type_flags);
static void CALLBACK channel_packet_callback(
	void *stream_context,
	void *packet_context,
	unsigned long status);
static boolean virtual_channel_new(
	short virtual_channel_index,
	short type_index);
static boolean create_inanity_channel(
	void);
static void virtual_channel_acquire_channel(
	short virtual_channel_index);
static short virtual_channel_get_channel_index(
	short virtual_channel_index);
static struct sound_channel *channel_get(
	short index);
static struct sound_virtual_channel *virtual_channel_get(
	short index);
static short channel_get_state(
	short index);

/* ---------- globals */

extern struct dsound_globals dsound_globals;
extern unsigned long const sound_sample_rate_samples_per_second[NUMBER_OF_SOUND_SAMPLE_RATES];
extern HRESULT interrupt_result;
extern boolean debug_sound_channels;
extern byte const dsound_effects_image[0x3A5C];

struct sound_platform_definition platform_sound_dsound =
{
	0,
	{ 0, 0 },
	dsound_initialize,
	dsound_dispose,
	dsound_set_listener_properties,
	dsound_begin_scene,
	dsound_end_scene,
	dsound_virtual_queue_sound,
	dsound_channel_update,
	dsound_virtual_stop,
	dsound_virtual_get_state,
	dsound_set_paused,
	dsound_flush,
	dsound_virtual_set_location,
	dsound_virtual_set_properties,
	0.25f,
};

static char dsound_error_string[MAXIMUM_DSOUND_ERROR_STRING_LENGTH+1];

/* ---------- public code */

unsigned long sound_samples_per_second(
	short sample_rate)
{
	match_assert(
		"c:\\halo\\source\\sound\\sound_definitions.h",
		0x135,
		sample_rate>=0 && sample_rate<NUMBER_OF_SOUND_SAMPLE_RATES);

	return sound_sample_rate_samples_per_second[sample_rate];
}

long dsound_volume_from_gain(
	real gain,
	long maximum_volume)
{
	long volume= maximum_volume;

	match_assert(
		"c:\\halo\\source\\sound\\sound_dsound.h",
		35,
		gain>=0.f && gain<=1.f);

	if (gain==0.f)
	{
		volume= DSBVOLUME_MIN;
	}
	else
	{
		long attenuation= (long)(2000.0*log10(gain) + maximum_volume);

		volume= PIN(attenuation, DSBVOLUME_MIN, volume);
	}

	return volume;
}

long dsound_frequency_from_pitch(
	long samples_per_second,
	real pitch)
{
	real frequency;

	match_assert(
		"c:\\halo\\source\\sound\\sound_dsound.h",
		54,
		samples_per_second==22050 || samples_per_second==44100);

	frequency= samples_per_second*pitch;

	if (frequency<DSBFREQUENCY_MIN)
	{
		frequency= DSBFREQUENCY_MIN;
	}
	else if (frequency>DSBFREQUENCY_MAX)
	{
		frequency= DSBFREQUENCY_MAX;
	}

	return (long)frequency;
}

long dsound_angle_from_angle(
	real angle)
{
	return (long)(angle * 57.29578f);
}

long dsound_occlusion_from_occlusion(
	real occlusion)
{
	return dsound_volume_from_gain(1.f-occlusion, 0);
}

long dsound_obstruction_from_obstruction(
	real obstruction)
{
	return dsound_volume_from_gain(1.f-obstruction, 0);
}

LPDIRECTSOUND dsound_get(
	void)
{
	return dsound_globals.initialized ? dsound_globals.direct_sound : NULL;
}

short dsound_virtual_get_state(
	short virtual_channel_index)
{
	struct sound_virtual_channel *vchannel= virtual_channel_get(virtual_channel_index);
	short state;

	if (vchannel->channel_index!=NONE)
	{
		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
			1527,
			channel_get(vchannel->channel_index)->virtual_channel_index==virtual_channel_index);

		state= channel_get_state(vchannel->channel_index);
	}
	else
	{
		state= _sound_channel_idle;
	}

	return state;
}

void dsound_virtual_stop(
	short virtual_channel_index)
{
	struct sound_virtual_channel *vchannel= virtual_channel_get(virtual_channel_index);

	if (vchannel->channel_index!=NONE)
	{
		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
			1505,
			channel_get(vchannel->channel_index)->virtual_channel_index==virtual_channel_index);

		channel_stop(vchannel->channel_index);

		channel_get(vchannel->channel_index)->virtual_channel_index= NONE;
		vchannel->channel_index= NONE;
	}

	return;
}

void dsound_channel_update(
	short channel_index)
{
	return;
}

void dsound_begin_scene(
	void)
{
	DirectSoundDoWork();

	if (strlen(dsound_error_string))
	{
		dsound_error(interrupt_result, dsound_error_string);
	}

	dsound_error_string[0]= 0;
	interrupt_result= 0;

	return;
}

void dsound_dispose(
	void)
{
	long index;

	for (index= 0; index<dsound_globals.actual_channel_count; index++)
	{
		struct sound_channel *channel= channel_get(index);

		if (channel->stream)
		{
			IDirectSoundStream_Release(channel->stream);
		}
	}

	if (dsound_globals.inanity_buffer)
	{
		IDirectSoundBuffer_Release(dsound_globals.inanity_buffer);
	}

	dsound_globals.actual_channel_count= 0;
	dsound_globals.virtual_channel_count= 0;

	if (dsound_globals.direct_sound)
	{
		IDirectSound_Release(dsound_globals.direct_sound);

		dsound_globals.direct_sound= NULL;
	}

	dsound_globals.initialized= FALSE;

	return;
}

void dsound_end_scene(
	void)
{
	HRESULT result= IDirectSound_CommitDeferredSettings(dsound_globals.direct_sound);

	if (result<0)
	{
		dsound_error(result, "couldn't commit deferred settings.");
	}

	if (dsound_globals.paused ?
		dsound_globals.pause_gain!=0.f :
		dsound_globals.pause_gain!=1.f)
	{
		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
			634,
			dsound_globals.pause_gain>=0 && dsound_globals.pause_gain<=1.f);

		if (dsound_globals.paused)
		{
			dsound_globals.pause_gain= (real)MAX(0.0, dsound_globals.pause_gain-0.15);
		}
		else
		{
			dsound_globals.pause_gain= (real)MIN(1.0, dsound_globals.pause_gain+0.15);
		}

		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
			645,
			dsound_globals.pause_gain>=0 && dsound_globals.pause_gain<=1.f);

		if (dsound_globals.paused)
		{
			short index;

			for (index= 0; index<dsound_globals.actual_channel_count; index++)
			{
				struct sound_channel *channel= channel_get(index);

				if (channel->state!=_sound_channel_idle)
				{
					IDirectSoundStream_SetVolume(
						channel->stream,
						dsound_volume_from_gain(dsound_globals.pause_gain*channel->gain, 0));
				}
			}
		}
	}

	if (debug_sound_channels)
	{
		char string[0x2000];
		short tab_stops[3]= {280};
		short index;

		draw_string_set_tab_stops(tab_stops, 1);

		string[0]= 0;

		for (index= 0; index<dsound_globals.actual_channel_count; index++)
		{
			struct sound_channel *channel= channel_get(index);

			if (index<16 || index>48)
			{
				if (channel->state!=_sound_channel_idle)
				{
					sprintf(
						string+strlen(string),
						"%d %1.2f %1.2f %s(%s)",
						channel->packet_count,
						channel->gain,
						channel->pitch,
						channel->playing_permutation ? channel->playing_permutation->name : "",
						channel->queued_permutation ? channel->queued_permutation->name : "");
				}

				sprintf(string+strlen(string), "|t");

				if (index&1)
				{
					sprintf(string+strlen(string), "|n");
				}
			}
		}

		render_debug_string(FALSE, string);
	}

	return;
}

void dsound_set_paused(
	boolean paused)
{
	short index;

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		728,
		paused!=dsound_globals.paused);

	if (paused)
	{
		for (index= 0; index<dsound_globals.actual_channel_count; index++)
		{
			struct sound_channel *channel= channel_get(index);

			if (channel->stopping)
			{
				while (!channel_finish_stopping(index))
					;
			}
		}
	}
	else
	{
		for (index= 0; index<dsound_globals.actual_channel_count; index++)
		{
			struct sound_channel *channel= channel_get(index);

			if (channel->state!=_sound_channel_idle)
			{
				IDirectSoundStream_Flush(channel->stream);

				channel->sample_offset= (long)(channel->sample_offset-
					(TEST_FLAG(channel->type_flags, _sound_channel_44k_bit) ? 2.f : 1.f)*
					(TEST_FLAG(channel->type_flags, _sound_channel_stereo_bit) ? 2.f : 1.f)*
					(TEST_FLAG(channel->type_flags, _sound_channel_compressed_bit) ?
						MAXIMUM_COMPRESSED_SOUND_PACKET_SIZE : MAXIMUM_SOUND_PACKET_SIZE)*
					(real)MAXIMUM_SOUND_PACKETS);

				channel->gain= 0.f;

				channel->sample_offset= FLOOR(channel->sample_offset, 0);
			}
		}

		DirectSoundDoWork();

		for (index= 0; index<dsound_globals.actual_channel_count; index++)
		{
			struct sound_channel *channel= channel_get(index);

			if (channel->packet_count!=0)
			{
				error(
					_error_silent,
					"DirectSound: you're screwed if you try to save and quit -- the devil.");

				channel->packet_count= 0;
			}

			if (channel->state!=_sound_channel_idle)
			{
				channel_submit_packets(index);
			}
		}
	}

	dsound_globals.paused= paused;

	return;
}

void dsound_flush(
	void)
{
	short index;

	for (index= 0; index<dsound_globals.actual_channel_count; index++)
	{
		struct sound_channel *channel= channel_get(index);

		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
			706,
			channel->stopping || channel->state==_sound_channel_idle);

		if (channel->stopping)
		{
			while (!channel_finish_stopping(index))
				;
		}

		if (channel->packet_count!=0)
		{
			error(
				_error_silent,
				"DirectSound: you're screwed if you try to save and quit -- the devil.");

			channel->packet_count= 0;
		}
	}

	return;
}

void dsound_set_listener_properties(
	struct platform_sound_listener_properties const *properties)
{
	if (!(0.05f>fabs(properties->position.x-dsound_globals.listener_position.x)) ||
		!(0.05f>fabs(properties->position.y-dsound_globals.listener_position.y)) ||
		!(0.05f>fabs(properties->position.z-dsound_globals.listener_position.z)) ||
		!dsound_globals.initialized)
	{
		HRESULT result= IDirectSound_SetPosition(
			dsound_globals.direct_sound,
			properties->position.x,
			properties->position.z,
			properties->position.y,
			DS3D_DEFERRED);

		if (result<0)
		{
			dsound_error(result, "couldn't set listener position.");
		}

		dsound_globals.listener_position= properties->position;
	}

	if (!(0.05f>fabs(properties->forward.i-dsound_globals.listener_forward.i)) ||
		!(0.05f>fabs(properties->forward.j-dsound_globals.listener_forward.j)) ||
		!(0.05f>fabs(properties->forward.k-dsound_globals.listener_forward.k)) ||
		!(0.05f>fabs(properties->up.i-dsound_globals.listener_up.i)) ||
		!(0.05f>fabs(properties->up.j-dsound_globals.listener_up.j)) ||
		!(0.05f>fabs(properties->up.k-dsound_globals.listener_up.k)) ||
		!dsound_globals.initialized)
	{
		HRESULT result= IDirectSound_SetOrientation(
			dsound_globals.direct_sound,
			properties->forward.i,
			properties->forward.k,
			properties->forward.j,
			properties->up.i,
			properties->up.k,
			properties->up.j,
			DS3D_DEFERRED);

		if (result<0)
		{
			dsound_error(result, "couldn't set listener orientation.");
		}

		dsound_globals.listener_forward= properties->forward;
		dsound_globals.listener_up= properties->up;
	}

	if (!(0.01f>fabs(properties->velocity.i-dsound_globals.listener_velocity.i)) ||
		!(0.01f>fabs(properties->velocity.j-dsound_globals.listener_velocity.j)) ||
		!(0.01f>fabs(properties->velocity.k-dsound_globals.listener_velocity.k)) ||
		!dsound_globals.initialized)
	{
		HRESULT result= IDirectSound_SetVelocity(
			dsound_globals.direct_sound,
			properties->velocity.i,
			properties->velocity.k,
			properties->velocity.j,
			DS3D_DEFERRED);

		if (result<0)
		{
			dsound_error(result, "couldn't set listener velocity.");
		}

		dsound_globals.listener_velocity= properties->velocity;
	}

	if (csmemcmp(properties->environment, &dsound_globals.listener_environment,
		sizeof(struct sound_environment_definition))!=0 ||
		!dsound_globals.initialized)
	{
		struct sound_environment_definition const *environment= properties->environment;
		DSI3DL2LISTENER i3dl2;

		dsound_globals.listener_environment= *environment;

		i3dl2.lRoom= dsound_volume_from_gain(environment->room_intensity, 0);
		i3dl2.lRoomHF= dsound_volume_from_gain(environment->room_intensity_hf, 0);
		i3dl2.flRoomRolloffFactor= environment->room_rolloff;
		i3dl2.flDecayTime= environment->decay_time;
		i3dl2.flDecayHFRatio= environment->decay_hf_ratio;
		i3dl2.lReflections= dsound_volume_from_gain(environment->reflections_intensity, 1000);
		i3dl2.flReflectionsDelay= environment->reflections_delay;
		i3dl2.lReverb= dsound_volume_from_gain(environment->reverb_intensity, 2000);
		i3dl2.flReverbDelay= environment->reverb_delay;
		i3dl2.flDiffusion= environment->diffusion*100.f;
		i3dl2.flDensity= environment->density*100.f;
		i3dl2.flHFReference= environment->hf_reference;

		IDirectSound_SetI3DL2Listener(dsound_globals.direct_sound, &i3dl2, DS3D_DEFERRED);
	}

	return;
}

boolean dsound_initialize(
	struct sound_preferences *preferences)
{
	boolean success= FALSE;
	HRESULT result;

	dsound_globals.initialized= FALSE;
	dsound_globals.paused= FALSE;
	dsound_globals.pause_gain= 1.f;
	dsound_globals.inanity_buffer= NULL;

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		234,
		preferences);

	result= DirectSoundCreate(NULL, &dsound_globals.direct_sound, NULL);

	if (result>=0)
	{
		DSCAPS caps;

		result= IDirectSound_GetCaps(dsound_globals.direct_sound, &caps);

		if (result>=0)
		{
			dsound_globals.caps= caps;

			result= IDirectSound_SetDistanceFactor(
				dsound_globals.direct_sound,
				3.048f,
				DS3D_IMMEDIATE);

			if (result>=0)
			{
				result= IDirectSound_SetRolloffFactor(
					dsound_globals.direct_sound,
					1.f,
					DS3D_IMMEDIATE);

				if (result>=0)
				{
					struct platform_sound_listener_properties listener_properties;
					short type_index;
					short virtual_channel_index;
					short channel_index;

					csmemset(&listener_properties, 0, sizeof(listener_properties));

					listener_properties.forward= *global_forward3d;
					listener_properties.up= *global_up3d;
					listener_properties.environment= &default_sound_environment;

					{
						DSEFFECTIMAGELOC image_location;
						LPDSEFFECTIMAGEDESC image_description;

						image_location.dwI3DL2ReverbIndex= 0;
						image_location.dwCrosstalkIndex= 1;

						result= IDirectSound_DownloadEffectsImage(
							dsound_globals.direct_sound,
							dsound_effects_image,
							sizeof(dsound_effects_image),
							&image_location,
							&image_description);

						if (result<0)
						{
							dsound_error(result, "could not download effects image.");
						}
					}

					IDirectSound_SetMixBinHeadroom(
						dsound_globals.direct_sound,
						DSMIXBIN_SPEAKER_MASK|DSMIXBIN_XTLK_MASK|DSMIXBIN_I3DL2|DSMIXBIN_FXSEND_MASK,
						0);

					DirectSoundUseFullHRTF();

					dsound_set_listener_properties(&listener_properties);

					success= TRUE;
					virtual_channel_index= 0;

					for (type_index= 0; type_index<NUMBER_OF_SOUND_CHANNEL_TYPES; type_index++)
					{
						short index;

						for (index= 0; index<preferences->virtual_channel_counts[type_index]; index++)
						{
							dsound_globals.virtual_channel_count++;

							success= success && virtual_channel_new(virtual_channel_index++, type_index);
						}
					}

					channel_index= 0;

					for (type_index= 0; type_index<NUMBER_OF_SOUND_CHANNEL_TYPES; type_index++)
					{
						short index;

						dsound_globals.type_first_channel_index[type_index]= channel_index;

						for (index= 0; index<preferences->actual_channel_counts[type_index]; index++)
						{
							dsound_globals.actual_channel_count++;

							success= success && channel_new(channel_index++, sound_channel_type_flags[type_index]);
						}
					}

					if (success && create_inanity_channel())
					{
						success= TRUE;
						dsound_globals.initialized= TRUE;
					}
					else
					{
						success= FALSE;

						dsound_dispose();
					}
				}
				else
				{
					dsound_error(result, "could not adjust rolloff factor");

					dsound_dispose();
				}
			}
			else
			{
				dsound_error(result, "could not adjust distance factor");

				dsound_dispose();
			}
		}
		else
		{
			dsound_error(result, "could not get caps for sound card?");

			dsound_dispose();
		}
	}
	else
	{
		dsound_error(result, "could not create direct sound object");

		dsound_dispose();
	}

	return success;
}

void dsound_virtual_set_location(
	short virtual_channel_index,
	boolean spatialized,
	struct sound_location const *location,
	real occlusion,
	real obstruction,
	boolean attenuate_direct_path)
{
	short channel_index= virtual_channel_get_channel_index(virtual_channel_index);

	if (channel_index!=NONE)
	{
		channel_set_location(
			channel_index,
			spatialized,
			location,
			occlusion,
			obstruction,
			attenuate_direct_path);
	}

	return;
}

void dsound_virtual_set_properties(
	short virtual_channel_index,
	struct platform_sound_channel_properties const *properties,
	boolean gain_only)
{
	short channel_index= virtual_channel_get_channel_index(virtual_channel_index);

	if (channel_index!=NONE)
	{
		channel_set_properties(channel_index, properties, gain_only);
	}

	return;
}

void dsound_virtual_queue_sound(
	short virtual_channel_index,
	struct sound_permutation *sound)
{
	short channel_index= virtual_channel_get_channel_index(virtual_channel_index);

	if (channel_index!=NONE)
	{
		channel_queue_sound(channel_index, sound);
	}

	return;
}

/* ---------- private code */

static void channel_set_i3dl2_source(
	short index)
{
	struct sound_channel *channel= channel_get(index);
	DSI3DL2BUFFER i3dl2;

	csmemset(&i3dl2, 0, sizeof(i3dl2));

	i3dl2.lDirect= 0;
	i3dl2.lDirectHF= 0;
	i3dl2.flRoomRolloffFactor= 0.f;
	i3dl2.Obstruction.flLFRatio= 0.f;
	i3dl2.Occlusion.flLFRatio= 0.2f;

	if (channel->spatialized)
	{
		real room_gain= 1.f-channel->reverb_attenuation;

		if (channel->attenuate_direct_path)
		{
			i3dl2.lDirectHF= dsound_volume_from_gain(platform_sound_dsound.direct_path_gain, 0);
			i3dl2.lDirect= dsound_volume_from_gain(platform_sound_dsound.direct_path_gain, 0);
		}
		else
		{
			room_gain*= 0.5f;
		}

		i3dl2.lRoom= dsound_volume_from_gain(room_gain, 0);
		i3dl2.lRoomHF= i3dl2.lRoom;
		i3dl2.Obstruction.lHFLevel= dsound_obstruction_from_obstruction(channel->obstruction);
		i3dl2.Occlusion.lHFLevel= dsound_occlusion_from_occlusion(channel->occlusion);
	}
	else
	{
		i3dl2.lRoom= DSBVOLUME_MIN;
		i3dl2.lRoomHF= DSBVOLUME_MIN;
		i3dl2.Obstruction.lHFLevel= 0;
		i3dl2.Occlusion.lHFLevel= 0;
	}

	IDirectSoundStream_SetI3DL2Source(channel->stream, &i3dl2, DS3D_DEFERRED);

	return;
}

static boolean virtual_channel_new(
	short virtual_channel_index,
	short type_index)
{
	struct sound_virtual_channel *vchannel= virtual_channel_get(virtual_channel_index);

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		422,
		type_index>=0 && type_index<NUMBER_OF_SOUND_CHANNEL_TYPES);

	vchannel->type_index= type_index;
	vchannel->channel_index= NONE;

	return TRUE;
}

static boolean create_inanity_channel(
	void)
{
	boolean success= FALSE;
	DSBUFFERDESC buffer_desc;
	WAVEFORMATEX wave_format;
	HRESULT result;

	wave_format.wFormatTag= WAVE_FORMAT_PCM;
	wave_format.wBitsPerSample= 16;
	wave_format.nChannels= 1;
	wave_format.nBlockAlign= 2;
	wave_format.nSamplesPerSec= 22050;
	wave_format.nAvgBytesPerSec= 44100;

	csmemset(&buffer_desc, 0, sizeof(buffer_desc));

	buffer_desc.dwSize= sizeof(buffer_desc);
	buffer_desc.dwFlags= 0;
	buffer_desc.dwBufferBytes= sizeof(dsound_globals.inanity_samples);
	buffer_desc.lpwfxFormat= &wave_format;
	buffer_desc.dwMixBinMask= DSMIXBIN_XTLK_BACK_LEFT|DSMIXBIN_XTLK_BACK_RIGHT|DSMIXBIN_I3DL2|
		DSMIXBIN_FXSEND_0|DSMIXBIN_FXSEND_1;

	result= IDirectSound_CreateSoundBuffer(
		dsound_globals.direct_sound,
		&buffer_desc,
		&dsound_globals.inanity_buffer,
		NULL);

	if (result>=0)
	{
		csmemset(dsound_globals.inanity_samples, 0, sizeof(dsound_globals.inanity_samples));

		IDirectSoundBuffer_SetBufferData(
			dsound_globals.inanity_buffer,
			dsound_globals.inanity_samples,
			sizeof(dsound_globals.inanity_samples));

		result= IDirectSoundBuffer_Play(dsound_globals.inanity_buffer, 0, 0, DSBPLAY_LOOPING);

		if (result>=0)
		{
			success= TRUE;
		}
		else
		{
			dsound_error(result, "failed to start the inanity channel.");
		}
	}
	else
	{
		dsound_error(result, "failed to create the inanity channel.");
	}

	return success;
}

static boolean channel_new(
	short channel_index,
	short type_flags)
{
	struct sound_channel *channel= channel_get(channel_index);
	boolean success= FALSE;
	unsigned long speaker_config;
	XBOXADPCMWAVEFORMAT wave_format;
	DSSTREAMDESC stream_desc;
	struct platform_sound_channel_properties properties;
	HRESULT result;

	channel->type_flags= type_flags;
	channel->virtual_channel_index= NONE;
	channel->stopping= FALSE;
	channel->playing_permutation= NULL;
	channel->queued_permutation= NULL;

	if (!TEST_FLAG(type_flags, _sound_channel_compressed_bit))
	{
		wave_format.wfx.wFormatTag= WAVE_FORMAT_PCM;
		wave_format.wfx.wBitsPerSample= 16;
		wave_format.wfx.nChannels= 2;
		wave_format.wfx.nBlockAlign= 4;
		wave_format.wfx.nSamplesPerSec= sound_sample_rate_samples_per_second[1];
		wave_format.wfx.nAvgBytesPerSec= wave_format.wfx.nSamplesPerSec*4;
	}
	else
	{
		wave_format.wfx.wFormatTag= WAVE_FORMAT_XBOX_ADPCM;
		wave_format.wfx.wBitsPerSample= 4;
		wave_format.wfx.nChannels= (WORD)(TEST_FLAG(type_flags, _sound_channel_stereo_bit) ? 2 : 1);
		wave_format.wfx.nBlockAlign= (WORD)(SOUND_COMPRESSED_BLOCK_SIZE*wave_format.wfx.nChannels);
		wave_format.wfx.nSamplesPerSec= sound_samples_per_second(
			TEST_FLAG(type_flags, _sound_channel_44k_bit));
		wave_format.wfx.nAvgBytesPerSec=
			wave_format.wfx.nSamplesPerSec/SOUND_COMPRESSED_SAMPLES_PER_BLOCK*
			wave_format.wfx.nBlockAlign;
		wave_format.wfx.cbSize= 2;
		wave_format.wSamplesPerBlock= SOUND_COMPRESSED_SAMPLES_PER_BLOCK;
	}

	csmemset(&stream_desc, 0, sizeof(stream_desc));

	stream_desc.dwMaxAttachedPackets= MAXIMUM_SOUND_PACKETS;

	stream_desc.lpwfxFormat= &wave_format.wfx;
	stream_desc.dwFlags= 0;
	stream_desc.lpfnCallback= channel_packet_callback;
	stream_desc.lpvContext= (LPVOID)channel_index;

	if (TEST_FLAG(type_flags, _sound_channel_3d_bit))
	{
		stream_desc.dwFlags= DSSTREAMCAPS_CTRL3D;
	}

	result= IDirectSound_CreateSoundStream(
		dsound_globals.direct_sound,
		&stream_desc,
		&channel->stream,
		NULL);

	if (result>=0)
	{
		if (TEST_FLAG(type_flags, _sound_channel_3d_bit))
		{
			struct sound_location location;

			csmemset(&location, 0, sizeof(location));
			location.forward= *global_forward3d;

			channel_set_location(channel_index, FALSE, &location, 0.f, 0.f, FALSE);
		}
		else
		{
			unsigned long mixbins= 0;
			long mixbin_volumes[MAXIMUM_DSOUND_MIXBINS];

			IDirectSound_GetSpeakerConfig(dsound_globals.direct_sound, &speaker_config);

			if (speaker_config & DSSPEAKER_ENABLE_AC3)
			{
				if (!TEST_FLAG(type_flags, _sound_channel_stereo_bit))
				{
					mixbins= DSMIXBIN_FRONT_LEFT|DSMIXBIN_FRONT_RIGHT|DSMIXBIN_FRONT_CENTER;

					mixbin_volumes[0]= dsound_volume_from_gain(0.5f, 0);
					mixbin_volumes[1]= dsound_volume_from_gain(0.5f, 0);
					mixbin_volumes[2]= dsound_volume_from_gain(0.5f, 0);
				}
				else
				{
					mixbins= DSMIXBIN_FRONT_LEFT|DSMIXBIN_FRONT_RIGHT|DSMIXBIN_BACK_LEFT|
						DSMIXBIN_BACK_RIGHT|DSMIXBIN_FXSEND_0|DSMIXBIN_FXSEND_1;

					mixbin_volumes[0]= dsound_volume_from_gain(1.f, 0);
					mixbin_volumes[1]= dsound_volume_from_gain(1.f, 0);
					mixbin_volumes[2]= dsound_volume_from_gain(0.5f, 0);
					mixbin_volumes[3]= dsound_volume_from_gain(0.5f, 0);
					mixbin_volumes[4]= dsound_volume_from_gain(0.5f, 0);
					mixbin_volumes[5]= dsound_volume_from_gain(0.5f, 0);
				}
			}
			else if (!TEST_FLAG(type_flags, _sound_channel_stereo_bit))
			{
				mixbins= DSMIXBIN_FRONT_LEFT|DSMIXBIN_FRONT_RIGHT;

				mixbin_volumes[0]= dsound_volume_from_gain(0.5f, 0);
				mixbin_volumes[1]= dsound_volume_from_gain(0.5f, 0);
			}

			if (mixbins)
			{
				IDirectSoundStream_SetMixBins(channel->stream, mixbins);
				IDirectSoundStream_SetMixBinVolumes(channel->stream, mixbins, mixbin_volumes);
			}
		}

		success= TRUE;

		csmemset(&properties, 0, sizeof(properties));

		properties.minimum_distance= 1.f;
		properties.maximum_distance= 1.f;

		channel_set_properties(channel_index, &properties, FALSE);
	}
	else
	{
		dsound_error(result, "couldn't create sound stream.");
	}

	return success;
}

static void CALLBACK channel_packet_callback(
	void *stream_context,
	void *packet_context,
	unsigned long status)
{
	short channel_index= (short)stream_context;

	if (channel_index>=0 && channel_index<dsound_globals.actual_channel_count)
	{
		struct sound_channel *channel= channel_get(channel_index);

		if (status==XMEDIAPACKET_STATUS_SUCCESS || status==XMEDIAPACKET_STATUS_FLUSHED)
		{
			sound_cache_sound_hardware_unlock(packet_context);

			channel->packet_count--;

			if (!dsound_globals.paused)
			{
				if (channel->packet_count==0)
				{
					channel->state= _sound_channel_idle;
				}
				else if (status!=XMEDIAPACKET_STATUS_FLUSHED)
				{
					channel_submit_packets(channel_index);
				}
			}
		}
		else if (status==XMEDIAPACKET_STATUS_FAILURE)
		{
			dsound_record_error(NULL, "status is failure.");
		}
		else if (status==XMEDIAPACKET_STATUS_PENDING)
		{
			dsound_record_error(NULL, "status is pending.");
		}
		else
		{
			dsound_record_error(NULL, "status is undefined.");
		}
	}
	else
	{
		dsound_record_error(NULL, "trying to queue sound to invalid channel.");
	}

	return;
}

static void channel_set_location(
	short channel_index,
	boolean spatialized,
	struct sound_location const *location,
	real occlusion,
	real obstruction,
	boolean attenuate_direct_path)
{
	struct sound_channel *channel= channel_get(channel_index);
	boolean spatialization_changed= FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		906,
		TEST_FLAG(channel->type_flags, _sound_channel_3d_bit));
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		907,
		channel->stream);

	if (channel->spatialized!=spatialized || !dsound_globals.initialized)
	{
		HRESULT result= IDirectSoundStream_SetMode(
			channel->stream,
			spatialized ? DS3DMODE_NORMAL : DS3DMODE_DISABLE,
			DS3D_DEFERRED);

		channel->spatialized= spatialized;
		spatialization_changed= TRUE;

		if (result<0)
		{
			dsound_error(result, "couldn't set channel spatialization.");
		}
	}

	if (!(0.05f>fabs(location->position.x-channel->position.x)) ||
		!(0.05f>fabs(location->position.y-channel->position.y)) ||
		!(0.05f>fabs(location->position.z-channel->position.z)) ||
		!dsound_globals.initialized)
	{
		HRESULT result= IDirectSoundStream_SetPosition(
			channel->stream,
			location->position.x,
			location->position.z,
			location->position.y,
			DS3D_DEFERRED);

		if (result<0)
		{
			dsound_error(result, "couldn't set channel position.");
		}

		channel->position= location->position;
	}

	if (!(0.05f>fabs(location->forward.i-channel->forward.i)) ||
		!(0.05f>fabs(location->forward.j-channel->forward.j)) ||
		!(0.05f>fabs(location->forward.k-channel->forward.k)) ||
		!dsound_globals.initialized)
	{
		HRESULT result;

		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
			933,
			valid_real_normal3d(&location->forward));

		result= IDirectSoundStream_SetConeOrientation(
			channel->stream,
			location->forward.i,
			location->forward.k,
			location->forward.j,
			DS3D_DEFERRED);

		if (result<0)
		{
			dsound_error(result, "couldn't set channel orientation.");
		}

		channel->forward= location->forward;
	}

	if (!(0.01f>fabs(location->translational_velocity.i-channel->translational_velocity.i)) ||
		!(0.01f>fabs(location->translational_velocity.j-channel->translational_velocity.j)) ||
		!(0.01f>fabs(location->translational_velocity.k-channel->translational_velocity.k)) ||
		!dsound_globals.initialized)
	{
		HRESULT result= IDirectSoundStream_SetVelocity(
			channel->stream,
			location->translational_velocity.i,
			location->translational_velocity.k,
			location->translational_velocity.j,
			DS3D_DEFERRED);

		if (result<0)
		{
			dsound_error(result, "couldn't set channel velocity.");
		}

		channel->translational_velocity= location->translational_velocity;
	}

	if (!(0.001f>fabs(occlusion-channel->occlusion)) ||
		!(0.001f>fabs(obstruction-channel->obstruction)) ||
		channel->attenuate_direct_path!=attenuate_direct_path ||
		spatialization_changed ||
		!dsound_globals.initialized)
	{
		channel->attenuate_direct_path= attenuate_direct_path;
		channel->occlusion= occlusion;
		channel->obstruction= obstruction;

		channel_set_i3dl2_source(channel_index);
	}

	return;
}

static void channel_set_properties(
	short channel_index,
	struct platform_sound_channel_properties const *properties,
	boolean gain_only)
{
	struct sound_channel *channel= channel_get(channel_index);
	real gain= dsound_globals.pause_gain*properties->gain;

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		980,
		properties->gain>=0.f && properties->gain<=1.f);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		981,
		dsound_globals.pause_gain>=0 && dsound_globals.pause_gain<=1.f);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		982,
		channel->stream);

	if (!(0.001f>fabs(gain-channel->gain)) || !dsound_globals.initialized)
	{
		HRESULT result= IDirectSoundStream_SetVolume(
			channel->stream,
			dsound_volume_from_gain(gain, 0));

		if (result<0)
		{
			dsound_error(result, "couldn't set channel volume.");
		}

		channel->gain= gain;
	}

	if (!gain_only)
	{
		if (!(0.001f>fabs(properties->pitch-channel->pitch)) || !dsound_globals.initialized)
		{
			unsigned long samples_per_second= sound_samples_per_second(
				TEST_FLAG(channel->type_flags, _sound_channel_44k_bit));
			HRESULT result= IDirectSoundStream_SetFrequency(
				channel->stream,
				dsound_frequency_from_pitch(samples_per_second, properties->pitch));

			if (result<0)
			{
				dsound_error(result, "couldn't set channel pitch.");
			}

			channel->pitch= properties->pitch;
		}

		if (TEST_FLAG(channel->type_flags, _sound_channel_3d_bit))
		{
			if (!(0.05f>fabs(properties->maximum_distance-channel->maximum_distance)) ||
				!dsound_globals.initialized)
			{
				HRESULT result= IDirectSoundStream_SetMaxDistance(
					channel->stream,
					properties->maximum_distance,
					DS3D_DEFERRED);

				if (result<0)
				{
					dsound_error(result, "couldn't set channel max distance.");
				}

				channel->maximum_distance= properties->maximum_distance;
			}

			if (!(0.05f>fabs(properties->minimum_distance-channel->minimum_distance)) ||
				!dsound_globals.initialized)
			{
				HRESULT result= IDirectSoundStream_SetMinDistance(
					channel->stream,
					properties->minimum_distance,
					DS3D_DEFERRED);

				if (result<0)
				{
					dsound_error(result, "couldn't set channel min distance.");
				}

				channel->minimum_distance= properties->minimum_distance;
			}

			if (!(0.034906585f>fabs(properties->cone_inside_angle-channel->cone_inside_angle)) ||
				!(0.034906585f>fabs(properties->cone_outside_angle-channel->cone_outside_angle)) ||
				!dsound_globals.initialized)
			{
				HRESULT result= IDirectSoundStream_SetConeAngles(
					channel->stream,
					dsound_angle_from_angle(properties->cone_inside_angle),
					dsound_angle_from_angle(properties->cone_outside_angle),
					DS3D_IMMEDIATE);

				if (result<0)
				{
					dsound_error(result, "couldn't set channel cone angles.");
				}

				channel->cone_inside_angle= properties->cone_inside_angle;
				channel->cone_outside_angle= properties->cone_outside_angle;
			}

			if (!(0.001f>fabs(properties->cone_outside_gain-channel->cone_outside_gain)) ||
				!dsound_globals.initialized)
			{
				HRESULT result= IDirectSoundStream_SetConeOutsideVolume(
					channel->stream,
					dsound_volume_from_gain(properties->cone_outside_gain, 0),
					DS3D_DEFERRED);

				if (result<0)
				{
					dsound_error(result, "couldn't set channel cone volume.");
				}

				channel->cone_outside_gain= properties->cone_outside_gain;
			}

			if (!(0.001f>fabs(properties->reverb_attenuation-channel->reverb_attenuation)) ||
				!dsound_globals.initialized)
			{
				channel->reverb_attenuation= properties->reverb_attenuation;

				channel_set_i3dl2_source(channel_index);
			}
		}
	}

	return;
}

static boolean channel_queue_packet(
	short index)
{
	struct sound_channel *channel= channel_get(index);
	boolean queued= FALSE;

	if (channel->playing_permutation)
	{
		if (channel->playing_permutation->cache_base_address)
		{
			if ((byte *)channel->playing_permutation->cache_base_address>=
					(byte *)physical_memory_get_sound_cache_base_address() &&
				(byte *)channel->playing_permutation->cache_base_address+channel->playing_permutation->samples.size<=
					(byte *)physical_memory_get_sound_cache_base_address()+SOUND_CACHE_SIZE)
			{
				struct sound_permutation *sound= channel->playing_permutation;
				XMEDIAPACKET packet;
				long remaining_size= sound->samples.size-channel->sample_offset;
				HRESULT result;

				channel->packet_count++;

				packet.pvBuffer= (byte *)sound->cache_base_address+channel->sample_offset;
				packet.pdwCompletedSize= NULL;
				packet.pdwStatus= NULL;
				packet.prtTimestamp= 0;
				packet.pContext= sound;

				if (remaining_size<=
					(TEST_FLAG(channel->type_flags, _sound_channel_44k_bit) ? 2.f : 1.f)*
					(TEST_FLAG(channel->type_flags, _sound_channel_stereo_bit) ? 2.f : 1.f)*
					(real)(TEST_FLAG(channel->type_flags, _sound_channel_compressed_bit) ?
						MAXIMUM_COMPRESSED_SOUND_PACKET_SIZE : MAXIMUM_SOUND_PACKET_SIZE))
				{
					if (channel->sample_offset)
					{
						channel->playing_permutation= channel->queued_permutation;
						channel->queued_permutation= NULL;
						channel->sample_offset= 0;

						packet.dwMaxSize= remaining_size;

						if (channel->state==_sound_channel_queued)
						{
							channel->state= _sound_channel_playing;
						}
					}
					else
					{
						long block_size= SOUND_COMPRESSED_BLOCK_SIZE*
							(TEST_FLAG(channel->type_flags, _sound_channel_stereo_bit) ? 2 : 1);

						packet.dwMaxSize= MAX(remaining_size/block_size/2, 1)*block_size;

						channel->sample_offset= packet.dwMaxSize;
					}
				}
				else
				{
					packet.dwMaxSize= (long)(
						(TEST_FLAG(channel->type_flags, _sound_channel_44k_bit) ? 2.f : 1.f)*
						(TEST_FLAG(channel->type_flags, _sound_channel_stereo_bit) ? 2.f : 1.f)*
						(real)(TEST_FLAG(channel->type_flags, _sound_channel_compressed_bit) ?
							MAXIMUM_COMPRESSED_SOUND_PACKET_SIZE : MAXIMUM_SOUND_PACKET_SIZE));

					channel->sample_offset += packet.dwMaxSize;
				}

				if (packet.dwMaxSize)
				{
					result= IDirectSoundStream_Process(channel->stream, &packet, NULL);

					if (result>=0)
					{
						sound_cache_sound_hardware_lock(sound);

						queued= TRUE;
					}
					else
					{
						dsound_record_error(&result, "couldn't queue sound packet.");
					}
				}
			}
			else
			{
				sprintf(
					temporary,
					"trying to queue sound %s but it's outside the valid range. (%ld)",
					channel->playing_permutation->name,
					channel->playing_permutation->cache_base_address);

				dsound_record_error(NULL, temporary);
			}
		}
		else
		{
			dsound_record_error(NULL, "trying to queue sound but samples is null.");
		}
	}
	else
	{
		dsound_record_error(NULL, "trying to queue sound but sound is null.");
	}

	return queued;
}

static void channel_submit_packets(
	short index)
{
	struct sound_channel *channel= channel_get(index);

	while (channel->packet_count<MAXIMUM_SOUND_PACKETS && channel->playing_permutation)
	{
		unsigned long status;
		HRESULT result= IDirectSoundStream_GetStatus(channel->stream, &status);

		if (result<0)
		{
			dsound_record_error(&result, "couldn't get channel status.");
			break;
		}

		if (!TEST_FLAG(status, 0))
		{
			break;
		}

		if (!channel_queue_packet(index))
		{
			break;
		}
	}

	return;
}

static void channel_queue_sound(
	short index,
	struct sound_permutation *sound)
{
	struct sound_channel *channel= channel_get(index);

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		1111,
		!dsound_globals.paused);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		1112,
		sound);

	switch (channel->state)
	{
		case _sound_channel_idle:
		{
			HRESULT result;

			channel->state= _sound_channel_playing;
			channel->playing_permutation= sound;
			channel->sample_offset= 0;
			channel->packet_count= 0;

			result= IDirectSound_CommitDeferredSettings(dsound_globals.direct_sound);

			if (result<0)
			{
				dsound_error(result, "couldn't commit deferred settings.");
			}

			channel_submit_packets(index);
			break;
		}

		case _sound_channel_playing:
			channel->state= _sound_channel_queued;
			channel->queued_permutation= sound;
			break;

		case _sound_channel_queued:
			channel->queued_permutation= sound;
			break;

		default:
			match_vassert(
				"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
				1144,
				FALSE,
				"bad DirectSound channel state.");
			break;
	}

	return;
}

static void dsound_record_error(
	HRESULT *result,
	char const *message)
{
	if (result)
	{
		interrupt_result= *result;
	}

	if (strlen(dsound_error_string)+strlen(message)<MAXIMUM_DSOUND_ERROR_STRING_LENGTH)
	{
		sprintf(dsound_error_string+strlen(dsound_error_string), message);
	}

	return;
}

static void virtual_channel_acquire_channel(
	short virtual_channel_index)
{
	struct sound_virtual_channel *vchannel= virtual_channel_get(virtual_channel_index);
	short channel_index;

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		1420,
		vchannel->channel_index==NONE);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		1421,
		vchannel->type_index>=0 && vchannel->type_index<NUMBER_OF_SOUND_CHANNEL_TYPES);

	for (channel_index= dsound_globals.type_first_channel_index[vchannel->type_index];
		vchannel->channel_index==NONE && channel_index<dsound_globals.actual_channel_count;
		channel_index++)
	{
		struct sound_channel *channel= channel_get(channel_index);

		if (channel->type_flags!=sound_channel_type_flags[vchannel->type_index])
		{
			break;
		}

		if (channel->virtual_channel_index==NONE &&
			(!channel->stopping || channel_finish_stopping(channel_index)))
		{
			vchannel->channel_index= channel_index;
		}
	}

	if (vchannel->channel_index!=NONE)
	{
		channel_get(vchannel->channel_index)->virtual_channel_index= virtual_channel_index;
	}
	else
	{
		error(
			_error_silent,
			"WARNING: ran out of actual sound channels of type %d",
			vchannel->type_index);
	}

	return;
}

static short virtual_channel_get_channel_index(
	short virtual_channel_index)
{
	struct sound_virtual_channel *vchannel= virtual_channel_get(virtual_channel_index);

	if (vchannel->channel_index==NONE)
	{
		virtual_channel_acquire_channel(virtual_channel_index);
	}

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		1468,
		vchannel->channel_index==NONE ||
			channel_get(vchannel->channel_index)->type_flags==sound_channel_type_flags[vchannel->type_index]);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		1469,
		vchannel->channel_index==NONE ||
			channel_get(vchannel->channel_index)->virtual_channel_index==virtual_channel_index);

	return vchannel->channel_index;
}

static boolean channel_finish_stopping(
	short index)
{
	struct sound_channel *channel= channel_get(index);
	boolean finished;

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		1212,
		channel->stopping);

	finished= DirectSoundGetStreamVoiceStatus(channel->stream)==0;

	if (finished)
	{
		IDirectSoundStream_Flush(channel->stream);

		channel->stopping= FALSE;
	}

	return finished;
}

static void dsound_error(
	HRESULT result,
	char const *format,
	...)
{
	char message[MAXIMUM_DSOUND_ERROR_MESSAGE_LENGTH];
	char const *result_name= "<unknown error>";
	va_list list;

	va_start(list, format);
	vsprintf(message, format, list);
	va_end(list);

	switch (result)
	{
		case DSERR_NOAGGREGATION:
			result_name= "DSERR_NOAGGREGATION";
			break;

		case DSERR_GENERIC:
			result_name= "DSERR_GENERIC";
			break;

		case DSERR_UNSUPPORTED:
			result_name= "DSERR_UNSUPPORTED";
			break;

		case DSERR_OUTOFMEMORY:
			result_name= "DSERR_OUTOFMEMORY";
			break;

		case DSERR_NODRIVER:
			result_name= "DSERR_NODRIVER";
			break;

		case DSERR_INVALIDCALL:
			result_name= "DSERR_INVALIDCALL";
			break;

		case DSERR_CONTROLUNAVAIL:
			result_name= "DSERR_CONTROLUNAVAIL";
			break;
	}

	error(_error_silent, "DirectSound:  '%s' (%s#%d)", message, result_name);

	return;
}

static void channel_stop(
	short index)
{
	struct sound_channel *channel= channel_get(index);

	if (channel->state!=_sound_channel_idle)
	{
		DirectSoundStopStream(channel->stream);

		channel->stopping= TRUE;
		channel->state= _sound_channel_idle;
	}

	channel->playing_permutation= NULL;
	channel->queued_permutation= NULL;

	return;
}

static struct sound_channel *channel_get(
	short index)
{
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		105,
		index>=0 && index<dsound_globals.actual_channel_count);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		106,
		index<MAXIMUM_SOUND_CHANNELS);

	return &dsound_globals.channels[index];
}

static struct sound_virtual_channel *virtual_channel_get(
	short index)
{
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		114,
		index>=0 && index<dsound_globals.virtual_channel_count);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_dsound_xbox.c",
		115,
		index<MAXIMUM_SOUND_CHANNELS);

	return &dsound_globals.virtual_channels[index];
}

static short channel_get_state(
	short index)
{
	return channel_get(index)->state;
}
