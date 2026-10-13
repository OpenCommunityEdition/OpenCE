/*
MAP_DOWNLOAD_CHECK.C

A check of the map downloads' file handling (port/linux/src/map_archives_zip.c)
outside the game (tools/test_map_download_check.py):

    map_download_check unzip <zip> <name> <size> <path>
    map_download_check verify <file> <name> <size> <piece length> <info hash>

"unzip" takes a map out of a map archive's zip, as the game does after
fetching it; "verify" checks a file against the index's info hash, as the
game does before using it (map_archives_verify).

Built with those units, the platform layer's POSIX files and this file's own
stand-in for the platform (platform_log): tools/linux_build.py.
*/

#include "platform.h"
#include "posix.h"
#include "map_archives.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the platform layer's, for the units */
void platform_log(const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	fprintf(stderr, "map_download_check: ");
	vfprintf(stderr, format, arguments);
	fprintf(stderr, "\n");
	va_end(arguments);
}

int main(int argc, char **argv)
{
	char error[192];

	if (argc == 6 && !strcmp(argv[1], "unzip"))
	{
		if (!map_archives_unzip(argv[2], argv[3], strtoull(argv[4], NULL, 10), argv[5], error, sizeof(error)))
		{
			printf("failed: %s\n", error);
			return 1;
		}
		printf("unzipped\n");
		return 0;
	}
	if (argc == 7 && !strcmp(argv[1], "verify"))
	{
		if (!map_archives_verify(argv[2], argv[3], strtoull(argv[4], NULL, 10), strtoul(argv[5], NULL, 10), argv[6],
			error, sizeof(error)))
		{
			printf("failed: %s\n", error);
			return 1;
		}
		printf("verified\n");
		return 0;
	}
	fprintf(stderr,
		"usage: map_download_check unzip <zip> <name> <size> <path>\n"
		"       map_download_check verify <file> <name> <size> <piece length> <info hash>\n");
	return 2;
}
