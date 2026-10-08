/*
HOST_MAIN.C

Entry point of the macOS port (Halo.app/Contents/MacOS/halo).

It loads the guest image (the game, built as x32 code) from the bundle,
gives it an environment describing where the game data and saves live, and
runs its main() on the process's main thread, which Cocoa requires for the
window and its events. Start-up, in this order:

1. claim the address space below 4 GB (host_memory.c), before anything
   else can map into it;
2. install the fatal signals' handlers on an alternate stack, so that a
   crash is logged and the process ends with _exit rather than reaching the
   system's crash reporting;
3. ignore SIGPIPE (sockets and pipes report EPIPE instead);
4. work out the paths and run the launcher (host_launcher.c: the game data
   check and import, on the ordinary stack);
5. load the guest image;
6. move the main thread onto a 16 MB stack in guest memory
   (host_stack.S) and call the guest's start there. SDL, Cocoa and OpenGL,
   which the guest calls, then run on that stack too.

Storage (see port/macos/README.md): the game data (the directory holding
maps/), config.toml, debug.txt and brokers.txt live in
~/Library/Application Support/OpenCE, the data root; saves go to its save/
subdirectory. Nothing is ever written inside the app bundle. HALO_DATA_ROOT
and HALO_SAVE_ROOT in the environment name others. The guest's environment
is built here, as on Android: those two, HOME, TZ, and every HALO_ variable
the app was started with (the settings' overrides, port/linux/src/port_config.c).

The log goes to standard error and to host.log in the data root.
*/

#include "host.h"
#include "host_linux.h"
#include "host_services.h"
#include "tomlc17.h"

#include <CoreFoundation/CoreFoundation.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <uuid/uuid.h>

/* ---------- logging and termination */

static int log_fd = -1;

static void log_write(const char *text, size_t length)
{
	ssize_t ignored;

	ignored = write(2, text, length);
	if (log_fd >= 0)
		ignored = write(log_fd, text, length);
	(void)ignored;
}

static void log_vprint(int priority, const char *format, va_list arguments)
{
	static const char letters[] = "??VDIWEF";
	char line[2048];
	int length;

	length = snprintf(line, sizeof(line), "%c/halo: ", priority >= 0 && priority < 8 ? letters[priority] : '?');
	length += vsnprintf(line + length, sizeof(line) - (size_t)length - 1, format, arguments);
	if (length > (int)sizeof(line) - 2)
		length = (int)sizeof(line) - 2;
	line[length++] = '\n';
	log_write(line, (size_t)length);
}

void host_logf(int priority, const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	log_vprint(priority, format, arguments);
	va_end(arguments);
}

/* the guest's log (Android's priorities) */
void host_log(int priority, const char *text)
{
	host_logf(priority, "%s", text);
}

void host_log_guest_output(int fd, const char *bytes, size_t size)
{
	ssize_t ignored;

	(void)fd;
	if (log_fd >= 0)
	{
		ignored = write(log_fd, bytes, size);
		(void)ignored;
	}
}

/* the host's own frames, for the crash reporter (host_memory.c) */
void host_log_crash_backtrace(void)
{
	void *frames[48];
	int count = backtrace(frames, 48);

	backtrace_symbols_fd(frames, count, 2);
	if (log_fd >= 0)
		backtrace_symbols_fd(frames, count, log_fd);
}

/* whether a dialog may be shown (not in the automated runs that hide the
window, which nobody would dismiss: host_launcher.c's test) */
static char data_root[PATH_MAX];

static int dialogs_allowed(void)
{
	return !host_unattended(data_root);
}

static void show_alert(const char *title, const char *message)
{
	CFStringRef header, text;
	CFOptionFlags response;

	if (!dialogs_allowed())
		return;
	header = CFStringCreateWithCString(NULL, title, kCFStringEncodingUTF8);
	text = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);
	if (!text)
		text = CFStringCreateWithCString(NULL, message, kCFStringEncodingMacRoman);
	/* (no main-thread requirement: safe whichever thread fails, the audio
	thread's included, while the main thread runs guest code) */
	CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, header, text, NULL,
		NULL, NULL, &response);
	if (header)
		CFRelease(header);
	if (text)
		CFRelease(text);
}

void host_fatal(const char *format, ...)
{
	char message[1024];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	host_logf(HOST_LOG_FATAL, "%s", message);
	show_alert("Halo", message);
	_exit(1);
}

void host_abort(const char *reason)
{
	host_logf(HOST_LOG_FATAL, "guest abort: %s", reason);
	host_log_crash_backtrace();
	/* (not abort(): the process ends without a crash report) */
	_exit(134);
}

void host_exit(int code)
{
	host_logf(HOST_LOG_INFO, "the game exited (%d)", code);
	_exit(code);
}

int host_errno(void)
{
	return host_linux_errno(errno);
}

/* ---------- paths */

static char save_root[PATH_MAX];

const char *host_data_root(void)
{
	return data_root;
}

const char *host_save_root(void)
{
	return save_root;
}

