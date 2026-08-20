/*
HS_RUNTIME.C

symbols in this file:
000B9850 0030:
	_code_000b9850 (0000)
000B9880 0050:
	_code_000b9880 (0000)
000B98D0 0050:
	_code_000b98d0 (0000)
000B9920 0050:
	_code_000b9920 (0000)
000B9970 0050:
	_code_000b9970 (0000)
000B99C0 0050:
	_code_000b99c0 (0000)
000B9A10 0090:
	_code_000b9a10 (0000)
000B9AA0 0100:
	_hs_runtime_initialize (0000)
000B9BA0 0080:
	_hs_runtime_dispose_from_old_map (0000)
000B9C20 0010:
	_hs_runtime_dispose (0000)
000B9C30 00b0:
	_code_000b9c30 (0000)
000B9CE0 00f0:
	_code_000b9ce0 (0000)
000B9DD0 0050:
	_code_000b9dd0 (0000)
000B9E20 0080:
	_code_000b9e20 (0000)
000B9EA0 0080:
	_code_000b9ea0 (0000)
000B9F20 0020:
	_code_000b9f20 (0000)
000B9F40 0150:
	_code_000b9f40 (0000)
000BA090 00b0:
	_code_000ba090 (0000)
000BA140 0060:
	_code_000ba140 (0000)
000BA1A0 0080:
	_code_000ba1a0 (0000)
000BA220 0020:
	_code_000ba220 (0000)
000BA240 0020:
	_code_000ba240 (0000)
000BA260 0020:
	_code_000ba260 (0000)
000BA280 0010:
	_code_000ba280 (0000)
000BA290 0020:
	_code_000ba290 (0000)
000BA2B0 0010:
	_code_000ba2b0 (0000)
000BA2C0 0020:
	_code_000ba2c0 (0000)
000BA2E0 0020:
	_code_000ba2e0 (0000)
000BA300 0010:
	_code_000ba300 (0000)
000BA310 0010:
	_code_000ba310 (0000)
000BA320 0040:
	_code_000ba320 (0000)
000BA360 0030:
	_code_000ba360 (0000)
000BA390 0080:
	_code_000ba390 (0000)
000BA410 0100:
	_hs_can_cast (0000)
000BA510 00c0:
	_hs_cast (0000)
000BA5D0 0580:
	_code_000ba5d0 (0000)
000BAB50 0190:
	_code_000bab50 (0000)
000BACE0 0040:
	_code_000bace0 (0000)
000BAD20 0020:
	_hs_runtime_get_executing_thread_name (0000)
000BAD40 0020:
	_hs_wake_by_name (0000)
000BAD60 0180:
	_render_debug_scripting (0000)
000BAEE0 0440:
	_render_debug_trigger_volumes (0000)
000BB320 0120:
	_hs_return (0000)
000BB440 0040:
	_code_000bb440 (0000)
000BB480 00f0:
	_hs_evaluate_wake (0000)
000BB570 0170:
	_code_000bb570 (0000)
000BB6E0 0060:
	_code_000bb6e0 (0000)
000BB740 01c0:
	_code_000bb740 (0000)
000BB900 0030:
	_hs_macro_function_evaluate (0000)
000BB930 00d0:
	_hs_evaluate_begin (0000)
000BBA00 0210:
	_hs_evaluate_begin_random (0000)
000BBC10 0190:
	_hs_evaluate_if (0000)
000BBDA0 0140:
	_hs_evaluate_set (0000)
000BBEE0 0130:
	_hs_evaluate_logical (0000)
000BC010 0180:
	_hs_evaluate_arithmetic (0000)
000BC190 00e0:
	_hs_evaluate_equality (0000)
000BC270 0210:
	_hs_evaluate_inequality (0000)
000BC480 01c0:
	_hs_evaluate_sleep (0000)
000BC640 0200:
	_hs_evaluate_sleep_until (0000)
000BC840 0100:
	_hs_evaluate_inspect (0000)
000BC940 0120:
	_hs_evaluate_object_cast_up (0000)
000BCA60 0180:
	_hs_evaluate_debug_string (0000)
000BCBE0 02f0:
	_code_000bcbe0 (0000)
000BCED0 02d0:
	_hs_runtime_initialize_for_new_map (0000)
000BD1A0 00b0:
	_hs_runtime_update (0000)
000BD250 00c0:
	_hs_runtime_evaluate (0000)
0026E880 0017:
	??_C@_0BH@JDOAAMFH@type?$DN?$DN_hs_type_boolean?$AA@ (0000)
0026E898 0030:
	??_C@_0DA@OFBKMPEO@c?3?2halo?2source?2hs?2hs_library_int@ (0000)
0026E8C8 0003:
	??_C@_02NJPGOMH@?$CFf?$AA@ (0000)
0026E8CC 0014:
	??_C@_0BE@OILODGFF@type?$DN?$DN_hs_type_real?$AA@ (0000)
0026E8E0 001d:
	??_C@_0BN@EMEJADII@type?$DN?$DN_hs_type_short_integer?$AA@ (0000)
0026E900 0004:
	??_C@_03JALODAI@?$CFld?$AA@ (0000)
0026E904 001c:
	??_C@_0BM@KMJMLGIL@type?$DN?$DN_hs_type_long_integer?$AA@ (0000)
0026E920 0016:
	??_C@_0BG@BOOAAIJG@type?$DN?$DN_hs_type_string?$AA@ (0000)
0026E938 0033:
	??_C@_0DD@LFJLKEGO@enum_value?$DO?$DN0?5?$CG?$CG?5enum_value?$DMenum@ (0000)
0026E96C 0016:
	??_C@_0BG@BHHILIEN@HS_TYPE_IS_ENUM?$CItype?$CJ?$AA@ (0000)
0026E984 0025:
	??_C@_0CF@LGMLOJMN@couldn?8t?5allocate?5scripting?5glob@ (0000)
0026E9AC 000c:
	??_C@_0M@IAKOKLGB@index?$CB?$DNNONE?$AA@ (0000)
0026E9B8 0024:
	??_C@_0CE@JAGJENNE@raise?5MAXIMUM_NUMBER_OF_HS_GLOBA@ (0000)
0026E9DC 001f:
	??_C@_0BP@HNAHKNME@c?3?2halo?2SOURCE?2hs?2hs_runtime?4c?$AA@ (0000)
0026E9FC 000b:
	??_C@_0L@KLMNBLNA@hs?5globals?$AA@ (0000)
0026EA08 000a:
	??_C@_09NFPDIMGO@hs?5thread?$AA@ (0000)
0026EA14 0010:
	??_C@_0BA@DMAAAIAF@?$CIend?5of?5script?$CJ?$AA@ (0000)
0026EA24 0033:
	??_C@_0DD@EECLOJLE@type?$CB?$DN_hs_thread_type_script?5?$HM?$HM?5@ (0000)
0026EA58 002a:
	??_C@_0CK@PILPJPBD@type?$DO?$DN0?5?$CG?$CG?5type?$DMNUMBER_OF_HS_THR@ (0000)
0026EA84 003a:
	??_C@_0DK@IFFNCFDC@hs_thread_get?$CIthread_index?$CJ?9?$DOtyp@ (0000)
0026EAC0 0014:
	??_C@_0BE@FDONEJOD@?$FLglobal?5initialize?$FN?$AA@ (0000)
0026EAD4 0012:
	??_C@_0BC@DIMBPFCM@?$FLconsole?5command?$FN?$AA@ (0000)
0026EAE8 003a:
	??_C@_0DK@FJONKAAK@a?5problem?5occurred?5while?5executi@ (0000)
0026EB24 0010:
	??_C@_0BA@OPEHPPEI@stack?5overflow?4?$AA@ (0000)
0026EB34 003f:
	??_C@_0DP@HONBOJOM@?$CIbyte?5?$CK?$CJ?5?$CInew_frame?$CL1?$CJ?$DMthread?9?$DOs@ (0000)
0026EB78 0046:
	??_C@_0EG@GLGLNCNE@frame?9?$DOdata?$CLframe?9?$DOsize?$CLsize?$DM?$DNth@ (0000)
0026EBC0 002f:
	??_C@_0CP@LGELPKKM@attempt?5to?5allocate?5zero?5space?5f@ (0000)
0026EBF0 0011:
	??_C@_0BB@IJDFEIOE@corrupted?5stack?4?$AA@ (0000)
0026EC04 0015:
	??_C@_0BF@NOEAJPHF@valid_thread?$CIthread?$CJ?$AA@ (0000)
0026EC1C 003a:
	??_C@_0DK@CIPMAIGO@desired_type?$DO?$DN0?5?$CG?$CG?5desired_type?$DM@ (0000)
0026EC58 0038:
	??_C@_0DI@FBLLEJHE@actual_type?$DO?$DN0?5?$CG?$CG?5actual_type?$DMNU@ (0000)
0026EC90 001c:
	??_C@_0BM@LDIPEIGF@hs_type_valid?$CIdesired_type?$CJ?$AA@ (0000)
0026ECAC 003b:
	??_C@_0DL@KFHDEBEB@actual_type?$DN?$DN_hs_passthrough?5?$HM?$HM?5@ (0000)
0026ECE8 000e:
	??_C@_0O@OAGBFBKC@bad?5typecast?4?$AA@ (0000)
0026ECF8 0027:
	??_C@_0CH@CMGFPKPO@hs_can_cast?$CIactual_type?0?5desired@ (0000)
0026ED20 002b:
	??_C@_0CL@BFAIEDCL@script?5?$CFs?5needs?5to?5be?5recompiled@ (0000)
0026ED4C 0011:
	??_C@_0BB@OIKAMJPF@no?5reason?5given?4?$AA@ (0000)
0026ED60 000a:
	??_C@_09DJENBFJK@?$FLunknown?$FN?$AA@ (0000)
0026ED6C 0003:
	??_C@_02BGAGALCL@?$HMt?$AA@ (0000)
0026ED70 0028:
	??_C@_0CI@KMEEEBHP@?$HMn?$HMn?$HMnscript?5name?$HMtsleep?5time?$HMtf@ (0000)
0026ED98 0028:
	??_C@_0CI@PIIEBFIK@script_name_node?9?$DOtype?$DN?$DN_hs_type@ (0000)
0026EDC0 0042:
	??_C@_0EC@EIHIFDEJ@TEST_FLAG?$CIscript_name_node?9?$DOflag@ (0000)
0026EE04 0018:
	??_C@_0BI@DEHGHNBF@?$CKexpression_index?$DN?$DNNONE?$AA@ (0000)
0026EE1C 001e:
	??_C@_0BO@IEBGLNDF@unexpected?5actual?5parameters?4?$AA@ (0000)
0026EE40 004b:
	??_C@_0EL@EPENHJGN@hs_syntax_get?$CI?$CKexpression_index?$CJ@ (0000)
0026EE8C 0017:
	??_C@_0BH@JHPKLPNE@corrupted?5syntax?5tree?4?$AA@ (0000)
0026EEA4 0018:
	??_C@_0BI@JEAIILKP@?$CKexpression_index?$CB?$DNNONE?$AA@ (0000)
0026EEBC 0023:
	??_C@_0CD@LMIAMJPE@function_index?$DN?$DN_hs_function_beg@ (0000)
0026EEE0 001a:
	??_C@_0BK@ICIEGLDA@?$CKargument_count?$DMLONG_BITS?$AA@ (0000)
0026EEFC 002a:
	??_C@_0CK@HFFDNMBO@function_index?$DN?$DN_hs_function_beg@ (0000)
0026EF28 0052:
	??_C@_0FC@PECKPIAM@parameter_types?$FL0?$FN?$DN?$DN_hs_type_sho@ (0000)
0026EF7C 0028:
	??_C@_0CI@EGAEHKHM@attempt?5to?5convert?5object?5?$CFs?5to?5@ (0000)
0026EFA4 0013:
	??_C@_0BD@HGOCLPEG@function?9?$DOevaluate?$AA@ (0000)
0026EFB8 0023:
	??_C@_0CD@JFOOFKBP@found?5a?5static?5script?5at?5topleve@ (0000)
0026EFE0 004f:
	??_C@_0EP@ECAJPAK@script?9?$DOscript_type?$CB?$DN_hs_script_@ (0000)
0026F030 001b:
	??_C@_0BL@LKLEHFKI@ran?5out?5of?5script?5threads?4?$AA@ (0000)
0026F04C 002c:
	??_C@_0CM@EIHAGODH@a?5global?5initialization?5attempte@ (0000)
0026F078 0020:
	??_C@_0CA@COEBOOJE@internal_thread?9?$DOsleep_until?$DN?$DN0?$AA@ (0000)
0026F098 0036:
	??_C@_0DG@NJPPFAFA@there?5are?5not?5enough?5threads?5to?5@ (0000)
002E2370 264c:
	_data_002e2370 (0000)
	_typecasting_procedures (00c8)
004535AC 0008:
	_bss_004535ac (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "hs/hs.h"
#include "hs/object_lists.h"
#include "hs/hs_scenario_definitions.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "saved games/game_state.h"
#include "cseries/errors.h"

/* ---------- constants */

