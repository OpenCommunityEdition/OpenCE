/* ---------- headers */

#include "cseries/cseries.h"
#include "cache/cache_files.h"
#include "cseries/errors.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

/* ---------- macros */

#define LOAD_UI_TAG(group, path, message) \
	if (tag_load(group, path, 0) == NONE) \
		error(_error_silent, message)

#define SCENARIO_GET(tag_index) ((struct scenario *)tag_get('scnr', tag_index))

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void ui_load_tags_for_scenario(
	long scenario_tag_index)
{
	struct scenario *scenario = SCENARIO_GET(scenario_tag_index);

	LOAD_UI_TAG('ustr', "ui\\multiplayer_game_text", "failed to load the multiplayer game text string list tag");
	LOAD_UI_TAG('bitm', "ui\\shell\\bitmaps\\white", "generic white texture bitmap");

	switch (scenario->type)
	{
	case _scenario_type_solo:
		LOAD_UI_TAG('Soul', "ui\\shell\\solo", "failed to load the solo scenario ui_widget_collection tag");
		LOAD_UI_TAG('ustr', "ui\\shell\\strings\\temp_strings", "failed to load miscellaneous localized strings tag");
		break;

	case _scenario_type_multiplayer:
		LOAD_UI_TAG('Soul', "ui\\shell\\multiplayer", "failed to load the multiplayer scenario ui_widget_collection tag");
		break;

	case _scenario_type_main_menu:
		LOAD_UI_TAG('Soul', "ui\\shell\\main_menu", "failed to load the main menu scenario ui_widget_collection_tag");
		LOAD_UI_TAG('vcky', "ui\\english", "failed to load the browser's virtual keyboard tag");
		LOAD_UI_TAG('ustr', "ui\\random_player_names", "failed to load random player names string list tag");
		LOAD_UI_TAG('mply', "ui\\multiplayer_scenarios", "failed to load the multiplayer scenario description tag");
		LOAD_UI_TAG('ustr', "ui\\saved_game_file_strings", "failed to load the default saved game filename string list tag");
		LOAD_UI_TAG('ustr', "ui\\default_multiplayer_game_setting_names", "failed to load the default playlist profile names string list tag");
		LOAD_UI_TAG('ustr', "ui\\shell\\strings\\game_variant_descriptions", "failed to load the multiplayer variant description string list tag");
		LOAD_UI_TAG('ustr', "ui\\shell\\main_menu\\player_profiles_select\\difficulty_names", "failed to load the game difficulty name string list tag");
		LOAD_UI_TAG('ustr', "ui\\shell\\strings\\default_player_profile_names", "failed to load the default player profile names string list tag");
		LOAD_UI_TAG('ustr', "ui\\shell\\main_menu\\player_profiles_select\\button_set_long_descriptions", "failed to load the player profile button set long descriptions string list tag");
		LOAD_UI_TAG('ustr', "ui\\shell\\main_menu\\player_profiles_select\\button_set_short_descriptions", "failed to load the player profile short descriptions string list tag");
		LOAD_UI_TAG('ustr', "ui\\shell\\main_menu\\player_profiles_select\\joystick_set_defaults_descriptions", "failed to load the default player profile joystick set descriptions string list tag");
		LOAD_UI_TAG('ustr', "ui\\shell\\main_menu\\player_profiles_select\\joystick_set_short_descriptions", "failed to load the player profile joystick set short descriptions string list tag");
		LOAD_UI_TAG('ustr', "ui\\shell\\main_menu\\player_profiles_select\\profile_description_labels", "failed to load the player profile description labels string list tag");
		LOAD_UI_TAG('snd!', "sound\\sfx\\ui\\cursor", "failed to load ui cursor sound tag");
		LOAD_UI_TAG('snd!', "sound\\sfx\\ui\\forward", "failed to load ui forward sound tag");
		LOAD_UI_TAG('snd!', "sound\\sfx\\ui\\back", "failed to load ui back sound tag");
		LOAD_UI_TAG('snd!', "sound\\sfx\\ui\\flag_failure", "failed to load ui failure sound tag");
		LOAD_UI_TAG('lsnd', "sound\\music\\title1\\title1", "failed to load main menu title music");
		break;

	default:
		display_assert("unknown scenario type", "c:\\halo\\SOURCE\\interface\\ui_widget_group.c", 506, TRUE);
		system_exit(-1);
		break;
	}

	return;
}

/* ---------- private code */
