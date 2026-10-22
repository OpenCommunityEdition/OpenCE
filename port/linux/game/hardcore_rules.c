/* Hardcore rules that are not fields of the Xbox variant. Classic games
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
#include "hardcore_rules.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "hardcore_timer.h"

#include <stdio.h>
#include <string.h>

struct network_game *network_game_server_get_game(struct network_game_server *server);

#define HARDCORE_RULES_VERSION 3
#define HARDCORE_RULES_MESSAGE_SIZE 35
#define HARDCORE_SKIP_SLOTS HALO_PORT_MAXIMUM_OBJECTS_PER_MAP

#ifdef HALO_PORT_MAXIMUM_NETWORK_PLAYERS
#define HARDCORE_PLAYER_SLOTS HALO_PORT_MAXIMUM_NETWORK_PLAYERS
#else
#define HARDCORE_PLAYER_SLOTS 16
#endif

static struct hardcore_rules hardcore_edit;
static struct hardcore_rules hardcore_live;
static unsigned char hardcore_skip_armed[HARDCORE_SKIP_SLOTS];
static long hardcore_shot_press[HARDCORE_SKIP_SLOTS];
static long hardcore_shot_seen[HARDCORE_SKIP_SLOTS];
static long hardcore_shot_weapon[HARDCORE_SKIP_SLOTS];
static int hardcore_random_percent;
static int hardcore_cheats_written;
/* set when this match is started. cleared when a classic mode is chosen,
   and when the host's match variant is a classic one. the lobby's live
   variant is still the previous map, so the bit alone is not on yet when
   the next map loads. */
static int hardcore_session;

static int hardcore_anniversary(void)
{
	struct game_variant const *variant = game_engine_get_variant();

	return variant && TEST_FLAG(variant->universal_variant.flags, _game_variant_anniversary_rules_bit);
}

void hardcore_rules_defaults(struct hardcore_rules *rules)
{
	csmemset(rules, 0, sizeof(*rules));
	rules->hill_move_s = 60;
	rules->movement_sounds = 1;
	/* training mode stays off. the pads and the volumes wait for that
	   switch. comp 2v2 turns the percent on by itself. */
	rules->team_scores = 1;
	/* fading is the stock drain. constant holds the extra shield. */
	rules->os_type = 1;
}

/* the menu's one switch for the pads and the volumes. an older sheet may
   have set only one of them. turning that switch on shows the percent.
   the percent can stay on without the pads, and its byte stays put. */
static void hardcore_training_pair(struct hardcore_rules *rules)
{
	unsigned char on = (rules->spawns_visible || rules->random_areas) ? 1 : 0;

	rules->spawns_visible = on;
	rules->random_areas = on;
	if (on)
		rules->random_pct = 1;
}

struct hardcore_rules *hardcore_rules_editing(void)
{
	return &hardcore_edit;
}

void hardcore_rules_reset_editing(void)
{
	hardcore_rules_defaults(&hardcore_edit);
}

void hardcore_rules_commit(void)
{
	hardcore_training_pair(&hardcore_edit);
	hardcore_live = hardcore_edit;
	hardcore_session = 1;
}

void hardcore_rules_clear_session(void)
{
	hardcore_session = 0;
}

/* port: the lobby's gametype is a classic one. the sheet from the mode
   stops with it, including a session the menu had not cleared. */
void hardcore_rules_host_variant(struct game_variant *variant)
{
	struct game_variant *live;

	if (variant && TEST_FLAG(variant->universal_variant.flags, _game_variant_anniversary_rules_bit))
		return;
	hardcore_session = 0;
	live = game_engine_get_variant();
	if (live)
		SET_FLAG(live->universal_variant.flags, _game_variant_anniversary_rules_bit, FALSE);
}

boolean hardcore_rules_active(void)
{
	/* port: the live multiplayer variant's bit. a leftover session must
	   not change a classic game, co-op, or the campaign. */
	return game_engine_running() && hardcore_anniversary() ? TRUE : FALSE;
}

boolean hardcore_rules_random_areas(void)
{
	return hardcore_rules_active() && hardcore_live.random_areas ? TRUE : FALSE;
}

const struct hardcore_rules *hardcore_rules_live(void)
{
	return &hardcore_live;
}