enum
{
	_hs_thread_type_script = 0,
	_hs_thread_type_global_initialize,
	_hs_thread_type_console_command,
	NUMBER_OF_HS_THREAD_TYPES,
};

enum
{
	_hs_syntax_node_primitive_bit = 0,
	_hs_function_wake = 0x15,
};

/* ---------- macros */

typedef long (*hs_typecasting_procedure)(long value);

/* ---------- structures */

union hs_conversion_result
{
	boolean boolean;
	short short_integer;
	long long_integer;
	real real;
	char const *string;
};

struct hs_runtime_globals
{
	byte reserved[4];
	boolean initialized;
	byte pad;
	short executing_thread_index;
};

struct hs_stack_frame
{
	struct hs_stack_frame *previous;
	long expression_index;
	void *result;
	short size;
	byte data[2];
};

struct hs_thread_datum
{
	short identifier;
	byte type;
	byte flags;
	long script_index;
	long sleep_until;
	long unknown0c;
	struct hs_stack_frame *stack;
	long unknown14;
	byte stack_data[0x200];
};

typedef char hs_thread_datum_size_assert[
	sizeof(struct hs_thread_datum) == 0x218 ? 1 : -1];

/* ---------- prototypes */

void code_000b9880(
	short type,
	union hs_conversion_result value,
	char *result);
