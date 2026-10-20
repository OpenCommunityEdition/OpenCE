#include "cseries.h"
#include "interface/hud_messaging.h"
#include "math/real_math.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_text.h"
#include "render/render.h"
#include "text/draw_string.h"
#include "port_config.h"
#include "performance_timer_render.h"

struct performance_timer_layout
{
	rectangle2d bounds;
	real anchor_x, anchor_y;
	short justification;
};

static void performance_timer_layout(
	rectangle2d const *window,
	rectangle2d const *viewport,
	long position,
	struct performance_timer_layout *layout)
{
	rectangle2d local = *window;
	/* Explicit rasterizer text bounds are local to the active viewport. */
	local.x0 -= viewport->x0;
	local.x1 -= viewport->x0;
	local.y0 -= viewport->y0;
	local.y1 -= viewport->y0;
	layout->bounds = local;
	layout->justification = 2;
	if (position == 1 || position == 2)
	{
		layout->bounds.y1 = MAX(local.y0, local.y1 - 12);
		layout->bounds.y0 = MAX(local.y0, layout->bounds.y1 - 32);
		layout->anchor_y = layout->bounds.y1;
	}
	else
	{
		/* Preserve the original Top Center layout exactly at 100 percent. */
		layout->bounds.y0 += 12;
		layout->bounds.y1 = MIN(local.y1, layout->bounds.y0 + 32);
		layout->anchor_y = layout->bounds.y0;
	}
	if (position == 2)
	{
		layout->bounds.x1 = MAX(local.x0, local.x1 - 12);
		layout->anchor_x = layout->bounds.x1;
		layout->justification = 1;
	}
	else
	{
		layout->anchor_x = ((real)local.x0 + local.x1) * 0.5f;
	}
}

void performance_timer_render(long ticks)
{
	long font = hud_get_font_index();
	long seconds;
	double configured_scale;
	real scale;
	real_argb_color color;
	struct performance_timer_layout layout;
	char text[32];

	if (ticks < 0 || font == NONE)
		return;
	seconds = ticks / TICKS_PER_SECOND;
	snprintf(text, sizeof(text), "%02ld:%02ld", seconds / 60, seconds % 60);
	configured_scale = config_real("display.timer_scale");
	scale = configured_scale >= 0.5 && configured_scale <= 1.0 ? (real)configured_scale : 1.0f;
	performance_timer_layout(&render.camera.window_bounds, &render.camera.viewport_bounds,
		config_integer("display.timer_position"), &layout);
	if (layout.bounds.x0 >= layout.bounds.x1 || layout.bounds.y0 >= layout.bounds.y1)
		return;
	hud_get_text_color(&color);
	draw_string_set_draw_mode(font, NONE, layout.justification, 0, &color);
	if (scale == 1.0f)
		rasterizer_draw_string(&layout.bounds, NULL, NULL, 0, text);
	else
		rasterizer_draw_string_scaled(&layout.bounds, NULL, text, scale, layout.anchor_x, layout.anchor_y);
}
