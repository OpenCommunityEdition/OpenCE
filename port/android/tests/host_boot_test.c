/*
HOST_BOOT_TEST.C

Tests of the boot code the Android and Linux hosts share
(port/android/host/host_boot.c).
Built and run by tools/test_host_boot.py with a temporary directory as
argv[1]; exits nonzero on a failure.
*/

#include "host.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static int failures;

#define CHECK(condition) \
	do \
	{ \
		if (!(condition)) \
		{ \
			printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
			failures++; \
		} \
	} while (0)

/* what host_boot.c calls elsewhere */

void host_logf(int priority, const char *format, ...)
{
	(void)priority;
	(void)format;
}

void host_fatal(const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	vprintf(format, arguments);
	va_end(arguments);
	printf("\n");
	exit(2);
}

void host_debug_start_sampler(const char *setting)
{
	(void)setting;
}

void *host_low_map(size_t size, int protection)
{
	(void)size;
	(void)protection;
	return NULL;
}

static const char *directory;

static void make_directory(const char *path)
{
	mkdir(path, 0755);
}

static void write_file(const char *path, const char *text)
{
	FILE *file = fopen(path, "w");

	if (!file)
	{
		printf("FAIL cannot write %s\n", path);
		failures++;
		return;
	}
	fputs(text, file);
	fclose(file);
}

static void test_directory_has_maps(void)
{
	char root[600];
	char path[700];

	snprintf(root, sizeof(root), "%s/data", directory);
	make_directory(root);
	CHECK(!host_directory_has_maps(root));
	snprintf(path, sizeof(path), "%s/maps", root);
	make_directory(path);
	CHECK(!host_directory_has_maps(root));
	snprintf(path, sizeof(path), "%s/maps/ui.map", root);
	write_file(path, "x");
	CHECK(host_directory_has_maps(root));
}

static void free_environment(struct host_environment *environment)
{
	int index;

	for (index = 0; index < environment->count; index++)
		free(environment->entries[index]);
	environment->count = 0;
}

static void test_environment_set(void)
{
	struct host_environment environment;
	char name[32];
	int index;

	memset(&environment, 0, sizeof(environment));
	host_environment_set(&environment, "A", "1");
	host_environment_set(&environment, "AB", "2");
	CHECK(environment.count == 2);
	CHECK(!strcmp(environment.entries[0], "A=1"));
	CHECK(!strcmp(environment.entries[1], "AB=2"));
	/* the same name replaces; a longer name sharing its prefix does not */
	host_environment_set(&environment, "A", "3");
	CHECK(environment.count == 2);
	CHECK(!strcmp(environment.entries[0], "A=3"));
	CHECK(!strcmp(environment.entries[1], "AB=2"));
	/* entries beyond the maximum are dropped */
	for (index = 0; index < HOST_ENVIRONMENT_MAXIMUM + 5; index++)
	{
		snprintf(name, sizeof(name), "N%d", index);
		host_environment_set(&environment, name, "v");
	}
	CHECK(environment.count == HOST_ENVIRONMENT_MAXIMUM);
	host_environment_set(&environment, "A", "4");
	CHECK(environment.count == HOST_ENVIRONMENT_MAXIMUM);
	CHECK(!strcmp(environment.entries[0], "A=4"));
	free_environment(&environment);
}

static void config_path(char *path, size_t size, const char *name)
{
	snprintf(path, size, "%s/%s", directory, name);
}

static void test_config_boolean_or(void)
{
	char path[600];

	config_path(path, sizeof(path), "boolean.toml");
	write_file(path, "[debug]\nmemory_watch = false\nsample_seconds = 5\nname = \"text\"\n");
	CHECK(host_config_boolean_or(path, "debug.memory_watch", 1) == 0);
	CHECK(host_config_boolean_or(path, "debug.missing", 1) == 1);
	CHECK(host_config_boolean_or(path, "debug.missing", 0) == 0);
	/* not a boolean */
	CHECK(host_config_boolean_or(path, "debug.sample_seconds", 1) == 1);
	CHECK(host_config_boolean_or(path, "debug.name", 0) == 0);
	config_path(path, sizeof(path), "absent.toml");
	CHECK(host_config_boolean_or(path, "debug.memory_watch", 1) == 1);
	CHECK(host_config_boolean_or(path, "debug.memory_watch", 0) == 0);
}

static void test_config_sample_seconds(void)
{
	char path[600];
	char text[32];

	config_path(path, sizeof(path), "sample.toml");
	write_file(path, "[debug]\nsample_seconds = 5\n");
	CHECK(host_config_sample_seconds(path, text, sizeof(text)) == 1);
	CHECK(!strcmp(text, "5"));
	config_path(path, sizeof(path), "unsampled.toml");
	write_file(path, "[debug]\nmemory_watch = true\n");
	CHECK(host_config_sample_seconds(path, text, sizeof(text)) == 0);
}

static void test_time_zone(void)
{
	char buffer[32];

	setenv("TZ", "EST5", 1);
	tzset();
	host_time_zone(buffer, sizeof(buffer));
	CHECK(!strcmp(buffer, "<L>5:00"));
	setenv("TZ", "CET-1", 1);
	tzset();
	host_time_zone(buffer, sizeof(buffer));
	CHECK(!strcmp(buffer, "<L>-1:00"));
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("usage: host_boot_test DIRECTORY\n");
		return 2;
	}
	directory = argv[1];
	test_directory_has_maps();
	test_environment_set();
	test_config_boolean_or();
	test_config_sample_seconds();
	test_time_zone();
	if (failures)
		printf("%d failures\n", failures);
	else
		printf("all tests passed\n");
	return failures != 0;
}