void code_000b98d0(
	short type,
	union hs_conversion_result value,
	char *result);
void code_000b9920(
	short type,
	union hs_conversion_result value,
	char *result);
void code_000b9970(
	short type,
	union hs_conversion_result value,
	char *result);
void code_000b99c0(
	short type,
	union hs_conversion_result value,
	char *result);
void code_000b9a10(
	short type,
	long enum_value,
	char *result);
union hs_conversion_result code_000ba220(
	union hs_conversion_result value);
union hs_conversion_result code_000ba240(
	union hs_conversion_result value);
union hs_conversion_result code_000ba260(
	union hs_conversion_result value);
union hs_conversion_result code_000ba280(
	union hs_conversion_result value);
long code_000ba290(
	long value);
union hs_conversion_result code_000ba2b0(
	union hs_conversion_result value);
long code_000ba2c0(
	long value);
union hs_conversion_result code_000ba2e0(
	union hs_conversion_result value);
long code_000ba300(
	union hs_conversion_result value);
long code_000ba310(
	union hs_conversion_result value);
long code_000ba320(
	short object_name_index);
long code_000ba360(
	long object_index);
static boolean code_000ba390(
	short actual_type,
	short desired_type);
static char const *code_000b9e20(
	long thread_index);
static void code_000b9f20(
	long thread_index);
