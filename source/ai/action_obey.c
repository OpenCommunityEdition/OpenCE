/*
ACTION_OBEY.C

symbols in this file:
00004F70 0040:
	_code_00004f70 (0000)
00004FB0 0230:
	_code_00004fb0 (0000)
000051E0 0070:
	_code_000051e0 (0000)
00005250 0040:
	_code_00005250 (0000)
00005290 0050:
	_code_00005290 (0000)
000052E0 0020:
	_code_000052e0 (0000)
00005300 0050:
	_code_00005300 (0000)
00005350 0130:
	_code_00005350 (0000)
00005480 0180:
	_action_obey_command_list_setup (0000)
00005600 0070:
	_action_obey_flush_command_indices (0000)
00005670 0030:
	_action_obey_flush_structure_indices (0000)
000056A0 0030:
	_action_obey_advance_command_list (0000)
000056D0 0030:
	_action_obey_begin (0000)
00005700 0030:
	_action_obey_end (0000)
00005730 0790:
	_action_obey_describe_command (0000)
00005EC0 0020:
	_vector_from_points2d (0000)
00005EE0 0040:
	_cross_product3d (0000)
00005F20 0030:
	_negate_vector3d (0000)
00005F50 0020:
	_random_range (0000)
00005F70 0150:
	_code_00005f70 (0000)
000060C0 10e0:
	_code_000060c0 (0000)
000071A0 0580:
	_code_000071a0 (0000)
00007720 0120:
	_code_00007720 (0000)
00007840 0050:
	_code_00007840 (0000)
00007890 0030:
	_action_obey_update (0000)
000078C0 00c0:
	_action_obey_perform (0000)
00007980 03e0:
	_action_obey_control (0000)
00243270 003c:
	??_C@_0DM@JNBLNJLG@?$CFs?3?5command?5list?5?$CFs?5is?5stuck?5loo@ (0000)
002432AC 0036:
	??_C@_0DG@FLEHDLBK@?$CFs?3?5command?5list?5?$CFs?5entry?5?$CD?$CFd?5tr@ (0000)
002432E4 0014:
	??_C@_0BE@EJBCJKCJ@targeting_reference?$AA@ (0000)
002432F8 0020:
	??_C@_0CA@HHCNJNKK@c?3?2halo?2SOURCE?2ai?2action_obey?4c?$AA@ (0000)
00243318 003e:
	??_C@_0DO@JCOPLIPB@?$CBactor?9?$DOmeta?4swarm?5?$HM?$HM?5?$CIactor?9?$DOme@ (0000)
00243358 0034:
	??_C@_0DE@LLDIJCBD@wrong?5structure?5bsp?0?5cannot?5exec@ (0000)
00243390 0044:
	??_C@_0EE@KEBHPAE@swarm?5actor?5?$CFs?5cannot?5execute?5co@ (0000)
002433D4 000d:
	??_C@_0N@CEGJDJND@?$DMunknown?5?$CFd?$DO?$AA@ (0000)
002433E4 0004:
	??_C@_03DFHDCPPK@die?$AA@ (0000)
002433E8 0014:
	??_C@_0BE@OLHBIEN@teleport?5to?5?$CIp?$CFd?$CJ?$CFs?$AA@ (0000)
002433FC 0013:
	??_C@_0BD@OIBANJLL@?5and?5face?5at?5?$CIp?$CFd?$CJ?$AA@ (0000)
0024340F 0001:
	??_C@_00CNPNBAHC@?$AA@ (0000)
00243410 0010:
	??_C@_0BA@MGBBCPDG@set?5radius?5?$CF?42f?$AA@ (0000)
00243420 000f:
	??_C@_0P@KENFEEEA@loop?5to?5?$CD?$CFd?5?$CFs?$AA@ (0000)
00243430 0012:
	??_C@_0BC@OEJHODGH@loop?5to?5?$DMnone?$DO?5?$CFs?$AA@ (0000)
00243444 001b:
	??_C@_0BL@DGGHLGE@only?5until?5told?5to?5advance?$AA@ (0000)
00243460 0007:
	??_C@_06KHDOHNKC@always?$AA@ (0000)
00243468 0008:
	??_C@_07KIIGOGND@wait?5?$CFs?$AA@ (0000)
00243470 0016:
	??_C@_0BG@LEMJPAIB@until?5told?5to?5advance?$AA@ (0000)
00243488 0014:
	??_C@_0BE@DKOCLLMJ@until?5visible?5enemy?$AA@ (0000)
0024349C 000e:
	??_C@_0O@NBMOMCME@until?5alerted?$AA@ (0000)
002434AC 000e:
	??_C@_0O@EFALGMLH@initiative?5?$CFs?$AA@ (0000)
002434BC 000a:
	??_C@_09DEIMKGFJ@action?5?$CFs?$AA@ (0000)
002434C8 000f:
	??_C@_0P@OKJMMNMD@vehicle?9scared?$AA@ (0000)
002434D8 000f:
	??_C@_0P@HOGHMCM@vehicle?9woohoo?$AA@ (0000)
002434E8 000b:
	??_C@_0L@JFPBJIOJ@dive?9right?$AA@ (0000)
002434F4 000a:
	??_C@_09GLAJHPNM@dive?9left?$AA@ (0000)
00243500 000a:
	??_C@_09JAMKJKJP@dive?9back?$AA@ (0000)
0024350C 0009:
	??_C@_08HHGPGIIN@dive?9fwd?$AA@ (0000)
00243518 000c:
	??_C@_0M@FCCJKDHL@evade?9right?$AA@ (0000)
00243524 000b:
	??_C@_0L@GEJKFJKH@evade?9left?$AA@ (0000)
00243530 000e:
	??_C@_0O@MHCNGAHE@surprise?9back?$AA@ (0000)
00243540 000f:
	??_C@_0P@OMDAPGKA@surprise?9front?$AA@ (0000)
00243550 0008:
	??_C@_07PEIBNLKE@berserk?$AA@ (0000)
00243558 000d:
	??_C@_0N@PMOONPD@targeting?5?$CFs?$AA@ (0000)
00243568 000c:
	??_C@_0M@FBODOLG@vocalize?5?$CFs?$AA@ (0000)
00243574 0012:
	??_C@_0BC@JABMHOI@play?5recording?5?$CFs?$AA@ (0000)
00243588 000b:
	??_C@_0L@HCEEAAG@animate?5?$CFs?$AA@ (0000)
00243594 000d:
	??_C@_0N@EHHELJPO@script?5?$CFs?5?$CFs?$AA@ (0000)
002435A4 0005:
	??_C@_04OKGDLNCL@NONE?$AA@ (0000)
002435AC 0012:
	??_C@_0BC@EGIAGBFJ@wake?9and?9continue?$AA@ (0000)
002435C0 0010:
	??_C@_0BA@GMMPCDLM@wait?9for?9finish?$AA@ (0000)
002435D0 001d:
	??_C@_0BN@CPMBOFMF@targeted?5jump?5?$CI?$CF?42fh?0?5?$CF?42fv?$CJ?$AA@ (0000)
002435F0 000d:
	??_C@_0N@MHAJODEH@running?5jump?$AA@ (0000)
00243600 0023:
	??_C@_0CD@CFLDIMMG@enter?5vehicle?5as?5?$CFs?5if?5within?5?$CF?4@ (0000)
00243624 0009:
	??_C@_08OFIBMPOE@any?9seat?$AA@ (0000)
00243630 0007:
	??_C@_06NCODBMHO@driver?$AA@ (0000)
00243638 000a:
	??_C@_09NFKEFBPD@passenger?$AA@ (0000)
00243644 0007:
	??_C@_06NJIDJFDL@gunner?$AA@ (0000)
0024364C 000f:
	??_C@_0P@ELLMOHEN@any?9non?9driver?$AA@ (0000)
0024365C 0017:
	??_C@_0BH@JKPBBGH@throw?5grenade?5at?5?$CIp?$CFd?$CJ?$AA@ (0000)
00243674 0018:
	??_C@_0BI@CFJADMHA@shoot?5at?5?$CIp?$CFd?$CJ?5for?5?$CF?41f?$AA@ (0000)
0024368C 000a:
	??_C@_09PCOPMOEK@crouch?5?$CFs?$AA@ (0000)
00243698 0007:
	??_C@_06OFNJOBDK@enable?$AA@ (0000)
002436A0 0008:
	??_C@_07DBNODEFN@disable?$AA@ (0000)
002436A8 0012:
	??_C@_0BC@MGCAJCHA@animation?5mode?5?$CFs?$AA@ (0000)
002436BC 0006:
	??_C@_05DDGCEFJL@panic?$AA@ (0000)
002436C4 0007:
	??_C@_06NDKEFKOH@combat?$AA@ (0000)
002436CC 0007:
	??_C@_06PDPNPFPP@asleep?$AA@ (0000)
002436D4 000a:
	??_C@_09CENHCFNA@noncombat?$AA@ (0000)
002436E0 0017:
	??_C@_0BH@IPOBIPFB@look?5?$CFs?5at?5?$CFs?5for?5?$CF?41f?$AA@ (0000)
002436F8 0008:
	??_C@_07CBDMFKJF@?$DMerror?$DO?$AA@ (0000)
00243700 001b:
	??_C@_0BL@FFKOOMBE@look?5?$CFs?5at?5player?5for?5?$CF?41f?$AA@ (0000)
0024371C 0031:
	??_C@_0DB@IOOLPGJB@look?5?$CFs?5at?5random?5one?5of?5?$CIp?$CFd?9p?$CF@ (0000)
00243750 001a:
	??_C@_0BK@IEJFKCKH@look?5?$CFs?5at?5?$CIp?$CFd?$CJ?5for?5?$CF?41f?$AA@ (0000)
0024376C 0015:
	??_C@_0BF@IFMPJNE@move?5?$CFs?5for?5?$CF?41f?5sec?$AA@ (0000)
00243784 0021:
	??_C@_0CB@IFAAGCMK@move?5?$CFs?5towards?5?$CIp?$CFd?$CJ?0?5dist?5?$CF?42f@ (0000)
002437A8 0024:
	??_C@_0CE@PMEAGMOK@move?5?$CFs?5along?5angle?5?$CF?41f?0?5dist?5?$CF@ (0000)
002437CC 000b:
	??_C@_0L@CPPIDKFP@any?9facing?$AA@ (0000)
002437D8 000a:
	??_C@_09GNCFCLDO@backwards?$AA@ (0000)
002437E4 0006:
	??_C@_05DHJDAOHK@right?$AA@ (0000)
002437EC 0005:
	??_C@_04GOOOJOPP@left?$AA@ (0000)
002437F4 0009:
	??_C@_08HMDMCCDA@forwards?$AA@ (0000)
00243800 001b:
	??_C@_0BL@OJAEMMNB@go?5to?5?$CIp?$CFd?$CJ?5and?5face?5?$CIp?$CFd?$CJ?$AA@ (0000)
0024381C 000f:
	??_C@_0P@POPHLLEH@go?5to?5?$CIp?$CFd?$CJ?5?$CFs?$AA@ (0000)
0024382C 000c:
	??_C@_0M@PMIIAAGB@keep_moving?$AA@ (0000)
00243838 000e:
	??_C@_0O@MHBHOGCE@stop_at_point?$AA@ (0000)
00243848 000b:
	??_C@_0L@ENHEDOMM@pause?5?$CF?41f?$AA@ (0000)
00243854 0012:
	??_C@_0BC@PPCIDFNG@forced?5aim?5weapon?$AA@ (0000)
00243868 0014:
	??_C@_0BE@MMENGNPN@forced?5exact?5facing?$AA@ (0000)
0024387C 0014:
	??_C@_0BE@HLOJAFEL@idle?5look?5with?5head?$AA@ (0000)
00243890 0011:
	??_C@_0BB@CNNKGAPI@idle?5turn?5around?$AA@ (0000)
002438A4 0010:
	??_C@_0BA@HMGFJNDM@idle?5aim?5weapon?$AA@ (0000)
002438B4 0011:
	??_C@_0BB@FOMPEOPI@?$CFs?3?5?$CFs?5?$CD?$CFd?$CFs?3?5?$CFs?$AA@ (0000)
002438C8 0008:
	??_C@_07OGBBMDNN@?5FAILED?$AA@ (0000)
002438D0 0006:
	??_C@_05GFOLEBJA@?$CFs?1?$CFs?$AA@ (0000)
002438D8 000f:
	??_C@_0P@MPLGIMAD@?$DMno?5encounter?$DO?$AA@ (0000)
002438E8 0004:
	__real@3d888889 (0000)
002438EC 0004:
	__real@3c8efa35 (0000)
002438F0 0004:
	__real@43b40000 (0000)
002438F4 0004:
	__real@3f7be76d (0000)
002438F8 0036:
	??_C@_0DG@HCPHDADC@current_command?9?$DOatom_type?5?$DN?$DN?5_a@ (0000)
00243930 0013:
	??_C@_0BD@NEGHAILN@finished_reference?$AA@ (0000)
*/