/* the Android host's import of the same name: the data root (0) or the
saves' (1) */
void host_android_path(int which, char *buffer, uint32_t size)
{
	snprintf(buffer, size, "%s", which ? save_root : data_root);
}

static void make_directories(const char *path)
{
	char partial[PATH_MAX];
	size_t index;

	snprintf(partial, sizeof(partial), "%s", path);
	for (index = 1; partial[index]; index++)
	{
		if (partial[index] == '/')
		{
			partial[index] = 0;
			mkdir(partial, 0755);
			partial[index] = '/';
		}
	}
	mkdir(partial, 0755);
}

static void choose_roots(void)
{
	const char *home = getenv("HOME");
	const char *data = getenv("HALO_DATA_ROOT");
	const char *saves = getenv("HALO_SAVE_ROOT");

	if (!home || !*home)
	{
		struct passwd *user = getpwuid(getuid());

		home = user && user->pw_dir ? user->pw_dir : "/tmp";
	}
	if (data && *data)
		snprintf(data_root, sizeof(data_root), "%s", data);
	else
		snprintf(data_root, sizeof(data_root), "%s/Library/Application Support/OpenCE", home);
	if (saves && *saves)
		snprintf(save_root, sizeof(save_root), "%s", saves);
	else
		snprintf(save_root, sizeof(save_root), "%s/save", data_root);
	make_directories(data_root);
	make_directories(save_root);
}

/* the directory holding the bundle's resources (Contents/Resources), or
the executable's own when it runs outside a bundle (build/macos/halo) */
static void resource_directory(char *buffer, size_t size)
{
	char executable[PATH_MAX], resolved[PATH_MAX];
	uint32_t length = sizeof(executable);
	char *slash;
	struct stat information;

	buffer[0] = 0;
	if (_NSGetExecutablePath(executable, &length) != 0)
		return;
	if (!realpath(executable, resolved))
		snprintf(resolved, sizeof(resolved), "%s", executable);
	slash = strrchr(resolved, '/');
	if (!slash)
		return;
	*slash = 0;
	snprintf(buffer, size, "%s/../Resources", resolved);
	if (stat(buffer, &information) == 0 && S_ISDIR(information.st_mode))
		return;
	snprintf(buffer, size, "%s", resolved);
}

static void *read_file(const char *path, size_t *size)
{
	int fd = open(path, O_RDONLY | O_CLOEXEC);
	struct stat information;
	char *data;
	size_t done = 0;

	if (fd < 0)
		return NULL;
	if (fstat(fd, &information) != 0 || !(data = malloc((size_t)information.st_size + 1)))
	{
		close(fd);
		return NULL;
	}
	while (done < (size_t)information.st_size)
	{
		ssize_t count = read(fd, data + done, (size_t)information.st_size - done);

		if (count <= 0)
			break;
		done += (size_t)count;
	}
	close(fd);
	if (done != (size_t)information.st_size)
	{
		free(data);
		return NULL;
	}
	data[done] = 0;
	*size = done;
	return data;
}

static int write_file(const char *path, const void *data, size_t size)
{
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
	ssize_t written;

	if (fd < 0)
		return 0;
	written = write(fd, data, size);
	close(fd);
	return written == (ssize_t)size;
}

/* what this machine is known by, for p2p.c's hardware id, which reads
hardware_id.txt in the data root in a guest (Android's LauncherActivity
writes ANDROID_ID there): the host UUID, the IOPlatformUUID macOS keeps for
the machine. Written beside and renamed, so the guest never reads half */
static void write_hardware_id(void)
{
	uuid_t uuid;
	struct timespec wait = { 1, 0 };
	char text[40], path[PATH_MAX + 32], partial[PATH_MAX + 32];

	if (gethostuuid(uuid, &wait) != 0)
	{
		host_logf(HOST_LOG_WARN, "no host UUID (%s); the hardware id is empty", strerror(errno));
		return;
	}
	uuid_unparse_lower(uuid, text);
	strcat(text, "\n");
	snprintf(path, sizeof(path), "%s/hardware_id.txt", data_root);
	snprintf(partial, sizeof(partial), "%s.tmp", path);
	if (!write_file(partial, text, strlen(text)) || rename(partial, path) != 0)
		host_logf(HOST_LOG_ERROR, "cannot write %s", path);
}

/* ---------- the guest's environment */

#define ENVIRONMENT_MAXIMUM 128

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
#define MAIN_STACK_GUARD 0x4000

extern char **environ;

/* The host executable is position independent, and the system places it
(with Rosetta's runtime and the first allocations after it) at a random
address in the low 256 MB, so it can, rarely, reach into the range the
guest image is linked at. Another start places it elsewhere. */
#define GUEST_IMAGE_RESERVE 0x04000000u
#define RELAUNCH_VARIABLE "HALO_HOST_RELAUNCHES"

