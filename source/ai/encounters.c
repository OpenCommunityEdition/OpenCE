/*
ENCOUNTERS.C

symbols in this file:
00047520 00f0:
	_encounters_initialize (0000)
00047610 0010:
	_encounters_dispose (0000)
00047620 0020:
	_encounters_dispose_from_old_map (0000)
00047640 04b0:
	_encounter_compute_activation_cluster_bit_vector (0000)
00047AF0 01b0:
	_encounter_detach_actor (0000)
00047CA0 0110:
	_encounter_attach_unit (0000)
00047DB0 00b0:
	_encounterless_attach_actor (0000)
00047E60 0140:
	_encounterless_detach_actor (0000)
00047FA0 0060:
	_encounter_get_by_name (0000)
00048000 0030:
	_encounter_iterator_new (0000)
00048030 0040:
	_encounter_iterator_next (0000)
00048070 0050:
	_encounter_actor_iterator_new (0000)
000480C0 0040:
	_encounter_actor_iterator_next (0000)
00048100 0080:
	_encounter_actor_iterator_prev (0000)
00048180 0040:
	_actor_iterator_new (0000)
000481C0 00a0:
	_actor_iterator_next (0000)
00048260 0050:
	_code_00048260 (0000)
000482B0 00f0:
	_code_000482b0 (0000)
000483A0 00a0:
	_encounter_modify_pursuit_desires (0000)
00048440 0280:
	_encounter_determine_pursuit_availability (0000)
000486C0 00d0:
	_code_000486c0 (0000)
00048790 0290:
	_code_00048790 (0000)
00048A20 0080:
	_code_00048a20 (0000)
00048AA0 0060:
	_encounterless_activate (0000)
00048B00 0050:
	_code_00048b00 (0000)
00048B50 00c0:
	_code_00048b50 (0000)
00048C10 00a0:
	_encounter_link_activation (0000)
00048CB0 00a0:
	_code_00048cb0 (0000)
00048D50 03d0:
	_code_00048d50 (0000)
00049120 01b0:
	_encounter_stand_down (0000)
000492D0 0090:
	_code_000492d0 (0000)
00049360 0070:
	_code_00049360 (0000)
000493D0 0030:
	_encounter_set_blind (0000)
00049400 0030:
	_encounter_set_deaf (0000)
00049430 00b0:
	_encounter_squad_timer_expire (0000)
000494E0 0100:
	_code_000494e0 (0000)
000495E0 0290:
	_code_000495e0 (0000)
00049870 00a0:
	_encounters_initialize_for_new_map (0000)
00049910 00d0:
	_encounters_unit_died (0000)
000499E0 0140:
	_encounter_verify_firing_position_owner_actor_indices (0000)
00049B20 0130:
	_encounter_build_firing_position_owner_actor_indices (0000)
00049C50 0100:
	_encounter_mark_examined_pursuit_position (0000)
00049D50 00b0:
	_encounter_pursuit_position_already_examined (0000)
00049E00 02e0:
	_encounter_get_actor_starting_location (0000)
0004A0E0 0030:
	_encounter_force_activate (0000)
0004A110 0030:
	_encounter_force_deactivate (0000)
0004A140 0110:
	_code_0004a140 (0000)
0004A250 07c0:
	_code_0004a250 (0000)
0004AA10 0170:
	_code_0004aa10 (0000)
0004AB80 0120:
	_encounter_spawn_actor (0000)
0004ACA0 0050:
	_encounter_set_respawn (0000)
0004ACF0 02c0:
	_code_0004acf0 (0000)
0004AFB0 0140:
	_code_0004afb0 (0000)
0004B0F0 0780:
	_code_0004b0f0 (0000)
0004B870 0220:
	_encounter_attach_actor (0000)
0004BA90 0470:
	_encounter_update_status (0000)
0004BF00 0080:
	_encounters_update_dirty_status (0000)
0004BF80 02f0:
	_encounter_create (0000)
0004C270 01c0:
	_code_0004c270 (0000)
0004C430 00c0:
	_encounters_create_for_new_map (0000)
0004C4F0 0100:
	_encounters_update (0000)
0024CDBC 0018:
	_global_empty_possibility (0000)
	_global_post_combat_translation_table (0010)
0024CDD4 000d:
	??_C@_0N@BNPPEOCF@pursuit_data?$AA@ (0000)
0024CDE4 000b:
	??_C@_0L@BAANOOMI@ai?5pursuit?$AA@ (0000)
0024CDF0 000e:
	??_C@_0O@CIMAAJHO@platoon_array?$AA@ (0000)
0024CE00 0008:
	??_C@_07GONBCAAG@platoon?$AA@ (0000)
0024CE08 000c:
	??_C@_0M@EHCGPGM@squad_array?$AA@ (0000)
0024CE14 0006:
	??_C@_05BDFMLHFG@squad?$AA@ (0000)
0024CE1C 000f:
	??_C@_0P@MECKMOOF@encounter_data?$AA@ (0000)
0024CE2C 001f:
	??_C@_0BP@EGKKNGMH@c?3?2halo?2SOURCE?2ai?2encounters?4c?$AA@ (0000)
0024CE50 0058:
	??_C@_0FI@BMEIILPH@?$CImove_position?9?$DOcluster_index?5?$DO?$DN@ (0000)
0024CEA8 005c:
	??_C@_0FM@DFHIGNCO@?$CIfiring_position?9?$DOcluster_index?5@ (0000)
0024CF04 003a:
	??_C@_0DK@EFAAMOOO@?$CIcluster_index?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIcluster@ (0000)
0024CF40 0031:
	??_C@_0DB@KHGAFALI@structure_bsp?9?$DOclusters?4count?5?$DM?$DN@ (0000)
0024CF74 001c:
	??_C@_0BM@ONCBDDBG@platoon?9?$DOoriginal_count?5?$DO?50?$AA@ (0000)
0024CF90 001a:
	??_C@_0BK@NALIBEDP@squad?9?$DOoriginal_count?5?$DO?50?$AA@ (0000)
0024CFAC 001e:
	??_C@_0BO@FMKIINLL@encounter?9?$DOoriginal_count?5?$DO?50?$AA@ (0000)
0024CFCC 001d:
	??_C@_0BN@DOEKPEEH@?$CKactor_index_reference?$CB?$DNNONE?$AA@ (0000)
0024CFEC 0025:
	??_C@_0CF@FBFKAFPA@actor?9?$DOmeta?4unit_index?5?$DN?$DN?5unit_i@ (0000)
0024D014 002f:
	??_C@_0CP@NCMOENFD@actor?9?$DOmeta?4encounter_index?5?$DN?$DN?5e@ (0000)
0024D044 001b:
	??_C@_0BL@OIOHEOEO@?$CBactor?9?$DOmeta?4encounterless?$AA@ (0000)
0024D060 0022:
	??_C@_0CC@PHCDCJEH@actor?9?$DOmeta?4encounter_index?$DN?$DNNON@ (0000)
0024D084 0022:
	??_C@_0CC@LJBHAGND@actor?9?$DOmeta?4platoon_index?5?$DN?$DN?5NON@ (0000)
0024D0A8 0020:
	??_C@_0CA@HNKMILAG@actor?9?$DOmeta?4squad_index?5?$DN?$DN?5NONE?$AA@ (0000)
0024D0C8 0024:
	??_C@_0CE@BMJAJPHM@actor?9?$DOmeta?4encounter_index?5?$DN?$DN?5N@ (0000)
0024D0F0 005d:
	??_C@_0FN@LOCOBBFO@WARNING?3?5too?5many?5actors?5searchi@ (0000)
0024D150 003d:
	??_C@_0DN@ECMJNADN@?$CFs?5?$CF04X?3?5coord?5?$CFd?5current?5?$CFd?1?$CFd?1@ (0000)
0024D190 0029:
	??_C@_0CJ@GDIDEEDO@overflowed?5MAXIMUM_PLATOONS_PER_@ (0000)
0024D1C0 0047:
	??_C@_0EH@JLNGBIIA@encounter_definition?9?$DOplatoons?4c@ (0000)
0024D208 0027:
	??_C@_0CH@CNGIJGEJ@overflowed?5MAXIMUM_SQUADS_PER_MA@ (0000)
0024D230 0043:
	??_C@_0ED@JFKFDEJF@encounter_definition?9?$DOsquads?4cou@ (0000)
0024D278 0063:
	??_C@_0GD@GOGHLEAJ@?$CIlink_encounter_index?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CI@ (0000)
0024D2DC 0036:
	??_C@_0DG@EPEOFOPA@parent_prop?9?$DOorphan_prop_index?5?$DN@ (0000)
0024D314 001a:
	??_C@_0BK@GDHJJECC@?$CBencounter?9?$DOenemy_visible?$AA@ (0000)
0024D330 001c:
	??_C@_0BM@IJGMLGCM@?$CFs?1?$CFs?3?5delay?5timer?5finished?$AA@ (0000)
0024D34C 0026:
	??_C@_0CG@HJILMOOD@?$CFs?1?$CFs?3?5delay?5timer?5started?5?$CI?$CF?41f@ (0000)
0024D374 0011:
	??_C@_0BB@HBMEBMNH@survivors?5?$CFd?5?$DN?50?$AA@ (0000)
0024D388 0012:
	??_C@_0BC@FNKOIBFL@survivors?5?$CFd?5?$DM?$DN?51?$AA@ (0000)
0024D39C 0021:
	??_C@_0CB@HJKPDMEB@survivors?5?$CFd?5?$DM?$DN?575?$CF?$CF?5of?5total?5?$CFd@ (0000)
0024D3C0 0021:
	??_C@_0CB@NHMKHBJA@survivors?5?$CFd?5?$DM?$DN?550?$CF?$CF?5of?5total?5?$CFd@ (0000)
0024D3E4 0021:
	??_C@_0CB@INABNAMN@survivors?5?$CFd?5?$DM?$DN?525?$CF?$CF?5of?5total?5?$CFd@ (0000)
0024D408 0018:
	??_C@_0BI@IGDFHOAI@survivors?5?$CFd?5?$DM?5total?5?$CFd?$AA@ (0000)
0024D420 0015:
	??_C@_0BF@FJJAHOMB@strength?5?$CF?42f?5?$DM?525?$CF?$CF?$AA@ (0000)
0024D438 0015:
	??_C@_0BF@NMGOFCOD@strength?5?$CF?42f?5?$DM?550?$CF?$CF?$AA@ (0000)
0024D450 0015:
	??_C@_0BF@JBHAPBLB@strength?5?$CF?42f?5?$DM?575?$CF?$CF?$AA@ (0000)
0024D468 004a:
	??_C@_0EK@PHDJOLGD@owner_actor_indices?$FLactor?9?$DOfirin@ (0000)
0024D4B8 0093:
	??_C@_0JD@GAKCCOHE@actor?9?$DOfiring_positions?4current_@ (0000)
0024D550 005a:
	??_C@_0FK@DNNFPKMF@firing_position_owner_actor_indi@ (0000)
0024D5B0 0084:
	??_C@_0IE@GJHEMMJI@?$CIpursuit?9?$DOnext_actor_index_index@ (0000)
0024D634 0038:
	??_C@_0DI@GONNFCHM@pursuit?9?$DOfiring_position_index?5?$DN@ (0000)
0024D66C 0014:
	??_C@_0BE@MIHACDDA@found_index?5?$CB?$DN?5NONE?$AA@ (0000)
0024D680 0035:
	??_C@_0DF@FCNHHGJA@BIT_VECTOR_TEST_FLAG?$CIsquad?9?$DOunus@ (0000)
0024D6B8 0063:
	??_C@_0GD@GIHDEKAB@?$CIselected_behavior_index?5?$DO?$DN?50?$CJ?5?$CG@ (0000)
0024D720 009d:
	??_C@_0JN@COMBHAJN@?$CIprimary_postcombat_behaviors?$FLpr@ (0000)
0024D7C0 0004:
	__real@3b888889 (0000)
0024D7C8 006c:
	??_C@_0GM@MFIMGOEE@WARNING?3?5cannot?5spawn?5actors?5in?5@ (0000)
0024D834 0022:
	??_C@_0CC@EIGEOIGF@?$CFs?1?$CFs?3?5randomly?5selected?5to?5spaw@ (0000)
0024D858 002b:
	??_C@_0CL@DJPLPKCC@?$CFs?1?$CFs?3?5current?5?$CFd?5?$DM?5max?5?$CFd?5?9?$DO?5de@ (0000)
0024D884 002f:
	??_C@_0CP@JPOKCDIH@?$CFs?1?$CFs?3?5unable?5to?5spawn?0?5out?5of?5s@ (0000)
0024D8B4 002e:
	??_C@_0CO@IIDKDHP@?$CFs?1?$CFs?3?5current?5?$CFd?5?$DM?5min?5?$CFd?5?9?$DO?5sp@ (0000)
0024D8E4 0018:
	??_C@_0BI@NJPCKEMP@?$CFs?1?$CFs?5triggered?5?$CFs?5rule?$AA@ (0000)
0024D8FC 000a:
	??_C@_09MEJNCKEO@attacking?$AA@ (0000)
0024D908 000a:
	??_C@_09GNBACBHC@defending?$AA@ (0000)
0024D914 0021:
	??_C@_0CB@JPLEGLHP@?$CFs?1?$CFs?5triggered?5maneuvering?5rule@ (0000)
0024D938 0030:
	??_C@_0DA@LKCBJOBO@?$CFs?3?5current?5?$CFs?1?$CF?41f?5best?5?$CFs?1?$CF?41f@ (0000)
0024D968 0004:
	__real@c47a0000 (0000)
0024D96C 0005:
	??_C@_04NEBKMGJO@stay?$AA@ (0000)
0024D974 0008:
	??_C@_07JBBGNPIP@migrate?$AA@ (0000)
0024D980 004d:
	??_C@_0EN@MMBDPONO@WARNING?3?5squad?5?$CFs?1?$CFs?5has?5an?5inva@ (0000)
0024D9D0 004b:
	??_C@_0EL@LEBBGDPA@WARNING?3?5actor?5changing?5to?5encou@ (0000)
0024DA1C 0035:
	??_C@_0DF@OEDIHECK@?$CBencounter?9?$DOenemy_visible?5?$CG?$CG?5?$CBen@ (0000)
0024DA54 002e:
	??_C@_0CO@DFDAGGAE@?$CFs?1?$CFs?3?5?$CFd?5current?5?$CFd?5leaders?0?5cr@ (0000)
0024DA84 000a:
	??_C@_09PONLBLAH@no?5leader?$AA@ (0000)
0024DA90 000b:
	??_C@_0L@JLIMGEDN@new?5leader?$AA@ (0000)
0024DA9C 000c:
	??_C@_0M@ELGJNJDG@ai_place?5?$CFs?$AA@ (0000)
0024DAA8 000f:
	??_C@_0P@IGLHCPJF@ai_place?5?$CFs?1?$CFs?$AA@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "ai/actor_activation.h"
#include "ai/actor_iterators.h"
#include "encounters.h"

#include "actors.h"
#include "actor_definitions.h"
#include "actor_iterators.h"
#include "actor_placement.h"
#include "actor_types.h"
#include "ai.h"
#include "ai_debug.h"
#include "ai_script.h"
#include "ai_scenario_definitions.h"
#include "props.h"
#include "cseries/errors.h"
#include "editor/editor_stubs.h"
#include "game/game.h"
#include "game/game_allegiance.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "math/integer_math.h"
#include "memory/data.h"
#include "saved games/game_state.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "units/units.h"

#include <stddef.h>

/* ---------- constants */

