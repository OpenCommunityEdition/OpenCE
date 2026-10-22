/* Host-selected practice presentation, separate from weapon/game rules.
 * Scenery is not a replicated object type. Each machine creates the reviewed
 * static marker locally, under the shared host setting. No new scenery network
 * protocol or script execution is needed, and randoms are never recreated. */
#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "cutscene/cinematics.h"
#include "interface/hud_messaging.h"
#include "interface/ui_widget.h"
#include "math/real_math.h"
#include "networking/network_game_globals.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "objects/object_types.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "text/draw_string.h"
#include "text/text_group.h"
#include "network_distributed.h"
#include "performance_options.h"
#include "hardcore_rules.h"
#include "performance_audio.h"
#include "performance_timer_schedule.h"
#include "performance_timer_items.h"
#include "performance_timer_render.h"
#include "../src/port_config.h"

/* The glowing pads are loaded from the user's hardcore map. These stay
declared here so a missing model keeps the line markers. */
boolean performance_spawn_marker_ready(void);
void performance_spawn_marker_render(struct player_starting_location const *starts, short count);
void performance_spawn_marker_reset(void);
boolean hardcore_rules_random_areas(void);
void performance_random_area_render(struct player_starting_location const *starts, short count);
void performance_random_area_reset(void);

struct performance_marker
{
	short name_index;
	long definition_index;
	/* Full datum identifier, not an absolute slot that can be reused. */
	long owned_object_index;
};

static unsigned long performance_flags;
static short performance_marker_count;
static struct performance_marker performance_markers[MAXIMUM_OBJECT_NAMES_PER_SCENARIO];
/* Stock maps already contain all spawn metadata. These markers are rendered
 * from those positions; no map edits, object tags or replicated objects. */
static short performance_fallback_count;
static struct player_starting_location performance_fallback[MAXIMUM_OBJECT_NAMES_PER_SCENARIO];
static long performance_audio_previous_tick = NONE;
static struct performance_timer_statistics performance_timer_statistics;
static long performance_audio_busy_until;
static long performance_item_pending[3] = {NONE, NONE, NONE};

static void performance_timer_audio_reset(void)
{
	short index;
	performance_audio_previous_tick = NONE;
	performance_audio_busy_until = 0;
	for (index = 0; index < 3; index++) performance_item_pending[index] = NONE;
}


unsigned long performance_options_get_flags(void)
{
	return performance_flags;
}

void performance_options_apply_host_flags(unsigned long flags)
{
	unsigned long old_flags = performance_flags;
	/* A malformed extension must never enable the known subset. */
	performance_flags = (flags & ~PERFORMANCE_OPTIONS_MASK) ? 0 : flags;
	if ((old_flags ^ performance_flags) & _performance_option_timer_audio)
	{
		performance_timer_audio_reset();
		if (!(performance_flags & _performance_option_timer_audio))
			halo_performance_audio_stop();
	}
}

static boolean performance_marker_definition_supported(long definition_index)
{
	struct object_definition const *definition;
	char const *name;
	short index;

	if (definition_index == NONE)
		return FALSE;
	name = tag_get_name(definition_index);
	if (_stricmp(name, "scenery\\spawn_marker_nhe\\spawn_marker_nhe") &&
		_stricmp(name, "scenery/spawn_marker_nhe/spawn_marker_nhe"))
		return FALSE;
	definition = object_definition_get(definition_index);
	/* The reviewed Prisoner/Downrush marker is a static model with no
	 * physics, collision, animation, side effects or runtime functions.
	 * Other tags with similar names are not implicitly trusted as markers. */
	if (definition->object.type != _object_type_scenery ||
		definition->object.model.index == NONE ||
		definition->object.animation_graph.index != NONE ||
		definition->object.collision_model.index != NONE ||
		definition->object.physics.index != NONE ||
		definition->object.creation_effect.index != NONE ||
		definition->object.attachments.count || definition->object.widgets.count ||
		definition->object.functions.count)
		return FALSE;
	for (index = 0; index < NUMBEROF(definition->object.function_modes); index++)
		if (definition->object.function_modes[index] != _object_function_none)
			return FALSE;
	return TRUE;
}

