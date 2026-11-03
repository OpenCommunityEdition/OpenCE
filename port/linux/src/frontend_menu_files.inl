/* A separate immutable parse preserves the stock Settings set when disabled.
 * The enabled set serves main-menu and paused Settings; map-owned pause roots
 * retain their existing definitions. The ordinary reader owns discovery. */
#include "../game/frontend_menu.h"

struct halo_menus const *frontend_menu_files_load(void)
{
	static int read, succeeded;
	static struct halo_menus menus;
	struct halo_menus const *legacy = halo_menus_load();
	struct reader reader;
	long index;

	if (!legacy || read)
		return succeeded ? &menus : legacy;
	read = 1;
	memset(&reader, 0, sizeof(reader));
	for (index = 0; index < file_count && !reader.failed; index++)
	{
		size_t length = strlen(files[index].path);

		if (length > 4 && !strcmp(files[index].path + length - 4, ".xml"))
			read_file(&reader, &files[index]);
	}
	free(reader.last_child);
	free(reader.last_handler);
	free(reader.last_input);
	free(reader.last_conditional);
	free(reader.last_replace);
	if (reader.failed || !reader.menus.widget_count)
	{
		platform_log("menus: modern frontend unavailable; using legacy menus");
		return legacy;
	}
	menus = reader.menus;
	changelog_menu_configure(&menus, config_boolean("display.changelog"));
	succeeded = 1;
	return &menus;
}
