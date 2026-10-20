/*
HOST_PHYSICS.C

The guest's physics batches (port/macos/include/host_physics.h), which it
gives the host once a tick with host_physics_step: checked and carried out
by the bridge (physics/physics_bridge.c) on one Jolt world, made when the
first batch arrives, which writes back every ragdoll's and body's pose
before the import returns.
*/

#include "host.h"
#include "physics/physics_bridge.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <time.h>

/* the guest (host_main.c) */
extern OpenceGuest *host_guest;

static physics_bridge *bridge;
static int logging = -1;
static atomic_uint refusals_logged;

static void *guest_pointer(void *context, uint32_t address, uint32_t size)
{
	(void)context;
	return opence_guest_host_pointer(host_guest, address, size);
}

/* whether a refusal is still logged */
static int log_refusal(void)
{
	unsigned int logged = atomic_fetch_add(&refusals_logged, 1);

	if (logged == HOST_PHYSICS_LOGGED_REFUSALS)
		host_logf(HOST_LOG_WARN, "physics: further refusals are not logged");
	return logged < HOST_PHYSICS_LOGGED_REFUSALS;
}

static void refused(void *context, uint32_t offset, uint32_t op, const char *reason)
{
	(void)context;
	if (log_refusal())
		host_logf(HOST_LOG_WARN, "physics command refused at byte %u (op %u): %s", offset, op, reason);
}

int host_physics_step(unsigned int batch, unsigned int size, unsigned int results, unsigned int results_size)
{
	physics_bridge_memory memory = {guest_pointer, NULL};
	char reason[256];
	struct timespec began, ended;
	int refusals;

	if (logging < 0)
		logging = getenv("HALO_HOST_PHYSICS_LOG") && *getenv("HALO_HOST_PHYSICS_LOG");
	if (!bridge)
		bridge = physics_bridge_create();
	if (!bridge)
	{
		if (log_refusal())
			host_logf(HOST_LOG_WARN, "physics: the world could not be made");
		return -1;
	}
	reason[0] = '\0';
	clock_gettime(CLOCK_MONOTONIC, &began);
	refusals = physics_bridge_step(bridge, &memory, batch, size, results, results_size, refused, NULL, reason,
		sizeof(reason));
	clock_gettime(CLOCK_MONOTONIC, &ended);
	if (refusals < 0)
	{
		if (log_refusal())
			host_logf(HOST_LOG_WARN, "physics batch refused: %u bytes at 0x%08x: %s", size, batch, reason);
		return -1;
	}
	if (logging)
		/* (and how long the step took, the benchmark's) */
		fprintf(stderr, "halo_host: physics step: %d refused, %u ragdolls, %u bodies, level %d, %.3f ms\n", refusals,
			physics_bridge_ragdolls(bridge), physics_bridge_bodies(bridge), physics_bridge_has_level(bridge),
			(double)(ended.tv_sec - began.tv_sec) * 1000.0 + (double)(ended.tv_nsec - began.tv_nsec) / 1e6);
	return refusals;
}