void performance_options_initialize_for_new_map(void)
{
	struct scenario *scenario = global_scenario_get();
	short name_index;
	long placement_size;
	struct tag_block *placements;

	performance_marker_count = 0;
	performance_fallback_count = 0;
	performance_timer_audio_reset();
	memset(&performance_timer_statistics, 0, sizeof(performance_timer_statistics));
	performance_timer_items_initialize();
	performance_options_apply_host_flags(game_engine_running() ?
		performance_variant_get_flags(game_engine_get_variant()) : 0);
	if (!game_engine_running())
		return;
	placements = scenario_get_object_type_scenario_datums(scenario,
		_object_type_scenery, &placement_size);
	for (name_index = 0; name_index < scenario->object_names.count &&
		name_index < MAXIMUM_OBJECT_NAMES_PER_SCENARIO; name_index++)
	{
		struct scenario_object_name const *name = TAG_BLOCK_GET_ELEMENT(
			&scenario->object_names, name_index, struct scenario_object_name);
		struct scenario_object_datum const *placement;
		struct scenario_object_palette_entry const *palette;
		struct performance_marker *marker;

		if (strncmp(name->name, "spawn_marker", 12) ||
			name->runtime_object_type != _object_type_scenery ||
			!VALID_INDEX(name->runtime_scenario_datum_index, placements->count))
			continue;
		placement = tag_block_get_element_with_size(placements,
			name->runtime_scenario_datum_index, placement_size);
		if (placement->name_index != name_index ||
			!VALID_INDEX(placement->palette_entry_index, scenario->scenery_palette.count))
			continue;
		palette = TAG_BLOCK_GET_ELEMENT(&scenario->scenery_palette,
			placement->palette_entry_index, struct scenario_object_palette_entry);
		if (!performance_marker_definition_supported(palette->reference.index))
			continue;
		marker = &performance_markers[performance_marker_count++];
		marker->name_index = name_index;
		marker->definition_index = palette->reference.index;
		marker->owned_object_index = NONE;
	}
	if (performance_marker_count == 0)
	{
		for (name_index = 0; name_index < scenario->players.count &&
			performance_fallback_count < NUMBEROF(performance_fallback); name_index++)
		{
			struct player_starting_location const *start = TAG_BLOCK_GET_ELEMENT(
				&scenario->players, name_index, struct player_starting_location);
			if (match_game_type(game_engine->type, NUMBEROF(start->game_types), start->game_types))
				performance_fallback[performance_fallback_count++] = *start;
		}
	}
}

void performance_options_dispose_from_old_map(void)
{
	/* The game's map teardown owns object destruction. Forget identifiers
	 * before that storage can be reused by another map. The pad's buffers
	 * belong to this map. Its bitmap record stays until the texture cache
	 * closes. */
	performance_spawn_marker_reset();
	performance_random_area_reset();
	performance_marker_count = 0;
	performance_fallback_count = 0;
	performance_timer_audio_reset();
	halo_performance_audio_stop();
	performance_flags = 0;
}

static struct object_datum *performance_marker_owned_object(struct performance_marker *marker)
{
	struct object_datum *object = marker->owned_object_index == NONE ? NULL :
		object_try_and_get(marker->owned_object_index);
	if (object && object->definition_index == marker->definition_index &&
		object->object.type == _object_type_scenery &&
		object->object.name_index == marker->name_index &&
		object_index_from_name_index(marker->name_index) == marker->owned_object_index)
		return object;
	marker->owned_object_index = NONE;
	return NULL;
}

void performance_options_timer_statistics(struct performance_timer_statistics *statistics)
{
	*statistics = performance_timer_statistics;
	if (!statistics->last_cue) statistics->last_cue = "none";
}

/* port: the beep and cortana clips are dry samples at the ear, so full
   scale covers gunfire. the timer volume setting still scales this. */
#define PERFORMANCE_TIMER_PLAY_GAIN 0.4f

static void performance_timer_audio_play(char const *cue, boolean item, long ticks)
{
	if (halo_performance_audio_play(cue, PERFORMANCE_TIMER_PLAY_GAIN))
	{
		performance_timer_statistics.last_cue = cue;
		if (item)
		{
			performance_timer_statistics.items++;
			performance_timer_statistics.items_by_category[!strcmp(cue, "rocket") ? 0 : !strcmp(cue, "camo") ? 1 : 2]++;
		}
		else if (!strcmp(cue, "timerbeep")) performance_timer_statistics.beeps++;
		else if (strstr(cue, "minute")) performance_timer_statistics.minutes++;
		else performance_timer_statistics.countdown++;
	}
	performance_audio_busy_until = ticks + halo_performance_audio_duration_ticks(cue);
}