static void code_000ba090(
	long thread_index);
static long code_000ba140(
	short script_index);
static long code_000ba1a0(
	char const *name);

/* ---------- globals */

extern hs_typecasting_procedure typecasting_procedures[NUMBER_OF_HS_TYPES][NUMBER_OF_HS_TYPES];
extern struct data_array *hs_global_data;
extern struct data_array *hs_thread_data;
extern struct data_array *hs_syntax_data;
extern short hs_external_global_count;
extern struct hs_runtime_globals bss_004535ac;

/* ---------- public code */

void hs_runtime_initialize(
	void)
{
	short global_index;
	long index;

	hs_thread_data = game_state_data_new("hs thread", 0x100, 0x218);
	hs_global_data = game_state_data_new("hs globals", 0x400, 8);
	if (hs_thread_data && hs_global_data)
	{
		match_vassert("c:\\halo\\SOURCE\\hs\\hs_runtime.c", 0xa9,
			hs_external_global_count*2<0x400,
			"raise MAXIMUM_NUMBER_OF_HS_GLOBALS.");
		data_make_valid(hs_global_data);
		for (global_index = 0;
			global_index<hs_external_global_count;
			global_index++)
		{
			index = datum_new_at_index(hs_global_data, global_index|0xaced0000);
			match_assert("c:\\halo\\SOURCE\\hs\\hs_runtime.c", 0xb1, index!=NONE);
		}
	}
	else
	{
		error(_error_immediate, "couldn't allocate scripting globals.");
	}

	return;
}

