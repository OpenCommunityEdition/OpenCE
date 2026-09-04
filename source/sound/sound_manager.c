/*
SOUND_MANAGER.C

symbols in this file:
001BB280 0070:
	_sound_valid_for_channel (0000)
001BB2F0 0010:
	_current_platform_definition (0000)
001BB300 0010:
	_sound_initialize_for_new_map (0000)
001BB310 0060:
	_sound_dispose (0000)
001BB370 0020:
	_sound_is_active (0000)
001BB390 0040:
	_sound_pause (0000)
001BB3D0 0010:
	_sound_render_time (0000)
001BB3E0 0070:
	_sound_reconnect_to_structure_bsp (0000)
001BB450 0020:
	_sound_try_and_get (0000)
001BB470 0010:
	_sound_enable (0000)
001BB480 0020:
	_sound_scripted_dialog_is_playing (0000)
001BB4A0 0020:
	_sound_manager_set_sound_environment (0000)
001BB4C0 0030:
	_code_001bb4c0 (0000)
001BB4F0 0020:
	_code_001bb4f0 (0000)
001BB510 0060:
	_code_001bb510 (0000)
001BB570 0040:
	_code_001bb570 (0000)
001BB5B0 0040:
	_code_001bb5b0 (0000)
001BB5F0 00a0:
	_code_001bb5f0 (0000)
001BB690 0030:
	_code_001bb690 (0000)
001BB6C0 0050:
	_code_001bb6c0 (0000)
001BB710 0020:
	_code_001bb710 (0000)
001BB730 00f0:
	_code_001bb730 (0000)
001BB820 0200:
	_code_001bb820 (0000)
001BBA20 0080:
	_code_001bba20 (0000)
001BBAA0 00a0:
	_code_001bbaa0 (0000)
001BBB40 00f0:
	_code_001bbb40 (0000)
001BBC30 0080:
	_code_001bbc30 (0000)
001BBCB0 0040:
	_code_001bbcb0 (0000)
001BBCF0 00f0:
	_code_001bbcf0 (0000)
001BBDE0 0020:
	_code_001bbde0 (0000)
001BBE00 0130:
	_code_001bbe00 (0000)
001BBF30 0050:
	_code_001bbf30 (0000)
001BBF80 0060:
	_code_001bbf80 (0000)
001BBFE0 00c0:
	_code_001bbfe0 (0000)
001BC0A0 0160:
	_code_001bc0a0 (0000)
001BC200 01b0:
	_sound_initialize (0000)
001BC3B0 0030:
	_code_001bc3b0 (0000)
001BC3E0 0170:
	_code_001bc3e0 (0000)
001BC550 0180:
	_code_001bc550 (0000)
001BC6D0 00c0:
	_code_001bc6d0 (0000)
001BC790 00d0:
	_code_001bc790 (0000)
001BC860 0110:
	_code_001bc860 (0000)
001BC970 0100:
	_code_001bc970 (0000)
001BCA70 0210:
	_code_001bca70 (0000)
001BCC80 0130:
	_code_001bcc80 (0000)
001BCDB0 00d0:
	_code_001bcdb0 (0000)
001BCE80 00c0:
	_code_001bce80 (0000)
001BCF40 0080:
	_sound_stop_impulse (0000)
001BCFC0 0070:
	_sound_stop_impulse_by_source_and_definition (0000)
001BD030 0060:
	_sound_stop_all (0000)
001BD090 00f0:
	_code_001bd090 (0000)
001BD180 0220:
	_code_001bd180 (0000)
001BD3A0 01a0:
	_code_001bd3a0 (0000)
001BD540 01e0:
	_code_001bd540 (0000)
001BD720 0550:
	_code_001bd720 (0000)
001BDC70 03d0:
	_sound_new_impulse (0000)
001BE040 0470:
	_sound_refresh_looping (0000)
001BE4B0 01f0:
	_code_001be4b0 (0000)
001BE6A0 01f0:
	_code_001be6a0 (0000)
001BE890 0360:
	_code_001be890 (0000)
001BEBF0 01f0:
	_code_001bebf0 (0000)
001BEDE0 0070:
	_sound_idle (0000)
001BEE50 02e0:
	_code_001bee50 (0000)
001BF130 00f0:
	_sound_dispose_from_old_map (0000)
001BF220 00f0:
	_sound_render (0000)
002AF820 000d:
	??_C@_0N@ICDMEFNC@sound_render?$AA@ (0000)
002AF830 001c:
	_rdata_002af830 (0000)
002AF84C 0036:
	??_C@_0DG@GEFEIAPP@index?$DO?$DN0?5?$CG?$CG?5index?$DMsound_manager_@ (0000)
002AF884 0025:
	??_C@_0CF@OMMIMEGJ@c?3?2halo?2SOURCE?2sound?2sound_manag@ (0000)
002AF8AC 0032:
	??_C@_0DC@IDLFGNMI@index?$DO?$DN0?5?$CG?$CG?5index?$DMMAXIMUM_NUMBER@ (0000)
002AF8E0 0034:
	??_C@_0DE@IJMAACKJ@sound_get?$CIsound_index?$CJ?9?$DOplaying_@ (0000)
002AF918 006a:
	??_C@_0GK@GIGJNEHE@sound?9?$DOplaying_channel_index?$DN?$DNNO@ (0000)
002AF988 0042:
	??_C@_0EC@NBNLKGJ@summary?9?$DOlike_source_count?$DMsumma@ (0000)
002AF9CC 003f:
	??_C@_0DP@JPHEDLPF@summary?9?$DOlike_definition_count?$DMs@ (0000)
002AFA10 0053:
	??_C@_0FD@BLJIIJII@summary?9?$DOmaximum_instance_count?$DM@ (0000)
002AFA68 004f:
	??_C@_0EP@DKNAFCF@summary?9?$DOmaximum_source_instance@ (0000)
002AFAB8 0016:
	??_C@_0BG@LLOIFBDE@properties?9?$DOpitch?$DO0?4f?$AA@ (0000)
002AFAD0 001d:
	??_C@_0BN@IEJOFEHG@?$CBchannel?9?$DOqueued_permutation?$AA@ (0000)
002AFAF0 000a:
	??_C@_09LAFJOCBG@?$CFs?$HMn?$CFf?5?$CFf?$AA@ (0000)
002AFAFC 003c:
	??_C@_0DM@PJIONELA@sound_manager_globals?4channel_co@ (0000)
002AFB38 000f:
	??_C@_0P@FIIBGGGF@looping?5sounds?$AA@ (0000)
002AFB48 0007:
	??_C@_06OHLGPBPO@sounds?$AA@ (0000)
002AFB50 0038:
	??_C@_0DI@CEKFNNAG@fade_in_sound_index?$CB?$DNNONE?5?$HM?$HM?5fad@ (0000)
002AFB88 000d:
	??_C@_0N@CFIAGGKK@seconds?$DO?$DN0?4f?$AA@ (0000)
002AFB98 0042:
	??_C@_0EC@MNDJFMMK@mode?$DN?$DN_sound_fade_mode_linear?5?$HM?$HM@ (0000)
002AFBDC 0024:
	??_C@_0CE@FPLGCI@listener_get?$CIlistener_index?$CJ?9?$DOva@ (0000)
002AFC00 002d:
	??_C@_0CN@BJAODDBE@challenger_sound_index?$CB?$DNchampion@ (0000)
002AFC30 0026:
	??_C@_0CG@JFKDGJLJ@sound_cache_sound_loaded?$CIpermuta@ (0000)
002AFC58 002d:
	??_C@_0CN@GMMAODPG@sound_get?$CIsound_index?$CJ?9?$DOtype?$DN?$DN_s@ (0000)
002AFC88 0004:
	__real@3be56042 (0000)
002AFC90 0048:
	??_C@_0EI@NEANMMGP@TEST_FLAG?$CIdefinition?9?$DOflags?0?5_so@ (0000)
002AFCD8 001c:
	??_C@_0BM@DILLGPMI@sound?9?$DOtype?$CB?$DN_sound_impulse?$AA@ (0000)
002AFCF8 006a:
	??_C@_0GK@CFIFHDCA@attempt?5to?5play?5a?5sound?5that?5was@ (0000)
002AFD64 0012:
	??_C@_0BC@IFEANPKO@sound?9?$DOtrack_data?$AA@ (0000)
002AFD78 006f:
	??_C@_0GP@PNOIFFBD@source?9?$DOspatialization_mode?$DN?$DN_so@ (0000)
002AFDE8 002d:
	??_C@_0CN@FLAINKPN@track_data_size?$DM?$DNMAXIMUM_SOUND_C@ (0000)
002AFE18 0029:
	??_C@_0CJ@HAJEDJHJ@loop?9?$DOdefinition_index?$DN?$DNdefiniti@ (0000)
002AFE48 0044:
	??_C@_0EE@CBCNGIMC@channel_get?$CIsound?9?$DOplaying_chann@ (0000)
002AFE8C 0010:
	??_C@_0BA@NKIBLFLK@listener?9?$DOvalid?$AA@ (0000)
002AFEA0 00ad:
	??_C@_0KN@CMACAPDF@TEST_FLAG?$CIsound?9?$DOflags?0?5_sound_d@ (0000)
002AFF50 002c:
	??_C@_0CM@MDCHMPMH@sound?9?$DOplaying_channel_index?$DN?$DNch@ (0000)
002AFF80 0098:
	??_C@_0JI@MGGHJBNL@sound_valid_for_channel?$CIdefiniti@ (0000)
002B0018 0004:
	__real@43960000 (0000)
00317478 060c:
	_sound_gain_under_dialog (0000)
	_platform_definitions (0004)
	_sound_fade_exponent (0608)
004D2D60 0178:
	_bss_004d2d60 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "data.h"
#include "cache/sound_cache.h"
#include "math/real_math.h"
#include "sound_manager.h"
#include "sound_classes.h"
#include "sound_definitions.h"
#include "sound_dsound.h"
#include "sound_environment_definitions.h"
#include "game_sound.h"
#include "sound_preferences.h"
#include "game/players.h"
#include "render/render_debug.h"
#include "scenario/scenario.h"
#include "tag_files/tag_files.h"

#include <math.h>
#include <stdio.h>

/* ---------- constants */

