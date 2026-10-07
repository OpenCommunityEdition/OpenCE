/*
SCREENSHOT.H

Frames saved as BMP files: every Nth frame (debug.screenshot_every), and one
frame asked for by name (the telnet console's port_screenshot,
telnet_console.c), which tools/render_test.py takes its pictures with. The
renderer (xgpu_device.h) reads its back buffer; the rest is here, so that every
renderer saves the same files.
*/

#ifndef SCREENSHOT_H
#define SCREENSHOT_H

/* writes rows of 32-bit BGRA pixels, the first row the top of the picture,
as a BMP file; alpha is written as 255 (the game keeps values of its own in
destination alpha, which image viewers would show as transparency). 1 on
success */
int screenshot_write_bmp(const char *path, unsigned char *pixels, unsigned long width, unsigned long height);

/* each frame's real time as the game counts it (main.c), from
debug.frame_time: more than 0, those seconds; 0, the time the frame took;
less than 0, held still (none) until the first screenshot is asked for, then
as many seconds as it says: the effects that move with real time are where
the settle frames move them, whatever renderer draws them, however long its
first frames take. Returned: those seconds, 0 for the frame's own, or less
than 0 while held */
double port_frame_time(void);

/* asks for the frame presented after settle_frames more frames, saved as
<debug.screenshot_directory>/<name>.bmp. 0, with why in message, when it
cannot be: a name that is not letters, digits, '-', '_' and '.', no
debug.screenshot_directory, or another still to be saved */
int screenshot_request(const char *name, long settle_frames, char *message, unsigned long size);
/* for the renderer, at each present: the path to save this frame to, or NULL */
const char *screenshot_due(void);
/* the renderer saved the frame screenshot_due asked for (or could not) */
void screenshot_saved(int saved);
/* once for each request, when it is done: "captured <path>" or "capture
failed <path>"; 0 when there is none */
int screenshot_result(char *message, unsigned long size);

#endif
