/*
HOST_VK_RENDER.C

The Vulkan renderer's host half: the logical
device, frames in flight, the game's render targets as images, and the commands
the guest hands over (port/android/guest/vk_commands.h): targets, clears and
present. host_vk_present.c has the surface and the swapchain.

The guest writes a frame's commands into its own memory and calls
host_vk_submit(); everything a command names is read during that call, and a
frame is one command buffer, recorded as the commands come and submitted at
PRESENT. At most one rendering (dynamic rendering) is open at a time. Every
image is transitioned from the layout it is tracked in, with a full barrier
(all commands to all commands): this phase is for being right.
*/

#include "host.h"
#include "host_vk.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct host_vk_backend host_vkb;
void *host_vk_window;
int host_vk_present_marker;
int host_vk_self_test;

#define B host_vkb

#define X(name) PFN_##name name;
HOST_VK_DEVICE_FUNCTIONS(X)
#undef X
PFN_vkCmdBeginRenderingKHR host_vk_cmd_begin_rendering;
PFN_vkCmdEndRenderingKHR host_vk_cmd_end_rendering;

/* how many failed calls are logged: a driver that fails every frame is not logged every frame */
#define ERROR_LOG_LIMIT 40

static uint64_t now_ns(void)
{
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);
	return (uint64_t)time.tv_sec * 1000000000ull + (uint64_t)time.tv_nsec;
}

int host_vk_check(VkResult result, const char *call)
{
	static unsigned logged;

	if (result == VK_SUCCESS)
		return 1;
	if (result == VK_ERROR_DEVICE_LOST)
	{
		if (!B.dead)
			host_logf(HOST_LOG_ERROR, "vk: VK_ERROR_DEVICE_LOST from %s at submission %llu, on %s; the backend stops "
				"(the game goes on without a picture)", call, (unsigned long long)B.submission, host_vk.line);
		B.dead = 1;
		host_vk_presenting = 0;
		return 0;
	}
	if (logged++ < ERROR_LOG_LIMIT)
		host_logf(HOST_LOG_ERROR, "vk: %s returned %d", call, (int)result);
	return 0;
}

/* a wait on a fence that takes more than two seconds says so, then goes on */
void host_vk_wait_fence(VkFence fence, const char *what, uint64_t number)
{
	uint64_t start = now_ns();
	VkResult result = vkWaitForFences(B.device, 1, &fence, VK_TRUE, 2000000000ull);

	if (result == VK_TIMEOUT)
	{
		host_logf(HOST_LOG_WARN, "vk: waiting for %s (submission %llu) has taken more than two seconds; waiting on",
			what, (unsigned long long)number);
		result = vkWaitForFences(B.device, 1, &fence, VK_TRUE, UINT64_MAX);
		host_logf(HOST_LOG_WARN, "vk: %s (submission %llu) was ready after %.1f s", what, (unsigned long long)number,
			(double)(now_ns() - start) / 1e9);
	}
	HOST_VK_CHECK(result);
}

/* GLSL to a shader module (Vulkan 1.0, SPIR-V 1.0), compiled by the backend's glslang (host_vk_shaders.c); VK_NULL_HANDLE after
logging why */
static VkShaderModule shader_module(const char *source, int fragment, const char *what)
{
	VkShaderModule module = VK_NULL_HANDLE;
	uint32_t *words;
	size_t count;
	char message[2048];

	if (!host_vk_glslang_compile(source, fragment, &words, &count, message, sizeof(message)))
	{
		host_logf(HOST_LOG_ERROR, "vk: cannot compile the %s: %s", what, message);
		return VK_NULL_HANDLE;
	}
	{
		VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

		info.codeSize = count * 4;
		info.pCode = words;
		if (!HOST_VK_CHECK(vkCreateShaderModule(B.device, &info, NULL, &module)))
			module = VK_NULL_HANDLE;
	}
	free(words);
	return module;
}

VkShaderModule host_vk_shader_module(const char *source, int fragment, const char *what)
{
	return shader_module(source, fragment, what);
}

/* ---------- frames */

/* B.lock is held: every caller is inside host_vk_submit */
static void retire(uint64_t number)
{
	if (number > B.retired)
		B.retired = number;
}

/* the current frame's command buffer, open for recording: once the GPU is done with the frame last
recorded in this slot (two are in flight) */
VkCommandBuffer host_vk_frame_command(void)
{
	struct host_vk_frame *frame = &B.frames[B.frame];

	if (!frame->recording)
	{
		VkCommandBufferBeginInfo begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

		if (frame->submitted)
		{
			host_vk_wait_fence(frame->fence, "the frame's fence", frame->number);
			retire(frame->number);
			HOST_VK_CHECK(vkResetFences(B.device, 1, &frame->fence));
			frame->submitted = 0;
		}
		host_vk_draw_frame_reset(frame);
		host_vk_visibility_retired((unsigned)(frame - B.frames));
		HOST_VK_CHECK(vkResetCommandPool(B.device, frame->pool, 0));
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (HOST_VK_CHECK(vkBeginCommandBuffer(frame->command, &begin)))
			frame->recording = 1;
	}
	return frame->command;
}

/* ends and submits the frame, numbered; wait_image is the swapchain image acquired for it (or UINT32_MAX) */
static void frame_submit(uint32_t wait_image)
{
	struct host_vk_frame *frame = &B.frames[B.frame];
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

	if (!frame->recording)
		return;
	frame->recording = 0;
	if (!HOST_VK_CHECK(vkEndCommandBuffer(frame->command)))
		return;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &frame->command;
	if (wait_image != UINT32_MAX)
	{
		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &frame->acquired;
		submit.pWaitDstStageMask = &wait_stage;
		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &B.chain.render_done[wait_image];
	}
	frame->number = B.submission + 1;
	if (!HOST_VK_CHECK(vkQueueSubmit(B.queue, 1, &submit, frame->fence)))
		return;
	B.submission = frame->number;
	frame->submitted = 1;
}

/* submits the frame as it is and waits for it: for the self-tests, which read what the GPU wrote back; 0 if it
could not be submitted */
static int frame_submit_and_wait(const char *what)
{
	struct host_vk_frame *frame = &B.frames[B.frame];

	frame_submit(UINT32_MAX);
	if (!frame->submitted)
		return 0;
	host_vk_wait_fence(frame->fence, what, frame->number);
	retire(frame->number);
	HOST_VK_CHECK(vkResetFences(B.device, 1, &frame->fence));
	frame->submitted = 0;
	return !B.dead;
}

/* the highest submission number whose fence has signalled; B.lock is held */
static uint32_t retired_locked(void)
{
	int index;

	if (B.state == 1 && !B.dead)
	{
		/* the fences tell without waiting; the latest frame is the one that can have been signalled last */
		for (index = 0; index < HOST_VK_FRAMES; index++)
		{
			struct host_vk_frame *frame = &B.frames[index];

			if (frame->submitted && frame->number > B.retired && vkGetFenceStatus(B.device, frame->fence) == VK_SUCCESS)
				B.retired = frame->number;
		}
	}
	return (uint32_t)B.retired;
}

/* the guest asks (its locks); the fences tell without waiting */
uint32_t host_vk_retired(void)
{
	uint32_t result;

	pthread_mutex_lock(&B.lock);
	result = retired_locked();
	pthread_mutex_unlock(&B.lock);
	return result;
}

/* ---------- the game's render targets */

