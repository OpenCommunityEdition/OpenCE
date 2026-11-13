/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "cseries/profile_rasterizer.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_console_vars.h"

#include <xtl.h>

#include "rasterizer_xbox.h"

/* ---------- constants */

enum
{
	MAXIMUM_RASTERIZER_PROFILE_CALLBACKS = 16,
	MAXIMUM_RASTERIZER_PROFILE_ERROR_REPORTS = 3
};

enum
{
	_rasterizer_profile_error_invalid_context = 0,
	_rasterizer_profile_error_begin_out_of_synch,
	_rasterizer_profile_error_end_out_of_synch,
	NUMBER_OF_RASTERIZER_PROFILE_ERRORS
};

enum
{
	_rasterizer_profile_frame_callback_end_bit = 0,
	_rasterizer_profile_callback_begin_bit = 31
};

/* ---------- macros */

/* ---------- structures */

struct rasterizer_profile_globals
{
	short active_profile_index;
	short pad02;
	short window_index;
	short pad06;
	const char *profile_names[NUMBER_OF_RASTERIZER_PROFILES];
};

typedef char rasterizer_profile_globals_size_assert[
	sizeof(struct rasterizer_profile_globals) == 124 ? 1 : -1];
typedef char rasterizer_profile_globals_window_index_offset_assert[
	offsetof(struct rasterizer_profile_globals, window_index) == 4 ? 1 : -1];
typedef char rasterizer_profile_globals_profile_names_offset_assert[
	offsetof(struct rasterizer_profile_globals, profile_names) == 8 ? 1 : -1];

struct rasterizer_profile_elapsed_state
{
	__int64 callback_start_times[MAXIMUM_RASTERIZER_PROFILE_CALLBACKS];
	__int64 pushbuffer_elapsed_times[NUMBER_OF_RASTERIZER_PROFILES];
	/* Written by the D3D callback and sampled by the CPU query path. */
	volatile __int64 elapsed_times[NUMBER_OF_RASTERIZER_PROFILES];
};

typedef char rasterizer_profile_elapsed_state_size_assert[
	sizeof(struct rasterizer_profile_elapsed_state) == 592 ? 1 : -1];
typedef char rasterizer_profile_elapsed_state_pushbuffer_offset_assert[
	offsetof(struct rasterizer_profile_elapsed_state, pushbuffer_elapsed_times) == 128 ? 1 : -1];
typedef char rasterizer_profile_elapsed_state_elapsed_offset_assert[
	offsetof(struct rasterizer_profile_elapsed_state, elapsed_times) == 360 ? 1 : -1];

struct rasterizer_profile_frame_state
{
	__int64 overhead;
	__int64 total;
	short callback_index;
	short pad12;
	short last_callback_index;
	short pad16;
};

typedef char rasterizer_profile_frame_state_size_assert[
	sizeof(struct rasterizer_profile_frame_state) == 24 ? 1 : -1];
typedef char rasterizer_profile_frame_state_last_callback_index_offset_assert[
	offsetof(
		struct rasterizer_profile_frame_state,
		last_callback_index) == 20 ? 1 : -1];

struct rasterizer_profile_state
{
	long profile_flags;
	/* Set by the D3D callback and drained by the CPU frame-begin path. */
	volatile short callback_errors;
	byte reserved06[6];
};

typedef char rasterizer_profile_state_size_assert[
	sizeof(struct rasterizer_profile_state) == 12 ? 1 : -1];

/* ---------- prototypes */

static boolean rasterizer_profile_enabled(
	void);

static void profile_assert(
	boolean condition,
	short profile,
	const char *message);
static void callback_function(
	unsigned long context);
static void frame_callback_function(
	unsigned long context);

/* ---------- globals */