static void relaunch_if_the_guest_ranges_are_taken(char *argv[])
{
	const char *previous = getenv(RELAUNCH_VARIABLE);
	int count = previous ? atoi(previous) : 0;
	char executable[PATH_MAX], text[16];
	uint32_t length = sizeof(executable);

	if (host_memory_claimed(HALO_GUEST_IMAGE_BASE, GUEST_IMAGE_RESERVE) &&
		host_memory_claimed(HALO_GUEST_WINDOW_BASE, HALO_GUEST_WINDOW_SIZE))
	{
		unsetenv(RELAUNCH_VARIABLE);
		return;
	}
	if (count >= 3 || _NSGetExecutablePath(executable, &length) != 0)
	{
		/* (host_load_image reports which range is taken) */
		return;
	}
	host_logf(HOST_LOG_WARN, "the guest's fixed address ranges are not free; starting again");
	snprintf(text, sizeof(text), "%d", count + 1);
	setenv(RELAUNCH_VARIABLE, text, 1);
	execv(executable, argv);
}

/* on the guest stack, for good */
static void game_main(uint64_t boot)
{
	host_run_guest_main((uint32_t)boot);
}

int main(int argc, char *argv[])
{
	struct environment environment = { { 0 }, 0 };
	char resources[PATH_MAX], path[PATH_MAX + 64], zone[64];
	size_t image_size = 0;
	void *image;
	uint32_t boot;
	char *stack;
	char **variable;

	(void)argc;
	/* 1: before anything maps into the low 4 GB */
	if (host_memory_claim() != 0)
		host_fatal("cannot reserve the address space below 4 GB the game needs");
	relaunch_if_the_guest_ranges_are_taken(argv);
	/* 2, 3 */
	host_install_signal_handlers();
	signal(SIGPIPE, SIG_IGN);

	/* 4 */
	choose_roots();
	snprintf(path, sizeof(path), "%s/host.log", data_root);
	log_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND | O_CLOEXEC, 0644);
	host_logf(HOST_LOG_INFO, "Halo for macOS starting; data %s, saves %s", data_root, save_root);
	if (host_launcher_prepare(data_root, sizeof(data_root), save_root, sizeof(save_root)) != 0)
	{
		host_logf(HOST_LOG_INFO, "the launcher was closed without game data");
		_exit(0);
	}
	resource_directory(resources, sizeof(resources));
	write_hardware_id();

	environment_set(&environment, "HOME", save_root);
	environment_set(&environment, "HALO_DATA_ROOT", data_root);
	environment_set(&environment, "HALO_SAVE_ROOT", save_root);
	time_zone(zone, sizeof(zone));
	environment_set(&environment, "TZ", zone);
	/* the settings' overrides (HALO_FULLSCREEN and the like) */
	for (variable = environ; *variable; variable++)
	{
		const char *equals = strchr(*variable, '=');
		char name[128];

		if (strncmp(*variable, "HALO_", 5) || !equals || (size_t)(equals - *variable) >= sizeof(name))
			continue;
		memcpy(name, *variable, (size_t)(equals - *variable));
		name[equals - *variable] = 0;
		if (strcmp(name, "HALO_DATA_ROOT") && strcmp(name, "HALO_SAVE_ROOT"))
			environment_set(&environment, name, equals + 1);
	}
	/* internet play's MQTT brokers (network.brokers_file): the bundle's
	list, written beside config.toml at each start, as a desktop update
	replaces the file beside its game */
	{
		size_t brokers_size = 0;
		void *brokers;

		snprintf(path, sizeof(path), "%s/brokers.txt", resources);
		brokers = read_file(path, &brokers_size);
		snprintf(path, sizeof(path), "%s/brokers.txt", data_root);
		if (!brokers || !write_file(path, brokers, brokers_size))
			host_logf(HOST_LOG_ERROR, "cannot write %s from the app's resources", path);
		free(brokers);
	}

	/* 5 */
	snprintf(path, sizeof(path), "%s/halo_guest.elf", resources);
	image = read_file(path, &image_size);
	if (!image)
		host_fatal("cannot read the game image %s (%s)", path, strerror(errno));
	if (host_load_image(image, image_size) != 0)
		host_fatal("cannot load the game image; see %s/host.log for details", data_root);
	free(image);

	{
		char seconds[32];

		snprintf(path, sizeof(path), "%s/config.toml", data_root);
		if (config_sample_seconds(path, seconds, sizeof(seconds)))
			host_debug_start_sampler(seconds);
	}
	boot = make_boot(&environment);

	/* 6: the guest's main thread is this one, on a stack below 4 GB */
	stack = host_low_map(MAIN_STACK_SIZE + MAIN_STACK_GUARD, PROT_READ | PROT_WRITE);
	if (!stack)
		host_fatal("cannot allocate the game's stack");
	mprotect(stack, MAIN_STACK_GUARD, PROT_NONE);
	host_debug_thread_started();
	host_logf(HOST_LOG_INFO, "starting the game on the main thread, stack %p-%p", (void *)(stack + MAIN_STACK_GUARD),
		(void *)(stack + MAIN_STACK_GUARD + MAIN_STACK_SIZE));
	host_switch_stack(stack + MAIN_STACK_GUARD + MAIN_STACK_SIZE, game_main, boot);
}
