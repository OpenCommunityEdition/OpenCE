/* PowerSpark's updater, the launcher's part: looks for a new version in the build's appcast (branding.plist's
   UpdateFeedURL), offers it, and downloads, checks, unpacks and swaps it in (update_core.h has the checks that
   need no window). Carbon controls made in code, as the other launcher windows; Mac OS X 10.4 to 10.6, ppc and
   i386. The HTTPS is PowerSpark-update's (update_fetch.c, mbedtls).

   Two ways in: at launch (LU_AT_LAUNCH: quiet, at most once a day, not when update.auto is false), and when
   the player asked (LU_ASKED: the in-game Settings' CHECK FOR UPDATES NOW quits the game and starts
   "PowerSpark --check-updates --wait-pid N"), which always says what it found, including why it could not look.
   Nothing of the player's (the settings folder) is touched: an update replaces PowerSpark.app only. */
#ifndef LAUNCHER_UPDATE_H
#define LAUNCHER_UPDATE_H

#include <stdio.h>
#include <sys/types.h>
#include "launcher_branding.h"

struct lu_env
{
	const char *app_path;		/* .../PowerSpark.app */
	const char *config_file;	/* config.toml: update.auto is read from it, and "Never Ask Again" writes it */
	const char *support;		/* the settings folder: the time of the last check is kept in it */
	const char *logs_dir;		/* update.log goes here */
	const struct lb_branding *branding;
	char version[32];		/* the running app's CFBundleShortVersionString */
	long build;			/* and CFBundleVersion */
	pid_t wait_pid;			/* a process (the game) to wait for before showing anything; 0: none */
	void (*alert)(const char *title, const char *message);
};

enum { LU_AT_LAUNCH = 0, LU_ASKED = 1 };

/* the build has a feed, a release key, a CA list and the downloader */
int lu_ready(const struct lu_env *env);
/* the launch should look: ready, update.auto is on (HALO_UPDATE_AUTO overrides), and a day has passed since
   the last look */
int lu_auto_wanted(const struct lu_env *env, long now);
/* Looks, and asks, and installs. 1: PowerSpark.app was replaced and will open again by itself, so the launcher
   must exit now. 0: go on (nothing newer, "Later", an error, or the player cancelled).
   HALO_UPDATE_ANSWER=yes|no|never answers the offer (as the game's debug.update_answer does), and
   HALO_MAC_UPDATE_HEADLESS=1 shows no window (for the tests). */
int lu_run(const struct lu_env *env, int mode);

/* prints each control's rectangle and its text's size for the update windows (with long sample texts); the
   number of items that do not fit */
int lu_layout_report(const struct lu_env *env, FILE *out);

/* the version of Mac OS X (10, 4 to 6) */
void lu_system_version(int *major, int *minor);

#endif