static void performance_timer_audio_update(long ticks)
{
	static char const *const item_cues[3] = {"rocket", "camo", "overshield"};
	unsigned preferences = (config_boolean("audio.timer_countdown") ? 1 : 0) |
		(config_boolean("audio.timer_beeps") ? 2 : 0) |
		(config_boolean("audio.timer_minutes") ? 4 : 0) |
		(config_boolean("audio.timer_items") ? 8 : 0);
	char const *cue;
	short index;
	unsigned due;
	if (preferences != performance_timer_statistics.preferences || ticks < performance_audio_previous_tick ||
		(ticks > performance_audio_previous_tick && ticks - performance_audio_previous_tick > TICKS_PER_SECOND))
	{
		performance_timer_audio_reset();
		halo_performance_audio_stop();
	}
	performance_timer_statistics.preferences = preferences;
	cue = performance_timer_cue_masked(performance_audio_previous_tick, ticks, preferences);
	due = (preferences & 8) ? performance_timer_items_due(performance_audio_previous_tick, ticks) : 0;
	for (index = 0; index < 3; index++)
		if (due & (1u << index)) performance_item_pending[index] = ticks;
	if (cue)
	{
		performance_timer_audio_play(cue, FALSE, ticks);
	}
	/* Announce recent waves once, between ordinary timer calls. Reserve the
	 * exact recording duration so item names neither overlap nor cut off the
	 * countdown. Expire crowded reminders instead of announcing stale items. */
	for (index = 0; index < 3; index++)
	{
		long duration, next;
		boolean fits = TRUE;
		if (performance_item_pending[index] == NONE) continue;
		if (ticks - performance_item_pending[index] > 6 * TICKS_PER_SECOND)
		{
			performance_item_pending[index] = NONE;
			continue;
		}
		if (cue || ticks < performance_audio_busy_until || halo_performance_audio_busy()) continue;
		duration = halo_performance_audio_duration_ticks(item_cues[index]);
		if (duration <= 0) { performance_item_pending[index] = NONE; continue; }
		for (next = ticks + 1; next <= ticks + duration; next++)
			if (performance_timer_cue_masked(next - 1, next, preferences)) { fits = FALSE; break; }
		if (!fits) continue;
		performance_timer_audio_play(item_cues[index], TRUE, ticks);
		performance_item_pending[index] = NONE;
		break;
	}
	performance_audio_previous_tick = ticks;
}

void performance_options_update(void)
{
	short index;
	boolean enabled;

	if (!game_in_progress() || !game_engine_running())
		return;
	/* port: hardcore ammo rules, each tick, and only while that mode is on. */
	hardcore_rules_tick();
	/* port: a hardcore match uses the timer recordings on any map. the
	   cue toggles still choose which lines play. a missing pack is silent. */
	if (((performance_flags & _performance_option_timer_audio) || hardcore_rules_active()) &&
		!cinematic_in_progress() && !game_engine_showing_postgame())
	{
		performance_timer_audio_update(performance_options_timer_ticks());
	}
	else
	{
		if (performance_audio_previous_tick != NONE) halo_performance_audio_stop();
		performance_timer_audio_reset();
	}
	enabled = (performance_flags & _performance_option_spawn_markers) != 0;
	for (index = 0; index < performance_marker_count; index++)
	{
		struct performance_marker *marker = &performance_markers[index];
		struct object_datum *object = performance_marker_owned_object(marker);

		if (!enabled)
		{
			if (object)
			{
				object_delete(marker->owned_object_index);
				marker->owned_object_index = NONE;
			}
			continue;
		}
		/* The name table prevents duplicates, including while a startup
		 * script's deletion waits for objects_update. Never adopt existing
		 * scenery: only our own creations may be deleted by the toggle. */
		if (!object && object_index_from_name_index(marker->name_index) == NONE)
		{
			unsigned long seed = get_random_seed();
			marker->owned_object_index = object_new_by_name(marker->name_index);
			/* Choosing the marker model's permutations must not consume the
			 * gameplay random sequence. Reviewed marker tags have no effects. */
			set_random_seed(seed);
			object = performance_marker_owned_object(marker);
		}
		if (object)
		{
			SET_FLAG(object->object.flags, _object_no_collisions_bit, TRUE);
			SET_FLAG(object->object.damage_flags, _object_cannot_take_damage_bit, TRUE);
		}
	}
}