enum
{
	MAXIMUM_SOUND_CHANNELS = 256,
	MAXIMUM_NUMBER_OF_LOCAL_PLAYERS = 4,
	MAXIMUM_SOUND_CALLBACK_DATA = 0x30,
};

enum sound_promotion_result
{
	_sound_promotion_dont,
	_sound_promotion_do,
	_sound_promotion_dont_play,
};

enum sound_spatialization_mode
{
	_sound_spatialization_mode_none,
	_sound_spatialization_mode_absolute,
	_sound_spatialization_mode_relative,
	NUMBER_OF_SOUND_SPATIALIZATION_MODES,
};

enum sound_type
{
	_sound_impulse = 0,
	_sound_start_track = 1,
	_sound_loop_track = 2,
	_sound_stopping_track = 3,
	_sound_stop_track = 4,
	NUMBER_OF_SOUND_TYPES = 5,
};

enum sound_fade_mode
{
	_sound_fade_mode_linear,
	_sound_fade_mode_crossfade,
};

enum sound_encoding
{
	_sound_encoding_mono = 0,
	_sound_encoding_stereo = 1,
	NUMBER_OF_SOUND_ENCODINGS = 2,
};

enum sound_compression
{
	_sound_compression_none = 0,
	_sound_compression_xbox_adpcm = 1,
	_sound_compression_ima_adpcm = 2,
	_sound_compression_ogg = 3,
	NUMBER_OF_SOUND_COMPRESSION_TYPES = 4,
};

enum sound_datum_flags
{
	_sound_delayed_bit,
	_sound_cached_bit,
	_sound_inaudible_bit,
	_sound_waiting_for_cache_bit,
	NUMBER_OF_SOUND_FLAGS,
};

enum sound_channel_flags
{
	_sound_channel_3d_bit,
	_sound_channel_stereo_bit,
	_sound_channel_44k_bit,
	_sound_channel_compressed_bit,
};

/* ---------- macros */

#define sound_get(index) ((struct sound_datum *)datum_get(sound_data, (index)))
#define sound_permutation_get(definition_index, pitch_range_index, permutation_index) \
	TAG_BLOCK_GET_ELEMENT( \
		&TAG_BLOCK_GET_ELEMENT( \
			&sound_definition_get(definition_index)->pitch_ranges, \
			pitch_range_index, \
			struct sound_pitch_range)->permutations, \
		permutation_index, \
		struct sound_permutation)

/* ---------- structures */

struct platform_sound_channel_properties;
struct platform_sound_listener_properties;
struct sound_location;
struct sound_permutation;
struct sound_preferences;

struct sound_source
{
	short spatialization_mode;
	short pad_2;
	real scale;
	real gain;
	struct sound_location location;
	real obstruction;
	real occlusion;
};

struct sound_listener
{
	boolean valid;
	boolean underwater;
	byte pad_2[2];
	real_matrix4x3 matrix;
	real_vector3d velocity;
};

