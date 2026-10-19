/*
HOST_RENDER.C

The guest's render batches (port/macos/include/opence_render.h), which it
gives the host one at a time with host_render_submit: the app's Metal
renderer's when it has installed one, else checked by the runtime
(opence_render_check) and dropped, so that the command-line host and the
tests run the guest's renderer with no window.
*/

#include "host.h"
#include "halo_host.h"

#include <stdatomic.h>
#include <stdlib.h>

/* the guest (host_main.c) */
extern OpenceGuest *host_guest;

static struct halo_host_render_backend render_backend;
static int render_backend_set;
static int logging = -1;
static atomic_uint refusals_logged;

void halo_host_set_render_backend(const struct halo_host_render_backend *backend)
{
	render_backend_set = backend != NULL;
	if (backend)
		render_backend = *backend;
}

/* whether a refusal is still logged */
static int log_refusal(void)
{
	unsigned int logged = atomic_fetch_add(&refusals_logged, 1);

	if (logged == HOST_RENDER_LOGGED_REFUSALS)
		host_logf(HOST_LOG_WARN, "render: further refusals are not logged");
	return logged < HOST_RENDER_LOGGED_REFUSALS;
}

static void refused(void *context, uint32_t offset, uint32_t op, const char *reason)
{
	(void)context;
	if (log_refusal())
		host_logf(HOST_LOG_WARN, "render command refused at byte %u (op %u): %s", offset, op, reason);
}

int host_render_submit(unsigned int batch, unsigned int size)
{
	OpenceRenderCheck check = {0};
	char reason[256];

	if (render_backend_set)
		return render_backend.submit(render_backend.context, batch, size);
	if (logging < 0)
		logging = getenv("HALO_HOST_RENDER_LOG") && *getenv("HALO_HOST_RENDER_LOG");
	reason[0] = '\0';
	if (opence_render_check(host_guest, batch, size, &check, reason, sizeof(reason), refused, NULL) !=
		OPENCE_STATUS_OK)
	{
		if (log_refusal())
			host_logf(HOST_LOG_WARN, "render batch refused: %u bytes at 0x%08x: %s", size, batch, reason);
		return -1;
	}
	if (logging)
		fprintf(stderr, "halo_host: render batch: %u commands, %u refused, fnv 0x%08x\n", check.commands,
			check.refused, check.fnv);
	return (int)check.refused;
}
