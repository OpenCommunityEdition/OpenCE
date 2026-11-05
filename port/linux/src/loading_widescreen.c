/* Only the stock loading effect's horizontal framing changes. The game
 * still draws its original picture, mask and feedback blur. Keep the
 * choice fixed until the session ends so drawing and capture agree. */
#include "loading_widescreen.h"
#include "port_config.h"

static int widescreen_active;

void loading_widescreen_begin(void)
{
	widescreen_active = config_boolean("display.widescreen_loading");
}

int loading_widescreen_active(void)
{
	return widescreen_active;
}

void loading_widescreen_end(void)
{
	widescreen_active = 0;
}