struct sound_channel_datum
{
	long sound_index;
	short type_flags;
	short pad_6;
	real estimated_tick_time;
	real pitch;
	struct sound_permutation *playing_permutation;
	struct sound_permutation *queued_permutation;
};

struct sound_datum
{
	short identifier;
	short type;
	word flags;
	short listener_index;
	long definition_index;
	long source_identifier;
	boolean (*track_proc)(
		long source_identifier,
		void const *track_data,
		struct sound_source *source);
	struct sound_source source;
	byte track_data[0x30];
	long start_time;
	real pitch;
	short playing_channel_index;
	short pitch_range_index;
	short permutation_index;
	short fade_mode;
	short loop_track_index;
	short pad_96;
	long next_definition_index;
	real fade_interpolation_start;
	real fade_interpolation_end;
	long fade_start_time;
	long fade_stop_time;
};

struct looping_sound_datum
{
	short identifier;
	short pad_2;
	long definition_index;
	long loop_identifier;
	struct sound_source source;
	boolean flip_flop;
	boolean alternate;
	boolean ordered_sounds_finished;
	byte pad_4F;
	short component_sound_count;
	short state;
	byte details[0x80];
	struct
	{
		long primary_sound_index;
	} tracks[4];
};

struct sound_platform_definition
{
	short platform_code;
	byte reserved0[2];
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
		struct sound_permutation const *permutation,
		long hardware_format,
		boolean looping,
		short playback_rate,
		boolean cache_miss);
	void (*channel_update)(
		short channel_index,
		boolean force,
		short playback_rate);
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
		boolean valid,
		struct sound_location const *location,
		real minimum_distance,
		real maximum_distance,
		boolean force,
		short playback_rate);
	void (*set_channel_properties)(
		short channel_index,
		struct platform_sound_channel_properties const *properties,
		boolean gain_only,
		boolean force,
		short playback_rate);
	void (*handle_audio_profile_changes)(
		boolean surround_sound,
		boolean headphones,
		long speaker_configuration);
	boolean (*hardware_reverb_enabled)(
		void);
};

struct sound_manager_globals
{
	boolean initialized;
	boolean enabled;
	boolean paused;
	boolean idling;
	long game_time_when_no_scripted_dialog_will_be_playing;
	struct sound_platform_definition *platform_definition;
	long render_time;
	real ticks_elapsed;
	boolean flip_flop;
	byte pad_15[3];
	struct sound_listener listeners[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
	struct sound_environment_definition sound_environment;
	real nondialog_gain;
	short channel_count;
	short pad_176;
};

typedef char verify_sound_class_definition_size[
	sizeof(struct sound_class_definition) == 0x2C ? 1 : -1];
typedef char verify_sound_source_size[
	sizeof(struct sound_source) == 0x40 ? 1 : -1];
typedef char verify_sound_listener_size[
	sizeof(struct sound_listener) == 0x44 ? 1 : -1];
typedef char verify_sound_channel_datum_size[
	sizeof(struct sound_channel_datum) == 0x18 ? 1 : -1];
typedef char verify_sound_datum_size[
	sizeof(struct sound_datum) == 0xAC ? 1 : -1];
typedef char verify_looping_sound_datum_size[
	sizeof(struct looping_sound_datum) == 0xE4 ? 1 : -1];

typedef char verify_sound_manager_globals_size[
	sizeof(struct sound_manager_globals) == 0x178 ? 1 : -1];
typedef char verify_sound_platform_definition_size[
	sizeof(struct sound_platform_definition) == 0x40 ? 1 : -1];
typedef char verify_sound_platform_dispose_offset[
	offsetof(struct sound_platform_definition, dispose) == 0x8 ? 1 : -1];
typedef char verify_sound_platform_pause_offset[
	offsetof(struct sound_platform_definition, set_pause) == 0x28 ? 1 : -1];
typedef char verify_sound_manager_paused_offset[
	offsetof(struct sound_manager_globals, paused) == 0x2 ? 1 : -1];
typedef char verify_sound_manager_dialog_time_offset[
	offsetof(
		struct sound_manager_globals,
		game_time_when_no_scripted_dialog_will_be_playing) == 0x4 ? 1 : -1];
typedef char verify_sound_manager_listeners_offset[
	offsetof(struct sound_manager_globals, listeners) == 0x18 ? 1 : -1];
typedef char verify_sound_manager_environment_offset[
	offsetof(struct sound_manager_globals, sound_environment) == 0x128 ? 1 : -1];
typedef char verify_sound_manager_channel_count_offset[
	offsetof(struct sound_manager_globals, channel_count) == 0x174 ? 1 : -1];

/* ---------- prototypes */

long game_time_get(
	void);
unsigned long system_milliseconds(
	void);

static void sound_update_time(
	void);
static real sound_scale_value(
	real base,
	real lower_bound_modifier,
	real upper_bound_modifier,
	real scale);
static boolean sound_definition_is_playable(
	long definition_index);
static struct sound_channel_datum *channel_get(
	short channel_index);
static struct sound_listener *listener_get(
	short listener_index);
static short sound_definition_promote(
	long definition_index);
static real sound_manager_master_gain(
	short class_index);
static void sound_delete(
	long sound_index);
static long sound_travel_milliseconds(
	real distance);
static boolean refresh_sound(
	long sound_index);
static short channel_get_state(
	short channel_index);
static boolean track_loop_track_sound(
	long looping_sound_index,
	void const *track_data,
	struct sound_source *source);
static void sound_set_definition_begin(
	long sound_index,
	long definition_index);
static long looping_sound_find(
	long identifier);
static real limit_pitch(
	real desired_pitch,
	real old_pitch,
	real maximum_bend);
static void render_debug_sound(
	long sound_index);
static real sound_scale_random_value(
	real base_lower_bound,
	real base_upper_bound,
	real lower_bound_modifier,
	real upper_bound_modifier,
	real scale);
static real source_distance(
	short listener_index,
	struct sound_source *source);
static real source_distance_squared(
	short listener_index,
	struct sound_source *source);
static short source_audible(
	struct sound_source *source,
	real maximum_distance);
static void refresh_sounds(
	void);
static real sound_calculate_fade(
	long sound_index);
static void sound_start_fade(
	short mode,
	real seconds,
	long fade_in_sound_index,
	long fade_out_sound_index);
static void channel_stop(
	short channel_index);
static void sound_stop(
	long sound_index);

/* ---------- globals */

extern struct data_array *looping_sound_data;
extern struct data_array *sound_data;
extern struct sound_channel_datum sound_channels[MAXIMUM_SOUND_CHANNELS];
extern boolean loud_dialog_hack;

static real const sound_pitch_range_fade_time = 0.5f;
static real const sound_inaudible_fade_out_time = 2.f;
static real const sound_inaudible_fade_back_in_time = 0.5f;
static real const sound_player_fade_out_time = 0.3f;
static real const oo_speed_of_sound = 8.9647064f;
static long const speed_of_sound_threshold = 250;
static real const sound_priority_epsilon = 0.1f;

real sound_gain_under_dialog = 0.7f;
struct sound_platform_definition *platform_definitions[2] =
{
	&platform_sound_dsound,
	NULL,
};
real sound_fade_exponent = 2.5f;
struct sound_manager_globals sound_manager_globals;

/* ---------- public code */

boolean sound_valid_for_channel(
	short compression,
	short encoding,
	short sample_rate,
	short spatialization_mode,
	short channel_type_flags)
{
	boolean valid = TRUE;

	if (!TEST_FLAG(
			channel_type_flags,
			_sound_channel_compressed_bit) != (compression == 0))
	{
		valid = FALSE;
	}

	if (!TEST_FLAG(
			channel_type_flags,
			_sound_channel_stereo_bit) != (encoding == 0))
	{
		valid = FALSE;
	}

	if (TEST_FLAG(
			channel_type_flags,
			_sound_channel_44k_bit) != sample_rate)
	{
		valid = FALSE;
	}

	if (!TEST_FLAG(channel_type_flags, _sound_channel_stereo_bit) &&
		!TEST_FLAG(channel_type_flags, _sound_channel_3d_bit) !=
			(spatialization_mode == _sound_spatialization_mode_none))
	{
		valid = FALSE;
	}

	return valid;
}

struct sound_platform_definition *current_platform_definition(
	void)
{
	return sound_manager_globals.platform_definition;
}

void sound_dispose(
	void)
{
	if (sound_manager_globals.initialized)
	{
		sound_manager_globals.platform_definition->dispose();
		data_make_invalid(sound_data);
		data_make_invalid(looping_sound_data);
		sound_manager_globals.initialized = FALSE;
	}

	if (sound_data)
	{
		data_dispose(sound_data);
	}

	if (looping_sound_data)
	{
		data_dispose(looping_sound_data);
	}

	sound_cache_delete();

	return;
}

void sound_enable(
	boolean enabled)
{
	sound_manager_globals.enabled = enabled;

	return;
}

boolean sound_is_active(
	void)
{
	return sound_manager_globals.initialized && sound_manager_globals.enabled;
}

void sound_pause(
	boolean paused)
{
	if (paused != sound_manager_globals.paused)
	{
		sound_manager_globals.paused = paused;
		sound_manager_globals.platform_definition->set_pause(paused);

		if (!paused)
		{
			sound_manager_globals.render_time = system_milliseconds();
		}
	}

	return;
}

boolean sound_try_and_get(
	long sound_index)
{
	return datum_try_and_get(sound_data, sound_index) != NULL;
}

void sound_manager_set_sound_environment(
	struct sound_environment_definition const *environment)
{
	sound_manager_globals.sound_environment = *environment;

