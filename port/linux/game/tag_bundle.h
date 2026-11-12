/*
TAG_BUNDLE.H

Tag bundles: a tag of a map and every tag it names, taken out of the map
whole, for this build to add to other maps (docs/custom_edition_caches.md,
"Tag bundles"). A bundle holds the tags as the map held them once loaded,
converted and checked (custom_edition_cache_load, custom_edition_cache_convert,
tag_validate_custom_edition_tags), with every pointer made an offset in the
bundle and every tag a reference names made a tag of the bundle, by the
schemas the validator checks them with (tag_schema.h). Nothing is changed but
the pointers and the tags named: a bundle's tags are a Custom Edition map's,
and are read as the game reads one's.

The file, all of it little-endian:

	header (TAG_BUNDLE_HEADER_BYTES)
	tags (tag_count of TAG_BUNDLE_TAG_BYTES): the first is the one taken out
	names: each tag's path, terminated, and the map's name and build
	data: each tag's root, then its blocks' elements and its data, as the
	  schema walks them, each start 4-aligned
	file data: bitmap pixels, sound samples and model vertices and strips,
	  which the map kept in its files

In data, a block's address and a data's address are offsets in data (an
empty one's 0), and their definitions 0; a reference's index and a tag
index are indices of the bundle's tags (NONE for none), and a reference's
name the offset in names of its tag's path (NONE for none); a file data's
file offset is an offset in file data. A gbxmodel part's vertex and strip
offsets ('mod2', which name where they are in a map's model data) are
offsets in file data, and so is a bitmap's pixels offset (its pixels_size
bytes of pixels, laid out as Halo PC's are).

Version 1 bundles held no bitmap pixels; they are refused.
*/

#ifndef __TAG_BUNDLE_H
#define __TAG_BUNDLE_H
#pragma once

/* ---------- constants */

/* "FABB" */
#define TAG_BUNDLE_SIGNATURE 0x42424146UL
#define TAG_BUNDLE_VERSION 2

enum
{
	TAG_BUNDLE_HEADER_BYTES = 0x40,
	/* the header's fields: unsigned longs */
	TAG_BUNDLE_HEADER_SIGNATURE_OFFSET = 0x00,
	TAG_BUNDLE_HEADER_VERSION_OFFSET = 0x04,
	TAG_BUNDLE_HEADER_FLAGS_OFFSET = 0x08,
	TAG_BUNDLE_HEADER_TAG_COUNT_OFFSET = 0x0C,
	TAG_BUNDLE_HEADER_TAGS_OFFSET = 0x10,
	TAG_BUNDLE_HEADER_NAMES_OFFSET = 0x14,
	TAG_BUNDLE_HEADER_NAMES_SIZE_OFFSET = 0x18,
	TAG_BUNDLE_HEADER_DATA_OFFSET = 0x1C,
	TAG_BUNDLE_HEADER_DATA_SIZE_OFFSET = 0x20,
	TAG_BUNDLE_HEADER_FILE_DATA_OFFSET = 0x24,
	TAG_BUNDLE_HEADER_FILE_DATA_SIZE_OFFSET = 0x28,
	/* offsets in names */
	TAG_BUNDLE_HEADER_MAP_NAME_OFFSET = 0x2C,
	TAG_BUNDLE_HEADER_MAP_BUILD_OFFSET = 0x30,

	TAG_BUNDLE_TAG_BYTES = 0x20,
	/* a tag's fields: unsigned longs */
	TAG_BUNDLE_TAG_GROUP_OFFSET = 0x00,
	TAG_BUNDLE_TAG_PARENT_GROUPS_OFFSET = 0x04,
	/* (offset in names) */
	TAG_BUNDLE_TAG_NAME_OFFSET = 0x0C,
	/* its root: offset in data, and size */
	TAG_BUNDLE_TAG_ROOT_OFFSET = 0x10,
	TAG_BUNDLE_TAG_ROOT_SIZE_OFFSET = 0x14,
	TAG_BUNDLE_TAG_FLAGS_OFFSET = 0x18,
	/* its index in the map it was taken from (for messages) */
	TAG_BUNDLE_TAG_MAP_INDEX_OFFSET = 0x1C,
};

/* the header's flags */
enum
{
	/* the tags are a Custom Edition map's (all bundles, for now) */
	_tag_bundle_custom_edition_bit = 0,
};

/* a tag's flags */
enum
{
	/* the map read it from a resource map (bitmaps.map and the like):
	Halo PC's own, as it shipped */
	_tag_bundle_tag_from_resource_map_bit = 0,
};

enum tag_bundle_status
{
	_tag_bundle_status_ok,
	/* not a tag of the map */
	_tag_bundle_status_no_such_tag,
	/* a tag of a group whose pointers no schema knows, or without data:
	problem_tag_index */
	_tag_bundle_status_unknown_group,
	/* a pointer of a tag not where the walk found the tags: problem_tag_index */
	_tag_bundle_status_bad_pointer,
	/* a bundle not laid out as tag_bundle_write lays one out */
	_tag_bundle_status_bad_bundle,
	_tag_bundle_status_too_large,
	_tag_bundle_status_out_of_memory,
	_tag_bundle_status_read_failed,
	_tag_bundle_status_write_failed,

