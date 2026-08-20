/*
ACTOR_PERCEPTION.C

symbols in this file:
0001D8C0 00d0:
	_actor_perception_acknowledge (0000)
0001D990 00f0:
	_actor_get_perception_knowledge (0000)
0001DA80 0140:
	_actor_get_vision_distances (0000)
0001DBC0 0040:
	_code_0001dbc0 (0000)
0001DC00 00f0:
	_code_0001dc00 (0000)
0001DCF0 0230:
	_actor_perception_desire_prop (0000)
0001DF20 00a0:
	_actor_perception_find_prop_pathfinding_location (0000)
0001DFC0 00c0:
	_actor_perception_find_killer_prop_index (0000)
0001E080 0010:
	_arctangent (0000)
0001E090 00f0:
	_actor_perception_find_recent_damaging_prop_index (0000)
0001E180 0050:
	_actor_perception_forget_recent_damage (0000)
0001E1D0 0060:
	_actor_perception_retreat_successful (0000)
0001E230 00f0:
	_actor_compute_prop_unopposable (0000)
0001E320 03a0:
	_actor_compute_prop_target_weight (0000)
0001E6C0 0200:
	_actor_situation_update_target_status (0000)
0001E8C0 0140:
	_actor_situation_combat_status_update (0000)
0001EA00 04f0:
	_actor_situation_update (0000)
0001EEF0 00f0:
	_actor_situation_try_new_target (0000)
0001EFE0 01b0:
	_actor_perception_friend_prop_is_attacking (0000)
0001F190 0190:
	_actor_perception_aiming_vector_test_blockage (0000)
0001F320 0150:
	_actor_emotion_flee_with_friends (0000)
0001F470 0080:
	_code_0001f470 (0000)
0001F4F0 0070:
	_code_0001f4f0 (0000)
0001F560 04f0:
	_code_0001f560 (0000)
0001FA50 00b0:
	_actor_berserk (0000)
0001FB00 0360:
	_actor_visibility_at_point (0000)
0001FE60 0240:
	_actor_audibility_at_point (0000)
000200A0 0170:
	_actor_perception_find_sense_position (0000)
00020210 01f0:
	_code_00020210 (0000)
00020400 0380:
	_prop_position_refresh (0000)
00020780 0210:
	_code_00020780 (0000)
00020990 05c0:
	_code_00020990 (0000)
00020F50 0180:
	_actor_expected_acknowledgement (0000)
000210D0 0090:
	_actor_perception_unreachable (0000)
00021160 0060:
	_actor_perception_tried_to_uncover (0000)
000211C0 0060:
	_actor_perception_tried_to_search (0000)
00021220 00a0:
	_actor_perception_abandoned_search (0000)
000212C0 0680:
	_actor_emotion_update (0000)
00021940 0110:
	_actor_perception_become_acknowledged (0000)
00021A50 0e60:
	_prop_status_refresh (0000)
000228B0 06d0:
	_code_000228b0 (0000)
00022F80 0310:
	_actor_perception_create_orphan_from_friend (0000)
00023290 0970:
	_code_00023290 (0000)
00023C00 1270:
	_actor_perception_update (0000)
00245AB8 0038:
	_global_combat_status_table (0000)
	_global_acknowledgement_speeds (0018)
00245AF0 0020:
	??_C@_0CA@OIEKNKJL@prop?9?$DOorphan_prop_index?5?$DN?$DN?5NONE?$AA@ (0000)
00245B10 0018:
	??_C@_0BI@EKGDDPPJ@prop_acknowledged?$CIprop?$CJ?$AA@ (0000)
00245B28 0027:
	??_C@_0CH@HJCPLECH@prop?9?$DOowner_actor_index?5?$DN?$DN?5actor@ (0000)
00245B50 0025:
	??_C@_0CF@BOOBPIOF@c?3?2halo?2SOURCE?2ai?2actor_percepti@ (0000)
00245B78 0004:
	__real@42100000 (0000)
00245B7C 0004:
	__real@43610000 (0000)
00245B80 0004:
	__real@44c80000 (0000)
00245B84 0022:
	??_C@_0CC@ECCIDOIG@damaging_prop_index?5?$CB?$DN?50x0000000@ (0000)
00245BA8 0014:
	??_C@_0BE@JCINMMJG@prop_orphaned?$CIprop?$CJ?$AA@ (0000)
00245BBC 0013:
	??_C@_0BD@GACNHPJP@target_prop?9?$DOenemy?$AA@ (0000)
00245BD0 005f:
	??_C@_0FP@LFCAOKBD@?$CIactor?9?$DOtarget?4target_type?5?$DO?$DN?50?$CJ@ (0000)
00245C30 003a:
	??_C@_0DK@BNBOMGMC@?$CIactor_type?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIactor_type@ (0000)
00245C6C 0010:
	??_C@_0BA@NODDFKHB@new_prop?9?$DOenemy?$AA@ (0000)
00245C80 004c:
	??_C@_0EM@HGNPMJBO@prop_acknowledged?$CIfriend_prop?$CJ?5?$CG@ (0000)
00245CCC 0004:
	__real@bf4ccccd (0000)
00245CD0 0004:
	__real@3f9ae148 (0000)
00245CD4 0004:
	__real@3eb851ec (0000)
00245CD8 0004:
	__real@bf490fdb (0000)
00245CDC 0004:
	__real@3f060a92 (0000)
00245CE0 0004:
	__real@3e19999a (0000)
00245CE4 0004:
	__real@3fd55555 (0000)
00245CE8 0004:
	__real@40010204 (0000)
00245CEC 0004:
	__real@3ee66666 (0000)
00245CF0 0018:
	??_C@_0BI@GICPJHMO@best_unit_index?5?$CB?$DN?5NONE?$AA@ (0000)
00245D08 0025:
	??_C@_0CF@GHOKMPCH@actor?9?$DOmeta?4swarm_unit_index?5?$CB?$DN?5@ (0000)
00245D30 0021:
	??_C@_0CB@MPKGJDOK@actor?9?$DOmeta?4swarm_unit_count?5?$DO?50@ (0000)
00245D54 003b:
	??_C@_0DL@MCFLLIBG@?$CIexisting_unit_index?5?$DN?$DN?5NONE?$CJ?5?$HM?$HM@ (0000)
00245D90 0004:
	__real@40100000 (0000)
00245D94 0018:
	??_C@_0BI@LJGDFJPJ@swarm_actor?9?$DOmeta?4swarm?$AA@ (0000)
00245DAC 0004:
	__real@3a91a2b4 (0000)
00245DB0 002f:
	??_C@_0CP@MPBEBNC@object?9?$DOobject?4type?5?$DN?$DN?5_object_t@ (0000)
00245DE0 0004:
	__real@383a69dc (0000)
00245DE8 00c6:
	??_C@_0MG@IKFAABEC@?$CIactor?9?$DOdanger_zone?4danger_type?5@ (0000)
00245EB0 0008:
	__real@3ff8000000000000 (0000)
00245EB8 0015:
	??_C@_0BF@NEEODENG@?$CBprop_orphaned?$CIprop?$CJ?$AA@ (0000)
00245ED0 0008:
	__real@bfa7a8d000000000 (0000)
00245ED8 0004:
	__real@3f680347 (0000)
00245EDC 0004:
	__real@3f7e147b (0000)
00245EE0 0004:
	__real@bb5a740e (0000)
00245EE4 0004:
	__real@bc888889 (0000)
00245EE8 0004:
	__real@bd088889 (0000)
00245EEC 0004:
	__real@3c888889 (0000)
00245EF0 0004:
	__real@3b5a740e (0000)
00245EF4 0006:
	??_C@_05KKADFBMH@?$CBdead?$AA@ (0000)
00245EFC 0038:
	??_C@_0DI@GEDMEBOM@current_orphan?9?$DOparent_prop_inde@ (0000)
00245F34 0038:
	??_C@_0DI@CJNILOBC@current_prop?9?$DOorphan_prop_index?5@ (0000)
00245F6C 0031:
	??_C@_0DB@FLDFMNOK@current_orphan?9?$DOowner_actor_inde@ (0000)
00245FA0 002f:
	??_C@_0CP@JODIIBNI@current_prop?9?$DOowner_actor_index?5@ (0000)
00245FD0 001e:
	??_C@_0BO@PHJOMEBD@prop_orphaned?$CIcurrent_orphan?$CJ?$AA@ (0000)
00245FF0 0022:
	??_C@_0CC@OGFEAFJH@prop_unacknowledged?$CIcurrent_prop@ (0000)
00246014 003c:
	??_C@_0DM@LBOGCEIF@actor_perception_refresh?5overflo@ (0000)
00246050 0008:
	??_C@_07BJLCCHPO@friends?$AA@ (0000)
00246058 0008:
	??_C@_07OMOEAPJD@enemies?$AA@ (0000)
00246060 0004:
	__real@3f31c71c (0000)
00246064 000c:
	??_C@_0M@JMHOJBDN@?$CBprop?9?$DOdead?$AA@ (0000)
00246070 0020:
	??_C@_0CA@JNPJKEOJ@prop?9?$DOparent_prop_index?5?$CB?$DN?5NONE?$AA@ (0000)
00246090 0017:
	??_C@_0BH@IAHDHPJP@new_state?$CB?$DNprop?9?$DOstate?$AA@ (0000)
002460A8 0012:
	??_C@_0BC@COANDKJL@?$CFs?3?5become?5aware?$CB?$AA@ (0000)
002460C0 0041:
	??_C@_0EB@HOGHONEE@?5?5awareness?5delta?3?5?$CF?42f?5?$CIcurrent@ (0000)
00246104 002b:
	??_C@_0CL@DNKONGGM@?$CFs?3?5knowledge?5?$CFs?5percep?5?$CFs?5?9?$DO?5aw@ (0000)
00246130 000d:
	??_C@_0N@OCHNCJBI@unmistakable?$AA@ (0000)
00246140 0005:
	??_C@_04PLMLMMEO@full?$AA@ (0000)
00246148 0008:
	??_C@_07JHIHCBKH@partial?$AA@ (0000)
00246150 0009:
	??_C@_08GMLBJMKA@definite?$AA@ (0000)
0024615C 000a:
	??_C@_09HJLAOOGF@searching?$AA@ (0000)
00246168 0008:
	??_C@_07IFJEFPGA@instant?$AA@ (0000)
00246170 0006:
	??_C@_05DAFACFLE@never?$AA@ (0000)
00246178 0051:
	??_C@_0FB@GKMFLAFD@?$CIprop?9?$DOperception?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIprop@ (0000)
002461D0 004c:
	??_C@_0EM@LBLHCEME@?$CIknowledge_type?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIknowle@ (0000)
0024621C 0018:
	??_C@_0BI@PALENGAG@?$CFs?3?5stop?5becoming?5aware?$AA@ (0000)
00246234 001a:
	??_C@_0BK@OHGMLFML@?$CFs?3?5start?5to?5become?5aware?$AA@ (0000)
00246250 0024:
	??_C@_0CE@DMMCLDJG@?$CBrefresh_status?5?$HM?$HM?5refresh_posit@ (0000)
00246274 0031:
	??_C@_0DB@OOOKPDMG@parent_prop?9?$DOorphan_prop_index?5?$DN@ (0000)
002462A8 002b:
	??_C@_0CL@GMGBPAA@prop?9?$DOunopposable_casualties_inf@ (0000)
002B6AE0 0004:
	_data_002b6ae0 (0000)
*/

/* ---------- headers */

#include "cseries.h"

#include "actor_definitions.h"
#include "actors.h"
#include "ai_debug.h"
#include "ai_profile.h"
#include "encounters.h"
#include "game/game.h"
#include "items/projectile_definitions.h"
#include "items/weapon_definitions.h"
#include "math/integer_math.h"
#include "props.h"
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"
#include "units/bipeds.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "units/vehicle_definitions.h"
#include "units/vehicles.h"
#include "units/biped_definitions.h"

/* ---------- constants */

enum
{
	_actor_danger_zone_suicide = 1,
	_actor_danger_zone_projectile,
	_actor_danger_zone_vehicle = 3
};

/* ---------- macros */

/*
 * January's Actor Perception names these shared HCEX actor slots
 * differently.  Keep the aliases typed through actor_datum rather than
 * overlaying the datum with an incompatible structure type.
 */
#define actor_perception_preferred_target_prop_index(actor) \
	((actor)->meta.interesting_orphan_index)

#define actor_perception_target_weight_combat_status(actor) \
	((actor)->state.combat_status)

#define actor_perception_audibility_combat_status(actor) \
	((actor)->state.mode)

#define actor_perception_distance_squared2d(a, b, delta_x, delta_y) \
	((delta_x) = (b)->x - (a)->x, \
		(delta_y) = (b)->y - (a)->y, \
		(delta_x) * (delta_x) + (delta_y) * (delta_y))

/* ---------- structures */

union actor_perception_prop_actor_slot
{
	long actor_index;

	struct
	{
		byte padding[3];
		boolean too_far;
	};
};

struct actor_perception_swarm_component_view
{
	short identifier;
	word flags;
	real_point3d position;
	byte __unknown010[SWARM_COMPONENT_DATUM_SIZE - 0x10];
};

struct actor_perception_ai_debug_state_view
{
	byte __unknown000[6];
	boolean perception_ignore_player_props;
	boolean perception_blind_player_props;
	byte __unknown008[3];
	boolean perception_blind_all_props;
	boolean perception_deaf_all_props;
	byte __unknown00D[0x94];
	boolean perception_trace_player_awareness;
};

struct actor_perception_debug_info_view
{
	byte __unknown0000[0x6578];
	short perception_awareness_speed;
	short evaluation_mode;
};

struct ai_profile_prop_counters
{
	byte __unknown000[0x4C8];
	short dead_acknowledged;
	byte __unknown4CA[0x86];
	short dead_orphan;
	byte __unknown552[0x86];
	short dead_unacknowledged;
	byte __unknown5DA[0x86];
	short enemy_acknowledged;
	byte __unknown662[0x86];
	short enemy_orphan;
	byte __unknown6EA[0x86];
	short enemy_unacknowledged;
	byte __unknown772[0x86];
	short friend_acknowledged;
	byte __unknown7FA[0x86];
	short friend_orphan;
	byte __unknown882[0x86];
	short friend_unacknowledged;
	byte __unknown90A[0x5D6];
};

struct ai_profile_globals
{
	long __unknown0;
	boolean enabled;
	byte __unknown5[7];

	union
	{
		byte map_data[0xEE0];
		struct ai_profile_prop_counters perception;
	} data;
};

#define swarm_component_get(index) \
	((struct actor_perception_swarm_component_view *)datum_get( \
		swarm_component_data, (index)))

#define actor_perception_ai_debug_get() \
	((struct actor_perception_ai_debug_state_view *)&ai_debug)

#define actor_perception_debug_get(index) \
	((struct actor_perception_debug_info_view *)&actor_debug_array[(index)])

#define actor_perception_object_get(index) \
	((struct object_datum *)object_get_and_verify_type( \
		(index), _object_mask_all))

/*
 * January actor/prop fields whose HCEX-derived shared structure positions do
 * not agree with this executable. Keep the executable-specific view local
 * until the complete January layouts are recovered.
 */
struct actor_danger_zone_view
{
	short danger_type;
	short hostility;
	short acknowledgement_timer;
	boolean currently_perceived;
	boolean noticed_danger;
	boolean allow_dive_evasion;
	boolean communicated;
	boolean attached_to_us;
	byte __unknown00B;
	long object_index;
	long owner_unit_index;
	real danger_radius;
	real_point3d initial_position;
	real_vector3d initial_velocity;
	real_point3d position;
	real_vector3d velocity;
	real_point3d predict_danger_position;
	real current_distance_from_actor;
	real bounding_sphere_radius;
	real_point3d bounding_sphere_center;
	short predicted_impact_ticks;
	byte __unknown06A[2];
};