/* ---------- headers */

#define random_range random_range_inline
#define vector_from_points2d vector_from_points2d_inline
#include "cseries.h"
#include "actions.h"

#include "actor_looking.h"
#include "actors.h"
#include "ai_debug.h"
#include "ai_scenario_definitions.h"
#include "cseries/errors.h"
#include "math/real_math.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "units/bipeds.h"
#include "units/units.h"
#undef vector_from_points2d
#undef random_range

/* ---------- constants */

enum
{
	OBEY_MAXIMUM_LOOP_COUNT = 10,
};

/* ---------- macros */

/* ---------- structures */

typedef void (*action_obey_individual_iterator_proc)(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data);

typedef char action_obey_simple_control_size_assert[
	sizeof(struct obey_individual_simple_control) == 0x24 ? 1 : -1];
typedef char action_obey_complex_control_size_assert[
	sizeof(struct obey_individual_complex_control) == 0x58 ? 1 : -1];
typedef char action_obey_state_size_assert[
	sizeof(struct obey_state_data) == 0x84 ? 1 : -1];
typedef char action_obey_state_offset_assert[
	offsetof(struct actor_datum, state.action_data.obey) == 0x9C ? 1 : -1];

/* ---------- prototypes */

static void action_obey_command_end(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	byte *next_command_index);
static void action_obey_individual_setup(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data);
static void action_obey_individual_flush_command_indices(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data);
static void action_obey_individual_begin(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data);
static void action_obey_individual_advance(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data);
static void action_obey_individual_end(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data);
static void action_obey_individuals_iterate(
	long actor_index,
	boolean initialize_structures,
	struct obey_state_data *state_data,
	action_obey_individual_iterator_proc iterator,
	void *user_data);