	return;
}

long sound_render_time(
	void)
{
	return sound_manager_globals.render_time;
}

boolean sound_scripted_dialog_is_playing(
	void)
{
	long game_time = game_time_get();

	return game_time <
		sound_manager_globals.game_time_when_no_scripted_dialog_will_be_playing;
}

void sound_initialize_for_new_map(
	void)
{
	return;
}

void sound_reconnect_to_structure_bsp(
	void)
{
	if (sound_manager_globals.initialized && sound_manager_globals.enabled)
	{
		long sound_index = data_next_index(sound_data, NONE);

		while (sound_index != NONE)
		{
			struct sound_datum *sound = sound_get(sound_index);

			if (sound->source.spatialization_mode ==
				_sound_spatialization_mode_absolute)
			{
				scenario_location_from_point(
					&sound->source.location.game_location,
					&sound->source.location.position);
			}

			sound_index = data_next_index(sound_data, sound_index);
		}
	}

	return;
}

void sound_initialize(
	void)
{
	struct sound_preferences *preferences;
	short platform_code;
	struct sound_platform_definition *platform_definition;
	short channel_index;
	short channel_type;

	sound_manager_globals.initialized = FALSE;
	sound_manager_globals.enabled = TRUE;
	read_sound_preferences(&preferences);
	sound_cache_new();
	sound_manager_globals.sound_environment = default_sound_environment;
	sound_manager_globals.nondialog_gain = 1.f;

	platform_code = preferences->platform;
	if (platform_code < 0 || platform_code >= 2)
	{
		return;
	}

	platform_definition = platform_definitions[platform_code];
	if (!platform_definition || platform_definition->platform_code != platform_code)
	{
		return;
	}

	sound_manager_globals.platform_definition = platform_definition;
	sound_data = data_new("sounds", 0x200, sizeof(struct sound_datum));
	if (!sound_data)
	{
		return;
	}

	looping_sound_data = data_new("looping sounds", 0x80, 0xE4);
	if (!looping_sound_data)
	{
		return;
	}

	if (!sound_manager_globals.platform_definition->initialize(preferences))
	{
		return;
	}

	channel_index = 0;
	data_make_valid(sound_data);
	data_make_valid(looping_sound_data);

	for (channel_type = 0; channel_type < NUMBEROF(sound_channel_type_flags); channel_type++)
	{
		short index;

		sound_manager_globals.channel_count +=
			preferences->virtual_channel_counts[channel_type];
		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_manager.c",
			0x168,
			sound_manager_globals.channel_count<=MAXIMUM_SOUND_CHANNELS);

		for (
			index = 0;
			index < preferences->virtual_channel_counts[channel_type];
			index++)
		{
			struct sound_channel_datum *channel = channel_get(channel_index++);

			channel->sound_index = NONE;
			channel->type_flags = sound_channel_type_flags[channel_type];
			channel->playing_permutation = NULL;
			channel->queued_permutation = NULL;
		}
	}

	sound_manager_globals.initialized = TRUE;

	return;
}

void sound_stop_impulse(
	long sound_index)
{
	if (sound_try_and_get(sound_index))
	{
		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_manager.c",
			0x2C1,
			sound_get(sound_index)->type==_sound_impulse);

		if (sound_get(sound_index)->type == _sound_impulse)
		{
			sound_start_fade(
				_sound_fade_mode_linear,
				0.3f,
				NONE,
				sound_index);
		}
	}

	return;
}