struct actor_perception_actor_view
{
	byte __unknown000[4];
	short type;
	boolean swarm;
	boolean frozen;
	boolean active;
	byte __unknown009[0xA];
	boolean dormant;
	byte __unknown014[4];
	long unit_index;
	byte __unknown01C[8];
	long swarm_unit_index;
	long swarm_cache_index;
	byte __unknown02C[8];
	long encounter_index;
	byte __unknown038[2];
	short encounter_squad_index;
	short encounter_platoon_index;
	short team;
	byte __unknown040[0x14];
	long preferred_target_prop_index;
	long definition_index;
	long variant_definition_index;
	byte __unknown060[0xA];
	short combat_status;
	short friend_state;
	short artificial_combat_status;
	short suspicion_combat_status;
	short target_combat_status;
	short transient_combat_status;
	long transient_combat_status_time;
	long combat_status_timer;
	long certain_combat_status_timer;
	long combat_status_decay_timer;
	long uncertain_combat_status_timer;
	boolean combat_status_high;
	byte __unknown08D[0x1B];
	short friend_fighting_count;
	byte __unknown0AA[0x9A];
	long body_leaf_index;
	short body_cluster_index;
	byte __unknown14A[0xE];
	long vehicle_index;
	boolean in_midair;
	boolean underwater;
	byte __unknown15E[3];
	boolean flying;
	boolean long_orphan_inspection;
	byte __unknown163[0x69];
	boolean corpse_interest_inhibited;
	byte __unknown1CD[7];
	short danger_relationship_type;
	byte __unknown1D6[2];
	union
	{
		long danger_relationship_handle;

		struct
		{
			byte __unknown1D8[2];
			byte danger_relationship_variant;
			byte __unknown1DB;
		};
	};
	byte __unknown1DC[0x10];
	char nearby_fighting_friend_count;
	boolean searching;
	byte __unknown1EE[0x14];
	boolean vehicle_passenger;
	byte __unknown203[0x65];
	union
	{
		short target_type;
		struct
	{
		short target_type;
	} target;
	};
	byte __unknown26A[2];
	long target_last_visible_time;
	long target_prop_index;
	byte __unknown274[8];
	boolean target_outside_active_area;
	byte __unknown27D[3];
	struct actor_danger_zone_view danger_zone;
	byte __unknown2EC[0x1C];
	short active_threat_count;
	byte __unknown30A[0x40];
	short pending_combat_status;
	long pending_combat_status_time;
	byte __unknown350[0x27];
	boolean sighted_friendly_player;
	boolean berserk;
	byte __unknown379[0x27];
	long corpse_ignore_time;
	byte __unknown3A4[4];
	short unopposable_retreat_timer;
	byte __unknown3AA[2];
	long unopposable_retreat_prop_index;
};

struct actor_perception_prop_view
{
	byte __unknown000[4];
	long owner_actor_index;
	byte __unknown008[4];
	long orphan_prop_index;
	short type;
	short team_index;
	boolean swarm;
	byte __unknown015[3];
	long unit_index;
	long actor_index;
	real suicide_radius;
	short state;
	short timer;
	long swarm_unit_selected_time;
	real awareness;
	short perception_result;
	short visibility_result;
	short audibility_result;
	short ineffability_result;
	short line_of_sight_result;
	short orphan_lifespan_ticks;
	short orphan_inspection_ticks;
	byte __unknown03E[2];
	real_vector3d orphan_hint_vector;
	short ticks_until_orphan;
	boolean orphan_corpse_cheated;
	byte __unknown04F;
	real target_weight;
	real look_interest;
	real last_idle_look_interest;
	long last_idle_look_time;
	boolean enemy;
	boolean ally;
	boolean ally_status_changed;
	boolean in_use;
	boolean refresh_stimuli;
	byte __unknown065;
	short perception;
	short perception_decay_ticks;
	short required_ticks;
	short ticks_since_damage;
	byte __unknown06E[2];
	real damage_inflicted_on_me;
	boolean currently_damaging_me;
	byte __unknown075;
	short dead_ticks;
	short visible_ticks;
	byte __unknown07A[2];
	long last_perceived_time;
	real_point3d last_perceived_body_position;
	long last_visible_time;
	real_point3d last_visible_head_position;
	short unreachable_ticks;
	byte __unknown09E[2];
	long last_unreachable_time;
	boolean unopposable_enemy;
	byte __unknown0A5[1];
	short unopposable_casualties_inflicted;
	short unopposable_casualty_decay_timer;
	short unopposable_trigger_hysteresis;
	short unopposable_trigger_timer;
	short unopposable_trigger_threshold;
	short ticks_since_definitely_located;
	byte __unknown0B2[2];
	long definite_knowledge_source_actor;
	boolean definitely_located;
	boolean tried_to_uncover;
	boolean tried_to_search;
	boolean abandoned_search;
	real_point3d body_position;
	real_point3d center_of_mass;
	real_vector3d velocity;
	real_vector3d actor_to_prop;
	long pathfinding_surface_index;
	real_point3d pathfinding_point;
	struct location body_location;
	real_point3d head_position;
	long vehicle_index;
	long attached_to_unit_index;
	boolean underwater;
	byte __unknown119[3];
	real distance;
	char lighting;
	char quantized_distance;
	char quantized_facing;
	char quantized_speed;
	char quantized_closing_speed;
	char child_units_attached;
	boolean delay_requirement_decision;
	boolean dead;
	boolean really_dead;
	boolean just_killed;
	boolean just_became_visible;
	boolean fighting;
	boolean in_combat;
	boolean noncombat;
	boolean player;
	boolean shooting;
	boolean flying;
	boolean active_camouflage;
	boolean flashlight;
	boolean ignore;
	boolean preferred_target;
	boolean vehicle_gunner;
	boolean dangerous_vehicle_driver;
};

struct actor_perception_encounter_view
{
	byte __unknown000[0x40];
	boolean force_blind;
	boolean deaf;
	boolean blind;
	byte __unknown043[1];
	boolean stand_down;
	boolean enemy_target;
	byte __unknown046[0xA];
	long postcombat_timer;
	byte __unknown054[4];
	long corpse_ignore_time;
};

struct actor_perception_unit_view
{
	byte __unknown000[0x3D0];
	short parent_seat_index;
};

struct actor_perception_target_unit_view
{
	byte __unknown000[0xB6];
	byte active_region_flags;
};

struct actor_perception_status_unit_view
{
	byte __unknown000[0x1B4];
	unsigned long status_flags;
};

struct actor_target_weight_weapon_definition_view
{
	byte __unknown000[0x40C];
	real minimum_target_range;
};

struct actor_emotion_priority_counts_view
{
	byte __unknown000[0x1EE];
	char priority_counts[12];
};

struct actor_perception_projectile_datum_view
{
	byte __unknown000[0x1DC];
	byte attachment_flags;
};

struct actor_perception_vehicle_definition_view
{
	byte __unknown000[4];
	real bounding_radius;
	byte __unknown008[0x2E8];
	byte danger_zone_flags;
};

struct actor_perception_projectile_view
{
	byte __unknown000[0x1F0];
	real time_remaining;
	real arming_rate;
};

struct actor_perception_communication_context
{
	short source_team;
	short destination_team;
	boolean enemy;
};

struct actor_perception_responsible_unit_view
{
	byte __unknown000[0x68];
	short team;
};

struct actor_situation_counts
{
	char enemy_count;
	char enemy_very_close_count;
	char enemy_priority_counts[10];
	char fighting_enemy_count;
	char priority_two_count;
	char priority_three_count;
	char priority_four_count;
	char priority_five_count;
	char priority_six_count;
	char priority_seven_count;
	char priority_eight_count;
	char close_friend_count;
	char close_active_friend_count;
	char close_active_dangerous_friend_count;
	char close_friend_type_counts[16];
	char close_active_friend_type_counts[16];
	char visible_friend_count;
	char visible_active_friend_count;
	char visible_friend_type_counts[16];
	char visible_active_friend_type_counts[16];
	char audible_friend_count;
	char audible_active_friend_count;
	char audible_friend_type_counts[16];
	char audible_active_friend_type_counts[16];
};

struct actor_situation_actor_view
{
	byte __unknown000[4];
	short type;
	byte __unknown006[0x66];
	short combat_status;
	byte __unknown06E[0x17E];
	struct actor_situation_counts counts;
	byte __unknown267[1];
	short target_type;
	byte __unknown26A[2];
	long target_last_visible_time;
	long target_prop_index;
	byte __unknown274[0x104];
	boolean berserk;
};

struct actor_situation_prop_view
{
	byte __unknown000[0x18];
	long unit_index;
	byte __unknown01C[8];
	short state;
	byte __unknown026[0xC];
	short combat_status;
	byte __unknown034[4];
	short perception;
	byte __unknown03A[0x16];
	real target_weight;
	byte __unknown054[0xC];
	boolean enemy;
	byte __unknown061[0x13];
	boolean recent_damage;
	byte __unknown075[0x27];
	short combat_status_detail;
	byte __unknown09E[0x7E];
	real distance;
	byte __unknown120[2];
	char visibility;
	byte __unknown123[4];
	boolean dead;
	byte __unknown128[5];
	boolean active;
	byte __unknown12E[1];
	boolean friend_attack_result;
	byte __unknown130[5];
	boolean dangerous_vehicle_driver;
};

struct actor_situation_unit_view
{
	byte __unknown000[0x1A4];
	long actor_index;
	byte __unknown1A8[0x20];
	long player_index;
};

struct actor_orphan_prop_view
{
	byte __unknown000[4];
	long owner_actor_index;
	long next_prop_index;
	long related_prop_index;
	short type;
	short team_index;
	boolean swarm;
	byte __unknown015[3];
	long unit_index;
	long actor_index;
	byte __unknown020[4];
	short state;
	byte __unknown026[0x16];
	short orphan_inspection_ticks;
	byte __unknown03E[0x12];
	real target_weight;
	byte __unknown054[0x50];
	boolean unopposable_enemy;
	byte __unknown0A5[0xB];
	short ticks_since_definitely_located;
	byte __unknown0B2[2];
	long definite_knowledge_source_actor;
	boolean definitely_located;
};

struct actor_perception_refresh_entry
{
	long unit_index;
	long prop_index;
	real priority;
};

/*
 * code_00023290 maintains separate enemy and friend candidate lists on its
 * stack. Each list has two 16-bit counters followed by 128 12-byte entries.
 */
struct actor_perception_refresh_list
{
	short accepted_count;
	short entry_count;
	struct actor_perception_refresh_entry entries[128];
};

struct actor_perception_refresh_locals
{
	struct object_cluster_iterator cluster_iterator;
	short dead_ticks;
	byte __unknown00A[2];
	real suicide_radius;
	struct actor_perception_actor_view *actor;
	short visible_ticks;
	byte __unknown016[2];
	long unit_index;
	struct prop_iterator iterator;
	struct actor_perception_actor_view *current_actor;

	union
	{
		struct structure_bsp *structure_bsp;
		short target_count;
	} bsp_or_target;

	boolean in_use;
	byte __unknown02D[3];
	boolean dead;
	byte __unknown031[3];
	unsigned long *cluster_pvs;
};

typedef char actor_perception_actor_view_target_prop_index_offset_assert[
	offsetof(struct actor_perception_actor_view, target_prop_index) == 0x270 ? 1 : -1];
typedef char actor_perception_actor_view_team_offset_assert[
	offsetof(struct actor_perception_actor_view, team) == 0x3E ? 1 : -1];
typedef char actor_perception_actor_view_definition_index_offset_assert[
	offsetof(struct actor_perception_actor_view, definition_index) == 0x58 ? 1 : -1];
typedef char actor_perception_actor_view_preferred_prop_offset_assert[
	offsetof(struct actor_perception_actor_view, preferred_target_prop_index) == 0x54 ? 1 : -1];
typedef char actor_perception_actor_view_variant_definition_offset_assert[
	offsetof(struct actor_perception_actor_view, variant_definition_index) == 0x5C ? 1 : -1];
typedef char actor_perception_actor_view_underwater_offset_assert[
	offsetof(struct actor_perception_actor_view, underwater) == 0x15D ? 1 : -1];
typedef char actor_perception_actor_view_berserk_offset_assert[
	offsetof(struct actor_perception_actor_view, berserk) == 0x378 ? 1 : -1];
typedef char actor_perception_actor_view_orphan_inspection_offset_assert[
	offsetof(struct actor_perception_actor_view, long_orphan_inspection) == 0x162 ? 1 : -1];
typedef char actor_perception_actor_view_sighted_friend_offset_assert[
	offsetof(struct actor_perception_actor_view, sighted_friendly_player) == 0x377 ? 1 : -1];
typedef char actor_perception_actor_view_danger_zone_offset_assert[
	offsetof(struct actor_perception_actor_view, danger_zone) == 0x280 ? 1 : -1];
typedef char actor_perception_actor_view_active_offset_assert[
	offsetof(struct actor_perception_actor_view, active) == 0x8 ? 1 : -1];
typedef char actor_perception_actor_view_dormant_offset_assert[
	offsetof(struct actor_perception_actor_view, dormant) == 0x13 ? 1 : -1];
typedef char actor_perception_actor_view_encounter_index_offset_assert[
	offsetof(struct actor_perception_actor_view, encounter_index) == 0x34 ? 1 : -1];
typedef char actor_perception_actor_view_swarm_unit_index_offset_assert[
	offsetof(struct actor_perception_actor_view, swarm_unit_index) == 0x24 ? 1 : -1];
typedef char actor_perception_actor_view_swarm_cache_index_offset_assert[
	offsetof(struct actor_perception_actor_view, swarm_cache_index) == 0x28 ? 1 : -1];
typedef char actor_perception_actor_view_body_cluster_index_offset_assert[
	offsetof(struct actor_perception_actor_view, body_cluster_index) == 0x148 ? 1 : -1];
typedef char actor_perception_actor_view_vehicle_index_offset_assert[
	offsetof(struct actor_perception_actor_view, vehicle_index) == 0x158 ? 1 : -1];
typedef char actor_perception_actor_view_corpse_interest_inhibited_offset_assert[
	offsetof(struct actor_perception_actor_view, corpse_interest_inhibited) == 0x1CC ? 1 : -1];
typedef char actor_perception_actor_view_corpse_ignore_time_offset_assert[
	offsetof(struct actor_perception_actor_view, corpse_ignore_time) == 0x3A0 ? 1 : -1];
typedef char actor_perception_prop_view_unopposable_enemy_offset_assert[
	offsetof(struct actor_perception_prop_view, unopposable_enemy) == 0xA4 ? 1 : -1];
typedef char actor_perception_actor_view_active_threat_count_offset_assert[
	offsetof(struct actor_perception_actor_view, active_threat_count) == 0x308 ? 1 : -1];
typedef char actor_perception_actor_view_target_combat_status_offset_assert[
	offsetof(struct actor_perception_actor_view, target_combat_status) == 0x72 ? 1 : -1];
typedef char actor_perception_actor_view_pending_combat_status_offset_assert[
	offsetof(struct actor_perception_actor_view, pending_combat_status) == 0x34A ? 1 : -1];
typedef char actor_perception_actor_view_nearby_fighting_friend_count_offset_assert[
	offsetof(struct actor_perception_actor_view, nearby_fighting_friend_count) == 0x1EC ? 1 : -1];
typedef char actor_perception_prop_view_in_combat_offset_assert[
	offsetof(struct actor_perception_prop_view, in_combat) == 0x12C ? 1 : -1];
typedef char actor_perception_prop_swarm_offset_assert[
	offsetof(struct prop_datum, swarm) == 0x14 ? 1 : -1];
typedef char actor_perception_prop_unit_index_offset_assert[
	offsetof(struct prop_datum, unit_index) == 0x18 ? 1 : -1];
typedef char actor_perception_prop_actor_index_offset_assert[
	offsetof(struct prop_datum, actor_index) == 0x1C ? 1 : -1];
typedef char actor_perception_prop_enemy_offset_assert[
	offsetof(struct prop_datum, enemy) == 0x60 ? 1 : -1];
typedef char actor_perception_prop_body_position_offset_assert[
	offsetof(struct prop_datum, body_position) == 0xBC ? 1 : -1];
typedef char actor_perception_prop_dead_offset_assert[
	offsetof(struct prop_datum, dead) == 0x127 ? 1 : -1];
typedef char actor_perception_prop_view_vehicle_gunner_offset_assert[
	offsetof(struct actor_perception_prop_view, vehicle_gunner) == 0x135 ? 1 : -1];
typedef char actor_perception_prop_view_player_offset_assert[
	offsetof(struct actor_perception_prop_view, player) == 0x12E ? 1 : -1];