static void hardcore_camo_write(int refraction, int falloff, int red, int green, int blue)
{
	real tint_red = (real)red * (1.0f / 255.0f);
	real tint_green = (real)green * (1.0f / 255.0f);
	real tint_blue = (real)blue * (1.0f / 255.0f);

	/* both ends of the fade, or a strong cloak slides back to the tag */
	global_rasterizer_data->active_camouflage_refraction_amount = (real)refraction;
	global_rasterizer_data->active_camouflage_distance_falloff = (real)falloff;
	global_rasterizer_data->active_camouflage_tint_color.red = tint_red;
	global_rasterizer_data->active_camouflage_tint_color.green = tint_green;
	global_rasterizer_data->active_camouflage_tint_color.blue = tint_blue;
	global_rasterizer_data->active_camouflage_hyper_stealth_refraction_amount = (real)refraction;
	global_rasterizer_data->active_camouflage_hyper_stealth_distance_falloff = (real)falloff;
	global_rasterizer_data->active_camouflage_hyper_stealth_tint_color.red = tint_red;
	global_rasterizer_data->active_camouflage_hyper_stealth_tint_color.green = tint_green;
	global_rasterizer_data->active_camouflage_hyper_stealth_tint_color.blue = tint_blue;
}

static void hardcore_apply_camo(void)
{
	if (!global_rasterizer_data || hardcore_live.camo == 0)
		return;
	/* mcc hardcore is 9, 9 and white. custom is the preset 11, 22 and
	   the pale tint. xbox leaves the tag, which is that pale look on a
	   stock map and something else on a map that changed it. */
	if (hardcore_live.camo == 1)
		hardcore_camo_write(9, 9, 255, 255, 255);
	else
		hardcore_camo_write(11, 22, 215, 214, 233);
}

static void hardcore_apply_presentation(void)
{
	unsigned long flags = performance_options_get_flags();

	/* The sheet is the authority. A later match that turns these off has to
	clear the bits the previous one set, on this machine and on a client. */
	if (hardcore_live.spawns_visible)
		flags |= _performance_option_spawn_markers;
	else
		flags &= ~_performance_option_spawn_markers;
	if (!hardcore_live.movement_sounds)
		flags |= _performance_option_silent_movement;
	else
		flags &= ~_performance_option_silent_movement;
	performance_options_apply_host_flags(flags);
	hardcore_apply_camo();
}

/* port: the menu's map list is built again after a match. defined with
   the menus (menu_functions.c). */
void hardcore_list_match_started(void);

void hardcore_rules_begin_map(void)
{
	hardcore_list_match_started();
	csmemset(hardcore_skip_armed, 0, sizeof(hardcore_skip_armed));
	csmemset(hardcore_shot_press, 0, sizeof(hardcore_shot_press));
	csmemset(hardcore_shot_seen, 0, sizeof(hardcore_shot_seen));
	csmemset(hardcore_shot_weapon, 0xFF, sizeof(hardcore_shot_weapon));
	hardcore_random_percent = -1;
	if (hardcore_cheats_written)
	{
		cheat.bottomless_clip = FALSE;
		cheat.infinite_ammo = FALSE;
		hardcore_cheats_written = 0;
	}
	/* port: the menu, co-op and the campaign have no multiplayer engine.
	   the last match's bit stays on that variant and would change their
	   shots. a multiplayer map has already copied its own variant. */
	if (!game_engine_running())
	{
		struct game_variant *stale = game_engine_get_variant();

		hardcore_session = 0;
		if (stale)
			SET_FLAG(stale->universal_variant.flags, _game_variant_anniversary_rules_bit, FALSE);
		return;
	}
	/* port: the match was started from this mode. stamp the bit the other
	   checks read, including when the loaded copy never received it.
	   a classic game this machine is hosting does not keep that session. */
	if (hardcore_session)
	{
		struct game_variant *variant = game_engine_get_variant();
		struct network_game_server *server = global_network_game_server_get();

		if (server)
		{
			struct network_game *hosted = network_game_server_get_game(server);

			hardcore_rules_host_variant(hosted ? &hosted->variant : NULL);
		}
		/* port: a client takes the variant it was sent. a leftover session
		   must not mark a classic game. */
		else if (!hardcore_anniversary())
			hardcore_session = 0;
		if (hardcore_session && variant)
			SET_FLAG(variant->universal_variant.flags, _game_variant_anniversary_rules_bit, TRUE);
	}
	if (!hardcore_rules_active())
		return;
	hardcore_apply_presentation();
}

void hardcore_rules_tick(void)
{
	if (!hardcore_rules_active())
	{
		if (hardcore_cheats_written)
		{
			cheat.bottomless_clip = FALSE;
			cheat.infinite_ammo = FALSE;
			hardcore_cheats_written = 0;
		}
		return;
	}
	/* port: a settings update replaces the practice flags with the variant's,
	   which does not carry the spawn markers. put them back each tick. */
	hardcore_apply_presentation();
	cheat.bottomless_clip = hardcore_live.infinite_ammo == 1;
	cheat.infinite_ammo = hardcore_live.infinite_ammo == 2;
	hardcore_cheats_written = 1;
}