void sound_stop_impulse_by_source_and_definition(
	long source_identifier,
	long definition_index)
{
	long sound_index = data_next_index(sound_data, NONE);

	while (sound_index != NONE)
	{
		struct sound_datum *sound = sound_get(sound_index);

		if (sound->type == _sound_impulse &&
			sound->source_identifier == source_identifier &&
			sound->definition_index == definition_index)
		{
			sound_stop_impulse(sound_index);
			break;
		}

		sound_index = data_next_index(sound_data, sound_index);
	}

	return;
}

/* ---------- private code */

static void sound_update_time(
	void)
{
	long render_time = system_milliseconds();

	sound_manager_globals.ticks_elapsed =
		(real)(render_time - sound_manager_globals.render_time) * 0.029999999f;
	sound_manager_globals.render_time = render_time;

	return;
}

static real sound_scale_value(
	real base,
	real lower_bound_modifier,
	real upper_bound_modifier,
	real scale)
{
	return ((upper_bound_modifier - lower_bound_modifier) * scale +
		lower_bound_modifier) * base;
}

static boolean sound_definition_is_playable(
	long definition_index)
{
	struct sound_definition *definition = sound_definition_get(definition_index);

	if (definition->pitch_ranges.count)
	{
		struct sound_pitch_range *pitch_range = TAG_BLOCK_GET_ELEMENT(
			&definition->pitch_ranges,
			0,
			struct sound_pitch_range);

		if (pitch_range->permutations.count &&
			!sound_class_get(definition->sound_class)->disabled)
		{
			return TRUE;
		}
	}

	return FALSE;
}

static struct sound_channel_datum *channel_get(
	short index)
{
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_manager.c",
		0x428,
		index>=0 && index<sound_manager_globals.channel_count);

	return &sound_channels[index];
}

static struct sound_listener *listener_get(
	short index)
{
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_manager.c",
		0x430,
		index>=0 && index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);

	return &sound_manager_globals.listeners[index];
}

static short sound_definition_promote(
	long definition_index)
{
	struct sound_definition *definition = sound_definition_get(definition_index);
	short promotion_count = definition->promotion_count;
	long longest_permutation_length;
	long promotion_limit;
	long promotion_counter;

	if (!promotion_count)
	{
		return _sound_promotion_dont;
	}

	longest_permutation_length = definition->longest_permutation_length;
	promotion_limit = longest_permutation_length * promotion_count;
	promotion_counter = definition->promotion_time - sound_manager_globals.render_time +
		definition->promotion_counter;
	definition->promotion_counter = promotion_counter;

	promotion_counter = MAX(promotion_counter, 0);
	definition->promotion_counter = promotion_counter;
	definition->promotion_time = sound_manager_globals.render_time;
	definition->promotion_counter = longest_permutation_length + promotion_counter;

	if (longest_permutation_length + promotion_counter > promotion_limit)
	{
		if (definition->promotion_sound.index == NONE)
		{
			definition->promotion_counter = promotion_counter;
			return _sound_promotion_dont_play;
		}

		definition->promotion_counter = 0;
		return _sound_promotion_do;
	}

	return _sound_promotion_dont;
}

static real sound_manager_master_gain(
	short class_index)
{
	real gain = sound_class_get_gain(class_index);

	if (class_index != _sound_class_scripted_dialog_to_player &&
		class_index != _sound_class_scripted_dialog_to_other &&
		class_index != _sound_class_scripted_dialog_force_unspatialized)
	{
		gain *= sound_manager_globals.nondialog_gain;
	}

	return gain;
}

static void sound_delete(
	long sound_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_manager.c",
		0x4CF,
		sound_get(sound_index)->playing_channel_index==NONE);
	datum_delete(sound_data, sound_index);

	return;
}

static long sound_travel_milliseconds(
	real distance)
{
	return (long)(distance * oo_speed_of_sound);
}

static boolean refresh_sound(
	long sound_index)
{
	struct sound_datum *sound = sound_get(sound_index);

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_manager.c",
		0x59F,
		sound->playing_channel_index==NONE ||
			channel_get(sound->playing_channel_index)->sound_index==sound_index);

	if (TEST_FLAG(sound->flags, _sound_delayed_bit) ||
		!sound->track_proc ||
		sound->start_time >= sound_manager_globals.render_time ||
		sound->track_proc(
			sound->source_identifier,
			sound->track_data,
			&sound->source))
	{
		return TRUE;
	}

	if (sound->type == _sound_impulse &&
		!sound_class_get(
			sound_definition_get(sound->definition_index)->sound_class)->speech)
	{
		sound->track_proc = NULL;
		return TRUE;
	}

	return FALSE;
}

static boolean track_loop_track_sound(
	long looping_sound_index,
	void const *track_data,
	struct sound_source *source)
{
	struct looping_sound_datum *looping_sound =
		datum_try_and_get(looping_sound_data, looping_sound_index);

	if (!looping_sound)
	{
		return FALSE;
	}

	*source = looping_sound->source;

	return TRUE;
}

static void sound_set_definition_begin(
	long sound_index,
	long definition_index)
{
	struct sound_datum *sound = datum_get(sound_data, sound_index);

	if (sound->definition_index != definition_index)
	{
		sound->next_definition_index = definition_index;
	}

	return;
}

static long looping_sound_find(
	long identifier)
{
	long looping_sound_index = data_next_index(looping_sound_data, NONE);

	while (looping_sound_index != NONE)
	{
		struct looping_sound_datum *looping_sound =
			datum_get(looping_sound_data, looping_sound_index);

		if (looping_sound->loop_identifier == identifier)
		{
			break;
		}

		looping_sound_index = data_next_index(
			looping_sound_data,
			looping_sound_index);
	}

	return looping_sound_index;
}

static real limit_pitch(
	real desired_pitch,
	real old_pitch,
	real maximum_bend)
{
	if (maximum_bend == 0.f || desired_pitch == old_pitch)
	{
		return desired_pitch;
	}

	if (desired_pitch > old_pitch)
	{
		return MIN(desired_pitch, old_pitch * maximum_bend);
	}

	return MAX(desired_pitch, old_pitch / maximum_bend);
}

static void render_debug_sound(
	long sound_index)
{
	if (debug_sound)
	{
		struct sound_datum *sound = sound_get(sound_index);
		char string[512];

		sound_definition_get(sound->definition_index);
		render_debug_sphere(
			FALSE,
			&sound->source.location.position,
			sound_definition_get_maximum_distance(sound->definition_index),
			global_real_argb_yellow);
		render_debug_sphere(
			FALSE,
			&sound->source.location.position,
			sound_definition_get_minimum_distance(sound->definition_index),
			global_real_argb_red);
		sprintf(
			string,
			"%s|n%f %f",
			tag_get_name(sound->definition_index),
			sound->source.obstruction,
			sound->source.occlusion);
		render_debug_string_at_point(
			FALSE,
			&sound->source.location.position,
			string,
			global_real_argb_white);
	}

	return;
}

