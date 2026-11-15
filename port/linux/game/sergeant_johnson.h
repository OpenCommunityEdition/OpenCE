#ifndef __SERGEANT_JOHNSON_H
#define __SERGEANT_JOHNSON_H

/* Sergeant Johnson where the campaign's story has him (sergeant_johnson.c) */

struct scenario_object_datum;

enum
{
	_sergeant_johnson_map_other,
	_sergeant_johnson_map_a10,
	_sergeant_johnson_map_a50,
};

/* the map a scenario's name is, of those he is placed in */
short sergeant_johnson_map(char const *scenario_name);
/* the variant of a squad's first marine (a leader's: 100 Johnson, 101
Lehto) in an encounter of a map, a50's Johnson alive or not */
short sergeant_johnson_leader_variant_in(short map, char const *encounter_name, short variant, boolean alive);
/* the variant of a map's object placed from the scenario */
short sergeant_johnson_scenario_variant_in(short map, char const *object_name, char const *definition_name,
	short variant, boolean alive);

/* a new map */
void sergeant_johnson_reset(void);
/* encounter_create: this map's leader */
short sergeant_johnson_leader_variant(char const *encounter_name, short variant);
/* object_new_from_scenario: this map's object */
short sergeant_johnson_scenario_variant(struct scenario_object_datum const *scenario_object, long definition_index,
	short variant);
/* ai_erase_all, before it erases */
void sergeant_johnson_erasing_all(void);
/* a unit died (unit_died) */
void sergeant_johnson_unit_died(long unit_index);

#endif
