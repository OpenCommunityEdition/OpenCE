/*
TAG_NAMES.H

Names made for the tags of a Custom Edition map that have none of their own
(tag_names.c): a protected map's, whose paths were taken out of it
("<protected>", nothing, or one path many tags share), for the Tag Injector
to list them and take them out by (tag_injector.c, tools/ce_tags.c).
*/

#ifndef __TAG_NAMES_H
#define __TAG_NAMES_H
#pragma once

/* ---------- prototypes/TAG_NAMES.C */

/* Gives each tag of a Custom Edition map's tag table (`tag_header`, as
tag_validate_custom_edition_tags passed it) without a name of its own one
made from what it is and what names it, under the map's name
("descent-quarantine\weapons\battle rifle\battle rifle"): its tag table's
names point at them, in memory the next tag_names_restore that makes names
or tag_names_dispose lets go. `made`, when not NULL, is given a byte a tag
(TRUE: its name was made), which the caller frees with free(). How many were
made; NONE when there was no memory for them (the names are as they were). */
long tag_names_restore(
	void *tag_header,
	char const *map_name,
	unsigned char **made);
void tag_names_dispose(
	void);

#endif