static void action_obey_directmovement_update_facing(
	long actor_index,
	long unit_index,
	struct obey_individual_simple_control *simple_control);
static void action_obey_individual_update(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data);

/* ---------- globals */

/* ---------- public code */

boolean action_obey_command_list_setup(
	long actor_index,
	short command_list_index,
	struct obey_state_data *state_data)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct scenario *scenario = global_scenario_get();
	struct ai_command_list_definition *command_list = NULL;
	boolean success = FALSE;

	match_assert("c:\\halo\\SOURCE\\ai\\action_obey.c", 1597, state_data);
	csmemset(state_data, 0, sizeof(*state_data));

	if (command_list_index >= 0 &&
		command_list_index < scenario->ai_command_lists.count)
	{
		command_list = TAG_BLOCK_GET_ELEMENT(
			&scenario->ai_command_lists,
			command_list_index,
			struct ai_command_list_definition);

		if (actor->meta.swarm && actor->meta.swarm_cache_index == NONE)
		{
			if (actor->meta.active)
			{
				char buffer[256];

				ai_debug_describe_actor(actor_index, NONE, TRUE, buffer, sizeof(buffer));
				error(
					_error_silent,
					"swarm actor %s cannot execute command list, ran out of swarm caches",
					buffer);
			}
			else
			{
				actor->state.command_list_index = command_list_index;
			}
		}
		else if (command_list->runtime_structure_bsp_reference_index != NONE &&
			command_list->runtime_structure_bsp_reference_index != global_structure_bsp_index)
		{
			error(
				_error_silent,
				"wrong structure bsp, cannot execute command list %s",
				command_list->name);
		}
		else
		{
			state_data->command_list_index = command_list_index;
			success = TRUE;
		}
	}

	if (success)
	{
		boolean initiative = TEST_FLAG(command_list->flags, _ai_command_list_allow_initiative_bit);
		boolean targeting = TEST_FLAG(command_list->flags, _ai_command_list_allow_targeting_bit);
		boolean allow_looking = !TEST_FLAG(command_list->flags, _ai_command_list_disable_looking_bit);
		boolean allow_communication = !TEST_FLAG(command_list->flags, _ai_command_list_disable_communication_bit);

		if (!allow_looking)
		{
			actor_look_secondary_stop(actor_index);
		}
		state_data->initiative = initiative;
		state_data->allow_looking = allow_looking;
		state_data->allow_communication = allow_communication;
		action_obey_individuals_iterate(
			actor_index,
			TRUE,
			state_data,
			action_obey_individual_setup,
			&targeting);
	}

	return success;
}

