/* PowerSpark's About and Send Feedback windows (Carbon controls made in code, Mac OS X 10.4 and later, ppc and
   i386). What the build adds of its own (a maker's paragraph, links, a logo, the feedback endpoint) comes from
   launcher_branding.h; without it the About window has the OpenCE link only and there is no feedback. */
#ifndef LAUNCHER_ABOUT_H
#define LAUNCHER_ABOUT_H

#include <stdio.h>
#include "launcher_branding.h"

struct la_env
{
	const char *app_path;		/* PowerSpark.app, for its icon */
	const char *home;
	const char *logs_dir;		/* ~/Library/Logs/OpenCE */
	const struct lb_branding *branding;
};

/* the windows, each modal; they return when closed */
void la_show_about(const struct la_env *env);
void la_show_feedback(const struct la_env *env);

/* prints every control's rectangle and its text's size in the system font (no window); the number of items
   that do not fit, as ld_layout_report does for the start window */
int la_layout_report(const struct la_env *env, FILE *out);

/* makes both windows without showing them, reads the controls back and removes them; the number of failures
   (needs a window server; for the checks) */
int la_selftest(const struct la_env *env, FILE *out);

#endif