enum
{
	MAXIMUM_ENCOUNTERS = 128,
	MAXIMUM_EXAMINED_PURSUIT_POSITIONS_PER_MAP = 256,
	NUMBER_OF_ACTOR_INDICES_PER_EXAMINED_PURSUIT_POSITION = 6,
};

enum
{
	MAXIMUM_LINK_ENCOUNTERS_PER_ENCOUNTER = 3,
	MAXIMUM_FIRING_POSITIONS_PER_ENCOUNTER = 512,
	NUMBER_OF_POST_COMBAT_POSSIBILITIES = 2,
	ENCOUNTER_REMAIN_ACTIVE_TIME = 150,
	ENCOUNTER_UPDATE_INTERVAL = TICKS_PER_SECOND/2,
};

// encounter_definition.searching
enum
{
	_encounter_searching_normal = 0,
	_encounter_searching_never,
	_encounter_searching_forever,
	NUMBER_OF_ENCOUNTER_SEARCHING_TYPES,
};

// actor_type_definition.when_to_search_at_target/when_to_pursue/when_to_search_pursuit
enum
{
	_actor_pursuit_always = 0,
	_actor_pursuit_not_when_group,
	_actor_pursuit_never,
};

// encounter_definition.flags (TU-local until ai_scenario_definitions.h names them)
enum
{
	_encounter_not_initially_created_bit = 0,
	_encounter_respawn_enabled_bit,
	_encounter_blind_bit,
	_encounter_deaf_bit,
};

