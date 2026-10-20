/* Evolved rules that are not fields of the Xbox variant. Classic games
never set the anniversary bit, so none of this runs for them. The bytes ride
a data message an older build ignores. */
#include "cseries.h"
#include "math/real_math.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/game_globals.h"
#include "game/cheats.h"
#include "game/players.h"
#include "game/players_runtime.h"
#include "scenario/scenario_definitions.h"
#include "game/performance_variant.h"
#include "interface/hud.h"
#include "interface/hud_definitions.h"
#include "interface/hud_messaging.h"
#include "interface/ui_widget.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "objects/objects.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/common/rasterizer_common.h"
#include "render/render.h"
#include "tag_files/tag_files.h"
#include "text/draw_string.h"
#include "units/units.h"
#include "halo_port_limits.h"
#include "performance_options.h"
#include "evolved_rules.h"
#include "evolved_timer.h"

#include <stdio.h>
#include <string.h>

#define EVOLVED_RULES_VERSION 3
#define EVOLVED_RULES_MESSAGE_SIZE 35
#define EVOLVED_SKIP_SLOTS HALO_PORT_MAXIMUM_OBJECTS_PER_MAP

#ifdef HALO_PORT_MAXIMUM_NETWORK_PLAYERS
#define EVOLVED_PLAYER_SLOTS HALO_PORT_MAXIMUM_NETWORK_PLAYERS
#else
#define EVOLVED_PLAYER_SLOTS 16
#endif

static struct evolved_rules evolved_edit;
static struct evolved_rules evolved_live;
static unsigned char evolved_skip_armed[EVOLVED_SKIP_SLOTS];
static long evolved_shot_press[EVOLVED_SKIP_SLOTS];
static long evolved_shot_seen[EVOLVED_SKIP_SLOTS];
static int evolved_random_percent;
static int evolved_cheats_written;
/* set when this match is started, cleared when a classic mode is chosen.
   the lobby's live variant is still the previous map, so the bit alone
   is not on yet when the next map loads. */
static int evolved_session;

static int evolved_anniversary(void)
{
	struct game_variant const *variant = game_engine_get_variant();

	return variant && TEST_FLAG(variant->universal_variant.flags, _game_variant_anniversary_rules_bit);
}

void evolved_rules_defaults(struct evolved_rules *rules)
{
	csmemset(rules, 0, sizeof(*rules));
	rules->hill_move_s = 60;
	rules->movement_sounds = 1;
	/* training mode: the spawn pads, and the darker volumes where a
	   teammate can respawn. the two bytes stay one switch. */
	rules->spawns_visible = 1;
	rules->random_areas = 1;
	rules->random_pct = 1;
}

/* the menu's one switch. an older sheet may have set only one of them.
   the percent is that switch too, and its byte stays where it is. */
static void evolved_training_pair(struct evolved_rules *rules)
{
	unsigned char on = (rules->spawns_visible || rules->random_areas) ? 1 : 0;

	rules->spawns_visible = on;
	rules->random_areas = on;
	rules->random_pct = on;
}

struct evolved_rules *evolved_rules_editing(void)
{
	return &evolved_edit;
}

void evolved_rules_reset_editing(void)
{
	evolved_rules_defaults(&evolved_edit);
}

void evolved_rules_commit(void)
{
	evolved_training_pair(&evolved_edit);
	evolved_live = evolved_edit;
	evolved_session = 1;
}

void evolved_rules_clear_session(void)
{
	evolved_session = 0;
}

boolean evolved_rules_active(void)
{
	return (evolved_session || evolved_anniversary()) ? TRUE : FALSE;
}

boolean evolved_rules_random_areas(void)
{
	return evolved_rules_active() && evolved_live.random_areas ? TRUE : FALSE;
}

const struct evolved_rules *evolved_rules_live(void)
{
	return &evolved_live;
}

