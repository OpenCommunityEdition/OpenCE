/*
HOST_VK.H

The Vulkan renderer's host half: what host_vk_startup
brings up and keeps for the backend. host.h declares the
entry points the rest of the host uses (host_vk_startup and
host_renderer_vulkan); this is for host_vk.c's own files.
*/

#ifndef HOST_VK_H
#define HOST_VK_H

#ifndef VK_USE_PLATFORM_ANDROID_KHR
#define VK_USE_PLATFORM_ANDROID_KHR
#endif
#include "host_vk_driver.h"

#include "../guest/vk_commands.h"

#include <pthread.h>

/* the instance-level entry points, loaded through the driver's
vkGetInstanceProcAddr; each is the core function, or the extension's where the
instance is 1.0-level and the extension stands in for it */
#define HOST_VK_INSTANCE_FUNCTIONS(X) \
	X(vkDestroyInstance) X(vkEnumeratePhysicalDevices) X(vkGetPhysicalDeviceProperties) \
	X(vkGetPhysicalDeviceQueueFamilyProperties) X(vkEnumerateDeviceExtensionProperties) \
	X(vkGetPhysicalDeviceFeatures2) X(vkGetPhysicalDeviceProperties2) X(vkGetDeviceProcAddr) \
	X(vkCreateAndroidSurfaceKHR) X(vkDestroySurfaceKHR) X(vkCreateDevice) \
	X(vkGetPhysicalDeviceMemoryProperties) X(vkGetPhysicalDeviceFormatProperties) \
	X(vkGetPhysicalDeviceSurfaceSupportKHR) X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
	X(vkGetPhysicalDeviceSurfaceFormatsKHR)

/* what the startup keeps: nothing in it is made twice */
struct host_vk
{
	PFN_vkGetInstanceProcAddr get_instance_proc;
	VkInstance instance;
	/* the version the instance was made for, and the one to use on the device:
	the lower of that and the device's own */
	uint32_t instance_api, api;
	VkPhysicalDevice physical;
	uint32_t queue_family;
	VkPhysicalDeviceProperties properties;
	/* the device extensions it lists, names only, VK_MAX_EXTENSION_NAME_SIZE each */
	char (*extensions)[VK_MAX_EXTENSION_NAME_SIZE];
	uint32_t extension_count;
	int validation;
	/* VkPhysicalDeviceDriverProperties.driverID, 0 if the driver does not say (names the pipeline cache's file) */
	uint32_t driver_id;
	VkDebugUtilsMessengerEXT messenger;
	/* the log line of the decision: which driver and device, for the device-lost report */
	char line[600];

#define X(name) PFN_##name name;
	HOST_VK_INSTANCE_FUNCTIONS(X)
#undef X
	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
};

/* valid after host_vk_startup returned 1; zeroed otherwise */
extern struct host_vk host_vk;

/* ---------- the backend (host_vk_render.c, host_vk_present.c)

Made at the first host_vk_submit, from what host_vk_startup kept. */

/* the device-level entry points: hidden, so that the library exports no symbol named
as a Vulkan function */
#define HOST_VK_DEVICE_FUNCTIONS(X) \
	X(vkDestroyDevice) X(vkGetDeviceQueue) X(vkDeviceWaitIdle) X(vkQueueSubmit) X(vkQueuePresentKHR) \
	X(vkAllocateMemory) X(vkFreeMemory) X(vkCreateImage) X(vkDestroyImage) X(vkGetImageMemoryRequirements) \
	X(vkBindImageMemory) X(vkCreateImageView) X(vkDestroyImageView) X(vkCreateCommandPool) \
	X(vkDestroyCommandPool) X(vkAllocateCommandBuffers) X(vkResetCommandPool) X(vkBeginCommandBuffer) \
	X(vkEndCommandBuffer) X(vkCreateFence) X(vkDestroyFence) X(vkWaitForFences) X(vkResetFences) \
	X(vkGetFenceStatus) X(vkCreateSemaphore) X(vkDestroySemaphore) X(vkCmdPipelineBarrier) \
	X(vkCmdClearAttachments) X(vkCmdClearColorImage) X(vkCmdBlitImage) X(vkCmdSetViewport) X(vkCmdSetScissor) \
	X(vkCmdBindPipeline) X(vkCmdPushConstants) X(vkCmdDraw) X(vkCreateShaderModule) X(vkDestroyShaderModule) \
	X(vkCreateGraphicsPipelines) X(vkDestroyPipeline) X(vkCreatePipelineLayout) X(vkDestroyPipelineLayout) \
	X(vkCreateSwapchainKHR) X(vkDestroySwapchainKHR) X(vkGetSwapchainImagesKHR) X(vkAcquireNextImageKHR) \
	X(vkCreateBuffer) X(vkDestroyBuffer) X(vkGetBufferMemoryRequirements) X(vkBindBufferMemory) X(vkMapMemory) \
	X(vkUnmapMemory) X(vkCmdCopyImageToBuffer) X(vkCmdBindVertexBuffers) \
	X(vkCreateDescriptorSetLayout) X(vkDestroyDescriptorSetLayout) X(vkCreatePipelineCache) X(vkDestroyPipelineCache) \
	X(vkGetPipelineCacheData) \
	X(vkCreateDescriptorPool) X(vkDestroyDescriptorPool) X(vkResetDescriptorPool) X(vkAllocateDescriptorSets) \
	X(vkUpdateDescriptorSets) X(vkCmdBindDescriptorSets) X(vkCmdBindIndexBuffer) X(vkCmdDrawIndexed) X(vkCmdSetDepthBias) \
	X(vkCmdSetBlendConstants) X(vkCmdSetStencilCompareMask) X(vkCmdSetStencilWriteMask) X(vkCmdSetStencilReference) \
	X(vkCreateSampler) X(vkDestroySampler) X(vkCmdCopyBufferToImage) X(vkCmdCopyImage) \
	X(vkCreateQueryPool) X(vkDestroyQueryPool) X(vkCmdResetQueryPool) X(vkCmdBeginQuery) X(vkCmdEndQuery) \
	X(vkCmdCopyQueryPoolResults)

