/* Modern frontend geometry uses the engine's centered 640-column coordinates.
 * Definitions remain immutable; the renderer and hit testing share a local copy. */
#include "cseries.h"
#include "frontend_menu.h"
#include "tag_files/tag_files.h"
#include <string.h>
#include <stdlib.h>

long halo_screen_width(void);
static int frontend_text_active;
#include "frontend_network_layout.inl"

static int frontend_network_role(char const *name)
{
	unsigned int i;
	for (i = 0; i < sizeof(frontend_network_widgets) / sizeof(frontend_network_widgets[0]); i++)
		if (!strcmp(name, frontend_network_widgets[i].name)) return frontend_network_widgets[i].role;
	return -1;
}

static int frontend_network_geometry(char const *name, int screen, short *left, short *right, short *shift)
{
	int extra = frontend_layout_width(screen) - 640, rows = extra * 3 / 5;
	int origin = -extra / 2;
	*shift = 0;
	switch (frontend_network_role(name))
	{
	case FRONTEND_NET_FULL: *left = (short)(-(screen - 640) / 2); *right = (short)(*left + screen); break;
	case FRONTEND_NET_ROW: case FRONTEND_NET_HEADING:
		*shift = (short)origin; *right += (short)rows; break;
	case FRONTEND_NET_DETAIL: case FRONTEND_NET_DETAIL_WRAP:
		*shift = (short)(origin + rows); *right += (short)(extra - rows); break;
	case FRONTEND_NET_DETAIL_FIXED: case FRONTEND_NET_DETAIL_ICON:
		*shift = (short)(origin + rows + (extra - rows) / 2); break;
	case FRONTEND_NET_LABEL: *right += (short)(rows * 2 / 3); break;
	case FRONTEND_NET_LABEL_FULL: case FRONTEND_NET_CHOOSER: *right += (short)rows; break;
	case FRONTEND_NET_VALUE: *shift = (short)(rows * 2 / 3); *right += (short)(rows - rows * 2 / 3); break;
	case FRONTEND_NET_BAR: case FRONTEND_NET_STATUS: *shift = (short)origin; *right += (short)extra; break;
	case FRONTEND_NET_BUTTON: *shift = (short)extra; break;
	}
	return 1;
}

void frontend_menu_text_begin(long tag)
{
	char const *name;
	frontend_text_active = 0;
	if (!frontend_menu_available()) return;
	name = tag_get_name(tag);
	frontend_text_active = name && !strncmp(name, "pc\\modern\\", 10);
}

void frontend_menu_text_end(void)
{
	frontend_text_active = 0;
}

void frontend_menu_clip_text(short left, short right, short *clipped_left, short *clipped_right)
{
	int width, origin;
	if (!frontend_text_active) return;
	width = (int)halo_screen_width();
	origin = -(width - 640) / 2;
	*clipped_left = (short)(left < origin ? origin : left);
	*clipped_right = (short)(right > origin + width ? origin + width : right);
}

int frontend_layout_width(int screen)
{
	return screen < 640 ? 640 : screen > 1280 ? 1280 : screen;
}

int frontend_layout_geometry(char const *name, int screen, short *left, short *right, short *shift)
{
	int width, extra, rows, origin, navigation;

	if (strncmp(name, "pc\\modern\\", 10) || !strncmp(name + 10, "entry", 5))
		return 0;
	if (!strncmp(name, "pc\\modern\\network\\", 18)) return frontend_network_geometry(name, screen, left, right, shift);
	navigation = !strncmp(name + 10, "multiplayer\\", 12) || !strncmp(name + 10, "host\\", 5);
	name = strrchr(name, '\\') + 1;
	width = frontend_layout_width(screen);
	extra = width - 640;
	rows = navigation ? extra * 9 / 20 : extra * 3 / 5;
	origin = -extra / 2;
	*shift = 0;
	if (!strcmp(name, "settings") || !strcmp(name, "screen") || !strcmp(name, "options"))
	{
		*left = (short)(-(screen - 640) / 2);
		*right = (short)(*left + screen);
	}
	else if (!strncmp(name, "row_", 4))
	{
		*shift = (short)origin;
		*right += (short)rows;
	}
	else if (!strncmp(name, "spinner_", 8))
	{
		*shift = (short)(rows - rows / 3);
		*right += (short)(rows / 3);
	}
	else if (!strncmp(name, "label_", 6))
		*right += (short)(navigation ? rows : rows - rows / 3);
	else if (!strcmp(name, "master_servers_value"))
	{
		*shift = (short)(rows - rows / 3);
		*right += (short)(rows / 3);
	}
	else if (!strcmp(name, "help") || !strncmp(name, "help_", 5))
	{
		*shift = (short)(origin + rows);
		*right += (short)(extra - rows);
	}
	else if (!strcmp(name, "button_bar"))
	{
		*shift = (short)origin;
		*right += (short)extra;
	}
	else if (!strcmp(name, "button_defaults"))
		*shift = 0;
	else if (!strncmp(name, "button_", 7))
		*shift = (short)extra;
	else if (!strcmp(name, "tabs"))
		*shift = (short)origin;
	else if (!strncmp(name, "tab_", 4))
		*shift = (short)(atoi(name + 4) * extra / 5);
	else if (!strcmp(name, "title"))
		*shift = (short)origin;
	else if (!strncmp(name, "section", 7) || !strncmp(name, "rule_", 5) ||
		!strcmp(name, "status") || !strcmp(name, "prompts"))
	{
		*shift = (short)origin;
		*right += (short)(!strcmp(name, "prompts") ? extra : rows);
	}
	return 1;
}

