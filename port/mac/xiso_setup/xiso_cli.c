/*
XISO_CLI.C

Command-line front end of the setup's extraction, for tests and scripts:
  xiso_cli install IMAGE DATA_ROOT   exit 0 installed, 1 refused, 2 cancelled
  xiso_cli verify MAPS_FOLDER        exit 0 if the folder holds the Xbox maps
Prints the reason on standard error.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xiso_install.h"

static volatile int cancel_requested;

static void progress(void *context, const char *file, unsigned long long done, unsigned long long total)
{
	(void)context;
	(void)file;
	/* (a test of cancelling: XISO_CLI_CANCEL_AFTER_MB=n stops the copy at n MB) */
	if (getenv("XISO_CLI_CANCEL_AFTER_MB") && done >> 20 >= (unsigned long long)atol(getenv("XISO_CLI_CANCEL_AFTER_MB")))
		cancel_requested = 1;
	if (total && (done * 10 / total) != ((done - 1) * 10 / total))
		fprintf(stderr, "%d%%\n", (int)(done * 100 / total));
}

int main(int argc, char **argv)
{
	char error[512] = "";
	int cancelled = 0;

	if (argc == 4 && !strcmp(argv[1], "install"))
	{
		if (xiso_install(argv[2], argv[3], progress, NULL, &cancel_requested, &cancelled, error, sizeof(error)))
		{
			printf("installed\n");
			return 0;
		}
		fprintf(stderr, "refused: %s\n", error);
		return cancelled ? 2 : 1;
	}
	if (argc == 3 && !strcmp(argv[1], "verify"))
	{
		if (xiso_verify_maps(argv[2], error, sizeof(error)))
		{
			printf("ok\n");
			return 0;
		}
		fprintf(stderr, "refused: %s\n", error);
		return 1;
	}
	fprintf(stderr, "usage: xiso_cli install IMAGE DATA_ROOT | verify MAPS_FOLDER\n");
	return 64;
}