long performance_options_timer_ticks(void)
{
	long ticks;
	if (!game_time_initialized() || !game_in_progress() || !game_engine_running())
		return NONE;
	/* Each distributed client ticks independently. Its latest received host
	 * time is the authoritative match clock, including for a late joiner.
	 * Do not substitute its local clock during packet loss or before sync. */
	ticks = network_game_distributed_client() ? distributed_latest_host_time() : game_time_get();
	return ticks < 0 ? NONE : ticks;
}

short performance_options_markers_supported_count(void)
{
	return performance_marker_count + performance_fallback_count;
}

static boolean performance_spawn_in_active_bsp(struct player_starting_location const *start)
{
	return start->structure_bsp_reference_index == NONE ||
		start->structure_bsp_reference_index == global_structure_bsp_index_get();
}

short performance_options_markers_visible_count(void)
{
	short index, count = 0;
	if (!game_time_initialized())
		return 0;
	if (performance_fallback_count && (performance_flags & _performance_option_spawn_markers))
	{
		for (index = 0; index < performance_fallback_count; index++)
			if (performance_spawn_in_active_bsp(&performance_fallback[index])) count++;
		return count;
	}
	for (index = 0; index < performance_marker_count; index++)
	{
		struct performance_marker *marker = &performance_markers[index];
		struct object_datum *object = performance_marker_owned_object(marker);
		struct object_header_datum *header = object ? object_header_get(marker->owned_object_index) : NULL;
		if (header && !TEST_FLAG(header->flags, _object_header_being_deleted_bit) &&
			!TEST_FLAG(object->object.flags, _object_invisible_bit))
			count++;
	}
	return count;
}

static void performance_options_render_spawn_points(void)
{
	short index, edge;
	real_argb_color const color = {1.0f, 0.15f, 1.0f, 0.25f};
	/* Debug lines flush after widgets. Hide them while menus are open so
	 * world-space markers cannot cover native menu labels. */
	if (!(performance_flags & _performance_option_spawn_markers) || ui_widgets_active()) return;
	/* The pad model is already in this frame's transparent geometry. */
	if (performance_spawn_marker_ready()) return;
	for (index = 0; index < performance_fallback_count; index++)
	{
		struct player_starting_location const *start = &performance_fallback[index];
		real_point3d tip = start->position, ring[4], top, direction;
		if (!performance_spawn_in_active_bsp(start)) continue;
		tip.z += 0.08f;
		top = tip; top.z += 0.7f;
		for (edge = 0; edge < 4; edge++)
		{
			ring[edge] = tip;
			ring[edge].x += edge == 0 ? 0.18f : edge == 2 ? -0.18f : 0;
			ring[edge].y += edge == 1 ? 0.18f : edge == 3 ? -0.18f : 0;
			ring[edge].z += 0.3f;
			rasterizer_debug_line(&tip, &ring[edge], &color);
		}
		for (edge = 0; edge < 4; edge++)
			rasterizer_debug_line(&ring[edge], &ring[(edge + 1) % 4], &color);
		rasterizer_debug_line(&tip, &top, &color);
		direction = tip;
		direction.x += cosine(start->facing) * 0.35f;
		direction.y += sine(start->facing) * 0.35f;
		rasterizer_debug_line(&tip, &direction, &color);
	}
}

void performance_options_render(void)
{
	if (!game_in_progress() || !game_engine_running() || cinematic_in_progress() || game_engine_showing_postgame())
		return;
	performance_options_render_spawn_points();
	/* port: hardcore markers and the random-spawn estimate. */
	hardcore_rules_render();
	if (performance_flags & _performance_option_match_timer)
		performance_timer_render(performance_options_timer_ticks());
}

void performance_options_render_world(void)
{
	if (!game_in_progress() || !game_engine_running() || cinematic_in_progress() ||
		game_engine_showing_postgame() || ui_widgets_active())
		return;
	if ((performance_flags & _performance_option_spawn_markers) && performance_fallback_count >= 1)
		performance_spawn_marker_render(performance_fallback, performance_fallback_count);
	/* The darker zones are the hardcore map's randoms mesh when it sits
	on the starts. A map that never placed one draws none. */
	if (hardcore_rules_random_areas())
		performance_random_area_render(performance_fallback, performance_fallback_count);
}