static LARGE_INTEGER rasterizer_profile_performance_counter_frequency = { 1 };
static struct rasterizer_profile_globals rasterizer_profile_globals =
{
	NONE,
	0,
	NONE,
	0,
	{
		"clear",
		"model sky",
		"models",
		"env lightmaps",
		"env shadows",
		"env lights",
		"env decals light",
		"env decals alpha-tested",
		"env textures",
		"env decals primary",
		"env decals secondary",
		"env lights specular",
		"env lightmaps specular",
		"env lightmaps ref.mask",
		"env reflection mirrors",
		"env reflections",
		"env transparent",
		"env fog",
		"env fog screen",
		"water",
		"env decals water",
		"detail objects",
		"queued transparents",
		"lens flare occl. submit",
		"lens flare occl. query",
		"lens flares",
		"screen effect",
		"HUD",
		"screen flash"
	}
};

static __int64 rasterizer_profile_callback_elapsed_times[MAXIMUM_RASTERIZER_PROFILE_CALLBACKS] = { 0 };
static __int64 rasterizer_profile_callback_end_times[MAXIMUM_RASTERIZER_PROFILE_CALLBACKS] = { 0 };
static struct rasterizer_profile_elapsed_state rasterizer_profile_elapsed_state = { 0 };
/* Initialized by the CPU and consumed by asynchronous D3D callbacks. */
static volatile __int64 rasterizer_profile_start_times[NUMBER_OF_RASTERIZER_PROFILES] = { 0 };
static struct rasterizer_profile_frame_state rasterizer_profile_frame_state = { 0 };
static short local_profile_enable = 0;
static struct rasterizer_profile_state rasterizer_profile_state = { 0 };
static short rasterizer_profile_error_count = 0;

/* ---------- public code */

boolean rasterizer_profile_initialize(
	void)
{
	long profile_count;
	long profile_index;

	profile_count = NUMBER_OF_RASTERIZER_PROFILES;
	profile_index = 0;
	do
	{
		rasterizer_profile_start_times[profile_index] = 0;
		rasterizer_profile_elapsed_state.elapsed_times[profile_index] = 0;
		profile_index++;
	}
	while (--profile_count);

	csmemset(rasterizer_profile_elapsed_state.callback_start_times, 0, sizeof(rasterizer_profile_elapsed_state.callback_start_times));
	csmemset((void *)rasterizer_profile_callback_end_times, 0, sizeof(rasterizer_profile_callback_end_times));
	csmemset((void *)rasterizer_profile_callback_elapsed_times, 0, sizeof(rasterizer_profile_callback_elapsed_times));
	QueryPerformanceFrequency(&rasterizer_profile_performance_counter_frequency);

	return TRUE;
}

void rasterizer_profile_frame_begin(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
		194,
		global_d3d_device);

	profile_assert(
		!TEST_FLAG(rasterizer_profile_state.callback_errors, _rasterizer_profile_error_invalid_context),
		NONE,
		"callback recieved invalid context");
	profile_assert(
		!TEST_FLAG(rasterizer_profile_state.callback_errors, _rasterizer_profile_error_begin_out_of_synch),
		NONE,
		"begin out-of-synch");
	profile_assert(
		!TEST_FLAG(rasterizer_profile_state.callback_errors, _rasterizer_profile_error_end_out_of_synch),
		NONE,
		"end out-of-synch");

	rasterizer_profile_state.callback_errors = 0;

	if (rasterizer_profile_enabled())
	{
		rasterizer_profile_state.profile_flags = 0;
		rasterizer_profile_globals.window_index = 0;
		rasterizer_profile_globals.active_profile_index = NONE;
		rasterizer_profile_frame_state.overhead = 0;
		rasterizer_profile_frame_state.callback_index = (short)((rasterizer_profile_frame_state.callback_index+1)%MAXIMUM_RASTERIZER_PROFILE_CALLBACKS);
		D3DDevice_InsertCallback(D3DCALLBACK_READ, frame_callback_function, rasterizer_profile_frame_state.callback_index*2);
	}

	return;
}

void rasterizer_profile_window_begin(
	void)
{
	rasterizer_profile_globals.window_index = global_window_parameters.window_index;
	rasterizer_profile_globals.active_profile_index = NONE;

	return;
}

void _rasterizer_profile_enable(
	boolean enable)
{
	if (enable)
	{
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
			244,
			local_profile_enable>0);
		local_profile_enable--;
	}
	else
	{
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
			249,
			local_profile_enable<100);
		local_profile_enable++;
	}

	return;
}