static void evolved_apply_camo(void)
{
	int amount;

	if (!global_rasterizer_data || evolved_live.camo == 0)
		return;
	/* MCC hardcore. Custom uses the same numbers. Xbox leaves the tag. */
	amount = 9;
	global_rasterizer_data->active_camouflage_refraction_amount = (real)amount;
	global_rasterizer_data->active_camouflage_distance_falloff = (real)amount;
	global_rasterizer_data->active_camouflage_tint_color.red = 1.0f;
	global_rasterizer_data->active_camouflage_tint_color.green = 1.0f;
	global_rasterizer_data->active_camouflage_tint_color.blue = 1.0f;
	global_rasterizer_data->active_camouflage_hyper_stealth_refraction_amount = (real)amount;
	global_rasterizer_data->active_camouflage_hyper_stealth_distance_falloff = (real)amount;
	global_rasterizer_data->active_camouflage_hyper_stealth_tint_color.red = 1.0f;
	global_rasterizer_data->active_camouflage_hyper_stealth_tint_color.green = 1.0f;
	global_rasterizer_data->active_camouflage_hyper_stealth_tint_color.blue = 1.0f;
}

static void evolved_apply_presentation(void)
{
	unsigned long flags = performance_options_get_flags();

	if (evolved_live.spawns_visible)
		flags |= _performance_option_spawn_markers;
	if (!evolved_live.movement_sounds)
		flags |= _performance_option_silent_movement;
	performance_options_apply_host_flags(flags);
	evolved_apply_camo();
}

void evolved_rules_begin_map(void)
{
	csmemset(evolved_skip_armed, 0, sizeof(evolved_skip_armed));
	csmemset(evolved_shot_press, 0, sizeof(evolved_shot_press));
	csmemset(evolved_shot_seen, 0, sizeof(evolved_shot_seen));
	evolved_random_percent = -1;
	if (evolved_cheats_written)
	{
		cheat.bottomless_clip = FALSE;
		cheat.infinite_ammo = FALSE;
		evolved_cheats_written = 0;
	}
	/* port: the match was started from this mode. stamp the bit the other
	   checks read, including when the loaded copy never received it. */
	if (evolved_session)
	{
		struct game_variant *variant = game_engine_get_variant();

		if (variant)
			SET_FLAG(variant->universal_variant.flags, _game_variant_anniversary_rules_bit, TRUE);
	}
	if (!evolved_rules_active())
		return;
	evolved_apply_presentation();
}

void evolved_rules_tick(void)
{
	if (!evolved_rules_active())
	{
		if (evolved_cheats_written)
		{
			cheat.bottomless_clip = FALSE;
			cheat.infinite_ammo = FALSE;
			evolved_cheats_written = 0;
		}
		return;
	}
	/* port: a settings update replaces the practice flags with the variant's,
	   which does not carry the spawn markers. put them back each tick. */
	evolved_apply_presentation();
	cheat.bottomless_clip = evolved_live.infinite_ammo == 1;
	cheat.infinite_ammo = evolved_live.infinite_ammo == 2;
	evolved_cheats_written = 1;
}

boolean evolved_rules_manual_pickup(long weapon_index)
{
	struct weapon_datum *weapon;
	struct weapon_definition *definition;

	/* The flag and the ball are weapons that do not take an inventory slot.
	A sniper is readied too, and still picks up on its own. */
	if (!evolved_rules_active() || !game_engine)
		return FALSE;
	weapon = weapon_try_and_get(weapon_index);
	if (!weapon)
		return FALSE;
	definition = weapon_definition_get(weapon->definition_index);
	if (!definition || !TEST_FLAG(definition->weapon.flags, _weapon_doesnt_count_toward_maximum_bit))
		return FALSE;
	if (game_engine->type == game_engine_ctf)
		return evolved_live.flag_pickup == 0;
	if (game_engine->type == game_engine_oddball)
		return evolved_live.ball_pickup == 0;
	return FALSE;
}

boolean evolved_rules_hill_ordered(void)
{
	return evolved_rules_active() && evolved_live.hill_order == 0;
}

long evolved_rules_hill_move_ticks(void)
{
	int seconds = evolved_live.hill_move_s;

	if (seconds < 5)
		seconds = 60;
	return (long)seconds * TICKS_PER_SECOND;
}

