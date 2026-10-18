/* PowerSpark.app's launcher: the program the Finder starts. It does what the kit's
   "Play OpenCE.command" and common.sh did, without Terminal: finds the game
   data, offers the first-run setup windows, sends the game's output to a log
   of its own in ~/Library/Logs/OpenCE and replaces itself with the game
   (PowerSpark-game, beside it), which keeps this process and its Dock icon.

   Mac OS X 10.4 and later, ppc and i386: C and Core Foundation only; Carbon
   for the startup dialog (launcher_dialog.c) and the Option key. Plain gnu89.

   Usage: PowerSpark [--dry-run] [--setup | --controller-setup] [--data-source
   auto|xbox|retail] [--dialog | --no-dialog | --dialog-dump |
   --dialog-layout] [--about | --feedback | --about-dump | --about-selftest] [game arguments]
     --dry-run          print what would be run (folders, data, settings set,
                        the game's command) and do nothing else
     --setup            open PowerSpark Setup (the Xbox disc image) and quit
     --controller-setup open the controller setup and quit
     --dialog           show the startup dialog even if it is turned off
     --no-dialog        do not show it (so does HALO_MAC_LAUNCHER_DIALOG=0, and
                        game arguments: they mean a scripted run)
     --dialog-dump      print what the dialog would show, and quit; no window
     --dialog-layout    print each control's rectangle and its text's width, and
                        quit with 1 if some text does not fit its control
     --about            show the About window and quit
     --feedback         show the Send Feedback window and quit (when the build has an endpoint)
     --about-dump       print the About and Send Feedback windows' layout like --dialog-layout
     --about-selftest   make both windows without showing them, and read their controls back
   The startup dialog (settings, Play, Quit, Setup, Controller Setup) opens
   before the game unless "Don't show this at startup" was checked (the file
   startup-dialog holds "off"); holding Option while the app opens shows it
   anyway. Environment: HALO_MAC_DATA_SOURCE (as --data-source),
   HALO_MAC_DATA_ROOT (the retail data, as the game reads it). Support files:
   see port/mac/README.md. */
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <mach-o/dyld.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "launcher_config.h"
#include "launcher_dialog.h"
#include "launcher_about.h"
#include "feedback_report.h"
#include "launcher_invite.h"

#define MAX_ENVIRONMENT 24
#define LOGS_KEPT 20

#if defined(__ppc__) || defined(__ppc64__)
#define ARCH_POWERPC 1
#else
#define ARCH_POWERPC 0
#endif

static int dry_run;
static char contents[PATH_MAX];  /* .../PowerSpark.app/Contents */
static char support[PATH_MAX];   /* ~/Library/Application Support/OpenCE */
static char beside[PATH_MAX];    /* the folder holding PowerSpark.app */
static char logs[PATH_MAX];      /* ~/Library/Logs/OpenCE */
static char config_file[PATH_MAX];
static char shown_environment[MAX_ENVIRONMENT][PATH_MAX + 64];
static int shown_environment_count;
/* what the dialog's buttons and status line need from main */
static const char *g_controller, *g_disc;
static char g_xbox_data[PATH_MAX];
static char g_retail[PATH_MAX];
static int g_have_retail;
static struct la_env g_about;
static struct lb_branding g_branding;

static int path_join(char *out, const char *a, const char *b)
{
	int n = snprintf(out, PATH_MAX, "%s/%s", a, b);

	return n > 0 && n < PATH_MAX;
}

