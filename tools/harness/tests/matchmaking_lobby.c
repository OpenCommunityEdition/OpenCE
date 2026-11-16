/*
MATCHMAKING_LOBBY.C (test)

A matchmade game's lobby (matchmaking.c).
*/

#include "harness.h"

#define UI_WIDGET_DEFINITION_TAG 0
#define MAIN_MENU_TAG 6
#define MULTIPLAYER_TAG 7
#define NUMBEROF(array) ((short)(sizeof(array) / sizeof((array)[0])))

static const char *loaded[8];
static long previous_of[8];
static int loaded_count;
static boolean closed;

static void ui_widgets_close_all(void)
{
	closed = TRUE;
}

static char const *pc_menus_screen(char const *name)
{
	return name;
}

static long tag_loaded(int group, char const *name)
{
	if (strstr(name, "multiplayer_type_select_screen"))
	{
		return MULTIPLAYER_TAG;
	}
	return strstr(name, "main_menu\\main_menu") ? MAIN_MENU_TAG : NONE;
}

static boolean ui_widget_load_by_name_or_tag(char const *name, long tag, void *parent, short local_player,
	long previous, long focused_parent, short focused_index)
{
	CHECK(closed, "the old history is closed before the lobby loads");
	loaded[loaded_count] = name;
	previous_of[loaded_count++] = previous;
	return TRUE;
}

#include "under_test.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("leave_to_multiplayer_then_main_menu")
	{
		int index;

		matchmaking_open_lobby();
		CHECK(loaded_count == 2, "the lobby loaded twice, not %d times", loaded_count);
		for (index = 0; index < loaded_count; index++)
		{
			CHECK(strstr(loaded[index], "connected_pregame_screen") != NULL, "only the lobby loaded: %s", loaded[index]);
		}
		CHECK(previous_of[0] == MAIN_MENU_TAG, "the first back is the main menu, not %ld", previous_of[0]);
		CHECK(previous_of[1] == MULTIPLAYER_TAG, "the lobby goes back to Multiplayer, not %ld", previous_of[1]);
	}
	return 0;
}