real evolved_rules_spawn_rating(long player_index, real_point3d const *position, real rating)
{
	struct player_datum *player;
	struct game_variant const *variant;
	long mates[EVOLVED_PLAYER_SLOTS];
	short count, index, mine, partner;

	if (!evolved_rules_active() || !evolved_live.fireteams || rating <= 0.0f || !position)
		return rating;
	variant = game_engine_get_variant();
	player = player_try_and_get(player_index);
	if (!variant || !variant->universal_variant.teams || !player || player->team_index < 0)
		return rating;
	count = 0;
	for (index = 0; index < EVOLVED_PLAYER_SLOTS && count < EVOLVED_PLAYER_SLOTS; index++)
	{
		struct player_datum *other = player_try_and_get(index);

		if (other && other->team_index == player->team_index)
			mates[count++] = index;
	}
	if (count <= 2)
		return rating;
	mine = 0;
	for (index = 0; index < count; index++)
		if (mates[index] == player_index)
			mine = index;
	partner = (mine ^ 1) < count ? (short)mates[mine ^ 1] : (short)player_index;
	for (index = 0; index < count; index++)
	{
		struct player_datum *other;
		struct object_datum *unit;
		real dx, dy, dz;

		if (mates[index] == player_index || mates[index] == partner)
			continue;
		other = player_try_and_get(mates[index]);
		if (!other || other->unit_index == NONE)
			continue;
		unit = object_try_and_get(other->unit_index);
		if (!unit)
			continue;
		dx = unit->object.position.x - position->x;
		dy = unit->object.position.y - position->y;
		dz = unit->object.position.z - position->z;
		if (dx * dx + dy * dy + dz * dz < 36.0f)
			return rating * 0.15f;
	}
	return rating;
}

boolean evolved_rules_shot_blocked(long weapon_index)
{
	int delay;
	long now;
	unsigned slot;

	delay = (int)evolved_live.aim_delay + (int)evolved_live.trig_delay;
	if (!evolved_rules_active() || delay <= 0 || weapon_index == NONE)
		return FALSE;
	now = game_time_get();
	slot = (unsigned)DATUM_INDEX_TO_ABSOLUTE_INDEX(weapon_index);
	if (slot >= EVOLVED_SKIP_SLOTS)
		return FALSE;
	if (evolved_shot_seen[slot] != now - 1)
		evolved_shot_press[slot] = now;
	evolved_shot_seen[slot] = now;
	return now - evolved_shot_press[slot] < delay;
}

static real evolved_scale(unsigned char which, real tag_value, real const *table, int count)
{
	if (!evolved_rules_active() || which == 0 || which >= count)
		return tag_value;
	return tag_value * table[which];
}

real evolved_rules_friction(real tag_value)
{
	static real const table[] = { 1.0f, 0.0f, 0.5f, 1.0f, 1.5f };

	return evolved_scale(evolved_live.friction, tag_value, table, 5);
}

real evolved_rules_adhesion(real tag_value)
{
	static real const table[] = { 1.0f, 0.0f, 0.5f, 1.0f, 1.5f };

	return evolved_scale(evolved_live.adhesion, tag_value, table, 5);
}

real evolved_rules_spread(long weapon_definition_index, real error)
{
	char const *name;
	unsigned char which;
	static real const table[] = { 1.0f, 0.5f, 1.5f, 2.0f };

	if (!evolved_rules_active() || weapon_definition_index == NONE)
		return error;
	name = tag_get_name(weapon_definition_index);
	if (!name)
		return error;
	which = 0;
	if (strstr(name, "pistol"))
		which = evolved_live.pistol_spread;
	else if (strstr(name, "shotgun"))
		which = evolved_live.shotgun_spread;
	return evolved_scale(which, error, table, 4);
}

void evolved_rules_skip_arm(long unit_index)
{
	unsigned slot = (unsigned)DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_index);

	if (unit_index == NONE || slot >= EVOLVED_SKIP_SLOTS)
		return;
	evolved_skip_armed[slot] = 1;
}

boolean evolved_rules_skip_consume(long unit_index)
{
	unsigned slot = (unsigned)DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_index);

	if (unit_index == NONE || slot >= EVOLVED_SKIP_SLOTS || !evolved_skip_armed[slot])
		return FALSE;
	evolved_skip_armed[slot] = 0;
	return evolved_rules_active() && evolved_live.skip_draw;
}

