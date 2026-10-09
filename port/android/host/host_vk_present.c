/*
HOST_VK_PRESENT.C

The Vulkan renderer's surface and swapchain,
on the game's window: the surface is made with vkCreateAndroidSurfaceKHR on the
window's ANativeWindow (no SDL_Vulkan_* call: SDL would load the phone's driver
beside the chosen one), the swapchain is FIFO with the surface's own images'
count plus one, and a frame is shown by a letterboxed blit of the game's back
buffer into the acquired image.

Android destroys the window's ANativeWindow when the app goes to the background
and makes a new one on return; SDL's lifecycle events do not reach the game's
loop, so the loss is found from what the calls return and from the
window's ANativeWindow changing. While there is no swapchain a frame's work is
still submitted (its fence and number move on) and nothing goes to the screen.
*/

#include "host.h"
#include "host_vk.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define B host_vkb

#define ACQUIRE_TIMEOUT_NS 1000000000ull
/* how often (in presented frames) the surface's size is looked at */
#define EXTENT_CHECK_FRAMES 60

static uint64_t now_ns(void)
{
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);
	return (uint64_t)time.tv_sec * 1000000000ull + (uint64_t)time.tv_nsec;
}

/* the window's ANativeWindow as it is now (a new one after the app was away), or NULL */
static void *native_window(void)
{
	if (!host_vk_window)
		return NULL;
	return SDL_GetPointerProperty(SDL_GetWindowProperties((SDL_Window *)host_vk_window),
		SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, NULL);
}

int host_vk_surface_make(int quiet)
{
	VkAndroidSurfaceCreateInfoKHR info = { VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR };
	void *native = native_window();
	VkResult result;

	if (B.surface)
		return 1;
	if (!native)
		return 0;
	info.window = (struct ANativeWindow *)native;
	result = host_vk.vkCreateAndroidSurfaceKHR(host_vk.instance, &info, NULL, &B.surface);
	if (result != VK_SUCCESS)
	{
		B.surface = VK_NULL_HANDLE;
		if (!quiet)
			host_vk_check(result, "vkCreateAndroidSurfaceKHR");
		return 0;
	}
	B.native_window = native;
	return 1;
}

/* ---------- the swapchain */

static const char *transform_name(VkSurfaceTransformFlagBitsKHR transform)
{
	switch (transform)
	{
	case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR: return "identity";
	case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR: return "rotate_90";
	case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR: return "rotate_180";
	case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR: return "rotate_270";
	default: return "other";
	}
}

static void swapchain_destroy(struct host_vk_swapchain *chain)
{
	uint32_t index;

	for (index = 0; index < chain->count; index++)
	{
		if (chain->render_done[index])
			vkDestroySemaphore(B.device, chain->render_done[index], NULL);
		if (chain->views[index])
			vkDestroyImageView(B.device, chain->views[index], NULL);
	}
	if (chain->handle)
		vkDestroySwapchainKHR(B.device, chain->handle, NULL);
	memset(chain, 0, sizeof(*chain));
}

/* the size the surface says its images are: its current extent, or the window's pixels where the surface leaves
it to the swapchain */
static int surface_extent(const VkSurfaceCapabilitiesKHR *caps, VkExtent2D *extent)
{
	*extent = caps->currentExtent;
	if (extent->width == 0xffffffffu)
	{
		int width = 0, height = 0;

		if (host_vk_window)
			SDL_GetWindowSizeInPixels((SDL_Window *)host_vk_window, &width, &height);
		extent->width = (uint32_t)width;
		extent->height = (uint32_t)height;
	}
	return extent->width && extent->height;
}

