/* See launcher_invite.h. No Carbon here: this is the part that can be
   tested without a Mac. */
#include "launcher_invite.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

int launcher_invite_write(const char *data_root, const char *link)
{
	char path[PATH_MAX], temporary[PATH_MAX];
	FILE *file;

	if (!link || !*link || !data_root || !*data_root)
		return 0;
	/* (only this game's scheme. Launch Services should send nothing else,
	and what is written here goes to the game's signalling) */
	if (strncmp(link, "halo://", 7) != 0)
		return 0;
	if (strchr(link, '\n') || strchr(link, '\r'))
		return 0;
	if (snprintf(temporary, sizeof(temporary), "%s/join_link.new", data_root) >= (int)sizeof(temporary))
		return 0;
	if (snprintf(path, sizeof(path), "%s/join_link.txt", data_root) >= (int)sizeof(path))
		return 0;
	file = fopen(temporary, "wb");
	if (!file)
		return 0;
	fprintf(file, "%s\n", link);
	if (fclose(file) != 0 || rename(temporary, path) != 0)
	{
		remove(temporary);
		return 0;
	}
	return 1;
}
