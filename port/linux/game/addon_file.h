/*
ADDON_FILE.H

The add-ons' file (docs/addons.md): a .cevehicle, a vehicle and the tags it
uses, made by tools/addon_build.c from a Custom Edition map and added to
the map a game plays by port/linux/game/addons.c. Every number is
little-endian; every offset counts from the file's start, unless it says
otherwise. Included by both, so it uses only <stdint.h>'s types.

The tags are as the Custom Edition loader leaves them once its conversion
and the validator have been through them (port/linux/game/cache_file_formats.c,
tag_validate.c): this build's layouts, but for gbxmodels, whose geometry is
in the model data and is made this build's as the add-on loads
(custom_edition_geometry.c). In the tag data, every pointer is an offset
within it and is listed among the relocations; a reference's or tag index's
tag is an index into the add-on's tags, or none and listed among the import
fields when it is the map's own. A bitmap's pixels and a sound's samples
are at offsets within the resources, a model part's vertices and strip
within the model data. An icon text index names its text, which the map's
list of them may number otherwise or lack.
*/

#ifndef __ADDON_FILE_H
#define __ADDON_FILE_H
#pragma once

/* ---------- headers */

#include <stdint.h>

/* ---------- constants */

/* "cevh", as its first four bytes read */
#define ADDON_FILE_SIGNATURE 0x68766563UL
#define ADDON_FILE_VERSION 1

enum addon_file_kind
{
	_addon_file_kind_vehicle = 1,
	_addon_file_kind_weapon,
};

enum
{
	ADDON_FILE_HEADER_BYTES = 0x80,
	/* a seat's label and an animation's name (the tags' strings) */
	ADDON_FILE_STRING_BYTES = 32,
	/* an import field's index when it is a tag reference, not a tag index
	alone (its group and name are the reference's own) */
	ADDON_FILE_IMPORT_REFERENCE_BIT = 0x80000000UL,
};

/* ---------- structures */

struct addon_file_section
{
	uint32_t offset;
	/* its elements, or its bytes for one of bytes */
	uint32_t count;
};

struct addon_file_header
{
	uint32_t signature;
	uint32_t version;
	uint32_t kind;
	uint32_t file_size;
	/* the vehicle's tag, among the tags */
	uint32_t root_tag;
	struct addon_file_section tags;				/* addon_file_tag */
	struct addon_file_section imports;			/* addon_file_import */
	struct addon_file_section tag_data;			/* bytes */
	struct addon_file_section relocations;		/* uint32_t: offsets in the tag data */
	struct addon_file_section import_fields;	/* addon_file_import_field */
	struct addon_file_section resources;		/* bytes */
	struct addon_file_section model_data;		/* bytes: vertices, then strips */
	/* where the strips start in the model data */
	uint32_t model_index_data_offset;
	struct addon_file_section seats;			/* addon_file_seat */
	struct addon_file_section seat_animations;	/* addon_file_seat_animation */
	struct addon_file_section icon_texts;		/* addon_file_icon_text */
	struct addon_file_section texts;			/* bytes: the icon texts' */
	/* a weapon's: the map's HUD globals' messaging icons, its tag among the
	tags plus one (0: none, the playing map's own), which its HUD interface's
	messaging icon (its pickup's picture) is a sprite of */
	uint32_t messaging_icons;
	/* the Halo PC behaviours (cache_file_formats.h's enum
	custom_edition_behaviour, as bits) of the map it was made from, which its
	bitmaps' channels follow (a meter's of bigass_open_v1.1's are the Xbox's) */
	uint32_t behaviours;
	uint32_t reserved[2];
};

/* one of the add-on's tags: its groups, and its name and root in the tag
data */
struct addon_file_tag
{
	uint32_t group_tag;
	uint32_t parent_group_tags[2];
	uint32_t name_offset;
	uint32_t root_offset;
};

/* one of the map's tags that the add-on's name: its group, and its name in
the tag data */
struct addon_file_import
{
	uint32_t group_tag;
	uint32_t name_offset;
};

/* a tag reference's or tag index's field in the tag data that names an
import (ADDON_FILE_IMPORT_REFERENCE_BIT and the import's index) */
struct addon_file_import_field
{
	uint32_t offset;
	uint32_t import;
};

/* a seat the riders' animation graphs need (a unit's seats name their
animations by label): its element of an animation graph's unit seats, in
the tag data, whose animations are named by the seat's animations */
struct addon_file_seat
{
	char label[ADDON_FILE_STRING_BYTES];
	/* the element, as the tag data has it */
	uint32_t element_offset;
	/* its animations: the first, and how many */
	uint32_t first_animation;
	uint32_t animation_count;
};

/* an animation index in a seat's element (or its weapon classes' or
types'), at offset in the tag data, and the animation it names */
struct addon_file_seat_animation
{
	uint32_t offset;
	char name[ADDON_FILE_STRING_BYTES];
};

/* an object's or a seat's icon text index (the HUD's alternate icon text:
what the game names a vehicle and its seats by as a player gets in), at
offset in the tag data, and the text it names, UTF-16 with its terminator,
at text_offset in the texts (text_bytes of them) */
struct addon_file_icon_text
{
	uint32_t offset;
	uint32_t text_offset;
	uint32_t text_bytes;
};

/* (the sizes both sides read) */
typedef char verify_addon_file_header_size[sizeof(struct addon_file_header) == ADDON_FILE_HEADER_BYTES ? 1 : -1];
typedef char verify_addon_file_tag_size[sizeof(struct addon_file_tag) == 20 ? 1 : -1];
typedef char verify_addon_file_seat_size[sizeof(struct addon_file_seat) == 44 ? 1 : -1];
typedef char verify_addon_file_seat_animation_size[sizeof(struct addon_file_seat_animation) == 36 ? 1 : -1];
typedef char verify_addon_file_icon_text_size[sizeof(struct addon_file_icon_text) == 12 ? 1 : -1];

#endif // __ADDON_FILE_H
