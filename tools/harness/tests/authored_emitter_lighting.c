#include "harness.h"
#include <string.h>
#define NUMBEROF(a) ((int)(sizeof(a) / sizeof((a)[0])))



struct object_datum { long definition_index; } object;
static int weapon_on, vehicle_on, projectile_on, lookups, generation;
static const char *names[64];
static long groups[64];
static int name_count;
int lighting_config_boolean(const char *name)
{
	return !strcmp(name, "display.weapon_glow") ? weapon_on :
		!strcmp(name, "display.vehicle_lighting") ? vehicle_on : projectile_on;
}
struct object_datum *object_try_and_get(long index) { return index == 1 ? &object : NULL; }
long tag_loaded(long group, const char *name)
{
	int i;
	lookups++;
	for (i = 0; i < name_count; i++)
		if (group == groups[i] && !strcmp(name, names[i])) return generation + i;
	names[name_count] = name;
	groups[name_count] = group;
	return generation + name_count++;
}
#include "under_test.inc"
int main(int argc, char **argv)
{
	int i, calls;
	float scale = 42;
	(void)argc; (void)argv;
	CHECK(!authored_emitter_lighting_profile(1, 10, &scale) && !lookups && scale == 42,
		"off performs no tag discovery or writes");
	for (i = 0; i < NUMBEROF(profiles); i++)
	{
		long light;
		weapon_on = profiles[i].category == 1;
		vehicle_on = profiles[i].category == 2;
		projectile_on = profiles[i].category == 3;
		object.definition_index = tag_loaded(profiles[i].group, profiles[i].object);
		light = tag_loaded('ligh', profiles[i].light);
		CHECK(authored_emitter_lighting_profile(1, light, &scale) == profiles[i].category &&
			scale == profiles[i].scale, "selected source uses its category and scale");
		calls = lookups;
		CHECK(authored_emitter_lighting_profile(1, light, &scale) && lookups == calls,
			"steady frames reuse cached identities");
		CHECK(!authored_emitter_lighting_profile(1, 60000, &scale), "unrelated light stays stock");
		CHECK(!authored_emitter_lighting_profile(2, light, &scale), "deleted owner stays stock");
		weapon_on = vehicle_on = projectile_on = 0;
		CHECK(!authored_emitter_lighting_profile(1, light, &scale), "disable immediately restores stock");
	}
	weapon_on = 1;
	generation = 1000;
	authored_emitter_lighting_forget();
	object.definition_index = tag_loaded('weap', "weapons\\energy sword\\energy sword");
	CHECK(authored_emitter_lighting_profile(1, tag_loaded('ligh', "weapons\\energy sword\\energy sword dynamic"), &scale),
		"new map resolves new tag identities");
	CHECK(!authored_emitter_lighting_profile(1, 1, &scale), "old map identity cannot match");
	return 0;
}
