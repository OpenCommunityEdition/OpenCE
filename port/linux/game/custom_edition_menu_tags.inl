/* A CE pause menu that supplies Halo PC's Change Settings action keeps
its own widgets and layout. Adding the generated Settings button would
obscure that screen and can move authored controls out of their artwork. */
static boolean custom_edition_pause_has_settings(struct tag_block const *screens, long quit_function)
{
	long screen;

	if (!custom_edition_cache_tags_loaded() || mcc_cache_tags_loaded())
		return FALSE;
	for (screen = 0; screen < screens->count; screen++)
	{
		long tag = ((struct tag_reference const *)screens->address)[screen].index;
		struct ui_widget_definition const *definition;
		long child;

		if (tag == NONE)
			continue;
		definition = tag_get(UI_WIDGET_DEFINITION_TAG, tag);
		child = pause_list_child(definition, quit_function);
		if (child != NONE)
		{
			struct ui_widget_child_reference const *children = definition->child_widgets.address;
			struct ui_widget_definition const *list = tag_get(UI_WIDGET_DEFINITION_TAG, children[child].widget_tag.index);

			if (pause_quit_button(list, 137) != NONE)
				return TRUE;
		}
	}
	return FALSE;
}
