/*
LEVEL_TAG_IMPORT.C

A level's import of tags from other maps (tag_import.h), as its tags load
(cache_files.c): each user's roots, in a fixed order, so that every machine
of a network game appends the same tags at the same indices. OpenCE has no
user yet. port/linux/TAG_INJECTION.md tells how to add one.
*/

#include "cseries.h"
#include "cseries/errors.h"

#include "tag_import.h"

void level_tag_import_unloaded(
	void)
{
	tag_import_level_unloaded();

	return;
}

void level_tag_import_loaded(
	char const *scenario_name)
{
	/* (whether a user wants tags in this level: none yet) */
	boolean wanted = FALSE;
	long count;
	unsigned long hash;

	level_tag_import_unloaded();
	if (!wanted || !tag_import_begin())
		return;
	tag_import_run();
	tag_import_end();
	tag_import_log("level");
	/* (the table as it is now, which a network game's machines must all
	have alike) */
	tag_import_tag_table_fingerprint(&count, &hash);
	error(_error_silent, "level: %s's tag table: %ld tags, fingerprint %08lx", scenario_name, count, hash);

	return;
}
