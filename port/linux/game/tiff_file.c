/*
TIFF_FILE.C

port: the builds leave out source/bitmaps/tiff_file.c and the libtiff it
writes with (port/linux/port.json). Its one function, tiff_export, is called
only by the Xbox's movie recorder (main.c's screenshot_record, while
main_movie_start's movie runs), which nothing starts; the port's screenshots
are its own (d3d8_gl.c).
*/

#include "cseries/cseries.h"
#include "bitmaps/tiff_file.h"

char const *tiff_export(
	struct file_reference *file,
	struct bitmap_data *bitmap)
{
	(void)file;
	(void)bitmap;
	return "this build writes no TIFF files";
}