/* makes a swapchain for the surface as it is now: FIFO, the surface's own transform left to the compositor
(preTransform identity: Android turns the picture), the first of B8G8R8A8_UNORM or R8G8B8A8_UNORM it lists in the
sRGB-nonlinear colour space, one image more than the minimum. Returns 0 (and says why, unless quiet) if it cannot. */
static int swapchain_create(struct host_vk_swapchain *chain, VkSwapchainKHR old, int quiet, VkSurfaceTransformFlagBitsKHR *transform_out)
{
	VkSurfaceCapabilitiesKHR caps;
	VkSwapchainCreateInfoKHR info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	VkSurfaceFormatKHR formats[32];
	uint32_t format_count = 32, index, count;
	VkResult result;
	int chosen = -1;
	VkCompositeAlphaFlagBitsKHR alpha;
	VkExtent2D extent;
	VkFormatProperties properties;

	memset(chain, 0, sizeof(*chain));
	result = host_vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(host_vk.physical, B.surface, &caps);
	if (result == VK_ERROR_SURFACE_LOST_KHR)
	{
		host_vk_surface_lost("vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
		return 0;
	}
	if (result != VK_SUCCESS)
		return quiet ? 0 : host_vk_check(result, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	result = host_vk.vkGetPhysicalDeviceSurfaceFormatsKHR(host_vk.physical, B.surface, &format_count, formats);
	if (result != VK_SUCCESS && result != VK_INCOMPLETE)
		return quiet ? 0 : host_vk_check(result, "vkGetPhysicalDeviceSurfaceFormatsKHR");
	for (index = 0; index < format_count && chosen < 0; index++)
	{
		if ((formats[index].format == VK_FORMAT_B8G8R8A8_UNORM || formats[index].format == VK_FORMAT_R8G8B8A8_UNORM) &&
			formats[index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			chosen = (int)index;
	}
	if (chosen < 0)
	{
		if (!quiet)
			host_logf(HOST_LOG_ERROR, "vk: the surface lists neither B8G8R8A8_UNORM nor R8G8B8A8_UNORM (sRGB nonlinear)");
		return 0;
	}
	if (!surface_extent(&caps, &extent))
	{
		if (!quiet)
			host_logf(HOST_LOG_WARN, "vk: the surface has no size yet");
		return 0;
	}
	if ((caps.supportedUsageFlags & (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) !=
		(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
	{
		if (!quiet)
			host_logf(HOST_LOG_ERROR, "vk: the surface cannot take colour attachment and transfer destination usage");
		return 0;
	}
	/* the picture is blitted into the swapchain's images, so their format has to be a blit destination */
	host_vk.vkGetPhysicalDeviceFormatProperties(host_vk.physical, formats[chosen].format, &properties);
	if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT))
	{
		if (!quiet)
			host_logf(HOST_LOG_ERROR, "vk: the swapchain format %d is not a blit destination here", (int)formats[chosen].format);
		return 0;
	}
	count = caps.minImageCount + 1;
	if (caps.maxImageCount && count > caps.maxImageCount)
		count = caps.maxImageCount;
	/* opaque if the surface offers it, else the first of the others (this device's surface offers only inherit) */
	alpha = (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR :
		(caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) ? VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR :
		(caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR :
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
	info.surface = B.surface;
	info.minImageCount = count;
	info.imageFormat = formats[chosen].format;
	info.imageColorSpace = formats[chosen].colorSpace;
	info.imageExtent = extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.preTransform = (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ?
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : caps.currentTransform;
	info.compositeAlpha = alpha;
	info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	info.clipped = VK_TRUE;
	info.oldSwapchain = old;
	result = vkCreateSwapchainKHR(B.device, &info, NULL, &chain->handle);
	if (result == VK_ERROR_SURFACE_LOST_KHR || result == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
	{
		chain->handle = VK_NULL_HANDLE;
		host_vk_surface_lost("vkCreateSwapchainKHR");
		return 0;
	}
	if (result != VK_SUCCESS)
	{
		chain->handle = VK_NULL_HANDLE;
		return quiet ? 0 : host_vk_check(result, "vkCreateSwapchainKHR");
	}
	chain->format = info.imageFormat;
	chain->extent = info.imageExtent;
	count = HOST_VK_SWAPCHAIN_IMAGES;
	result = vkGetSwapchainImagesKHR(B.device, chain->handle, &count, chain->images);
	if (result != VK_SUCCESS && result != VK_INCOMPLETE)
	{
		host_vk_check(result, "vkGetSwapchainImagesKHR");
		swapchain_destroy(chain);
		return 0;
	}
	chain->count = count;
	for (index = 0; index < count; index++)
	{
		VkSemaphoreCreateInfo semaphore = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

		if (!host_vk_check(vkCreateSemaphore(B.device, &semaphore, NULL, &chain->render_done[index]), "vkCreateSemaphore"))
		{
			/* the handle and the semaphores made so far */
			swapchain_destroy(chain);
			return 0;
		}
		if (host_vk_present_marker)
		{
			/* the marker is drawn in a rendering on the image */
			VkImageViewCreateInfo view = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };

			view.image = chain->images[index];
			view.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view.format = chain->format;
			view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view.subresourceRange.levelCount = 1;
			view.subresourceRange.layerCount = 1;
			if (!host_vk_check(vkCreateImageView(B.device, &view, NULL, &chain->views[index]), "vkCreateImageView"))
			{
				swapchain_destroy(chain);
				return 0;
			}
		}
	}
	if (transform_out)
		*transform_out = info.preTransform;
	return 1;
}

/* the swapchain and the surface are gone: waits for the device, then destroys them */
static void present_teardown(void)
{
	if (B.device)
		vkDeviceWaitIdle(B.device);
	swapchain_destroy(&B.chain);
	if (B.surface)
		host_vk.vkDestroySurfaceKHR(host_vk.instance, B.surface, NULL);
	B.surface = VK_NULL_HANDLE;
	B.native_window = NULL;
	host_vk_presenting = 0;
}

/* the surface is gone (found by acquire, by present, or by the window's ANativeWindow changing): counted and
logged once, however many tries it takes to make another */
void host_vk_surface_lost(const char *where)
{
	if (!B.lost_at)
	{
		B.lost_at = now_ns();
		B.lost_attempts = 0;
		host_logf(HOST_LOG_INFO, "vk: the surface is lost (%s)", where);
		/* the app is going to the background, which Android may follow with the process's end */
		host_vk_pipeline_cache_save("the surface was lost");
	}
	present_teardown();
}

/* a swapchain, if there is none: on the surface there is, or on one made now. Logs the return from a loss once. */
static int ensure_swapchain(void)
{
	VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	void *native = native_window();

	/* the window has a new ANativeWindow: the surface is on the old one */
	if (B.surface && native != B.native_window)
		host_vk_surface_lost(native ? "the window's ANativeWindow changed" : "the window has no ANativeWindow");
	if (B.chain.handle)
		return 1;
	if (B.lost_at)
		B.lost_attempts++;
	if (!B.surface && !host_vk_surface_make(1))
		return 0;
	if (!swapchain_create(&B.chain, VK_NULL_HANDLE, B.lost_at != 0, &transform))
		return 0;
	if (B.lost_at)
	{
		host_logf(HOST_LOG_INFO, "vk: the surface is back after %.1f ms, %u attempts: swapchain %ux%u, %u images, "
			"transform %s", (double)(now_ns() - B.lost_at) / 1e6, B.lost_attempts, B.chain.extent.width,
			B.chain.extent.height, B.chain.count, transform_name(transform));
		B.lost_at = 0;
	}
	else
	{
		host_logf(HOST_LOG_INFO, "vk: swapchain %ux%u, %u images, format %s, FIFO, preTransform %s", B.chain.extent.width,
			B.chain.extent.height, B.chain.count, B.chain.format == VK_FORMAT_B8G8R8A8_UNORM ? "B8G8R8A8_UNORM" :
			"R8G8B8A8_UNORM", transform_name(transform));
	}
	return 1;
}

/* makes the swapchain again from the old one, with the device idle first; the reason and the time are logged */
static void swapchain_recreate(const char *reason)
{
	uint64_t start = now_ns();
	struct host_vk_swapchain made, retired;
	VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	int ok;

	vkDeviceWaitIdle(B.device);
	ok = swapchain_create(&made, B.chain.handle, 0, &transform);
	/* the old swapchain is retired by the new one's creation; the device is idle, so its images are not in use */
	/* (if the creation found the surface lost, host_vk_surface_lost has destroyed the old swapchain and the surface
	already, and B.chain is empty here) */
	retired = B.chain;
	memset(&B.chain, 0, sizeof(B.chain));
	swapchain_destroy(&retired);
	if (ok)
	{
		B.chain = made;
		B.counts.recreations++;
		host_logf(HOST_LOG_INFO, "vk: swapchain made again (%s): %ux%u, %u images, %.2f ms", reason, B.chain.extent.width,
			B.chain.extent.height, B.chain.count, (double)(now_ns() - start) / 1e6);
	}
	else
	{
		host_vk_presenting = 0;
		host_logf(HOST_LOG_WARN, "vk: the swapchain could not be made again (%s)", reason);
	}
}

/* does the surface's size differ from the swapchain's? Looked at when the driver says SUBOPTIMAL (Android says so
for the rotation of the display, which is not a reason to make the swapchain again) and every so often */
static void extent_check(void)
{
	VkSurfaceCapabilitiesKHR caps;
	VkExtent2D extent;
	VkResult result;

	result = host_vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(host_vk.physical, B.surface, &caps);
	if (result == VK_ERROR_SURFACE_LOST_KHR)
	{
		host_vk_surface_lost("vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
		return;
	}
	if (result != VK_SUCCESS || !surface_extent(&caps, &extent))
		return;
	if (extent.width != B.chain.extent.width || extent.height != B.chain.extent.height)
		swapchain_recreate("the surface's size changed");
}

/* ---------- presenting */

uint32_t host_vk_present_record(VkCommandBuffer command, struct host_vk_target *back_buffer)
{
	static unsigned frames_since_check;
	VkResult result;
	uint32_t index = 0;
	struct host_vk_frame *frame = &B.frames[B.frame];
	VkImage image;
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	VkImageBlit blit;
	int attempt, acquired = 0;
	int64_t width, height, x, y;

	if (!ensure_swapchain())
	{
		host_vk_presenting = 0;
		return UINT32_MAX;
	}
	if (++frames_since_check >= EXTENT_CHECK_FRAMES)
	{
		frames_since_check = 0;
		extent_check();
		if (!B.chain.handle)
			return UINT32_MAX;
	}
	for (attempt = 0; attempt < 2 && !acquired; attempt++)
	{
		result = vkAcquireNextImageKHR(B.device, B.chain.handle, ACQUIRE_TIMEOUT_NS, frame->acquired, VK_NULL_HANDLE, &index);
		if (result == VK_SUCCESS)
		{
			acquired = 1;
		}
		else if (result == VK_SUBOPTIMAL_KHR)
		{
			/* the image is acquired and its semaphore signalled: it is used; the size is looked at after the present */
			acquired = 1;
			frames_since_check = EXTENT_CHECK_FRAMES;
		}
		else if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			swapchain_recreate("out of date");
			if (!B.chain.handle)
				return UINT32_MAX;
		}
		else if (result == VK_ERROR_SURFACE_LOST_KHR)
		{
			host_vk_surface_lost("vkAcquireNextImageKHR");
			return UINT32_MAX;
		}
		else if (result == VK_TIMEOUT || result == VK_NOT_READY)
		{
			/* with the app away nothing frees an image: the surface is as good as lost */
			host_vk_surface_lost("vkAcquireNextImageKHR timed out");
			return UINT32_MAX;
		}
		else
		{
			host_vk_check(result, "vkAcquireNextImageKHR");
			return UINT32_MAX;
		}
	}
	if (!acquired)
		return UINT32_MAX;
	image = B.chain.images[index];

	/* the back buffer is read by the blit; the image is cleared black (the bars) and the picture blitted into it, the
	letterbox as d3d8_gl.c's Present works it out: the window's width, or its height, whichever the picture's shape fits */
	host_vk_target_transition(command, back_buffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL,
		1, &barrier);
	{
		VkClearColorValue black = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdClearColorImage(command, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &black, 1, &range);
	}
	/* (the clear is a transfer write and the blit another: a barrier between them) */
	{
		VkMemoryBarrier memory = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };

		memory.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memory.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memory, 0, NULL,
			0, NULL);
	}
	width = B.chain.extent.width;
	height = width * (int64_t)back_buffer->key.pixel_height / (int64_t)back_buffer->key.pixel_width;
	if (height > (int64_t)B.chain.extent.height)
	{
		height = B.chain.extent.height;
		width = height * (int64_t)back_buffer->key.pixel_width / (int64_t)back_buffer->key.pixel_height;
	}
	x = ((int64_t)B.chain.extent.width - width) / 2;
	y = ((int64_t)B.chain.extent.height - height) / 2;
	if (width > 0 && height > 0)
	{
		memset(&blit, 0, sizeof(blit));
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.layerCount = 1;
		blit.srcOffsets[1].x = (int32_t)back_buffer->key.pixel_width;
		blit.srcOffsets[1].y = (int32_t)back_buffer->key.pixel_height;
		blit.srcOffsets[1].z = 1;
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.layerCount = 1;
		blit.dstOffsets[0].x = (int32_t)x;
		blit.dstOffsets[0].y = (int32_t)y;
		blit.dstOffsets[1].x = (int32_t)(x + width);
		blit.dstOffsets[1].y = (int32_t)(y + height);
		blit.dstOffsets[1].z = 1;
		vkCmdBlitImage(command, back_buffer->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, B.blit_filter);
	}
	if (host_vk_present_marker && width > 0 && height > 0 && B.chain.views[index])
	{
		/* a red square at the picture's top left and a green one at its top right, so that a screenshot shows
		which way up the picture is and where its corners land. vkCmdClearColorImage clears whole subresources only,
		so the squares are cleared by vkCmdClearAttachments in a rendering on the swapchain image. */
		VkRenderingAttachmentInfo attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		VkRenderingInfo rendering = { VK_STRUCTURE_TYPE_RENDERING_INFO };
		VkClearAttachment clear;
		VkClearRect rect;
		uint32_t size = (uint32_t)(width / 16 > 16 ? width / 16 : 16);
		int square;

		if (size > (uint32_t)height)
			size = (uint32_t)height;
		barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0,
			NULL, 1, &barrier);
		attachment.imageView = B.chain.views[index];
		attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		rendering.renderArea.extent = B.chain.extent;
		rendering.layerCount = 1;
		rendering.colorAttachmentCount = 1;
		rendering.pColorAttachments = &attachment;
		host_vk_cmd_begin_rendering(command, &rendering);
		memset(&clear, 0, sizeof(clear));
		clear.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		memset(&rect, 0, sizeof(rect));
		rect.layerCount = 1;
		rect.rect.extent.width = size;
		rect.rect.extent.height = size;
		rect.rect.offset.y = (int32_t)y;
		for (square = 0; square < 2; square++)
		{
			clear.clearValue.color.float32[0] = square == 0 ? 1.0f : 0.0f;
			clear.clearValue.color.float32[1] = square == 0 ? 0.0f : 1.0f;
			clear.clearValue.color.float32[3] = 1.0f;
			rect.rect.offset.x = square == 0 ? (int32_t)x : (int32_t)(x + width - size);
			vkCmdClearAttachments(command, 1, &clear, 1, &rect);
		}
		host_vk_cmd_end_rendering(command);
		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	else
	{
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = 0;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL,
		1, &barrier);
	return index;
}

void host_vk_present_queue(uint32_t image_index)
{
	VkPresentInfoKHR present = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	VkResult result;
	static int first = 1;

	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &B.chain.render_done[image_index];
	present.swapchainCount = 1;
	present.pSwapchains = &B.chain.handle;
	present.pImageIndices = &image_index;
	result = vkQueuePresentKHR(B.queue, &present);
	if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
	{
		if (first)
			host_logf(HOST_LOG_INFO, "vk: first frame presented");
		first = 0;
		host_vk_presenting = 1;
		if (result == VK_SUBOPTIMAL_KHR)
			extent_check();
	}
	else if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		swapchain_recreate("out of date");
	}
	else if (result == VK_ERROR_SURFACE_LOST_KHR)
	{
		host_vk_surface_lost("vkQueuePresentKHR");
	}
	else
	{
		host_vk_check(result, "vkQueuePresentKHR");
		host_vk_presenting = 0;
	}
}
