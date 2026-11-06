/* mendicant_internal.h: the declarations between MENDICANT AI's modules
(port/linux/game/mendicant; PORT_SPEC.md section 5.2). Nothing here is used by
the game's sources. Include after cseries.h, math/real_math.h and <stdio.h>. */
#ifndef HALO_MENDICANT_INTERNAL_H
#define HALO_MENDICANT_INTERNAL_H

#include "mendicant.h"

struct actor_datum;
struct unit_datum;
struct firing_position_definition;

#ifndef MENDICANT_MAX_POSITIONS
#define MENDICANT_MAX_POSITIONS 512
#endif
#ifndef MENDICANT_MAX_UNITS
#define MENDICANT_MAX_UNITS 64
#endif
#ifndef STANCES
#define STANCES 5 /* the ways of fighting a soldier's own model chooses among (bridge->stance) */
#endif
/* mendicant_unit_side's sides (the doctrine's SIDE_COVENANT and SIDE_MARINES) */
#define MENDICANT_SIDE_COVENANT 0
#define MENDICANT_SIDE_MARINES 1
/* actor_type_definitions maps each type to its behaviour: two types each are Elites and Marines */
#define MENDICANT_ACTOR_IS_ELITE(type) ((type) == 0 || (type) == 5)
#define MENDICANT_ACTOR_IS_MARINE(type) ((type) == 6 || (type) == 7)

/* each actor's part in its unit's drill, which sets how it moves and fires */
enum { ROLE_NONE, ROLE_SUPPORT, ROLE_FLANKER, ROLE_ASSAULT, ROLE_HIDDEN, ROLE_BAIT,
	ROLE_SECURITY, ROLE_OVERWATCH, ROLE_SHIFT_FIRE, ROLE_GRENADIER, ROLE_RUNNER, ROLE_MARKSMAN, ROLES };
/* the cover the next position searches prefer (mendicant_cover_want) */
enum { MENDICANT_COVER_NONE = 0, MENDICANT_COVER_FIGHTING, MENDICANT_COVER_CONCEALED };
/* a unit's elements */
enum { ELEMENT_BASE, ELEMENT_MANEUVER, ELEMENT_RESERVE, ELEMENTS };

struct mendicant_member { long actor_index; struct actor_datum *actor; };
/* a step's reference: the target (or a stand-in), and the axis from it to the unit's middle */
struct mendicant_geometry { real_point3d const *target; real axis_x, axis_y; };

/* ---- M2 command (mendicant_command.c): what the other modules use */
extern long order_actor[MENDICANT_MAX_ACTORS];
extern short order_position[MENDICANT_MAX_ACTORS];
extern char actor_role[MENDICANT_MAX_ACTORS];
extern unsigned char actor_grenade_due[MENDICANT_MAX_ACTORS];
extern real actor_leash[MENDICANT_MAX_ACTORS][4];
extern real actor_face[MENDICANT_MAX_ACTORS][3];
extern real unit_assault_range[MENDICANT_MAX_ACTORS];
extern long commanded_time[MENDICANT_MAX_ACTORS];
extern long leash_corrections;
extern boolean bridge_pending;
int mendicant_team_mode(short team);      /* MODE_GAME / MODE_DRILL / MODE_MODEL */
int mendicant_side_of(short team);
int mendicant_unit_side(struct actor_datum *actor);   /* MENDICANT_SIDE_COVENANT / _MARINES */
boolean mendicant_actor_can_drill(struct actor_datum *actor);
void mendicant_order(long actor_index, short position); /* releases a go order by drill or follow first */
void mendicant_set_role(long actor_index, int role);
unsigned long mendicant_drill_rand(void);
boolean mendicant_bridge_waiting(void);  /* a request is out and not answered */
/* candidates, team features, then mendicant_bridge_submit (individuals use it too) */
void mendicant_bridge_request(long key, struct mendicant_member *members, int count,
	real_point3d const *target, real axis_x, real axis_y, boolean commanded, long time);
