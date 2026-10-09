/*
HOST_MAIN.C

Entry point of the Android port (SDL_main, called by SDLActivity on its
own thread).

It loads the guest image (the game, built as ILP32 code) from the APK's
assets, gives it an environment describing where the game data and saves
live, and runs its main() on a thread of its own with its stack in guest
memory (host_thread.c), on which everything here after startup runs; the
SDL thread waits for it.

Storage (see port/android/README.md): the game data (the directory holding
maps/) is the app's external files directory,
/sdcard/Android/data/<package>/files, where the launcher activity copies it
on first run; saves go to its save/ subdirectory. The settings,
config.toml, live there too (port/linux/src/port_config.c, which the game
reads); this file reads only debug.sample_seconds from it, for the sampler
that runs here.
*/

#include "host.h"
#include "tomlc17.h"
#include "game_data_profile.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <android/log.h>
#include <jni.h>
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

void host_install_signal_handlers(void);

/* ---------- the log file

The launcher creates a timestamped MediaStore Downloads log before native
startup, then passes an append descriptor through JNI. This works with
Android scoped storage; direct fopen into Download is only a legacy fallback.
The app-private halo_log.txt remains a second native log. */

#define LOG_DIRECTORY "/sdcard/Download/HaloCE"
#define LOGS_KEPT 10

/* the log in Download (a new file each run) and its copy in the app's own
storage (which an app can always write) */
static FILE *log_files[2];
static pthread_mutex_t log_file_lock = PTHREAD_MUTEX_INITIALIZER;

static int compare_names(const void *a, const void *b)
{
	return strcmp(*(char *const *)a, *(char *const *)b);
}

/* the oldest of the logs in Download beyond the newest LOGS_KEPT, deleted
(their names sort by when they were made) */
static void old_logs_delete(void)
{
	DIR *directory = opendir(LOG_DIRECTORY);
	struct dirent *entry;
	char *names[256];
	int count = 0, index;

	if (!directory)
		return;
	while ((entry = readdir(directory)) && count < 256)
	{
		if (!strncmp(entry->d_name, "halo_log_", 9) && strstr(entry->d_name, ".txt"))
			names[count++] = strdup(entry->d_name);
	}
	closedir(directory);
	qsort(names, (size_t)count, sizeof(names[0]), compare_names);
	for (index = 0; index < count; index++)
	{
		if (index < count - LOGS_KEPT)
		{
			char path[600];

			snprintf(path, sizeof(path), "%s/%s", LOG_DIRECTORY, names[index]);
			unlink(path);
		}
		free(names[index]);
	}
}

