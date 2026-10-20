/* Presentation-only item wave reminders derived from the active map and
 * variant. Never sample the gameplay RNG, inspect pickups, or spawn objects. */
#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "items/equipment_definitions.h"
#include "objects/object_types.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "performance_timer_items.h"

struct performance_item_wave
{
	long period;
	unsigned category;
};
static struct performance_item_wave performance_item_waves[MAXIMUM_SCENARIO_NETGAME_EQUIPMENT_PER_SCENARIO];
static short performance_item_wave_count;

static unsigned performance_timer_item_category(long definition_index)
{
	struct object_definition const *definition;
	if (definition_index == NONE) return 0;
	definition = object_definition_get(definition_index);
	/* Only weapon remapping is pure. Generic equipment remapping consumes
	 * gameplay RNG for grenades at higher player counts, even during a read. */
	if (definition->object.type == _object_type_weapon)
	{
		definition_index = game_engine_remap_weapon(definition_index);
		/* game_engine.c's canonical game-globals weapon list: rocket launcher=7. */
		return definition_index != NONE && weapon_definition_index_to_list_index(definition_index) == 7 ? 1 : 0;
	}
	if (definition->object.type == _object_type_equipment)
	{
		short type = equipment_definition_get(definition_index)->equipment.powerup_type;
		unsigned long flags = game_engine_get_variant()->universal_variant.flags;
		if (type == _equipment_powerup_active_camouflage &&
			!TEST_FLAG(flags, _game_variant_always_invisible_bit)) return 2;
		if (type == _equipment_powerup_overshield &&
			!TEST_FLAG(flags, _game_variant_no_shields_bit)) return 4;
	}
	return 0;
}

void performance_timer_items_initialize(void)
{
	struct scenario const *scenario = global_scenario_get();
	short index;
	performance_item_wave_count = 0;
	if (!game_engine_running()) return;
	for (index = 0; index < scenario->netgame_equipment.count &&
		index < MAXIMUM_SCENARIO_NETGAME_EQUIPMENT_PER_SCENARIO; index++)
	{
		struct scenario_netgame_equipment const *equipment = TAG_BLOCK_GET_ELEMENT(
			&scenario->netgame_equipment, index, struct scenario_netgame_equipment);
		struct item_collection_definition const *collection;
		short permutation_index;
		long seconds, total_weight = 0;
		unsigned category = 0;
		boolean predictable = TRUE;
		if (equipment->item_collection.index == NONE || !match_game_type(
			game_engine->type, NUMBEROF(equipment->game_type), equipment->game_type)) continue;
		collection = tag_get(ITEM_COLLECTION_DEFINITION_TAG, equipment->item_collection.index);
		for (permutation_index = 0; permutation_index < collection->permutations.count; permutation_index++)
		{
			struct item_permutation_definition const *permutation = TAG_BLOCK_GET_ELEMENT(
				&collection->permutations, permutation_index, struct item_permutation_definition);
			unsigned candidate;
			total_weight = (long)((real)total_weight + permutation->weight);
			if (permutation->weight <= 0) continue;
			candidate = performance_timer_item_category(permutation->item.index);
			if (!candidate || (category && candidate != category)) predictable = FALSE;
			category = candidate;
		}
		/* A mixed random collection cannot truthfully promise a specific item. */
		if (!predictable || !category || total_weight <= 0) continue;
		seconds = equipment->spawn_time ? equipment->spawn_time : collection->spawn_time;
		if (!seconds) seconds = 30;
		if (seconds < 0) continue;
		performance_item_waves[performance_item_wave_count].period = seconds * TICKS_PER_SECOND;
		performance_item_waves[performance_item_wave_count++].category = category;
	}
}

unsigned performance_timer_items_due(long previous, long current)
{
	short index;
	unsigned result = 0;
	if (previous < 0 || current <= previous || current - previous > TICKS_PER_SECOND) return 0;
	for (index = 0; index < performance_item_wave_count; index++)
	{
		struct performance_item_wave const *wave = &performance_item_waves[index];
		if (current / wave->period > previous / wave->period) result |= wave->category;
	}
	return result;
}
