/* CE-style bot controls. Bot settings apply host policy only on OK;
ordinary settings keep the existing load/save and cancellation behavior. */
#include "bots.h"

static boolean bots_menu_event(struct widget_instance *widget, char const *event)
{
	static char const *const names[] = { "op_add_any", "op_add_red", "op_add_blue",
		"op_drop_any", "op_drop_red", "op_drop_blue" };
	/* Deferred error dialogs retain the pointer until the next UI update. */
	static wchar_t error_message[180];
	char status[180];
	wchar_t message[180];
	long index, character;
	struct widget_instance *list;

	if (!strcmp(event, "port bots settings save"))
	{
		settings_each(screen_of(widget), setting_changed_save);
		if (!bots_lobby_settings_apply(status, sizeof(status)))
		{
			for (character = 0; status[character] && character < (long)NUMBEROF(error_message) - 1; character++)
				error_message[character] = (wchar_t)(unsigned char)status[character];
			error_message[character] = 0;
			display_error_text_deferred(error_message, NONE);
		}
		return TRUE;
	}

	if (!strcmp(event, "port bots init"))
	{
		/* This description has no string list: rendering must retain its
		owned text buffer rather than replacing it with a static caption. */
		text_set(widget, L"Host only. Add/remove during a match;\nuse Bot Count in the lobby.");
		return TRUE;
	}
	if (!strcmp(event, "port bots host entry"))
	{
		widget->visible = global_network_game_server_get() != NULL;
		return TRUE;
	}
	for (index = 0; index < (long)NUMBEROF(names); index++)
		if (!strcmp(widget->name, names[index]))
		{
			bots_manage(index < 3, index % 3 - 1, status, sizeof(status));
			for (character = 0; status[character] && character < (long)NUMBEROF(message) - 1; character++)
				message[character] = (wchar_t)(unsigned char)status[character];
			message[character] = 0;
			/* A column list owns its description separately from its children. */
			list = named(screen_of(widget), "options_menu", 0);
			text_set(list ? list->parameters.list.extended_description : NULL, message);
			return TRUE;
		}
	return FALSE;
}
