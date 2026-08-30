/*
AI_PROFILE.C

symbols in this file:
00041C90 0020:
	_ai_profile_initialize (0000)
00041CB0 0010:
	_ai_profile_dispose (0000)
00041CC0 0020:
	_ai_profile_initialize_for_new_map (0000)
00041CE0 0010:
	_ai_profile_dispose_from_old_map (0000)
00041CF0 0110:
	_ai_profile_update (0000)
00041E00 0070:
	_ai_profile_display (0000)
00041E70 0090:
	_code_00041e70 (0000)
00041F00 0040:
	_ai_profile_change_render_spray (0000)
00041F40 0010:
	_code_00041f40 (0000)
00041F50 0010:
	_code_00041f50 (0000)
00041F60 0050:
	_code_00041f60 (0000)
00041FB0 0010:
	_code_00041fb0 (0000)
00041FC0 0010:
	_code_00041fc0 (0000)
00041FD0 0050:
	_code_00041fd0 (0000)
00042020 0010:
	_code_00042020 (0000)
00042030 0060:
	_code_00042030 (0000)
00042090 0070:
	_code_00042090 (0000)
00042100 0060:
	_code_00042100 (0000)
00042160 0090:
	_code_00042160 (0000)
000421F0 0070:
	_code_000421f0 (0000)
00042260 0060:
	_code_00042260 (0000)
000422C0 0150:
	_code_000422c0 (0000)
00042410 0080:
	_ai_profile_render (0000)
0024BBC8 0012:
	??_C@_0BC@FECLANMB@activation?5status?$AA@ (0000)
0024BBE0 0059:
	??_C@_0FJ@HFLBIAGN@?$CImeter?9?$DOhistory_next_index?5?$DO?$DN?50?$CJ@ (0000)
0024BC3C 001e:
	??_C@_0BO@JBPKCAGJ@definition?9?$DOmeter_id?5?$DN?$DN?5index?$AA@ (0000)
0024BC5C 001f:
	??_C@_0BP@IDLEFHAL@c?3?2halo?2SOURCE?2ai?2ai_profile?4c?$AA@ (0000)
0024BC80 004f:
	??_C@_0EP@EALMABPN@ai?5enc?5?$CF?52d?1?$CF?53d?0?5actor?5?$CF?53d?1?$CF?53@ (0000)
0024BCD0 0012:
	??_C@_0BC@LJCLCOPD@AI?5line?9spray?3?5?$CFs?$AA@ (0000)
0024BCE4 001e:
	??_C@_0BO@PNNEPAEO@encounters?5?$CFd?1?$CFd?$HMtprops?5?$CFd?1?$CFd?$AA@ (0000)
0024BD04 001e:
	??_C@_0BO@GPBPAFNH@actors?5?$CFd?1?$CFd?1?$CFd?$HMunits?5?$CFd?1?$CFd?$CFd?$AA@ (0000)
0024BD24 0022:
	??_C@_0CC@KKENFKIE@swarms?5?$CFd?1?$CFd?1?$CFd?$HMtcomponents?5?$CFd?1?$CF@ (0000)
0024BD48 003d:
	??_C@_0DN@KDGEGKJF@props?5a?1o?1u?3?$HMtenemy?5?$CFd?1?$CFd?1?$CFd?$HMtfr@ (0000)
0024BD88 003d:
	??_C@_0DN@CMELKBKH@collisions?5?$CFd?$HMtlineofsight?5?$CFd?$HMtl@ (0000)
0024BDC8 002e:
	??_C@_0CO@HHCJMMCK@path_flood?5?$CFd?$HMtpath_find?5?$CFd?$HMtact@ (0000)
002B7458 00ec:
	_global_ai_meter_definitions (0000)
*/

/* ---------- headers */

#include "ai/ai_profile.h"

#include "cseries/cseries.h"

#include "ai/actors.h"
#include "ai/encounters.h"
#include "ai/props.h"

#include "main/console.h"
#include "memory/data.h"

/* ---------- constants */

enum
{
	NUMBER_OF_AI_METERS = 28,
	NUMBER_OF_AI_RENDER_SPRAYS = 3,
};

/* ---------- macros */

/* ---------- structures */

struct ai_profile_globals
{
	short __unknown0;
	short render_spray;
	boolean enabled;
	byte __unknown5[7];
	byte map_data[0xEE0];
};

typedef short (*ai_meter_sample_proc)(
	void);

struct ai_meter_definition
{
	short meter_id;
	short __unknown2;
	ai_meter_sample_proc sample_proc;
};

struct ai_meter_definitions
{
	struct ai_meter_definition meters[NUMBER_OF_AI_METERS];
	char const *render_spray_names[NUMBER_OF_AI_RENDER_SPRAYS];
};

/* ---------- prototypes */

/* ---------- globals */

struct ai_profile_globals ai_profile;
extern struct ai_meter_definitions global_ai_meter_definitions;

/* ---------- public code */

void ai_profile_initialize(
	void)
{
	csmemset(&ai_profile, 0, sizeof(ai_profile));
	ai_profile.enabled = TRUE;
	return;
}

void ai_profile_initialize_for_new_map(
	void)
{
	csmemset(ai_profile.map_data, 0, sizeof(ai_profile.map_data));
	return;
}

void ai_profile_dispose(
	void)
{
	return;
}

void ai_profile_dispose_from_old_map(
	void)
{
	return;
}

short ai_profile_change_render_spray(
	void)
{
	ai_profile.render_spray =
		(ai_profile.render_spray + 1) % NUMBER_OF_AI_RENDER_SPRAYS;
	console_printf(
		FALSE,
		"AI line-spray: %s",
		global_ai_meter_definitions.render_spray_names[ai_profile.render_spray]);

	return ai_profile.render_spray;
}

short code_00041f40(
	void)
{
	return encounter_data->actual_count;
}

short code_00041f50(
	void)
{
	return actor_data->actual_count;
}

short code_00041fc0(
	void)
{
	return swarm_data->actual_count;
}

short code_00041fb0(
	void)
{
	return prop_data->actual_count;
}

short code_00042020(
	void)
{
	return swarm_component_data->actual_count;
}

/* ---------- private code */
