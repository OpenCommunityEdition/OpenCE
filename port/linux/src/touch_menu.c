/*
TOUCH_MENU.C

The menus' pointer from a touchscreen (touch_menu.h).
*/

#include "touch_menu.h"

#include <math.h>
#include <string.h>

void touch_menu_init(struct touch_menu *menu, const struct touch_menu_settings *settings)
{
	memset(menu, 0, sizeof(*menu));
	menu->settings = *settings;
}

void touch_menu_reset(struct touch_menu *menu)
{
	struct touch_menu_settings settings = menu->settings;

	touch_menu_init(menu, &settings);
}

void touch_menu_down(struct touch_menu *menu, unsigned long long finger, float x, float y)
{
	if (menu->finger_down)
		return;
	menu->finger_down = 1;
	menu->finger = finger;
	menu->down_x = menu->last_x = x;
	menu->down_y = menu->last_y = y;
	menu->scrolling = 0;
	menu->remainder = 0.0f;
	/* a finger on the screen stops a list still scrolling, so that a tap
	hits what the finger sees */
	menu->pending_steps = 0;
}

void touch_menu_move(struct touch_menu *menu, unsigned long long finger, float x, float y)
{
	float along;

	if (!menu->finger_down || finger != menu->finger)
		return;
	if (!menu->scrolling)
	{
		float dx = x - menu->down_x, dy = y - menu->down_y;

		if (dx * dx + dy * dy <= menu->settings.slop * menu->settings.slop)
			return;
		/* the steps count from the slop's edge on the axis: the slop is not
		a step, and a fast first sample is not lost */
		menu->scrolling = 1;
		menu->axis = fabsf(dx) > fabsf(dy);
		if (menu->axis)
			menu->last_x = menu->down_x + (dx > 0.0f ? menu->settings.slop : -menu->settings.slop);
		else
			menu->last_y = menu->down_y + (dy > 0.0f ? menu->settings.slop : -menu->settings.slop);
	}
	along = menu->axis ? x - menu->last_x : y - menu->last_y;
	menu->last_x = x;
	menu->last_y = y;
	menu->remainder += along;
	while (menu->remainder >= menu->settings.step)
	{
		menu->pending_steps++;
		menu->remainder -= menu->settings.step;
	}
	while (menu->remainder <= -menu->settings.step)
	{
		menu->pending_steps--;
		menu->remainder += menu->settings.step;
	}
}

void touch_menu_up(struct touch_menu *menu, unsigned long long finger, float x, float y)
{
	if (!menu->finger_down || finger != menu->finger)
		return;
	touch_menu_move(menu, finger, x, y);
	if (!menu->scrolling)
	{
		menu->output.moved = 1;
		menu->output.x = menu->output.click_x = menu->down_x;
		menu->output.y = menu->output.click_y = menu->down_y;
		menu->output.clicks++;
	}
	menu->finger_down = 0;
	menu->scrolling = 0;
}

void touch_menu_cancel(struct touch_menu *menu)
{
	menu->finger_down = 0;
	menu->scrolling = 0;
	menu->remainder = 0.0f;
	menu->pending_steps = 0;
}

void touch_menu_read(struct touch_menu *menu, struct touch_menu_output *output)
{
	int limit = menu->settings.steps_per_read;
	int steps = menu->pending_steps;

	if (steps > limit)
		steps = limit;
	if (steps < -limit)
		steps = -limit;
	*output = menu->output;
	output->wheel_steps = steps;
	menu->pending_steps -= steps;
	menu->output.moved = 0;
	menu->output.clicks = 0;
}