// squad_definition.unique_leader_type (TU-local until ai_scenario_definitions.h names them)
enum
{
	_unique_leader_type_normal = 0,
	_unique_leader_type_none,
	_unique_leader_type_random,
	_unique_leader_type_sergeant_johnson,
	_unique_leader_type_sergeant_lehto,
	NUMBER_OF_UNIQUE_LEADER_TYPES,
};

enum
{
	_ai_reference_squad_bit = 15,
};

/* ---------- macros */

#define pursuit_get(index) ((struct pursuit_datum *)datum_get(pursuit_data, (index)))

/* ---------- structures */

struct pursuit_datum
{
	short identifier;
	short firing_position_index;
	long last_examined_time;
	short actor_count;
	short next_actor_index_index;
	long actor_indices[NUMBER_OF_ACTOR_INDICES_PER_EXAMINED_PURSUIT_POSITION];
	long next_pursuit_index;
};

struct post_combat_possibility
{
	long actor_index;
	real weight;
	long prop_index;
	long unit_index;
};

struct encounter_iterator
{
	struct data_iterator data;
	long index;
	boolean active_only;
};

struct encounter_actor_iterator
{
	long encounter_index;
	long index;
	long next_index;
};

struct actor_iterator
{
	struct data_iterator encounter_iterator;
	boolean iterated_encounterless_list;
	boolean active_only;
	byte pad[2];
	long index;
	long next_index;
};

/* `ai_profile` is owned by ai_profile.c.  Encounters only owns the January
 * encounter-update counter at +0x94, so keep the rest opaque here. */
struct ai_profile_encounter_data
{
	byte __unknown0[0x94];
	short encounter_update_count;
};

struct encounter_ai_globals_prefix
{
	boolean ai_active;
	boolean ai_initialized_for_map;
	boolean ai_has_control_data;
	boolean time_given_this_frame;
	short last_highest_service_timer;
	short current_highest_service_timer;
	long first_encounterless_actor_index;
};

typedef char encounter_iterator_size_assert[
	sizeof(struct encounter_iterator) == 0x18 ? 1 : -1];
typedef char encounter_iterator_index_offset_assert[
	offsetof(struct encounter_iterator, index) == 0x10 ? 1 : -1];
typedef char encounter_iterator_active_only_offset_assert[
	offsetof(struct encounter_iterator, active_only) == 0x14 ? 1 : -1];
typedef char encounter_actor_iterator_size_assert[
	sizeof(struct encounter_actor_iterator) == 0xC ? 1 : -1];
typedef char encounter_actor_iterator_index_offset_assert[
	offsetof(struct encounter_actor_iterator, index) == 0x4 ? 1 : -1];
typedef char encounter_actor_iterator_next_index_offset_assert[
	offsetof(struct encounter_actor_iterator, next_index) == 0x8 ? 1 : -1];
typedef char actor_iterator_size_assert[
	sizeof(struct actor_iterator) == 0x1C ? 1 : -1];
typedef char actor_iterator_iterated_encounterless_offset_assert[
	offsetof(struct actor_iterator, iterated_encounterless_list) == 0x10 ? 1 : -1];
typedef char actor_iterator_active_only_offset_assert[
	offsetof(struct actor_iterator, active_only) == 0x11 ? 1 : -1];
typedef char actor_iterator_index_offset_assert[
	offsetof(struct actor_iterator, index) == 0x14 ? 1 : -1];
typedef char actor_iterator_next_index_offset_assert[
	offsetof(struct actor_iterator, next_index) == 0x18 ? 1 : -1];
typedef char encounter_ai_globals_initialized_offset_assert[
	offsetof(struct encounter_ai_globals_prefix, ai_initialized_for_map) == 0x1 ? 1 : -1];
typedef char encounter_ai_globals_encounterless_actor_offset_assert[
	offsetof(struct encounter_ai_globals_prefix, first_encounterless_actor_index) == 0x8 ? 1 : -1];
typedef char encounter_datum_active_offset_assert[
	offsetof(struct encounter_datum, active) == 0xD ? 1 : -1];
typedef char encounter_datum_status_dirty_offset_assert[
	offsetof(struct encounter_datum, status_dirty) == 0x28 ? 1 : -1];
typedef char encounter_datum_first_actor_index_offset_assert[
	offsetof(struct encounter_datum, first_actor_index) == 0x14 ? 1 : -1];
typedef char encounter_datum_blind_offset_assert[
	offsetof(struct encounter_datum, blind) == 0x40 ? 1 : -1];
typedef char encounter_datum_deaf_offset_assert[
	offsetof(struct encounter_datum, deaf) == 0x41 ? 1 : -1];
typedef char actor_datum_next_actor_index_offset_assert[
	offsetof(struct actor_datum, meta.next_actor_index) == 0x2C ? 1 : -1];
typedef char pursuit_datum_size_assert[
	sizeof(struct pursuit_datum) == 0x28 ? 1 : -1];

/* ---------- prototypes */

static void encounter_update_squads(
	long encounter_index);
void encounter_update_respawn(
	long encounter_index);
void encounter_update_platoons(
	long encounter_index);
void encounter_update_follow(
	long encounter_index);
void encounter_control_actors(
	long encounter_index);

static void encounter_clear_pursuit(
	long encounter_index);
static boolean encounter_place_actor(
	long encounter_index,
	short squad_index,
	short initial_variant,
	boolean spawning);
static void encounterless_deactivate(
	long actor_index);
static void encounters_test_activation(
	void);
static long encounter_find_pursuit(
	long encounter_index,
	short firing_position_index,
	long history_start_time,
	boolean force_create);
static boolean encounter_activate(
	long encounter_index);
static void encounter_deactivate(
	long encounter_index);
static void encounter_update_timers(
	long encounter_index);
static short squad_get_actor_type(
	struct squad_definition *squad_definition);

/* ---------- globals */

extern struct encounter_ai_globals_prefix *ai_globals;
extern struct ai_profile_encounter_data ai_profile;

struct data_array *encounter_data;
struct platoon_datum *platoon_array;
struct squad_datum *squad_array;
struct data_array *pursuit_data;

/* ---------- public code */

void encounters_initialize(
	void)
{
	encounter_data = game_state_data_new("encounter", MAXIMUM_ENCOUNTERS, sizeof(struct encounter_datum));
	match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 110, encounter_data);

	squad_array = (struct squad_datum *)game_state_malloc("squad", "squad", MAXIMUM_SQUADS_PER_MAP * sizeof(struct squad_datum));
	match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 113, squad_array);

	platoon_array = (struct platoon_datum *)game_state_malloc("platoon", "platoon", MAXIMUM_PLATOONS_PER_MAP * sizeof(struct platoon_datum));
	match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 116, platoon_array);

	pursuit_data = game_state_data_new("ai pursuit", MAXIMUM_EXAMINED_PURSUIT_POSITIONS_PER_MAP, sizeof(struct pursuit_datum));
	match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 119, pursuit_data);

	return;
}

void encounters_dispose(
	void)
{
	return;
}

void encounters_dispose_from_old_map(
	void)
{
	data_make_invalid(encounter_data);
	data_make_invalid(pursuit_data);
	return;
}

void encounter_iterator_new(
	struct encounter_iterator *iterator,
	boolean active_only)
{
	if (ai_globals->ai_initialized_for_map)
	{
		data_iterator_new(&iterator->data, encounter_data);
		iterator->active_only = active_only;
	}

	return;
}

struct encounter_datum *encounter_iterator_next(
	struct encounter_iterator *iterator)
{
	struct encounter_datum *result = NULL;

