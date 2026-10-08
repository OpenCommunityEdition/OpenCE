/* Widget pause accounting, real clock pause and real looping-sound expiry.
   The fake scene refreshes its loop only while game time is active, as the
   menu's game_sound_update does. No assets, audio device or user profile. */
#include "harness.h"
#include "config.inc"
#define match_vassert(file, line, condition, message) CHECK(condition, "%s", message)

struct widget_instance { boolean pause_game_time; };
struct definition { unsigned flags; };
static struct { long pause_game_time_count; boolean sound_paused; } widget_globals;
static struct { boolean initialized, active, paused; } clock_state = { TRUE, TRUE, FALSE }, *game_time_globals = &clock_state;
static boolean we_are_at_the_main_menu, coop, audio_paused;
static int sound_pause_calls, resets, scene_frames;
static boolean network_coop_active(void) { return coop; }
static boolean game_time_get_paused(void) { return clock_state.paused; }
static void sound_pause(boolean pause) { audio_paused = pause; sound_pause_calls++; }
static void main_menu_ensure_player_queues_exist(void) { }
static void game_time_dispose_from_old_map(void) { resets++; }
static void game_time_initialize_for_new_map(void) { }
static void game_time_start(void) { clock_state.active = TRUE; }
struct loop { struct datum_header header; boolean flip_flop; };
static struct { boolean flip_flop; } sound_manager_globals;
static struct data_array *looping_sound_data;
static long looping_sound_index;

#include "under_test.inc"

static void frames(int count)
{
	struct loop *music = datum_get(looping_sound_data, looping_sound_index);
	while (count--)
	{
		if (clock_state.active)
		{
			scene_frames++;
			music->flip_flop = sound_manager_globals.flip_flop;
		}
		if (!audio_paused)
		{
			expire_loop(music);
			sound_manager_globals.flip_flop = !sound_manager_globals.flip_flop;
		}
	}
}

static void check_running(void)
{
	CHECK(clock_state.active && !clock_state.paused, "scene clock stopped");
	CHECK(!widget_globals.pause_game_time_count && !audio_paused && !sound_pause_calls, "unexpected pause accounting/audio call");
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	struct definition paused = { FLAG(_widget_pause_game_time_bit) }, ordinary = { 0 };
	struct widget_instance a = { 0 }, b = { 0 }, error = { 0 };
	looping_sound_data = game_state_data_new("music", 1, sizeof(struct loop));
	looping_sound_index = datum_new(looping_sound_data);
	CASE("main-menu")
	{
		we_are_at_the_main_menu = TRUE;
		open_widget(&a, &paused);
		open_widget(&b, &paused);
		check_running();
		frames(7200);
		CHECK(scene_frames == 7200 && looping_sound_data->count == 1, "scene/music did not continue through menu frames");
		close_widget(&a); close_widget(&b);
		CHECK(!resets, "closing a shell menu reset game time");
		check_running();
		return 0;
	}
	CASE("main-menu-error")
	{
		we_are_at_the_main_menu = TRUE;
		open_widget(&error, &ordinary);
		error_pause(&error, TRUE);
		check_running();
		frames(120);
		CHECK(scene_frames == 120 && looping_sound_data->count == 1, "error dialog interrupted scene/music");
		close_widget(&error);
		CHECK(!resets, "error dialog reset shell clock");
		return 0;
	}
	CASE("campaign-nested")
	{
		open_widget(&a, &paused); open_widget(&b, &paused);
		CHECK(clock_state.paused && !clock_state.active && audio_paused, "campaign did not pause");
		CHECK(widget_globals.pause_game_time_count == 2 && sound_pause_calls == 1, "nested campaign pause accounting");
		frames(120);
		CHECK(!scene_frames && looping_sound_data->count == 1, "paused campaign ticked or music expired");
		close_widget(&a);
		CHECK(clock_state.paused && audio_paused && widget_globals.pause_game_time_count == 1, "closed parent resumed child");
		close_widget(&b);
		CHECK(clock_state.active && !clock_state.paused && !audio_paused && sound_pause_calls == 2 && !resets, "last menu did not resume campaign normally");
		return 0;
	}
	CASE("campaign-error")
	{
		open_widget(&a, &paused);
		open_widget(&error, &ordinary);
		error_pause(&error, TRUE); error_pause(&error, TRUE);
		CHECK(widget_globals.pause_game_time_count == 2 && sound_pause_calls == 1, "error paused twice or ignored campaign pause");
		close_widget(&a);
		CHECK(clock_state.paused && audio_paused, "remaining error did not hold pause");
		close_widget(&error);
		CHECK(!widget_globals.pause_game_time_count && clock_state.active && !audio_paused && sound_pause_calls == 2, "error left campaign paused");
		return 0;
	}
	CASE("coop")
	{
		coop = TRUE;
		open_widget(&a, &paused); open_widget(&error, &ordinary);
		error_pause(&error, TRUE);
		check_running();
		frames(120);
		CHECK(scene_frames == 120, "co-op simulation stopped");
		close_widget(&a); close_widget(&error);
		check_running();
		return 0;
	}
	CASE("unrequested")
	{
		open_widget(&a, &ordinary); error_pause(&a, FALSE);
		check_running(); close_widget(&a); check_running();
		return 0;
	}
	CHECK(FALSE, "unknown case %s", case_name);
}
