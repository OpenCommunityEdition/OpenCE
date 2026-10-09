/*
HOST_VK_VISIBILITY.C

The Vulkan renderer's host half: the game's visibility tests (lens flares), as
occlusion queries. The guest sends VK_COMMAND_VISIBILITY_BEGIN where the game begins a test and VK_COMMAND_VISIBILITY_END (the
game's slot and the pixels each of the game's pixels covers in the target) where it ends it; the draws between are the test's. A query may
not span a rendering's end, so a test takes a group of four query slots (reset together at its beginning, outside rendering) and
each rendering it spans uses one of them (more than four are not counted: said once); the parts are added up.

At the frame's end the results are copied (vkCmdCopyQueryPoolResults, outside rendering, waiting on the queries) into a
host-visible buffer of the frame's slot; when that frame's fence has next passed, each test's samples are divided by the
target's scale area (visibility_unscaled in d3d8_gl.c: the game divides by its own test's area, so a count at the screen's scale
would make flares too bright) and kept as the slot's latest count. host_vk_visibility gives the latest count and never waits, as
d3d8_gl.c's query buffer path: the latest from this test, or from an earlier one while the GPU is behind.
*/

#include "host.h"
#include "host_vk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B host_vkb

#define SLOTS 4096
#define PARTS 4
#define GROUPS (SLOTS / PARTS)

struct test
{
	uint32_t index;
	float area;
	uint32_t group, parts;
};

static struct
{
	VkQueryPool pool;
	int ready;
	/* the test being made */
	int active, open;
	struct test current;
	uint32_t next_group;
	/* each frame slot's tests and the buffer their results are copied into */
	struct test *tests[HOST_VK_FRAMES];
	unsigned count[HOST_VK_FRAMES], capacity[HOST_VK_FRAMES];
	VkBuffer buffer[HOST_VK_FRAMES];
	VkDeviceMemory memory[HOST_VK_FRAMES];
	uint32_t *mapped[HOST_VK_FRAMES];
	/* the latest count of each of the game's slots */
	uint32_t latest[SLOTS];
	unsigned reported_over;
} V;

/* the query pool and each frame slot's result buffer; B.lock is held. Without them the tests report 0 */
void host_vk_visibility_start(void)
{
	VkQueryPoolCreateInfo info = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
	unsigned slot;

	info.queryType = VK_QUERY_TYPE_OCCLUSION;
	info.queryCount = SLOTS;
	if (!HOST_VK_CHECK(vkCreateQueryPool(B.device, &info, NULL, &V.pool)))
		return;
	for (slot = 0; slot < HOST_VK_FRAMES; slot++)
	{
		void *mapped;

		if (!host_vk_buffer_make(SLOTS * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT, &V.buffer[slot], &V.memory[slot],
			&mapped))
			return;
		V.mapped[slot] = mapped;
		memset(mapped, 0, SLOTS * sizeof(uint32_t));
	}
	V.ready = 1;
}

/* a part of the current test is open: end it (before a rendering ends, or at the test's end) */
void host_vk_visibility_close(VkCommandBuffer command)
{
	if (!V.open)
		return;
	vkCmdEndQuery(command, V.pool, V.current.group * PARTS + V.current.parts - 1);
	V.open = 0;
}

/* called with a rendering open, at each draw: a test being made has a query open while its draws are made */
void host_vk_visibility_draw(VkCommandBuffer command)
{
	if (!V.ready || !V.active || V.open)
		return;
	if (V.current.parts >= PARTS)
	{
		if (!V.reported_over++)
			host_logf(HOST_LOG_WARN, "vk: a visibility test spans more than %d renderings; the draws after them are not counted",
				PARTS);
		return;
	}
	vkCmdBeginQuery(command, V.pool, V.current.group * PARTS + V.current.parts,
		B.occlusion_query_precise ? VK_QUERY_CONTROL_PRECISE_BIT : 0);
	V.current.parts++;
	V.open = 1;
}

void host_vk_visibility_begin(void)
{
	VkCommandBuffer command;

	if (!V.ready)
		return;
	if (V.active)
		host_vk_visibility_close(host_vk_frame_command());
	/* the group's queries are reset outside a rendering */
	host_vk_rendering_end();
	command = host_vk_frame_command();
	memset(&V.current, 0, sizeof(V.current));
	V.current.group = V.next_group++ % GROUPS;
	vkCmdResetQueryPool(command, V.pool, V.current.group * PARTS, PARTS);
	V.active = 1;
}

void host_vk_visibility_end(const struct vk_command_visibility_end *end)
{
	unsigned slot = (unsigned)B.frame;

	if (!V.ready || !V.active)
		return;
	host_vk_visibility_close(host_vk_frame_command());
	V.active = 0;
	V.current.index = end->index % SLOTS;
	V.current.area = end->area;
	if (!V.current.parts)
		return;
	if (V.count[slot] == V.capacity[slot])
	{
		unsigned capacity = V.capacity[slot] ? V.capacity[slot] * 2 : 64;
		struct test *grown = realloc(V.tests[slot], sizeof(*grown) * capacity);

		if (!grown)
			return;
		V.tests[slot] = grown;
		V.capacity[slot] = capacity;
	}
	V.tests[slot][V.count[slot]++] = V.current;
}

/* the frame's end, outside a rendering, before it is submitted: the results of the frame's tests are copied */
void host_vk_visibility_frame_end(VkCommandBuffer command)
{
	unsigned slot = (unsigned)B.frame, index;
	VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };

	if (!V.ready)
		return;
	/* a test left open at the frame's end is dropped: its END, in the next frame, finds none (and its count would go to no
	slot the game named) */
	if (V.active)
	{
		host_vk_visibility_close(command);
		V.active = 0;
	}
	if (!V.count[slot])
		return;
	for (index = 0; index < V.count[slot]; index++)
	{
		const struct test *test = &V.tests[slot][index];

		vkCmdCopyQueryPoolResults(command, V.pool, test->group * PARTS, test->parts, V.buffer[slot],
			(VkDeviceSize)test->group * PARTS * sizeof(uint32_t), sizeof(uint32_t), VK_QUERY_RESULT_WAIT_BIT);
	}
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, NULL, 0, NULL);
}

/* the frame slot's fence has passed: its tests' counts, in the game's pixels, are the slots' latest */
void host_vk_visibility_retired(unsigned slot)
{
	unsigned index, part;

	if (!V.ready)
		return;
	for (index = 0; index < V.count[slot]; index++)
	{
		const struct test *test = &V.tests[slot][index];
		uint64_t samples = 0;

		for (part = 0; part < test->parts; part++)
			samples += V.mapped[slot][test->group * PARTS + part];
		/* a count in the target's pixels, to the game's (a screen drawn larger than the game's 480 lines) */
		if (test->area > 1.0f)
			samples = (uint64_t)((float)samples / test->area + 0.5f);
		V.latest[test->index] = samples > 0xffffffffu ? 0xffffffffu : (uint32_t)samples;
	}
	V.count[slot] = 0;
}

/* the guest asks (D3DDevice_GetVisibilityTestResult): never waits */
uint32_t host_vk_visibility(uint32_t index)
{
	uint32_t result;

	pthread_mutex_lock(&B.lock);
	result = V.latest[index % SLOTS];
	pthread_mutex_unlock(&B.lock);
	return result;
}
