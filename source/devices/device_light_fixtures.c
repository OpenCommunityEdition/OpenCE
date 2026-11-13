/* ---------- headers */

#include "cseries.h"
#include "devices/device_light_fixtures.h"
#include "devices/devices.h"
#include "objects/object_types.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void light_fixtures_initialize(
	void)
{
	return;
}

void light_fixtures_dispose(
	void)
{
	return;
}

void light_fixtures_initialize_for_new_map(
	void)
{
	return;
}

void light_fixtures_dispose_from_old_map(
	void)
{
	return;
}

void light_fixture_place(
	long object_index,
	struct scenario_light_fixture_datum *scenario_light_fixture)
{
	struct light_fixture_datum *light_fixture = light_fixture_get(object_index);

	light_fixture_definition_get(light_fixture->definition_index);
	device_add_scenario_information(object_index, &scenario_light_fixture->device);
	light_fixture->light_fixture.color = scenario_light_fixture->color;
	light_fixture->light_fixture.intensity = scenario_light_fixture->intensity;
	light_fixture->light_fixture.falloff_angle = scenario_light_fixture->falloff_angle;
	light_fixture->light_fixture.cutoff_angle = scenario_light_fixture->cutoff_angle;

	return;
}

boolean light_fixture_new(
	long object_index)
{
	struct light_fixture_datum *light_fixture = light_fixture_get(object_index);

	light_fixture_definition_get(light_fixture->definition_index);
	return TRUE;
}

void light_fixture_delete(
	long object_index)
{
	return;
}

boolean light_fixture_update(
	long object_index)
{
	struct light_fixture_datum *light_fixture = light_fixture_get(object_index);

	light_fixture_definition_get(light_fixture->definition_index);
	return TRUE;
}

/* ---------- private code */