typedef char actor_perception_prop_view_perception_result_offset_assert[
	offsetof(struct actor_perception_prop_view, perception_result) == 0x30 ? 1 : -1];
typedef char actor_perception_prop_view_dangerous_vehicle_driver_offset_assert[
	offsetof(struct actor_perception_prop_view, dangerous_vehicle_driver) == 0x136 ? 1 : -1];
typedef char actor_perception_swarm_component_position_offset_assert[
	offsetof(struct actor_perception_swarm_component_view, position) == 4 ? 1 : -1];
typedef char actor_perception_status_unit_flags_offset_assert[
	offsetof(struct actor_perception_status_unit_view, status_flags) == 0x1B4 ? 1 : -1];
typedef char actor_perception_target_unit_active_region_offset_assert[
	offsetof(struct actor_perception_target_unit_view, active_region_flags) == 0xB6 ? 1 : -1];
typedef char actor_target_weight_definition_melee_velocity_offset_assert[
	offsetof(struct actor_definition, berserk.melee_leap_velocity) == 0x38C ? 1 : -1];
typedef char actor_target_weight_variant_melee_range_offset_assert[
	offsetof(struct actor_variant_definition, ranged_combat.melee_range) == 0x160 ? 1 : -1];
typedef char actor_target_weight_variant_berserk_melee_range_offset_assert[
	offsetof(struct actor_variant_definition, ranged_combat.berserk_melee_range) == 0x170 ? 1 : -1];
typedef char actor_target_weight_firing_maximum_range_offset_assert[
	offsetof(struct actor_variant_definition, ranged_combat.maximum_firing_range) == 0x74 ? 1 : -1];
typedef char actor_target_weight_firing_combat_range_offset_assert[
	offsetof(struct actor_variant_definition, ranged_combat.combat_range_upper_bound) == 0xA0 ? 1 : -1];
typedef char actor_target_weight_weapon_minimum_range_offset_assert[
	offsetof(struct actor_target_weight_weapon_definition_view, minimum_target_range) == 0x40C ? 1 : -1];
typedef char actor_perception_preferred_target_prop_index_offset_assert[
	offsetof(struct actor_datum, meta.interesting_orphan_index) == 0x54 ? 1 : -1];
typedef char actor_perception_audibility_combat_status_offset_assert[
	offsetof(struct actor_datum, state.mode) == 0x6A ? 1 : -1];
typedef char actor_perception_target_weight_combat_status_offset_assert[
	offsetof(struct actor_datum, state.combat_status) == 0x6E ? 1 : -1];
typedef char actor_emotion_actor_unit_offset_assert[
	offsetof(struct actor_datum, meta.unit_index) == 0x18 ? 1 : -1];
typedef char actor_emotion_actor_definition_offset_assert[
	offsetof(struct actor_datum, meta.definition_index) == 0x58 ? 1 : -1];
typedef char actor_emotion_actor_combat_status_offset_assert[
	offsetof(struct actor_datum, state.combat_status) == 0x6E ? 1 : -1];
typedef char actor_emotion_actor_body_position_offset_assert[
	offsetof(struct actor_datum, input.position.body_position) == 0x12C ? 1 : -1];
typedef char actor_emotion_actor_body_vitality_offset_assert[
	offsetof(struct actor_datum, input.body_vitality) == 0x1B8 ? 1 : -1];
typedef char actor_emotion_actor_external_orders_offset_assert[
	offsetof(struct actor_datum, external_orders) == 0x1C8 ? 1 : -1];
typedef char actor_emotion_actor_target_offset_assert[
	offsetof(struct actor_datum, target.target_type) == 0x268 ? 1 : -1];
typedef char actor_emotion_actor_emotions_offset_assert[
	offsetof(struct actor_datum, emotions) == 0x350 ? 1 : -1];
typedef char actor_emotion_actor_firing_position_offset_assert[
	offsetof(struct actor_datum, firing_positions.current_position_index) == 0x3B8 ? 1 : -1];
typedef char actor_emotion_actor_control_moving_offset_assert[
	offsetof(struct actor_datum, control.moving) == 0x504 ? 1 : -1];
typedef char actor_emotion_actor_control_vector_offset_assert[
	offsetof(struct actor_datum, control.moving_towards_vector) == 0x518 ? 1 : -1];
typedef char actor_emotion_priority_counts_offset_assert[
	offsetof(struct actor_emotion_priority_counts_view, priority_counts) == 0x1EE ? 1 : -1];
typedef char actor_emotion_definition_crouch_type_offset_assert[
	offsetof(struct actor_definition, defensive.defensive_crouch_type) == 0x2F8 ? 1 : -1];
typedef char actor_emotion_definition_attacking_threshold_offset_assert[
	offsetof(struct actor_definition, defensive.defensive_threshold_attacking) == 0x2FC ? 1 : -1];
typedef char actor_emotion_definition_defending_threshold_offset_assert[
	offsetof(struct actor_definition, defensive.defensive_threshold_defending) == 0x300 ? 1 : -1];
typedef char actor_emotion_definition_minimum_stand_offset_assert[
	offsetof(struct actor_definition, defensive.defensive_crouch_min_stand_time) == 0x304 ? 1 : -1];
typedef char actor_emotion_definition_minimum_crouch_offset_assert[
	offsetof(struct actor_definition, defensive.defensive_crouch_min_crouch_time) == 0x308 ? 1 : -1];
typedef char actor_perception_ai_debug_ignore_players_offset_assert[
	offsetof(struct actor_perception_ai_debug_state_view, perception_ignore_player_props) == 6 ? 1 : -1];
typedef char actor_perception_ai_debug_blind_players_offset_assert[
	offsetof(struct actor_perception_ai_debug_state_view, perception_blind_player_props) == 7 ? 1 : -1];
typedef char actor_perception_ai_debug_blind_all_offset_assert[
	offsetof(struct actor_perception_ai_debug_state_view, perception_blind_all_props) == 11 ? 1 : -1];
typedef char actor_perception_ai_debug_deaf_all_offset_assert[
	offsetof(struct actor_perception_ai_debug_state_view, perception_deaf_all_props) == 12 ? 1 : -1];
typedef char actor_perception_ai_debug_trace_offset_assert[
	offsetof(struct actor_perception_ai_debug_state_view, perception_trace_player_awareness) == 0xA1 ? 1 : -1];
typedef char actor_perception_debug_awareness_speed_offset_assert[
	offsetof(struct actor_perception_debug_info_view, perception_awareness_speed) == 0x6578 ? 1 : -1];
typedef char actor_perception_projectile_attachment_flags_offset_assert[
	offsetof(struct actor_perception_projectile_datum_view, attachment_flags) == 0x1DC ? 1 : -1];
typedef char actor_perception_source_unit_sound_offset_assert[
	offsetof(struct unit_definition, unit.constant_sound) == 0x182 ? 1 : -1];
typedef char actor_perception_source_actor_target_offset_assert[
	offsetof(struct actor_datum, target.target_type) == 0x268 ? 1 : -1];
typedef char actor_perception_source_actor_shooting_offset_assert[
	offsetof(struct actor_datum, orders.combat.shoot_at_target) == 0x454 ? 1 : -1];
typedef char actor_perception_actor_weapon_range_offset_assert[
	offsetof(struct actor_datum, control.weapon_maximum_range) == 0x608 ? 1 : -1];
typedef char actor_perception_definition_melee_range_offset_assert[
	offsetof(struct actor_definition, berserk.melee_attack_range) == 0x37C ? 1 : -1];
typedef char actor_perception_profile_size_assert[
	sizeof(struct ai_profile_globals) == 0xEEC ? 1 : -1];
typedef char actor_perception_profile_dead_acknowledged_offset_assert[
	offsetof(struct ai_profile_globals, data.perception.dead_acknowledged) == 0x4D4 ? 1 : -1];
typedef char actor_perception_profile_dead_orphan_offset_assert[
	offsetof(struct ai_profile_globals, data.perception.dead_orphan) == 0x55C ? 1 : -1];
typedef char actor_perception_profile_dead_unacknowledged_offset_assert[
	offsetof(struct ai_profile_globals, data.perception.dead_unacknowledged) == 0x5E4 ? 1 : -1];
typedef char actor_perception_profile_enemy_acknowledged_offset_assert[
	offsetof(struct ai_profile_globals, data.perception.enemy_acknowledged) == 0x66C ? 1 : -1];
typedef char actor_perception_profile_enemy_orphan_offset_assert[
	offsetof(struct ai_profile_globals, data.perception.enemy_orphan) == 0x6F4 ? 1 : -1];
typedef char actor_perception_profile_enemy_unacknowledged_offset_assert[
	offsetof(struct ai_profile_globals, data.perception.enemy_unacknowledged) == 0x77C ? 1 : -1];
typedef char actor_perception_profile_friend_acknowledged_offset_assert[
	offsetof(struct ai_profile_globals, data.perception.friend_acknowledged) == 0x804 ? 1 : -1];
typedef char actor_perception_profile_friend_orphan_offset_assert[
	offsetof(struct ai_profile_globals, data.perception.friend_orphan) == 0x88C ? 1 : -1];
typedef char actor_perception_profile_friend_unacknowledged_offset_assert[
	offsetof(struct ai_profile_globals, data.perception.friend_unacknowledged) == 0x914 ? 1 : -1];
typedef char actor_perception_vehicle_definition_radius_offset_assert[
	offsetof(struct actor_perception_vehicle_definition_view, bounding_radius) == 4 ? 1 : -1];
typedef char actor_perception_vehicle_definition_danger_zone_offset_assert[
	offsetof(struct actor_perception_vehicle_definition_view, danger_zone_flags) == 0x2F0 ? 1 : -1];
typedef char actor_perception_responsible_unit_team_offset_assert[
	offsetof(struct actor_perception_responsible_unit_view, team) == 0x68 ? 1 : -1];
typedef char actor_perception_encounter_view_blind_offset_assert[
	offsetof(struct actor_perception_encounter_view, blind) == 0x42 ? 1 : -1];
typedef char actor_perception_encounter_view_stand_down_offset_assert[
	offsetof(struct actor_perception_encounter_view, stand_down) == 0x44 ? 1 : -1];
typedef char actor_perception_encounter_view_enemy_target_offset_assert[
	offsetof(struct actor_perception_encounter_view, enemy_target) == 0x45 ? 1 : -1];
typedef char actor_perception_encounter_view_postcombat_timer_offset_assert[
	offsetof(struct actor_perception_encounter_view, postcombat_timer) == 0x50 ? 1 : -1];
typedef char actor_perception_encounter_view_corpse_ignore_time_offset_assert[
	offsetof(struct actor_perception_encounter_view, corpse_ignore_time) == 0x58 ? 1 : -1];
typedef char actor_visibility_variant_modified_vision_range_offset_assert[
	offsetof(struct actor_variant_definition, ranged_combat.modified_vision_range) == 0x150 ? 1 : -1];
typedef char actor_visibility_debug_info_size_assert[
	sizeof(struct actor_debug_info) == 0x657C ? 1 : -1];
typedef char actor_visibility_debug_info_last_time_offset_assert[
	offsetof(struct actor_debug_info, vision_last_time) == 0x656C ? 1 : -1];
typedef char actor_situation_counts_size_assert[
	sizeof(struct actor_situation_counts) == 0x7B ? 1 : -1];
typedef char actor_situation_actor_view_counts_offset_assert[
	offsetof(struct actor_situation_actor_view, counts) == 0x1EC ? 1 : -1];
typedef char actor_situation_actor_view_target_prop_index_offset_assert[
	offsetof(struct actor_situation_actor_view, target_prop_index) == 0x270 ? 1 : -1];
typedef char actor_situation_actor_view_berserk_offset_assert[
	offsetof(struct actor_situation_actor_view, berserk) == 0x378 ? 1 : -1];
typedef char actor_situation_prop_view_target_weight_offset_assert[
	offsetof(struct actor_situation_prop_view, target_weight) == 0x50 ? 1 : -1];
typedef char actor_situation_prop_view_distance_offset_assert[
	offsetof(struct actor_situation_prop_view, distance) == 0x11C ? 1 : -1];
typedef char actor_situation_unit_view_actor_index_offset_assert[
	offsetof(struct actor_situation_unit_view, actor_index) == 0x1A4 ? 1 : -1];
typedef char actor_situation_unit_view_player_index_offset_assert[
	offsetof(struct actor_situation_unit_view, player_index) == 0x1C8 ? 1 : -1];
typedef char actor_orphan_prop_view_related_prop_index_offset_assert[
	offsetof(struct actor_orphan_prop_view, related_prop_index) == 0xC ? 1 : -1];
typedef char actor_orphan_prop_view_orphan_inspection_ticks_offset_assert[
	offsetof(struct actor_orphan_prop_view, orphan_inspection_ticks) == 0x3C ? 1 : -1];
typedef char actor_orphan_prop_view_target_weight_offset_assert[
	offsetof(struct actor_orphan_prop_view, target_weight) == 0x50 ? 1 : -1];
typedef char actor_orphan_prop_view_unopposable_enemy_offset_assert[
	offsetof(struct actor_orphan_prop_view, unopposable_enemy) == 0xA4 ? 1 : -1];
typedef char actor_orphan_prop_view_definite_source_offset_assert[
	offsetof(struct actor_orphan_prop_view, definite_knowledge_source_actor) == 0xB4 ? 1 : -1];
typedef char actor_orphan_prop_view_definitely_located_offset_assert[
	offsetof(struct actor_orphan_prop_view, definitely_located) == 0xB8 ? 1 : -1];
typedef char actor_danger_zone_view_size_assert[
	sizeof(struct actor_danger_zone_view) == 0x6C ? 1 : -1];
typedef char actor_danger_zone_view_object_index_offset_assert[
	offsetof(struct actor_danger_zone_view, object_index) == 0xC ? 1 : -1];
typedef char actor_danger_zone_view_danger_radius_offset_assert[
	offsetof(struct actor_danger_zone_view, danger_radius) == 0x14 ? 1 : -1];
typedef char actor_danger_zone_view_initial_position_offset_assert[
	offsetof(struct actor_danger_zone_view, initial_position) == 0x18 ? 1 : -1];
typedef char actor_danger_zone_view_initial_velocity_offset_assert[
	offsetof(struct actor_danger_zone_view, initial_velocity) == 0x24 ? 1 : -1];
typedef char actor_danger_zone_view_position_offset_assert[
	offsetof(struct actor_danger_zone_view, position) == 0x30 ? 1 : -1];
typedef char actor_danger_zone_view_projected_position_offset_assert[
	offsetof(struct actor_danger_zone_view, predict_danger_position) == 0x48 ? 1 : -1];
typedef char actor_danger_zone_view_distance_offset_assert[
	offsetof(struct actor_danger_zone_view, current_distance_from_actor) == 0x54 ? 1 : -1];
typedef char actor_danger_zone_view_midpoint_offset_assert[
	offsetof(struct actor_danger_zone_view, bounding_sphere_center) == 0x5C ? 1 : -1];
typedef char actor_danger_zone_view_impact_ticks_offset_assert[
	offsetof(struct actor_danger_zone_view, predicted_impact_ticks) == 0x68 ? 1 : -1];
typedef char actor_perception_refresh_entry_size_assert[
	sizeof(struct actor_perception_refresh_entry) == 0xC ? 1 : -1];
typedef char actor_perception_refresh_list_entries_offset_assert[
	offsetof(struct actor_perception_refresh_list, entries) == 4 ? 1 : -1];
typedef char actor_perception_refresh_list_size_assert[
	sizeof(struct actor_perception_refresh_list) == 0x604 ? 1 : -1];

/*
 * Runtime perception values in the January actor definition. The shared
 * HCEX-derived definition still labels two of these slots as unused.
 */
struct actor_perception_definition_view
{
	unsigned long flags;
	byte __unknown004[0x18];
	real maximum_vision_angle;
	real central_vision_angle;
	byte __unknown024[4];
	real peripheral_distance;
	real maximum_peripheral_distance;
};

/* ---------- prototypes */

long code_0001dbc0(
	void const *a,
	void const *b);

void actor_situation_update_target_status(
	long actor_index);

void actor_situation_combat_status_update(
	long actor_index);

short actor_action_class(
	long actor_index);

real actor_compute_prop_target_weight(
	long actor_index,
	long prop_index);

boolean actor_compute_prop_unopposable(
	long actor_index,
	long prop_index);