void action_obey_flush_command_indices(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct obey_state_data *state_data = &actor->state.action_data.obey;
	struct scenario *scenario = global_scenario_get();

	if (state_data->command_list_index < 0 ||
		state_data->command_list_index >= scenario->ai_command_lists.count)
	{
		state_data->command_list_index = NONE;
		state_data->finished = TRUE;
		actor_action_change(actor_index, _actor_action_none, NULL);
	}
	else
	{
		action_obey_individuals_iterate(
			actor_index,
			FALSE,
			state_data,
			action_obey_individual_flush_command_indices,
			NULL);
	}

	return;
}

void action_obey_flush_structure_indices(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct obey_state_data *state_data = &actor->state.action_data.obey;

	if (state_data->complex_control.destination_valid)
	{
		state_data->complex_control.destination_surface_index = NONE;
	}

	return;
}

void action_obey_advance_command_list(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct obey_state_data *state_data = &actor->state.action_data.obey;

	action_obey_individuals_iterate(
		actor_index,
		FALSE,
		state_data,
		action_obey_individual_advance,
		NULL);

	return;
}

void action_obey_begin(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct obey_state_data *state_data = &actor->state.action_data.obey;

	action_obey_individuals_iterate(
		actor_index,
		FALSE,
		state_data,
		action_obey_individual_begin,
		NULL);

	return;
}

