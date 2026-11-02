/* GulchCE's frontend selection is latched at the main-menu boundary.
 * It never changes display.menus, which also governs legacy pause menus. */
#include "cseries.h"
#include "frontend_menu.h"
#include "frontend_input.h"
#include "keyboard_glyphs.h"
#include "../src/port_config.h"
#include "tag_files/tag_groups.h"
#include <string.h>

boolean main_menu_is_active(void);
boolean network_game_is_active(void);

static int frontend_menu_loaded;

#include "frontend_network_routes.inl"

int frontend_menu_requested(void)
{
	char const *value = config_string("display.menu_frontend");
	return !strcmp(value, "modern") || !strcmp(value, "pc") || !strcmp(value, "xbox");
}

void frontend_menu_map_loaded(char const *name, int mcc)
{
	frontend_menu_loaded = !mcc && !strcmp(name, "ui") && frontend_menu_requested();
	frontend_input_enable(frontend_menu_loaded);
}

void frontend_menu_unloaded(void)
{
	frontend_menu_session_reset();
	frontend_menu_loaded = 0;
	frontend_input_enable(0);
	frontend_menu_text_end();
	frontend_panel_end();
	frontend_glyphs_dispose();
	keyboard_glyphs_dispose();
}

int frontend_menu_available(void)
{
	return frontend_menu_loaded && tag_loaded('DeLa', FRONTEND_MENU_SCREEN) != NONE;
}

char const *frontend_menu_root(char const *legacy)
{
	char const *value, *root;

	/* Loading the optional set also makes PC tags available to an Xbox
	 * frontend. A missing modern screen must still respect its old policy. */
	if (frontend_menu_loaded && !strcmp(legacy, FRONTEND_MENU_PC) &&
		csstrcasecmp(config_string("display.menus"), "pc"))
		legacy = FRONTEND_MENU_XBOX;
	if (!frontend_menu_available())
		return legacy;
	value = config_string("display.menu_frontend");
	root = !strcmp(value, "pc") ? FRONTEND_MENU_PC : !strcmp(value, "xbox") ? FRONTEND_MENU_XBOX : legacy;
	return tag_loaded('DeLa', root) != NONE ? root : legacy;
}

/* Explicit root routes only. A pregame host/client is active on ui.map;
 * that is allowed here, but never in the separate style-switching policy. */
long frontend_menu_destination(long tag)
{
	char const *name;
	long modern;
	unsigned int index;
	if (tag == NONE || !frontend_menu_available() || !main_menu_is_active() ||
		strcmp(config_string("display.menu_frontend"), "modern")) return tag;
	name = tag_get_name(tag);
	if (!name) return tag;
	for (index = 0; index < sizeof(frontend_network_routes) / sizeof(frontend_network_routes[0]); index++)
		if (!strcmp(name, frontend_network_routes[index].legacy))
		{
			modern = tag_loaded('DeLa', frontend_network_routes[index].modern);
			return modern == NONE ? tag : modern;
		}
	if (network_game_is_active() || strcmp(name, "pc\\main_menu\\multiplayer_type_select\\multiplayer_type_select_screen")) return tag;
	modern = tag_loaded('DeLa', "pc\\modern\\multiplayer\\screen");
	return modern == NONE ? tag : modern;
}