int frontend_menu_geometry(long tag, short *left, short *right, short *shift)
{
	char const *name;
	if (!frontend_menu_available())
		return 0;
	name = tag_get_name(tag);
	return name && frontend_layout_geometry(name, (int)halo_screen_width(), left, right, shift);
}

int frontend_menu_wrap(long tag)
{
	char const *name;
	if (!frontend_menu_available())
		return 0;
	name = tag_get_name(tag);
	return name && !strncmp(name, "pc\\modern\\", 10) &&
		(!strcmp(strrchr(name, '\\') + 1, "help") || !strcmp(strrchr(name, '\\') + 1, "help_value") || frontend_network_role(name) == FRONTEND_NET_DETAIL_WRAP);
}

/* Native descriptions contain manual |n breaks (including split words such
 * as wea-|npon). Reflow only the modern instance copy for its current width.
 * Authored alignment belongs to the modern widget, not those legacy strings. */
void frontend_menu_reflow_text(wchar_t *text)
{
	wchar_t *read = text, *write = text;
	while (*read)
	{
		if (*read == L'|' && (read[1] == L'c' || read[1] == L'l' || read[1] == L'r'))
		{
			read += 2;
			continue;
		}
		if (*read == L'\r' || *read == L'\n' || (*read == L'|' && read[1] == L'n'))
		{
			do
			{
				read += *read == L'|' ? 2 : 1;
			} while (*read == L'\r' || *read == L'\n' || (*read == L'|' && read[1] == L'n'));
			if (write > text && write[-1] == L'-') write--;
			else if (write > text && write[-1] != L' ' && *read && *read != L' ') *write++ = L' ';
			continue;
		}
		*write++ = *read++;
	}
	*write = 0;
}

/* Stock previews occupy a subrectangle of a padded bitmap. Keep the native
 * texel placement for them; only our independently generated art stretches. */
int frontend_menu_stretch_bitmap(long tag)
{
	char const *name;
	if (!frontend_menu_available()) return 0;
	name = tag_get_name(tag);
	return name && !strncmp(name, "pc\\modern\\", 10) &&
		frontend_network_role(name) != FRONTEND_NET_DETAIL_FIXED;
}

int frontend_menu_bitmap_region(long tag, short *width, short *height)
{
	char const *name;
	if (!frontend_menu_available()) return 0;
	name = tag_get_name(tag);
	if (!name || frontend_network_role(name) != FRONTEND_NET_DETAIL_ICON) return 0;
	name = strrchr(name, '\\') + 1;
	*width = (short)(!strcmp(name, "difficulty_indicator") ? 44 : 200);
	*height = (short)(!strcmp(name, "difficulty_indicator") ? 48 : 211);
	return 1;
}

/* One option-text size for the whole screen, never a size chosen per row.
 * The narrow layout uses the small UI face; wide screens use the large face
 * with both columns expanded. Headings and footer buttons retain their roles. */
long frontend_menu_font(long tag, long normal)
{
	char const *name;
	long font;
	if (!frontend_menu_available()) return normal;
	name = tag_get_name(tag);
	if (!name || strncmp(name, "pc\\modern\\", 10)) return normal;
	if (!strncmp(name, "pc\\modern\\network\\", 18))
	{
		int role = frontend_network_role(name);
		if (role != FRONTEND_NET_LABEL && role != FRONTEND_NET_LABEL_FULL && role != FRONTEND_NET_VALUE && role != FRONTEND_NET_CHOOSER) return normal;
		font = tag_loaded('font', halo_screen_width() < 832 ? "ui\\small_ui" : "ui\\large_ui");
		return font == NONE ? normal : font;
	}
	name = strrchr(name, '\\') + 1;
	if (strncmp(name, "label_", 6) && strncmp(name, "spinner_", 8) && strcmp(name, "master_servers_value")) return normal;
	font = tag_loaded('font', halo_screen_width() < 832 ? "ui\\small_ui" : "ui\\large_ui");
	return font == NONE ? normal : font;
}