	NUMBER_OF_TAG_BUNDLE_STATUSES
};

/* ---------- structures */

/* the tags of a Custom Edition map, as tag_validate_custom_edition_tags
passed them, at CUSTOM_EDITION_TAG_CACHE_ADDRESS, and its files */
struct tag_bundle_source
{
	void *tag_header;
	long loaded_size;
	/* reads size bytes at offset in the combined space of the map's files
	(custom_edition_cache_combine_resource_offsets: the map's own offsets
	first), returning FALSE unless every byte was read */
	boolean (*read)(void *context, unsigned long offset, unsigned long size, void *buffer);
	void *context;
	char const *map_name;
	char const *map_build;
};

/* a bundle tag_bundle_load checked: its parts, in the bundle's bytes */
struct tag_bundle_loaded
{
	long tag_count;
	/* (tag_count of TAG_BUNDLE_TAG_BYTES) */
	byte const *tags;
	char const *names;
	unsigned long names_size;
	byte *data;
	unsigned long data_size;
	/* (NULL when it was not loaded: tag_bundle_load_tags; where it is in the
	bundle's bytes either way) */
	byte const *file_data;
	unsigned long file_data_size;
	unsigned long file_data_offset;
	/* where the problem was: a tag of the bundle, or NONE */
	long problem_tag_index;
};

/* a region of a bundle's file data: a bitmap's pixels, a sound's samples, a
gbxmodel part's vertices or strip (offset: in file data) */
struct tag_bundle_file_region
{
	unsigned long offset;
	unsigned long size;
};

/* writes size bytes after those written before, returning FALSE unless they
were all written */
typedef boolean (*tag_bundle_write_proc)(
	void *context,
	void const *bytes,
	unsigned long size);

struct tag_bundle_report
{
	enum tag_bundle_status status;
	/* the tag the status is about, or NONE */
	long problem_tag_index;
	long tag_count;
	unsigned long data_size;
	unsigned long file_data_size;
	unsigned long size;
};

/* ---------- prototypes/TAG_BUNDLE.C */

char const *tag_bundle_status_describe(
	enum tag_bundle_status status);

/* The tags a bundle of tag tag_index (its absolute index) holds: that one,
then every tag it names, and every tag those name, in the order they are
first named. *tag_indices is a list of their absolute indices, which the
caller frees with free(), and *tag_count how many. */
enum tag_bundle_status tag_bundle_tags(
	struct tag_bundle_source const *source,
	long tag_index,
	long **tag_indices,
	long *tag_count,
	long *problem_tag_index);

/* Writes the bundle of tag tag_index (its absolute index) with write. */
enum tag_bundle_status tag_bundle_write(
	struct tag_bundle_source const *source,
	long tag_index,
	tag_bundle_write_proc write,
	void *write_context,
	struct tag_bundle_report *report);

/* Checks the bundle of size bytes at bundle (a file: untrusted), which must
be laid out exactly as tag_bundle_write lays one out, by the same schemas,
and makes its data's pointers this process's, in place: a block's and a
data's address point into its data, a reference's name into its names.
Tags (references' indices, tag indices) stay the bundle's, and file data
offsets (and gbxmodel parts' vertex and strip offsets) offsets in its file
data. Nothing in the bundle is used before it is checked. */
enum tag_bundle_status tag_bundle_load(
	byte *bundle,
	unsigned long size,
	struct tag_bundle_loaded *loaded);

/* Where a bundle's file data starts, by its header (TAG_BUNDLE_HEADER_BYTES
of it, unchecked): its bytes before it are its tags, which
tag_bundle_load_tags loads without the rest. */
unsigned long tag_bundle_file_data_offset(
	byte const *header);
/* As tag_bundle_load, of a bundle of `size` bytes whose first `tags_size`
alone are at bundle: all but its file data, which must start there. Its
file data is checked as laid out as tag_bundle_write lays it out, but not
read: its pixels, samples and vertices are read from the bundle's file as
they are wanted (tag_injector.c). */
enum tag_bundle_status tag_bundle_load_tags(
	byte *bundle,
	unsigned long tags_size,
	unsigned long size,
	struct tag_bundle_loaded *loaded);
/* As tag_bundle_load_tags, and the regions of its file data (those of
more than no bytes), in the order they lie: *regions, which the caller
frees with free() (NULL when there are none, or it fails), and how many. */
enum tag_bundle_status tag_bundle_load_tags_regions(
	byte *bundle,
	unsigned long tags_size,
	unsigned long size,
	struct tag_bundle_loaded *loaded,
	struct tag_bundle_file_region **regions,
	long *region_count);

#endif