static real sound_scale_random_value(
	real base_lower_bound,
	real base_upper_bound,
	real lower_bound_modifier,
	real upper_bound_modifier,
	real scale)
{
	real base = real_seed_random_range(
		get_global_local_random_seed_address(),
		base_lower_bound,
		base_upper_bound);

	return sound_scale_value(
		base,
		lower_bound_modifier,
		upper_bound_modifier,
		scale);
}

static real sound_calculate_fade(
	long sound_index)
{
	struct sound_datum *sound = sound_get(sound_index);
	real fade = 1.f;

	if (sound->fade_start_time != sound->fade_stop_time)
	{
		fade = ((real)sound_manager_globals.render_time - sound->fade_start_time) /
			(sound->fade_stop_time - sound->fade_start_time);
		fade = PIN(fade, 0.f, 1.f);

		switch (sound->fade_mode)
		{
		case _sound_fade_mode_linear:
			break;

		case _sound_fade_mode_crossfade:
			if (sound->fade_interpolation_end > sound->fade_interpolation_start)
			{
				fade = (real)pow(fade, 1.f / sound_fade_exponent);
			}
			else
			{
				fade = (real)(1.0 - pow(
					1.f - fade,
					1.f / sound_fade_exponent));
			}
			break;

		default:
			match_vassert(
				"c:\\halo\\SOURCE\\sound\\sound_manager.c",
				0xA76,
				FALSE,
				NULL);
			break;
		}

		if (fade == 1.f)
		{
			sound->fade_stop_time = 0;
			sound->fade_start_time = 0;
		}

		fade = (sound->fade_interpolation_end -
			sound->fade_interpolation_start) * fade +
			sound->fade_interpolation_start;
	}

	return fade;
}

static void sound_start_fade(
	short mode,
	real seconds,
	long fade_in_sound_index,
	long fade_out_sound_index)
{
	long fade_start_time;
	long fade_stop_time;

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_manager.c",
		0x43F,
		mode==_sound_fade_mode_linear || mode==_sound_fade_mode_crossfade);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_manager.c",
		0x440,
		seconds>=0.f);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_manager.c",
		0x441,
		fade_in_sound_index!=NONE || fade_out_sound_index!=NONE);

	fade_start_time = sound_manager_globals.render_time - 1;
	fade_stop_time = (long)(seconds * 1000.f + fade_start_time);
	fade_stop_time = MAX(fade_stop_time, sound_manager_globals.render_time);

	if (fade_in_sound_index != NONE)
	{
		struct sound_datum *sound = sound_get(fade_in_sound_index);

		if (sound->fade_start_time != sound->fade_stop_time)
		{
			sound->fade_interpolation_start =
				sound_calculate_fade(fade_in_sound_index);
		}
		else
		{
			sound->fade_interpolation_start = 0.f;
		}

		sound->fade_interpolation_end = 1.f;
		sound->fade_mode = mode;
		sound->fade_start_time = fade_start_time;
		sound->fade_stop_time = fade_stop_time;
	}

	if (fade_out_sound_index != NONE)
	{
		struct sound_datum *sound = sound_get(fade_out_sound_index);

		sound->fade_interpolation_start =
			sound_calculate_fade(fade_out_sound_index);
		sound->fade_interpolation_end = 0.f;
		sound->fade_mode = mode;
		sound->fade_start_time = fade_start_time;
		sound->fade_stop_time = fade_stop_time;
	}

	return;
}

static short channel_get_state(
	short channel_index)
{
	struct sound_channel_datum *channel = channel_get(channel_index);
	short state = sound_manager_globals.platform_definition->get_channel_state(
		channel_index);

	if (channel->queued_permutation && state < 2)
	{
		sound_cache_sound_finished(channel->playing_permutation);
		channel->playing_permutation = channel->queued_permutation;
		channel->queued_permutation = NULL;
		channel->estimated_tick_time = 0.f;

		if (!_sound_cache_sound_request(
			channel->playing_permutation,
			FALSE,
			FALSE,
			FALSE))
		{
			state = 0;
		}
	}

	if (channel->playing_permutation && state < 1)
	{
		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_manager.c",
			0x868,
			!channel->queued_permutation);
		sound_cache_sound_finished(channel->playing_permutation);
		channel->playing_permutation = NULL;
	}

	channel->estimated_tick_time +=
		channel->pitch * sound_manager_globals.ticks_elapsed;

	return state;
}

static void channel_stop(
	short channel_index)
{
	struct sound_channel_datum *channel = channel_get(channel_index);

	if (channel->queued_permutation)
	{
		sound_cache_sound_finished(channel->queued_permutation);
		channel->queued_permutation = NULL;
	}

	if (channel->playing_permutation)
	{
		sound_cache_sound_finished(channel->playing_permutation);
		channel->playing_permutation = NULL;
	}

	sound_manager_globals.platform_definition->stop_channel(channel_index);

	return;
}

static void sound_stop(
	long sound_index)
{
	struct sound_datum *sound = sound_get(sound_index);
	struct sound_definition *definition =
		sound_definition_get(sound->definition_index);

	if (sound->playing_channel_index != NONE)
	{
		channel_get(sound->playing_channel_index)->sound_index = NONE;
		channel_stop(sound->playing_channel_index);
		sound->playing_channel_index = NONE;
	}
	else if (TEST_FLAG(sound->flags, _sound_cached_bit))
	{
		struct sound_permutation *permutation = sound_permutation_get(
			sound->definition_index,
			sound->pitch_range_index,
			sound->permutation_index);

		sound_cache_sound_finished(permutation);
	}

	if (sound->type != _sound_impulse)
	{
		struct looping_sound_datum *looping_sound = datum_try_and_get(
			looping_sound_data,
			sound->source_identifier);

		if (looping_sound)
		{
			looping_sound->component_sound_count--;
			if (looping_sound->tracks[sound->loop_track_index].primary_sound_index ==
				sound_index)
			{
				looping_sound->tracks[sound->loop_track_index].primary_sound_index = NONE;
			}
		}
	}

	if (definition->scripting_sound_index == sound_index)
	{
		definition->scripting_sound_index = NONE;
	}

	sound_delete(sound_index);

	return;
}

static real source_distance(
	short listener_index,
	struct sound_source *source)
{
	real distance;

