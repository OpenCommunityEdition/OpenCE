/*
MENDICANT_MODELS.C

MENDICANT AI's runtime (M1): the trained models answer the game's requests
in the game's own process, in place of brain.py.

The weights are one file, models.aibw (tools/mendicant/export_models.py), found
through the mods folder: ai.mendicant.weights is its tag-file key, by
default "mendicant\models.aibw", which is the file
mods/<mod>/tags/mendicant/models.aibw of a mod with no `maps =` in its
mod.toml. It is read when a map starts and again whenever its size or time
changes between maps. A missing file, a damaged one, one of a newer format
or one made for another doctrine leaves the runtime off, and the drill
decides, as it did without brain.py.

mendicant_runtime_answer does what brain.py's Bridge.serve did, operation
for operation (mendicant_infer.c holds policy.py's models):

1. the strategic model of the request's side picks a drill for an Elite's
   unit (or a Marine squad) when the request asks for one: a new strategy
   (a chain, a side and a tempo) or the next step of one, sampled within
   what the doctrine allows, at ai.mendicant.variety;
2. the side's per-enemy model scores each member's candidate positions, and
   each member takes his best position no one else has;
3. soldiers on their own (no unit commands them: team[7] below 0.5) each
   get a way of fighting from the stance model;
4. the answer is published by setting response_sequence last.

Its random numbers are its own (splitmix64), seeded at each map by
ai.mendicant.seed, or from the map's name and the clock when that is 0: the
game's random seeds are never touched, so the netcode and replays are as
they were.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "main/main.h"

#include "mods.h" /* port/linux/game/mods.c (-iquote port/linux/game) */
#include "mendicant_internal.h"
#include "mendicant_infer.h"

#include <stdlib.h>
#include <string.h>

void platform_log(char const *format, ...);
unsigned long system_milliseconds(void);

/* ---------- constants */

/* the teams' numbers in a request (team_index) */
#define MENDICANT_MODELS_TEAM_HUMAN 2
#define MENDICANT_MODELS_TEAM_COVENANT 3

/* the largest models file read (the full-precision file is 6.6 MB) */
#define MENDICANT_MODELS_MAXIMUM_FILE_SIZE (64UL * 1024 * 1024)

/* the doctrine's largest number of chains the mask reads */
#define MENDICANT_MODELS_MAXIMUM_CHAINS 64

/* what the last look for the file found, so that each change is logged once */
enum
{
	_mendicant_models_status_none = 0,
	_mendicant_models_status_off,
	_mendicant_models_status_missing,
	_mendicant_models_status_unreadable,
	_mendicant_models_status_refused,
	_mendicant_models_status_loaded
};

/* ---------- private variables */

static struct
{
	/* the weights, and whether they are loaded */
	struct mendicant_infer_weights weights;
	boolean loaded;
	/* the file they came from (or that was refused), as port_mods sees it */
	char path[256];
	unsigned long size;
	unsigned long stamp[2];
	boolean stamp_valid;
	int status;
	/* the doctrine the game runs, as the decision mask reads it */
	unsigned char chain_covenant[MENDICANT_MODELS_MAXIMUM_CHAINS];
	/* the sampling's random state */
	unsigned long long random_state;
} mendicant_models;

/* a strategic decision's scratch (about 66 KB) and the request's inputs and
outputs: static, never on the (Android guest's) stack */
static struct mendicant_infer_workspace mendicant_models_workspace;
static float mendicant_models_plan_features[MENDICANT_INFER_PLAN_FEATURES];
static float mendicant_models_logits[MENDICANT_INFER_MAX_DECISIONS];
static unsigned char mendicant_models_allowed[MENDICANT_INFER_MAX_DECISIONS];
static float mendicant_models_scores[MENDICANT_INFER_SQUAD * MENDICANT_INFER_CANDIDATES];

/* ---------- private code */

/* FNV-1a of a text */
static unsigned long mendicant_models_text_hash(char const *text)
{
	unsigned long hash = 0x811C9DC5UL;

	for (; text && *text; text++)
		hash = ((hash ^ (unsigned char)*text) * 0x01000193UL) & 0xFFFFFFFFUL;
	return hash;
}

/* the sampling's random numbers for this map: ai.mendicant.seed, or the
map's name and the clock */
static void mendicant_models_seed(void)
{
	unsigned long long seed;

	if (mendicant_config.seed)
	{
		seed = (unsigned long long)(unsigned long)mendicant_config.seed;
	}
	else
	{
		seed = (unsigned long long)mendicant_models_text_hash(main_get_map_name()) << 32 |
			(unsigned long long)(system_milliseconds() & 0xFFFFFFFFUL);
	}
	mendicant_models.random_state = seed;
}