	if (ai_globals->ai_initialized_for_map)
	{
		do
		{
			result = (struct encounter_datum *)data_iterator_next(&iterator->data);
		} while (result && iterator->active_only && !result->active);

		iterator->index = iterator->data.datum_index;
	}

	return result;
}

void encounter_actor_iterator_new(
	struct encounter_actor_iterator *iterator,
	long encounter_index)
{
	if (!ai_globals->ai_initialized_for_map)
		return;

	iterator->encounter_index = encounter_index;
	iterator->index = NONE;
	if (encounter_index == NONE)
		iterator->next_index = ai_globals->first_encounterless_actor_index;
	else
		iterator->next_index = encounter_get(encounter_index)->first_actor_index;

	return;
}

struct actor_datum *encounter_actor_iterator_next(
	struct encounter_actor_iterator *iterator)
{
	struct actor_datum *result = NULL;

	if (ai_globals->ai_initialized_for_map)
	{
		long next_index = iterator->next_index;

		iterator->index = next_index;
		if (next_index != NONE)
		{
			result = actor_get(next_index);
			iterator->next_index = result->meta.next_actor_index;
		}
	}

	return result;
}

void encounter_set_blind(
	long encounter_index,
	boolean blind)
{
	if (ai_globals->ai_initialized_for_map)
		encounter_get(encounter_index)->blind = blind;

	return;
}

void encounter_set_deaf(
	long encounter_index,
	boolean deaf)
{
	if (ai_globals->ai_initialized_for_map)
		encounter_get(encounter_index)->deaf = deaf;

	return;
}

void actor_iterator_new(
	struct actor_iterator *iterator,
	boolean active_only)
{
	if (!ai_globals->ai_initialized_for_map)
		return;

	data_iterator_new(&iterator->encounter_iterator, encounter_data);
	iterator->iterated_encounterless_list = FALSE;
	iterator->next_index = NONE;
	iterator->index = NONE;
	iterator->active_only = active_only;

	return;
}

void encounterless_activate(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	match_assert(
		"c:\\halo\\SOURCE\\ai\\encounters.c",
		0x720,
		actor->meta.encounterless);
	actor->meta.encounterless_active_timer = 90;
	actor_set_active(actor_index, TRUE);

	return;
}

void encounters_update_dirty_status(
	void)
{
	struct encounter_iterator iterator;
	struct encounter_datum *encounter;

	encounter_iterator_new(&iterator, FALSE);
	while ((encounter = encounter_iterator_next(&iterator)) != NULL)
	{
		if (encounter->status_dirty)
			encounter_update_status(iterator.index);
	}

	return;
}

long encounter_get_by_name(
	char const *encounter_name)
{
	long encounter_index = NONE;
	struct scenario *scenario = global_scenario_get();

	if (scenario)
	{
		long i;

		for (i = 0; i < scenario->ai_encounters.count; ++i)
		{
			struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
				&scenario->ai_encounters, i, struct encounter_definition);

			if (csstrncmp(encounter_definition->name, encounter_name, TAG_STRING_LENGTH+1) == 0)
			{
				encounter_index = i;
				break;
			}
		}
	}

	return encounter_index;
}

struct actor_datum *encounter_actor_iterator_prev(
	struct encounter_actor_iterator *iterator)
{
	struct actor_datum *result = NULL;

	if (ai_globals->ai_initialized_for_map)
	{
		long actor_index = encounter_get(iterator->encounter_index)->first_actor_index;
		long previous_index = NONE;

		while (actor_index != iterator->index && actor_index != NONE)
		{
			previous_index = actor_index;
			result = actor_get(actor_index);
			actor_index = result->meta.next_actor_index;
		}

		if (actor_index != iterator->index)
			return NULL;

		iterator->next_index = actor_index;
		iterator->index = previous_index;
	}

	return result;
}

struct actor_datum *actor_iterator_next(
	struct actor_iterator *iterator)
{
	struct actor_datum *result = NULL;

	if (ai_globals->ai_initialized_for_map)
	{
		if (iterator->next_index == NONE)
		{
			struct encounter_datum *encounter;

			do
			{
				encounter = (struct encounter_datum *)data_iterator_next(&iterator->encounter_iterator);
				if (!encounter)
					break;
				if (!iterator->active_only || encounter->active)
					iterator->next_index = encounter->first_actor_index;
			} while (iterator->next_index == NONE);

			if (!encounter && !iterator->iterated_encounterless_list)
			{
				iterator->next_index = ai_globals->first_encounterless_actor_index;
				iterator->iterated_encounterless_list = TRUE;
			}
		}

		do
		{
			long next_index = iterator->next_index;

			iterator->index = next_index;
			if (next_index == NONE)
				break;

			result = actor_get(next_index);
			iterator->next_index = result->meta.next_actor_index;
			if (iterator->active_only && !result->meta.active)
				result = NULL;
		} while (!result);
	}

	return result;
}

void encounter_modify_pursuit_desires(
	long encounter_index,
	short squad_index,
	boolean *pursue_tenacious,
	short *group_pursuit_restriction,
	boolean *group_pursuit_controller,
	short *desired_target_search,
	short *desired_pursuit,
	short *desired_pursuit_search)
{
	struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index), struct encounter_definition);
	struct squad_definition *squad_definition = TAG_BLOCK_GET_ELEMENT(
		&encounter_definition->squads, squad_index, struct squad_definition);
	short searching = encounter_definition->searching;

	if (TEST_FLAG(squad_definition->flags, _squad_never_search_bit))
		searching = _encounter_searching_never;

	switch (searching)
	{
	case _encounter_searching_never:
		*group_pursuit_restriction = 1;
		*desired_pursuit = _actor_pursuit_never;
		*desired_pursuit_search = _actor_pursuit_never;
		break;

	case _encounter_searching_forever:
		*pursue_tenacious = TRUE;
		*desired_target_search = _actor_pursuit_always;
		*desired_pursuit = _actor_pursuit_always;
		*desired_pursuit_search = _actor_pursuit_always;
		*group_pursuit_controller = FALSE;
		break;
	}

	return;
}

boolean encounter_link_activation(
	long encounter_index,
	short link_encounter_index)
{
	struct encounter_datum *encounter = encounter_get(encounter_index);
	boolean result = FALSE;
	short i;

	match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 1916, (link_encounter_index >= 0) && (link_encounter_index < global_scenario_get()->ai_encounters.count));

	for (i = 0; i < encounter->link_encounter_count; ++i)
	{
		if (encounter->link_encounter_indices[i] == link_encounter_index)
		{
			result = TRUE;
			break;
		}
	}

	if (!result && encounter->link_encounter_count < MAXIMUM_LINK_ENCOUNTERS_PER_ENCOUNTER)
	{
		encounter->link_encounter_indices[encounter->link_encounter_count] = link_encounter_index;
		encounter->link_encounter_count++;
		result = TRUE;
	}

	return result;
}

void encounterless_attach_actor(
	long actor_index)
{
	if (ai_globals->ai_initialized_for_map)
	{
		struct actor_datum *actor = actor_get(actor_index);

		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 758, actor->meta.encounter_index==NONE);
		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 759, !actor->meta.encounterless);

		actor->meta.next_actor_index = ai_globals->first_encounterless_actor_index;
		ai_globals->first_encounterless_actor_index = actor_index;
		actor->meta.encounterless = TRUE;
		actor->meta.encounterless_active_timer = actor->meta.active ? 90 : 0;
		actor_flush_position_indices(actor_index);
	}

	return;
}

boolean encounter_mark_examined_pursuit_position(
	long encounter_index,
	long actor_index,
	short firing_position_index,
	long history_start_time)
{
	boolean marked = FALSE;
	long pursuit_index = encounter_find_pursuit(encounter_index, firing_position_index, history_start_time, TRUE);

	if (pursuit_index != NONE)
	{
		struct pursuit_datum *pursuit = pursuit_get(pursuit_index);
		short i;

		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 1031, pursuit->firing_position_index == firing_position_index);

		for (i = 0; i < NUMBER_OF_ACTOR_INDICES_PER_EXAMINED_PURSUIT_POSITION; ++i)
		{
			if (pursuit->actor_indices[i] == actor_index)
				break;
		}

		if (i >= NUMBER_OF_ACTOR_INDICES_PER_EXAMINED_PURSUIT_POSITION)
		{
			match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 1044, (pursuit->next_actor_index_index >= 0) && (pursuit->next_actor_index_index < NUMBER_OF_ACTOR_INDICES_PER_EXAMINED_PURSUIT_POSITION));
			pursuit->actor_indices[pursuit->next_actor_index_index] = actor_index;
			pursuit->next_actor_index_index = (pursuit->next_actor_index_index+1) % NUMBER_OF_ACTOR_INDICES_PER_EXAMINED_PURSUIT_POSITION;
			pursuit->actor_count++;
			marked = TRUE;
		}

		pursuit->last_examined_time = game_time_get();
	}

	return marked;
}