#define X(name) extern PFN_##name name __attribute__((visibility("hidden")));
HOST_VK_DEVICE_FUNCTIONS(X)
#undef X
/* dynamic rendering: core in 1.3, the extension's before */
extern PFN_vkCmdBeginRenderingKHR host_vk_cmd_begin_rendering __attribute__((visibility("hidden")));
extern PFN_vkCmdEndRenderingKHR host_vk_cmd_end_rendering __attribute__((visibility("hidden")));

#define HOST_VK_FRAMES 2
#define HOST_VK_SWAPCHAIN_IMAGES 16
#define HOST_VK_TARGET_BUCKETS 64

/* a render target or depth buffer of the game, as an image */
struct host_vk_target
{
	struct vk_surface key;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkImageLayout layout;
	VkImageAspectFlags aspect;
	/* the number of the latest command that bound it (B.target_clock): which target a texture's address names, and whether a mip
	composite is out of date */
	uint64_t bound;
	/* drawn multisampled (display.anti_aliasing): the image of samples a pixel that renderings draw into instead, the colour's
	resolved into image as each ends; samples 0 when there is none */
	uint32_t samples;
	VkImage multisample_image;
	VkDeviceMemory multisample_memory;
	VkImageView multisample_view;
	VkImageLayout multisample_layout;
	struct host_vk_target *next_in_bucket;
};

struct host_vk_frame
{
	VkCommandPool pool;
	VkCommandBuffer command;
	VkFence fence;
	VkSemaphore acquired;
	int recording; /* the command buffer is open */
	int ring_valid; /* its upload ring has been reset for this use of the frame (host_vk_data.c) */
	struct host_vk_ring *ring;
	/* the descriptor pools a draw's sets come from (host_vk_draw.c): reset when the frame's fence has passed */
	VkDescriptorPool *pools;
	unsigned pool_count, pool_capacity, pool_current;
	int submitted; /* the fence is (to be) signalled by submission number */
	uint64_t number;
};

struct host_vk_swapchain
{
	VkSwapchainKHR handle;
	VkFormat format;
	VkExtent2D extent;
	uint32_t count;
	VkImage images[HOST_VK_SWAPCHAIN_IMAGES];
	VkSemaphore render_done[HOST_VK_SWAPCHAIN_IMAGES];
	VkImageView views[HOST_VK_SWAPCHAIN_IMAGES]; /* only with debug.vk_present_marker */
};

struct host_vk_backend
{
	int state; /* 0 not made yet, 1 ready, 2 not available (said why) */
	int dead; /* the device was lost: nothing more is recorded */
	VkDevice device;
	VkQueue queue;
	uint32_t family;
	VkSurfaceKHR surface;
	void *native_window; /* the ANativeWindow the surface was made on */
	VkPhysicalDeviceMemoryProperties memory;
	VkFormat color_format, depth_format;
	VkFilter blit_filter;
	/* the optional device features that were enabled */
	int sampler_anisotropy, fill_mode_non_solid, occlusion_query_precise, texture_compression_bc;
	VkFormatFeatureFlags swapchain_features;