void hs_runtime_dispose(
	void)
{
	data_make_invalid(hs_global_data);

	return;
}

void hs_runtime_dispose_from_old_map(
	void)
{
	short global_index;

	data_make_invalid(hs_thread_data);
	for (global_index = hs_external_global_count;
		global_index<hs_global_data->count;
		global_index++)
	{
		if (datum_try_and_get(hs_global_data, global_index))
			datum_delete(hs_global_data, global_index);
	}
	bss_004535ac.initialized = FALSE;

	return;
}

char const *hs_runtime_get_executing_thread_name(
	void)
{
	char const *name;

	name = NULL;
	if (bss_004535ac.executing_thread_index != NONE)
		name = code_000b9e20(bss_004535ac.executing_thread_index);
	if (!name)
		name = "[unknown]";

	return name;
}

boolean hs_wake_by_name(
	char const *name)
{
	long thread_index;
	boolean result;

	thread_index = code_000ba1a0(name);
	result = FALSE;
	if (thread_index != NONE)
	{
		code_000ba090(thread_index);
		result = TRUE;
	}

	return result;
}

void hs_evaluate_wake(
	short function_index,
	long thread_index,
	boolean initialize)
{
	long wake_thread_index;
	struct hs_thread_datum *thread;
	struct hs_syntax_node *script_name_node;

	thread = datum_get(hs_thread_data, thread_index);
	script_name_node = datum_get(
		hs_syntax_data,
		((struct hs_syntax_node *)datum_get(
			hs_syntax_data,
			((struct hs_syntax_node *)datum_get(
				hs_syntax_data,
				thread->stack->expression_index))->data))->next_node_index);

	match_assert("c:\\halo\\source\\hs\\hs_library_internal_runtime.h", 0x22c,
		function_index==_hs_function_wake);
	match_assert("c:\\halo\\source\\hs\\hs_library_internal_runtime.h", 0x22d,
		TEST_FLAG(script_name_node->flags, _hs_syntax_node_primitive_bit));
	match_assert("c:\\halo\\source\\hs\\hs_library_internal_runtime.h", 0x22e,
		script_name_node->type==_hs_type_script);

	wake_thread_index = code_000ba140((short)script_name_node->data);
	if (wake_thread_index != NONE)
		code_000ba090(wake_thread_index);
	hs_return(thread_index, 0);

	return;
}

/* ---------- private code */

static char const *code_000b9e20(
	long thread_index)
{
	struct hs_thread_datum *thread;
	char const *name;

	name = NULL;
	thread = datum_get(hs_thread_data, thread_index);
	switch (thread->type)
	{
	case _hs_thread_type_script:
		name = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->hs_scripts,
			((struct hs_thread_datum *)datum_get(
				hs_thread_data,
				thread_index))->script_index,
			struct hs_script)->name;
		break;
	case _hs_thread_type_global_initialize:
		name = "[global initialize]";
		break;
	case _hs_thread_type_console_command:
		name = "[console command]";
		break;
	default:
		display_assert(NULL, "c:\\halo\\SOURCE\\hs\\hs_runtime.c", 0x2a9, TRUE);
		system_exit(-1);
		break;
	}

	return name;
}

static void code_000b9f20(
	long thread_index)
{
	struct hs_thread_datum *thread;

	thread = datum_get(hs_thread_data, thread_index);
	thread->stack = thread->stack->previous;

	return;
}