boolean hardcore_rules_manual_pickup(long weapon_index)
{
	struct weapon_datum *weapon;
	struct weapon_definition *definition;

	/* The flag and the ball are weapons that do not take an inventory slot.
	A sniper is readied too, and still picks up on its own. */
	if (!hardcore_rules_active() || !game_engine)
		return FALSE;
	weapon = weapon_try_and_get(weapon_index);
	if (!weapon)
		return FALSE;
	definition = weapon_definition_get(weapon->definition_index);
	if (!definition || !TEST_FLAG(definition->weapon.flags, _weapon_doesnt_count_toward_maximum_bit))
		return FALSE;
	if (game_engine->type == game_engine_ctf)
		return hardcore_live.flag_pickup == 0;
	if (game_engine->type == game_engine_oddball)
		return hardcore_live.ball_pickup == 0;
	return FALSE;
}

boolean hardcore_rules_hill_ordered(void)
{
	return hardcore_rules_active() && hardcore_live.hill_order == 0;
}

long hardcore_rules_hill_move_ticks(void)
{
	int seconds = hardcore_live.hill_move_s;

	if (seconds < 5)
		seconds = 60;
	return (long)seconds * TICKS_PER_SECOND;
}

real hardcore_rules_spawn_rating(long player_index, real_point3d const *position, real rating)
{
	struct player_datum *player;
	struct game_variant const *variant;
	long mates[HARDCORE_PLAYER_SLOTS];
	long self;
	short count, index, mine, partner;
	boolean found;

	if (!hardcore_rules_active() || !hardcore_live.fireteams || rating <= 0.0f || !position)
		return rating;
	variant = game_engine_get_variant();
	player = player_try_and_get(player_index);
	if (!variant || !variant->universal_variant.teams || !player || player->team_index < 0)
		return rating;
	count = 0;
	for (index = 0; index < HARDCORE_PLAYER_SLOTS && count < HARDCORE_PLAYER_SLOTS; index++)
	{
		struct player_datum *other = player_try_and_get(index);

		if (other && !other->quit_out_of_game && other->team_index == player->team_index)
			mates[count++] = index;
	}
	if (count <= 2)
		return rating;
	/* mates[] is a slot. player_index carries a salt, so the slot is the low word. */
	mine = 0;
	self = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
	found = FALSE;
	for (index = 0; index < count; index++)
		if (mates[index] == self)
		{
			mine = index;
			found = TRUE;
		}
	if (!found)
		return rating;
	partner = (mine ^ 1) < count ? (short)mates[mine ^ 1] : (short)self;
	for (index = 0; index < count; index++)
	{
		struct player_datum *other;
		struct object_datum *unit;
		real dx, dy, dz;

		if (mates[index] == self || mates[index] == partner)
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

boolean hardcore_rules_shot_blocked(long weapon_index)
{
	int delay;
	long now;
	unsigned slot;

	delay = (int)hardcore_live.aim_delay + (int)hardcore_live.trig_delay;
	if (!hardcore_rules_active() || delay <= 0 || weapon_index == NONE)
		return FALSE;
	now = game_time_get();
	slot = (unsigned)DATUM_INDEX_TO_ABSOLUTE_INDEX(weapon_index);
	if (slot >= HARDCORE_SKIP_SLOTS)
		return FALSE;
	/* the shot checks again the same tick the hold was noted. a gap is
	   the trigger coming up. the same tick must not start the wait over.
	   a new weapon in a freed slot is not the hold that slot had. */
	if (hardcore_shot_weapon[slot] != weapon_index ||
		(hardcore_shot_seen[slot] != now && hardcore_shot_seen[slot] != now - 1))
		hardcore_shot_press[slot] = now;
	hardcore_shot_weapon[slot] = weapon_index;
	hardcore_shot_seen[slot] = now;
	return now - hardcore_shot_press[slot] < delay;
}

static real hardcore_scale(unsigned char which, real tag_value, real const *table, int count)
{
	if (!hardcore_rules_active() || which == 0 || which >= count)
		return tag_value;
	return tag_value * table[which];
}

/* tag stays the map's number. the named stops are amounts, so full is 1
   and is not another way to leave the tag. strong is past that. */
static real hardcore_amount(unsigned char which, real tag_value)
{
	static real const table[] = { 0.0f, 0.0f, 0.5f, 1.0f, 1.5f };

	if (!hardcore_rules_active() || which == 0 || which >= 5)
		return tag_value;
	return table[which];
}

real hardcore_rules_friction(real tag_value)
{
	return hardcore_amount(hardcore_live.friction, tag_value);
}

real hardcore_rules_adhesion(real tag_value)
{
	return hardcore_amount(hardcore_live.adhesion, tag_value);
}

real hardcore_rules_spread(long weapon_definition_index, real error)
{
	char const *name;
	char const *leaf;
	unsigned char which;
	static real const table[] = { 1.0f, 0.5f, 1.5f, 2.0f };

	if (!hardcore_rules_active() || weapon_definition_index == NONE)
		return error;
	name = tag_get_name(weapon_definition_index);
	if (!name)
		return error;
	/* the leaf, so the plasma pistol is not the pistol */
	leaf = strrchr(name, '\\');
	if (!leaf)
		leaf = strrchr(name, '/');
	leaf = leaf ? leaf + 1 : name;
	which = 0;
	if (!strcmp(leaf, "pistol"))
		which = hardcore_live.pistol_spread;
	else if (!strcmp(leaf, "shotgun"))
		which = hardcore_live.shotgun_spread;
	return hardcore_scale(which, error, table, 4);
}

void hardcore_rules_skip_arm(long unit_index)
{
	unsigned slot = (unsigned)DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_index);

	if (unit_index == NONE || slot >= HARDCORE_SKIP_SLOTS)
		return;
	hardcore_skip_armed[slot] = 1;
}

boolean hardcore_rules_skip_consume(long unit_index)
{
	unsigned slot = (unsigned)DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_index);

	if (unit_index == NONE || slot >= HARDCORE_SKIP_SLOTS || !hardcore_skip_armed[slot])
		return FALSE;
	hardcore_skip_armed[slot] = 0;
	return hardcore_rules_active() && hardcore_live.skip_draw;
}

