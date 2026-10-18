/*
PLATFORM.C (the Mac's)

The one function of platform.h that port/mac does not already define.
platform_log is in player_services.c; platform_data_root is here because
the Linux layer answers it from xbox_files.c, which the Mac build does not
compile — it has its own answer, in engine_files.c, reached by a different
name.

Internet play is what wants it: p2p.c reads and writes join_link.txt in
the data root, which is how a launcher hands a halo:// invite to the game
that is already running.
*/

#include <limits.h>
#include <string.h>

#include "platform.h"
#include "engine_files.h"

const char *platform_data_root(void)
{
	/* (the root does not change while the game runs: the launcher sets
	HALO_MAC_DATA_ROOT before it starts, and nothing moves it after) */
	static char root[PATH_MAX];
	static int looked;

	if (!looked)
	{
		size_t length;

		looked = 1;
		if (!mac_engine_files_data_root(root, sizeof(root)))
			root[0] = '\0';
		/* trailing slashes off, as the Linux layer's does, so that the
		"%s/join_link.txt" its callers write does not come out doubled */
		length = strlen(root);
		while (length > 1 && root[length - 1] == '/')
			root[--length] = '\0';
	}
	return root;
}

/* (no box with several buttons yet: Carbon's standard alert has two, and
engine_messages.c shows one-button messages on the window's own thread) */
int platform_ask(const char *title, const char *text, const char *const *labels, int count, int default_index)
{
	(void)title; (void)text; (void)labels; (void)count; (void)default_index;
	return -1;
}