void actor_stimulus_prop_acknowledged(
	long actor_index,
	long prop_index,
	long stimulus,
	boolean initial_acknowledgement);

long ai_get_responsible_unit(
	long object_index,
	boolean include_self);

long vehicle_find_pathfinding_surface_index(
	long vehicle_index,
	real_point3d *position);

long biped_find_pathfinding_surface_index(
	long biped_index,
	real_point3d *position);

boolean actor_attacking_target(
	long actor_index,
	real_vector3d *attack_vector);

void ai_communication_event(
	short type,
	long unit_index,
	long prop_index,
	long object_index,
	long position_index,
	long structure_index,
	void const *context);

void actor_discard_firing_position(
	long actor_index,
	short firing_position_index,
	boolean temporary);

boolean actor_type_flood_desire_shamble(
	long actor_index);

void actor_stimulus_noticed_danger_zone(
	long actor_index,
	short danger_type,
	short hostility,
	long object_index,
	real_point3d const *position);

void actor_stimulus_prop_just_killed(
	long actor_index,
	long prop_index);

void actor_stimulus_prop_sighted(
	long actor_index,
	long prop_index,
	boolean initial_acknowledgement);

void actor_stimulus_enter_combat_found_body(
	long actor_index,
	long prop_index);

void actor_stimulus_enter_combat_perceived_enemy(
	long actor_index,
	long prop_index);

static long code_00020210(
	long swarm_actor_index,
	long actor_index,
	long existing_unit_index,
	boolean mark,
	struct actor_position_data const *position);

static boolean code_00020780(
	long actor_index,
	long vehicle_index,
	boolean mark,
	struct actor_position_data const *position);

void code_00023290(
	long actor_index);

void code_000228b0(
	long actor_index,
	long object_index,
	struct actor_perception_refresh_list *friend_list,
	struct actor_perception_refresh_list *enemy_list);

struct actor_variant_definition *actor_combat_get_firing_variant_definition(
	long actor_index);

void *actor_get_weapon_definition(
	long actor_index);

void actor_input_sample_position(
	long actor_index,
	long unit_index,
	struct actor_position_data *position);

boolean vehicle_causes_collision_damage(
	long vehicle_index);

boolean game_team_is_enemy(
	short team_index0,
	short team_index1);

boolean actor_expected_acknowledgement(
	long actor_index,
	long prop_index);

short ai_test_line_of_sight(
	struct actor_position_data const *position,
	short source_cluster_index,
	real_point3d const *target_position,
	short target_cluster_index,
	short mode,
	boolean ignore_blockers,
	long vehicle_index,
	boolean actor_in_vehicle);

short actor_visibility_at_point(
	long actor_index,
	struct actor_position_data const *position,
	real_point3d const *target_position,
	char lighting,
	short line_of_sight,
	boolean use_maximum_distance,
	boolean target_is_player,
	short perception_knowledge);

short actor_audibility_at_point(
	long actor_index,
	struct actor_position_data const *position,
	real_point3d const *source_position,
	struct location const *source_location,
	short source_type,
	real scale,
	short line_of_sight);

static boolean code_0001dc00(
	long actor_index,
	long object_index,
	real suicide_radius,
	real distance,
	boolean enemy,
	boolean visible);

real actor_look_compute_prop_interest(
	long actor_index,
	long prop_index);

static __inline boolean prop_acknowledged(
	struct prop_datum const *prop)
{
	return prop->state >= _prop_state_becoming_unacknowledged &&
		prop->state <= _prop_state_acknowledged;
}

static __inline void actor_perception_midpoint3d(
	real_point3d const *p0,
	real_point3d const *p1,
	real_point3d *result)
{
	result->x = (p0->x + p1->x) * 0.5f;
	result->y = (p0->y + p1->y) * 0.5f;
	result->z = (p0->z + p1->z) * 0.5f;

	return;
}

/* ---------- globals */

extern struct ai_profile_globals ai_profile;

short const global_combat_status_table[NUMBER_OF_ACTOR_TARGET_TYPES] =
{
	0, 0, 0, 1, 2, 2, 3, 4, 5, 5, 7, 7
};

short const global_acknowledgement_speeds[4][4] =
{
	{ 0, 0, 1, 3 },
	{ 0, 1, 2, 3 },
	{ 0, 2, 3, 4 },
	{ 0, 3, 4, 4 }
};

long data_002b6ae0 = NONE;

/* ---------- public code */

boolean actor_perception_desire_prop(
	long actor_index,
	short prop_type,
	long unit_index,
	union actor_perception_prop_actor_slot prop_actor,
	boolean in_use,
	boolean player,
	boolean enemy,
	boolean dead,
	short dead_ticks,
	real suicide_radius,
	real distance_squared,
	short required_ticks,
	boolean *too_far_reference)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	struct actor_perception_actor_view *related_actor;
	boolean desire = dead;

	if (prop_actor.actor_index == NONE)
		related_actor = NULL;
	else
		related_actor =
			(struct actor_perception_actor_view *)actor_get(
				prop_actor.actor_index);

	prop_actor.too_far = FALSE;

	if ((!enemy || dead) && prop_type >= 4 && prop_type <= 5)
	{
		desire = FALSE;
	}
	else if (player)
	{
		desire = TRUE;
	}
	else if (related_actor &&
		(!related_actor->active || related_actor->dormant))
	{
		desire = FALSE;
	}
	else if (prop_type == NONE && (in_use || required_ticks > 0))
	{
		desire = TRUE;
	}
	else if (distance_squared > 1600.0f)
	{
		desire = FALSE;
	}
	else if (dead)
	{
		desire = TRUE;

		if (actor->encounter_index != NONE)
		{
			struct actor_perception_encounter_view *encounter =
				(struct actor_perception_encounter_view *)
					encounter_get(actor->encounter_index);
			struct unit_datum *unit = unit_get(unit_index);
			long ignore_time = encounter->corpse_ignore_time;
			boolean inactive_encounter;

			if (ignore_time <= actor->corpse_ignore_time)
				ignore_time = actor->corpse_ignore_time;

			if (ignore_time != NONE &&
				(unit->unit.time_of_death == NONE ||
					unit->unit.time_of_death < ignore_time))
			{
				desire = FALSE;
			}

			inactive_encounter =
				!encounter->enemy_target &&
				!encounter->stand_down &&
				!encounter->blind;

			if (!desire)
				goto done;

			if (inactive_encounter)
			{
				if (distance_squared < 225.0f)
				{
					desire = TRUE;
					goto done;
				}

				desire = FALSE;
				goto done;
			}
		}

		if (suicide_radius > 0.0f)
		{
			desire = TRUE;
		}
		else if (enemy && dead_ticks > 150)
		{
			desire = FALSE;
		}
		else if (actor_action_class(actor_index) > 1)
		{
			desire = FALSE;
		}
		else
		{
			real maximum_distance_squared = 16.0f;

			if (!enemy && actor->combat_status < 3)
				maximum_distance_squared = 64.0f;

			desire = distance_squared < maximum_distance_squared;
		}
	}
	else
	{
		if (enemy)
		{
			desire = TRUE;
			prop_actor.too_far = distance_squared > 36.0f;
		}
		else
		{
			desire = distance_squared < 225.0f;

			if (actor->artificial_combat_status >= 4)
				prop_actor.too_far = TRUE;
			else if (actor->corpse_interest_inhibited)
				prop_actor.too_far = FALSE;
			else
				prop_actor.too_far = distance_squared > 16.0f;
		}
	}

done:
	if (too_far_reference)
		*too_far_reference = prop_actor.too_far;

	return desire;
}

void actor_perception_acknowledge(
	long actor_index,
	long prop_index,
	long stimulus,
	boolean initial_acknowledgement)
{
	struct prop_datum *prop = prop_get(prop_index);

#line 1037 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
	assert(prop->owner_actor_index == actor_index);
	assert(prop_acknowledged(prop));
#line 1039 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
	vassert(prop->orphan_prop_index == NONE, "prop->orphan_prop_index == NONE");
#line 300 "source\\ai\\actor_perception.c"

	prop->tried_to_search = FALSE;
	prop->tried_to_uncover = FALSE;
	prop->abandoned_search = FALSE;
	prop->refresh_stimuli = TRUE;

	actor_stimulus_prop_acknowledged(
		actor_index,
		prop_index,
		stimulus,
		initial_acknowledgement);

	return;
}

long actor_get_perception_knowledge(
	long actor_index,
	long prop_index)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);

	if (prop_index != NONE)
	{
		struct actor_perception_prop_view *prop =
			(struct actor_perception_prop_view *)prop_get(prop_index);

#line 1394 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		assert(prop->owner_actor_index == actor_index);
#line 390 "source\\ai\\actor_perception.c"

		if (prop->state >= 2 && prop->state <= 3)
			return 3;

		if (prop->perception == 1 || prop->perception == 2)
			return 3;

		if (!prop->enemy &&
			(!prop->dead || actor->combat_status >= 3))
		{
			return 3;
		}

		if (prop->orphan_prop_index != NONE)
		{
			struct actor_perception_prop_view *orphan =
				(struct actor_perception_prop_view *)prop_get(
					prop->orphan_prop_index);
			long result = (orphan->definitely_located != FALSE) + 2;

			if ((short)result != NONE)
				return result;
		}
	}

	if (actor->artificial_combat_status >= 2)
		return 2;

	return actor->combat_status >= 3;
}

void actor_get_vision_distances(
	long actor_index,
	real maximum_vision_distance,
	real perception_factor,
	real horizontal_angle,
	real *full_distance_reference,
	real *partial_distance_reference)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_perception_definition_view const *definition =
		(struct actor_perception_definition_view const *)
			actor_definition_get(actor->meta.definition_index);
	real full_distance;
	real partial_distance;

	if (horizontal_angle > definition->peripheral_distance)
	{
		partial_distance = 0.0f;
		full_distance = 0.0f;
	}
	else
	{
		real maximum_distance =
			maximum_vision_distance * perception_factor;
		real peripheral_distance =
			perception_factor * definition->maximum_peripheral_distance;
		real minimum_full_distance = 0.7f * maximum_distance;
		real minimum_partial_distance = 0.7f * peripheral_distance;

		if (minimum_partial_distance > 3.5f)
			minimum_partial_distance = 3.5f;

		if (horizontal_angle > definition->central_vision_angle)
		{
			partial_distance = peripheral_distance;
			full_distance = minimum_partial_distance;
		}
		else
		{
			real maximum_vision_angle =
				definition->maximum_vision_angle;
			real full_vision_angle = 0.8f * maximum_vision_angle;

			if (horizontal_angle < maximum_vision_angle)
			{
				partial_distance = maximum_distance;
			}
			else
			{
				real interpolation =
					(horizontal_angle - maximum_vision_angle) /
					(definition->central_vision_angle -
						maximum_vision_angle);

				partial_distance =
					(1.0f - interpolation) * maximum_distance +
					peripheral_distance * interpolation;
			}

			if (horizontal_angle < full_vision_angle)
			{
				full_distance = minimum_full_distance;
			}
			else
			{
				real interpolation =
					(horizontal_angle - full_vision_angle) /
					(definition->central_vision_angle -
						full_vision_angle);

				full_distance =
					(1.0f - interpolation) *
						minimum_full_distance +
					minimum_partial_distance * interpolation;
			}
		}
	}

	*partial_distance_reference = partial_distance;
	*full_distance_reference = full_distance;

	return;
}

void actor_situation_update_target_status(
	long actor_index)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);

	if (actor->target_prop_index == NONE)
	{
		actor->target_type = 0;
		actor->target_last_visible_time = NONE;
		actor->target_outside_active_area = FALSE;
	}
	else
	{
		struct actor_perception_prop_view *target_prop =
			(struct actor_perception_prop_view *)prop_get(
				actor->target_prop_index);
		struct actor_perception_target_unit_view *target_unit =
			(struct actor_perception_target_unit_view *)
				unit_get(target_prop->unit_index);
		short target_type;

#line 4291 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		assert(target_prop->enemy);
#line 510 "source\\ai\\actor_perception.c"

		switch (target_prop->state)
		{
		case _prop_state_unacknowledged:
			target_type = 0;
			actor->target_prop_index = NONE;
			actor->target_last_visible_time = NONE;
			break;

		case _prop_state_becoming_acknowledged:
			target_type = 1;
			break;

		case _prop_state_becoming_unacknowledged:
		case _prop_state_acknowledged:
			if (target_prop->dead)
				target_type = 2;
			else if (target_prop->currently_damaging_me)
				target_type = 11;
			else if (target_prop->visibility_result >= 2)
				target_type = 10;
			else if (target_prop->line_of_sight_result != 0 &&
				target_prop->line_of_sight_result != 1)
			{
				target_type = 7;
			}
			else if (target_prop->quantized_facing <= 2 &&
				target_prop->distance < 6.f)
			{
				target_type = 9;
			}
			else
			{
				target_type = 8;
			}
			break;

		case _prop_state_inspected_orphan:
			if (target_prop->dead)
				target_type = 2;
			else
				target_type =
					target_prop->abandoned_search ? 3 : 4;
			break;

		case _prop_state_uninspected_orphan:
			target_type =
				(target_prop->definitely_located != FALSE) + 5;
			break;

		default:
			display_assert(
				NULL,
				"c:\\halo\\SOURCE\\ai\\actor_perception.c",
				4362,
				TRUE);
			system_exit(-1);
			break;
		}

		actor->target_type = target_type;

		if (target_prop->state >=
				_prop_state_becoming_unacknowledged &&
			target_prop->state <= _prop_state_acknowledged)
		{
			actor->target_outside_active_area =
				!target_prop->dead;

			if (target_prop->visibility_result > 0)
				actor->target_last_visible_time =
					target_prop->last_visible_time;
		}
		else
		{
			actor->target_outside_active_area =
				!TEST_FLAG(
					target_unit->active_region_flags,
					2);
		}
	}

	return;
}