void hardcore_rules_on_kill(long killing_player_index, long dead_player_index)
{
	struct player_datum *killer;
	struct unit_datum *killer_unit;

	if (!hardcore_rules_active() || !hardcore_live.kill_restores ||
		killing_player_index == NONE || killing_player_index == dead_player_index)
		return;
	killer = player_try_and_get(killing_player_index);
	killer_unit = killer && killer->unit_index != NONE ? unit_try_and_get(killer->unit_index) : NULL;
	/* a dead killer stays dead. an overshield is the number past full,
	   so the shield is left there. the body still comes back. */
	if (!killer_unit || TEST_FLAG(killer_unit->object.damage_flags, _object_dead_bit))
		return;
	if (killer_unit->object.shield_vitality <= 1.0f)
		killer_unit->object.shield_vitality = 1.0f;
	killer_unit->object.body_vitality = 1.0f;
}

boolean hardcore_rules_overshield_fades(void)
{
	return !hardcore_rules_active() || hardcore_live.os_type != 0;
}

boolean hardcore_rules_omit_spawn(long now, long respawn_seconds)
{
	if (!hardcore_rules_active() || now != 0)
		return FALSE;
	if (hardcore_live.omit_first == 1)
		return TRUE;
	return hardcore_live.omit_first == 2 && respawn_seconds >= 30;
}

long hardcore_rules_item_period(long period_ticks, long authored_seconds)
{
	(void)authored_seconds;
	if (!hardcore_rules_active() || hardcore_live.item_spawn_s == 0)
		return period_ticks;
	return (long)hardcore_live.item_spawn_s * TICKS_PER_SECOND;
}

/* slot 0 can be empty while another local player is on this machine */
static long hardcore_local_player_index(void)
{
	short slot;

	if (!players_globals)
		return NONE;
	slot = local_player_get_next(NONE);
	if (slot == NONE)
		return NONE;
	return local_player_get_player_index(slot);
}

static void hardcore_random_update(void)
{
	struct player_datum *local;
	struct unit_datum *body;
	real near_weight, total_weight;
	short index, count;
	long player_index;

	hardcore_random_percent = -1;
	/* the percent is its own byte. training mode is the pads and the
	   volumes. a free for all still has a respawn weight. */
	if (!hardcore_live.random_pct || !players_globals ||
		players_globals->local_player_count <= 0)
		return;
	player_index = hardcore_local_player_index();
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
	hardcore_random_percent = (int)(100.0f * (1.0f - near_weight / total_weight) + 0.5f);
	if (hardcore_random_percent < 0)
		hardcore_random_percent = 0;
	if (hardcore_random_percent > 100)
		hardcore_random_percent = 100;
}

