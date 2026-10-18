/*
MENU_CURSOR.C

The menus' mouse pointer (menu_cursor.h): a redraw of the Halo PC and Mac
menus' own cursor, as a player knows it: a blue arrowhead pointing up and
left, a white and lavender bevelled edge round a deep blue body with a bright
blue glint, and two lavender chevrons stacked under it, about 20 by 30 menu
pixels, its tip the hot spot. It was drawn by eye for this port, a row at a
time below ('o' white edge, 'l' lavender, 'm' mid blue, 'b' glint, 'd' deep
blue), after looking at the game's cursor; no pixel of the game's is copied,
and nothing was traced. It is drawn row by row as quads in the menus' own
640x480 coordinates.
*/

#include "cseries.h"
#include "menu_cursor.h"
#include "cutscene/cinematics.h"
#include "input_prompts.h"

int config_boolean(const char *name);

static const char menu_cursor_rows[][24] =
{
	"o",
	"oo",
	"olo",
	"ollo",
	"oldlo",
	"olddlo",
	"oldbdlo",
	"oldbddlo",
	"oldbbddlo",
	"oldbbdddlo",
	"oldbbbdddlo",
	"oldbbbbdddlo",
	"oldbbbbddddlo",
	"oldbbbbbddddlo",
	"oldddddddddddlo",
	"olddddddddddddlo",
	"oldddddddddddddlo",
	"olddddddddddlo",
	"oldddddddlo       ll",
	"olddddlo        llmm",
	"oldlo         llmm",
	"oo         lllmm",
	"         llmmm",
	"       llmm",
	"     llmm     ll",
	"     mm     llmm",
	"         lllmm",
	"       llmmm",
	"     llmm",
	"     mm"
};

/* the colour of each letter of the rows above (ARGB) */
static pixel32 menu_cursor_color(
	char shade)
{
	switch (shade)
	{
	case 'o':
		return 0xffe6ecff;
	case 'l':
		return 0xff9fb0f2;
	case 'm':
		return 0xff6878e8;
	case 'b':
		return 0xff3a5cff;
	}

	return 0xff1424b8;
}

static int menu_cursor_active;
static int menu_cursor_x, menu_cursor_y;

void menu_cursor_note(
	int active,
	const struct halo_ui_pointer *pointer)
{
	menu_cursor_active = active && pointer && !pointer->touch && config_boolean("input.menu_cursor");
	if (menu_cursor_active)
	{
		menu_cursor_x = pointer->x;
		menu_cursor_y = pointer->y;
	}

	return;
}

int menu_cursor_replaces_system(void)
{
	return menu_cursor_active;
}

void menu_cursor_draw(
	void)
{
	long row;

	if (!menu_cursor_active || !platform_prompts_keyboard())
		return;
	for (row = 0; row < (long)NUMBEROF(menu_cursor_rows); row++)
	{
		const char *cells = menu_cursor_rows[row];
		long column = 0;

		while (cells[column])
		{
			if (cells[column] == ' ')
			{
				column++;
			}
			else
			{
				/* a run of one colour: one quad */
				rectangle2d span;
				char shade = cells[column];
				long start = column;

				while (cells[column] == shade)
					column++;
				span.x0 = (short)(menu_cursor_x + start);
				span.x1 = (short)(menu_cursor_x + column);
				span.y0 = (short)(menu_cursor_y + row);
				span.y1 = span.y0 + 1;
				draw_quad(&span, menu_cursor_color(shade));
			}
		}
	}

	return;
}