real actor_compute_prop_target_weight(
	long actor_index,
	long prop_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct prop_datum *prop = prop_get(prop_index);
	struct actor_definition *actor_definition;
	struct actor_variant_definition *variant_definition;
	struct actor_variant_definition *firing_variant;
	struct actor_target_weight_weapon_definition_view *weapon_definition;
	short range_weight;
	short knowledge_weight = 0;
	struct
	{
		long target_weight;
		long preferred_weight;
		real bonus_weight;
	} weights;

	if (prop->ignore ||
		!prop->enemy ||
		(prop->state >= 0 && prop->state <= 1) ||
		(prop->dead && prop->dead_ticks >= 150) ||
		prop->type == 15)
	{
		return 0.0f;
	}

	actor_definition =
		actor_definition_get(actor->meta.definition_index);
	variant_definition =
		actor_variant_definition_get(actor->meta.variant_definition_index);
	weights.target_weight = 0;
	weights.preferred_weight = 0;
	weights.bonus_weight = 0.0f;

	if (actor->meta.swarm)
	{
		range_weight = 0;
		goto range_weight_done;
	}

	if (prop->unreachable_ticks > 0)
	{
		range_weight = 0;
		goto range_weight_done;
	}

	if (!actor_has_ranged_weapon(actor_index))
	{
		real maximum_range;

		if (actor->emotions.berserk)
			maximum_range =
				variant_definition->ranged_combat.melee_range;
		else
			maximum_range =
				variant_definition->ranged_combat.berserk_melee_range;

		if (prop->distance < 2.0f)
		{
			range_weight = 5;
			if (prop->state != _prop_state_inspected_orphan)
				goto range_weight_done;
		}

		if (prop->vehicle_index != NONE)
			range_weight = 0;
		else if (prop->flying &&
			actor_definition->berserk.melee_leap_velocity == 0.0f)
			range_weight = 0;
		else if (prop->underwater !=
			actor->input.underwater)
			range_weight = 1;
		else if (prop->distance < maximum_range)
			range_weight = 3;
		else
			goto ranged_weapon_range_two;
	}
	else
	{
		weapon_definition =
			(struct actor_target_weight_weapon_definition_view *)
				actor_get_weapon_definition(actor_index);
		firing_variant =
			actor_combat_get_firing_variant_definition(actor_index);

		if (weapon_definition != NULL &&
			prop->distance <
				weapon_definition->minimum_target_range)
		{
ranged_weapon_range_two:
			range_weight = 2;
			goto range_weight_done;
		}

		if (prop->underwater != actor->input.underwater)
		{
			range_weight = 2;
			goto range_weight_done;
		}

		if (prop->distance < 2.0f)
		{
			range_weight = 5;
			if (prop->state != _prop_state_inspected_orphan)
				goto range_weight_done;
		}

		if (prop->distance <
			firing_variant->ranged_combat.combat_range_upper_bound)
			range_weight = 3;
		else if (prop->distance <
			firing_variant->ranged_combat.maximum_firing_range)
			range_weight = 2;
		else
			range_weight = 1;
	}

range_weight_done:
	if (prop->dead)
		knowledge_weight = 1;
	else
	{
		if (!actor->meta.swarm &&
			prop->currently_damaging_me &&
			prop->unreachable_ticks == 0)
		{
			knowledge_weight = 6;
		}
		else if (prop->state >= 2 &&
			prop->state <= 3)
		{
			if (actor->meta.swarm)
				knowledge_weight = 4;
			else if (prop->unreachable_ticks > 0)
			{
				knowledge_weight = 3;
			}
			else if (prop->line_of_sight != 0 &&
				prop->line_of_sight != 1)
			{
				knowledge_weight = 3;
			}
			/* Preserve January's quantized-facing read at +0x122. */
			else if (prop->shooting &&
				prop->quantized_facing <= 1)
			{
				knowledge_weight = 5;
			}
			else
				knowledge_weight = 4;
		}
		else
		{
			if (prop->state < _prop_state_uninspected_orphan ||
				prop->state > _prop_state_inspected_orphan)
			{
				display_assert(
					"prop_orphaned(prop)",
					"c:\\halo\\SOURCE\\ai\\actor_perception.c",
					4230,
					TRUE);
				system_exit(-1);
			}

			if (prop->definitely_located)
				knowledge_weight = 3;
			else
				knowledge_weight =
					(prop->state ==
						_prop_state_uninspected_orphan) + 1;
		}
	}

	if (actor->target.target_prop_index == NONE)
	{
		if (prop->player ||
			prop_index ==
				actor_perception_preferred_target_prop_index(actor))
		{
			weights.bonus_weight = 3.0f;
		}
	}
	else if (prop_index == actor->target.target_prop_index &&
		actor_perception_target_weight_combat_status(actor) >= 3)
	{
		weights.target_weight = 1;
	}

	if (prop->preferred_target)
		weights.preferred_weight = 2;

	weights.target_weight =
		((short)weights.target_weight +
			(short)weights.preferred_weight) +
		(knowledge_weight + range_weight);

	/* Preserve January's swapped target/distance scale constants. */
	return
		weights.target_weight * 10.0f +
		(5.0f / (prop->distance * 0.1f + 1.0f) +
			weights.bonus_weight);
}




struct actor_emotion_priority_prop_view
{
	byte __unknown000[0x24];
	short state;
	byte __unknown026[0xC];
	short combat_status;
	byte __unknown034[0x40];
	boolean recent_damage;
	byte __unknown075[0x2F];
	boolean unopposable;
	byte __unknown0A5[0x7D];
	char visibility;
	byte __unknown123[0xC];
	boolean friend_attacking;
};

typedef char actor_emotion_priority_prop_state_offset_assert[
	offsetof(struct actor_emotion_priority_prop_view, state) == 0x24 ? 1 : -1];
typedef char actor_emotion_priority_prop_combat_status_offset_assert[
	offsetof(struct actor_emotion_priority_prop_view, combat_status) == 0x32 ? 1 : -1];
typedef char actor_emotion_priority_prop_recent_damage_offset_assert[
	offsetof(struct actor_emotion_priority_prop_view, recent_damage) == 0x74 ? 1 : -1];
typedef char actor_emotion_priority_prop_unopposable_offset_assert[
	offsetof(struct actor_emotion_priority_prop_view, unopposable) == 0xA4 ? 1 : -1];
typedef char actor_emotion_priority_prop_visibility_offset_assert[
	offsetof(struct actor_emotion_priority_prop_view, visibility) == 0x122 ? 1 : -1];
typedef char actor_emotion_priority_prop_friend_attacking_offset_assert[
	offsetof(struct actor_emotion_priority_prop_view, friend_attacking) == 0x12F ? 1 : -1];

static long code_0001f4f0(
	long prop_index)
{
	struct actor_emotion_priority_prop_view *prop;
	long priority;

	prop =
		(struct actor_emotion_priority_prop_view *)prop_get(prop_index);
	priority = 0;

	if (prop->state >= _prop_state_becoming_unacknowledged &&
		prop->state <= _prop_state_acknowledged &&
		prop->unopposable)
	{
		if (prop->recent_damage)
		{
			priority = 4;
		}
		else if (prop->friend_attacking)
		{
			priority = (prop->visibility <= 1) + 2;
		}
		else if (prop->combat_status >= 2)
		{
			priority = 1;
		}
	}

	return priority;
}

struct actor_emotion_prop_view;

struct actor_emotion_target
{
	short priority;
	short pad;
	long prop_index;
	long unit_index;
	struct actor_emotion_prop_view *prop;
	short count;
	short pad2;
	real minimum_distance_squared;
	long closest_unit_index;
};

struct actor_emotion_actor_view
{
	byte __unknown000[0x58];
	long definition_index;
	byte __unknown05C[0x348];
	long last_emotion_target_time;
	short emotion_target_ticks;
	byte __unknown3AA[2];
	long emotion_target_prop_index;
	long emotion_target_time;
};

struct actor_emotion_definition_view
{
	byte __unknown000[0x268];
	short normal_threshold;
	short vehicle_threshold;
	short player_threshold;
	byte __unknown26E[2];
	real trigger_delay_lower;
	real trigger_delay_upper;
	short casualty_threshold;
	short friend_threshold;
	byte __unknown27C[0xC];
	real target_duration_lower;
	real target_duration_upper;
};

struct actor_emotion_prop_view
{
	byte __unknown000[0x18];
	long unit_index;
	long actor_index;
	byte __unknown020[4];
	short state;
	byte __unknown026[0xC];
	short combat_status;
	byte __unknown034[0x2C];
	boolean enemy;
	byte __unknown061[0x13];
	boolean recent_damage;
	byte __unknown075[3];
	short dead_ticks;
	byte __unknown07A[0x2A];
	boolean unopposable;
	byte __unknown0A5;
	short unopposable_casualties;
	short unopposable_casualty_decay_timer;
	short emotion_trigger_ticks;
	short emotion_trigger_age;
	short emotion_trigger_threshold;
	byte __unknown0B0[0x6C];
	real distance;
	byte __unknown120[2];
	char visibility;
	byte __unknown123[0xB];
	boolean player;
	boolean friend_attacking;
	byte __unknown130[5];
	boolean vehicle_gunner;
	boolean dangerous_vehicle_driver;
};

typedef char actor_emotion_target_size_assert[
	sizeof(struct actor_emotion_target) == 0x1C ? 1 : -1];
typedef char actor_emotion_actor_last_time_offset_assert[
	offsetof(struct actor_emotion_actor_view, last_emotion_target_time) == 0x3A4 ? 1 : -1];
typedef char actor_emotion_actor_ticks_offset_assert[
	offsetof(struct actor_emotion_actor_view, emotion_target_ticks) == 0x3A8 ? 1 : -1];
typedef char actor_emotion_actor_prop_offset_assert[
	offsetof(struct actor_emotion_actor_view, emotion_target_prop_index) == 0x3AC ? 1 : -1];
typedef char actor_emotion_prop_distance_offset_assert[
	offsetof(struct actor_emotion_prop_view, distance) == 0x11C ? 1 : -1];
typedef char actor_emotion_prop_driver_offset_assert[
	offsetof(struct actor_emotion_prop_view, dangerous_vehicle_driver) == 0x136 ? 1 : -1];

static short code_0001f470(
	struct actor_emotion_target *targets,
	long unit_index,
	long actor_index,
	short *target_count,
	short maximum_target_count)
{
	short count;
	short target_index = NONE;
	short index;

	count = *target_count;
	for (index = 0; index < count; index++)
	{
		if (targets[index].unit_index == unit_index)
		{
			target_index = index;
			break;
		}
	}

	if (target_index == NONE && count < maximum_target_count)
	{
		*target_count = count + 1;
		targets[count].priority = 0;
		targets[count].prop_index = NONE;
		targets[count].unit_index = NONE;
		targets[count].prop = NULL;
		targets[count].count = 0;
		targets[count].minimum_distance_squared = FLT_MAX;
		targets[count].closest_unit_index = NONE;
		target_index = count;
	}

	return target_index;
}



short actor_visibility_at_point(
	long actor_index,
	struct actor_position_data const *position,
	real_point3d const *target_position,
	char lighting,
	short line_of_sight,
	boolean use_maximum_distance,
	boolean target_is_player,
	short perception_knowledge)
{
	long result = 0;

	if (line_of_sight == 0 || line_of_sight == 1)
	{
		struct actor_datum *actor = actor_get(actor_index);
		struct actor_definition *definition =
			actor_definition_get(actor->meta.definition_index);
		struct actor_variant_definition *firing_variant =
			actor_combat_get_firing_variant_definition(actor_index);
		real maximum_distance =
			definition->perception.maximum_vision_distance;
		real_vector3d direction;
		real distance_squared;
		real full_distance;
		real partial_distance;

		if (firing_variant->ranged_combat.modified_vision_range > 0.0f)
		{
			maximum_distance =
				firing_variant->ranged_combat.modified_vision_range;
		}

		{
			real knowledge_factor = 1.0f;

			switch (perception_knowledge)
			{
			case 0:
				knowledge_factor = 0.4f;
				break;
			case 1:
				knowledge_factor = 0.6f;
				break;
			case 2:
				knowledge_factor = 0.8f;
				break;
			case 3:
				knowledge_factor = 1.0f;
				break;
			default:
				display_assert(
					"!\"unreachable\"",
					"c:\\halo\\SOURCE\\ai\\actor_perception.c",
					1268,
					TRUE);
				system_exit(-1);
				break;
			}
			maximum_distance *= knowledge_factor;
		}

		{
			vector_from_points3d(
				&position->head_position,
				target_position,
				&direction);
			distance_squared = magnitude_squared3d(&direction);

			if (distance_squared < maximum_distance * maximum_distance)
			{
				real perception_factor = 1.0f;

				if (!TEST_FLAG(definition->flags, 0))
				{
					switch (lighting)
					{
					case 0:
						perception_factor = 0.3f;
						break;
					case 1:
						perception_factor = 0.7f;
						break;
					}
				}

				{
					real fog =
						scenario_fog_at_point(
							&position->body_location,
							&position->head_position,
							target_position);

					if (fog > 0.8f)
					{
						perception_factor = 0.15f;
					}
					else
					{
						if (fog > 0.2f)
						{
							perception_factor =
								(0.8f - fog) *
								perception_factor *
								1.6666666f;
						}

						if (perception_factor > 0.15f)
							goto perception_factor_ready;

						perception_factor = 0.15f;
					}
				}

perception_factor_ready:
				{
					struct actor_debug_info *debug =
						&actor_debug_array[
							DATUM_INDEX_TO_ABSOLUTE_INDEX(actor_index)];

					if (target_is_player)
					{
						debug->vision_last_time = game_time_get();
						debug->vision_last_maximum_distance =
							maximum_distance;
						debug->vision_last_perception_factor =
							perception_factor;
					}
				}

				{
					real visible_distance =
						perception_factor * maximum_distance;
					real visible_distance_i = visible_distance;
					real visible_distance_j = visible_distance;

					if (distance_squared <
						visible_distance_i * visible_distance_j)
					{
						if (!actor->meta.swarm && use_maximum_distance)
						{
							real_vector3d facing;
							real pitch;

							facing.i =
								dot_product3d(
									&direction,
									&actor->input.looking_vector);
							facing.j =
								dot_product3d(
									&direction,
									&actor->input.looking_left_vector);
							facing.k =
								dot_product3d(
									&direction,
									&actor->input.looking_up_vector);
							pitch =
								arctangent(
									facing.k,
									square_root(
										facing.i * facing.i +
											facing.j * facing.j));

							if (pitch > 0.5235988f ||
								pitch < -0.7853982f)
							{
								full_distance = 0.0f;
								visible_distance = 0.0f;
							}
							else
							{
								actor_get_vision_distances(
									actor_index,
									maximum_distance,
									perception_factor,
									fabs(arctangent(
										facing.j,
										facing.i)),
									&full_distance,
									&partial_distance);
								visible_distance = partial_distance;
							}
						}
						else
						{
							full_distance =
								0.7f * visible_distance;
						}

						if (line_of_sight == 0 &&
							distance_squared <
								full_distance * full_distance)
						{
							result =
								distance_squared < 36.0f ? 3 : 2;
						}
						else if (distance_squared <
							visible_distance * visible_distance)
						{
							result = 1;
						}
					}
				}
			}
		}
	}

	return result;
}

short actor_audibility_at_point(
	long actor_index,
	struct actor_position_data const *position,
	real_point3d const *source_position,
	struct location const *source_location,
	short source_type,
	real scale,
	short line_of_sight)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_definition *definition =
		actor_definition_get(actor->meta.definition_index);
	short result = 0;

	if (source_type != 0 &&
		position->body_location.cluster_index != NONE &&
		source_location->cluster_index != NONE)
	{
		real maximum_distance =
			definition->perception.hearing_distance;
		real encoded_distance = -1.0f;
		real audible_distance = -1.0f;
		real_vector3d direction;
		real distance_squared;

		vector_from_points3d(
			&position->head_position,
			source_position,
			&direction);
		distance_squared = magnitude_squared3d(&direction);

		if (dot_product3d(
				&direction,
				&position->body_facing) < 0.0f)
		{
			maximum_distance *= 0.8f;
		}

		if (actor_perception_audibility_combat_status(actor) == 2)
		{
			maximum_distance *= 0.7f;
		}
		else if (actor_perception_audibility_combat_status(actor) == 1)
		{
			maximum_distance *= 0.4f;
		}

		if (source_type == 4)
			maximum_distance *= 0.2f;
		else if (source_type == 1)
			maximum_distance *= 0.45f;
		else if (source_type == 3)
			maximum_distance *= 0.7f;

		if (scenario_location_deafening(&position->body_location) ||
			scenario_location_deafening(source_location))
		{
			maximum_distance *= 0.25f;
		}

		if (line_of_sight != 0 && line_of_sight != 1)
			maximum_distance *= 0.7f;

		if (distance_squared < maximum_distance * maximum_distance)
		{
			char encoded_sound_distance =
				(char)structure_bsp_get_cluster_encoded_sound_distance(
					global_structure_bsp_get(),
					source_location->cluster_index,
					position->body_location.cluster_index);

			if (!(encoded_sound_distance & FLAG(7)))
			{
				encoded_distance =
					((byte)encoded_sound_distance & ~FLAG(7)) *
						2.015748f;
				audible_distance = encoded_distance * 2.0f;
				audible_distance =
					MAX(
						audible_distance,
						square_root(distance_squared));

				if (audible_distance < maximum_distance)
					result = (source_type >= 3) + 2;
			}
		}

		{
			struct actor_debug_info *debug =
				&actor_debug_array[
					DATUM_INDEX_TO_ABSOLUTE_INDEX(actor_index)];

			debug->field_A8 = maximum_distance;
			debug->field_A4 = TRUE;
			debug->field_A6 = result;
			debug->field_B0 = encoded_distance;
			debug->field_AC = square_root(distance_squared);
			debug->field_B4 = audible_distance;
		}
	}

	return result;
}