void action_obey_end(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct obey_state_data *state_data = &actor->state.action_data.obey;

	action_obey_individuals_iterate(
		actor_index,
		FALSE,
		state_data,
		action_obey_individual_end,
		NULL);

	return;
}

void action_obey_update(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct obey_state_data *state_data = &actor->state.action_data.obey;

	action_obey_individuals_iterate(
		actor_index,
		FALSE,
		state_data,
		action_obey_individual_update,
		NULL);

	return;
}

short random_range(
	short lower_bound,
	short upper_bound)
{
	return seed_random_range(
		get_global_random_seed_address(),
		lower_bound,
		upper_bound);
}

real_vector2d *vector_from_points2d(
	real_point2d const *a,
	real_point2d const *b,
	real_vector2d *result)
{
	result->i = b->x-a->x;
	result->j = b->y-a->y;

	return result;
}


/* ---------- private code */

static void action_obey_command_end(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	byte *next_command_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct ai_command_list_definition *command_list = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_command_lists,
		command_list_index,
		struct ai_command_list_definition);

	if (simple_control->current_command_index < command_list->commands.count)
	{
		struct ai_command_definition *command = TAG_BLOCK_GET_ELEMENT(
			&command_list->commands,
			simple_control->current_command_index,
			struct ai_command_definition);

		switch (command->atom_type)
		{
		case _ai_atom_go_to:
		case _ai_atom_go_to_and_face:
			if (unit_index == actor->meta.unit_index)
			{
				actor_move_halt(actor_index);
			}
			if (complex_control)
			{
				complex_control->destination_valid = FALSE;
				complex_control->destination_facing = FALSE;
			}
			break;

		case _ai_atom_move_direction:
		case _ai_atom_move_immediate:
			SET_FLAG(simple_control->simple_control_flags, _obey_simple_directmovement_bit, FALSE);
			simple_control->directmovement.facing = NONE;
			break;

		case _ai_atom_shoot:
			if (complex_control)
			{
				complex_control->shoot_at_target = FALSE;
			}
			break;

		case _ai_atom_running_jump:
		case _ai_atom_targeted_jump:
			SET_FLAG(simple_control->simple_control_flags, _obey_simple_jump_bit, FALSE);
			simple_control->jump.delay_ticks = 0;
			break;

		case _ai_atom_loop:
			{
				boolean loop = TRUE;

				if (command->atom_modifier == _ai_atom_loop_modifier_until_told_to_advance)
				{
					loop = !TEST_FLAG(simple_control->metadata_flags, _obey_metadata_told_to_advance_bit);
					SET_FLAG(simple_control->metadata_flags, _obey_metadata_told_to_advance_bit, FALSE);
					SET_FLAG(simple_control->metadata_flags, _obey_metadata_waiting_for_advance_notification_bit, loop);
				}

				if (loop)
				{
					if (command->command_index == simple_control->current_command_index)
					{
						char buffer[512];

						ai_debug_describe_actor(actor_index, NONE, TRUE, buffer, sizeof(buffer));
						error(
							_error_silent,
							"%s: command list %s entry #%d tried to loop to itself",
							buffer,
							command_list->name,
							simple_control->current_command_index);
					}
					else if (simple_control->loop_counter >= OBEY_MAXIMUM_LOOP_COUNT)
					{
						char buffer[512];

						ai_debug_describe_actor(actor_index, NONE, TRUE, buffer, sizeof(buffer));
						error(
							_error_silent,
							"%s: command list %s is stuck looping (aborting on loop #%d)",
							buffer,
							command_list->name,
							simple_control->current_command_index);
					}
					else
					{
						*next_command_index = command->command_index;
						simple_control->loop_counter += 1;
					}
				}
			}
			break;

		case _ai_atom_animate:
			{
				struct biped_datum *biped = biped_try_and_get(unit_index);

				if (biped)
				{
					SET_FLAG(biped->biped.flags, _biped_absolute_movement_bit, FALSE);
					SET_FLAG(biped->biped.flags, _biped_no_collision_bit, FALSE);
				}
			}
			break;

		case _ai_atom_look:
		case _ai_atom_look_random:
		case _ai_atom_look_player:
		case _ai_atom_look_object:
			if (unit_index == actor->meta.unit_index)
			{
				actor_look_secondary_stop(actor_index);
			}
			break;
		}
	}

	return;
}