static void mendicant_models_status(int status, char const *path, char const *why)
{
	if (status == mendicant_models.status)
		return;
	mendicant_models.status = status;
	switch (status)
	{
	case _mendicant_models_status_missing:
		platform_log("mendicant: no models (%s is in no mod): the drill decides", mendicant_config.weights);
		break;
	case _mendicant_models_status_unreadable:
		platform_log("mendicant: models %s could not be read: the drill decides", path);
		break;
	case _mendicant_models_status_refused:
		platform_log("mendicant: models %s refused, %s: the drill decides", path, why);
		break;
	case _mendicant_models_status_loaded:
		platform_log("mendicant: models %s: strategy %s/%s, per-enemy %s/%s, stance %s/%s (Covenant/Marines)", path,
			mendicant_models.weights.has_strategy[_mendicant_infer_covenant] ? "yes" : "no",
			mendicant_models.weights.has_strategy[_mendicant_infer_marines] ? "yes" : "no",
			mendicant_models.weights.has_squad[_mendicant_infer_covenant] ? "yes" : "no",
			mendicant_models.weights.has_squad[_mendicant_infer_marines] ? "yes" : "no",
			mendicant_models.weights.has_stance[_mendicant_infer_covenant] ? "yes" : "no",
			mendicant_models.weights.has_stance[_mendicant_infer_marines] ? "yes" : "no");
		break;
	}
}

static void mendicant_models_unload(void)
{
	if (mendicant_models.loaded)
		mendicant_infer_free(&mendicant_models.weights);
	mendicant_models.loaded = FALSE;
}

/* the model side of a request's team: 0 the Covenant, 1 the Marines, NONE
another team (brain.py had a model only for teams 3 and 2) */
static int mendicant_models_side(long team_index)
{
	if (team_index == MENDICANT_MODELS_TEAM_COVENANT)
		return _mendicant_infer_covenant;
	if (team_index == MENDICANT_MODELS_TEAM_HUMAN)
		return _mendicant_infer_marines;
	return NONE;
}

/* brain.py's strategy branch: the plan (drill, side, tempo) and the step */
static void mendicant_models_strategy(struct model_bridge *bridge, int side)
{
	struct mendicant_infer_weights const *weights = &mendicant_models.weights;
	long drill = -1, plan_side = 1, tempo = 0, step = -1;

	if (bridge->plan_request && side != NONE && weights->has_strategy[side])
	{
		long plans = (long)weights->chains * 4;
		int decision;
		float u;

		memcpy(mendicant_models_plan_features, bridge->plan_features, sizeof(float) * 16);
		memcpy(mendicant_models_plan_features + 16, bridge->plan_extra, sizeof(float) * 32);
		memcpy(mendicant_models_plan_features + MENDICANT_INFER_SUMMARY_FEATURES, bridge->plan_entities,
			sizeof(float) * MENDICANT_INFER_ENTITIES * MENDICANT_INFER_ENTITY_FEATURES);
		mendicant_infer_strategy_logits(&weights->strategy[side], &mendicant_models_workspace,
			mendicant_models_plan_features, mendicant_models_logits);
		mendicant_infer_decision_mask(mendicant_models_allowed, (int)weights->chains, (int)weights->step_choices,
			mendicant_models.chain_covenant, (int)bridge->plan_kind, (unsigned long long)bridge->step_mask,
			side == _mendicant_infer_covenant);
		u = mendicant_infer_random(&mendicant_models.random_state);
		decision = mendicant_infer_sample(mendicant_models_logits, mendicant_models_allowed, (int)weights->decisions,
			(float)mendicant_config.variety, u, NULL);
		if (decision >= plans)
		{
			step = decision - plans;
		}
		else
		{
			/* policy.plan_parts */
			drill = decision / 4;
			plan_side = (decision & 1) ? 1 : -1;
			tempo = (decision >> 1) & 1;
		}
	}
	bridge->plan_drill = drill;
	bridge->plan_side = plan_side;
	bridge->plan_tempo = tempo;
	bridge->plan_step = step;
}

/* brain.py's per-enemy branch: each member's candidate, and the stances of
soldiers on their own */
static void mendicant_models_squad(struct model_bridge *bridge, int side, int count)
{
	struct mendicant_infer_weights const *weights = &mendicant_models.weights;
	int member;

	if (side == NONE || !weights->has_squad[side])
	{
		/* no model for this side: stay put (brain.py left the stances as they
		were; 0 is the game's own way of fighting) */
		for (member = 0; member < count; member++)
		{
			bridge->choice[member] = 0;
			bridge->stance[member] = 0;
		}
		return;
	}
	{
		float side_sign;

		mendicant_infer_squad_scores(&weights->squad[side], bridge->team, &bridge->features[0][0][0], count,
			mendicant_models_scores, &side_sign);
		mendicant_infer_assign(mendicant_models_scores, &bridge->candidate_count[0], &bridge->positions[0][0], count,
			&bridge->choice[0]);
	}
	if (bridge->team[7] < 0.5f && weights->has_stance[side])
	{
		/* Brain.choose_stances: sampled from the softmax, with no variety */
		static unsigned char const all_allowed[MENDICANT_INFER_STANCES] = { 1, 1, 1, 1, 1 };

		for (member = 0; member < count; member++)
		{
			float logits[MENDICANT_INFER_STANCES];
			float u;

			mendicant_infer_stance_logits(&weights->squad[side], &bridge->features[member][0][0], bridge->team, logits);
			u = mendicant_infer_random(&mendicant_models.random_state);
			bridge->stance[member] = (short)mendicant_infer_sample(logits, all_allowed, MENDICANT_INFER_STANCES, 1.0f, u,
				NULL);
		}
	}
	else
	{
		for (member = 0; member < count; member++)
			bridge->stance[member] = 0;
	}
}