void encounter_force_activate(
	long encounter_index)
{
	encounter_get(encounter_index)->remain_active_timer = ENCOUNTER_REMAIN_ACTIVE_TIME;
	encounter_activate(encounter_index);

	return;
}

void encounter_force_deactivate(
	long encounter_index)
{
	encounter_get(encounter_index)->remain_active_timer = 0;
	encounter_deactivate(encounter_index);

	return;
}

void encounter_set_respawn(
	long encounter_index,
	boolean respawn)
{
	if (ai_globals->ai_initialized_for_map)
	{
		encounter_get(encounter_index)->respawn_enabled = respawn;
		encounter_force_activate(encounter_index);
	}

	return;
}

void encounter_squad_timer_expire(
	long encounter_index,
	short squad_index)
{
	struct encounter_datum *encounter = encounter_get(encounter_index);
	struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index), struct encounter_definition);
	struct squad_datum *squad = encounter_get_squad(encounter, squad_index);
	struct squad_definition *squad_definition = TAG_BLOCK_GET_ELEMENT(
		&encounter_definition->squads, squad_index, struct squad_definition);

	squad->delay_timer = 0;

	if (TEST_FLAG(squad_definition->flags, _squad_magic_sight_after_timer_bit))
	{
		ai_scripting_magically_see_players(
			DATUM_INDEX_NEW(
				DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index),
				FLAG(_ai_reference_squad_bit) | (squad_index & UNSIGNED_CHAR_MAX)));
	}

	if (ai_debug.print_rules)
		console_printf(FALSE, "%s/%s: delay timer finished", encounter_definition->name, squad_definition->name);

	return;
}

void encounters_create_for_new_map(
	void)
{
	struct scenario *scenario = global_scenario_get();
	struct encounter_iterator iterator;
	struct encounter_datum *encounter;

	encounter_iterator_new(&iterator, FALSE);
	while ((encounter = encounter_iterator_next(&iterator)) != NULL)
	{
		struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
			&scenario->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.index), struct encounter_definition);
		boolean create = !TEST_FLAG(encounter_definition->flags, _encounter_not_initially_created_bit);

		if (DATUM_INDEX_TO_ABSOLUTE_INDEX(ai_debug.selected_squad_index) == DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.index) || create)
		{
			encounter_create(iterator.index, NONE, NONE);
		}
	}

	return;
}

void encounters_unit_died(
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);

	if (unit->object.owner_team_index != NONE)
	{
		struct encounter_iterator iterator;
		struct encounter_datum *encounter;

		encounter_iterator_new(&iterator, TRUE);
		while ((encounter = encounter_iterator_next(&iterator)) != NULL)
		{
			if (game_team_is_enemy(encounter->team_index, unit->object.owner_team_index) &&
				encounter->enemy_target && !encounter->stand_down && !encounter->post_combat)
			{
				encounter->enemies_defeated++;
			}
		}
	}

	return;
}

void encounter_attach_unit(
	long encounter_index,
	long unit_index)
{
	struct encounter_datum *encounter = encounter_get(encounter_index);
	struct unit_datum *unit = unit_get(unit_index);

	if (unit->unit.swarm_actor_index != NONE)
	{
		struct actor_datum *actor = actor_get(unit->unit.swarm_actor_index);

		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 603, actor->meta.encounter_index == encounter_index);
	}
	else
	{
		struct actor_datum *actor = actor_get(unit->unit.actor_index);

		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 609, actor->meta.encounter_index == encounter_index);
		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 610, actor->meta.unit_index == unit_index);
	}

	if (!game_engine_running() && encounter->team_index == 0)
	{
		encounter->team_index = unit->object.owner_team_index;
		if (encounter->first_actor_index != NONE)
			ai_update_team_status();
	}

	return;
}

boolean encounter_spawn_actor(
	long encounter_index,
	short squad_index)
{
	if (ai_globals->ai_initialized_for_map && encounter_place_actor(encounter_index, squad_index, 0, TRUE))
	{
		struct encounter_datum *encounter = encounter_get(encounter_index);
		struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index), struct encounter_definition);
		struct squad_datum *squad = encounter_get_squad(encounter, squad_index);
		struct squad_definition *squad_definition = TAG_BLOCK_GET_ELEMENT(
			&encounter_definition->squads, squad_index, struct squad_definition);

		encounter->current_count++;
		squad->current_count++;
		if (squad_definition->respawn_total_count > 0)
			squad->respawn_actors_left--;

		encounter->respawn_delay_ticks = real_random_range(encounter_definition->respawn_time_lower_bound, encounter_definition->respawn_time_upper_bound) * TICKS_PER_SECOND;
		squad->respawn_delay_ticks = real_random_range(squad_definition->respawn_time_lower_bound, squad_definition->respawn_time_upper_bound) * TICKS_PER_SECOND;
	}

	/* Original January and HCEA behavior: the successful placement is not reported to the caller;
	 * this routine returns FALSE on every path. */
	return FALSE;
}

void encounter_verify_firing_position_owner_actor_indices(
	long encounter_index)
{
	struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index), struct encounter_definition);
	long owner_actor_indices[MAXIMUM_FIRING_POSITIONS_PER_ENCOUNTER];
	struct encounter_actor_iterator iterator;
	struct actor_datum *actor;

	csmemset(owner_actor_indices, NONE, encounter_definition->firing_positions.count*sizeof(long));

	encounter_actor_iterator_new(&iterator, encounter_index);
	while ((actor = encounter_actor_iterator_next(&iterator)) != NULL)
	{
		if (actor->firing_positions.current_position_index != NONE)
		{
			match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 302, actor->firing_positions.current_position_index>=0 && actor->firing_positions.current_position_index < encounter_definition->firing_positions.count);
			match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 303, owner_actor_indices[actor->firing_positions.current_position_index]==NONE);
			owner_actor_indices[actor->firing_positions.current_position_index] = iterator.index;
		}
	}

	return;
}

void encounter_build_firing_position_owner_actor_indices(
	long encounter_index,
	long *firing_position_owner_actor_indices)
{
	struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index), struct encounter_definition);
	struct encounter_actor_iterator iterator;
	struct actor_datum *actor;

	csmemset(firing_position_owner_actor_indices, NONE, encounter_definition->firing_positions.count*sizeof(long));

	encounter_actor_iterator_new(&iterator, encounter_index);
	while ((actor = encounter_actor_iterator_next(&iterator)) != NULL)
	{
		if (actor->firing_positions.current_position_index != NONE)
		{
			match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 332, actor->firing_positions.current_position_index>=0 && actor->firing_positions.current_position_index < encounter_definition->firing_positions.count);
			match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 333, firing_position_owner_actor_indices[actor->firing_positions.current_position_index]==NONE);
			firing_position_owner_actor_indices[actor->firing_positions.current_position_index] = iterator.index;
		}
	}

	return;
}

void encounterless_detach_actor(
	long actor_index)
{
	if (ai_globals->ai_initialized_for_map)
	{
		struct actor_datum *actor = actor_get(actor_index);
		long *actor_index_reference;

		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 782, actor->meta.encounterless);
		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 783, actor->meta.encounter_index == NONE);
		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 784, actor->meta.squad_index == NONE);
		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 785, actor->meta.platoon_index == NONE);

		for (actor_index_reference = &ai_globals->first_encounterless_actor_index;
			*actor_index_reference != actor_index;
			actor_index_reference = &actor_get(*actor_index_reference)->meta.next_actor_index)
		{
			match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 796, *actor_index_reference!=NONE);
		}

		*actor_index_reference = actor->meta.next_actor_index;
		actor->meta.encounterless = FALSE;
		actor->meta.next_actor_index = NONE;
		actor->meta.force_active = FALSE;
	}

	return;
}