void evolved_rules_on_kill(long killing_player_index, long dead_player_index)
{
	struct player_datum *killer;
	struct unit_datum *killer_unit;

	if (!evolved_rules_active() || !evolved_live.kill_restores ||
		killing_player_index == NONE || killing_player_index == dead_player_index)
		return;
	killer = player_try_and_get(killing_player_index);
	killer_unit = killer && killer->unit_index != NONE ? unit_try_and_get(killer->unit_index) : NULL;
	/* An overshield is the same number past full. Restoring would delete it. */
	if (killer_unit && killer_unit->object.shield_vitality <= 1.0f)
	{
		killer_unit->object.shield_vitality = 1.0f;
		killer_unit->object.body_vitality = 1.0f;
	}
}

boolean evolved_rules_omit_spawn(long now, long respawn_seconds)
{
	if (!evolved_rules_active() || now != 0)
		return FALSE;
	if (evolved_live.omit_first == 1)
		return TRUE;
	return evolved_live.omit_first == 2 && respawn_seconds >= 30;
}

long evolved_rules_item_period(long period_ticks, long authored_seconds)
{
	(void)authored_seconds;
	if (!evolved_rules_active() || evolved_live.item_spawn_s == 0)
		return period_ticks;
	return (long)evolved_live.item_spawn_s * TICKS_PER_SECOND;
}

static void evolved_random_update(void)
{
	struct player_datum *local;
	struct unit_datum *body;
	struct game_variant const *variant;
	real near_weight, total_weight;
	short index, count;
	long player_index;

	evolved_random_percent = -1;
	variant = game_engine_get_variant();
	if (!evolved_live.random_areas || !variant || !variant->universal_variant.teams ||
		!players_globals || players_globals->local_player_count <= 0)
		return;
	player_index = players_globals->local_players[0];
	local = player_try_and_get(player_index);
	if (!local || local->unit_index == NONE)
		return;
	body = unit_try_and_get(local->unit_index);
	if (!body)
		return;
	near_weight = 0.0f;
	total_weight = 0.0f;
	count = player_get_starting_location_count();
	for (index = 0; index < count; index++)
	{
		struct player_starting_location const *start = player_get_starting_location(index);
		real rating, dx, dy, dz;

		if (!start)
			continue;
		rating = game_engine_get_starting_location_rating(player_index, start);
		if (rating <= 0.0f)
			continue;
		rating = rating * rating;
		total_weight += rating;
		dx = start->position.x - body->object.position.x;
		dy = start->position.y - body->object.position.y;
		dz = start->position.z - body->object.position.z;
		if (dx * dx + dy * dy + dz * dz < 36.0f)
			near_weight += rating;
	}
	if (total_weight <= 0.0f)
		return;
	evolved_random_percent = (int)(100.0f * (1.0f - near_weight / total_weight) + 0.5f);
	if (evolved_random_percent < 0)
		evolved_random_percent = 0;
	if (evolved_random_percent > 100)
		evolved_random_percent = 100;
}

static void evolved_marker_line(real_point3d const *position, real_argb_color const *color)
{
	real_point3d base, top;

	base = *position;
	top = base;
	top.z += 0.8f;
	rasterizer_debug_line(&base, &top, color);
}

static void evolved_markers(void)
{
	struct player_datum *local;
	real_argb_color const outline = { 1.0f, 0.2f, 0.85f, 1.0f };
	real_argb_color const shield = { 1.0f, 1.0f, 0.85f, 0.2f };
	long index;

	if (ui_widgets_active() || !players_globals || players_globals->local_player_count <= 0)
		return;
	if (!evolved_live.outlines && !evolved_live.os_visible)
		return;
	local = player_try_and_get(players_globals->local_players[0]);
	for (index = 0; index < EVOLVED_PLAYER_SLOTS; index++)
	{
		struct player_datum *player = player_try_and_get(index);
		struct unit_datum *unit;

		if (!player || player->unit_index == NONE)
			continue;
		unit = unit_try_and_get(player->unit_index);
		if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		if (evolved_live.outlines && local && player != local &&
			local->team_index >= 0 && player->team_index == local->team_index)
			evolved_marker_line(&unit->object.position, &outline);
		if (evolved_live.os_visible && unit->object.shield_vitality > 1.0f)
			evolved_marker_line(&unit->object.position, &shield);
	}
}