void code_0001f560(
	long actor_index)
{
	struct actor_emotion_target targets[16];
	struct actor_emotion_actor_view *actor;
	struct prop_iterator iterator;
	short target_count;
	long target_prop_index;
	struct actor_emotion_definition_view *definition;
	struct actor_emotion_prop_view *prop;

	actor =
		(struct actor_emotion_actor_view *)actor_get(actor_index);
	definition =
		(struct actor_emotion_definition_view *)
			actor_definition_get(actor->definition_index);
	target_count = 0;

	prop_iterator_new(&iterator, actor_index);
	prop =
		(struct actor_emotion_prop_view *)
			prop_iterator_next(&iterator);
	while (prop != NULL)
	{
		short priority =
			(short)code_0001f4f0(iterator.index);

		if (priority > 0)
			goto add_direct_emotion_target;

		goto consider_friend_emotion_target;

add_direct_emotion_target:
		{
			short target_index = (short)code_0001f470(
				targets,
				prop->unit_index,
				actor_index,
				&target_count,
				NUMBEROF(targets));

			if (target_index != NONE)
			{
				struct actor_emotion_target *target =
					&targets[target_index];

				if (target->priority < priority)
				{
					target->prop_index = iterator.index;
					target->unit_index = prop->unit_index;
					target->prop = prop;
					target->priority = priority;
				}
			}
		}
		goto next_emotion_prop;

consider_friend_emotion_target:
		if (prop->state >= _prop_state_becoming_unacknowledged &&
			prop->state <= _prop_state_acknowledged &&
			!prop->enemy &&
			prop->actor_index != NONE &&
			prop->distance < 8.0f)
		{
			struct actor_emotion_actor_view *friend_actor =
				(struct actor_emotion_actor_view *)actor_get(prop->actor_index);

			if (friend_actor->emotion_target_ticks != 0 &&
				friend_actor->emotion_target_prop_index != NONE &&
				(actor->last_emotion_target_time == NONE ||
					actor->last_emotion_target_time <=
						friend_actor->emotion_target_time))
			{
				struct actor_emotion_prop_view *friend_target_prop =
					(struct actor_emotion_prop_view *)prop_get(
						friend_actor->emotion_target_prop_index);

				target_prop_index =
					prop_get_active_by_unit_index(
						actor_index,
						friend_target_prop->unit_index);

				if (target_prop_index != NONE)
				{
					struct actor_emotion_prop_view *target_prop =
						(struct actor_emotion_prop_view *)
							prop_get(target_prop_index);

					if (target_prop->state >=
							_prop_state_becoming_unacknowledged &&
						target_prop->state <= _prop_state_acknowledged &&
						target_prop->unopposable)
					{
						short target_index = (short)code_0001f470(
							targets,
							friend_target_prop->unit_index,
							actor_index,
							&target_count,
							NUMBEROF(targets));

						if (target_index != NONE)
						{
							struct actor_emotion_target *target =
								&targets[target_index];
							real distance_squared =
								friend_target_prop->distance *
									friend_target_prop->distance;

							target->count++;
							if (distance_squared <
								target->minimum_distance_squared)
							{
								target->minimum_distance_squared =
									distance_squared;
								target->closest_unit_index =
									prop->actor_index;
							}

							if (target->prop_index == NONE)
							{
								target->prop_index =
									target_prop_index;
								target->unit_index =
									target_prop->unit_index;
								target->prop = target_prop;
							}
						}
					}
				}
			}
		}

next_emotion_prop:
		prop =
			(struct actor_emotion_prop_view *)
				prop_iterator_next(&iterator);
	}

	if (target_count > 0)
	{
		short target_index;

		for (target_index = 0;
			target_index < target_count;
			target_index++)
		{
			struct actor_emotion_target *target = &targets[target_index];
			struct actor_emotion_prop_view *target_prop = target->prop;
			short threshold = definition->normal_threshold;
			boolean player_triggered = FALSE;

			if (target_prop->vehicle_gunner ||
				target_prop->dangerous_vehicle_driver)
			{
				threshold = definition->vehicle_threshold;
			}

			if (target_prop->player &&
				definition->player_threshold > 0 &&
				definition->player_threshold < threshold)
			{
				threshold = definition->player_threshold;
			}

			if (threshold > 0 &&
				target->priority >= threshold)
			{
				if (target_prop->player)
					player_triggered = TRUE;
				else
					target_prop->emotion_trigger_ticks = 22;
			}
			else if (target_prop->player)
			{
				target_prop->emotion_trigger_ticks = 22;
			}

			if (target_prop->emotion_trigger_ticks > 0)
			{
				if (target_prop->emotion_trigger_age == 0)
				{
					target_prop->emotion_trigger_threshold =
						(short)(real_seed_random_range(
							get_global_random_seed_address(),
							definition->trigger_delay_lower,
							definition->trigger_delay_upper) *
							30.0f);
				}

				target_prop->emotion_trigger_ticks--;
				target_prop->emotion_trigger_age++;
			}

			if (target_prop->dead_ticks >= 45 ||
				target->priority >= 4)
			{
				if (target_prop->emotion_trigger_threshold > 0 &&
					target_prop->emotion_trigger_age >=
						target_prop->emotion_trigger_threshold)
				{
					target->priority =
						MAX(target->priority, 7);
				}

				if (player_triggered)
					target->priority =
						MAX(target->priority, 8);

				if (definition->casualty_threshold > 0 &&
					target_prop->unopposable_casualties >=
						definition->casualty_threshold)
				{
					target->priority =
						MAX(target->priority, 9);
				}

				if (definition->friend_threshold > 0 &&
					target->count >=
						definition->friend_threshold)
				{
					target->priority =
						MAX(target->priority, 6);
				}
			}
		}
	}

	if (actor->emotion_target_ticks > 0)
	{
		actor->emotion_target_ticks--;
		if (actor->emotion_target_ticks == 0)
		{
			actor->last_emotion_target_time = game_time_get();
			return;
		}
	}
	else
	{
		long best_prop_index = NONE;
		short best_priority = 5;
		short target_index;

		for (target_index = 0;
			target_index < target_count;
			target_index++)
		{
			if (targets[target_index].priority > best_priority &&
				targets[target_index].prop_index != NONE)
			{
				best_priority = targets[target_index].priority;
				best_prop_index = targets[target_index].prop_index;
			}
		}

		if (best_prop_index != NONE)
		{
			actor->emotion_target_ticks =
				(short)(real_seed_random_range(
					get_global_random_seed_address(),
					definition->target_duration_lower,
					definition->target_duration_upper) *
					30.0f);
			actor->emotion_target_prop_index = best_prop_index;
			actor->emotion_target_time = game_time_get();
		}
	}

	return;
}

void actor_berserk(
	long actor_index,
	boolean berserk)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (berserk != actor->emotions.berserk)
	{
		actor->emotions.berserk = berserk;
		actor->emotions.played_berserk_sound = FALSE;

		if (actor->meta.swarm)
		{
			long object_index;

			for (object_index = actor->meta.swarm_unit_index;
				object_index != NONE;)
			{
				struct unit_datum *unit = unit_get(object_index);

				SET_FLAG(
					unit->object.damage_flags,
					_object_melee_attack_inhibited_bit,
					TRUE);
				object_index = unit->unit.swarm_next_unit_index;
			}
		}
		else
		{
			struct unit_datum *unit = unit_get(actor->meta.unit_index);

			SET_FLAG(
				unit->unit.flags,
				7,
				berserk);
		}

		if (berserk)
			actor->emotions.forced_to_charge = TRUE;
	}

	return;
}

long actor_perception_find_killer_prop_index(
	long actor_index,
	long prop_index,
	boolean enemies_only)
{
	struct prop_datum *prop = prop_get(prop_index);
	struct unit_datum *unit = unit_get(prop->unit_index);
	long killer_prop_index = NONE;
	long most_recent_damage_time = 0;
	short attacker_index;

	for (attacker_index = 0;
		attacker_index < MAXIMUM_ATTACKERS_PER_UNIT;
		attacker_index++)
	{
		long *attacker_object_index =
			&unit->unit.attackers[attacker_index].object_index;
		long damage_time = attacker_object_index[-2];
		long unit_index =
			ai_get_responsible_unit(
				*attacker_object_index,
				TRUE);

		if (unit_index != NONE)
		{
			long current_prop_index =
				prop_get_active_by_unit_index(
					actor_index,
					unit_index);

			if (current_prop_index != NONE)
			{
				struct prop_datum *current_prop =
					prop_get(current_prop_index);

				if (current_prop->state >=
						_prop_state_becoming_unacknowledged &&
					current_prop->state <=
						_prop_state_acknowledged &&
					(current_prop->enemy || !enemies_only) &&
					damage_time > most_recent_damage_time)
				{
					killer_prop_index = current_prop_index;
					most_recent_damage_time = damage_time;
				}
			}
		}
	}

	return killer_prop_index;
}

long actor_perception_find_recent_damaging_prop_index(
	long actor_index,
	boolean enemies_only)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	long damaging_prop_index = NONE;

	if (actor->unit_index != NONE)
	{
		struct unit_datum *unit = unit_get(actor->unit_index);
		unsigned long most_recent_damage_time = 0;
		short attacker_index;

		for (attacker_index = 0;
			attacker_index < MAXIMUM_ATTACKERS_PER_UNIT;
			attacker_index++)
		{
			struct unit_attacker *attacker =
				&unit->unit.attackers[attacker_index];
			long unit_index =
				ai_get_responsible_unit(
					attacker->object_index,
					TRUE);

			if (unit_index != NONE)
			{
				long current_prop_index =
					prop_get_active_by_unit_index(
						actor_index,
						unit_index);

				if (current_prop_index != NONE)
				{
					struct prop_datum *current_prop =
						prop_get(current_prop_index);

					if (current_prop->state >=
							_prop_state_becoming_unacknowledged &&
						current_prop->state <=
							_prop_state_acknowledged &&
						(current_prop->enemy || !enemies_only) &&
						attacker->game_time_stamp >
							most_recent_damage_time)
					{
						damaging_prop_index =
							current_prop_index;
						most_recent_damage_time =
							attacker->game_time_stamp;
					}
				}
			}

		}

#line 3726 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		assert(damaging_prop_index != 0x00000000);
#line 500 "source\\ai\\actor_perception.c"
	}

	return damaging_prop_index;
}

void actor_perception_forget_recent_damage(
	long actor_index)
{
	struct prop_iterator iterator;
	struct prop_datum *prop;

	prop_iterator_new(&iterator, actor_index);
	prop = prop_iterator_next(&iterator);
	while (prop != NULL)
	{
		prop->currently_damaging_me = FALSE;
		prop->ticks_since_damage = NONE;
		prop = prop_iterator_next(&iterator);
	}

	return;
}

void actor_perception_retreat_successful(
	long actor_index)
{
	struct prop_iterator iterator;
	struct prop_datum *prop;

	actor_get(actor_index);
	prop_iterator_new(&iterator, actor_index);
	prop = prop_iterator_next(&iterator);
	while (prop != NULL)
	{
		prop->unopposable_trigger_hysteresis = 0;
		prop->unopposable_trigger_threshold = 0;
		prop->unopposable_trigger_timer = 0;
		prop = prop_iterator_next(&iterator);
	}

	return;
}

boolean actor_compute_prop_unopposable(
	long actor_index,
	long prop_index)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	struct actor_perception_prop_view *prop =
		(struct actor_perception_prop_view *)prop_get(prop_index);
	short type = prop->state;
	boolean result = FALSE;

	if (type >= 2 && type <= 3 &&
		prop->enemy &&
		!prop->dead)
	{
		if (prop->unreachable_ticks != 0 &&
			(actor->target_prop_index == prop_index ||
				!actor->searching))
		{
			result = TRUE;
		}
		else if ((prop->vehicle_gunner ||
			prop->dangerous_vehicle_driver) &&
			!actor->flying &&
			!actor->vehicle_passenger)
		{
			result = TRUE;
		}
		else if (prop->type == 15)
		{
			result = TRUE;
		}
	}

	if (prop->unopposable_enemy && !result)
	{
		prop->unopposable_trigger_hysteresis = 0;
		prop->unopposable_trigger_threshold = 0;
		prop->unopposable_trigger_timer = 0;
	}

	if (type >= 2 && type <= 3 &&
		!result &&
		actor->unopposable_retreat_timer > 0 &&
		actor->unopposable_retreat_prop_index == prop_index)
	{
		actor->unopposable_retreat_timer = 0;
		actor->unopposable_retreat_prop_index = NONE;
	}

	prop->unopposable_enemy = result;

	return result;
}

void actor_perception_find_prop_pathfinding_location(
	long actor_index,
	long prop_index)
{
	struct actor_perception_prop_view *prop =
		(struct actor_perception_prop_view *)prop_get(prop_index);

#line 3585 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
	assert(prop->owner_actor_index == actor_index);
#line 510 "source\\ai\\actor_perception.c"

	if (prop->pathfinding_surface_index == NONE)
	{
		if (prop->vehicle_index != NONE)
		{
			prop->pathfinding_surface_index =
				vehicle_find_pathfinding_surface_index(
					prop->vehicle_index,
					&prop->pathfinding_point);
			return;
		}

		if (biped_try_and_get(prop->unit_index))
		{
			prop->pathfinding_surface_index =
				biped_find_pathfinding_surface_index(
					prop->unit_index,
					&prop->pathfinding_point);
		}
	}

	return;
}

void actor_perception_tried_to_uncover(
	long actor_index,
	long prop_index)
{
	if (prop_index != NONE)
	{
		struct actor_datum *actor = actor_get(actor_index);
		struct prop_datum *prop = prop_get(prop_index);

		prop->tried_to_uncover = TRUE;
		if (prop_index != actor->target.target_prop_index)
		{
		}
		else
		{
			actor_situation_update_target_status(actor_index);
			actor_situation_combat_status_update(actor_index);
		}
	}

	return;
}

void actor_perception_tried_to_search(
	long actor_index,
	long prop_index)
{
	if (prop_index != NONE)
	{
		struct actor_datum *actor = actor_get(actor_index);
		struct prop_datum *prop = prop_get(prop_index);

		prop->tried_to_search = TRUE;
		if (prop_index != actor->target.target_prop_index)
		{
		}
		else
		{
			actor_situation_update_target_status(actor_index);
			actor_situation_combat_status_update(actor_index);
		}
	}

	return;
}

void actor_perception_abandoned_search(
	long actor_index,
	long prop_index)
{
	if (prop_index == NONE)
	{
		struct actor_datum *actor = actor_get(actor_index);

		actor->firing_positions.pursuit_positions_count = 0;
		actor->firing_positions.pursuit_fired_at_orphan = FALSE;
		actor->firing_positions.pursuit_communicated_lost_contact = FALSE;
		actor->state.artificial_combat_status = 0;
		actor->state.suspicion_combat_status = 0;
		actor_situation_combat_status_update(actor_index);
	}
	else
	{
		struct actor_datum *actor = actor_get(actor_index);
		struct prop_datum *prop = prop_get(prop_index);

		if (prop->state == _prop_state_uninspected_orphan)
			prop->state = _prop_state_inspected_orphan;

		prop->abandoned_search = TRUE;
		if (prop_index != actor->target.target_prop_index)
		{
		}
		else
		{
			actor_situation_update_target_status(actor_index);
			actor_situation_combat_status_update(actor_index);
		}
	}

	return;
}


void actor_perception_unreachable(
	long actor_index,
	long prop_index,
	boolean unreachable)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct prop_datum *prop = prop_get(prop_index);

	if (unreachable)
	{
		if (prop->unreachable_ticks == 0)
			prop->unreachable_ticks = 1;

		prop->last_unreachable_time = game_time_get();
	}
	else
	{
		prop->unreachable_ticks = 0;
		prop->last_unreachable_time = NONE;
	}

	prop->unopposable_enemy =
		actor_compute_prop_unopposable(actor_index, prop_index);
	prop->target_weight =
		actor_compute_prop_target_weight(actor_index, prop_index);

	return;
}

boolean actor_perception_become_acknowledged(
	long actor_index,
	long prop_index,
	boolean *expected_acknowledgement_out)
{
	struct actor_perception_prop_view *prop =
		(struct actor_perception_prop_view *)prop_get(prop_index);
	boolean result = FALSE;
	boolean expected_acknowledgement = FALSE;

	if (prop->state < 2 || prop->state > 3)
	{
		long orphaned = prop->orphan_prop_index != NONE;

		expected_acknowledgement =
			actor_expected_acknowledgement(actor_index, prop_index);

		if (orphaned)
		{
			struct actor_perception_prop_view *orphan =
				(struct actor_perception_prop_view *)prop_get(
					prop->orphan_prop_index);

			prop->target_weight = orphan->target_weight;
			prop->look_interest = orphan->look_interest;
			prop->last_idle_look_interest =
				orphan->last_idle_look_interest;
			prop->last_idle_look_time = orphan->last_idle_look_time;
			prop->unreachable_ticks = orphan->unreachable_ticks;
			prop->last_unreachable_time = orphan->last_unreachable_time;
			prop->unopposable_enemy = orphan->unopposable_enemy;
			prop->unopposable_casualties_inflicted =
				orphan->unopposable_casualties_inflicted;
			prop->unopposable_casualty_decay_timer =
				orphan->unopposable_casualty_decay_timer;

			actor_switch_props(
				actor_index,
				prop->orphan_prop_index,
				prop_index);
			prop_delete(actor_index, prop->orphan_prop_index);
			prop->orphan_prop_index = NONE;
		}

		prop->state = 3;
		actor_perception_acknowledge(
			actor_index,
			prop_index,
			orphaned,
			expected_acknowledgement);
		result = TRUE;
	}

	if (expected_acknowledgement_out)
		*expected_acknowledgement_out = expected_acknowledgement;

	return result;
}