	switch (source->spatialization_mode)
	{
	case _sound_spatialization_mode_none:
		distance = 0.f;
		break;

	case _sound_spatialization_mode_absolute:
		{
			real_point3d const *listener_position =
				&listener_get(listener_index)->matrix.position;
			real x = listener_position->x - source->location.position.x;
			real y = listener_position->y - source->location.position.y;
			real z = listener_position->z - source->location.position.z;

			distance = square_root(y * y + (x * x + z * z));
		}
		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_manager.c",
			0x58D,
			listener_get(listener_index)->valid);
		break;

	case _sound_spatialization_mode_relative:
		distance = square_root(
			source->location.position.x * source->location.position.x +
			source->location.position.y * source->location.position.y +
			source->location.position.z * source->location.position.z);
		break;

	default:
		/* BUG (original): January returns the uninitialized distance after
		 * reporting an invalid spatialization mode. The HCEA binary preserves
		 * the same invalid-mode stack read; valid enum values never take it. */
		match_vassert(
			"c:\\halo\\SOURCE\\sound\\sound_manager.c",
			0x593,
			FALSE,
			NULL);
		break;
	}

	return distance;
}

static real source_distance_squared(
	short listener_index,
	struct sound_source *source)
{
	real distance_squared;

	switch (source->spatialization_mode)
	{
	case _sound_spatialization_mode_none:
		distance_squared = 0.f;
		break;

	case _sound_spatialization_mode_absolute:
		distance_squared = distance_squared3d(
			&source->location.position,
			&listener_get(listener_index)->matrix.position);
		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_manager.c",
			0x574,
			listener_get(listener_index)->valid);
		break;

	case _sound_spatialization_mode_relative:
		distance_squared =
			source->location.position.x * source->location.position.x +
			source->location.position.y * source->location.position.y +
			source->location.position.z * source->location.position.z;
		break;

	default:
		match_vassert(
			"c:\\halo\\SOURCE\\sound\\sound_manager.c",
			0x57A,
			FALSE,
			NULL);
		break;
	}

	return distance_squared;
}

void sound_stop_all(
	void)
{
	if (sound_manager_globals.initialized)
	{
		long sound_index = data_next_index(sound_data, NONE);

		while (sound_index != NONE)
		{
			sound_stop(sound_index);
			sound_index = data_next_index(sound_data, sound_index);
		}

		data_delete_all(looping_sound_data);
		sound_manager_globals.platform_definition->flush();
	}

	sound_manager_globals.game_time_when_no_scripted_dialog_will_be_playing = 0;

	return;
}

long sound_new_impulse(
	long definition_index,
	struct sound_source *source,
	long source_identifier,
	boolean (*track_proc)(
		long source_identifier,
		void const *track_data,
		struct sound_source *source),
	void const *track_data,
	short track_data_size)
{
	struct sound_definition *definition = sound_definition_get(definition_index);
	real scale = source->scale;
	long result = NONE;
	short listener_index;
	short promotion_result;
	long sound_index;
	struct sound_datum *sound;
	long travel_milliseconds;

	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_manager.c",
		0x240,
		track_data_size<=MAXIMUM_SOUND_CALLBACK_DATA);
	match_assert(
		"c:\\halo\\SOURCE\\sound\\sound_manager.c",
		0x242,
		source->spatialization_mode==_sound_spatialization_mode_none ||
			valid_real_normal3d(&source->location.forward));

	if (definition->sound_class == _sound_class_scripted_dialog_to_player ||
		definition->sound_class == _sound_class_scripted_dialog_to_other ||
		definition->sound_class ==
			_sound_class_scripted_dialog_force_unspatialized)
	{
		long game_time = game_time_get();
		long dialog_stop_time = game_time +
			30 * definition->longest_permutation_length / 1000 + 10;

		if (dialog_stop_time >
			sound_manager_globals.game_time_when_no_scripted_dialog_will_be_playing)
		{
			sound_manager_globals.game_time_when_no_scripted_dialog_will_be_playing =
				dialog_stop_time;
		}

		if (loud_dialog_hack)
		{
			source->spatialization_mode = _sound_spatialization_mode_none;
		}
	}

	if (definition->sound_class ==
		_sound_class_scripted_dialog_force_unspatialized)
	{
		source->spatialization_mode = _sound_spatialization_mode_none;
	}

	if (!sound_manager_globals.initialized || !sound_manager_globals.enabled)
	{
		return result;
	}

	if (definition->compression != _sound_compression_xbox_adpcm ||
		(definition->encoding != _sound_encoding_stereo &&
			(definition->encoding != _sound_encoding_mono ||
				definition->sample_rate != 0)))
	{
		error(
			_error_silent,
			"attempt to play a sound that was not a mono 22k compressed sound or a stereo 22k or 44k compressed sound.");
		return result;
	}

	if (source->scale == 0.f && definition->zero_gain_modifier == 0.f)
	{
		return result;
	}

	if (real_seed_random(get_global_local_random_seed_address()) <=
		sound_scale_value(
			definition->skip_fraction,
			definition->zero_skip_fraction_modifier,
			definition->one_skip_fraction_modifier,
			scale))
	{
		return result;
	}

	{
		real maximum_distance =
			sound_definition_get_maximum_distance(definition_index);

		if (!sound_definition_is_playable(definition_index))
		{
			return result;
		}

		listener_index = source_audible(source, maximum_distance);
	}

	if (listener_index == NONE)
	{
		return result;
	}

	promotion_result = sound_definition_promote(definition_index);
	if (promotion_result != _sound_promotion_dont)
	{
		if (promotion_result == _sound_promotion_do)
		{
			return sound_new_impulse(
				definition->promotion_sound.index,
				source,
				source_identifier,
				track_proc,
				track_data,
				track_data_size);
		}

		return result;
	}

	sound_index = datum_new(sound_data);
	if (sound_index == NONE)
	{
		return result;
	}

	result = sound_index;
	sound = sound_get(sound_index);
	travel_milliseconds = sound_travel_milliseconds(
		source_distance(listener_index, source));

	sound->definition_index = definition_index;
	sound->playing_channel_index = NONE;
	sound->listener_index = listener_index;
	sound->type = _sound_impulse;
	sound->source_identifier = source_identifier;
	sound->flags = 0;
	sound->pitch = sound_scale_random_value(
		definition->random_pitch_bounds.lower,
		definition->random_pitch_bounds.upper,
		definition->zero_pitch_modifier,
		definition->one_pitch_modifier,
		source->scale);
	sound->source = *source;
	sound->track_proc = track_proc;

	if (track_proc)
	{
		match_assert(
			"c:\\halo\\SOURCE\\sound\\sound_manager.c",
			0x28E,
			sound->track_data);
		csmemcpy(sound->track_data, track_data, track_data_size);
	}

	sound->pitch_range_index = sound_definition_find_pitch_range_by_pitch(
		definition,
		sound->pitch,
		NONE);
	sound->permutation_index = sound_definition_next_permutation(
		definition,
		sound->pitch_range_index,
		NONE);
	sound->loop_track_index = NONE;
	sound->fade_stop_time = 0;
	sound->fade_start_time = 0;
	_sound_cache_sound_request(
		sound_permutation_get(
			sound->definition_index,
			sound->pitch_range_index,
			sound->permutation_index),
		FALSE,
		TRUE,
		FALSE);

	if (travel_milliseconds > speed_of_sound_threshold)
	{
		sound->start_time = sound_manager_globals.render_time + travel_milliseconds;
		SET_FLAG(sound->flags, _sound_delayed_bit, TRUE);
	}
	else
	{
		sound->start_time = sound_manager_globals.render_time;
	}

	return result;
}

