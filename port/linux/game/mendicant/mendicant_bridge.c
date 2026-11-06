/*
MENDICANT_BRIDGE.C

The model bridge: struct model_bridge, the request the command layer writes
and the answer the models give, in one of two places.

- In-process (ai.mendicant.bridge empty): a static struct here, answered at
  once by mendicant_runtime_answer (mendicant_runtime.c) when the weights are
  loaded. With no weights there is no bridge (mendicant_bridge is NULL), and
  the Covenant run the drill alone, as the hook library did without brain.py.
- Shared memory (desktop Linux, ai.mendicant.bridge is a path): the file
  trainer.py or brain.py maps, with the layout ai_brain.c had (magic
  0x41494231, 34948 bytes), so both run against this build unchanged. The
  game publishes request_sequence last; the model answers with
  response_sequence last. In training the game runs as fast as it can, so a
  request waits up to 200 ms for the trainer's answer.
*/

#include "cseries.h"
#include "math/real_math.h"

#include <stddef.h>
#include <string.h>

#include "mendicant_internal.h"

/* port/linux/src/platform.h */
void platform_log(char const *format, ...);

/* ---------- constants */

/* (the 32-bit desktop Linux build's: not the 64-bit builds' guest, nor
Windows, Android or the web) */
#if !defined(HALO_WINDOWS) && !defined(HALO_ANDROID) && !defined(HALO_WEB) && !defined(HALO_GUEST)
#define MENDICANT_SHARED_MEMORY 1
#endif

/* how long a training request waits for the trainer's answer */
#define MENDICANT_TRAINING_WAIT_NANOSECONDS 200000000ULL
#define MENDICANT_DEFAULT_EPISODE_TICKS 6000

/* ---------- the layout trainer.py and brain.py read (trainer.py:43-66, brain.py:15-31) */

#define MENDICANT_BRIDGE_CHECK(name, condition) typedef char mendicant_bridge_check_##name[(condition) ? 1 : -1];
MENDICANT_BRIDGE_CHECK(team, offsetof(struct model_bridge, team) == 24)
MENDICANT_BRIDGE_CHECK(candidate_count, offsetof(struct model_bridge, candidate_count) == 56)
MENDICANT_BRIDGE_CHECK(choice, offsetof(struct model_bridge, choice) == 120)
MENDICANT_BRIDGE_CHECK(features, offsetof(struct model_bridge, features) == 184)
MENDICANT_BRIDGE_CHECK(positions, offsetof(struct model_bridge, positions) == 30904)
MENDICANT_BRIDGE_CHECK(team_index, offsetof(struct model_bridge, team_index) == 32440)
MENDICANT_BRIDGE_CHECK(team_mode, offsetof(struct model_bridge, team_mode) == 32444)
MENDICANT_BRIDGE_CHECK(seed, offsetof(struct model_bridge, seed) == 32452)
MENDICANT_BRIDGE_CHECK(plan_features, offsetof(struct model_bridge, plan_features) == 32572)
MENDICANT_BRIDGE_CHECK(plan_request, offsetof(struct model_bridge, plan_request) == 32636)
MENDICANT_BRIDGE_CHECK(plan_extra, offsetof(struct model_bridge, plan_extra) == 32664)
MENDICANT_BRIDGE_CHECK(chief_request_sequence, offsetof(struct model_bridge, chief_request_sequence) == 32792)
MENDICANT_BRIDGE_CHECK(scenario_mode, offsetof(struct model_bridge, scenario_mode) == 32960)
MENDICANT_BRIDGE_CHECK(chains_started, offsetof(struct model_bridge, chains_started) == 33288)
MENDICANT_BRIDGE_CHECK(plan_kind, offsetof(struct model_bridge, plan_kind) == 33312)
MENDICANT_BRIDGE_CHECK(step_mask, offsetof(struct model_bridge, step_mask) == 33316)
MENDICANT_BRIDGE_CHECK(plan_step, offsetof(struct model_bridge, plan_step) == 33324)
MENDICANT_BRIDGE_CHECK(discipline_declined, offsetof(struct model_bridge, discipline_declined) == 33328)
MENDICANT_BRIDGE_CHECK(plan_entities, offsetof(struct model_bridge, plan_entities) == 33344)
MENDICANT_BRIDGE_CHECK(enemy_step_label, offsetof(struct model_bridge, enemy_step_label) == 34880)
MENDICANT_BRIDGE_CHECK(stance, offsetof(struct model_bridge, stance) == 34884)
MENDICANT_BRIDGE_CHECK(size, sizeof(struct model_bridge) == 34948)

