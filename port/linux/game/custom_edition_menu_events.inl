/* CE map-authored widgets use Halo PC function numbers. This adapter is
included beside the widget dispatcher because widget instances are private
there. Generated menus and Xbox/MCC widgets never enter it. */
static short custom_edition_menu_event(struct widget_instance *widget,
	struct event_record *event, word function_index, boolean *widget_deleted)
{
	if (!custom_edition_menu_widget(widget))
		return NONE;
	/* port: a Custom Edition map's own screens (its pause menu's New Game)
	name Halo PC's functions, which this build does not have; the three the
	port needs are run here, the rest succeed and do nothing so that what
	their handlers open opens */
	if (function_index == _halo_pc_function_back)
	{
		extern boolean pc_menu_network_back(struct widget_instance *widget);
		extern boolean pc_menu_gamepad_back(struct widget_instance *widget);

		if (pc_menu_gamepad_back(widget) || pc_menu_network_back(widget))
			return TRUE;
	}
	if (function_index == _halo_pc_function_back)
	{
		extern void ui_widget_port_go_back(struct widget_instance *widget);

		ui_widget_port_go_back(widget);
		*widget_deleted = TRUE;
		return TRUE;
	}
	if (function_index == _halo_pc_function_map_row)
	{
		extern void pc_menu_map_row_click(struct widget_instance *row);

		/* port: a Custom Edition map's colour screen's row: its first and last rows page the colours; the others
		choose one and do the screen's OK */
		if (widget->parent && widget->parent->name && !strcmp(widget->parent->name, "color_select_list"))
		{
			extern boolean pc_menu_color_row_click(struct widget_instance *row);

			pc_menu_color_row_click(widget);
			return TRUE;
		}
		pc_menu_map_row_click(widget);
		return TRUE;
	}
	if (function_index == _halo_pc_function_map_ok)
	{
		extern boolean pc_menu_activate_screen(struct widget_instance *widget,
			struct event_record *event, boolean *widget_deleted);

		return pc_menu_activate_screen(widget, event, widget_deleted);
	}
	if (function_index == _halo_pc_function_restart)
	{
		extern boolean pc_menu_restart_game(boolean *widget_deleted);

		return pc_menu_restart_game(widget_deleted);
	}
	if (function_index == _halo_pc_function_change_settings)
	{
		/* port: the settings screen the pause menu opens closes at once unless
		a profile is being edited ("close if not editing profile"), so
		player 1's is begun first; with none, the screen is not opened */
		return pc_menu_profile_edit_begin();
	}
		if (function_index == 113)
	{
		extern boolean pc_menu_controls_load(void);
		struct widget_instance *panel;

		/* port: a Custom Edition map's controls screen holds its advanced
		settings (the mouse's sensitivities) as a second panel over the first,
		shown when they are asked for: it starts hidden, and disabled so that
		it takes no focus (this runs on the screen's created event, before
		the focus is given) */
		for (panel = widget->child; panel; panel = panel->next)
		{
			if (!strcmp(panel->name, "advanced_settings_screen"))
			{
				panel->visible = FALSE;
				panel->disabled = TRUE;
			}
			else if (!strcmp(panel->name, "controls_options_menu") && panel->child &&
				!strcmp(panel->child->name, "op_device"))
			{
				/* port: the first device row has nothing to choose in a
				map's controls screen: the command groups' row is the second */
				panel->child->visible = FALSE;
				panel->child->disabled = TRUE;
			}
		}
		return pc_menu_controls_load();
	}
	if (function_index == 115)
	{
		extern boolean pc_menu_controls_begin_binding(struct widget_instance *widget);

		return pc_menu_controls_begin_binding(widget);
	}
	if (function_index == 127)
	{
		extern boolean pc_menu_controls_save(void);

		return pc_menu_controls_save();
	}
	if (function_index == 136)
	{
		extern boolean pc_menu_controls_defaults(void);

		return pc_menu_controls_defaults();
	}

	/* port: a Custom Edition map's colour screen (its list is named
	color_select_list; the port's own is color_select_list_2) runs the Xbox's
	functions 61 (created) and 63 (OK), which assume the Xbox's spinner list:
	the port's colour functions run instead */
	if ((function_index == 61 || function_index == 63) && widget->name &&
		!strcmp(widget->name, "color_select_list"))
	{
		extern boolean pc_menu_color_initialize(struct widget_instance *list);
		extern boolean pc_menu_color_choose(void);

		return function_index == 61 ? pc_menu_color_initialize(widget) : pc_menu_color_choose();
	}
	if (function_index == 166 || function_index == 167 || function_index == 187)
	{
		extern short pc_menu_network_function(struct widget_instance *widget, word function);
		short network_result = pc_menu_network_function(widget, function_index);

		if (network_result != NONE)
			return network_result;
	}
	if (function_index == 114 || function_index == 125 || function_index == 135)
	{
		extern short pc_menu_video_function(struct widget_instance *widget, word function,
			boolean *widget_deleted);
		short video_result = pc_menu_video_function(widget, function_index, widget_deleted);

		if (video_result != NONE)
			return video_result;
	}
	if (function_index == 123 || function_index == 124 || function_index == 134)
	{
		extern short pc_menu_audio_function(struct widget_instance *widget, word function);
		short audio_result = pc_menu_audio_function(widget, function_index);

		if (audio_result != NONE)
			return audio_result;
	}
	if (function_index == 121 || function_index == 122 || function_index == 133)
	{
		extern short pc_menu_mouse_function(struct widget_instance *widget, word function);
		short mouse_result = pc_menu_mouse_function(widget, function_index);

		if (mouse_result != NONE)
			return mouse_result;
	}

	if (function_index >= 129 && function_index <= 132)
	{
		extern short pc_menu_gamepad_function(struct widget_instance *widget, word function);
		short gamepad_result = pc_menu_gamepad_function(widget, function_index);

		if (gamepad_result != NONE)
			return gamepad_result;
	}
	if (function_index == 152 && widget->name && !strcmp(widget->name, "controls_settings_screen"))
	{
		extern boolean pc_menu_controls_back(struct widget_instance *widget, boolean *widget_deleted);

		return pc_menu_controls_back(widget, widget_deleted);
	}
	if (function_index >= 102 && function_index < PC_MENU_FUNCTION_BASE)
		return TRUE;
	return NONE;
}