static void log_file_open(void)
{
	char path[600], package[256] = "";
	time_t now = time(NULL);
	struct tm local;
	FILE *command;
	JNIEnv *env = (JNIEnv *)SDL_GetAndroidJNIEnv();
	jobject activity = env ? (jobject)SDL_GetAndroidActivity() : NULL;
	if (activity)
	{
		jclass type = (*env)->GetObjectClass(env, activity);
		jmethodID method = type ? (*env)->GetMethodID(env, type, "openGameLog", "()I") : NULL;
		int descriptor = method ? (*env)->CallIntMethod(env, activity, method) : -1;
		if ((*env)->ExceptionCheck(env))
		{
			(*env)->ExceptionClear(env);
			descriptor = -1;
		}
		if (descriptor >= 0)
		{
			log_files[0] = fdopen(descriptor, "a");
			if (!log_files[0]) close(descriptor);
		}
		if (type) (*env)->DeleteLocalRef(env, type);
		(*env)->DeleteLocalRef(env, activity);
	}

	/* Download/HaloCE/halo_log_<date>_<time>.txt: made new each run, never
	renamed or replaced (another installation's file may not be this
	one's to change) */
	if (!log_files[0])
	{
		localtime_r(&now, &local);
		mkdir("/sdcard/Download", 0775);
		mkdir(LOG_DIRECTORY, 0775);
		old_logs_delete();
		snprintf(path, sizeof(path), "%s/halo_log_%04d-%02d-%02d_%02d-%02d-%02d.txt", LOG_DIRECTORY,
			local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
		log_files[0] = fopen(path, "w");
		if (log_files[0])
			__android_log_print(ANDROID_LOG_INFO, "halo", "the log is also written to %s", path);
		else
			__android_log_print(ANDROID_LOG_WARN, "halo", "no log in Download (%s: %s)", path, strerror(errno));
	}

	/* and the app's own storage: /sdcard/Android/data/<package>/files, the
	package from the process's name ("com.halo.decomp.vr:halo_game") */
	command = fopen("/proc/self/cmdline", "r");
	if (command)
	{
		size_t length = fread(package, 1, sizeof(package) - 1, command);
		char *colon;

		package[length] = 0;
		colon = strchr(package, ':');
		if (colon)
			*colon = 0;
		fclose(command);
	}
	if (package[0])
	{
		snprintf(path, sizeof(path), "/sdcard/Android/data/%s/files/halo_log.txt", package);
		log_files[1] = fopen(path, "w");
	}
}

void host_log_file_line(const char *line)
{
	struct timespec now;
	struct tm local;
	size_t length = strlen(line);
	int which;

	if (!log_files[0] && !log_files[1])
		return;
	while (length && (line[length - 1] == '\r' || line[length - 1] == '\n'))
		length--;
	clock_gettime(CLOCK_REALTIME, &now);
	localtime_r(&now.tv_sec, &local);
	pthread_mutex_lock(&log_file_lock);
	for (which = 0; which < 2; which++)
	{
		if (!log_files[which])
			continue;
		fprintf(log_files[which], "%02d:%02d:%02d.%03ld %.*s\n", local.tm_hour, local.tm_min, local.tm_sec,
			now.tv_nsec / 1000000, (int)length, line);
		/* at once: a crash must not lose the lines before it */
		fflush(log_files[which]);
	}
	pthread_mutex_unlock(&log_file_lock);
}

/* ---------- logging and termination */

void host_logf(int priority, const char *format, ...)
{
	va_list arguments;
	char line[1024];

	va_start(arguments, format);
	vsnprintf(line, sizeof(line), format, arguments);
	va_end(arguments);
	__android_log_write(priority, "halo", line);
	host_log_file_line(line);
}

void host_log(int priority, const char *text)
{
	__android_log_write(priority, "halo", text);
	host_log_file_line(text);
}

void host_fatal(const char *format, ...)
{
	char message[1024];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	__android_log_write(ANDROID_LOG_FATAL, "halo", message);
	host_log_file_line(message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", message, NULL);
	_exit(1);
}

void host_abort(const char *reason)
{
	__android_log_print(ANDROID_LOG_FATAL, "halo", "guest abort: %s", reason);
	{
		char line[600];

		snprintf(line, sizeof(line), "guest abort: %s", reason);
		host_log_file_line(line);
	}
	abort();
}

void host_exit(int code)
{
	JNIEnv *env = (JNIEnv *)SDL_GetAndroidJNIEnv();
	jobject activity = env ? (jobject)SDL_GetAndroidActivity() : NULL;
	if (activity)
	{
		jclass type = (*env)->GetObjectClass(env, activity);
		jmethodID method = type ? (*env)->GetMethodID(env, type, "prepareGameExit", "()V") : NULL;
		if (method) (*env)->CallVoidMethod(env, activity, method);
		if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
		if (type) (*env)->DeleteLocalRef(env, type);
		(*env)->DeleteLocalRef(env, activity);
	}
	host_logf(HOST_LOG_INFO, "the game exited (%d)", code);
	/* the process ends with the game; Android restarts it from the
	launcher next time */
	_exit(code);
}

int host_errno(void)
{
	return errno;
}

/* ---------- paths */

static char data_root[512];
static char save_root[512];

void host_android_path(int which, char *buffer, uint32_t size)
{
	snprintf(buffer, size, "%s", which ? save_root : data_root);
}

#ifdef HALO_VR
#define SHARED_DATA_ROOT "/sdcard/Documents/HaloCE"
#endif

static int directory_has_maps(const char *root)
{
	char path[600];
	struct stat information;

	snprintf(path, sizeof(path), "%s/maps/ui.map", root);
	return stat(path, &information) == 0;
}

/* Directories the app creates in its external storage are private to it
(mode 0770 under the app's own group), so the shell user (adb) cannot list
them. Open the save tree for reading, with set-group-ID directories as
posix_make_directory creates them (port/linux/src/posix_files.c). */
static int share_entry(const char *path, const struct stat *information, int type, struct FTW *walk)
{
	(void)information;
	(void)walk;
	if (type == FTW_D || type == FTW_DP)
		chmod(path, 02775);
	else if (type == FTW_F)
		chmod(path, 0664);
	return 0;
}

static void share_save_tree(const char *root)
{
	nftw(root, share_entry, 16, FTW_PHYS);
}

/* ---------- the guest's environment */

#define ENVIRONMENT_MAXIMUM 64

struct environment
{
	char *entries[ENVIRONMENT_MAXIMUM];
	int count;
};

static void environment_set(struct environment *environment, const char *name, const char *value)
{
	size_t length = strlen(name);
	char *entry;
	int index;

	entry = malloc(length + strlen(value) + 2);
	sprintf(entry, "%s=%s", name, value);
	for (index = 0; index < environment->count; index++)
	{
		if (!strncmp(environment->entries[index], name, length) && environment->entries[index][length] == '=')
		{
			free(environment->entries[index]);
			environment->entries[index] = entry;
			return;
		}
	}
	if (environment->count < ENVIRONMENT_MAXIMUM)
		environment->entries[environment->count++] = entry;
	else
		free(entry);
}

/* debug.sample_seconds from config.toml, as text for the sampler, or 0 */
static int config_sample_seconds(const char *path, char *text, size_t size)
{
	toml_result_t result = toml_parse_file_ex(path);
	int found = 0;

	if (!result.ok)
		return 0;
	{
		toml_datum_t seconds = toml_seek(result.toptab, "debug.sample_seconds");
		double value = seconds.type == TOML_FP64 ? seconds.u.fp64 :
			seconds.type == TOML_INT64 ? (double)seconds.u.int64 : 0.0;

		if (value > 0.0)
		{
			snprintf(text, size, "%g", value);
			found = 1;
		}
	}
	toml_free(result);
	return found;
}

/* POSIX TZ for the current local offset (the guest's musl has no zone
database) */
static void time_zone(char *buffer, size_t size)
{
	time_t now = time(NULL);
	struct tm local;
	long offset;

	localtime_r(&now, &local);
	offset = -local.tm_gmtoff;
	snprintf(buffer, size, "<L>%s%ld:%02ld", offset < 0 ? "-" : "", labs(offset) / 3600, (labs(offset) / 60) % 60);
}

/* copies argv and the environment into guest memory */
static uint32_t make_boot(const struct environment *environment)
{
	size_t size = 0x10000;
	char *memory = host_low_map(size, PROT_READ | PROT_WRITE);
	struct halo_guest_boot *boot = (struct halo_guest_boot *)memory;
	uint32_t *argv = (uint32_t *)(memory + sizeof(*boot));
	uint32_t *environ_list = argv + 2;
	char *strings = (char *)(environ_list + ENVIRONMENT_MAXIMUM + 1);
	int index;

	if (!memory)
		host_fatal("cannot allocate the guest's environment");
	strcpy(strings, "halo");
	argv[0] = (uint32_t)(uintptr_t)strings;
	argv[1] = 0;
	strings += strlen(strings) + 1;
	for (index = 0; index < environment->count; index++)
	{
		size_t length = strlen(environment->entries[index]) + 1;

		if (strings + length > memory + size)
			break;
		memcpy(strings, environment->entries[index], length);
		environ_list[index] = (uint32_t)(uintptr_t)strings;
		strings += length;
	}
	environ_list[index] = 0;
	boot->argc = 1;
	boot->argv = (uint32_t)(uintptr_t)argv;
	boot->environment = (uint32_t)(uintptr_t)environ_list;
	boot->page_size = (uint32_t)getpagesize();
	return (uint32_t)(uintptr_t)boot;
}

/* ---------- main */

#define MAIN_STACK_SIZE (16 * 1024 * 1024)

static void *game_main(void *unused)
{
	struct environment environment = { { 0 }, 0 };
	const char *external;
	char zone[64];
	char path[600];
	size_t image_size = 0;
	void *image;
	uint32_t boot;

	(void)unused;
	external = SDL_GetAndroidExternalStoragePath();
	if (!external)
		host_fatal("Android storage is unavailable: %s", SDL_GetError());
	snprintf(data_root, sizeof(data_root), "%s", external);
#ifdef HALO_VR
	/* The Steam Frame runs Android in a container (Lepton) that can be
	reset, taking the app's own storage with it; its Documents folder is
	the headset's, which stays. Game data put there is preferred, with the
	saves and settings beside it. */
	if (directory_has_maps(SHARED_DATA_ROOT))
		snprintf(data_root, sizeof(data_root), "%s", SHARED_DATA_ROOT);
#endif
    {
        char selected[sizeof(data_root)];
        int profile=halo_game_data_profile(data_root,selected,sizeof(selected));
        if(profile<0)host_fatal("The selected game-data set is incomplete. Open Game files & versions in the launcher and select or import a complete set.");
        if(profile>0)snprintf(data_root,sizeof(data_root),"%s",selected);
    }
	snprintf(save_root, sizeof(save_root), "%s/save", data_root);
	/* readable by adb (the shell user), for managing saves */
	mkdir(save_root, 0775);
	share_save_tree(save_root);
	if (!directory_has_maps(data_root))
	{
		host_fatal("The Halo game data was not found.\n\nCopy supported Xbox Halo CE data, "
			"the folder that contains maps, into\n%s\nor use Game files & versions in the launcher.", data_root);
	}

	environment_set(&environment, "HOME", save_root);
	environment_set(&environment, "HALO_DATA_ROOT", data_root);
	environment_set(&environment, "HALO_SAVE_ROOT", save_root);
	{
		/* a mod the launcher installed that is made of Custom Edition maps
		(LauncherActivity.java, the mods) leaves this file while it is in:
		the game then loads such maps (game.custom_edition) */
		struct stat information;

		snprintf(path, sizeof(path), "%s/mods/custom_edition.on", data_root);
		if (stat(path, &information) == 0)
		{
			environment_set(&environment, "HALO_CUSTOM_EDITION", "1");
			host_logf(HOST_LOG_INFO, "a Custom Edition mod is installed: Custom Edition maps load");
		}
		/* the launcher's main menu choice (the VR build): a flat screen
		leaves this file, which the main menu's 3D setting gives way to
		(vr.menu_3d, which has no place in the pause menu's settings) */
		snprintf(path, sizeof(path), "%s/vr_menu_flat.on", data_root);
		if (stat(path, &information) == 0)
		{
			environment_set(&environment, "HALO_VR_MENU_3D", "false");
			host_logf(HOST_LOG_INFO, "the launcher asks for the main menu on a flat screen");
		}
	}
	{
		/* the game renders 480 lines at the display's aspect ratio
		(landscape) unless display.screen_width says otherwise (d3d8_gl.c) */
		const SDL_DisplayMode *mode;
		char width[16];

		SDL_InitSubSystem(SDL_INIT_VIDEO);
		mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
		if (mode && mode->w > 0 && mode->h > 0)
		{
			int longer = mode->w > mode->h ? mode->w : mode->h;
			int shorter = mode->w > mode->h ? mode->h : mode->w;

			snprintf(width, sizeof(width), "%d", (480 * longer / shorter) & ~1);
			environment_set(&environment, "HALO_DISPLAY_WIDTH", width);
			host_logf(HOST_LOG_INFO, "display %dx%d: rendering %sx480", mode->w, mode->h, width);
		}
	}
	time_zone(zone, sizeof(zone));
	environment_set(&environment, "TZ", zone);
	snprintf(path, sizeof(path), "%s/config.toml", data_root);

	image = SDL_LoadFile("halo_guest.elf", &image_size);
	if (!image)
		host_fatal("cannot read the game image from the APK: %s", SDL_GetError());
	/* test26: the files were found (the launcher checked them); this is the
	device's memory, said so in words, with the log for the details */
	if (host_load_image(image, image_size) != 0)
		host_fatal("The game files are fine, but the game could not start on this device: %s "
			"Restart the headset or phone and try again. If it keeps happening, send the log "
			"(Download/HaloCE).", host_memory_failure ? host_memory_failure : "it could not load its code.");
	SDL_free(image);

	{
		char seconds[32];

		if (config_sample_seconds(path, seconds, sizeof(seconds)))
			host_debug_start_sampler(seconds);
		/* stacks in the log when the game stops showing frames */
		host_debug_start_watchdog();
	}
	boot = make_boot(&environment);
	host_logf(HOST_LOG_INFO, "data %s, saves %s", data_root, save_root);
	host_run_guest_main(boot);
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	log_file_open();
	host_logf(HOST_LOG_INFO, "Halo for Android starting");
	host_install_signal_handlers();
	if (host_native_thread_create(game_main, NULL, MAIN_STACK_SIZE) != 0)
		host_fatal("cannot start the game thread");
	/* the game ends the process itself (host_exit) */
	for (;;)
		pause();
}
