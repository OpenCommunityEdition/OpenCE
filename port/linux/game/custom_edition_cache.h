/*
CUSTOM_EDITION_CACHE.H

What the native builds' cache file loader (source/cache/cache_files.c and
cache_files_windows.c) does with Halo Custom Edition caches, OpenSauce ".yelo"
caches among them: it finds them, says what they are, and refuses them, since
their tags are laid out for Halo PC and not for this build. The format itself
is read by cache_file_formats.c; docs/custom_edition_caches.md describes what
is and is not supported.
*/

#ifndef __CUSTOM_EDITION_CACHE_H
#define __CUSTOM_EDITION_CACHE_H

/* ---------- prototypes */

/* When `header` (CACHE_FILE_HEADER_BYTES bytes, with `build` its build
string field) is a Custom Edition cache header, logs what it is, asserts when
`fatal` as cache_file_header_verify does, and returns TRUE. */
boolean custom_edition_cache_refuse(
	void const *header,
	char const *build,
	char const *scenario_name,
	boolean fatal);

/* When the ".map" file `path` names does not exist but an OpenSauce ".yelo"
file of the same name does, makes `path` (`path_size` characters) name that
instead. */
void opensauce_cache_path_find(
	char *path,
	long path_size);

#endif