static void barrier_image(VkCommandBuffer command, VkImage image, VkImageAspectFlags aspect, VkImageLayout from,
	VkImageLayout to, VkPipelineStageFlags destination_stage, VkAccessFlags destination_access)
{
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

	barrier.srcAccessMask = from == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = destination_access;
	barrier.oldLayout = from;
	barrier.newLayout = to;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, destination_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

/* the target goes to a layout, from the one it is in: always a barrier, even to the same layout, because a
rendering's writes are not visible to the next without one */
void host_vk_target_transition(VkCommandBuffer command, struct host_vk_target *target, VkImageLayout layout)
{
	barrier_image(command, target->image, target->aspect, target->layout, layout, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
	target->layout = layout;
}

static unsigned bucket_of(uint32_t data)
{
	return (data >> 4) % HOST_VK_TARGET_BUCKETS;
}

uint32_t host_vk_memory_type(uint32_t bits, VkMemoryPropertyFlags preferred)
{
	uint32_t index, fallback = UINT32_MAX;

	for (index = 0; index < B.memory.memoryTypeCount; index++)
	{
		if (!(bits & (1u << index)))
			continue;
		if ((B.memory.memoryTypes[index].propertyFlags & preferred) == preferred)
			return index;
		if (fallback == UINT32_MAX)
			fallback = index;
	}
	return fallback;
}

static VkImageUsageFlags color_usage, depth_usage;

/* the first time a target is drawn into it is cleared (black; depth 1, stencil 0), so that a target read before
anything is drawn into it reads the same on every driver */
static void target_clear_first(struct host_vk_target *target)
{
	VkCommandBuffer command = host_vk_frame_command();
	VkRenderingAttachmentInfo attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	VkRenderingAttachmentInfo stencil = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	VkRenderingInfo info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
	int depth = target->key.kind == VK_SURFACE_DEPTH;

	host_vk_target_transition(command, target, depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	attachment.imageView = target->view;
	attachment.imageLayout = target->layout;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	if (depth)
	{
		attachment.clearValue.depthStencil.depth = 1.0f;
		attachment.clearValue.depthStencil.stencil = 0;
		stencil = attachment;
		info.pDepthAttachment = &attachment;
		info.pStencilAttachment = &stencil;
	}
	else
	{
		info.colorAttachmentCount = 1;
		info.pColorAttachments = &attachment;
	}
	info.renderArea.extent.width = target->key.pixel_width;
	info.renderArea.extent.height = target->key.pixel_height;
	info.layerCount = 1;
	host_vk_cmd_begin_rendering(command, &info);
	host_vk_cmd_end_rendering(command);
}

/* the image for a surface, made (and cleared) the first time it is named; NULL if there is none or none can be made */
struct host_vk_target *host_vk_target_get(const struct vk_surface *surface)
{
	struct host_vk_target *target;
	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	VkImageViewCreateInfo view = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocation = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	unsigned bucket;
	int depth;

	if ((surface->kind != VK_SURFACE_COLOR && surface->kind != VK_SURFACE_DEPTH) || !surface->pixel_width ||
		!surface->pixel_height)
		return NULL;
	bucket = bucket_of(surface->data);
	for (target = B.buckets[bucket]; target; target = target->next_in_bucket)
	{
		if (!memcmp(&target->key, surface, sizeof(*surface)))
			return target;
	}
	depth = surface->kind == VK_SURFACE_DEPTH;
	if (surface->pixel_width > host_vk.properties.limits.maxImageDimension2D ||
		surface->pixel_height > host_vk.properties.limits.maxImageDimension2D)
	{
		host_logf(HOST_LOG_ERROR, "vk: the %ux%u target at %08x is larger than the device's images; it is not drawn into",
			(unsigned)surface->pixel_width, (unsigned)surface->pixel_height, (unsigned)surface->data);
		return NULL;
	}
	target = calloc(1, sizeof(*target));
	if (!target)
		return NULL;
	target->key = *surface;
	target->layout = VK_IMAGE_LAYOUT_UNDEFINED;
	target->aspect = depth ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = depth ? B.depth_format : B.color_format;
	info.extent.width = surface->pixel_width;
	info.extent.height = surface->pixel_height;
	info.extent.depth = 1;
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = depth ? depth_usage : color_usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (!HOST_VK_CHECK(vkCreateImage(B.device, &info, NULL, &target->image)))
		goto fail;
	vkGetImageMemoryRequirements(B.device, target->image, &requirements);
	allocation.allocationSize = requirements.size;
	allocation.memoryTypeIndex = host_vk_memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocation.memoryTypeIndex == UINT32_MAX)
	{
		host_logf(HOST_LOG_ERROR, "vk: no memory type for the %ux%u target at %08x", (unsigned)surface->pixel_width,
			(unsigned)surface->pixel_height, (unsigned)surface->data);
		goto fail;
	}
	if (!HOST_VK_CHECK(vkAllocateMemory(B.device, &allocation, NULL, &target->memory)) ||
		!HOST_VK_CHECK(vkBindImageMemory(B.device, target->image, target->memory, 0)))
		goto fail;
	view.image = target->image;
	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.format = info.format;
	view.subresourceRange.aspectMask = target->aspect;
	view.subresourceRange.levelCount = 1;
	view.subresourceRange.layerCount = 1;
	if (!HOST_VK_CHECK(vkCreateImageView(B.device, &view, NULL, &target->view)))
		goto fail;
	target->next_in_bucket = B.buckets[bucket];
	B.buckets[bucket] = target;
	B.images++;
	host_logf(HOST_LOG_INFO, "vk: %s target %08x, %ux%u (the game's %ux%u), image %u", depth ? "depth" : "colour",
		(unsigned)surface->data, (unsigned)surface->pixel_width, (unsigned)surface->pixel_height,
		(unsigned)surface->width, (unsigned)surface->height, B.images);
	/* the clear is recorded outside any rendering: the one open is the game's, and is ended first */
	host_vk_rendering_end();
	target_clear_first(target);
	return target;
fail:
	if (target->view)
		vkDestroyImageView(B.device, target->view, NULL);
	if (target->memory)
		vkFreeMemory(B.device, target->memory, NULL);
	if (target->image)
		vkDestroyImage(B.device, target->image, NULL);
	free(target);
	return NULL;
}

/* ---------- multisampling

With display.anti_aliasing's multisampling the guest names the samples a pixel of the targets it binds
(vk_command_targets): the screen's, and any target drawn together with one of them, from then on (d3d8_vk.c). Such a
target gets a multisampled image beside its own, which its renderings draw into; the colour's samples are resolved into
the target's image as each rendering ends, so that everything else - a draw sampling the target, the mip composites, the
anti-aliasing pass, the present - reads the target's image as before. A depth buffer is not resolved: nothing samples a
depth buffer, and the game clears the screen's at the start of each frame. A multisampled image is cleared when it is
made (black; depth 1, stencil 0), as the OpenGL ES renderer clears its multisampled renderbuffers: the setting changes
between frames, and the frame clears the screen's targets before it draws. */

uint32_t host_vk_samples_usable(uint32_t samples)
{
	const VkPhysicalDeviceLimits *limits = &host_vk.properties.limits;
	VkSampleCountFlags counts = limits->framebufferColorSampleCounts & limits->framebufferDepthSampleCounts &
		limits->framebufferStencilSampleCounts;
	uint32_t usable;

	for (usable = 8; usable > 1; usable /= 2)
	{
		if (usable <= samples && (counts & usable))
			return usable;
	}
	return 1;
}

uint32_t host_vk_samples_supported(uint32_t samples)
{
	if (!host_vk.instance || !host_vk.physical)
		return 1;
	return host_vk_samples_usable(samples);
}

/* multisampled images let go, and the submission after which nothing uses them */
#define RETIRED_MULTISAMPLES 64

static struct
{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	uint64_t after;
} retired_multisamples[RETIRED_MULTISAMPLES];
static unsigned retired_multisample_count;

static uint32_t retired_locked(void);

/* those the GPU is done with are destroyed (all of them, when the device is idle: force) */
static void multisamples_destroy_retired(int force)
{
	unsigned index, kept = 0;
	uint64_t retired = force ? UINT64_MAX : retired_locked();

	for (index = 0; index < retired_multisample_count; index++)
	{
		if (retired_multisamples[index].after <= retired)
		{
			vkDestroyImageView(B.device, retired_multisamples[index].view, NULL);
			vkDestroyImage(B.device, retired_multisamples[index].image, NULL);
			vkFreeMemory(B.device, retired_multisamples[index].memory, NULL);
		}
		else
		{
			retired_multisamples[kept++] = retired_multisamples[index];
		}
	}
	retired_multisample_count = kept;
}

/* the target's multisampled image is let go once the frame being recorded, which may use it, has passed */
static void multisample_retire(struct host_vk_target *target)
{
	if (!target->samples)
		return;
	multisamples_destroy_retired(0);
	if (retired_multisample_count == RETIRED_MULTISAMPLES)
	{
		HOST_VK_CHECK(vkDeviceWaitIdle(B.device));
		multisamples_destroy_retired(1);
	}
	retired_multisamples[retired_multisample_count].image = target->multisample_image;
	retired_multisamples[retired_multisample_count].memory = target->multisample_memory;
	retired_multisamples[retired_multisample_count].view = target->multisample_view;
	retired_multisamples[retired_multisample_count].after = B.submission + 1;
	retired_multisample_count++;
	target->multisample_image = VK_NULL_HANDLE;
	target->multisample_memory = VK_NULL_HANDLE;
	target->multisample_view = VK_NULL_HANDLE;
	target->samples = 0;
}

/* the multisampled image's transition to a layout (outside a rendering), as host_vk_target_transition */
static void multisample_transition(VkCommandBuffer command, struct host_vk_target *target, VkImageLayout layout)
{
	barrier_image(command, target->multisample_image, target->aspect, target->multisample_layout, layout,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
	target->multisample_layout = layout;
}

/* the target drawn with samples a pixel from now on (0 or 1: into its own image): its multisampled image made (and
cleared) or let go. 0 if one could not be made (the target is then drawn into as it is) */
static int multisample_set(struct host_vk_target *target, uint32_t samples)
{
	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	VkImageViewCreateInfo view = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocation = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	VkRenderingAttachmentInfo attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	VkRenderingAttachmentInfo stencil = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	VkRenderingInfo rendering = { VK_STRUCTURE_TYPE_RENDERING_INFO };
	VkCommandBuffer command;
	int depth = target->key.kind == VK_SURFACE_DEPTH;

	if (samples < 2)
		samples = 0;
	if (target->samples == samples)
		return 1;
	host_vk_rendering_end();
	multisample_retire(target);
	if (!samples)
		return 1;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = depth ? B.depth_format : B.color_format;
	info.extent.width = target->key.pixel_width;
	info.extent.height = target->key.pixel_height;
	info.extent.depth = 1;
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = (VkSampleCountFlagBits)samples;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = depth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (!HOST_VK_CHECK(vkCreateImage(B.device, &info, NULL, &target->multisample_image)))
		goto fail;
	vkGetImageMemoryRequirements(B.device, target->multisample_image, &requirements);
	allocation.allocationSize = requirements.size;
	allocation.memoryTypeIndex = host_vk_memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocation.memoryTypeIndex == UINT32_MAX ||
		!HOST_VK_CHECK(vkAllocateMemory(B.device, &allocation, NULL, &target->multisample_memory)) ||
		!HOST_VK_CHECK(vkBindImageMemory(B.device, target->multisample_image, target->multisample_memory, 0)))
		goto fail;
	view.image = target->multisample_image;
	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.format = info.format;
	view.subresourceRange.aspectMask = target->aspect;
	view.subresourceRange.levelCount = 1;
	view.subresourceRange.layerCount = 1;
	if (!HOST_VK_CHECK(vkCreateImageView(B.device, &view, NULL, &target->multisample_view)))
		goto fail;
	target->samples = samples;
	target->multisample_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	host_logf(HOST_LOG_INFO, "vk: %s target %08x, %ux%u, drawn with %u samples a pixel", depth ? "depth" : "colour",
		(unsigned)target->key.data, (unsigned)target->key.pixel_width, (unsigned)target->key.pixel_height, (unsigned)samples);

	/* cleared, as a target is when it is made */
	command = host_vk_frame_command();
	multisample_transition(command, target, depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	attachment.imageView = target->multisample_view;
	attachment.imageLayout = target->multisample_layout;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	if (depth)
	{
		attachment.clearValue.depthStencil.depth = 1.0f;
		stencil = attachment;
		rendering.pDepthAttachment = &attachment;
		rendering.pStencilAttachment = &stencil;
	}
	else
	{
		rendering.colorAttachmentCount = 1;
		rendering.pColorAttachments = &attachment;
	}
	rendering.renderArea.extent.width = target->key.pixel_width;
	rendering.renderArea.extent.height = target->key.pixel_height;
	rendering.layerCount = 1;
	host_vk_cmd_begin_rendering(command, &rendering);
	host_vk_cmd_end_rendering(command);
	return 1;
fail:
	host_logf(HOST_LOG_ERROR, "vk: the %ux%u target at %08x cannot be drawn with %u samples a pixel; drawn with one",
		(unsigned)target->key.pixel_width, (unsigned)target->key.pixel_height, (unsigned)target->key.data, (unsigned)samples);
	if (target->multisample_view)
		vkDestroyImageView(B.device, target->multisample_view, NULL);
	if (target->multisample_memory)
		vkFreeMemory(B.device, target->multisample_memory, NULL);
	if (target->multisample_image)
		vkDestroyImage(B.device, target->multisample_image, NULL);
	target->multisample_image = VK_NULL_HANDLE;
	target->multisample_memory = VK_NULL_HANDLE;
	target->multisample_view = VK_NULL_HANDLE;
	target->samples = 0;
	return 0;
}

/* ---------- rendering */

void host_vk_rendering_end(void)
{
	if (B.rendering)
	{
		/* a query may not span the rendering's end */
		host_vk_visibility_close(B.frames[B.frame].command);
		host_vk_cmd_end_rendering(B.frames[B.frame].command);
		B.rendering = 0;
	}
}

/* opens a rendering on the current targets, loading what they hold and storing what is drawn; 0 if there are
none. Viewport and scissor are set after every opening: Vulkan keeps no state across renderings. */
int host_vk_rendering_begin(void)
{
	VkCommandBuffer command;
	VkRenderingAttachmentInfo color = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	VkRenderingAttachmentInfo depth = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	VkRenderingAttachmentInfo stencil = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	VkRenderingInfo info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
	VkViewport viewport;
	VkRect2D scissor;
	uint32_t width = UINT32_MAX, height = UINT32_MAX;

	if (B.rendering)
		return 1;
	if (!B.color && !B.depth)
		return 0;
	command = host_vk_frame_command();
	if (B.color)
	{
		host_vk_target_transition(command, B.color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		color.imageView = B.color->view;
		color.imageLayout = B.color->layout;
		color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		/* multisampled: drawn into its samples, kept for the next rendering, and resolved into the target's image as
		this one ends */
		if (B.samples)
		{
			multisample_transition(command, B.color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			color.imageView = B.color->multisample_view;
			color.imageLayout = B.color->multisample_layout;
			color.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
			color.resolveImageView = B.color->view;
			color.resolveImageLayout = B.color->layout;
		}
		info.colorAttachmentCount = 1;
		info.pColorAttachments = &color;
		width = B.color->key.pixel_width;
		height = B.color->key.pixel_height;
	}
	if (B.depth)
	{
		host_vk_target_transition(command, B.depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		depth.imageView = B.depth->view;
		depth.imageLayout = B.depth->layout;
		depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		if (B.samples)
		{
			multisample_transition(command, B.depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			depth.imageView = B.depth->multisample_view;
			depth.imageLayout = B.depth->multisample_layout;
		}
		stencil = depth;
		info.pDepthAttachment = &depth;
		info.pStencilAttachment = &stencil;
		/* the render area is within every attachment */
		if (B.depth->key.pixel_width < width)
			width = B.depth->key.pixel_width;
		if (B.depth->key.pixel_height < height)
			height = B.depth->key.pixel_height;
	}
	info.renderArea.extent.width = width;
	info.renderArea.extent.height = height;
	info.layerCount = 1;
	host_vk_cmd_begin_rendering(command, &info);
	B.rendering = 1;
	B.area_width = width;
	B.area_height = height;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)width;
	viewport.height = (float)height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = width;
	scissor.extent.height = height;
	vkCmdSetViewport(command, 0, 1, &viewport);
	vkCmdSetScissor(command, 0, 1, &scissor);
	return 1;
}

/* the built-in pipeline of a clear of some channels only: one per colour write mask (and per whether a depth buffer
is bound, since a pipeline must match the rendering's attachments), made the first time. Its vertex shader makes a
triangle that covers the viewport from gl_VertexIndex; the scissor makes the rectangle; the colour is a push constant. */
static VkPipeline clear_pipeline(uint32_t mask, int has_depth, uint32_t samples)
{
	VkPipeline *slot = &B.clear_pipelines[mask][has_depth != 0][samples >= 8 ? 3 : samples >= 4 ? 2 : samples >= 2 ? 1 : 0];
	VkPipelineShaderStageCreateInfo stages[2] = { { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
		{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO } };
	VkPipelineVertexInputStateCreateInfo vertex_input = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VkPipelineInputAssemblyStateCreateInfo assembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	VkPipelineViewportStateCreateInfo viewport = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	VkPipelineRasterizationStateCreateInfo raster = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	VkPipelineMultisampleStateCreateInfo multisample = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	VkPipelineDepthStencilStateCreateInfo depth_stencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	VkPipelineColorBlendAttachmentState blend_attachment;
	VkPipelineColorBlendStateCreateInfo blend = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	VkPipelineRenderingCreateInfo rendering = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	VkGraphicsPipelineCreateInfo info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	if (*slot)
		return *slot;
	if (!B.clear_vertex || !B.clear_fragment)
		return VK_NULL_HANDLE;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = B.clear_vertex;
	stages[0].pName = "main";
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = B.clear_fragment;
	stages[1].pName = "main";
	assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	viewport.viewportCount = 1;
	viewport.scissorCount = 1;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = VK_CULL_MODE_NONE;
	raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	raster.lineWidth = 1.0f;
	multisample.rasterizationSamples = samples > 1 ? (VkSampleCountFlagBits)samples : VK_SAMPLE_COUNT_1_BIT;
	/* depth and stencil untouched: tests and writes off */
	memset(&blend_attachment, 0, sizeof(blend_attachment));
	blend_attachment.colorWriteMask = mask;
	blend.attachmentCount = 1;
	blend.pAttachments = &blend_attachment;
	dynamic.dynamicStateCount = 2;
	dynamic.pDynamicStates = dynamic_states;
	rendering.colorAttachmentCount = 1;
	rendering.pColorAttachmentFormats = &B.color_format;
	rendering.depthAttachmentFormat = has_depth ? B.depth_format : VK_FORMAT_UNDEFINED;
	rendering.stencilAttachmentFormat = has_depth ? B.depth_format : VK_FORMAT_UNDEFINED;
	info.pNext = &rendering;
	info.stageCount = 2;
	info.pStages = stages;
	info.pVertexInputState = &vertex_input;
	info.pInputAssemblyState = &assembly;
	info.pViewportState = &viewport;
	info.pRasterizationState = &raster;
	info.pMultisampleState = &multisample;
	info.pDepthStencilState = &depth_stencil;
	info.pColorBlendState = &blend;
	info.pDynamicState = &dynamic;
	info.layout = B.clear_layout;
	info.basePipelineIndex = -1;
	if (!HOST_VK_CHECK(vkCreateGraphicsPipelines(B.device, VK_NULL_HANDLE, 1, &info, NULL, slot)))
		*slot = VK_NULL_HANDLE;
	return *slot;
}

static const char *const clear_vertex_source =
	"#version 450\n"
	"void main()\n"
	"{\n"
	"\tvec2 p = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));\n"
	"\tgl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n"
	"}\n";

static const char *const clear_fragment_source =
	"#version 450\n"
	"layout(push_constant) uniform Colour { vec4 colour; } c;\n"
	"layout(location = 0) out vec4 out_colour;\n"
	"void main()\n"
	"{\n"
	"\tout_colour = c.colour;\n"
	"}\n";

/* ---------- commands */

static void command_targets(const struct vk_command_targets *command)
{
	struct host_vk_target *color = NULL, *depth = NULL;
	uint32_t samples = command->samples > 1 ? host_vk_samples_usable(command->samples) : 0;

	/* a colour surface that is not a colour format, or a depth one that is not a depth format, is not bound */
	if (command->color.kind == VK_SURFACE_COLOR)
		color = host_vk_target_get(&command->color);
	if (command->depth.kind == VK_SURFACE_DEPTH)
		depth = host_vk_target_get(&command->depth);
	/* multisampled: both are, or neither (a rendering's attachments have one count) */
	if (samples < 2)
		samples = 0;
	if (samples && ((color && !multisample_set(color, samples)) || (depth && !multisample_set(depth, samples))))
		samples = 0;
	if (!samples)
	{
		if (color)
			multisample_set(color, 0);
		if (depth)
			multisample_set(depth, 0);
	}
	/* a change is a pair that differs from the one last named (the guest names its targets again after each present) */
	if (color != B.last_color || depth != B.last_depth)
		B.counts.target_changes++;
	if (color)
		color->bound = ++B.target_clock;
	B.last_color = color;
	B.last_depth = depth;
	if (color != B.color || depth != B.depth || samples != B.samples)
	{
		host_vk_rendering_end();
		B.color = color;
		B.depth = depth;
		B.samples = samples;
	}
	B.counts.targets++;
}

static void command_clear(const struct vk_command_clear *command, uint32_t size)
{
	uint32_t mask = command->flags & 0xf;
	int depth = B.depth && (command->flags & VK_CLEAR_DEPTH);
	int stencil = B.depth && (command->flags & VK_CLEAR_STENCIL);
	int whole = mask == 0xf, partial;
	uint32_t index, kept = 0, count = command->rectangle_count;
	VkClearAttachment attachments[2];
	uint32_t attachment_count = 0;
	VkClearRect *rectangles;
	VkCommandBuffer cmd;

	B.counts.clears++;
	if (!B.color)
		mask = whole = 0;
	if (!mask && !depth && !stencil)
		return;
	if (sizeof(*command) + (size_t)count * 4 * sizeof(uint32_t) > size)
	{
		host_logf(HOST_LOG_ERROR, "vk: a clear of %u rectangles does not fit its %u bytes; it is dropped", (unsigned)count,
			(unsigned)size);
		return;
	}
	partial = mask != 0 && !whole;
	if (partial && !clear_pipeline(mask, B.depth != NULL, B.samples))
	{
		/* (glslang or the driver failed: said when it did) the depth and stencil parts are still cleared */
		partial = 0;
		mask = 0;
		if (!depth && !stencil)
			return;
	}
	if (!count || !host_vk_rendering_begin())
		return;
	cmd = host_vk_frame_command();
	rectangles = malloc(sizeof(*rectangles) * count);
	if (!rectangles)
		return;
	/* the rectangles, within the render area (a rectangle outside it is a fault in the guest's arithmetic, which
	would be a validation error; the part inside is cleared) */
	for (index = 0; index < count; index++)
	{
		const uint32_t *rectangle = command->rectangles[index];
		int64_t x0 = (int32_t)rectangle[0], y0 = (int32_t)rectangle[1];
		int64_t x1 = x0 + (int32_t)rectangle[2], y1 = y0 + (int32_t)rectangle[3];

		if (x0 < 0)
			x0 = 0;
		if (y0 < 0)
			y0 = 0;
		if (x1 > (int64_t)B.area_width)
			x1 = B.area_width;
		if (y1 > (int64_t)B.area_height)
			y1 = B.area_height;
		if (x0 >= x1 || y0 >= y1)
			continue;
		rectangles[kept].rect.offset.x = (int32_t)x0;
		rectangles[kept].rect.offset.y = (int32_t)y0;
		rectangles[kept].rect.extent.width = (uint32_t)(x1 - x0);
		rectangles[kept].rect.extent.height = (uint32_t)(y1 - y0);
		rectangles[kept].baseArrayLayer = 0;
		rectangles[kept].layerCount = 1;
		kept++;
	}
	if (!kept)
	{
		free(rectangles);
		return;
	}
	memset(attachments, 0, sizeof(attachments));
	if (whole)
	{
		attachments[attachment_count].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		attachments[attachment_count].colorAttachment = 0;
		attachments[attachment_count].clearValue.color.float32[0] = command->color[0];
		attachments[attachment_count].clearValue.color.float32[1] = command->color[1];
		attachments[attachment_count].clearValue.color.float32[2] = command->color[2];
		attachments[attachment_count].clearValue.color.float32[3] = command->color[3];
		attachment_count++;
	}
	if (depth || stencil)
	{
		float z = command->depth < 0.0f ? 0.0f : command->depth > 1.0f ? 1.0f : command->depth;

		/* (not-a-number is 0 here: the comparisons above leave it as it is, so test it) */
		if (z != z)
			z = 0.0f;
		attachments[attachment_count].aspectMask = (depth ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
			(stencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
		attachments[attachment_count].clearValue.depthStencil.depth = z;
		attachments[attachment_count].clearValue.depthStencil.stencil = command->stencil & 0xff;
		attachment_count++;
	}
	if (attachment_count)
	{
		vkCmdClearAttachments(cmd, attachment_count, attachments, kept, rectangles);
		B.counts.clears_attachments++;
	}
	if (partial)
	{
		/* vkCmdClearAttachments ignores colour write masks, so a clear of some channels is drawn: the pipeline
		writes only them */
		VkPipeline pipeline = clear_pipeline(mask, B.depth != NULL, B.samples);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdPushConstants(cmd, B.clear_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16, command->color);
		for (index = 0; index < kept; index++)
		{
			vkCmdSetScissor(cmd, 0, 1, &rectangles[index].rect);
			vkCmdDraw(cmd, 3, 1, 0, 0);
		}
		{
			VkRect2D scissor = { { 0, 0 }, { B.area_width, B.area_height } };

			vkCmdSetScissor(cmd, 0, 1, &scissor);
		}
		B.counts.clears_drawn++;
	}
	free(rectangles);
}

/* ---------- the clears' self-test (with debug.vk_present_marker): the game's clears are black, and the fog's clear of
alpha only did not occur in the first map's opening, so the clears are tried on a small target of their own and read back:
a full clear, then a clear of alpha only on the left half and of red only on the top right quarter. The picture is
the same on every driver or one of them is wrong. */

static int near(unsigned char value, int wanted)
{
	return value >= wanted - 1 && value <= wanted + 1;
}

static void clears_selftest(void)
{
	struct vk_surface color_surface = { 0xfffffff0u, 8, 8, 8, 8, VK_SURFACE_COLOR };
	struct vk_surface depth_surface = { 0xfffffff1u, 8, 8, 8, 8, VK_SURFACE_DEPTH };
	struct
	{
		struct vk_command_targets targets;
		struct { struct vk_command_clear header; uint32_t rectangles[1][4]; } clear;
	} commands;
	VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	VkMemoryAllocateInfo allocation = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	VkMemoryRequirements requirements;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkBufferImageCopy copy;
	VkCommandBuffer command;
	struct host_vk_target *color;
	unsigned char *pixels = NULL;
	int bad = 0, x, y;
	uint32_t type;

	memset(&commands, 0, sizeof(commands));
	commands.targets.color = color_surface;
	commands.targets.depth = depth_surface;
	command_targets(&commands.targets);
	color = B.color;
	if (!color)
	{
		host_logf(HOST_LOG_ERROR, "vk: clears self-test: FAILED (no target)");
		return;
	}
	/* a full clear of all four channels, with depth and stencil */
	commands.clear.header.flags = VK_CLEAR_RED | VK_CLEAR_GREEN | VK_CLEAR_BLUE | VK_CLEAR_ALPHA | VK_CLEAR_DEPTH | VK_CLEAR_STENCIL;
	commands.clear.header.color[0] = 0.25f;
	commands.clear.header.color[1] = 0.5f;
	commands.clear.header.color[2] = 0.75f;
	commands.clear.header.color[3] = 1.0f;
	commands.clear.header.depth = 0.5f;
	commands.clear.header.stencil = 7;
	commands.clear.header.rectangle_count = 1;
	commands.clear.rectangles[0][2] = 8;
	commands.clear.rectangles[0][3] = 8;
	command_clear(&commands.clear.header, sizeof(commands.clear));
	/* alpha only, left half */
	commands.clear.header.flags = VK_CLEAR_ALPHA;
	commands.clear.header.color[3] = 0.5f;
	commands.clear.rectangles[0][2] = 4;
	command_clear(&commands.clear.header, sizeof(commands.clear));
	/* red only, top right quarter, with a depth clear in the same command */
	commands.clear.header.flags = VK_CLEAR_RED | VK_CLEAR_DEPTH;
	commands.clear.header.color[0] = 1.0f;
	commands.clear.rectangles[0][0] = 4;
	commands.clear.rectangles[0][2] = 4;
	commands.clear.rectangles[0][3] = 4;
	command_clear(&commands.clear.header, sizeof(commands.clear));
	host_vk_rendering_end();
	B.color = B.depth = NULL;

	command = host_vk_frame_command();
	buffer_info.size = 8 * 8 * 4;
	buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (!HOST_VK_CHECK(vkCreateBuffer(B.device, &buffer_info, NULL, &buffer)))
		goto done;
	vkGetBufferMemoryRequirements(B.device, buffer, &requirements);
	type = host_vk_memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (type == UINT32_MAX || !(B.memory.memoryTypes[type].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
	{
		host_logf(HOST_LOG_ERROR, "vk: clears self-test: no host-visible memory");
		goto done;
	}
	allocation.allocationSize = requirements.size;
	allocation.memoryTypeIndex = type;
	if (!HOST_VK_CHECK(vkAllocateMemory(B.device, &allocation, NULL, &memory)) ||
		!HOST_VK_CHECK(vkBindBufferMemory(B.device, buffer, memory, 0)))
		goto done;
	host_vk_target_transition(command, color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	memset(&copy, 0, sizeof(copy));
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.layerCount = 1;
	copy.imageExtent.width = 8;
	copy.imageExtent.height = 8;
	copy.imageExtent.depth = 1;
	vkCmdCopyImageToBuffer(command, color->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &copy);
	{
		VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
		vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, NULL, 0, NULL);
	}
	if (!frame_submit_and_wait("the clears self-test"))
		goto done;
	if (!HOST_VK_CHECK(vkMapMemory(B.device, memory, 0, VK_WHOLE_SIZE, 0, (void **)&pixels)))
		goto done;
	for (y = 0; y < 8; y++)
	{
		for (x = 0; x < 8; x++)
		{
			const unsigned char *texel = pixels + (y * 8 + x) * 4; /* B, G, R, A */
			int red = (x >= 4 && y < 4) ? 255 : 64, alpha = x < 4 ? 128 : 255;

			if (!near(texel[0], 191) || !near(texel[1], 128) || !near(texel[2], red) || !near(texel[3], alpha))
			{
				if (bad++ < 4)
					host_logf(HOST_LOG_ERROR, "vk: clears self-test: texel (%d,%d) is B%u G%u R%u A%u, wanted B191 G127-128 R%d A%d",
						x, y, texel[0], texel[1], texel[2], texel[3], red, alpha);
			}
		}
	}
	vkUnmapMemory(B.device, memory);
	host_logf(bad ? HOST_LOG_ERROR : HOST_LOG_INFO, "vk: clears self-test: %s (a full clear, alpha only on the left half, red only on the "
		"top right quarter; %u partial clears drawn, %u by vkCmdClearAttachments)", bad ? "FAILED" : "ok", B.counts.clears_drawn,
		B.counts.clears_attachments);
done:
	if (buffer)
		vkDestroyBuffer(B.device, buffer, NULL);
	if (memory)
		vkFreeMemory(B.device, memory, NULL);
	memset(&B.counts, 0, sizeof(B.counts));
}

/* ---------- the data self-test (with debug.vk_self_test): VK_COMMAND_TEST_DRAW draws three vertices of the frame's data
into a 16x16 target of the host's own with a built-in pipeline (a position and a colour), reads it back, and says whether
every texel is the colour expected. The guest puts a buffer, rewrites it in place and puts it again, and then a third
larger than its stream: the first two coming out red and green prove the copies were made at the puts, the third that parts
of one id land contiguously across a hand-over. */

static struct
{
	VkPipelineLayout layout;
	VkShaderModule vertex, fragment;
	VkPipeline pipeline;
	int tried;
} T;

static const char *const test_vertex_source =
	"#version 450\n"
	"layout(location = 0) in vec3 position;\n"
	"layout(location = 1) in vec4 colour;\n"
	"layout(location = 0) out vec4 v_colour;\n"
	"void main()\n"
	"{\n"
	"\tv_colour = colour;\n"
	"\tgl_Position = vec4(position, 1.0);\n"
	"}\n";

static const char *const test_fragment_source =
	"#version 450\n"
	"layout(location = 0) in vec4 v_colour;\n"
	"layout(location = 0) out vec4 out_colour;\n"
	"void main()\n"
	"{\n"
	"\tout_colour = v_colour;\n"
	"}\n";

#define TEST_SIZE 16

static VkPipeline test_pipeline(void)
{
	VkPipelineShaderStageCreateInfo stages[2] = { { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
		{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO } };
	VkVertexInputBindingDescription binding;
	VkVertexInputAttributeDescription attributes[2];
	VkPipelineVertexInputStateCreateInfo vertex_input = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VkPipelineInputAssemblyStateCreateInfo assembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	VkPipelineViewportStateCreateInfo viewport = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	VkPipelineRasterizationStateCreateInfo raster = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	VkPipelineMultisampleStateCreateInfo multisample = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	VkPipelineColorBlendAttachmentState blend_attachment;
	VkPipelineColorBlendStateCreateInfo blend = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	VkPipelineRenderingCreateInfo rendering = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	VkGraphicsPipelineCreateInfo info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	VkPipelineLayoutCreateInfo layout = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

	if (T.pipeline || T.tried)
		return T.pipeline;
	T.tried = 1;
	T.vertex = shader_module(test_vertex_source, 0, "self-test vertex shader");
	T.fragment = shader_module(test_fragment_source, 1, "self-test fragment shader");
	if (!T.vertex || !T.fragment || !HOST_VK_CHECK(vkCreatePipelineLayout(B.device, &layout, NULL, &T.layout)))
		return VK_NULL_HANDLE;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = T.vertex;
	stages[0].pName = "main";
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = T.fragment;
	stages[1].pName = "main";
	binding.binding = 0;
	binding.stride = sizeof(struct vk_test_vertex);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	attributes[0].location = 0;
	attributes[0].binding = 0;
	attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributes[0].offset = offsetof(struct vk_test_vertex, position);
	attributes[1].location = 1;
	attributes[1].binding = 0;
	attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes[1].offset = offsetof(struct vk_test_vertex, color);
	vertex_input.vertexBindingDescriptionCount = 1;
	vertex_input.pVertexBindingDescriptions = &binding;
	vertex_input.vertexAttributeDescriptionCount = 2;
	vertex_input.pVertexAttributeDescriptions = attributes;
	assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	viewport.viewportCount = 1;
	viewport.scissorCount = 1;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = VK_CULL_MODE_NONE;
	raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	raster.lineWidth = 1.0f;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	memset(&blend_attachment, 0, sizeof(blend_attachment));
	blend_attachment.colorWriteMask = 0xf;
	blend.attachmentCount = 1;
	blend.pAttachments = &blend_attachment;
	dynamic.dynamicStateCount = 2;
	dynamic.pDynamicStates = dynamic_states;
	rendering.colorAttachmentCount = 1;
	rendering.pColorAttachmentFormats = &B.color_format;
	info.pNext = &rendering;
	info.stageCount = 2;
	info.pStages = stages;
	info.pVertexInputState = &vertex_input;
	info.pInputAssemblyState = &assembly;
	info.pViewportState = &viewport;
	info.pRasterizationState = &raster;
	info.pMultisampleState = &multisample;
	info.pColorBlendState = &blend;
	info.pDynamicState = &dynamic;
	info.layout = T.layout;
	info.basePipelineIndex = -1;
	if (!HOST_VK_CHECK(vkCreateGraphicsPipelines(B.device, VK_NULL_HANDLE, 1, &info, NULL, &T.pipeline)))
		T.pipeline = VK_NULL_HANDLE;
	return T.pipeline;
}

static void command_test_draw(const struct vk_command_test_draw *command)
{
	struct vk_surface surface = { 0xffffffe0u, TEST_SIZE, TEST_SIZE, TEST_SIZE, TEST_SIZE, VK_SURFACE_COLOR };
	struct host_vk_target *target;
	VkBuffer vertices, readback = VK_NULL_HANDLE;
	VkDeviceSize vertices_offset;
	VkDeviceMemory readback_memory = VK_NULL_HANDLE;
	VkCommandBuffer cmd;
	VkRenderingAttachmentInfo attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	VkRenderingInfo rendering = { VK_STRUCTURE_TYPE_RENDERING_INFO };
	VkViewport viewport = { 0.0f, 0.0f, TEST_SIZE, TEST_SIZE, 0.0f, 1.0f };
	VkRect2D scissor = { { 0, 0 }, { TEST_SIZE, TEST_SIZE } };
	VkBufferImageCopy copy;
	VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	unsigned char *pixels = NULL, first[4] = { 0, 0, 0, 0 };
	int bad = 0, index;
	const char *failure = NULL;

	if (!host_vk_data_find(command->data.id, command->data.offset, 3 * sizeof(struct vk_test_vertex), &vertices,
		&vertices_offset))
	{
		failure = "the data is not there";
		goto done;
	}
	if (!test_pipeline())
	{
		failure = "no pipeline";
		goto done;
	}
	target = host_vk_target_get(&surface);
	if (!target)
	{
		failure = "no target";
		goto done;
	}
	host_vk_rendering_end();
	cmd = host_vk_frame_command();
	host_vk_target_transition(cmd, target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	attachment.imageView = target->view;
	attachment.imageLayout = target->layout;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	rendering.renderArea.extent.width = TEST_SIZE;
	rendering.renderArea.extent.height = TEST_SIZE;
	rendering.layerCount = 1;
	rendering.colorAttachmentCount = 1;
	rendering.pColorAttachments = &attachment;
	host_vk_cmd_begin_rendering(cmd, &rendering);
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, T.pipeline);
	vkCmdBindVertexBuffers(cmd, 0, 1, &vertices, &vertices_offset);
	vkCmdDraw(cmd, 3, 1, 0, 0);
	host_vk_cmd_end_rendering(cmd);
	if (!host_vk_buffer_make(TEST_SIZE * TEST_SIZE * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, &readback, &readback_memory,
		(void **)&pixels))
	{
		failure = "no readback buffer";
		goto done;
	}
	host_vk_target_transition(cmd, target, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	memset(&copy, 0, sizeof(copy));
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.layerCount = 1;
	copy.imageExtent.width = TEST_SIZE;
	copy.imageExtent.height = TEST_SIZE;
	copy.imageExtent.depth = 1;
	vkCmdCopyImageToBuffer(cmd, target->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback, 1, &copy);
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, NULL, 0, NULL);
	if (!frame_submit_and_wait("the data self-test"))
	{
		failure = "the frame could not be submitted";
		goto done;
	}
	memcpy(first, pixels, 4);
	for (index = 0; index < TEST_SIZE * TEST_SIZE; index++)
	{
		const unsigned char *texel = pixels + index * 4; /* B, G, R, A */

		if (!near(texel[2], (int)command->expected[0]) || !near(texel[1], (int)command->expected[1]) ||
			!near(texel[0], (int)command->expected[2]) || !near(texel[3], 255))
			bad++;
	}
	if (bad)
		failure = "texels differ";
done:
	if (failure)
		host_logf(HOST_LOG_ERROR, "vk: data self-test: id %u FAILED (%s; wanted R%u G%u B%u, first texel R%u G%u B%u A%u, %d of %d texels differ)",
			(unsigned)command->data.id, failure, (unsigned)command->expected[0], (unsigned)command->expected[1],
			(unsigned)command->expected[2], first[2], first[1], first[0], first[3], bad, TEST_SIZE * TEST_SIZE);
	else
		host_logf(HOST_LOG_INFO, "vk: data self-test: id %u ok (wanted R%u G%u B%u, seen R%u G%u B%u)", (unsigned)command->data.id,
			(unsigned)command->expected[0], (unsigned)command->expected[1], (unsigned)command->expected[2], first[2], first[1],
			first[0]);
	if (readback)
		vkDestroyBuffer(B.device, readback, NULL);
	if (readback_memory)
		vkFreeMemory(B.device, readback_memory, NULL);
	if (command->last)
		host_vk_data_frame_end();
}

static void log_statistics(void)
{
	struct host_vk_counts *c = &B.counts;

	host_logf(HOST_LOG_INFO, "vk: %u frames: %u hand-overs; commands: %u targets, %u clears, %u presents; clears: %u drawn, "
		"%u by vkCmdClearAttachments; %u target changes; %u images; submissions %llu, retired %u; %u swapchain "
		"re-creations; validation errors %u; data: %u records, %u KB a frame, %u ring buffers", c->frames, c->hand_overs,
		c->targets, c->clears, c->presents, c->clears_drawn, c->clears_attachments, c->target_changes, B.images,
		(unsigned long long)B.submission, retired_locked(), c->recreations, host_vk_validation_errors(), c->data_records,
		c->data_bytes / 1024 / (c->frames ? c->frames : 1), host_vk_data_buffers());
	host_logf(HOST_LOG_INFO, "vk: draws in %u frames: %u made; skipped %u for a shader, %u for a pipeline, %u no target, %u data "
		"missing, %u other; %u with a texture missing; textures: %u images, %u MB of memory, %u KB copied, %u copies skipped",
		c->frames, c->draws_made, c->draws_skipped_shader, c->draws_skipped_pipeline, c->draws_skipped_target,
		c->draws_skipped_data, c->draws_skipped_other, c->draws_texture_missing, host_vk_texture_images(),
		host_vk_texture_megabytes(), c->texture_bytes / 1024, c->textures_skipped);
	{
		char services[400];

		static unsigned lines;

		host_vk_services_statistics(services, sizeof(services));
		/* the services' counts change slowly: one line in ten (about every four seconds) */
		if (services[0] && lines++ % 10 == 0)
			host_logf(HOST_LOG_INFO, "vk: services: %s", services);
	}
	c->draws_ready = c->draws_skipped_shader = c->draws_skipped_pipeline = 0;
	c->draws_made = c->draws_skipped_target = c->draws_skipped_data = c->draws_skipped_other = c->draws_texture_missing = 0;
	c->textures_skipped = c->texture_bytes = 0;
	c->frames = c->hand_overs = c->targets = c->clears = c->presents = c->clears_drawn = c->clears_attachments = 0;
	c->target_changes = 0;
	c->data_records = c->data_bytes = 0;
}

static void backend_unavailable(const char *format, ...) __attribute__((format(printf, 1, 2)));

/* The frame acquired a swapchain image but could not be submitted (vkQueueSubmit failed, and said why): the acquire
semaphore is signalled with nothing to wait on it, and the image is held. An empty batch waits on the semaphore and
signals the image's render-done semaphore, so that the image can be presented (it shows what it held) and the
semaphore is free for the frame's next acquire. If that fails too, the swapchain is let go and the semaphore made
anew once the device is idle (nothing can be waiting on it then but the presentation engine, which the swapchain's
destruction releases). */
static void present_unsubmitted(uint32_t image)
{
	struct host_vk_frame *frame = &B.frames[B.frame];
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	VkSemaphoreCreateInfo semaphore = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &frame->acquired;
	submit.pWaitDstStageMask = &wait_stage;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &B.chain.render_done[image];
	if (HOST_VK_CHECK(vkQueueSubmit(B.queue, 1, &submit, VK_NULL_HANDLE)))
	{
		host_vk_present_queue(image);
		return;
	}
	if (B.dead)
		return;
	host_vk_surface_lost("a frame could not be submitted");
	vkDeviceWaitIdle(B.device);
	vkDestroySemaphore(B.device, frame->acquired, NULL);
	frame->acquired = VK_NULL_HANDLE;
	if (!HOST_VK_CHECK(vkCreateSemaphore(B.device, &semaphore, NULL, &frame->acquired)))
		backend_unavailable("the frame's acquire semaphore could not be made again");
}

static void command_present(const struct vk_command_present *command)
{
	VkCommandBuffer cmd = host_vk_frame_command();
	struct host_vk_target *back_buffer = NULL;
	uint32_t image = UINT32_MAX;

	host_vk_rendering_end();
	host_vk_visibility_frame_end(cmd);
	if (command->back_buffer.kind == VK_SURFACE_COLOR)
		back_buffer = host_vk_target_get(&command->back_buffer);
	/* the next frame starts with nothing bound (the guest tells the host its targets again) */
	B.color = B.depth = NULL;
	B.samples = 0;
	if (back_buffer && !B.dead)
		image = host_vk_present_record(cmd, back_buffer);
	else
		host_vk_presenting = 0;
	frame_submit(image);
	if (image != UINT32_MAX && !B.dead && !B.frames[B.frame].submitted)
		present_unsubmitted(image);
	else if (image != UINT32_MAX && !B.dead)
		host_vk_present_queue(image);
	host_vk_data_frame_end();
	host_vk_services_tick();
	B.frame = (B.frame + 1) % HOST_VK_FRAMES;
	B.counts.presents++;
	if (++B.counts.frames == 60)
		log_statistics();
}

/* ---------- the device */

static const VkFormat color_candidates[] = { VK_FORMAT_B8G8R8A8_UNORM };

/* the features of a format, optimal tiling */
static VkFormatFeatureFlags features_of(VkFormat format)
{
	VkFormatProperties properties;

	host_vk.vkGetPhysicalDeviceFormatProperties(host_vk.physical, format, &properties);
	return properties.optimalTilingFeatures;
}

static int enabled_add(const char **names, uint32_t *count, const char *name)
{
	uint32_t index;

	for (index = 0; index < host_vk.extension_count; index++)
	{
		if (!strcmp(host_vk.extensions[index], name))
		{
			names[(*count)++] = name;
			return 1;
		}
	}
	return 0;
}

static void backend_unavailable(const char *format, ...)
{
	char reason[300];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(reason, sizeof(reason), format, arguments);
	va_end(arguments);
	host_logf(HOST_LOG_ERROR, "vk: the backend draws nothing for the rest of the run: %s; display.renderer = \"gl\" "
		"is the way back (the game goes on without a picture)", reason);
	B.state = 2;
	host_vk_presenting = 0;
}

/* the logical device, its functions, the formats, the frames and the built-in pipeline layout; run at the first
hand-over (the game's window exists by then). Returns 1 when ready; 0 if the window has no surface yet (tried again at the
next hand-over) or the backend is not available (state 2, said why) */
static int device_create(void)
{
	VkDeviceQueueCreateInfo queue_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	VkDeviceCreateInfo info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering =
		{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR };
	const char *names[8];
	uint32_t name_count = 0, index, family_count = 0;
	VkQueueFamilyProperties *families;
	float priority = 1.0f;
	VkFormatFeatureFlags color_features, depth_features;
	VkResult result;
	int missing = 0;

	/* the surface first: the queue family has to be able to present to it */
	if (!host_vk_surface_make(1))
	{
		static int said;

		if (!said)
			host_logf(HOST_LOG_INFO, "vk: the game's window has no surface yet; the backend waits for one");
		said = 1;
		return 0;
	}
	host_vk.vkGetPhysicalDeviceQueueFamilyProperties(host_vk.physical, &family_count, NULL);
	families = malloc(sizeof(*families) * (family_count ? family_count : 1));
	if (!families)
		return 0;
	host_vk.vkGetPhysicalDeviceQueueFamilyProperties(host_vk.physical, &family_count, families);
	/* the family the startup chose, if it can present; otherwise the first graphics family that can */
	B.family = UINT32_MAX;
	for (index = 0; index <= family_count && B.family == UINT32_MAX; index++)
	{
		uint32_t family = index == 0 ? host_vk.queue_family : index - 1;
		VkBool32 present = VK_FALSE;

		if (family >= family_count || !(families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) || !families[family].queueCount)
			continue;
		if (host_vk.vkGetPhysicalDeviceSurfaceSupportKHR(host_vk.physical, family, B.surface, &present) == VK_SUCCESS && present)
			B.family = family;
	}
	free(families);
	if (B.family == UINT32_MAX)
	{
		backend_unavailable("no graphics queue family can present to the window's surface");
		return 0;
	}
	if (B.family != host_vk.queue_family)
		host_logf(HOST_LOG_INFO, "vk: queue family %u chosen at the startup cannot present; using %u", host_vk.queue_family,
			B.family);

	/* the formats: colour targets B8G8R8A8_UNORM (the Xbox's A8R8G8B8 byte order), depth-stencil D24S8 or D32S8 */
	B.color_format = color_candidates[0];
	color_features = features_of(B.color_format);
	if (!(color_features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) || !(color_features & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
		(host_vk.api >= VK_API_VERSION_1_1 && !(color_features & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)))
	{
		backend_unavailable("B8G8R8A8_UNORM cannot be a colour attachment and a blit source here (features 0x%x)",
			(unsigned)color_features);
		return 0;
	}
	color_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (host_vk.api < VK_API_VERSION_1_1 || (color_features & VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
		color_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (color_features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
		color_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	B.blit_filter = (color_features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	if (B.blit_filter != VK_FILTER_LINEAR)
		host_logf(HOST_LOG_WARN, "vk: B8G8R8A8_UNORM cannot be filtered linearly here; the picture is presented unfiltered");
	B.depth_format = VK_FORMAT_D24_UNORM_S8_UINT;
	depth_features = features_of(B.depth_format);
	if (!(depth_features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
	{
		B.depth_format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		depth_features = features_of(B.depth_format);
	}
	if (!(depth_features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
	{
		backend_unavailable("neither D24_UNORM_S8_UINT nor D32_SFLOAT_S8_UINT can be a depth-stencil attachment");
		return 0;
	}
	depth_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (depth_features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
		depth_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (host_vk.api < VK_API_VERSION_1_1 || (depth_features & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT))
		depth_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (host_vk.api < VK_API_VERSION_1_1 || (depth_features & VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
		depth_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	host_vk.vkGetPhysicalDeviceMemoryProperties(host_vk.physical, &B.memory);

	/* the device: swapchain, and dynamic rendering (the 1.3 feature, or the extension with what it depends on
	before 1.2: host_vk_startup found them) */
	enabled_add(names, &name_count, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	if (host_vk.api >= VK_API_VERSION_1_3)
	{
		features13.dynamicRendering = VK_TRUE;
		features.pNext = &features13;
	}
	else
	{
		enabled_add(names, &name_count, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		if (host_vk.api < VK_API_VERSION_1_2)
		{
			enabled_add(names, &name_count, VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
			enabled_add(names, &name_count, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
		}
		if (host_vk.api < VK_API_VERSION_1_1)
		{
			enabled_add(names, &name_count, VK_KHR_MULTIVIEW_EXTENSION_NAME);
			enabled_add(names, &name_count, VK_KHR_MAINTENANCE_2_EXTENSION_NAME);
		}
		dynamic_rendering.dynamicRendering = VK_TRUE;
		features.pNext = &dynamic_rendering;
	}
	/* the optional features the draws use, where the device has them: anisotropic filtering, non-solid fill (wireframe and
	point fill), precise occlusion queries (visibility tests) and the block-compressed textures (BC1 to BC3) */
	{
		VkPhysicalDeviceFeatures2 available = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

		host_vk.vkGetPhysicalDeviceFeatures2(host_vk.physical, &available);
		features.features.samplerAnisotropy = available.features.samplerAnisotropy;
		features.features.fillModeNonSolid = available.features.fillModeNonSolid;
		features.features.occlusionQueryPrecise = available.features.occlusionQueryPrecise;
		features.features.textureCompressionBC = available.features.textureCompressionBC;
		B.sampler_anisotropy = available.features.samplerAnisotropy != 0;
		B.fill_mode_non_solid = available.features.fillModeNonSolid != 0;
		B.occlusion_query_precise = available.features.occlusionQueryPrecise != 0;
		B.texture_compression_bc = available.features.textureCompressionBC != 0;
	}
	for (index = 0; index < name_count; index++)
		host_logf(HOST_LOG_INFO, "vk: device extension %s", names[index]);
	queue_info.queueFamilyIndex = B.family;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = &priority;
	info.pNext = &features;
	info.queueCreateInfoCount = 1;
	info.pQueueCreateInfos = &queue_info;
	info.enabledExtensionCount = name_count;
	info.ppEnabledExtensionNames = names;
	result = host_vk.vkCreateDevice(host_vk.physical, &info, NULL, &B.device);
	if (result != VK_SUCCESS)
	{
		backend_unavailable("vkCreateDevice returned %d", (int)result);
		return 0;
	}

	/* the device-level functions: one missing is a failure */
#define X(name) \
	name = (PFN_##name)host_vk.vkGetDeviceProcAddr(B.device, #name); \
	if (!name) { host_logf(HOST_LOG_ERROR, "vk: the device has no %s", #name); missing++; }
	HOST_VK_DEVICE_FUNCTIONS(X)
#undef X
	host_vk_cmd_begin_rendering = (PFN_vkCmdBeginRenderingKHR)host_vk.vkGetDeviceProcAddr(B.device,
		host_vk.api >= VK_API_VERSION_1_3 ? "vkCmdBeginRendering" : "vkCmdBeginRenderingKHR");
	host_vk_cmd_end_rendering = (PFN_vkCmdEndRenderingKHR)host_vk.vkGetDeviceProcAddr(B.device,
		host_vk.api >= VK_API_VERSION_1_3 ? "vkCmdEndRendering" : "vkCmdEndRenderingKHR");
	if (!host_vk_cmd_begin_rendering || !host_vk_cmd_end_rendering)
	{
		host_logf(HOST_LOG_ERROR, "vk: the device has no dynamic rendering entry points");
		missing++;
	}
	if (missing)
	{
		if (vkDestroyDevice)
			vkDestroyDevice(B.device, NULL);
		B.device = VK_NULL_HANDLE;
		backend_unavailable("the device lacks %d entry points", missing);
		return 0;
	}
	vkGetDeviceQueue(B.device, B.family, 0, &B.queue);

	/* frames in flight */
	for (index = 0; index < HOST_VK_FRAMES; index++)
	{
		struct host_vk_frame *frame = &B.frames[index];
		VkCommandPoolCreateInfo pool = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		VkCommandBufferAllocateInfo allocate = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		VkFenceCreateInfo fence = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		VkSemaphoreCreateInfo semaphore = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

		pool.queueFamilyIndex = B.family;
		allocate.commandBufferCount = 1;
		allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		if (!HOST_VK_CHECK(vkCreateCommandPool(B.device, &pool, NULL, &frame->pool)))
			goto failed;
		allocate.commandPool = frame->pool;
		if (!HOST_VK_CHECK(vkAllocateCommandBuffers(B.device, &allocate, &frame->command)) ||
			!HOST_VK_CHECK(vkCreateFence(B.device, &fence, NULL, &frame->fence)) ||
			!HOST_VK_CHECK(vkCreateSemaphore(B.device, &semaphore, NULL, &frame->acquired)))
			goto failed;
	}

	/* the built-in pipeline of clears of some channels only: its shaders compiled now, its pipelines when first needed */
	{
		VkPipelineLayoutCreateInfo layout = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		VkPushConstantRange range;

		range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		range.offset = 0;
		range.size = 16;
		layout.pushConstantRangeCount = 1;
		layout.pPushConstantRanges = &range;
		if (!HOST_VK_CHECK(vkCreatePipelineLayout(B.device, &layout, NULL, &B.clear_layout)))
			goto failed;
		B.clear_vertex = shader_module(clear_vertex_source, 0, "clear vertex shader");
		B.clear_fragment = shader_module(clear_fragment_source, 1, "clear fragment shader");
		if (!B.clear_vertex || !B.clear_fragment)
			host_logf(HOST_LOG_ERROR, "vk: clears of some channels only (the fog's alpha clear) will be skipped");
	}
	B.state = 1;
	host_vk_services_start();
	host_vk_visibility_start();
	if (host_vk_self_test)
		clears_selftest();
	host_logf(HOST_LOG_INFO, "vk: device ready: queue family %u, colour targets %s, depth targets %s, blit filter %s, "
		"dynamic rendering from %s; anisotropy %d, non-solid fill %d, precise occlusion %d, BC textures %d", B.family, "B8G8R8A8_UNORM",
		B.depth_format == VK_FORMAT_D24_UNORM_S8_UINT ? "D24_UNORM_S8_UINT" : "D32_SFLOAT_S8_UINT",
		B.blit_filter == VK_FILTER_LINEAR ? "linear" : "nearest",
		host_vk.api >= VK_API_VERSION_1_3 ? "Vulkan 1.3" : "VK_KHR_dynamic_rendering", B.sampler_anisotropy,
		B.fill_mode_non_solid, B.occlusion_query_precise, B.texture_compression_bc);
	return 1;
failed:
	backend_unavailable("the device's frames could not be made");
	return 0;
}

/* the backend's device, made if it is not yet (B.lock is held): the services' imports need it before the first hand-over */
void host_vk_device_ensure_locked(void)
{
	if (B.state == 0)
		device_create();
}

/* whether the device reads a format as a vertex attribute (the guest asks once for each of the optional ones and expands the
rest to four floats) */
uint32_t host_vk_format_supported(uint32_t format)
{
	VkFormatProperties properties;

	if (!host_vk.instance || !host_vk.physical)
		return 1;
	host_vk.vkGetPhysicalDeviceFormatProperties(host_vk.physical, (VkFormat)format, &properties);
	return (properties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) != 0;
}

uint32_t host_vk_bc_supported(void)
{
	static const VkFormat formats[] = { VK_FORMAT_BC1_RGBA_UNORM_BLOCK, VK_FORMAT_BC2_UNORM_BLOCK, VK_FORMAT_BC3_UNORM_BLOCK };
	/* (the transfer bits are Vulkan 1.1's: before it, every format that can be sampled can be copied to) */
	const VkFormatFeatureFlags wanted = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
		(host_vk.api >= VK_API_VERSION_1_1 ? VK_FORMAT_FEATURE_TRANSFER_DST_BIT : 0);
	unsigned index;

	if (!host_vk_backend_ensure() || !B.texture_compression_bc)
		return 0;
	for (index = 0; index < 3; index++)
	{
		if ((features_of(formats[index]) & wanted) != wanted)
			return 0;
	}
	return 1;
}

/* runs size bytes of commands at commands (a guest address), during the call */
void host_vk_submit(uint32_t commands, uint32_t size)
{
	const unsigned char *at = (const unsigned char *)(uintptr_t)commands;
	const unsigned char *end = at + size;
	static int said_unknown;

	if (!host_vk.instance)
		return;
	pthread_mutex_lock(&B.lock);
	if (B.state == 0)
		device_create();
	if (B.state != 1 || B.dead)
	{
		pthread_mutex_unlock(&B.lock);
		return;
	}
	B.counts.hand_overs++;
	while (!B.dead && at + sizeof(struct vk_command_header) <= end)
	{
		const struct vk_command_header *header = (const struct vk_command_header *)at;
		uint32_t offset = (uint32_t)(at - (const unsigned char *)(uintptr_t)commands);

		if (header->size < sizeof(*header) || (header->size & 3) || at + header->size > end)
		{
			if (!said_unknown)
				host_logf(HOST_LOG_ERROR, "vk: a command of %u bytes (type %u) at offset %u runs past the stream's end; "
					"the rest of the hand-over is dropped", (unsigned)header->size, (unsigned)header->type, (unsigned)offset);
			said_unknown = 1;
			break;
		}
		switch (header->type)
		{
		case VK_COMMAND_TARGETS:
			if (header->size >= sizeof(struct vk_command_targets))
				command_targets((const struct vk_command_targets *)header);
			break;
		case VK_COMMAND_CLEAR:
			if (header->size >= sizeof(struct vk_command_clear))
				command_clear((const struct vk_command_clear *)header, header->size);
			break;
		case VK_COMMAND_PRESENT:
			if (header->size >= sizeof(struct vk_command_present))
				command_present((const struct vk_command_present *)header);
			break;
		case VK_COMMAND_DATA:
			if (header->size >= sizeof(struct vk_command_data) && host_vk_data_command((const struct vk_command_data *)header, header->size))
				break;
			at = end;
			continue;
		case VK_COMMAND_PIPELINE:
			if (header->size >= sizeof(struct vk_command_pipeline))
				host_vk_pipeline_command((const struct vk_command_pipeline *)header);
			break;
		case VK_COMMAND_DRAW:
			if (header->size >= sizeof(struct vk_command_draw))
				host_vk_command_draw((const struct vk_command_draw *)header, header->size);
			break;
		case VK_COMMAND_TEXTURE:
			if (header->size >= sizeof(struct vk_command_texture))
				host_vk_texture_command((const struct vk_command_texture *)header);
			break;
		case VK_COMMAND_TEXTURE_DATA:
			if (header->size >= sizeof(struct vk_command_texture_data))
				host_vk_texture_data_command((const struct vk_command_texture_data *)header);
			break;
		case VK_COMMAND_TEXTURE_FREE:
			if (header->size >= sizeof(struct vk_command_texture_free))
				host_vk_texture_free_command((const struct vk_command_texture_free *)header);
			break;
		case VK_COMMAND_COMPOSITE:
			if (header->size >= sizeof(struct vk_command_composite))
				host_vk_composite_command((const struct vk_command_composite *)header);
			break;
		case VK_COMMAND_VISIBILITY_BEGIN:
			host_vk_visibility_begin();
			break;
		case VK_COMMAND_VISIBILITY_END:
			if (header->size >= sizeof(struct vk_command_visibility_end))
				host_vk_visibility_end((const struct vk_command_visibility_end *)header);
			break;
		case VK_COMMAND_ANTI_ALIAS:
			if (header->size >= sizeof(struct vk_command_anti_alias))
				host_vk_anti_alias_command((const struct vk_command_anti_alias *)header);
			break;
		case VK_COMMAND_TEST_DRAW:
			if (header->size >= sizeof(struct vk_command_test_draw))
				command_test_draw((const struct vk_command_test_draw *)header);
			break;
		default:
			if (!said_unknown)
				host_logf(HOST_LOG_ERROR, "vk: an unknown command, type %u, at offset %u; the rest of the hand-over is "
					"dropped", (unsigned)header->type, (unsigned)offset);
			said_unknown = 1;
			at = end;
			continue;
		}
		at += header->size;
	}
	pthread_mutex_unlock(&B.lock);
}

static void __attribute__((constructor)) backend_lock_init(void)
{
	pthread_mutex_init(&B.lock, NULL);
}
