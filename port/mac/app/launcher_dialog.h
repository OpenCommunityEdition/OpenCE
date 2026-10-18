/* PowerSpark's startup dialog (Carbon controls made in code, Mac OS X 10.4 and later,
   ppc and i386). Every option is shown on every Mac; only the defaults differ. */
#ifndef LAUNCHER_DIALOG_H
#define LAUNCHER_DIALOG_H

#include "launcher_config.h"

struct ld_env
{
	const char *app_path;		/* PowerSpark.app, for its icon */
	void (*run_controller_setup)(void);
	void (*run_disc_setup)(void);
	/* describes the game data found, for the dialog's status line */
	void (*data_status)(char *out, size_t size);
};

/* shows the dialog. 1: Play (the model holds the choices), 0: Quit, -1: it could not be shown */
int ld_run(struct lc_model *model, const struct ld_env *env);
/* the display's modes of 640x480 or more, widest first (one of each size); the count */
int ld_modes(int modes[][2], int max);

#endif
