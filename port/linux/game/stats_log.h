/*
STATS_LOG.H

display.stats_log: a line a second in the log with the frame rate, the mode,
the map, the renderer and the size of the 3D view
(port/linux/game/stats_log.c).
*/

#ifndef STATS_LOG_H
#define STATS_LOG_H

/* called once a frame; writes its line when the setting is on and a second
has passed */
void stats_log_tick(void);

/* the renderer's own text, once the GL context exists (d3d8_gl.c's
gl_initialize) */
void stats_log_set_renderer(
	const char *vendor,
	const char *renderer);

#endif