static void hardcore_marker_line(real_point3d const *position, real_argb_color const *color)
{
	real_point3d base, top;

	base = *position;
	top = base;
	top.z += 0.8f;
	rasterizer_debug_line(&base, &top, color);
}

static void hardcore_markers(void)
{
	struct player_datum *local;
	real_argb_color const outline = { 1.0f, 0.2f, 0.85f, 1.0f };
	real_argb_color const shield = { 1.0f, 1.0f, 0.85f, 0.2f };
	long index;

	if (ui_widgets_active() || !players_globals || players_globals->local_player_count <= 0)
		return;
	if (!hardcore_live.outlines && !hardcore_live.os_visible)
		return;
	local = player_try_and_get(hardcore_local_player_index());
	for (index = 0; index < HARDCORE_PLAYER_SLOTS; index++)
	{
		struct player_datum *player = player_try_and_get(index);
		struct unit_datum *unit;

		if (!player || player->unit_index == NONE)
			continue;
		unit = unit_try_and_get(player->unit_index);
		if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		if (hardcore_live.outlines && local && player != local &&
			local->team_index >= 0 && player->team_index == local->team_index)
			hardcore_marker_line(&unit->object.position, &outline);
		if (hardcore_live.os_visible && unit->object.shield_vitality > 1.0f)
			hardcore_marker_line(&unit->object.position, &shield);
	}
}

void hardcore_rules_render(void)
{
	long font;
	real_argb_color color;
	rectangle2d bounds;
	char text[48];
	wchar_t wide[48];
	long index;

	if (!hardcore_rules_active() || !game_in_progress())
		return;
	hardcore_markers();
	hardcore_random_update();
	if (hardcore_random_percent < 0 || ui_widgets_active())
		return;
	/* the clock's font and top, so the percent is on its line */
	font = hud_globals ? hud_globals->messaging.single_player_font.index : NONE;
	if (font == NONE)
		font = hud_get_font_index();
	if (font == NONE)
		return;
	snprintf(text, sizeof(text), "RANDOM %d%%", hardcore_random_percent);
	/* window_bounds is the title-safe frame, and that frame ends 36 above
	the screen. The clock is on the screen, so this row is too. */
	bounds = render.camera.window_bounds;
	bounds.x0 = (short)(bounds.x0 + 16);
	bounds.y0 = hardcore_timer_top();
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

boolean hardcore_rules_write_message(void *buffer, long buffer_size)
{
	unsigned char *bytes = buffer;
	unsigned long header;

	if (!buffer || buffer_size < HARDCORE_RULES_MESSAGE_SIZE)
		return FALSE;
	csmemset(buffer, 0, HARDCORE_RULES_MESSAGE_SIZE);
	/* Host order until the connection write swaps the header word. */
	header = ((unsigned long)HARDCORE_RULES_MESSAGE_SIZE << 4) | (2u << 2);
	bytes[0] = (unsigned char)header;
	bytes[1] = (unsigned char)(header >> 8);
	bytes[2] = 'C';
	bytes[3] = 'R';
	bytes[4] = 'U';
	bytes[5] = 'L';
	bytes[6] = HARDCORE_RULES_VERSION;
	csmemcpy(bytes + 7, &hardcore_live, sizeof(hardcore_live));
	return TRUE;
}

boolean hardcore_rules_take_message(void const *message, long size)
{
	unsigned char const *bytes = message;

	if (!message || size != HARDCORE_RULES_MESSAGE_SIZE)
		return FALSE;
	if (bytes[2] != 'C' || bytes[3] != 'R' || bytes[4] != 'U' || bytes[5] != 'L' ||
		bytes[6] != HARDCORE_RULES_VERSION)
		return FALSE;
	if (bytes[0] != (unsigned char)(((unsigned)HARDCORE_RULES_MESSAGE_SIZE << 4) | (2u << 2)) ||
		bytes[1] != (unsigned char)((((unsigned)HARDCORE_RULES_MESSAGE_SIZE << 4) | (2u << 2)) >> 8))
		return FALSE;
	csmemcpy(&hardcore_live, bytes + 7, sizeof(hardcore_live));
	hardcore_training_pair(&hardcore_live);
	/* port: the loaded variant may not have the bit yet. the session is
	   what makes this match apply the sheet, as it does on the host. */
	hardcore_session = 1;
	if (game_in_progress() && hardcore_rules_active())
		hardcore_apply_presentation();
	return TRUE;
}

/* port: a classic variant ends the match this machine joined. the host's
   session belongs to its menu and is not cleared from here. */
void hardcore_rules_observe_match(boolean hardcore)
{
	if (!hardcore)
		hardcore_session = 0;
}
