/* ---------- headers */

#include "cseries.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "memory/byte_swapping.h"
#include "memory/data.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

static void hs_syntax_data_byte_swap(
	void *owner,
	void *data,
	long size);

static struct tag_field hs_script_fields[6];
static struct tag_field hs_global_fields[7];
static unsigned long hs_reference_group_tags[3];
static struct tag_field hs_reference_fields[3];
static struct tag_field hs_source_fields[3];
static byte_swap_code hs_data_array_codes[14];
static struct byte_swap_definition hs_data_array_definition;
static byte_swap_code hs_syntax_node_codes[10];
static struct byte_swap_definition hs_syntax_node_definition;

/* ---------- globals */

struct tag_enum_definition hs_script_types_enum =
{
	5,
	hs_script_type_names,
	NULL,
};

struct tag_enum_definition hs_types_enum =
{
	49,
	hs_type_names,
	NULL,
};

static struct tag_field hs_script_fields[6] =
{
	{ _tag_field_string, 0, "name*", NULL },
	{ _tag_field_enum, 0, "script type*", &hs_script_types_enum },
	{ _tag_field_enum, 0, "return type*", &hs_types_enum },
	{ _tag_field_long_integer, 0, "root expression index*", NULL },
	{ _tag_field_pad, 0, NULL, (void *)52 },
	{ _tag_field_terminator, 0, NULL, NULL },
};

struct tag_block_definition hs_scripts_block =
{
	"hs_scripts_block",
	0,
	512,
	sizeof(struct hs_script),
	NULL,
	hs_script_fields,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static struct tag_field hs_global_fields[7] =
{
	{ _tag_field_string, 0, "name*", NULL },
	{ _tag_field_enum, 0, "type*", &hs_types_enum },
	{ _tag_field_pad, 0, NULL, (void *)2 },
	{ _tag_field_pad, 0, NULL, (void *)4 },
	{ _tag_field_long_integer, 0, "initialization expression index*", NULL },
	{ _tag_field_pad, 0, NULL, (void *)48 },
	{ _tag_field_terminator, 0, NULL, NULL },
};

struct tag_block_definition hs_globals_block =
{
	"hs_globals_block",
	0,
	128,
	sizeof(struct hs_global),
	NULL,
	hs_global_fields,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static unsigned long hs_reference_group_tags[3] = { 0, NONE, 0 };

static struct tag_field hs_reference_fields[3] =
{
	{ _tag_field_pad, 0, NULL, (void *)24 },
	{ _tag_field_tag_reference, 0, "reference*^", hs_reference_group_tags },
	{ _tag_field_terminator, 0, NULL, NULL },
};

struct tag_block_definition hs_references_block =
{
	"hs_references_block",
	0,
	256,
	sizeof(struct hs_reference),
	NULL,
	hs_reference_fields,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

struct tag_data_definition hs_source_data_definition =
{
	"hs_source_data_definition",
	0,
	0x40000,
	NULL,
};

static struct tag_field hs_source_fields[3] =
{
	{ _tag_field_string, 0, "name*", NULL },
	{ _tag_field_data, 0, "source", &hs_source_data_definition },
	{ _tag_field_terminator, 0, NULL, NULL },
};

struct tag_block_definition hs_source_files_block =
{
	"hs_source_files_block",
	0,
	8,
	sizeof(struct hs_source_file),
	NULL,
	hs_source_fields,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static byte_swap_code hs_data_array_codes[14] =
{
	_begin_bs_array,
	1,
	32,
	_2byte,
	_2byte,
	1,
	3,
	_4byte,
	_2byte,
	_2byte,
	_2byte,
	_2byte,
	_4byte,
	_end_bs_array,
};

static struct byte_swap_definition hs_data_array_definition =
{
	"data_array_header",
	sizeof(struct data_array),
	hs_data_array_codes,
	BYTE_SWAP_DEFINITION_SIGNATURE,
	FALSE,
};

static byte_swap_code hs_syntax_node_codes[10] =
{
	_begin_bs_array,
	1,
	_2byte,
	_2byte,
	_2byte,
	_2byte,
	_4byte,
	_4byte,
	_4byte,
	_end_bs_array,
};

static struct byte_swap_definition hs_syntax_node_definition =
{
	"syntax_node",
	sizeof(struct hs_syntax_node),
	hs_syntax_node_codes,
	BYTE_SWAP_DEFINITION_SIGNATURE,
	FALSE,
};

struct tag_data_definition hs_syntax_data_definition =
{
	"hs_syntax_data_definition",
	0,
	380076,
	hs_syntax_data_byte_swap,
};

struct tag_data_definition hs_string_data_definition =
{
	"hs_string_data_definition",
	0,
	0x40000,
	NULL,
};

/* ---------- public code */

static void hs_syntax_data_byte_swap(
	void *owner,
	void *data,
	long size)
{
	long data_size;

	if (size)
	{
		match_assert("c:\\halo\\SOURCE\\hs\\hs_scenario_definitions.c", 109,
			size>=sizeof(struct data_array));

		if (!memcmp(data, "csirtpn \0edo", 12))
		{
			byte_swap_data(&hs_data_array_definition, data, 1);
			byte_swap_data(&hs_syntax_node_definition, data, 0x4000);
			return;
		}

		data_size= size-sizeof(struct data_array);
		match_assert("c:\\halo\\SOURCE\\hs\\hs_scenario_definitions.c", 121,
			data_size>=0 && (data_size%sizeof(struct hs_syntax_node))==0);

		if (data_size>=0 && (data_size%sizeof(struct hs_syntax_node))==0)
		{
			long syntax_node_count= data_size/sizeof(struct hs_syntax_node);

			byte_swap_data(&hs_data_array_definition, data, 1);
			data= (byte *)data+sizeof(struct data_array);
			byte_swap_data(&hs_syntax_node_definition, data, syntax_node_count);
		}
	}

	return;
}