boolean actor_expected_acknowledgement(
	long actor_index,
	long prop_index)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	struct actor_perception_prop_view *prop =
		(struct actor_perception_prop_view *)prop_get(prop_index);
	struct prop_iterator iterator;
	struct actor_perception_prop_view *current_prop;
	boolean result = FALSE;
	real delta_x;
	real delta_y;

#line 3613 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
	match_vassert(
		__FILE__,
		__LINE__,
		!(prop->state >= _prop_state_uninspected_orphan &&
			prop->state <= _prop_state_inspected_orphan),
		"!prop_orphaned(prop)");
#line 790 "source\\ai\\actor_perception.c"

	prop_iterator_new(&iterator, actor_index);
	current_prop =
		(struct actor_perception_prop_view *)prop_iterator_next(&iterator);
	while (current_prop != NULL)
	{
		if (iterator.index != prop_index &&
			(current_prop->unit_index == prop->unit_index ||
				current_prop->actor_index == prop->actor_index ||
				(prop->enemy &&
					current_prop->enemy &&
					((current_prop->state >=
							_prop_state_uninspected_orphan &&
							current_prop->state <=
							_prop_state_inspected_orphan) ||
						(current_prop->state >=
							_prop_state_becoming_unacknowledged &&
							current_prop->state <=
							_prop_state_acknowledged)))) &&
			actor_perception_distance_squared2d(
				&current_prop->body_position,
				&prop->body_position,
				delta_x,
				delta_y) < 6.25f &&
			fabs(current_prop->body_position.z - prop->body_position.z) <
				1.5f &&
			dot_product3d(
				&current_prop->actor_to_prop,
				&prop->actor_to_prop) > 0.5f)
		{
			result = TRUE;
		}

		current_prop =
			(struct actor_perception_prop_view *)prop_iterator_next(
				&iterator);
	}

	return result;
}

void actor_perception_find_sense_position(
	long actor_index,
	real_point3d const *position,
	long unused,
	struct actor_position_data *sense_position)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->meta.swarm)
	{
		struct swarm_datum *swarm =
			swarm_get(actor->meta.swarm_cache_index);
		real best_distance_squared = FLT_MAX;
		long best_unit_index = NONE;
		short unit_index;

#line 1637 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		assert(actor->meta.swarm_unit_count > 0);
#line 1634 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		match_vassert(
			__FILE__,
			__LINE__,
			actor->meta.swarm_unit_index != NONE,
			"actor->meta.swarm_unit_index != NONE");
#line 970 "source\\ai\\actor_perception.c"

		for (unit_index = 0; unit_index < swarm->unit_count; unit_index++)
		{
			struct actor_perception_swarm_component_view *component =
				swarm_component_get(swarm->component_indices[unit_index]);
			real distance_squared =
				distance_squared3d(&component->position, position);

			if (distance_squared < best_distance_squared)
			{
				best_distance_squared = distance_squared;
				best_unit_index = swarm->unit_indices[unit_index];
			}
		}

#line 1651 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		match_vassert(
			__FILE__,
			__LINE__,
			best_unit_index != NONE,
			"best_unit_index != NONE");
#line 990 "source\\ai\\actor_perception.c"

		actor_input_sample_position(
			actor_index,
			best_unit_index,
			sense_position);
	}
	else
	{
		*sense_position = actor->input.position;
	}

	return;
}

static long code_00020210(
	long swarm_actor_index,
	long actor_index,
	long existing_unit_index,
	boolean mark,
	struct actor_position_data const *position)
{
	struct actor_datum *swarm_actor = actor_get(swarm_actor_index);
	long best_unit_index = NONE;

#line 1677 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
	assert(swarm_actor->meta.swarm);
#line 1001 "source\\ai\\actor_perception.c"

	if (swarm_actor->meta.swarm_cache_index != NONE)
	{
		struct swarm_datum *swarm =
			swarm_get(swarm_actor->meta.swarm_cache_index);
		real best_distance_squared = FLT_MAX;
		short unit_index;

		for (unit_index = 0; unit_index < swarm->unit_count; unit_index++)
		{
			struct actor_perception_swarm_component_view *component =
				swarm_component_get(swarm->component_indices[unit_index]);
			real distance_squared =
				distance_squared3d(
					&component->position,
					&position->body_position);

			if (TEST_FLAG(component->flags, 1))
			{
				distance_squared *= 2.25f;
			}
			else if (swarm->unit_indices[unit_index] == existing_unit_index)
			{
				distance_squared *= 0.36f;
			}

			if (distance_squared < best_distance_squared)
			{
				best_distance_squared = distance_squared;
				best_unit_index = swarm->unit_indices[unit_index];
			}

			if (mark)
			{
				object_mark_function(swarm->unit_indices[unit_index]);
			}
		}
	}
	else
	{
		long unit_index = swarm_actor->meta.swarm_unit_index;
		real best_distance_squared = FLT_MAX;

		while (unit_index != NONE)
		{
			struct unit_datum *unit = unit_get(unit_index);
			real_point3d origin;
			real distance_squared;

			object_get_origin(unit_index, &origin);
			distance_squared =
				distance_squared3d(&origin, &position->body_position);
			if (unit_index == existing_unit_index)
			{
				distance_squared *= 0.36f;
			}

			if (distance_squared < best_distance_squared)
			{
				best_distance_squared = distance_squared;
				best_unit_index = unit_index;
			}

			if (mark)
			{
				object_mark_function(unit_index);
			}

			unit_index = unit->unit.swarm_next_unit_index;
		}
	}

#line 1749 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
	match_vassert(
		__FILE__,
		__LINE__,
		existing_unit_index == NONE || best_unit_index != NONE,
		"(existing_unit_index == NONE) || (best_unit_index != NONE)");
#line 1090 "source\\ai\\actor_perception.c"

	return best_unit_index;
}

void prop_position_refresh(
	long actor_index,
	long prop_index,
	struct actor_position_data *position,
	boolean refresh_position,
	boolean refresh_vehicle)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->meta.active)
	{
		long current_prop_index = prop_index;
		struct prop_datum *prop = prop_get(current_prop_index);
		struct unit_datum *unit = unit_get(prop->unit_index);

		if (!refresh_position &&
			prop->state >= _prop_state_uninspected_orphan &&
			prop->state <= _prop_state_inspected_orphan)
		{
			boolean corpse_stopped;

			if (prop->orphan_corpse_cheated)
			{
				goto update_actor_to_prop;
			}

			if (TEST_FLAG(unit->object.damage_flags, 2) &&
				((struct actor_perception_unit_view *)unit)
						->parent_seat_index == 0 &&
				prop->perception == 0 &&
				magnitude_squared3d(&unit->object.translational_velocity) <
					0.010000000707805157f)
			{
				corpse_stopped = TRUE;
			}
			else
			{
				corpse_stopped = FALSE;
			}

			if (actor->target.target_prop_index == current_prop_index &&
				(!prop->tried_to_uncover || !prop->tried_to_search))
			{
				goto update_actor_to_prop;
			}

			if (!corpse_stopped)
			{
				goto update_actor_to_prop;
			}

			prop->orphan_corpse_cheated = TRUE;
			prop->dead = TRUE;
		}

		if (prop->swarm &&
			prop->actor_index != NONE &&
			refresh_vehicle)
		{
			long current_time = game_time_get();

			if (prop->swarm_unit_selected_time + 90 <= current_time)
			{
				long new_unit_index;

				prop->swarm_unit_selected_time = current_time;
				new_unit_index =
					code_00020210(
						prop->actor_index,
						actor_index,
						prop->unit_index,
						FALSE,
						&actor->input.position);
				if (new_unit_index != prop->unit_index)
				{
					prop->unit_index = new_unit_index;
					unit = unit_get(new_unit_index);
					if (prop->state >= _prop_state_uninspected_orphan &&
						prop->state <= _prop_state_inspected_orphan)
					{
						struct prop_datum *parent_prop =
							prop_get(prop->orphan_prop_index);
						parent_prop->unit_index = prop->unit_index;
					}
					else if (prop->orphan_prop_index != NONE)
					{
						struct prop_datum *parent_prop =
							prop_get(prop->orphan_prop_index);
						parent_prop->unit_index = prop->unit_index;
					}
				}
			}
		}

		unit_get_head_position(prop->unit_index, &prop->head_position);
		object_get_origin(prop->unit_index, &prop->body_position);
		unit_get_center_of_mass(prop->unit_index, &prop->center_of_mass);
		prop->velocity = unit->object.translational_velocity;
		prop->pathfinding_surface_index = NONE;
		prop->body_location =
			actor_perception_object_get(
				object_get_ultimate_parent(prop->unit_index))
				->object.location;
		prop->underwater =
			scenario_location_underwater(
				&prop->body_location,
				&prop->center_of_mass,
				NULL);
		prop->vehicle_index = NONE;
		prop->vehicle_gunner = FALSE;
		prop->dangerous_vehicle_driver = FALSE;
		prop->attached_to_unit_index = NONE;

		if (unit->object.parent_object_index != NONE)
		{
			struct object_datum *parent =
				actor_perception_object_get(
					unit->object.parent_object_index);

			if (parent->object.type == _object_type_vehicle)
			{
				struct unit_datum *vehicle = (struct unit_datum *)parent;

				prop->vehicle_index = unit->object.parent_object_index;
				prop->vehicle_gunner =
					vehicle->unit.gunner_object_index == prop->unit_index ||
					prop->type == 15;
				prop->dangerous_vehicle_driver =
					vehicle->unit.driver_object_index == prop->unit_index &&
					vehicle_causes_collision_damage(prop->vehicle_index);
			}
			else if (((1 << parent->object.type) & 3) != 0)
			{
				prop->attached_to_unit_index =
					unit->object.parent_object_index;
			}
		}

		prop->child_units_attached = 0;
		{
			long child_object_index = unit->object.first_child_object_index;

			while (child_object_index != NONE)
			{
				struct object_datum *child =
					actor_perception_object_get(child_object_index);

				if (((1 << child->object.type) & _object_mask_unit) != 0)
				{
					prop->child_units_attached++;
				}

				child_object_index = child->object.next_object_index;
			}
		}

update_actor_to_prop:
		actor_perception_find_sense_position(
			actor_index,
			&prop->body_position,
			current_prop_index,
			position);
		vector_from_points3d(
			&position->body_position,
			&prop->body_position,
			&prop->actor_to_prop);
		prop->distance = normalize3d(&prop->actor_to_prop);
		if (prop->distance == 0.0f)
		{
			prop->actor_to_prop = *global_forward3d;
		}
	}

	return;
}

static boolean code_00020780(
	long actor_index,
	long vehicle_index,
	boolean mark,
	struct actor_position_data const *position)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	boolean result = FALSE;

	if (actor->vehicle_index == NONE)
	{
		struct unit_datum *vehicle = vehicle_get(vehicle_index);
		struct actor_perception_vehicle_definition_view *vehicle_definition =
			(struct actor_perception_vehicle_definition_view *)
				vehicle_definition_get(vehicle->definition_index);

		if (TEST_FLAG(vehicle_definition->danger_zone_flags, 7) &&
			magnitude_squared3d(&vehicle->object.translational_velocity) >
				0.0011111111380159855f)
		{
			real_point3d vehicle_origin;
			struct actor_position_data sampled_position;
			real distance;

			object_get_origin(vehicle_index, &vehicle_origin);
			if (position == NULL)
			{
				actor_perception_find_sense_position(
					actor_index,
					&vehicle_origin,
					NONE,
					&sampled_position);
				position = &sampled_position;
			}

			distance =
				distance3d(&position->body_position, &vehicle_origin);
			if (distance <
				vehicle_definition->bounding_radius + 10.0f)
			{
				if (actor->danger_zone.danger_type <
						_actor_danger_zone_vehicle ||
					(actor->danger_zone.danger_type ==
							_actor_danger_zone_vehicle &&
						actor->danger_zone.object_index !=
							vehicle_index &&
						distance <
							actor->danger_zone
								.current_distance_from_actor))
				{
					struct actor_perception_responsible_unit_view
						*responsible_unit;

					csmemset(
						&actor->danger_zone,
						0,
						sizeof(actor->danger_zone));
					actor->danger_zone.object_index = vehicle_index;
					actor->danger_zone.danger_type =
						_actor_danger_zone_vehicle;
					actor->danger_zone.owner_unit_index =
						vehicle->unit.driver_object_index;
					actor->danger_zone.danger_radius =
						vehicle_definition->bounding_radius;
					actor->danger_zone.initial_position = vehicle_origin;
					actor->danger_zone.initial_velocity =
						vehicle->object.translational_velocity;
					actor->danger_zone.acknowledgement_timer = 20;
					actor->danger_zone.currently_perceived = mark;
					actor->danger_zone.hostility = 0;

					if (actor->danger_zone.owner_unit_index != NONE)
					{
						responsible_unit =
							(struct actor_perception_responsible_unit_view *)
								unit_get(
									vehicle->unit
										.driver_object_index);
						if (!game_team_is_enemy(
								actor->team,
								responsible_unit->team))
						{
							actor->danger_zone.hostility = 1;
						}
					}

					result = TRUE;
				}
			}
		}
	}

	return result;
}


/*
 * January caller skeleton used while reconstructing the full status refresh.
 * Keep the real call expression active so VC7 can derive code_00020780's
 * private EAX argument from its actual translation-unit context.
 */



void code_000228b0(
	long actor_index,
	long object_index,
	struct actor_perception_refresh_list *enemy_list,
	struct actor_perception_refresh_list *friend_list)
{
	struct
	{
		struct actor_datum *actor;
		struct object_datum *current_object;
	} pointers;
#define actor pointers.actor
#define current_object pointers.current_object

	actor = actor_get(actor_index);