void encounter_detach_actor(
	long actor_index,
	boolean died)
{
	if (ai_globals->ai_initialized_for_map)
	{
		struct actor_datum *actor = actor_get(actor_index);

		if (actor->meta.encounter_index != NONE)
		{
			struct encounter_datum *encounter = encounter_get(actor->meta.encounter_index);
			long *actor_index_reference;

			for (actor_index_reference = &encounter->first_actor_index;
				*actor_index_reference != actor_index;
				actor_index_reference = &actor_get(*actor_index_reference)->meta.next_actor_index)
			{
				match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 544, *actor_index_reference!=NONE);
			}
			*actor_index_reference = actor->meta.next_actor_index;

			if (!died)
			{
				struct squad_datum *squad = encounter_get_squad(encounter, actor->meta.squad_index);

				match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 556, encounter->original_count > 0);
				encounter->original_count--;

				if (actor->meta.unique_leader)
				{
					match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 561, encounter->unique_leader_count > 0);
					encounter->unique_leader_count--;
				}

				match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 565, squad->original_count > 0);
				squad->original_count--;

				if (actor->meta.platoon_index != NONE)
				{
					struct platoon_datum *platoon = encounter_get_platoon(encounter, actor->meta.platoon_index);

					match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 573, platoon->original_count > 0);
					platoon->original_count--;
				}
			}

			actor->meta.next_actor_index = NONE;
			actor->meta.encounter_index = NONE;
			actor->meta.platoon_index = NONE;
			actor->meta.squad_index = NONE;
			encounter->status_dirty = TRUE;
		}
	}

	return;
}

void encounter_stand_down(
	long encounter_index)
{
	struct encounter_datum *encounter = encounter_get(encounter_index);
	struct encounter_actor_iterator iterator;
	struct actor_datum *actor;

	match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 2431, !encounter->enemy_visible);

	encounter->stand_down = TRUE;
	encounter->enemies_defeated = 0;
	encounter_clear_pursuit(encounter_index);

	encounter_actor_iterator_new(&iterator, encounter_index);
	while ((actor = encounter_actor_iterator_next(&iterator)) != NULL)
	{
		struct prop_iterator prop_iterator;
		struct prop_datum *prop;

		prop_iterator_new(&prop_iterator, iterator.index);
		while ((prop = prop_iterator_next(&prop_iterator)) != NULL)
		{
			if (prop->state >= _prop_state_uninspected_orphan && prop->state <= _prop_state_inspected_orphan &&
				prop->enemy && prop_iterator.index != actor->target.target_prop_index)
			{
				struct prop_datum *parent_prop;

				match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 2458, prop->parent_prop_index != NONE);
				parent_prop = prop_get(prop->parent_prop_index);
				match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 2463, parent_prop->orphan_prop_index == prop_iterator.index);
				parent_prop->orphan_prop_index = NONE;

				actor_switch_props(iterator.index, prop_iterator.index, NONE);
				prop_delete(iterator.index, prop_iterator.index);
			}
		}
	}

	return;
}

void encounter_attach_actor(
	long actor_index,
	long encounter_index,
	short squad_index,
	boolean has_previous_team)
{
	if (ai_globals->ai_initialized_for_map)
	{
		struct actor_datum *actor = actor_get(actor_index);
		struct actor_definition *actor_definition = actor_definition_get(actor->meta.definition_index); // January fetches the definition but never reads it
		struct encounter_datum *encounter = encounter_get(encounter_index);
		struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index), struct encounter_definition);
		struct squad_datum *squad = encounter_get_squad(encounter, squad_index);
		struct squad_definition *squad_definition = TAG_BLOCK_GET_ELEMENT(
			&encounter_definition->squads, squad_index, struct squad_definition);
		short platoon_index = squad_definition->platoon_index;
		boolean activated = FALSE;

		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 650, actor->meta.encounter_index==NONE);

		actor->meta.disconnected_encounter_index = NONE;
		actor->meta.disconnected_squad_index = NONE;
		actor->meta.next_actor_index = encounter->first_actor_index;
		encounter->first_actor_index = actor_index;

		if (platoon_index < 0 || platoon_index >= encounter_definition->platoons.count)
			platoon_index = NONE;

		actor->meta.encounter_index = encounter_index;
		actor->meta.squad_index = squad_index;
		actor->meta.platoon_index = platoon_index;

		if (actor->meta.active && !actor->meta.dormant)
		{
			encounter_get(encounter_index)->remain_active_timer = ENCOUNTER_REMAIN_ACTIVE_TIME;
			activated = encounter_activate(encounter_index);
		}

		if (!activated)
		{
			actor_set_active(actor_index, encounter->active);
			if (encounter->active)
				actor_set_dormant(actor_index, FALSE);
		}

		if (actor->meta.unit_index != NONE)
			encounter_attach_unit(encounter_index, actor->meta.unit_index);

		if (actor->meta.team_index != encounter->team_index)
		{
			if (has_previous_team)
			{
				if (encounter->current_count == 0)
				{
					encounter->team_index = actor->meta.team_index;
					ai_update_team_status();
				}
				else
				{
					error(_error_silent, "WARNING: actor changing to encounter %s/%s is being forced to change teams", encounter_definition->name, squad_definition->name);
					actor_set_team(actor_index, encounter->team_index);
				}
			}
			else
			{
				actor_set_team(actor_index, encounter->team_index);
			}
		}

		encounter->original_count++;
		squad->original_count++;
		if (actor->meta.unique_leader)
			encounter->unique_leader_count++;

		if (platoon_index != NONE)
		{
			struct platoon_datum *platoon = encounter_get_platoon(encounter, platoon_index);

			actor->external_orders.defending = platoon->defending;
			actor->emotions.currently_defending = platoon->defending;
			platoon->original_count++;
		}

		encounter->status_dirty = TRUE;
	}

	return;
}

void encounter_create(
	long encounter_index,
	short desired_platoon_index,
	short desired_squad_index)
{
	if (ai_globals->ai_initialized_for_map)
	{
		struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index), struct encounter_definition);
		boolean all_squads = desired_platoon_index == NONE && desired_squad_index == NONE;
		short squad_index;

		if (ai_debug.print_placement)
		{
			if (desired_platoon_index != NONE)
			{
				console_printf(FALSE, "ai_place %s/%s", encounter_definition->name,
					TAG_BLOCK_GET_ELEMENT(&encounter_definition->platoons, desired_platoon_index, struct platoon_definition)->name);
			}
			else if (desired_squad_index != NONE)
			{
				console_printf(FALSE, "ai_place %s/%s", encounter_definition->name,
					TAG_BLOCK_GET_ELEMENT(&encounter_definition->squads, desired_squad_index, struct squad_definition)->name);
			}
			else
			{
				console_printf(FALSE, "ai_place %s", encounter_definition->name);
			}
		}

		for (squad_index = 0; squad_index < encounter_definition->squads.count; ++squad_index)
		{
			struct squad_definition *squad_definition = TAG_BLOCK_GET_ELEMENT(
				&encounter_definition->squads, squad_index, struct squad_definition);
			short initial_variant = 0;
			short actor_count;
			short actor_type;
			short i;

			if (!all_squads && squad_index != desired_squad_index)
			{
				if (squad_definition->platoon_index == NONE || squad_definition->platoon_index != desired_platoon_index)
					continue;
			}

			switch (game_difficulty_level_get())
			{
			case _game_difficulty_level_easy:
			case _game_difficulty_level_normal:
				actor_count = squad_definition->min_count;
				break;
			case _game_difficulty_level_hard:
				actor_count = (squad_definition->max_count + squad_definition->min_count) / 2;
				break;
			case _game_difficulty_level_impossible:
				actor_count = squad_definition->max_count;
				break;
			default:
				match_vassert("c:\\halo\\SOURCE\\ai\\encounters.c", 1730, FALSE, NULL);
			}

			actor_type = squad_get_actor_type(squad_definition);

			switch (squad_definition->unique_leader_type)
			{
			case _unique_leader_type_normal:
				{
					struct encounter_datum *encounter = encounter_get(encounter_index);
					boolean create_leader = FALSE;

					if (actor_type == _actor_marine)
					{
						if (encounter->unique_leader_count == 0)
							create_leader = encounter->original_count + actor_count >= 4;
						else if (encounter->unique_leader_count == 1)
							create_leader = encounter->original_count + actor_count >= 10;
					}

					if (ai_debug.print_placement)
					{
						console_printf(FALSE, "%s/%s: %d current %d leaders, create %d -> %s",
							encounter_definition->name, squad_definition->name,
							encounter->original_count, encounter->unique_leader_count, actor_count,
							create_leader ? "new leader" : "no leader");
					}

					if (!create_leader)
						break;
				}
				// fall through
			case _unique_leader_type_random:
				if (actor_type == _actor_marine)
					initial_variant = 100 + random_range(0, 2);
				break;
			case _unique_leader_type_sergeant_johnson:
				if (actor_type == _actor_marine)
					initial_variant = 100;
				break;
			case _unique_leader_type_sergeant_lehto:
				if (actor_type == _actor_marine)
					initial_variant = 101;
				break;
			case _unique_leader_type_none:
				break;
			}

			for (i = 0; i < actor_count; ++i)
			{
				encounter_place_actor(encounter_index, squad_index, initial_variant, FALSE);
				initial_variant = 0;
			}
		}

		encounter_update_status(encounter_index);
		encounters_test_activation();
	}

	return;
}

