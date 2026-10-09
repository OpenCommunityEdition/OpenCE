/*
TIFF_FILE.C

port: the builds leave out source/bitmaps/tiff_file.c and the libtiff it
writes with (port/linux/port.json). Its one function, tiff_export, is
called by main.c's screenshot_record: for the Xbox's movie recorder
(main_movie_start, which nothing starts) and for the screenshot_count and
screenshot_size globals (screenshot_render). Those read the back buffer's
memory, which the port draws into with OpenGL, never the CPU, so they never
had the picture; the port's screenshots are its own (the Screenshot key,
controls.screenshot, and debug.screenshot_every: d3d8_gl.c).
*/

#include "cseries/cseries.h"
#include "bitmaps/tiff_file.h"

char const *tiff_export(
	struct file_reference *file,
	struct bitmap_data *bitmap)
{
	(void)file;
	(void)bitmap;
	return "this build writes no TIFF screenshots: use the Screenshot key (controls.screenshot)";
}