	while (object_index != NONE)
	{
		current_object = actor_perception_object_get(object_index);

		if (object_mark_function(object_index))
		{
			if (current_object->object.type == _object_type_biped)
			{
				struct unit_datum *unit =
					(struct unit_datum *)current_object;
				struct actor_position_data position;
				struct actor_perception_actor_view *unit_actor;
				struct actor_perception_actor_view *current_actor;
				struct actor_perception_refresh_list *list;
				struct unit_definition *unit_definition;
				real_point3d origin;
				real distance_squared;
				real suicide_radius;
				long unit_index;
				long unit_actor_index;
				long prop_index;
				short dead_ticks;
				boolean player;
				boolean enemy;
				boolean dead;
				boolean candidate;

				object_get_origin(object_index, &origin);
				actor_perception_find_sense_position(
					actor_index,
					&origin,
					NONE,
					&position);

				unit_actor_index = unit->unit.swarm_actor_index;
				if (unit_actor_index != NONE)
				{
					unit_index =
						code_00020210(
							unit_actor_index,
							actor_index,
							NONE,
							TRUE,
							&position);
					if (unit_index != NONE)
					{
						unit = unit_get(unit_index);
						object_get_origin(unit_index, &origin);
					}
				}
				else
				{
					unit_actor_index = unit->unit.actor_index;
					unit_index = object_index;
				}

				if (unit_index == NONE ||
					unit_actor_index == actor_index)
					goto object_done;

				unit_definition = unit_definition_get(unit->definition_index);
				player = unit->unit.player_index != NONE;
				enemy =
					game_team_is_enemy(
						actor->meta.team_index,
						unit->object.owner_team_index);

				if (!TEST_FLAG(unit->object.damage_flags, _object_dead_bit) ||
					unit->unit.feign_death_timer != 0)
				{
					dead = FALSE;
					dead_ticks = 0;
				}
				else
				{
					dead = TRUE;
					if (unit->unit.time_of_death == NONE)
						dead_ticks = 0x7FFF;
					else
						dead_ticks =
							(short)game_time_get() -
							(short)unit->unit.time_of_death;
				}

				suicide_radius = unit_definition->unit.ai_danger_radius;
				distance_squared =
					distance_squared3d(
						&origin,
						&position.body_position);

				if (suicide_radius > 0.0f &&
					(dead ||
						unit->unit.animation.state ==
							0x1E))
				{
					code_0001dc00(
						actor_index,
						unit_index,
						suicide_radius,
						square_root(distance_squared),
						enemy,
						FALSE);
				}

				current_actor =
					(struct actor_perception_actor_view *)actor_get(
						actor_index);
				unit_actor =
					unit_actor_index == NONE
						? NULL
						: (struct actor_perception_actor_view *)actor_get(
							unit_actor_index);
				candidate = FALSE;

				if (player)
					goto choose_list;

				if (unit_actor != NULL &&
					(!unit_actor->active || unit_actor->dormant))
				{
					goto object_done;
				}

				if (distance_squared > 1600.0f)
					goto object_done;

				if (dead)
				{
					boolean interest_allowed = TRUE;
					boolean encounter_clear = FALSE;

					if (current_actor->encounter_index != NONE)
					{
						struct actor_perception_encounter_view *encounter =
							(struct actor_perception_encounter_view *)
								encounter_get(current_actor->encounter_index);
						struct unit_datum *corpse_unit =
							unit_get(unit_index);
						long ignore_time = encounter->corpse_ignore_time;

						if (ignore_time <= current_actor->corpse_ignore_time)
							ignore_time = current_actor->corpse_ignore_time;
						if (ignore_time != NONE &&
							(corpse_unit->unit.time_of_death == NONE ||
								corpse_unit->unit.time_of_death < ignore_time))
						{
							interest_allowed = FALSE;
						}

						encounter_clear =
							!encounter->enemy_target &&
							!encounter->stand_down &&
							!encounter->blind;
					}

					if (!interest_allowed)
						goto object_done;
					if (encounter_clear)
					{
						if (distance_squared < 225.0f)
							goto choose_list;
						goto object_done;
					}
					goto corpse_fallback;
				}
				goto live_unit;

choose_list:
				list = enemy ? enemy_list : friend_list;
				if (candidate)
				{
#line 2966 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
					assert(!dead);
#line 3258 "source\\ai\\actor_perception.c"

					if (list->entry_count < 128)
					{
						list->entries[list->entry_count].prop_index = NONE;
						list->entries[list->entry_count].unit_index =
							unit_index;
						list->entries[list->entry_count].priority =
							distance_squared;
						list->entry_count++;
					}
					goto object_done;
				}

				goto create_prop;

corpse_fallback:
				if (suicide_radius > 0.0f)
					goto choose_list;

				if ((!enemy || dead_ticks <= 150) &&
					actor_action_class(actor_index) <= 1)
				{
					real maximum_distance_squared = 16.0f;

					if (!enemy && current_actor->combat_status < 3)
						maximum_distance_squared = 64.0f;
					if (distance_squared < maximum_distance_squared)
						goto choose_list;
				}
				goto object_done;

live_unit:
				if (enemy)
				{
					candidate = distance_squared > 36.0f;
					goto choose_list;
				}

				if (distance_squared >= 225.0f)
					goto object_done;

				if (current_actor->combat_status >= 4)
				{
					candidate = TRUE;
				}
				else if (!current_actor->corpse_interest_inhibited &&
					distance_squared > 16.0f)
				{
					candidate = TRUE;
				}
				goto choose_list;

create_prop:
				prop_index =
					prop_new_unacknowledged(
						actor_index,
						unit_index,
						enemy);
				if (prop_index != NONE)
				{
					prop_position_refresh(
						actor_index,
						prop_index,
						&position,
						FALSE,
						FALSE);
					if (!dead)
						list->accepted_count++;
				}
			}
			else if (current_object->object.type == _object_type_vehicle)
			{
				struct unit_datum *vehicle =
					(struct unit_datum *)current_object;

				if (vehicle->unit.driver_object_index == NONE)
				{
					code_00020780(
						actor_index,
						object_index,
						FALSE,
						NULL);
				}
			}
			else if (current_object->object.type == _object_type_projectile)
			{
				struct actor_perception_projectile_datum_view *projectile =
					(struct actor_perception_projectile_datum_view *)
						current_object;
				struct projectile_definition *projectile_definition =
					projectile_definition_get(
						current_object->definition_index);

				if (projectile_definition->projectile.danger_radius > 0.0f &&
					(current_object->object.parent_object_index == NONE ||
						TEST_FLAG(projectile->attachment_flags, 5)))
				{
					struct actor_position_data position;
					real_point3d origin;
					real distance;

					object_get_origin(object_index, &origin);
					actor_perception_find_sense_position(
						actor_index,
						&origin,
						NONE,
						&position);
					distance =
						distance3d(
							&origin,
							&position.body_position);

					if (distance <
							projectile_definition->projectile.danger_radius +
								10.0f &&
						(actor->danger_zone.danger_type < 2 ||
							(actor->danger_zone.danger_type == 2 &&
								actor->danger_zone.object_index != object_index &&
								distance <
									actor->danger_zone
										.current_distance_from_actor)))
					{
						long owner_unit_index = NONE;
						struct object_datum *owner_object = NULL;

						csmemset(
							&actor->danger_zone,
							0,
							sizeof(actor->danger_zone));
						actor->danger_zone.danger_type = 2;
						actor->danger_zone.object_index = object_index;
						actor->danger_zone.danger_radius =
							projectile_definition->projectile.danger_radius;
						actor->danger_zone.initial_position = origin;
						actor->danger_zone.initial_velocity =
							current_object->object.translational_velocity;
						actor->danger_zone.acknowledgement_timer = 30;
						actor->danger_zone.currently_perceived = FALSE;
						actor->danger_zone.hostility = 0;

						if (current_object->object.owner_object_index != NONE)
						{
							owner_object =
								object_try_and_get(
									current_object->object.owner_object_index);
						}

						if (owner_object != NULL &&
							TEST_FLAG(
								_object_mask_unit,
								owner_object->object.type))
						{
							owner_unit_index =
								current_object->object.owner_object_index;
							if (actor->meta.unit_index == NONE ||
								owner_unit_index != actor->meta.unit_index)
							{
								if (!game_team_is_enemy(
									actor->meta.team_index,
									current_object->object.owner_team_index))
								{
									actor->danger_zone.hostility = 1;
								}
							}
							else
							{
								actor->danger_zone.hostility = 2;
							}
						}

						actor->danger_zone.owner_unit_index =
							owner_unit_index;
					}
				}
			}
		}

object_done:
		if (current_object->object.first_child_object_index != NONE)
		{
			code_000228b0(
				actor_index,
				current_object->object.first_child_object_index,
				enemy_list,
				friend_list);
		}
		object_index = current_object->object.next_object_index;
	}

	return;

#undef actor
#undef current_object
}

boolean actor_perception_create_orphan_from_friend(
	long actor_index,
	long unit_index,
	long source_actor_index,
	long friend_prop_index)
{
	struct actor_orphan_prop_view *current_prop;
	struct actor_position_data position;
	long current_prop_index;
	long current_orphan_index;
	boolean result;

	result = TRUE;
	current_prop_index =
		prop_get_base_by_unit_index(
			actor_index,
			unit_index,
			TRUE,
			FALSE);

	if (current_prop_index == NONE)
		goto done;

	current_prop =
		(struct actor_orphan_prop_view *)prop_get(current_prop_index);
	if (current_prop->state >= _prop_state_becoming_unacknowledged &&
		current_prop->state <= _prop_state_acknowledged)
	{
		result = FALSE;
	}
	else if (current_prop->related_prop_index != NONE)
	{
		struct actor_orphan_prop_view *current_orphan;
		boolean refresh_position;

		current_orphan_index = current_prop->related_prop_index;
		current_orphan =
			(struct actor_orphan_prop_view *)
				prop_get(current_orphan_index);
		refresh_position = FALSE;

#line 3759 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		match_vassert(
			__FILE__,
			__LINE__,
			current_prop->state >= _prop_state_unacknowledged &&
				current_prop->state <= _prop_state_becoming_acknowledged,
			"prop_unacknowledged(current_prop)");
#line 3760 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		match_vassert(
			__FILE__,
			__LINE__,
			current_orphan->state >= _prop_state_uninspected_orphan &&
				current_orphan->state <= _prop_state_inspected_orphan,
			"prop_orphaned(current_orphan)");
#line 3762 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		match_vassert(
			__FILE__,
			__LINE__,
			current_prop->owner_actor_index == actor_index,
			"current_prop->owner_actor_index == actor_index");
#line 3763 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		match_vassert(
			__FILE__,
			__LINE__,
			current_orphan->owner_actor_index == actor_index,
			"current_orphan->owner_actor_index == actor_index");
#line 3764 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		match_vassert(
			__FILE__,
			__LINE__,
			current_prop->related_prop_index == current_orphan_index,
			"current_prop->orphan_prop_index == current_orphan_index");
#line 3765 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		match_vassert(
			__FILE__,
			__LINE__,
			current_orphan->related_prop_index == current_prop_index,
			"current_orphan->parent_prop_index == current_prop_index");
#line 545 "source\\ai\\actor_perception.c"

		if (friend_prop_index != NONE)
		{
			prop_orphan_update_information(
				actor_index,
				current_orphan_index,
				friend_prop_index);
			current_prop->unit_index = current_orphan->unit_index;
		}
		else
		{
			current_orphan->state = _prop_state_uninspected_orphan;
			current_orphan->orphan_inspection_ticks = 0;
			refresh_position = TRUE;
		}

		prop_position_refresh(
			actor_index,
			current_orphan_index,
			&position,
			refresh_position,
			TRUE);
		prop_status_refresh(
			actor_index,
			current_orphan_index,
			&position);
		current_prop_index = current_orphan_index;
		current_prop =
			(struct actor_orphan_prop_view *)
				prop_get(current_orphan_index);
	}
	else
	{
#line 3802 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		match_vassert(
			__FILE__,
			__LINE__,
			current_prop->state >= _prop_state_unacknowledged &&
				current_prop->state <= _prop_state_becoming_acknowledged,
			"prop_unacknowledged(current_prop)");
#line 586 "source\\ai\\actor_perception.c"

		if (friend_prop_index != NONE)
		{
			current_prop_index =
				prop_orphan_from_friend(
					actor_index,
					current_prop_index,
					friend_prop_index);
			if (current_prop_index != NONE)
			{
				struct actor_orphan_prop_view *new_prop =
					(struct actor_orphan_prop_view *)
						prop_get(current_prop_index);
				new_prop->unit_index = current_prop->unit_index;
				new_prop->actor_index = current_prop->actor_index;
				new_prop->swarm = current_prop->swarm;
			}
		}
		else
		{
			prop_position_refresh(
				actor_index,
				current_prop_index,
				&position,
				FALSE,
				FALSE);
			current_prop_index =
				prop_orphan_transition(
					actor_index,
					current_prop_index);
		}

		if (current_prop_index == NONE)
		{
			result = FALSE;
			goto done;
		}

		current_prop =
			(struct actor_orphan_prop_view *)
				prop_get(current_prop_index);
	}

	if (current_prop != NULL)
	{
		if (source_actor_index == NONE ||
			(friend_prop_index != NONE &&
				((struct actor_situation_prop_view *)
					prop_get(friend_prop_index))->combat_status >= 2))
		{
			current_prop->definitely_located = TRUE;
			current_prop->ticks_since_definitely_located = 0;
			current_prop->definite_knowledge_source_actor =
				source_actor_index;
		}

		current_prop->unopposable_enemy =
			actor_compute_prop_unopposable(
				actor_index,
				current_prop_index);
		current_prop->target_weight =
			actor_compute_prop_target_weight(
				actor_index,
				current_prop_index);
	}

done:
	return result;
}

boolean actor_emotion_flee_with_friends(
	long actor_index,
	real *desire_to_flee)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	struct prop_iterator iterator;
	struct actor_perception_prop_view *prop;
	short fighting_friend_count = 0;
	short fleeing_friend_count = 0;
	real modifier;

	prop_iterator_new(&iterator, actor_index);
	prop =
		(struct actor_perception_prop_view *)prop_iterator_next(&iterator);
	while (prop != NULL)
	{
		if (prop->state >= _prop_state_becoming_unacknowledged &&
			prop->state <= _prop_state_acknowledged &&
			!prop->enemy &&
			prop->type == actor->type &&
			prop->actor_index != NONE)
		{
			struct actor_perception_actor_view *friend_actor =
				(struct actor_perception_actor_view *)actor_get(
					prop->actor_index);

			if (friend_actor->active_threat_count > 0 ||
				(friend_actor->friend_state == 4 &&
					friend_actor->friend_fighting_count > 0))
			{
				fighting_friend_count++;
			}
			else if (prop->in_combat)
			{
				fleeing_friend_count++;
			}
		}

		prop =
			(struct actor_perception_prop_view *)prop_iterator_next(
				&iterator);
	}

	if (fighting_friend_count > 1)
		return TRUE;

	if (fleeing_friend_count > 1)
		modifier = 1.0f - (fleeing_friend_count - 1) * 0.25f;
	else
		modifier = 1.0f + (1 - fleeing_friend_count) * 0.5f;

	*desire_to_flee *= PIN(modifier, 0.0f, 2.0f);

	return FALSE;
}

boolean actor_situation_try_new_target(
	long actor_index,
	long new_prop_index)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	struct actor_perception_prop_view *new_prop =
		(struct actor_perception_prop_view *)prop_get(new_prop_index);
	struct actor_perception_prop_view *target_prop;
	boolean result = FALSE;

	if (actor->target_prop_index == NONE)
		target_prop = NULL;
	else
		target_prop =
			(struct actor_perception_prop_view *)prop_get(
				actor->target_prop_index);

	new_prop->target_weight =
		actor_compute_prop_target_weight(actor_index, new_prop_index);

	if (new_prop->target_weight > 0.0f)
	{
#line 4685 "c:\\halo\\SOURCE\\ai\\actor_perception.c"
		assert(new_prop->enemy);
#line 700 "source\\ai\\actor_perception.c"

		if (!target_prop ||
			new_prop->target_weight >= target_prop->target_weight)
		{
			actor->target_type = 0;
			actor->target_prop_index = new_prop_index;
			actor->target_last_visible_time = NONE;
			actor_situation_update_target_status(actor_index);
			actor_situation_combat_status_update(actor_index);
			result = TRUE;
		}
	}

	return result;
}


/* ---------- private code */

long code_0001dbc0(
	void const *a,
	void const *b)
{
	real a_value = ((real const *)a)[2];
	real b_value = ((real const *)b)[2];

	if (a_value < b_value)
		return -1;

	if (a_value > b_value)
		return 1;

	return 0;
}

static boolean code_0001dc00(
	long actor_index,
	long object_index,
	real suicide_radius,
	real distance,
	boolean enemy,
	boolean visible)
{
	struct actor_perception_actor_view *actor =
		(struct actor_perception_actor_view *)actor_get(actor_index);
	struct actor_danger_zone_view *danger_zone;
	boolean result = FALSE;

	if (suicide_radius + 10.0f > distance)
	{
		danger_zone = &actor->danger_zone;

		if (actor->danger_zone.danger_type < 1 ||
			(actor->danger_zone.danger_type == 1 &&
				actor->danger_zone.object_index != object_index &&
				distance < actor->danger_zone.current_distance_from_actor))
		{
			struct unit_datum *unit = unit_get(object_index);

			memset(
				danger_zone,
				0,
				sizeof(*danger_zone));
			actor->danger_zone.danger_type = 1;
			actor->danger_zone.object_index = object_index;
			actor->danger_zone.danger_radius = suicide_radius;
			object_get_origin(
				object_index,
				&actor->danger_zone.initial_position);
			actor->danger_zone.initial_velocity =
				unit->object.translational_velocity;
			actor->danger_zone.acknowledgement_timer = 6;
			actor->danger_zone.currently_perceived = visible;
			actor->danger_zone.hostility = !enemy;
			result = TRUE;
		}
	}

	return result;
}