void rasterizer_profile_begin(
	short profile)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
		259,
		global_d3d_device);

	if (rasterizer_profile_enabled() && rasterizer_profile_globals.window_index==0 && local_profile_enable==0)
	{
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
			265,
			profile>=0 && profile<NUMBER_OF_RASTERIZER_PROFILES);
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
			266,
			global_d3d_device);

		profile_assert(
			!TEST_FLAG(rasterizer_profile_state.profile_flags, profile),
			profile,
			"profile duplication within frame (begin)");
		profile_assert(
			rasterizer_profile_globals.active_profile_index==NONE,
			profile,
			"profile begin/end pairing incorrect (begin)");

		D3DDevice_InsertCallback(D3DCALLBACK_READ, callback_function, profile|FLAG(_rasterizer_profile_callback_begin_bit));
		rasterizer_profile_globals.active_profile_index = profile;
		rasterizer_profile_elapsed_state.pushbuffer_elapsed_times[profile] = 0;
	}

	return;
}

void rasterizer_profile_end(
	short profile)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
		294,
		global_d3d_device);

	if (rasterizer_profile_enabled() && rasterizer_profile_globals.window_index==0 && local_profile_enable==0)
	{
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
			300,
			profile>=0 && profile<NUMBER_OF_RASTERIZER_PROFILES);
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
			301,
			global_d3d_device);

		profile_assert(
			!TEST_FLAG(rasterizer_profile_state.profile_flags, profile),
			profile,
			"profile duplication within frame (end)");
		profile_assert(
			rasterizer_profile_globals.active_profile_index==profile,
			profile,
			"profile begin/end pairing incorrect (end)");

		D3DDevice_InsertCallback(D3DCALLBACK_WRITE, callback_function, profile);
		rasterizer_profile_elapsed_state.pushbuffer_elapsed_times[profile] = 0;
		rasterizer_profile_globals.active_profile_index = NONE;
		SET_FLAG(rasterizer_profile_state.profile_flags, profile, TRUE);
	}

	return;
}

const char *rasterizer_profile_get_string(
	short profile)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
		363,
		profile>=0 && profile<NUMBER_OF_RASTERIZER_PROFILES);

	return rasterizer_profile_globals.profile_names[profile];
}

real rasterizer_profile_query(
	short profile)
{
	real result = 0.0f;

	if (rasterizer_profile_enabled())
	{
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
			375,
			global_d3d_device);

		if (profile==NUMBER_OF_RASTERIZER_PROFILES)
		{
			short profile_index;

			for (profile_index=0; profile_index<NUMBER_OF_RASTERIZER_PROFILES; profile_index++)
			{
				__int64 elapsed = rasterizer_profile_elapsed_state.elapsed_times[profile_index];

				result += (real)elapsed;
			}

			result /= (real)rasterizer_profile_performance_counter_frequency.QuadPart;
		}
		else
		{
			match_assert(
				"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
				391,
				profile>=0 && profile<NUMBER_OF_RASTERIZER_PROFILES);

			profile_assert(
				rasterizer_profile_globals.active_profile_index==NONE,
				profile,
				"profile not completed (query)");

			if (TEST_FLAG(rasterizer_profile_state.profile_flags, profile))
			{
				__int64 elapsed = rasterizer_profile_elapsed_state.elapsed_times[profile];

				result = (real)elapsed / (real)rasterizer_profile_performance_counter_frequency.QuadPart;
			}
			else
			{
				result = -1.0f;
			}
		}
	}

	return result;
}

long rasterizer_profile_query_pushbuffer(
	short profile)
{
	long result = 0;

	if (rasterizer_profile_enabled())
	{
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
			415,
			global_d3d_device);

		if (profile==NUMBER_OF_RASTERIZER_PROFILES)
		{
			return 0;
		}

		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
			430,
			profile>=0 && profile<NUMBER_OF_RASTERIZER_PROFILES);

		profile_assert(
			rasterizer_profile_globals.active_profile_index==NONE,
			profile,
			"profile not completed (query)");

		if (TEST_FLAG(rasterizer_profile_state.profile_flags, profile))
		{
			result = (long)MIN(rasterizer_profile_elapsed_state.pushbuffer_elapsed_times[profile], LONG_MAX);
		}
		else
		{
			result = NONE;
		}
	}

	return result;
}

