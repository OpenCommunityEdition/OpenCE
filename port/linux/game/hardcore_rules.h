/* Hardcore match rules that are not fields of the Xbox variant.
The variant the network already sends is unchanged. These bytes ride a
data message old builds ignore, and they apply only while that variant's
anniversary bit is set. */
#ifndef __HARDCORE_RULES_H
#define __HARDCORE_RULES_H
#pragma once

#include "cseries.h"
#include "math/real_math.h"

struct hardcore_rules
{
	unsigned char flag_pickup;      /* 0 manual, 1 automatic */
	unsigned char ball_pickup;
	unsigned char hill_order;       /* 0 ordered, 1 random after the first */
	unsigned char hill_move_s;      /* seconds the hill stays; 60 is the old period */
	unsigned char fireteams;
	unsigned char infinite_ammo;    /* 0 no, 1 bottomless clip, 2 magazines */
	unsigned char kill_restores;
	unsigned char skip_draw;
	unsigned char movement_sounds;  /* 1 on */
	unsigned char spawns_visible;  /* training mode, with random_areas */
	unsigned char random_pct;       /* 1 the respawn percent. this byte stays put */
	unsigned char omit_first;       /* 0 no, 1 yes, 2 power items */
	unsigned char item_spawn_s;     /* 0 off, else seconds for every item */
	unsigned char camo;             /* 0 xbox, 1 mcc, 2 custom */
	unsigned char team_scores;      /* 0 off, 1 the counters. this byte stays put */
	unsigned char outlines;
	unsigned char os_visible;
	unsigned char os_type;          /* 0 constant, 1 fading */
	unsigned char reserved_d;       /* unused, so the byte after it stays put */
	unsigned char reserved_a;       /* unused, so the bytes after it stay put */
	unsigned char friction;         /* 0 the tag, then none, half, full, strong */
	unsigned char adhesion;
	unsigned char aim_delay;        /* ticks */
	unsigned char trig_delay;
	unsigned char pistol_spread;    /* 0 the tag, 1 tight, 2 wide, 3 double */
	unsigned char shotgun_spread;
	unsigned char reserved_b;       /* unused, so the byte after it stays put */
	unsigned char random_areas;     /* training mode, with spawns_visible */
};

typedef char hardcore_rules_size_assert[sizeof(struct hardcore_rules) == 28 ? 1 : -1];

void hardcore_rules_defaults(struct hardcore_rules *rules);
struct hardcore_rules *hardcore_rules_editing(void);
void hardcore_rules_reset_editing(void);
void hardcore_rules_commit(void);
/* A classic mode was chosen. The match that was started stays until then. */
void hardcore_rules_clear_session(void);
struct game_variant;
/* The host's match is this variant. A classic one ends the mode. */
void hardcore_rules_host_variant(struct game_variant *variant);

boolean hardcore_rules_active(void);
boolean hardcore_rules_random_areas(void);
const struct hardcore_rules *hardcore_rules_live(void);

void hardcore_rules_begin_map(void);
void hardcore_rules_tick(void);
void hardcore_rules_render(void);

boolean hardcore_rules_manual_pickup(long weapon_index);
boolean hardcore_rules_hill_ordered(void);
long hardcore_rules_hill_move_ticks(void);
real hardcore_rules_spawn_rating(long player_index, real_point3d const *position, real rating);
boolean hardcore_rules_shot_blocked(long weapon_index);
real hardcore_rules_friction(real tag_value);
real hardcore_rules_adhesion(real tag_value);
real hardcore_rules_spread(long weapon_definition_index, real error);
void hardcore_rules_skip_arm(long unit_index);
boolean hardcore_rules_skip_consume(long unit_index);
void hardcore_rules_on_kill(long killing_player_index, long dead_player_index);
boolean hardcore_rules_omit_spawn(long game_time, long respawn_seconds);
/* Fading is the stock drain. Constant holds whatever is left above full. */
boolean hardcore_rules_overshield_fades(void);
long hardcore_rules_item_period(long period_ticks, long authored_seconds);

boolean hardcore_rules_write_message(void *buffer, long buffer_size);
boolean hardcore_rules_take_message(void const *message, long size);
/* A settings update. A classic variant ends a match this machine joined. */
void hardcore_rules_observe_match(boolean hardcore);

#endif
