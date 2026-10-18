/*
TEST_LAUNCHER_INVITE.C

launcher_invite.c on any computer:

    cc -std=gnu89 -Wall -o test_launcher_invite test_launcher_invite.c \
        launcher_invite.c && ./test_launcher_invite <folder>

The Apple event that brings the link is Carbon and is not tested here; what
is tested is what the launcher does with one once it has it.
*/

#include "launcher_invite.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

static void check(int ok, const char *what)
{
	if (!ok)
	{
		printf("FAILED: %s\n", what);
		failures++;
	}
}

/* the file's contents, or NULL */
static char *slurp(const char *path)
{
	static char text[2048];
	FILE *file = fopen(path, "rb");
	size_t length;

	if (!file)
		return NULL;
	length = fread(text, 1, sizeof(text) - 1, file);
	fclose(file);
	text[length] = 0;
	return text;
}

int main(int argc, char **argv)
{
	char path[1024], temporary[1024];
	const char *root = argc > 1 ? argv[1] : ".";
	char *text;

	sprintf(path, "%s/join_link.txt", root);
	sprintf(temporary, "%s/join_link.new", root);
	remove(path);
	remove(temporary);

	/* a link is written, with its end of line */
	check(launcher_invite_write(root, "halo://join/abc123") == 1, "a halo link is written");
	text = slurp(path);
	check(text && strcmp(text, "halo://join/abc123\n") == 0, "the link is what was written");

	/* and nothing is left beside it */
	check(slurp(temporary) == NULL, "the temporary file is renamed, not left");

	/* a second link replaces the first */
	check(launcher_invite_write(root, "halo://join/second") == 1, "a second link is written");
	text = slurp(path);
	check(text && strcmp(text, "halo://join/second\n") == 0, "the second link replaced the first");

	/* anything that is not this game's scheme is refused, and what was
	already there is left alone */
	check(launcher_invite_write(root, "http://example.com/") == 0, "http is refused");
	check(launcher_invite_write(root, "javascript:alert(1)") == 0, "javascript is refused");
	check(launcher_invite_write(root, "halo:join/no-slashes") == 0, "halo: without // is refused");
	check(launcher_invite_write(root, "") == 0, "an empty link is refused");
	check(launcher_invite_write(root, NULL) == 0, "no link is refused");
	check(launcher_invite_write(NULL, "halo://join/abc") == 0, "no data root is refused");
	check(launcher_invite_write("", "halo://join/abc") == 0, "an empty data root is refused");
	text = slurp(path);
	check(text && strcmp(text, "halo://join/second\n") == 0, "a refused link left the last one");

	/* a link carrying a line of its own cannot be used to write two */
	check(launcher_invite_write(root, "halo://join/a\nhalo://join/b") == 0, "a link with a newline is refused");

	/* a data root that cannot be written is a failure, not a crash */
	check(launcher_invite_write("/no/such/folder/here", "halo://join/abc") == 0,
		"an unwritable data root is refused");

	remove(path);
	if (failures)
		printf("%d check(s) failed\n", failures);
	else
		printf("launcher invite: all checks passed\n");
	return failures ? 1 : 0;
}