static void code_000ba090(
	long thread_index)
{
	struct hs_thread_datum *thread;
	struct hs_syntax_node *syntax;

	thread = datum_get(hs_thread_data, thread_index);
	if (thread->sleep_until != NONE)
	{
		thread->sleep_until = 0;
		if (TEST_FLAG(thread->flags, 1))
		{
			thread->sleep_until = thread->unknown0c;
			SET_FLAG(thread->flags, 1, FALSE);
		}
		else
		{
			if (thread->stack->expression_index != NONE)
			{
				syntax = datum_get(hs_syntax_data, thread->stack->expression_index);
				if (syntax->index == 0x14)
				{
					code_000b9f20(thread_index);
					return;
				}
			}

			if (thread->stack->previous &&
				thread->stack->previous->expression_index != NONE)
			{
				syntax = datum_get(
					hs_syntax_data,
					thread->stack->previous->expression_index);
				if (syntax->index == 0x14)
				{
					code_000b9f20(thread_index);
					code_000b9f20(thread_index);
					SET_FLAG(thread->flags, 0, FALSE);
				}
			}
		}
	}

	return;
}

static long code_000ba1a0(
	char const *name)
{
	long thread_index;

	thread_index = data_next_index(hs_thread_data, NONE);
	while (thread_index != NONE)
	{
		struct hs_thread_datum *thread;

		thread = datum_get(hs_thread_data, thread_index);
		if (thread->script_index != NONE)
		{
			struct hs_script const *script;

			script = TAG_BLOCK_GET_ELEMENT(
				&global_scenario_get()->hs_scripts,
				thread->script_index,
				struct hs_script);
			if (_stricmp(script->name, name) == 0)
				return thread_index;
		}
		thread_index = data_next_index(hs_thread_data, thread_index);
	}

	return NONE;
}

static long code_000ba140(
	short script_index)
{
	long thread_index;

	thread_index = data_next_index(hs_thread_data, NONE);
	while (thread_index != NONE)
	{
		struct hs_thread_datum *thread;

		thread = datum_get(hs_thread_data, thread_index);
		if (thread->script_index == script_index)
			return thread_index;
		thread_index = data_next_index(hs_thread_data, thread_index);
	}

	return NONE;
}

void code_000b9880(
	short type,
	union hs_conversion_result value,
	char *result)
{
	match_assert("c:\\halo\\source\\hs\\hs_library_internal_runtime.h", 0x241,
		type==_hs_type_boolean);

	sprintf(result, "%s", value.boolean ? "true" : "false");

	return;
}

void code_000b98d0(
	short type,
	union hs_conversion_result value,
	char *result)
{
	match_assert("c:\\halo\\source\\hs\\hs_library_internal_runtime.h", 0x24c,
		type==_hs_type_real);

	sprintf(result, "%f", value.real);

	return;
}

void code_000b9920(
	short type,
	union hs_conversion_result value,
	char *result)
{
	match_assert("c:\\halo\\source\\hs\\hs_library_internal_runtime.h", 0x257,
		type==_hs_type_short_integer);

	sprintf(result, "%d", value.short_integer);

	return;
}

void code_000b9970(
	short type,
	union hs_conversion_result value,
	char *result)
{
	match_assert("c:\\halo\\source\\hs\\hs_library_internal_runtime.h", 0x262,
		type==_hs_type_long_integer);

	sprintf(result, "%ld", value.long_integer);

	return;
}

void code_000b99c0(
	short type,
	union hs_conversion_result value,
	char *result)
{
	match_assert("c:\\halo\\source\\hs\\hs_library_internal_runtime.h", 0x26d,
		type==_hs_type_string);

	sprintf(result, "%s", value.string);

	return;
}

void code_000b9a10(
	short type,
	long enum_value,
	char *result)
{
	struct hs_enum_definition *enum_definition;

	enum_definition = &hs_enum_table[type-_hs_type_enum_game_difficulty];
	match_assert("c:\\halo\\source\\hs\\hs_library_internal_runtime.h", 0x27b,
		HS_TYPE_IS_ENUM(type));
	match_vassert("c:\\halo\\source\\hs\\hs_library_internal_runtime.h", 0x27c,
		(short)enum_value>=0 && (short)enum_value<enum_definition->count,
		"enum_value>=0 && enum_value<enum_definition->count");

	sprintf(result, "%s", enum_definition->values[(short)enum_value]);

	return;
}

