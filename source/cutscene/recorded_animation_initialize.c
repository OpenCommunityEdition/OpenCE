/*
RECORDED_ANIMATION_INITIALIZE.C

symbols in this file:
00081D80 0070:
	_recorded_animation_byteswap_unit_control (0000)
00081DF0 0090:
	_recorded_animation_initialize_unit_control (0000)
00081E80 0070:
	_recorded_animation_write_unit_control (0000)
00258B44 000e:
	??_C@_0O@FLAIBBAO@real_vector3d?$AA@ (0000)
00258B54 000e:
	??_C@_0O@FKMKHLDJ@real_vector2d?$AA@ (0000)
002DCE10 0110:
	_data_002dce10 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "memory/byte_swapping.h"
#include "math/real_math.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct recorded_unit_control
{
	byte control_flags;
	byte control_mode;
	short animation_state;
	short weapon_index;
	short version2_field;
	short version3_field;
	short pad;
	real_vector2d throttle;
	long reserved;
	long version1_field;
	real_vector3d facing;
	real_vector3d aiming;
	real_vector3d looking;
};

struct recorded_animation_control_field
{
	struct byte_swap_definition *definition;
	long size;
	long unit_control_offset;
};

struct recorded_animation_layout_data
{
	byte_swap_code real_vector2d_codes[2];
	struct byte_swap_definition real_vector2d_definition;
	byte_swap_code real_vector3d_codes[3];
	struct byte_swap_definition real_vector3d_definition;
	long version_table_alignment;
	struct recorded_animation_control_field version0_fields[10];
	struct recorded_animation_control_field version1_fields[2];
	struct recorded_animation_control_field version2_fields[2];
	struct recorded_animation_control_field version3_fields[2];
	struct recorded_animation_control_field *fields_by_version[4];
};

/* ---------- prototypes */

extern struct byte_swap_definition byte_bs_definition;
extern struct byte_swap_definition word_bs_definition;
extern struct byte_swap_definition long_bs_definition;

/* ---------- globals */

struct recorded_animation_layout_data data_002dce10 =
{
	{ _4byte, _4byte },
	{ "real_vector2d", sizeof(real_vector2d), data_002dce10.real_vector2d_codes, BYTE_SWAP_DEFINITION_SIGNATURE, FALSE },
	{ _4byte, _4byte, _4byte },
	{ "real_vector3d", sizeof(real_vector3d), data_002dce10.real_vector3d_codes, BYTE_SWAP_DEFINITION_SIGNATURE, FALSE },
	0, /* the original independently declared version table begins on an 8-byte data boundary */
	{
		{ &byte_bs_definition, sizeof(byte), offsetof(struct recorded_unit_control, control_flags) },
		{ &byte_bs_definition, sizeof(byte), offsetof(struct recorded_unit_control, control_mode) },
		{ &word_bs_definition, sizeof(short), offsetof(struct recorded_unit_control, animation_state) },
		{ &word_bs_definition, sizeof(short), offsetof(struct recorded_unit_control, weapon_index) },
		{ &word_bs_definition, sizeof(short), NONE },
		{ &data_002dce10.real_vector2d_definition, sizeof(real_vector2d), offsetof(struct recorded_unit_control, throttle) },
		{ &data_002dce10.real_vector3d_definition, sizeof(real_vector3d), offsetof(struct recorded_unit_control, facing) },
		{ &data_002dce10.real_vector3d_definition, sizeof(real_vector3d), offsetof(struct recorded_unit_control, aiming) },
		{ &data_002dce10.real_vector3d_definition, sizeof(real_vector3d), offsetof(struct recorded_unit_control, looking) },
		{ NULL, NONE, NONE },
	},
	{
		{ &long_bs_definition, sizeof(long), offsetof(struct recorded_unit_control, version1_field) },
		{ NULL, NONE, NONE },
	},
	{
		{ &word_bs_definition, sizeof(short), offsetof(struct recorded_unit_control, version2_field) },
		{ NULL, NONE, NONE },
	},
	{
		{ &word_bs_definition, sizeof(short), offsetof(struct recorded_unit_control, version3_field) },
		{ NULL, NONE, NONE },
	},
	{
		data_002dce10.version0_fields,
		data_002dce10.version1_fields,
		data_002dce10.version2_fields,
		data_002dce10.version3_fields,
	},
};

/* ---------- public code */

void recorded_animation_byteswap_unit_control(byte **stream, byte unit_control_data_version)
{
	short version_index;

	for (version_index = 0; version_index < MAX(unit_control_data_version, 1); version_index++)
	{
		struct recorded_animation_control_field *field = data_002dce10.fields_by_version[version_index];

		while (field->size != NONE)
		{
			byte_swap_data(field->definition, *stream, 1);
			*stream += field->size;
			field++;
		}
	}
}

/* ---------- private code */
