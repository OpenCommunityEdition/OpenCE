/*
TAG_IMPORT.H

The tag importer (tag_import.c): tags another map has that the loaded level
has not, copied into it from a tag pack (tag_pack.h) with every tag they
name, and appended to its tag table, the same on every machine that asks
for the same roots in the same order. port/linux/TAG_INJECTION.md tells how
to use it.

A level's import, as it loads (level_tag_import.c, which calls each user
in turn):

	if (tag_import_begin())
	{
		tag_import_roots("opence_floodfection", "c40", roots, count);
		...
		tag_import_run();
		tag_import_end();
	}

and tag_import_level_unloaded as it goes. A tag the level has already is
its own, not copied; one asked for from two maps is copied once, from the
map asked first.
*/

#ifndef __TAG_IMPORT_H
#define __TAG_IMPORT_H

/* a tag to copy, by its group and name */
struct tag_import_root
{
	long group;
	char const *name;
};

/* a map of a pack, open to be copied from */
struct tag_import_donor;

/* the lists a level's copying gathers into: 0 when there is no room. Each
level's import is one begin, the roots, one run and one end */
int tag_import_begin(void);
/* these roots of a pack's map, each with what it needs, or the roots the
level has already. The map is read from its pack, never itself: a pack not
written is logged and nothing is copied from it. How many roots were found */
int tag_import_roots(char const *pack, char const *map, struct tag_import_root const *roots, int count);
/* the tags wanted copied and appended to the table */
void tag_import_run(void);
/* the lists let go (the copies stay, with the level) */
void tag_import_end(void);
/* the copies let go, and the level's own table back, as the level goes */
void tag_import_level_unloaded(void);
/* what the level's import read, for the log */
void tag_import_log(char const *who);

/* a pack's map opened for the level's import, to choose its roots by its
tags (a pool of every weapon of a map): with tag_import_want, then
tag_import_release when done with its bytes. NULL when it cannot be read */
struct tag_import_donor *tag_import_open(char const *pack, char const *map);
int tag_import_want(struct tag_import_donor *donor, long absolute);
void tag_import_release(struct tag_import_donor *donor);

/* a pack's map opened to look at, outside a level (the menus), read from
the map itself when the pack is not written. Closed with tag_import_peek_close */
struct tag_import_donor *tag_import_peek(char const *pack, char const *map);
void tag_import_peek_close(struct tag_import_donor *donor);

/* a donor's tags: how many, each one's group, name (NULL when its name is
not in its tags) and its data, when `bytes` of it are in them (NULL
otherwise); its scenario's (NONE) */
long tag_import_tag_count(struct tag_import_donor *donor);
long tag_import_tag_group(struct tag_import_donor *donor, long absolute);
char const *tag_import_tag_name(struct tag_import_donor *donor, long absolute);
void *tag_import_tag_data(struct tag_import_donor *donor, long absolute, unsigned long bytes);
long tag_import_scenario(struct tag_import_donor *donor);
/* a pointer of a donor's tags as this process reads it, when `bytes` from
there are in them; NULL otherwise. And a string (a reference's name) of at
most 255 letters */
void *tag_import_address(struct tag_import_donor *donor, void const *pointer, unsigned long bytes);
char const *tag_import_string(struct tag_import_donor *donor, void const *pointer);
char const *tag_import_donor_map(struct tag_import_donor *donor);

/* a bitmap's or sound's bytes of an imported tag, from its pack. 0: this
tag is the open map's, and the caller reads that */
int tag_import_donor_read(long tag_index, long offset, long size, void *buffer);
/* whether a tag is one copied in (its bitmaps' pixels are read at once:
xbox_texture_cache.c) */
int tag_import_tag_imported(long tag_index);
/* the level's tag table: how many tags, and a hash of each one's group and
name in order, which a network game's machines must have alike */
void tag_import_tag_table_fingerprint(long *count, unsigned long *hash);

#endif /* __TAG_IMPORT_H */