static int is_dir(const char *path)
{
	struct stat st;

	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int is_file(const char *path)
{
	struct stat st;

	return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static void make_dirs(const char *path)
{
	char copy[PATH_MAX], *p;

	snprintf(copy, sizeof(copy), "%s", path);
	for (p = copy + 1; *p; ++p)
	{
		if (*p == '/')
		{
			*p = 0;
			mkdir(copy, 0755);
			*p = '/';
		}
	}
	mkdir(copy, 0755);
}

/* sets an environment variable for the game and records it for --dry-run */
static void set_game_environment(const char *name, const char *value)
{
	if (shown_environment_count < MAX_ENVIRONMENT)
		snprintf(shown_environment[shown_environment_count++], sizeof(shown_environment[0]), "%s=%s", name, value);
	setenv(name, value, 1);
}

/* ---------- halo:// invite links

Launch Services opens this bundle for a halo:// link (CFBundleURLTypes in
Info.plist) and sends the URL in an Apple event, GURL. The game reads an
invite from <data root>/join_link.txt — that is how a launcher hands one
to a copy already playing (port/linux/src/p2p.c, which renames the file
and reads it once a second) — so the event is turned into that file. The
game then joins whether this launch starts it or one is already running.

The event is queued before the application runs and arrives as soon as it
asks for one, so a short wait catches it. A start with no link waits that
long once and no longer; there is no way to know in advance that none is
coming. */

static char invite_link[1024];

static pascal OSErr invite_event(const AppleEvent *event, AppleEvent *reply, long context)
{
	DescType type;
	Size size = 0;
	OSErr error;

	(void)reply;
	(void)context;
	error = AEGetParamPtr(event, keyDirectObject, typeChar, &type,
		invite_link, (Size)(sizeof(invite_link) - 1), &size);
	if (error != noErr)
		return error;
	if (size < 0 || size > (Size)(sizeof(invite_link) - 1))
		size = 0;
	invite_link[size] = 0;
	return noErr;
}

/* waits a third of a second for a GURL event, and leaves invite_link empty
when none comes */
static void collect_invite_link(void)
{
	AEEventHandlerUPP handler;
	UInt32 deadline;

	handler = NewAEEventHandlerUPP(invite_event);
	if (!handler)
		return;
	if (AEInstallEventHandler(kInternetEventClass, kAEGetURL, handler, 0, false) != noErr)
	{
		DisposeAEEventHandlerUPP(handler);
		return;
	}
	deadline = TickCount() + 20;
	while (!invite_link[0] && TickCount() < deadline)
	{
		EventRecord record;

		if (WaitNextEvent(highLevelEventMask, &record, 1, NULL) &&
			record.what == kHighLevelEvent)
		{
			AEProcessAppleEvent(&record);
		}
	}
	AERemoveEventHandler(kInternetEventClass, kAEGetURL, handler, false);
	DisposeAEEventHandlerUPP(handler);
}

static void alert(const char *title, const char *message)
{
	CFStringRef t, m;
	CFOptionFlags response;

	fprintf(stderr, "PowerSpark: %s: %s\n", title, message);
	if (dry_run)
		return;
	t = CFStringCreateWithCString(NULL, title, kCFStringEncodingUTF8);
	m = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);
	CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, t, m, NULL, NULL, NULL,
		&response);
	CFRelease(t);
	CFRelease(m);
}

/* 0: first button, 1: second, 2: third */
static int choose(const char *title, const char *message, const char *b0, const char *b1, const char *b2)
{
	CFStringRef t = CFStringCreateWithCString(NULL, title, kCFStringEncodingUTF8);
	CFStringRef m = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);
	CFStringRef s0 = CFStringCreateWithCString(NULL, b0, kCFStringEncodingUTF8);
	CFStringRef s1 = CFStringCreateWithCString(NULL, b1, kCFStringEncodingUTF8);
	CFStringRef s2 = CFStringCreateWithCString(NULL, b2, kCFStringEncodingUTF8);
	CFOptionFlags response = 0;

	CFUserNotificationDisplayAlert(0, kCFUserNotificationNoteAlertLevel, NULL, NULL, NULL, t, m, s0, s1, s2,
		&response);
	CFRelease(t);
	CFRelease(m);
	CFRelease(s0);
	CFRelease(s1);
	CFRelease(s2);
	return response == kCFUserNotificationDefaultResponse ? 0 :
		response == kCFUserNotificationAlternateResponse ? 1 : 2;
}

