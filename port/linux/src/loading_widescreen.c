/* Only the stock loading effect's horizontal framing changes. The game
 * still draws its original picture, mask and feedback blur. Keep the
 * choice fixed until the session ends so drawing and capture agree. */
#include "loading_widescreen.h"
#include "port_config.h"

/* the loading picture's proportions (320x240) */
#define LOADING_PICTURE_ASPECT (4.0f / 3.0f)

static int widescreen_active;
static int fill_active;

void loading_widescreen_begin(void)
{
	widescreen_active = config_boolean("display.widescreen_loading");
	fill_active = widescreen_active && config_boolean("display.widescreen_loading_fill");
}

int loading_widescreen_active(void)
{
	return widescreen_active;
}

/* With display.widescreen_loading_fill, the picture keeps its proportions
 * across a widened frame: it fills the frame's width, and the rows above
 * and below what the frame shows are left out (its columns at the sides,
 * for a frame narrower than 4:3). Only the picture's texture coordinates
 * change: the sweep, blur and feedback are drawn over the frame as
 * before. */
void loading_widescreen_picture_crop(float frame_aspect, float *crop_columns, float *crop_rows)
{
	*crop_columns = 0.0f;
	*crop_rows = 0.0f;
	if (!fill_active || !(frame_aspect > 0.0f))
		return;
	if (frame_aspect > LOADING_PICTURE_ASPECT)
		*crop_rows = (1.0f - LOADING_PICTURE_ASPECT / frame_aspect) * 0.5f;
	else
		*crop_columns = (1.0f - frame_aspect / LOADING_PICTURE_ASPECT) * 0.5f;
}

void loading_widescreen_end(void)
{
	widescreen_active = 0;
	fill_active = 0;
}