	struct host_vk_frame frames[HOST_VK_FRAMES];
	int frame;
	uint64_t submission; /* the number of the latest submission */
	uint64_t retired; /* the highest number seen retired */
	pthread_mutex_t lock;

	struct host_vk_target *buckets[HOST_VK_TARGET_BUCKETS];
	unsigned images;
	struct host_vk_target *color, *depth; /* bound now (NULL: none) */
	uint32_t samples; /* and the samples a pixel they are drawn with (0: one) */
	uint64_t target_clock;
	struct host_vk_target *last_color, *last_depth; /* the pair a command last named, for the statistics */
	int rendering; /* a rendering is open on them */
	uint32_t area_width, area_height; /* its render area */

	struct host_vk_swapchain chain;
	uint64_t lost_at; /* from the loss of the surface until it is made again */
	unsigned lost_attempts;
	unsigned presented_since_lost;

	/* what every draw's pipeline is made with (host_vk_shaders.c): set 0 as vk_shaders.h's bindings say, and the one
	cache every pipeline is made in, saved per driver */
	VkDescriptorSetLayout draw_set_layout;
	VkPipelineLayout draw_layout;
	VkPipelineCache pipeline_cache;

	/* the built-in pipeline of clears of some channels only */
	VkPipelineLayout clear_layout;
	VkShaderModule clear_vertex, clear_fragment;
	/* by colour write mask, whether a depth buffer is bound, and the samples a pixel (1, 2, 4, 8) */
	VkPipeline clear_pipelines[16][2][4];

	/* counted for the log line every 60 frames, and in total */
	struct host_vk_counts
	{
		unsigned frames, hand_overs, targets, clears, presents, clears_drawn, clears_attachments, target_changes;
		unsigned recreations;
		unsigned data_records, data_bytes;
		unsigned draws_ready, draws_skipped_shader, draws_skipped_pipeline;
		unsigned draws_made, draws_skipped_target, draws_skipped_data, draws_skipped_other, draws_texture_missing;
		unsigned textures_skipped, texture_bytes;
	} counts;
};

extern struct host_vk_backend host_vkb;

/* host_vk_texture.c: the guest's textures as images */
struct host_vk_image
{
	uint32_t id, kind, format, width, height, depth, levels, layers;
	VkImage image;
	VkImageView view;
	VkImageLayout layout;
	unsigned block;
	VkDeviceSize offset, size;
	int has_memory;
};
void host_vk_texture_command(const struct vk_command_texture *command);
void host_vk_texture_data_command(const struct vk_command_texture_data *command);
void host_vk_texture_free_command(const struct vk_command_texture_free *command);
/* the images written since a draw last needed them are made readable (outside a rendering: ends the one open) */
void host_vk_textures_flush(void);
/* a frame slot's fence has passed: the images retired in it are destroyed */
void host_vk_texture_frame_reset(unsigned slot);
/* the image of a guest's id, or NULL */
struct host_vk_image *host_vk_image_get(uint32_t id);
unsigned host_vk_texture_images(void);
unsigned host_vk_texture_megabytes(void);

/* host_vk_visibility.c: the visibility tests, as occlusion queries */
void host_vk_visibility_start(void);
void host_vk_visibility_begin(void);
void host_vk_visibility_end(const struct vk_command_visibility_end *end);
/* with a rendering open, at each draw: the query of a test being made is begun */
void host_vk_visibility_draw(VkCommandBuffer command);
/* a part of a test that is open is ended (before its rendering ends) */
void host_vk_visibility_close(VkCommandBuffer command);
/* the frame's end, outside a rendering: its tests' results are copied */
void host_vk_visibility_frame_end(VkCommandBuffer command);
/* the frame slot's fence has passed: its tests' counts become the slots' latest */
void host_vk_visibility_retired(unsigned slot);

/* host_vk_draw.c: the mip composites */
void host_vk_composite_command(const struct vk_command_composite *command);
/* the colour target bound last at a physical address (NULL if none) */
struct host_vk_target *host_vk_target_at(uint32_t data);

/* host_vk_draw.c: the draws */
void host_vk_command_draw(const struct vk_command_draw *draw, uint32_t size);
/* the frame's descriptor pools are reset, its fence having passed (host_vk_frame_command) */
void host_vk_draw_frame_reset(struct host_vk_frame *frame);
/* the draw's pools and samplers are let go when the backend is */
/* opens a rendering on the bound targets if there is none open; 0 if there are no targets */
int host_vk_rendering_begin(void);