/* ---------- platform (port/linux/src/posix_mendicant.c) */

#ifdef MENDICANT_SHARED_MEMORY
void *posix_mendicant_bridge_map(char const *path, unsigned long size);
void posix_mendicant_yield(void);
unsigned long long posix_mendicant_monotonic_ns(void);
#endif

/* ---------- globals */

struct model_bridge *mendicant_bridge = NULL;

/* the in-process bridge: brain.py's place, answered by mendicant_runtime.c */
static struct model_bridge mendicant_local_bridge;
/* the shared memory, mapped once (it stays mapped for the process) */
static struct model_bridge *mendicant_shared_bridge = NULL;
#ifdef MENDICANT_SHARED_MEMORY
static char mendicant_shared_bridge_path[sizeof(mendicant_config.bridge)];
#endif

/* ---------- public code */

/* binds mendicant_bridge for this map (mendicant_initialize_for_new_map):
the shared memory when ai.mendicant.bridge names it, else the in-process
models when their weights are loaded, else none */
void mendicant_bridge_open(
	void)
{
	mendicant_bridge = NULL;

#ifdef MENDICANT_SHARED_MEMORY
	if (mendicant_config.bridge[0])
	{
		if (!mendicant_shared_bridge)
		{
			mendicant_shared_bridge = (struct model_bridge *)posix_mendicant_bridge_map(mendicant_config.bridge,
				(unsigned long)sizeof(struct model_bridge));
			if (mendicant_shared_bridge)
			{
				strcpy(mendicant_shared_bridge_path, mendicant_config.bridge);
				mendicant_shared_bridge->magic = BRIDGE_MAGIC;
				if (!mendicant_shared_bridge->episode_ticks)
					mendicant_shared_bridge->episode_ticks = MENDICANT_DEFAULT_EPISODE_TICKS;
				platform_log("mendicant: model bridge %s", mendicant_config.bridge);
			}
			else
			{
				platform_log("mendicant: cannot open the model bridge %s, using the drill", mendicant_config.bridge);
			}
		}
		else if (strcmp(mendicant_shared_bridge_path, mendicant_config.bridge))
		{
			platform_log("mendicant: the model bridge stays %s until the game restarts", mendicant_shared_bridge_path);
		}
		mendicant_bridge = mendicant_shared_bridge;
		return;
	}
#endif

	if (mendicant_config.models && mendicant_runtime_ready())
	{
		memset(&mendicant_local_bridge, 0, sizeof(mendicant_local_bridge));
		mendicant_local_bridge.magic = BRIDGE_MAGIC;
		mendicant_local_bridge.episode_ticks = MENDICANT_DEFAULT_EPISODE_TICKS;
		mendicant_bridge = &mendicant_local_bridge;
	}

	return;
}

/* the bridge is the shared memory (a trainer or brain.py answers it) */
boolean mendicant_bridge_shared(
	void)
{
	return mendicant_bridge != NULL && mendicant_bridge == mendicant_shared_bridge;
}

/* a request is complete (the command layer has written it): published, and
answered now in-process, or waited for in training */
void mendicant_bridge_submit(
	void)
{
	struct model_bridge *bridge = mendicant_bridge;

	if (!bridge)
		return;

	__sync_synchronize();
	bridge->request_sequence++;

#ifdef MENDICANT_SHARED_MEMORY
	if (bridge == mendicant_shared_bridge)
	{
		if (mendicant_training)
		{
			unsigned long long began = posix_mendicant_monotonic_ns();

			do
			{
				__sync_synchronize();
				if (bridge->response_sequence == bridge->request_sequence)
					break;
				posix_mendicant_yield();
			}
			while (posix_mendicant_monotonic_ns() - began < MENDICANT_TRAINING_WAIT_NANOSECONDS);
		}
		return;
	}
#endif

	mendicant_runtime_answer(bridge);

	return;
}

/* the model has answered the last request */
boolean mendicant_bridge_answered(
	void)
{
	struct model_bridge *bridge = mendicant_bridge;

	if (!bridge)
		return FALSE;
	__sync_synchronize();
	return bridge->response_sequence == bridge->request_sequence;
}
