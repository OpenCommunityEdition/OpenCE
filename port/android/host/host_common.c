/*
HOST_COMMON.C

What every host's main shares (the Android app's, port/android/host, and
the Linux executable's, port/linux/host): logging and termination, the
guest's environment and boot block, and the debugging settings of
config.toml that the host reads itself.

The log's lines go to the sink the host's main chooses (host_log_set_sink).
Signal handlers log through it too (host_memory.c, host_debug.c), so a sink
writes with neither locks nor stdio.
*/

#include "host.h"
#include "tomlc17.h"

#include <SDL3/SDL.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif

/* ---------- logging and termination */

/* until the host sets one: the standard error stream, or on Android (whose
app has none) logcat */
static void default_sink(int priority, const char *line)
{
#ifdef __ANDROID__
	__android_log_write(priority, "halo", line);
#else
	size_t length = strlen(line);

	(void)priority;
	if (write(2, line, length) < 0 || write(2, "\n", 1) < 0)
		return;
#endif
}

static host_log_sink log_sink = default_sink;

void host_log_set_sink(host_log_sink sink)
{
	log_sink = sink ? sink : default_sink;
}

void host_log(int priority, const char *text)
{
	log_sink(priority, text ? text : "");
}

void host_logf(int priority, const char *format, ...)
{
	char line[1024];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(line, sizeof(line), format, arguments);
	va_end(arguments);
	log_sink(priority, line);
}

void host_fatal(const char *format, ...)
{
	char message[1024];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	log_sink(HOST_LOG_FATAL, message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo", message, NULL);
	_exit(1);
}

void host_abort(const char *reason)
{
	host_logf(HOST_LOG_FATAL, "guest abort: %s", reason ? reason : "");
	abort();
}

void host_exit(int code)
{
	host_logf(HOST_LOG_INFO, "the game exited (%d)", code);
	/* (what the host itself printed: the guest's output does not go
	through the host's stdio, host_syscall.c) */
	fflush(stdout);
	fflush(stderr);
	_exit(code);
}

int host_errno(void)
{
	return errno;
}

/* ---------- the guest's environment */

static void environment_put(struct host_environment *environment, char *entry, size_t name_length)
{
	int index;

	for (index = 0; index < environment->count; index++)
	{
		if (!strncmp(environment->entries[index], entry, name_length) && environment->entries[index][name_length] == '=')
		{
			free(environment->entries[index]);
			environment->entries[index] = entry;
			return;
		}
	}
	if (environment->count < HOST_ENVIRONMENT_MAXIMUM)
	{
		environment->entries[environment->count++] = entry;
		return;
	}
	host_logf(HOST_LOG_WARN, "the guest's environment is full (%d entries): %.*s is left out",
		HOST_ENVIRONMENT_MAXIMUM, (int)name_length, entry);
	free(entry);
}

void host_environment_set(struct host_environment *environment, const char *name, const char *value)
{
	size_t name_length = strlen(name);
	size_t size = name_length + strlen(value) + 2;
	char *entry = malloc(size);

	if (!entry)
	{
		host_logf(HOST_LOG_ERROR, "out of memory for the guest's environment: %s is left out", name);
		return;
	}
	snprintf(entry, size, "%s=%s", name, value);
	environment_put(environment, entry, name_length);
}

void host_environment_add(struct host_environment *environment, const char *entry)
{
	const char *equals = strchr(entry, '=');
	char *copy;

	if (!equals || equals == entry)
		return;
	copy = strdup(entry);
	if (!copy)
	{
		host_logf(HOST_LOG_ERROR, "out of memory for the guest's environment: %.*s is left out",
			(int)(equals - entry), entry);
		return;
	}
	environment_put(environment, copy, (size_t)(equals - entry));
}

void host_time_zone(char *buffer, size_t size)
{
	time_t now = time(NULL);
	struct tm local;
	long offset;

	localtime_r(&now, &local);
	offset = -local.tm_gmtoff;
	snprintf(buffer, size, "<L>%s%ld:%02ld", offset < 0 ? "-" : "", labs(offset) / 3600, (labs(offset) / 60) % 60);
}

/* The boot block, then argv[2], then the environment's pointers (one more
for its NULL), then the strings. */
uint32_t host_make_boot(const struct host_environment *environment, const char *program)
{
	size_t pointers = sizeof(struct halo_guest_boot) + (2 + environment->count + 1) * sizeof(uint32_t);
	size_t size = pointers + strlen(program) + 1;
	struct halo_guest_boot *boot;
	uint32_t *argv, *environ_list;
	char *memory, *strings;
	int index;

	for (index = 0; index < environment->count; index++)
		size += strlen(environment->entries[index]) + 1;
	memory = host_low_map(size, PROT_READ | PROT_WRITE);
	if (!memory)
		host_fatal("cannot allocate the guest's environment (%zu bytes)", size);
	boot = (struct halo_guest_boot *)memory;
	argv = (uint32_t *)(memory + sizeof(*boot));
	environ_list = argv + 2;
	strings = memory + pointers;

	memcpy(strings, program, strlen(program) + 1);
	argv[0] = (uint32_t)(uintptr_t)strings;
	argv[1] = 0;
	strings += strlen(program) + 1;
	for (index = 0; index < environment->count; index++)
	{
		size_t length = strlen(environment->entries[index]) + 1;

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

/* ---------- config.toml's debugging settings */

void host_apply_debug_settings(const char *path, const char *hashes_reason)
{
	toml_result_t result = toml_parse_file_ex(path);
	double sample_seconds = 0.0;
	int memory_watch = 1;

	if (result.ok)
	{
		toml_datum_t seconds = toml_seek(result.toptab, "debug.sample_seconds");
		toml_datum_t watch = toml_seek(result.toptab, "debug.memory_watch");

		sample_seconds = seconds.type == TOML_FP64 ? seconds.u.fp64 :
			seconds.type == TOML_INT64 ? (double)seconds.u.int64 : 0.0;
		if (watch.type == TOML_BOOLEAN)
			memory_watch = watch.u.boolean ? 1 : 0;
		toml_free(result);
	}
	/* (the setting's variable, as the game's settings have one: the Linux
	host's environment) */
	if (getenv("HALO_SAMPLE"))
		sample_seconds = atof(getenv("HALO_SAMPLE"));
	if (getenv("HALO_MEMORY_WATCH") && *getenv("HALO_MEMORY_WATCH"))
	{
		/* ("0", "false", "no" and "off" are false, as for the game's settings) */
		const char *value = getenv("HALO_MEMORY_WATCH");

		memory_watch = !(!strcmp(value, "0") || !strcasecmp(value, "false") || !strcasecmp(value, "no") ||
			!strcasecmp(value, "off"));
	}
	if (sample_seconds > 0.0)
	{
		char text[32];

		snprintf(text, sizeof(text), "%g", sample_seconds);
		host_debug_start_sampler(text);
	}
	if (hashes_reason)
	{
		host_memory_watch_use_hashes();
		host_logf(HOST_LOG_INFO, "write tracking: page hashes (%s)", hashes_reason);
	}
	else if (!memory_watch)
	{
		host_memory_watch_use_hashes();
		host_logf(HOST_LOG_INFO, "write tracking: page hashes (debug.memory_watch = false)");
	}
	else
	{
		host_logf(HOST_LOG_INFO, "write tracking: page protection");
	}
}