union hs_conversion_result code_000ba220(
	union hs_conversion_result value)
{
	value.boolean = value.long_integer==0;

	return value;
}

union hs_conversion_result code_000ba240(
	union hs_conversion_result value)
{
	value.boolean = value.short_integer==0;

	return value;
}

union hs_conversion_result code_000ba260(
	union hs_conversion_result value)
{
	union hs_conversion_result result;

	result.boolean = csstrlen(value.string)==0;

	return result;
}

union hs_conversion_result code_000ba280(
	union hs_conversion_result value)
{
	union hs_conversion_result result;

	result.long_integer = 0;

	return result;
}

long code_000ba290(
	long value)
{
	value = *(short *)&value;
	*(real *)&value = (real)*(long *)&value;

	return value;
}

union hs_conversion_result code_000ba2b0(
	union hs_conversion_result value)
{
	value.real = value.long_integer;

	return value;
}

long code_000ba2c0(
	long value)
{
	value = *(short *)&value+1;
	*(real *)&value = (real)*(long *)&value;

	return value;
}

union hs_conversion_result code_000ba2e0(
	union hs_conversion_result value)
{
	value.short_integer = (short)value.real;

	return value;
}

long code_000ba300(
	union hs_conversion_result value)
{
	return (long)value.real;
}

long code_000ba310(
	union hs_conversion_result value)
{
	union hs_conversion_result result;

	result.short_integer = value.short_integer;

	return result.long_integer;
}

long code_000ba320(
	short object_name_index)
{
	long object_index;
	long object_list_index = NONE;

	object_index = object_index_from_name_index(object_name_index);
	if (object_index != NONE)
	{
		object_list_index = object_list_new();
		object_list_add(object_list_index, object_index);
	}

	return object_list_index;
}

long code_000ba360(
	long object_index)
{
	long object_list_index = NONE;

	if (object_index != NONE)
	{
		object_list_index = object_list_new();
		object_list_add(object_list_index, object_index);
	}

	return object_list_index;
}

static boolean code_000ba390(
	short actual_type,
	short desired_type)
{
	word actual_type_mask;
	word desired_type_mask;

	desired_type_mask = hs_object_type_masks[desired_type];
	actual_type_mask = hs_object_type_masks[actual_type];

	match_assert("c:\\halo\\SOURCE\\hs\\hs_runtime.c", 0x599,
		actual_type>=0 && actual_type<NUMBER_OF_HS_OBJECT_TYPES);
	match_assert("c:\\halo\\SOURCE\\hs\\hs_runtime.c", 0x59a,
		desired_type>=0 && desired_type<NUMBER_OF_HS_OBJECT_TYPES);

	return (actual_type_mask & desired_type_mask)==actual_type_mask;
}

boolean hs_can_cast(
	short actual_type,
	short desired_type)
{
	short object_type;

	match_assert("c:\\halo\\SOURCE\\hs\\hs_runtime.c", 0x5a4,
		actual_type==_hs_passthrough || hs_type_valid(actual_type));
	match_assert("c:\\halo\\SOURCE\\hs\\hs_runtime.c", 0x5a5,
		hs_type_valid(desired_type));

	if (actual_type==_hs_passthrough || actual_type==desired_type)
	{
		return TRUE;
	}

	if (HS_TYPE_IS_OBJECT_NAME(desired_type))
	{
		object_type = desired_type - _hs_type_object_name;
		if (HS_TYPE_IS_OBJECT_NAME(actual_type))
		{
			return code_000ba390(
				actual_type-_hs_type_object_name,
				object_type);
		}
		else if (!HS_TYPE_IS_OBJECT(actual_type))
		{
			return FALSE;
		}

		goto cast_object_type;
	}
	else if (HS_TYPE_IS_OBJECT(desired_type))
	{
		if (!HS_TYPE_IS_OBJECT(actual_type))
		{
			return FALSE;
		}

		object_type = desired_type - _hs_type_object;

cast_object_type:
		return code_000ba390(
			actual_type-_hs_type_object,
			object_type);
	}
	else
	{
		return typecasting_procedures[desired_type][actual_type] != NULL;
	}
}