/* runs a program and waits; its exit status, or -1 */
static int run_and_wait(char *const argv[])
{
	int status;
	pid_t pid = fork();

	if (pid < 0)
		return -1;
	if (pid == 0)
	{
		execv(argv[0], argv);
		_exit(127);
	}
	while (waitpid(pid, &status, 0) < 0)
		if (errno != EINTR)
			return -1;
	return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* the text of a small file (at most 256 KB), NUL terminated; free() it */
static char *read_text(const char *path)
{
	FILE *f = fopen(path, "rb");
	char *text;
	size_t n;

	if (!f)
		return NULL;
	text = (char *)malloc(262145);
	if (!text)
	{
		fclose(f);
		return NULL;
	}
	n = fread(text, 1, 262144, f);
	fclose(f);
	text[n] = 0;
	return text;
}

/* writes text to path through path.tmp; 0 or -1 */
static int write_text_file(const char *path, const char *text, size_t length)
{
	char tmp[PATH_MAX];
	FILE *f;
	int ok;

	snprintf(tmp, sizeof(tmp), "%s.tmp", path);
	f = fopen(tmp, "wb");
	if (!f)
		return -1;
	ok = fwrite(text, 1, length, f) == length;
	if (fclose(f) != 0)
		ok = 0;
	if (ok && rename(tmp, path) != 0)
		ok = 0;
	if (!ok)
		remove(tmp);
	return ok ? 0 : -1;
}

/* An older Controller Setup wrote "input.gamepad_*" lines at the top of config.toml, above the
   file's own [input] table; the game refuses such a file (table defined before) and starts with
   its defaults. The lines are moved into the table, once, keeping the file as it was beside it
   (config.toml.bak-before-repair). Returns the text to use (text, or its replacement). */
static char *repair_config(const char *path, char *text)
{
	size_t length;
	char backup[PATH_MAX];
	char *fixed;
	int moved;

	if (!text || dry_run)
		return text;
	length = strlen(text);
	fixed = (char *)malloc(length + 1);
	if (!fixed)
		return text;
	memcpy(fixed, text, length + 1);
	moved = lc_text_repair(&fixed, &length);
	if (moved > 0)
	{
		snprintf(backup, sizeof(backup), "%s.bak-before-repair", path);
		if (!is_file(backup))
			write_text_file(backup, text, strlen(text));
		if (write_text_file(path, fixed, length) == 0)
		{
			fprintf(stderr, "PowerSpark: moved %d settings of %s into their [input] table\n", moved, path);
			free(text);
			return fixed;
		}
	}
	free(fixed);
	return text;
}

/* Whether the game will accept config.toml. When it will not, it ignores the whole file and
   starts with its defaults (the player's choices seem lost), so say so, with the line.
   The message goes in message (a line for --dry-run); returns 1 if the file is valid or missing. */
static int config_is_valid(const char *text, char *message, size_t size)
{
	message[0] = 0;
	return !text || lc_text_check(text, strlen(text), message, size);
}

/* the value of the first line "name = ..." (at the start of a line, with or
   without a section's prefix), its quotes taken off; 1 if found */
static int line_value(const char *text, const char *name, char *out, size_t size)
{
	size_t length = strlen(name);
	const char *p = text;

	while (*p)
	{
		const char *line = p, *end = strchr(p, '\n');
		const char *v;

		p = end ? end + 1 : p + strlen(p);
		if (strncmp(line, name, length) != 0)
			continue;
		v = line + length;
		while (*v == ' ' || *v == '\t')
			++v;
		if (*v != '=')
			continue;
		++v;
		while (*v == ' ' || *v == '\t')
			++v;
		if (*v == '"')
		{
			size_t n = 0;

			++v;
			while (*v && *v != '"' && *v != '\n' && n + 1 < size)
				out[n++] = *v++;
			out[n] = 0;
		}
		else
		{
			size_t n = 0;

			while (*v && *v != '\n' && *v != '\r' && *v != ' ' && *v != '#' && n + 1 < size)
				out[n++] = *v++;
			out[n] = 0;
		}
		return 1;
	}
	return 0;
}

static int has_maps(const char *root)
{
	char p[PATH_MAX];

	return path_join(p, root, "Maps") && is_dir(p);
}

/* removes all but the newest LOGS_KEPT-1 logs (the names sort by date) */
static void prune_logs(void)
{
	DIR *d = opendir(logs);
	struct dirent *e;
	char (*names)[64];
	int count = 0, i, j, capacity = 512;

	if (!d)
		return;
	names = (char (*)[64])malloc(sizeof(char[64]) * capacity);
	if (!names)
	{
		closedir(d);
		return;
	}
	while ((e = readdir(d)) != NULL && count < capacity)
		if (strncmp(e->d_name, "OpenCE-", 7) == 0 && strlen(e->d_name) < 63)
			strcpy(names[count++], e->d_name);
	closedir(d);
	for (i = 1; i < count; ++i)
		for (j = i; j > 0 && strcmp(names[j - 1], names[j]) > 0; --j)
		{
			char tmp[64];

			strcpy(tmp, names[j]);
			strcpy(names[j], names[j - 1]);
			strcpy(names[j - 1], tmp);
		}
	for (i = 0; i < count - (LOGS_KEPT - 1); ++i)
	{
		char p[PATH_MAX];

		if (path_join(p, logs, names[i]))
			unlink(p);
	}
	free(names);
}

/* does the engine's marker file in Resources list a feature? (the build
   script writes engine.txt after looking at the game program) */
static int engine_has(const char *feature)
{
	char p[PATH_MAX], *text;
	int found;

	if (!path_join(p, contents, "Resources/engine.txt"))
		return 0;
	text = read_text(p);
	if (!text)
		return 0;
	found = strstr(text, feature) != NULL;
	free(text);
	return found;
}

/* the Xbox disc's maps: in the support folder, else the XboxData folder beside the app (as the test kit keeps
   it). xbox_data and xbox_maps name where they are, or where PowerSpark Setup will put them */
static int detect_xbox(char *xbox_data, char *xbox_maps)
{
	char ui[PATH_MAX], beside_data[PATH_MAX], beside_maps[PATH_MAX];

	path_join(xbox_data, support, "XboxData");
	path_join(xbox_maps, xbox_data, "maps");
	path_join(ui, xbox_maps, "ui.map");
	if (is_file(ui))
		return 1;
	path_join(beside_data, beside, "XboxData");
	path_join(beside_maps, beside_data, "maps");
	path_join(ui, beside_maps, "ui.map");
	if (is_file(ui))
	{
		snprintf(xbox_data, PATH_MAX, "%s", beside_data);
		snprintf(xbox_maps, PATH_MAX, "%s", beside_maps);
		return 1;
	}
	return 0;
}

static void dialog_controller_setup(void)
{
	char *av[] = { (char *)g_controller, "--config", config_file, NULL };

	if (is_file(g_controller))
		run_and_wait(av);
}

static void dialog_disc_setup(void)
{
	char *av[] = { (char *)g_disc, "--data", g_xbox_data, NULL };

	if (is_file(g_disc))
		run_and_wait(av);
}

static void dialog_data_status(char *out, size_t size)
{
	char data[PATH_MAX], maps[PATH_MAX];
	int xbox = detect_xbox(data, maps);

	if (xbox && g_have_retail)
		snprintf(out, size, "Found: Xbox disc data and MacSoft's Halo.");
	else if (xbox)
		snprintf(out, size, "Found: Xbox disc data.");
	else if (g_have_retail)
		snprintf(out, size, "Found: MacSoft's Halo. To play from your Xbox disc, click Set Up from Disc Image.");
	else
		snprintf(out, size, "No game data yet. Click Set Up from Disc Image and choose your Halo disc image.");
}

static void dialog_about(void)
{
	la_show_about(&g_about);
}

/* starts the sender on the Outbox (reports kept from an earlier try) without waiting for it: the first process
   is waited for, its child (started by a second fork) is left to launchd, so that no zombie follows the game */
static void flush_outbox(void)
{
	char dir[PATH_MAX], version[32] = "?";
	DIR *d;
	struct dirent *e;
	int any = 0;
	pid_t pid;

	if (!lb_feedback_ready(&g_branding) || fb_outbox_dir(g_about.home, dir, sizeof(dir), 0) != 0)
		return;
	d = opendir(dir);
	if (!d)
		return;
	while ((e = readdir(d)) != NULL)
	{
		size_t n = strlen(e->d_name);

		if (e->d_name[0] != '.' && n > 5 && strcmp(e->d_name + n - 5, ".json") == 0)
			any = 1;
	}
	closedir(d);
	if (!any)
		return;
	{
		CFBundleRef bundle = CFBundleGetMainBundle();
		CFTypeRef v = bundle ? CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundleShortVersionString")) : NULL;

		if (v && CFGetTypeID(v) == CFStringGetTypeID())
			CFStringGetCString((CFStringRef)v, version, sizeof(version), kCFStringEncodingUTF8);
	}
	pid = fork();
	if (pid == 0)
	{
		if (fork() == 0)
		{
			int null = open("/dev/null", O_RDWR);

			setsid();
			if (null >= 0)
			{
				dup2(null, 0);
				dup2(null, 1);
				dup2(null, 2);
			}
			execl(g_branding.sender_path, g_branding.sender_path, "--url", g_branding.feedback_url, "--ca",
				g_branding.ca_path, "--token", g_branding.feedback_token, "--app-version", version, dir, (char *)NULL);
		}
		_exit(0);
	}
	if (pid > 0)
		while (waitpid(pid, NULL, 0) < 0 && errno == EINTR)
			;
}

/* "off" in a one-word file of the support folder: the startup dialog is not shown */
static int read_dialog_hidden(void)
{
	char p[PATH_MAX], word[16], *t;
	int hidden = 0;

	path_join(p, support, "startup-dialog");
	t = read_text(p);
	if (t)
	{
		hidden = sscanf(t, "%15s", word) == 1 && strcmp(word, "off") == 0;
		free(t);
	}
	return hidden;
}

static void write_word_file(const char *name, const char *word)
{
	char p[PATH_MAX];
	FILE *f;

	path_join(p, support, name);
	f = fopen(p, "wb");
	if (f)
	{
		fprintf(f, "%s\n", word);
		fclose(f);
	}
}

int main(int argc, char **argv)
{
	char exe[PATH_MAX], real[PATH_MAX], macos[PATH_MAX], resources[PATH_MAX], game[PATH_MAX];
	char menus[PATH_MAX], controller[PATH_MAX], disc[PATH_MAX];
	char retail[PATH_MAX], xbox_data[PATH_MAX], xbox_maps[PATH_MAX], ui[PATH_MAX], bitmaps[PATH_MAX];
	char sounds[PATH_MAX], source_name[32], data_root[PATH_MAX], text[PATH_MAX], log_path[PATH_MAX];
	char *config_text, *args[64];
	const char *home = getenv("HOME"), *request, *env_data;
	uint32_t size = sizeof(exe);
	int i, nargs = 0, want_setup = 0, want_controller = 0, have_retail = 0, have_xbox = 0, use_xbox = 0;
	int config_dir_support;
	int force_dialog = 0, no_dialog = 0, dialog_dump = 0, dialog_layout = 0;
	int about_dump = 0, about_show = 0, feedback_show = 0, about_selftest = 0;
	struct lc_model dialog_model;
	const char *dialog_env;
	time_t now;
	struct tm *tm;
	char *slash;

	request = getenv("HALO_MAC_DATA_SOURCE");
	snprintf(source_name, sizeof(source_name), "%s", request && *request ? request : "");
	/* (before the data root is looked for, because the event is waiting
	now; it is written out below, once there is a root to write it in) */
	collect_invite_link();
	for (i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "--dry-run") == 0)
			dry_run = 1;
		else if (strcmp(argv[i], "--setup") == 0)
			want_setup = 1;
		else if (strcmp(argv[i], "--controller-setup") == 0)
			want_controller = 1;
		else if (strcmp(argv[i], "--dialog") == 0)
			force_dialog = 1;
		else if (strcmp(argv[i], "--no-dialog") == 0)
			no_dialog = 1;
		else if (strcmp(argv[i], "--dialog-layout") == 0)
			dialog_layout = dry_run = 1;
		else if (strcmp(argv[i], "--about-dump") == 0)
			about_dump = dry_run = 1;
		else if (strcmp(argv[i], "--about-selftest") == 0)
			about_selftest = dry_run = 1;
		else if (strcmp(argv[i], "--about") == 0)
			about_show = 1;
		else if (strcmp(argv[i], "--feedback") == 0)
			feedback_show = 1;
		else if (strcmp(argv[i], "--dialog-dump") == 0)
			dialog_dump = dry_run = 1;	/* (it writes and runs nothing) */
		else if (strcmp(argv[i], "--data-source") == 0 && i + 1 < argc)
			snprintf(source_name, sizeof(source_name), "%s", argv[++i]);
		else if (strncmp(argv[i], "-psn_", 5) == 0)
			;
		else if (nargs < 40)
			args[nargs++] = argv[i];
	}

	/* the bundle */
	if (_NSGetExecutablePath(exe, &size) != 0 || !realpath(exe, real))
		return 70;
	slash = strrchr(real, '/');
	if (!slash)
		return 70;
	*slash = 0;
	snprintf(macos, sizeof(macos), "%s", real);
	slash = strrchr(real, '/');
	if (!slash)
		return 70;
	*slash = 0;
	snprintf(contents, sizeof(contents), "%s", real);
	path_join(resources, contents, "Resources");
	path_join(game, macos, "PowerSpark-game");
	path_join(menus, resources, "menus");
	path_join(controller, resources, "Helpers/OpenCE Controller Setup.app/Contents/MacOS/OpenCE Controller Setup");
	path_join(disc, resources, "Helpers/OpenCE Setup.app/Contents/MacOS/OpenCE Setup");

	/* the player's folders. The app is PowerSpark, but these keep the name
	   OpenCE: the settings, saves and map cache of every earlier version live
	   there, and the logs' names (OpenCE-*.log) are what prune_logs counts */
	if (!home || !*home)
		home = "/tmp";
	snprintf(support, sizeof(support), "%s/Library/Application Support/OpenCE", home);
	snprintf(logs, sizeof(logs), "%s/Library/Logs/OpenCE", home);
	path_join(config_file, support, "config.toml");
	path_join(xbox_data, support, "XboxData");
	/* the folder holding PowerSpark.app (contents is .../PowerSpark.app/Contents) */
	snprintf(beside, sizeof(beside), "%s", contents);
	{
		char *slash = strrchr(beside, '/');
		if (slash) *slash = 0;
		slash = strrchr(beside, '/');
		if (slash) *slash = 0;
	}
	path_join(xbox_maps, xbox_data, "maps");
	if (!dry_run)
	{
		make_dirs(support);
		make_dirs(logs);
	}

	/* the settings file: the engine reads config.toml from the folder it
	   names (HALO_MAC_CONFIG_DIR). An engine without that is copied into
	   the support folder, where its config.toml is a link to the real one */
	config_dir_support = engine_has("config_dir");
	if (!config_dir_support)
	{
		char staged_dir[PATH_MAX], staged[PATH_MAX], link[PATH_MAX];
		struct stat a, b;

		path_join(staged_dir, support, "engine");
		path_join(staged, staged_dir, "PowerSpark-game");
		path_join(link, staged_dir, "config.toml");
		if (!dry_run)
		{
			make_dirs(staged_dir);
			if (stat(game, &a) == 0 && (stat(staged, &b) != 0 || a.st_size != b.st_size || a.st_mtime > b.st_mtime))
			{
				char *cp[] = { "/bin/cp", "-p", game, staged, NULL };

				if (run_and_wait(cp) != 0)
				{
					alert("PowerSpark cannot start", "The game program could not be copied into Application Support.");
					return 1;
				}
			}
			symlink("../config.toml", link);
		}
		snprintf(game, sizeof(game), "%s", staged);
	}

	/* the data: the retail game's (HALO_MAC_DATA_ROOT, then paths.data, then
	   a GameData folder in the support folder, then Halo.app) and the maps
	   OpenCE Setup copied out of the player's Xbox disc */
	config_text = repair_config(config_file, read_text(config_file));
	retail[0] = 0;
	env_data = getenv("HALO_MAC_DATA_ROOT");
	if (env_data && *env_data)
		snprintf(retail, sizeof(retail), "%s", env_data);
	else if (config_text && line_value(config_text, "data", text, sizeof(text)) && *text)
		snprintf(retail, sizeof(retail), "%s", text);
	else
	{
		char candidate[PATH_MAX];

		if (path_join(candidate, support, "GameData") && has_maps(candidate))
			snprintf(retail, sizeof(retail), "%s", candidate);
		else if (path_join(candidate, beside, "GameData") && has_maps(candidate))
			snprintf(retail, sizeof(retail), "%s", candidate);
		else if (has_maps("/Applications/Halo.app/Contents/Resources/GameData"))
			snprintf(retail, sizeof(retail), "%s", "/Applications/Halo.app/Contents/Resources/GameData");
		else
		{
			snprintf(candidate, sizeof(candidate), "%s/Applications/Halo.app/Contents/Resources/GameData", home);
			if (has_maps(candidate))
				snprintf(retail, sizeof(retail), "%s", candidate);
		}
	}
	have_retail = retail[0] && has_maps(retail);
	have_xbox = detect_xbox(xbox_data, xbox_maps);
	path_join(ui, xbox_maps, "ui.map");
	g_have_retail = have_retail;
	snprintf(g_retail, sizeof(g_retail), "%s", retail);

	/* the data source: a switch, so that Xbox maps can become the default
	   without a new launcher. HALO_MAC_DATA_SOURCE, --data-source, or the
	   first word of the support folder's data-source file */
	if (!*source_name)
	{
		char p[PATH_MAX], *t;

		path_join(p, support, "data-source");
		t = read_text(p);
		if (t)
		{
			sscanf(t, "%31s", source_name);
			free(t);
		}
	}
	if (strcmp(source_name, "xbox") != 0 && strcmp(source_name, "retail") != 0)
		snprintf(source_name, sizeof(source_name), "auto");

	g_controller = controller;
	g_disc = disc;
	snprintf(g_xbox_data, sizeof(g_xbox_data), "%s", xbox_data);

	/* the startup dialog: the settings, Play and Quit, Setup and Controller Setup. It opens unless it was turned
	   off (or the run is scripted: game arguments, --no-dialog, HALO_MAC_LAUNCHER_DIALOG=0); Option shows it
	   anyway. It writes config.toml with the keys the game's Settings menus use */
	dialog_env = getenv("HALO_MAC_LAUNCHER_DIALOG");
	lc_model_load(&dialog_model, config_text);
	snprintf(dialog_model.data_source, sizeof(dialog_model.data_source), "%s", source_name);
	snprintf(dialog_model.data_source_original, sizeof(dialog_model.data_source_original), "%s", source_name);
	dialog_model.hide = dialog_model.hide_original = read_dialog_hidden();
	/* the About and Send Feedback windows: what the build adds (branding.plist), and the feedback sender */
	lb_load(&g_branding, resources);
	path_join(g_branding.sender_path, macos, "PowerSpark-send");
	g_about.home = home;
	g_about.logs_dir = logs;
	g_about.branding = &g_branding;
	{
		static char app_path[PATH_MAX];

		snprintf(app_path, sizeof(app_path), "%s", contents);
		if (strlen(app_path) > 9)
			app_path[strlen(app_path) - 9] = 0;	/* .../PowerSpark.app/Contents */
		g_about.app_path = app_path;
	}
	if (about_dump)
		return la_layout_report(&g_about, stdout) != 0;
	if (about_selftest)
		return la_selftest(&g_about, stdout) != 0;
	if (about_show)
	{
		la_show_about(&g_about);
		return 0;
	}
	if (feedback_show)
	{
		la_show_feedback(&g_about);
		return 0;
	}
	if (dialog_layout)
	{
		/* (no window: each control's rectangle and its text's width in the system font; 1 if some text does not fit) */
		return ld_layout_report(&dialog_model, stdout) != 0;
	}
	if (dialog_dump)
	{
		int modes[64][2], n = ld_modes(modes, 64);

		printf("startup dialog (would be shown: %s)\n", read_dialog_hidden() ? "no, turned off; Option shows it" : "yes");
		lc_model_dump(&dialog_model, stdout);
		printf("  display modes (%d):", n);
		for (i = 0; i < n; ++i)
			printf(" %dx%d", modes[i][0], modes[i][1]);
		printf("\n");
		{
			char status[256];

			dialog_data_status(status, sizeof(status));
			printf("  data status: %s\n", status);
		}
		return 0;
	}
	if (!dry_run && !want_setup && !want_controller && !no_dialog && !(dialog_env && strcmp(dialog_env, "0") == 0) &&
		(force_dialog || (nargs == 0 && (!dialog_model.hide || (GetCurrentKeyModifiers() & optionKey)))))
	{
		struct ld_env env;
		char app[PATH_MAX];
		int shown;

		snprintf(app, sizeof(app), "%s", contents);
		if (strlen(app) > 9)
			app[strlen(app) - 9] = 0;	/* .../PowerSpark.app/Contents */
		env.app_path = app;
		env.run_controller_setup = dialog_controller_setup;
		env.run_disc_setup = dialog_disc_setup;
		env.data_status = dialog_data_status;
		env.show_about = dialog_about;
		shown = ld_run(&dialog_model, &env);
		if (shown == 0)
			return 0;
		if (shown < 0)
		{
			/* (no window: the old chooser) */
			int pick = choose("PowerSpark", "What would you like to open?", "Play", "Controller Setup",
				"Disc Image Setup");

			if (pick == 1)
				want_controller = 1;
			else if (pick == 2)
				want_setup = 1;
		}
		else
		{
			if (lc_model_save(&dialog_model, config_file) != 0)
				fprintf(stderr, "PowerSpark: cannot write %s\n", config_file);
			if (strcmp(dialog_model.data_source, dialog_model.data_source_original) != 0)
			{
				write_word_file("data-source", dialog_model.data_source);
				snprintf(source_name, sizeof(source_name), "%s", dialog_model.data_source);
			}
			if (dialog_model.hide != dialog_model.hide_original)
				write_word_file("startup-dialog", dialog_model.hide ? "off" : "on");
			/* (Disc Image Setup may have added the maps; Controller Setup may have written config.toml) */
			have_xbox = detect_xbox(xbox_data, xbox_maps);
			path_join(ui, xbox_maps, "ui.map");
			free(config_text);
			config_text = read_text(config_file);
		}
	}

	if (want_controller)
	{
		char *av[] = { controller, "--config", config_file, NULL };

		if (dry_run)
		{
			printf("would run: %s --config %s\n", controller, config_file);
			return 0;
		}
		return run_and_wait(av) == 0 ? 0 : 1;
	}
	if (want_setup)
	{
		char *av[] = { disc, "--data", xbox_data, NULL };

		if (dry_run)
		{
			printf("would run: %s --data %s\n", disc, xbox_data);
			return 0;
		}
		return run_and_wait(av) == 0 ? 0 : 1;
	}

	/* no data at all (no Xbox maps and no retail Halo): the disc setup window, on both kinds of Mac */
	if (!have_retail && !have_xbox && strcmp(source_name, "retail") != 0)
	{
		if (dry_run)
			printf("no game data: would run PowerSpark Setup (%s --data %s)\n", disc, xbox_data);
		else if (is_file(disc))
		{
			char *av[] = { disc, "--data", xbox_data, NULL };

			run_and_wait(av);
			have_xbox = is_file(ui);
		}
	}

	if (strcmp(source_name, "xbox") == 0)
		use_xbox = have_xbox;
	else if (strcmp(source_name, "retail") == 0)
		use_xbox = 0;
	else
		use_xbox = have_xbox;	/* auto: the Xbox disc's maps whenever they are there, on every Mac */
	if (!use_xbox && !have_retail)
	{
		if (dry_run)
			printf("no usable game data (data source %s)\n", source_name);
		else if (strcmp(source_name, "retail") == 0)
			alert("PowerSpark needs Halo's maps",
				"The data source is set to the installed Halo (the data-source file or HALO_MAC_DATA_SOURCE), "
				"but no Halo is installed. Delete the data-source file in ~/Library/Application Support/OpenCE "
				"to play from your Xbox disc, or install Halo in the Applications folder.");
		else
			alert("PowerSpark needs Halo's maps",
				"Open PowerSpark again and choose your Halo: Combat Evolved Xbox disc image when asked "
				"(or hold Option while it opens and choose Disc Image Setup).");
		if (!dry_run)
			return 1;
	}

	/* the game's command and settings */
	set_game_environment("HALO_MAC_CONFIG_DIR", support);
	if (!config_dir_support)
		shown_environment_count--, unsetenv("HALO_MAC_CONFIG_DIR");
	set_game_environment("HALO_MAC_MENU_ART_DIR", menus);
	set_game_environment("HALO_START_IN_MENU", "1");
	/* the Xbox disc's movies (bink/ and default.xbe of the XboxData folder), when it has them:
	   the game plays them in place of MacSoft's logos and ending */
	if (have_xbox)
	{
		path_join(text, xbox_data, "bink/intro.bik");
		if (is_file(text))
			set_game_environment("HALO_MAC_XBOX_DATA", xbox_data);
	}
	if (use_xbox)
	{
		/* the retail folder, when there is one, for its Movies; the maps
		   themselves are the Xbox disc's */
		snprintf(data_root, sizeof(data_root), "%s", have_retail && strcmp(source_name, "xbox") != 0 ? retail : xbox_data);
		set_game_environment("HALO_MAC_DATA_ROOT", data_root);
		/* the inflated maps (each disc map is unpacked once) and the saves are the player's,
		   in Application Support; nothing is written to the data folder or the app */
		snprintf(text, sizeof(text), "%s/map-cache", support);
		set_game_environment("HALO_MAC_MAP_CACHE", text);
		snprintf(text, sizeof(text), "%s/saves", support);
		if (!dry_run)
			make_dirs(text);
		set_game_environment("HALO_MAC_SAVE_ROOT", text);
	}
	else
	{
		snprintf(data_root, sizeof(data_root), "%s", retail);
		set_game_environment("HALO_MAC_DATA_ROOT", data_root);
		/* the title screen (ui.map) is the menus' background; Blood Gulch
		   only if it is missing */
		snprintf(text, sizeof(text), "%s/Maps/ui.map", data_root);
		snprintf(ui, sizeof(ui), "%s", is_file(text) ? text : "");
		if (!*ui)
			snprintf(ui, sizeof(ui), "%s/Maps/bloodgulch.map", data_root);
		snprintf(bitmaps, sizeof(bitmaps), "%s/Maps/bitmaps.map", data_root);
		snprintf(sounds, sizeof(sounds), "%s/Maps/sounds.map", data_root);
	}

	/* an invite from a halo:// link, for the game this is about to start or
	for one already playing (launcher_invite.c) */
	if (invite_link[0] && !launcher_invite_write(data_root, invite_link))
		fprintf(stderr, "PowerSpark: the invite link could not be handed to the game\n");

	{
		char *cmd[64];
		int n = 0, k;

		cmd[n++] = game;
		cmd[n++] = ui;
		if (!use_xbox)
		{
			cmd[n++] = bitmaps;
			cmd[n++] = sounds;
		}
		cmd[n++] = "--gpu-play";
		for (k = 0; k < nargs && n < 62; ++k)
			cmd[n++] = args[k];
		cmd[n] = NULL;

		now = time(NULL);
		tm = localtime(&now);
		snprintf(log_path, sizeof(log_path), "%s/OpenCE-%04d%02d%02d-%02d%02d%02d.log", logs, tm->tm_year + 1900,
			tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

		if (dry_run)
		{
			printf("bundle:        %s\n", contents);
			printf("support:       %s\nconfig:        %s%s\n", support, config_file,
				config_text ? "" : " (the game writes it on its first run)");
			printf("log:           %s\n", log_path);
			config_is_valid(config_text, text, sizeof(text));
			printf("config check:  %s\n", text[0] ? text : "valid");
			printf("architecture:  %s\n", ARCH_POWERPC ? "PowerPC" : "Intel");
			printf("data source:   %s -> %s (retail data: %s, Xbox maps: %s)\n", source_name,
				use_xbox ? "xbox" : "retail", have_retail ? retail : "none", have_xbox ? xbox_maps : "none");
			printf("controller setup on first run: %s\n",
				config_text && (line_value(config_text, "gamepad_setup_done", text, sizeof(text)) ||
					line_value(config_text, "input.gamepad_setup_done", text, sizeof(text))) &&
					strcmp(text, "true") == 0 ? "done" : "yes");
			printf("feedback:      %s\n", lb_feedback_ready(&g_branding) ? g_branding.feedback_url : "off");
			printf("config via:    %s\n", config_dir_support ? "HALO_MAC_CONFIG_DIR" : "staged copy of the game");
			for (k = 0; k < shown_environment_count; ++k)
				printf("env:           %s\n", shown_environment[k]);
			printf("command:       ");
			for (k = 0; cmd[k]; ++k)
				printf("%s'%s'", k ? " " : "", cmd[k]);
			printf("\n");
			return 0;
		}

		/* the controller setup, the first time (it records in config.toml that it was offered) */
		if (is_file(controller) && !getenv("HALO_SKIP_CONTROLLER_SETUP"))
		{
			int done = config_text &&
				(line_value(config_text, "gamepad_setup_done", text, sizeof(text)) ||
				 line_value(config_text, "input.gamepad_setup_done", text, sizeof(text))) &&
				strcmp(text, "true") == 0;

			if (!done)
			{
				char *av[] = { controller, "--config", config_file, NULL };

				run_and_wait(av);
			}
		}
		free(config_text);
		{
			/* (read again: the dialog and the controller setup may have written it) */
			char *latest = read_text(config_file);

			if (!config_is_valid(latest, text, sizeof(text)))
			{
				char message[400];

				snprintf(message, sizeof(message),
					"The game cannot read its settings file, %s: %s. It will start with its default settings "
					"(so your display and other choices are not used) until the file is fixed or deleted.",
					config_file, text);
				alert("PowerSpark's settings file has an error", message);
			}
			free(latest);
		}

		flush_outbox();
		prune_logs();
		{
			int fd = open(log_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

			if (fd >= 0)
			{
				dup2(fd, 1);
				dup2(fd, 2);
				close(fd);
			}
		}
		printf("PowerSpark %s, %s, data source %s\n", ARCH_POWERPC ? "PowerPC" : "Intel", contents,
			use_xbox ? "xbox" : "retail");
		for (k = 0; cmd[k]; ++k)
			printf("%s'%s'", k ? " " : "", cmd[k]);
		printf("\n");
		fflush(stdout);
		chdir(support);
		execv(game, cmd);
		fprintf(stderr, "PowerSpark: cannot start %s: %s\n", game, strerror(errno));
		return 71;
	}
}