void sound_dispose_from_old_map(
	void)
{
	if (!sound_manager_globals.paused &&
		sound_manager_globals.initialized &&
		sound_manager_globals.enabled)
	{
		long start_time = (long)system_milliseconds();
		long sound_index = data_next_index(sound_data, NONE);

		if (sound_index != NONE)
		{
			real stop_time;

			do
			{
				sound_start_fade(
					_sound_fade_mode_linear,
					0.3f,
					NONE,
					sound_index);
				sound_index = data_next_index(sound_data, sound_index);
			}
			while (sound_index != NONE);

			stop_time = (real)start_time + 300.f;
			while ((real)system_milliseconds() < stop_time)
			{
				sound_idle();
			}
		}
	}

	if (sound_manager_globals.paused)
	{
		sound_manager_globals.paused = FALSE;
		sound_manager_globals.platform_definition->set_pause(FALSE);
		sound_manager_globals.render_time = system_milliseconds();
	}

	sound_stop_all();
	if (looping_sound_data)
	{
		data_delete_all(looping_sound_data);
	}

	return;
}

static short source_audible(
	struct sound_source *source,
	real maximum_distance)
{
	short nearest_listener_index = NONE;
	short listener_index;

	if (source->spatialization_mode == _sound_spatialization_mode_none)
	{
		return 0;
	}

	if (source->spatialization_mode == _sound_spatialization_mode_relative)
	{
		if (source_distance_squared(NONE, source) < maximum_distance)
		{
			return 0;
		}
	}
	else
	{
		real nearest_distance_squared = REAL_MAX;

		for (
			listener_index = 0;
			listener_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
			listener_index++)
		{
			if (listener_get(listener_index)->valid)
			{
				real distance_squared = source_distance_squared(
					listener_index,
					source);

				if (distance_squared < nearest_distance_squared)
				{
					nearest_listener_index = listener_index;
					nearest_distance_squared = distance_squared;
				}
			}
		}

		if (nearest_listener_index != NONE)
		{
			compute_sound_obstruction(
				nearest_listener_index,
				source,
				square_root(nearest_distance_squared));
		}

		if (nearest_distance_squared > maximum_distance * maximum_distance ||
			source->occlusion == 1.f)
		{
			return NONE;
		}
	}

	return nearest_listener_index;
}

static void refresh_sounds(
	void)
{
	boolean all_players_dead = players_are_all_dead();
	boolean dialog_playing = FALSE;
	long sound_index = data_next_index(sound_data, NONE);

	while (sound_index != NONE)
	{
		struct sound_datum *sound = sound_get(sound_index);
		struct sound_definition *definition =
			sound_definition_get(sound->definition_index);
		word channel_index = (word)sound->playing_channel_index;
		boolean stop_sound = FALSE;

		if (channel_index != (word)NONE &&
			!channel_get_state((short)channel_index) &&
			sound->type != _sound_loop_track &&
			sound->type != _sound_stopping_track)
		{
			stop_sound = TRUE;
		}

		if (!stop_sound && !refresh_sound(sound_index))
		{
			stop_sound = TRUE;
		}

		if (!stop_sound)
		{
			short listener_index = source_audible(
				&sound->source,
				sound_definition_get_maximum_distance(
					sound->definition_index));

			render_debug_sound(sound_index);

			if (definition->sound_class ==
					_sound_class_scripted_dialog_to_player ||
				definition->sound_class ==
					_sound_class_scripted_dialog_to_other ||
				definition->sound_class ==
					_sound_class_scripted_dialog_force_unspatialized)
			{
				dialog_playing = TRUE;
			}

			if (listener_index == NONE)
			{
				if (!TEST_FLAG(sound->flags, _sound_inaudible_bit))
				{
					sound_start_fade(
						_sound_fade_mode_linear,
						sound_inaudible_fade_out_time,
						NONE,
						sound_index);
					SET_FLAG(sound->flags, _sound_inaudible_bit, TRUE);
				}
			}
			else
			{
				sound->listener_index = listener_index;
				if (TEST_FLAG(sound->flags, _sound_inaudible_bit))
				{
					sound_start_fade(
						_sound_fade_mode_linear,
						sound_inaudible_fade_back_in_time,
						sound_index,
						NONE);
					SET_FLAG(sound->flags, _sound_inaudible_bit, FALSE);
				}
			}

			if (all_players_dead)
			{
				if (definition->sound_class ==
					_sound_class_scripted_dialog_to_player)
				{
					if (sound->playing_channel_index == NONE)
					{
						stop_sound = TRUE;
					}
					else
					{
						sound_start_fade(
							_sound_fade_mode_linear,
							sound_player_fade_out_time,
							NONE,
							sound_index);
					}
				}
				else if (definition->sound_class ==
						_sound_class_scripted_dialog_to_other &&
					sound->playing_channel_index == NONE)
				{
					stop_sound = TRUE;
				}
			}
		}

		if (stop_sound)
		{
			sound_stop(sound_index);
		}

		sound_index = data_next_index(sound_data, sound_index);
	}

	if (dialog_playing)
	{
		real rate = sound_manager_globals.ticks_elapsed * 0.03f;
		real delta = sound_gain_under_dialog -
			sound_manager_globals.nondialog_gain;

		sound_manager_globals.nondialog_gain += PIN(delta, -rate, rate);
	}
	else
	{
		real rate = sound_manager_globals.ticks_elapsed * 0.007f;
		real delta = 1.f - sound_manager_globals.nondialog_gain;

		sound_manager_globals.nondialog_gain += PIN(delta, -rate, rate);
	}

	return;
}