void encounters_update(
	void)
{
	long time = game_time_get();
	short phase;
	struct encounter_iterator iterator;
	struct encounter_datum *encounter;

	if (time % TICKS_PER_SECOND == 0)
	{
		encounters_update_dirty_status();
		encounters_test_activation();
	}
	phase = time % ENCOUNTER_UPDATE_INTERVAL;

	encounter_iterator_new(&iterator, TRUE);
	while ((encounter = encounter_iterator_next(&iterator)) != NULL)
	{
		long encounter_index = iterator.index;

		ai_profile.encounter_update_count++;
		if (DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index) % ENCOUNTER_UPDATE_INTERVAL == phase)
		{
			encounter_update_status(encounter_index);
			encounter_update_timers(encounter_index);
			encounter_update_respawn(encounter_index);
			encounter_update_squads(encounter_index);
			encounter_update_platoons(encounter_index);
			encounter_update_follow(encounter_index);
			encounter_control_actors(encounter_index);
		}
	}

	return;
}

/* ---------- private code */

static void encounter_clear_pursuit(
	long encounter_index)
{
	struct encounter_datum *encounter = encounter_get(encounter_index);
	long pursuit_index;

	for (pursuit_index = encounter->first_pursuit_index; pursuit_index != NONE; pursuit_index = encounter->first_pursuit_index)
	{
		encounter->first_pursuit_index = pursuit_get(pursuit_index)->next_pursuit_index;
		datum_delete(pursuit_data, pursuit_index);
	}

	return;
}

static long encounter_find_pursuit(
	long encounter_index,
	short firing_position_index,
	long history_start_time,
	boolean force_create)
{
	struct encounter_datum *encounter = encounter_get(encounter_index);
	long pursuit_index = encounter->first_pursuit_index;
	boolean reset = FALSE;

	while (pursuit_index != NONE)
	{
		struct pursuit_datum *pursuit = pursuit_get(pursuit_index);

		if (pursuit->firing_position_index == firing_position_index)
		{
			if (pursuit->last_examined_time < history_start_time)
				reset = TRUE;
			break;
		}

		pursuit_index = pursuit->next_pursuit_index;
	}

	if (pursuit_index == NONE)
	{
		if (force_create)
		{
			pursuit_index = datum_new(pursuit_data);
			if (pursuit_index != NONE)
			{
				struct pursuit_datum *pursuit = pursuit_get(pursuit_index);

				pursuit->firing_position_index = firing_position_index;
				pursuit->next_pursuit_index = encounter->first_pursuit_index;
				encounter->first_pursuit_index = pursuit_index;
				reset = TRUE;
			}
			else
			{
				error(_error_silent, "WARNING: too many actors searching, exceeded MAXIMUM_EXAMINED_PURSUIT_POSITIONS_PER_MAP (%d)", MAXIMUM_EXAMINED_PURSUIT_POSITIONS_PER_MAP);
			}
		}
	}

	if (reset)
	{
		struct pursuit_datum *pursuit = pursuit_get(pursuit_index);

		pursuit->last_examined_time = NONE;
		pursuit->actor_count = 0;
		pursuit->next_actor_index_index = 0;
		csmemset(pursuit->actor_indices, NONE, sizeof(pursuit->actor_indices));

		if (!force_create)
			return NONE;
	}

	return pursuit_index;
}

static boolean encounter_activate(
	long encounter_index)
{
	struct encounter_datum *encounter = encounter_get(encounter_index);
	struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index), struct encounter_definition);

	if (encounter_definition->runtime_structure_bsp_reference_index == NONE ||
		encounter_definition->runtime_structure_bsp_reference_index == global_structure_bsp_index)
	{
		if (!encounter->active)
		{
			struct encounter_actor_iterator iterator;

			encounter_actor_iterator_new(&iterator, encounter_index);
			while (encounter_actor_iterator_next(&iterator))
				actor_set_active(iterator.index, TRUE);
		}

		encounter->last_active_time = game_time_get();
		encounter->active = TRUE;
	}

	return encounter->active;
}

static void encounter_deactivate(
	long encounter_index)
{
	struct encounter_actor_iterator iterator;
	struct actor_datum *actor;

	encounter_get(encounter_index)->active = FALSE;
	encounter_clear_pursuit(encounter_index);

	encounter_actor_iterator_new(&iterator, encounter_index);
	while ((actor = encounter_actor_iterator_next(&iterator)) != NULL)
	{
		if (actor->meta.active)
			actor_set_active(iterator.index, FALSE);
		actor_verify_activation(iterator.index);
	}

	return;
}

static short squad_get_actor_type(
	struct squad_definition *squad_definition)
{
	struct scenario *scenario = global_scenario_get();
	short actor_palette_index = squad_definition->actor_palette_index;
	short actor_type = _actor_none;

	if (actor_palette_index >= 0 && actor_palette_index < scenario->ai_actor_palette.count)
	{
		long actor_variant_definition_index = TAG_BLOCK_GET_ELEMENT(
			&scenario->ai_actor_palette, actor_palette_index, struct tag_reference)->index;

		if (actor_variant_definition_index != NONE)
		{
			long actor_definition_index = actor_variant_definition_get(actor_variant_definition_index)->actor_reference.index;

			if (actor_definition_index != NONE)
				actor_type = actor_definition_get(actor_definition_index)->type;
		}
	}

	return actor_type;
}

static boolean encounter_post_combat_add_possibility(
	struct post_combat_possibility *possibility_array,
	long actor_index,
	real weight,
	long prop_index,
	long unit_index)
{
	boolean added = FALSE;
	short i, j;

	for (i = 0; i < NUMBER_OF_POST_COMBAT_POSSIBILITIES; ++i)
	{
		if (weight > possibility_array[i].weight)
		{
			for (j = NUMBER_OF_POST_COMBAT_POSSIBILITIES-1; j > i; --j)
				possibility_array[j] = possibility_array[j-1];

			possibility_array[i].weight = weight;
			possibility_array[i].actor_index = actor_index;
			possibility_array[i].prop_index = prop_index;
			possibility_array[i].unit_index = unit_index;
			added = TRUE;
		}
	}

	return added;
}

