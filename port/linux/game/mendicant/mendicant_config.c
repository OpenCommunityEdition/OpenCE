/*
MENDICANT_CONFIG.C

MENDICANT AI's settings: every ai.mendicant.* key of port_config.c read into
mendicant_config at the start of each map (mendicant_initialize_for_new_map).
Each key has its HALO_AI_MENDICANT_* environment variable through
port_config.c. On desktop Linux, a key whose HALO_* variable is unset also
reads the hook library's AI_BRAIN_* name, so that trainer.py and play.sh run
this build with no change but the binary's path.
*/

#include "cseries.h"
#include "math/real_math.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mendicant.h"

/* port/linux/src/port_config.c */
int config_boolean(char const *name);
long config_integer(char const *name);
double config_real(char const *name);
char const *config_string(char const *name);

/* ---------- constants */

/* the hook library's environment names are read on desktop Linux only */
#if !defined(HALO_WINDOWS) && !defined(HALO_ANDROID) && !defined(HALO_WEB)
#define MENDICANT_LEGACY_ENVIRONMENT 1
#endif

/* ---------- globals */

struct mendicant_config mendicant_config;

/* ---------- private code */

#ifdef MENDICANT_LEGACY_ENVIRONMENT
/* the legacy variable's value, when the setting's own HALO_* variable is unset */
static char const *mendicant_legacy(
	char const *environment,
	char const *legacy)
{
	if (getenv(environment))
		return NULL;
	return getenv(legacy);
}
#endif

static void mendicant_config_copy_string(
	char *destination,
	size_t size,
	char const *source)
{
	strncpy(destination, source ? source : "", size - 1);
	destination[size - 1] = 0;

	return;
}

/* ---------- public code */

/* reads every key; TRUE when anything differs from the last read */
boolean mendicant_config_read(
	void)
{
	static boolean read_before = FALSE;
	struct mendicant_config previous = mendicant_config;
	struct mendicant_config *config = &mendicant_config;
#ifdef MENDICANT_LEGACY_ENVIRONMENT
	char const *legacy;
#endif

	memset(config, 0, sizeof(*config));
	config->enabled = config_boolean("ai.mendicant.enabled") != 0;
	config->models = config_boolean("ai.mendicant.models") != 0;
	mendicant_config_copy_string(config->weights, sizeof(config->weights), config_string("ai.mendicant.weights"));
	config->variety = (real)config_real("ai.mendicant.variety");
	config->seed = config_integer("ai.mendicant.seed");
	config->individuals = config_boolean("ai.mendicant.individuals") != 0;
	config->discipline = config_boolean("ai.mendicant.discipline") != 0;
	config->targeting = config_boolean("ai.mendicant.targeting") != 0;
	config->aim = (real)config_real("ai.mendicant.aim");
	config->aim_difficulty = config_boolean("ai.mendicant.aim_difficulty") != 0;
	config->aim_moving = config_boolean("ai.mendicant.aim_moving") != 0;
	config->marine_fire = (real)config_real("ai.mendicant.marine_fire");
	config->command_radius = (real)config_real("ai.mendicant.command_radius");
#ifdef MENDICANT_LEGACY_ENVIRONMENT
	/* the training bridge, its runs and its logs: desktop Linux only */
	mendicant_config_copy_string(config->bridge, sizeof(config->bridge), config_string("ai.mendicant.bridge"));
	config->training = config_boolean("ai.mendicant.training") != 0;
	config->training_skip = config_integer("ai.mendicant.training_skip");
	mendicant_config_copy_string(config->sample_log, sizeof(config->sample_log), config_string("ai.mendicant.sample_log"));
	mendicant_config_copy_string(config->drill_log, sizeof(config->drill_log), config_string("ai.mendicant.drill_log"));
	mendicant_config_copy_string(config->player_log, sizeof(config->player_log), config_string("ai.mendicant.player_log"));
	mendicant_config_copy_string(config->move_test_log, sizeof(config->move_test_log), config_string("ai.mendicant.move_test_log"));
	config->vehicle_test = config_boolean("ai.mendicant.vehicle_test") != 0;
	config->aim_debug = config_boolean("ai.mendicant.aim_debug") != 0;

	/* the hook library's names, each with the meaning it had there */
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT", "AI_BRAIN_DRILL")) != NULL)
		config->enabled = TRUE; /* set at all: on */
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_TRAINING", "AI_BRAIN_TRAINING")) != NULL)
	{
		/* training implied the drill */
		config->training = TRUE;
		if (!getenv("HALO_AI_MENDICANT"))
			config->enabled = TRUE;
	}
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_BRIDGE", "AI_BRAIN_BRIDGE")) != NULL)
		mendicant_config_copy_string(config->bridge, sizeof(config->bridge), legacy);
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_TRAINING_SKIP", "AI_BRAIN_TRAINING_SKIP")) != NULL)
		config->training_skip = atoi(legacy);
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_SAMPLE_LOG", "AI_BRAIN_LOG")) != NULL)
		mendicant_config_copy_string(config->sample_log, sizeof(config->sample_log), legacy);
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_DRILL_LOG", "AI_BRAIN_DRILL_LOG")) != NULL)
		mendicant_config_copy_string(config->drill_log, sizeof(config->drill_log), legacy);
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_PLAYER_LOG", "AI_BRAIN_PLAYER_LOG")) != NULL)
		mendicant_config_copy_string(config->player_log, sizeof(config->player_log), legacy);
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_MOVE_TEST_LOG", "AI_BRAIN_MOVE_TEST")) != NULL)
		mendicant_config_copy_string(config->move_test_log, sizeof(config->move_test_log), legacy);
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_VEHICLE_TEST", "AI_BRAIN_VEHICLE_TEST")) != NULL)
		config->vehicle_test = TRUE;
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_AIM_DEBUG", "AI_BRAIN_AIM_DEBUG")) != NULL)
		config->aim_debug = TRUE;
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_DISCIPLINE", "AI_BRAIN_DISCIPLINE")) != NULL)
		config->discipline = strcmp(legacy, "0") != 0; /* only 0 left panic and pursuit to the game */
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_TARGETING", "AI_BRAIN_TARGETING")) != NULL)
		config->targeting = atoi(legacy) != 0;
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_AIM", "AI_BRAIN_AIM")) != NULL)
		config->aim = (real)atof(legacy);
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_AIM_DIFFICULTY", "AI_BRAIN_AIM_DIFFICULTY")) != NULL)
		config->aim_difficulty = atoi(legacy) != 0;
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_AIM_MOVING", "AI_BRAIN_AIM_MOVING")) != NULL)
		config->aim_moving = atoi(legacy) != 0;
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_MARINE_FIRE", "AI_BRAIN_MARINE_FIRE")) != NULL)
		config->marine_fire = (real)atof(legacy);
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_COMMAND_RADIUS", "AI_BRAIN_COMMAND_RADIUS")) != NULL)
		config->command_radius = (real)atof(legacy);
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_INDIVIDUALS", "AI_BRAIN_INDIVIDUALS")) != NULL)
		config->individuals = atoi(legacy) > 0;
	if ((legacy = mendicant_legacy("HALO_AI_MENDICANT_VARIETY", "AI_BRAIN_VARIETY")) != NULL)
		config->variety = (real)atof(legacy);
#endif

	/* the hook library kept the Marines' rate of fire at a tenth or more */
	if (!(config->marine_fire >= 0.1f))
		config->marine_fire = 0.1f;

	if (!read_before)
	{
		read_before = TRUE;
		return TRUE;
	}
	return memcmp(&previous, config, sizeof(previous)) != 0;
}