/* (mendicant_units.c) */
extern real position_reach_centre[3], position_reach; /* a Covenant defender's post, around a step's orders */
void mendicant_centre_of(struct mendicant_member *members, int count, int element, real_point3d *centre);

/* the drill's sides (doctrine_table.h has the same enum; the command layer
includes that, so it sees these as MENDICANT_SIDE_* only) */
#ifndef MENDICANT_COMMAND_PRIVATE
enum { SIDE_COVENANT = MENDICANT_SIDE_COVENANT, SIDE_MARINES = MENDICANT_SIDE_MARINES };
#endif
#define ACTOR_IS_ELITE(type) MENDICANT_ACTOR_IS_ELITE(type)
#define ACTOR_IS_MARINE(type) MENDICANT_ACTOR_IS_MARINE(type)
#define ACTOR_IS_GRUNT(type) ((type) == 2 || (type) == 14)
#define ACTOR_IS_JACKAL(type) ((type) == 1)

/* ---- what M2 calls in M3 movement (mendicant_move.c) */
void mendicant_maneuver_check_map(void);
void mendicant_follow_chief(long time);
void mendicant_go_release_drill(long actor_index);
void mendicant_cover_want(int cover, real_point3d const *threat); /* a NULL threat keeps the last one */
struct firing_position_definition *mendicant_encounter_firing_positions(long encounter_index, int *count);
boolean mendicant_firing_position_owned(long encounter_index, short position);
boolean mendicant_position_usable(long encounter_index, short position, struct firing_position_definition const *entry);
boolean mendicant_drill_move(struct mendicant_member *member, struct mendicant_geometry const *geometry, int side,
	real angle_low, real angle_high, real range_low, real range_high, long *used);
void mendicant_drill_hold(struct mendicant_member *member, long *used);
boolean mendicant_assign_position(struct mendicant_member *member, short position);
boolean mendicant_go_fallback(struct mendicant_member *member, real_point3d const *point, real tolerance);
int mendicant_maneuver_slot(long encounter_index, real_point3d const *point);
int mendicant_maneuver_near(real_point3d const *point, real radius);
real_point3d const *mendicant_maneuver_pool_point(int candidate);
boolean mendicant_within_reach(real_point3d const *point);
int mendicant_cover_score(real_point3d const *position, real_point3d const *threat);
real mendicant_cover_bonus(real_point3d const *position);

/* ---- what M2 calls in M4 combat (mendicant_combat.c) */
void mendicant_individuals_update(long time);
void mendicant_apply_stance(long actor_index, int stance); /* 1..STANCES-1: its role; else no role */
int mendicant_weapon_class(struct unit_datum *unit);
boolean mendicant_carries_plasma_pistol(struct unit_datum *unit);

/* ---- M3 movement (mendicant_move.c): the rest of its services */
enum { GO_STATUS_NONE = 0, GO_STATUS_MOVING, GO_STATUS_ARRIVED, GO_STATUS_YIELDING,
	GO_STATUS_NO_GROUND = -1, GO_STATUS_NO_PATH = -2, GO_STATUS_GONE = -3 };
int actor_go_to(long actor_index, real_point3d const *point, real tolerance, int urgency);
int actor_go_status(long actor_index);
void actor_go_cancel(long actor_index);
void mendicant_follow_cancel(long actor_index);
int mendicant_line_blocked(real_point3d const *from, real_point3d const *to); /* -1 no BSP, 0 clear, 1 blocked */
int mendicant_drill_position(struct actor_datum *actor, struct mendicant_geometry const *geometry, int side,
	real angle_low, real angle_high, real range_low, real range_high, long *used);

/* ---- M4 combat: the rest of its services */
void mendicant_vehicles_reset(void);
boolean mendicant_carries_assault_rifle(struct unit_datum *unit);

#endif