static void action_obey_individual_setup(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data)
{
	boolean *targeting_reference = user_data;

	csmemset(simple_control, 0, sizeof(*simple_control));
	simple_control->current_command_index = NONE;
	match_assert("c:\\halo\\SOURCE\\ai\\action_obey.c", 1360, targeting_reference);
	SET_FLAG(simple_control->metadata_flags, _obey_metadata_targeting_bit, *targeting_reference);

	if (complex_control)
	{
		csmemset(complex_control, 0, sizeof(*complex_control));
		complex_control->override_movement_type = NONE;
	}

	return;
}

static void action_obey_individual_flush_command_indices(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data)
{
	struct ai_command_list_definition *command_list = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_command_lists,
		command_list_index,
		struct ai_command_list_definition);

	if (simple_control->current_command_index >= command_list->commands.count)
	{
		simple_control->current_command_index = NONE;
	}

	return;
}

static void action_obey_individual_begin(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data)
{
	struct ai_command_list_definition *command_list = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_command_lists,
		command_list_index,
		struct ai_command_list_definition);

	if (TEST_FLAG(command_list->flags, _ai_command_list_disable_falling_damage_bit))
	{
		struct unit_datum *unit = unit_get(unit_index);

		SET_FLAG(unit->unit.flags, _unit_no_falling_damage_bit, TRUE);
	}

	return;
}

static void action_obey_individual_advance(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data)
{
	SET_FLAG(simple_control->metadata_flags, _obey_metadata_waiting_for_advance_notification_bit, FALSE);
	SET_FLAG(simple_control->metadata_flags, _obey_metadata_told_to_advance_bit, TRUE);

	return;
}