/* host_vk_render.c: records a command buffer and logs; check() reports a failed call and
stops the backend if the device was lost */
int host_vk_check(VkResult result, const char *call);
#define HOST_VK_CHECK(call) host_vk_check((call), #call)
void host_vk_wait_fence(VkFence fence, const char *what, uint64_t number);
/* the transition of a target to a layout (outside a rendering) */
void host_vk_target_transition(VkCommandBuffer command, struct host_vk_target *target, VkImageLayout layout);
/* ends the open rendering, if any */
void host_vk_rendering_end(void);
/* the target for a surface of a command, made (and cleared) the first time; NULL if there is none */
struct host_vk_target *host_vk_target_get(const struct vk_surface *surface);
/* the current frame's command buffer, open for recording */
VkCommandBuffer host_vk_frame_command(void);
/* the samples a pixel the device can draw colour and depth with: the most it has up to samples (1 for none) */
uint32_t host_vk_samples_usable(uint32_t samples);
/* a shader module from GLSL (glslang), or VK_NULL_HANDLE (said why) */
VkShaderModule host_vk_shader_module(const char *source, int fragment, const char *what);

/* host_vk_post.c: display.anti_aliasing's FXAA over a rectangle of a target (VK_COMMAND_ANTI_ALIAS) */
void host_vk_anti_alias_command(const struct vk_command_anti_alias *command);

/* a memory type allowed by bits with the properties wanted, else the first allowed; UINT32_MAX if none */
uint32_t host_vk_memory_type(uint32_t bits, VkMemoryPropertyFlags preferred);
/* the upload rings and the frame's data table (host_vk_data.c): the guest's DATA records land here */
/* a DATA record (size bytes): placed in the frame's ring. 0 if it is bad (said once): the rest of the
hand-over is dropped */
int host_vk_data_command(const struct vk_command_data *command, uint32_t size);
/* a command's data: the buffer and the offset in it of size bytes at offset in the data of id; 0 (said once)
if the id or the range is not known */
int host_vk_data_find(uint32_t id, uint32_t offset, uint32_t size, VkBuffer *buffer, VkDeviceSize *buffer_offset);
/* the frame's data is let go (PRESENT): its ring is reset when the frame is used again */
void host_vk_data_frame_end(void);
/* ring buffers alive, in all frames */
unsigned host_vk_data_buffers(void);
/* a buffer of host-visible, coherent memory, mapped; 0 (said) if it cannot be made */
int host_vk_buffer_make(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer *buffer, VkDeviceMemory *memory, void **mapped);

/* glslang (host_vk_shaders.c), one at a time: GLSL (a vertex or a fragment shader) to SPIR-V for Vulkan 1.0, in memory
malloc'd for the caller; 0 with glslang's message otherwise. Loaded on first use from libhalo_glslang.so */
int host_vk_glslang_compile(const char *source, int fragment, uint32_t **words, size_t *count, char *message,
	size_t message_size);

/* the shader and pipeline services (host_vk_shaders.c). Started at the device's creation, with the draw layout
and the pipeline cache made by the caller */
void host_vk_services_start(void);
/* the pipeline a draw asks for (VK_COMMAND_PIPELINE); B.lock is held */
void host_vk_pipeline_command(const struct vk_command_pipeline *command);
/* the pipeline for a draw's two shader handles and state, queued if new: the VkPipeline once it is ready, else VK_NULL_HANDLE
(counted as skipped); B.lock is held */
VkPipeline host_vk_pipeline_find(uint32_t vertex_handle, uint32_t pixel_handle, const struct vk_pipeline_state *state);
/* the statistics line's part, and the services' tick (a frame): the cache is saved every five minutes if pipelines
were made since */
void host_vk_services_statistics(char *text, size_t size);
void host_vk_services_tick(void);
/* the pipeline cache is written to the device (why says what asked: the log's line) */
void host_vk_pipeline_cache_save(const char *why);
/* host_vk_render.c: the device made if it is not yet, B.lock held */
void host_vk_device_ensure_locked(void);
/* the backend's device is made, if it can be: the services' imports need it. 1 if it is ready */
int host_vk_backend_ensure(void);

/* host_vk_present.c */
/* the surface and swapchain for the window as it is now; 0 if there is no window to make one on */
int host_vk_surface_make(int quiet);
/* the loss of the surface or of the swapchain's use: counted and logged once, however many tries it takes */
void host_vk_surface_lost(const char *where);
/* called with the frame's command buffer open, after the game's rendering: acquires a swapchain image
and records the blit of the back buffer into it. Returns the image's index, or UINT32_MAX when there is
nothing to present to (no swapchain, surface lost) */
uint32_t host_vk_present_record(VkCommandBuffer command, struct host_vk_target *back_buffer);
/* after the frame was submitted: presents the image acquired by host_vk_present_record */
void host_vk_present_queue(uint32_t image_index);
void host_vk_present_destroy(void);

#endif