/* ---------- public code */

/* at each map while ai.mendicant.models is set: seeds the sampling, and
reads the models when they are not loaded or their file changed. FALSE:
there are no models, and the drill decides. */
boolean mendicant_runtime_load(void)
{
	long file;
	char const *path;
	unsigned long size = 0, stamp[2] = { 0, 0 };
	unsigned char *data;
	int error;

	mendicant_models_seed();
	if (!mendicant_config.models)
	{
		mendicant_runtime_dispose();
		mendicant_models.status = _mendicant_models_status_off;
		return FALSE;
	}

	file = port_mods_file_find(mendicant_config.weights, NULL);
	if (file == NONE)
	{
		mendicant_runtime_dispose();
		mendicant_models_status(_mendicant_models_status_missing, NULL, NULL);
		return FALSE;
	}
	path = port_mods_file_path(file);
	if (!path)
		path = mendicant_config.weights;
	if (!port_mods_file_stamp(file, &size, stamp) || !size || size > MENDICANT_MODELS_MAXIMUM_FILE_SIZE)
	{
		mendicant_runtime_dispose();
		mendicant_models_status(_mendicant_models_status_unreadable, path, NULL);
		return FALSE;
	}

	/* the same file as last time, loaded or refused: as it was */
	if (mendicant_models.stamp_valid && size == mendicant_models.size && stamp[0] == mendicant_models.stamp[0] &&
		stamp[1] == mendicant_models.stamp[1] && !strncmp(path, mendicant_models.path, sizeof(mendicant_models.path) - 1))
	{
		return mendicant_models.loaded;
	}

	mendicant_models_unload();
	mendicant_models.stamp_valid = TRUE;
	mendicant_models.size = size;
	mendicant_models.stamp[0] = stamp[0];
	mendicant_models.stamp[1] = stamp[1];
	strncpy(mendicant_models.path, path, sizeof(mendicant_models.path) - 1);
	mendicant_models.path[sizeof(mendicant_models.path) - 1] = 0;
	/* (a new file is news, whatever came of the last) */
	mendicant_models.status = _mendicant_models_status_none;

	data = (unsigned char *)malloc(size);
	if (!data || !port_mods_file_read(file, 0, data, size))
	{
		free(data);
		mendicant_models_status(_mendicant_models_status_unreadable, path, NULL);
		return FALSE;
	}
	error = mendicant_infer_load(&mendicant_models.weights, data, size, mendicant_doctrine_hash());
	free(data);
	if (error == _mendicant_infer_ok && (mendicant_models.weights.chains != (unsigned long)mendicant_doctrine_chains() ||
		mendicant_models.weights.step_choices != (unsigned long)mendicant_doctrine_step_choices() ||
		mendicant_models.weights.chains > MENDICANT_MODELS_MAXIMUM_CHAINS))
	{
		mendicant_infer_free(&mendicant_models.weights);
		error = _mendicant_infer_error_doctrine;
	}
	if (error != _mendicant_infer_ok)
	{
		mendicant_infer_free(&mendicant_models.weights);
		mendicant_models_status(_mendicant_models_status_refused, path, mendicant_infer_error_string(error));
		return FALSE;
	}
	memset(mendicant_models.chain_covenant, 0, sizeof(mendicant_models.chain_covenant));
	mendicant_doctrine_chain_covenant(mendicant_models.chain_covenant);
	mendicant_models.loaded = TRUE;
	mendicant_models_status(_mendicant_models_status_loaded, path, NULL);
	return TRUE;
}

/* the weights freed: the next load reads the file again */
void mendicant_runtime_dispose(void)
{
	mendicant_models_unload();
	mendicant_models.stamp_valid = FALSE;
	mendicant_models.path[0] = 0;
}

boolean mendicant_runtime_ready(void)
{
	return mendicant_models.loaded;
}

/* brain.py Bridge.serve: answers the request in the bridge, then sets
response_sequence */
void mendicant_runtime_answer(struct model_bridge *bridge)
{
	int count, side;

	if (!bridge)
		return;
	if (!mendicant_models.loaded)
	{
		/* (never asked of a runtime without models: the drill decides then) */
		return;
	}
	count = (int)bridge->member_count;
	if (count < 0)
		count = 0;
	if (count > MENDICANT_INFER_SQUAD)
		count = MENDICANT_INFER_SQUAD;
	side = mendicant_models_side((long)bridge->team_index);

	mendicant_models_strategy(bridge, side);
	mendicant_models_squad(bridge, side, count);

	/* the answer is complete before it is published */
	__sync_synchronize();
	bridge->response_sequence = bridge->request_sequence;
}