static boolean encounter_place_actor(
	long encounter_index,
	short squad_index,
	short initial_variant,
	boolean spawning)
{
	struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index), struct encounter_definition);
	struct squad_definition *squad_definition = TAG_BLOCK_GET_ELEMENT(
		&encounter_definition->squads, squad_index, struct squad_definition);
	short starting_location_index = encounter_get_actor_starting_location(encounter_index, squad_index, spawning);

	if (starting_location_index != NONE)
	{
		struct actor_starting_location *starting_location = TAG_BLOCK_GET_ELEMENT(
			&squad_definition->starting_locations, starting_location_index, struct actor_starting_location);
		short actor_palette_index = squad_definition->actor_palette_index;

		if (starting_location->actor_variant_index != NONE)
			actor_palette_index = starting_location->actor_variant_index;

		if (actor_palette_index >= 0 && actor_palette_index < global_scenario_get()->ai_actor_palette.count)
		{
			struct tag_reference *actor_palette_entry = TAG_BLOCK_GET_ELEMENT(
				&global_scenario_get()->ai_actor_palette, actor_palette_index, struct tag_reference);

			if (actor_palette_entry->index != NONE)
			{
				struct actor_variant_definition *actor_variant_definition = actor_variant_definition_get(actor_palette_entry->index);
				boolean upgrade_major = FALSE;

				if (actor_variant_definition->major_upgrade_reference.index != NONE)
				{
					boolean random = FALSE;
					real chance = 0.f;

					ai_get_major_upgrade_chance(squad_definition->major_upgrade, &upgrade_major, &random, &chance);
					if (random)
						upgrade_major = ai_consider_major_upgrade(encounter_index, squad_index, chance);
				}

				return actor_place(actor_palette_entry->index, encounter_index, squad_index, starting_location, upgrade_major, initial_variant) != NONE;
			}
		}
		else
		{
			error(_error_silent, "WARNING: cannot spawn actors in %s/%s because the actor variant specified for this squad is NONE or invalid", encounter_definition->name, squad_definition->name);
		}
	}

	return FALSE;
}

static void encounter_update_timers(
	long encounter_index)
{
	struct encounter_datum *encounter = encounter_get(encounter_index);

	if (encounter->enemy_visible)
		encounter->enemy_visible_timer = 0;
	else if (encounter->enemy_visible_timer != NONE)
		encounter->enemy_visible_timer += ENCOUNTER_UPDATE_INTERVAL;

	if (encounter->enemy_alive)
		encounter->enemy_alive_timer = 0;
	else if (encounter->enemy_alive_timer != NONE)
		encounter->enemy_alive_timer += ENCOUNTER_UPDATE_INTERVAL;

	if (encounter->post_combat && encounter->post_combat_delay)
	{
		if (encounter->post_combat_delay_timer > ENCOUNTER_UPDATE_INTERVAL)
			encounter->post_combat_delay_timer -= ENCOUNTER_UPDATE_INTERVAL;
		else
			encounter->post_combat_delay_timer = 0;
	}

	return;
}

static void encounter_update_squads(
	long encounter_index)
{
	struct encounter_datum *encounter = encounter_get(encounter_index);
	struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
		&global_scenario_get()->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index), struct encounter_definition);
	short squad_index;

	for (squad_index = 0; squad_index < encounter->squad_count; ++squad_index)
	{
		struct squad_datum *squad = encounter_get_squad(encounter, squad_index);
		struct squad_definition *squad_definition = TAG_BLOCK_GET_ELEMENT(
			&encounter_definition->squads, squad_index, struct squad_definition);

		if (squad->delay_timer > 0 && !TEST_FLAG(squad_definition->flags, _squad_delay_forever_bit))
		{
			if (squad->delay_timer_started)
			{
				if (squad->delay_timer > ENCOUNTER_UPDATE_INTERVAL)
					squad->delay_timer -= ENCOUNTER_UPDATE_INTERVAL;
				else
					encounter_squad_timer_expire(encounter_index, squad_index);
			}
			else
			{
				squad->delay_timer_started = TEST_FLAG(squad_definition->flags, _squad_timer_starts_immediately_bit) ||
					encounter->current_in_combat_count > 0;
				if (squad->delay_timer_started && ai_debug.print_rules)
				{
					console_printf(FALSE, "%s/%s: delay timer started (%.1f sec)",
						encounter_definition->name, squad_definition->name, squad_definition->squad_delay_timer);
				}
			}
		}
	}

	return;
}

static void encounterless_deactivate(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 1838, actor->meta.encounterless);
	actor->meta.encounterless_active_timer = 0;
	actor_set_active(actor_index, FALSE);

	return;
}

static void encounters_test_activation(
	void)
{
	struct structure_bsp *structure_bsp = global_structure_bsp_get();
	unsigned long const *combined_pvs = players_get_combined_pvs();
	unsigned long activation_cluster_bit_vector[BIT_VECTOR_SIZE_IN_LONGS(MAXIMUM_CLUSTERS_PER_STRUCTURE)];
	struct data_iterator iterator;
	struct encounter_datum *encounter;
	struct actor_datum *actor;
	long actor_index;

	for (actor_index = ai_globals->first_encounterless_actor_index; actor_index != NONE; actor_index = actor->meta.next_actor_index)
	{
		actor = actor_get(actor_index);
		match_assert("c:\\halo\\SOURCE\\ai\\encounters.c", 2234, actor->meta.encounterless);

		if (actor->meta.swarm)
		{
			actor->meta.dormant_desire = TRUE;

			if (actor->meta.swarm_cache_index == NONE)
			{
				long unit_index = actor->meta.swarm_unit_index;

				while (unit_index != NONE)
				{
					struct unit_datum *unit = unit_get(unit_index);
					short cluster_index = object_get(object_get_ultimate_parent(unit_index))->object.location.cluster_index;

					if (cluster_index != NONE && BIT_VECTOR_TEST_FLAG(combined_pvs, cluster_index))
					{
						actor->meta.dormant_desire = FALSE;
						break;
					}

					unit_index = unit->unit.swarm_next_unit_index;
				}
			}
			else
			{
				struct swarm_datum *swarm = swarm_get(actor->meta.swarm_cache_index);
				short i;

				for (i = 0; i < swarm->unit_count; ++i)
				{
					short cluster_index = object_get(object_get_ultimate_parent(swarm->unit_indices[i]))->object.location.cluster_index;

					if (cluster_index != NONE && BIT_VECTOR_TEST_FLAG(combined_pvs, cluster_index))
					{
						actor->meta.dormant_desire = FALSE;
						break;
					}
				}
			}
		}
		else
		{
			short cluster_index = object_get(object_get_ultimate_parent(actor->meta.unit_index))->object.location.cluster_index;

			if (cluster_index == NONE)
				actor->meta.dormant_desire = TRUE;
			else
				actor->meta.dormant_desire = !BIT_VECTOR_TEST_FLAG(combined_pvs, cluster_index);
		}

		if (!actor->meta.dormant_desire || actor->meta.force_active || game_in_editor() || ai_debug.force_all_active)
		{
			encounterless_activate(actor_index);
		}
		else if (actor->meta.encounterless_active_timer > TICKS_PER_SECOND)
		{
			actor->meta.encounterless_active_timer -= TICKS_PER_SECOND;
		}
		else
		{
			actor->meta.encounterless_active_timer = 0;
			encounterless_deactivate(actor_index);
		}

		actor_verify_activation(actor_index);
	}

	data_iterator_new(&iterator, encounter_data);
	while ((encounter = (struct encounter_datum *)data_iterator_next(&iterator)) != NULL)
	{
		long encounter_index = iterator.datum_index;
		struct encounter_definition *encounter_definition = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->ai_encounters, DATUM_INDEX_TO_ABSOLUTE_INDEX(encounter_index), struct encounter_definition);
		boolean force_active = encounter->force_active || game_in_editor() || encounter->respawn_delay_ticks > 0 || ai_debug.force_all_active;
		boolean active = FALSE;

		if (encounter_definition->runtime_structure_bsp_reference_index == NONE ||
			encounter_definition->runtime_structure_bsp_reference_index == global_structure_bsp_index)
		{
			encounter_compute_activation_cluster_bit_vector(encounter_index, TRUE, MAXIMUM_CLUSTERS_PER_STRUCTURE, combined_pvs, activation_cluster_bit_vector);
			active = bit_vector_and(structure_bsp->clusters.count, combined_pvs, activation_cluster_bit_vector, NULL) || force_active;
		}

		if (active)
		{
			encounter->remain_active_timer = ENCOUNTER_REMAIN_ACTIVE_TIME;
			encounter_activate(encounter_index);
		}
		else if (encounter->active && encounter->remain_active_timer > TICKS_PER_SECOND)
		{
			encounter->remain_active_timer -= TICKS_PER_SECOND;
		}
		else
		{
			boolean link_active = FALSE;
			short i;

			for (i = 0; i < encounter->link_encounter_count; ++i)
			{
				if (encounter_get(encounter->link_encounter_indices[i])->remain_active_timer > 0)
					link_active = TRUE;
			}

			encounter->remain_active_timer = 0;
			if (link_active)
				encounter_activate(encounter_index);
			else
				encounter_deactivate(encounter_index);
		}
	}

	return;
}