void evolved_rules_render(void)
{
	long font;
	real_argb_color color;
	rectangle2d bounds;
	char text[48];
	wchar_t wide[48];
	long index;

	if (!evolved_rules_active() || !game_in_progress())
		return;
	evolved_markers();
	evolved_random_update();
	if (evolved_random_percent < 0 || ui_widgets_active())
		return;
	/* the clock's font and top, so the percent is on its line */
	font = hud_globals ? hud_globals->messaging.single_player_font.index : NONE;
	if (font == NONE)
		font = hud_get_font_index();
	if (font == NONE)
		return;
	snprintf(text, sizeof(text), "RANDOM %d%%", evolved_random_percent);
	/* window_bounds is the title-safe frame, and that frame ends 36 above
	the screen. The clock is on the screen, so this row is too. */
	bounds = render.camera.window_bounds;
	bounds.x0 = (short)(bounds.x0 + 16);
	bounds.y0 = evolved_timer_top();
	bounds.y1 = 480;
	if (bounds.x0 >= bounds.x1 || bounds.y0 >= bounds.y1)
		return;
	for (index = 0; text[index] && index < 47; index++)
		wide[index] = (wchar_t)text[index];
	wide[index] = 0;
	hud_get_text_color(&color);
	draw_string_set_draw_mode(font, NONE, 0, 0, &color);
	rasterizer_text_set_shadow_color(0xC0000000);
	rasterizer_draw_unicode_string(&bounds, &bounds, NULL, 0, wide);
	rasterizer_text_set_shadow_color(0);
}

boolean evolved_rules_write_message(void *buffer, long buffer_size)
{
	unsigned char *bytes = buffer;
	unsigned long header;

	if (!buffer || buffer_size < EVOLVED_RULES_MESSAGE_SIZE)
		return FALSE;
	csmemset(buffer, 0, EVOLVED_RULES_MESSAGE_SIZE);
	/* Host order until the connection write swaps the header word. */
	header = ((unsigned long)EVOLVED_RULES_MESSAGE_SIZE << 4) | (2u << 2);
	bytes[0] = (unsigned char)header;
	bytes[1] = (unsigned char)(header >> 8);
	bytes[2] = 'C';
	bytes[3] = 'R';
	bytes[4] = 'U';
	bytes[5] = 'L';
	bytes[6] = EVOLVED_RULES_VERSION;
	csmemcpy(bytes + 7, &evolved_live, sizeof(evolved_live));
	return TRUE;
}

boolean evolved_rules_take_message(void const *message, long size)
{
	unsigned char const *bytes = message;

	if (!message || size != EVOLVED_RULES_MESSAGE_SIZE)
		return FALSE;
	if (bytes[2] != 'C' || bytes[3] != 'R' || bytes[4] != 'U' || bytes[5] != 'L' ||
		bytes[6] != EVOLVED_RULES_VERSION)
		return FALSE;
	if (bytes[0] != (unsigned char)(((unsigned)EVOLVED_RULES_MESSAGE_SIZE << 4) | (2u << 2)) ||
		bytes[1] != (unsigned char)((((unsigned)EVOLVED_RULES_MESSAGE_SIZE << 4) | (2u << 2)) >> 8))
		return FALSE;
	csmemcpy(&evolved_live, bytes + 7, sizeof(evolved_live));
	evolved_training_pair(&evolved_live);
	/* port: the loaded variant may not have the bit yet. the session is
	   what makes this match apply the sheet, as it does on the host. */
	evolved_session = 1;
	if (game_in_progress())
		evolved_apply_presentation();
	return TRUE;
}

/* port: a classic variant ends the match this machine joined. the host's
   session belongs to its menu and is not cleared from here. */
void evolved_rules_observe_match(boolean evolved)
{
	if (!evolved)
		evolved_session = 0;
}