void rasterizer_profile_frame_end(
	void)
{
	if (rasterizer_profile_enabled())
	{
		match_assert(
			"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
			455,
			global_d3d_device);

		rasterizer_profile_frame_state.total = (long)MIN(-rasterizer_profile_frame_state.overhead, LONG_MAX);

		D3DDevice_InsertCallback(D3DCALLBACK_WRITE, frame_callback_function, rasterizer_profile_frame_state.callback_index*2+1);

		profile_rasterizer_stats(
			(real)rasterizer_profile_callback_elapsed_times[rasterizer_profile_frame_state.last_callback_index]*1000/(real)rasterizer_profile_performance_counter_frequency.QuadPart,
			rasterizer_profile_frame_state.total);
	}

	return;
}

void rasterizer_profile_window_end(
	void)
{
	return;
}

void rasterizer_profile_dispose(
	void)
{
	return;
}

/* ---------- private code */

static boolean rasterizer_profile_enabled(
	void)
{
	return rasterizer_debug_options.statistics_mode == 3 ||
		rasterizer_debug_options.profile_log_enabled;
}

static void profile_assert(
	boolean condition,
	short profile,
	const char *message)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
		60,
		message);

	if (!condition && rasterizer_profile_error_count<MAXIMUM_RASTERIZER_PROFILE_ERROR_REPORTS)
	{
		if (profile!=NONE)
		{
			error(
				_error_silent,
				"### PROFILE (#%d): %s -- tell Bernie!",
				profile,
				message);
		}
		else
		{
			error(_error_silent, "### PROFILE: %s -- tell Bernie!", message);
		}

		rasterizer_profile_error_count++;
	}

	return;
}

static void callback_function(
	unsigned long context)
{
	LARGE_INTEGER counter;
	boolean begin = TEST_FLAG(context, _rasterizer_profile_callback_begin_bit);
	short profile = (short)context;

	if (profile>=0 && profile<NUMBER_OF_RASTERIZER_PROFILES)
	{
		QueryPerformanceCounter(&counter);

		if (begin)
		{
			if (rasterizer_profile_start_times[profile]!=0)
			{
				SET_FLAG(rasterizer_profile_state.callback_errors, _rasterizer_profile_error_begin_out_of_synch, TRUE);
			}

			rasterizer_profile_start_times[profile] = counter.QuadPart;
		}
		else
		{
			if (rasterizer_profile_start_times[profile]!=0)
			{
				rasterizer_profile_elapsed_state.elapsed_times[profile] = counter.QuadPart-rasterizer_profile_start_times[profile];
				rasterizer_profile_start_times[profile] = 0;
			}
			else
			{
				SET_FLAG(rasterizer_profile_state.callback_errors, _rasterizer_profile_error_end_out_of_synch, TRUE);
			}
		}
	}
	else
	{
		SET_FLAG(rasterizer_profile_state.callback_errors, _rasterizer_profile_error_invalid_context, TRUE);
	}

	return;
}

static void frame_callback_function(
	unsigned long context)
{
	LARGE_INTEGER counter;
	short callback_index = (short)(context>>1);

	if (callback_index>=0 && callback_index<MAXIMUM_RASTERIZER_PROFILE_CALLBACKS)
	{
		QueryPerformanceCounter(&counter);

		if (TEST_FLAG(context, _rasterizer_profile_frame_callback_end_bit))
		{
			rasterizer_profile_callback_end_times[callback_index] = counter.QuadPart;
			rasterizer_profile_callback_elapsed_times[callback_index] = counter.QuadPart-rasterizer_profile_elapsed_state.callback_start_times[callback_index];
			rasterizer_profile_frame_state.last_callback_index = callback_index;
		}
		else
		{
			rasterizer_profile_elapsed_state.callback_start_times[callback_index] = counter.QuadPart;
		}
	}

	return;
}