static void action_obey_individual_end(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data)
{
	struct unit_datum *unit = unit_get(unit_index);

	if (!TEST_FLAG(simple_control->metadata_flags, _obey_metadata_commands_finished_bit))
	{
		byte next_command_index;

		action_obey_command_end(
			actor_index,
			unit_index,
			command_list_index,
			simple_control,
			complex_control,
			&next_command_index);
	}
	SET_FLAG(unit->unit.flags, _unit_no_falling_damage_bit, FALSE);

	return;
}

static void action_obey_individuals_iterate(
	long actor_index,
	boolean initialize_structures,
	struct obey_state_data *state_data,
	action_obey_individual_iterator_proc iterator,
	void *user_data)
{
	struct actor_datum *actor = actor_get(actor_index);

	match_assert(
		"c:\\halo\\SOURCE\\ai\\action_obey.c",
		1553,
		!actor->meta.swarm || (actor->meta.swarm_cache_index != NONE));

	if (actor->meta.swarm)
	{
		struct swarm_datum *swarm = swarm_get(actor->meta.swarm_cache_index);
		short component_index;

		for (component_index = 0; component_index < swarm->unit_count; component_index++)
		{
			struct swarm_component_datum *component =
				swarm_component_get(swarm->component_indices[component_index]);

			if (initialize_structures)
			{
				csmemset(&component->obey, 0, sizeof(component->obey));
				SET_FLAG(component->flags, _swarm_component_wander_bit, FALSE);
				SET_FLAG(component->flags, _swarm_component_obey_bit, TRUE);
			}

			if (TEST_FLAG(component->flags, _swarm_component_obey_bit))
			{
				iterator(
					actor_index,
					swarm->unit_indices[component_index],
					state_data->command_list_index,
					&component->obey,
					NULL,
					user_data);
			}
		}
	}
	else
	{
		iterator(
			actor_index,
			actor->meta.unit_index,
			state_data->command_list_index,
			&state_data->simple_control,
			&state_data->complex_control,
			user_data);
	}

	return;
}

static void action_obey_directmovement_update_facing(
	long actor_index,
	long unit_index,
	struct obey_individual_simple_control *simple_control)
{
	struct actor_datum *actor = actor_get(actor_index);
	real_vector3d facing;
	real_vector3d perpendicular;

	if (unit_index == actor->meta.unit_index)
	{
		facing = actor->input.facing_vector;
	}
	else
	{
		unit_get_facing_vector(unit_index, &facing);
	}

	switch (simple_control->directmovement.facing)
	{
	case _ai_atom_move_facing_forwards:
		simple_control->directmovement.vector = facing;
		break;

	case _ai_atom_move_facing_backwards:
		negate_vector3d(&facing, &simple_control->directmovement.vector);
		break;

	case _ai_atom_move_facing_left:
	case _ai_atom_move_facing_right:
		cross_product3d(global_up3d, &facing, &perpendicular);
		if (normalize3d(&perpendicular) == 0.0f)
		{
			struct unit_datum *unit = unit_get(unit_index);

			cross_product3d(&unit->object.up, &facing, &perpendicular);
			if (normalize3d(&perpendicular) == 0.0f)
			{
				perpendicular = *global_forward3d;
			}
		}
		if (simple_control->directmovement.facing == _ai_atom_move_facing_left)
		{
			simple_control->directmovement.vector = perpendicular;
		}
		else
		{
			negate_vector3d(&perpendicular, &simple_control->directmovement.vector);
		}
		break;
	}

	return;
}

static void action_obey_individual_update(
	long actor_index,
	long unit_index,
	short command_list_index,
	struct obey_individual_simple_control *simple_control,
	struct obey_individual_complex_control *complex_control,
	void *user_data)
{
	if (simple_control->pause_timer > 0)
	{
		simple_control->pause_timer -= 1;
	}

	if (TEST_FLAG(simple_control->simple_control_flags, _obey_simple_jump_bit))
	{
		if (simple_control->jump.delay_ticks > 0)
		{
			simple_control->jump.delay_ticks -= 1;
		}
	}

	if (TEST_FLAG(simple_control->simple_control_flags, _obey_simple_directmovement_bit) &&
		TEST_FLAG(simple_control->simple_control_flags, _obey_simple_directmovement_update_continuously_bit))
	{
		action_obey_directmovement_update_facing(actor_index, unit_index, simple_control);
	}

	return;
}
