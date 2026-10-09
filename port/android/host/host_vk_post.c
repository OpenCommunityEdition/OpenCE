/*
HOST_VK_POST.C

The Vulkan renderer's anti-aliasing pass (display.anti_aliasing's "fxaa"): VK_COMMAND_ANTI_ALIAS
(port/android/guest/vk_commands.h), FXAA over the rectangle of a window's 3D view in a colour target, as
port/linux/src/xgpu_post.c draws it for the OpenGL renderers. The target's pixels are copied into an image of the pass's
own first, so that the pass reads none of what it writes; one triangle covering the rectangle (its corners from
gl_VertexIndex, no vertex data) finds its texels from gl_FragCoord, and only colour is written back: the game keeps values
of its own in destination alpha. A target's rows are the picture's from the top, as the copy's are, so no coordinate is
turned over.

The pass's pipeline is made the first time it is asked for; if it cannot be, the view is drawn as it is (said once).
*/

#include "host.h"
#include "host_vk.h"

#include <stdlib.h>
#include <string.h>

#define B host_vkb

static struct
{
	int made, failed;
	VkShaderModule vertex, fragment;
	VkDescriptorSetLayout set_layout;
	VkPipelineLayout layout;
	VkPipeline pipeline;
	VkSampler sampler;
	VkDescriptorPool pool;
	VkDescriptorSet set;
	/* the target's pixels before the pass, at the size of the target last passed over */
	VkImage copy;
	VkDeviceMemory copy_memory;
	VkImageView copy_view;
	VkImageLayout copy_layout;
	uint32_t width, height;
} post;

/* one triangle over the whole viewport */
static const char vertex_source[] =
	"#version 450\n"
	"void main()\n"
	"{\n"
	"\tgl_Position = vec4(float((gl_VertexIndex & 1) << 2) - 1.0, float((gl_VertexIndex & 2) << 1) - 1.0, 0.0, 1.0);\n"
	"}\n";

/* xgpu_post.c's fxaa_source, its uniforms a push constant block and its sampler at binding 0 */
static const char fragment_source[] =
	"#version 450\n"
	"layout(set = 0, binding = 0) uniform sampler2D color_texture;\n"
	"layout(push_constant) uniform pass\n"
	"{\n"
	"\t/* 1 / width, 1 / height, width, height */\n"
	"\tvec4 metrics;\n"
	"\t/* the rectangle's outermost texel centres, as texture coordinates */\n"
	"\tvec4 bounds;\n"
	"};\n"
	"layout(location = 0) out vec4 result;\n"
	"\n"
	"const float EDGE_THRESHOLD = 0.125;\n"
	"const float EDGE_THRESHOLD_MINIMUM = 0.0312;\n"
	"const float SUBPIXEL_QUALITY = 0.75;\n"
	"const int STEPS = 12;\n"
	"const float STEP_LENGTHS[12] = float[12](1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0);\n"
	"\n"
	"vec3 color_at(vec2 position)\n"
	"{\n"
	"\treturn textureLod(color_texture, clamp(position, bounds.xy, bounds.zw), 0.0).rgb;\n"
	"}\n"
	"\n"
	"float luma_at(vec2 position)\n"
	"{\n"
	"\treturn dot(color_at(position), vec3(0.299, 0.587, 0.114));\n"
	"}\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tvec2 position = gl_FragCoord.xy * metrics.xy;\n"
	"\tfloat luma = luma_at(position);\n"
	"\tfloat up = luma_at(position + vec2(0.0, -metrics.y));\n"
	"\tfloat down = luma_at(position + vec2(0.0, metrics.y));\n"
	"\tfloat left = luma_at(position + vec2(-metrics.x, 0.0));\n"
	"\tfloat right = luma_at(position + vec2(metrics.x, 0.0));\n"
	"\tfloat highest = max(luma, max(max(up, down), max(left, right)));\n"
	"\tfloat range = highest - min(luma, min(min(up, down), min(left, right)));\n"
	"\n"
	"\t/* no edge: the target has the pixel already */\n"
	"\tif (range < max(EDGE_THRESHOLD_MINIMUM, highest * EDGE_THRESHOLD))\n"
	"\t\tdiscard;\n"
	"\tfloat up_left = luma_at(position + vec2(-metrics.x, -metrics.y));\n"
	"\tfloat up_right = luma_at(position + vec2(metrics.x, -metrics.y));\n"
	"\tfloat down_left = luma_at(position + vec2(-metrics.x, metrics.y));\n"
	"\tfloat down_right = luma_at(position + vec2(metrics.x, metrics.y));\n"
	"\n"
	"\t/* how much the pixel stands out from its neighbourhood (a feature\n"
	"\tthinner than a pixel) */\n"
	"\tfloat average = (2.0 * (up + down + left + right) + up_left + up_right + down_left + down_right) / 12.0;\n"
	"\tfloat subpixel = smoothstep(0.0, 1.0, clamp(abs(average - luma) / range, 0.0, 1.0));\n"
	"\tsubpixel = subpixel * subpixel * SUBPIXEL_QUALITY;\n"
	"\n"
	"\t/* an edge along the rows where the luma changes more from row to row\n"
	"\tthan from column to column; across it, the side it changes more to */\n"
	"\tbool horizontal = abs(up_left + down_left - 2.0 * left) + 2.0 * abs(up + down - 2.0 * luma) +\n"
	"\t\tabs(up_right + down_right - 2.0 * right) >=\n"
	"\t\tabs(up_left + up_right - 2.0 * up) + 2.0 * abs(left + right - 2.0 * luma) +\n"
	"\t\tabs(down_left + down_right - 2.0 * down);\n"
	"\tfloat before = horizontal ? up : left;\n"
	"\tfloat after = horizontal ? down : right;\n"
	"\tbool before_steeper = abs(before - luma) >= abs(after - luma);\n"
	"\tfloat gradient = 0.25 * max(abs(before - luma), abs(after - luma));\n"
	"\tfloat edge_luma = 0.5 * ((before_steeper ? before : after) + luma);\n"
	"\tvec2 across = (horizontal ? vec2(0.0, metrics.y) : vec2(metrics.x, 0.0)) * (before_steeper ? -1.0 : 1.0);\n"
	"\tvec2 along = horizontal ? vec2(metrics.x, 0.0) : vec2(0.0, metrics.y);\n"
	"\n"
	"\t/* along the edge, half a pixel across, to where its luma ends either way */\n"
	"\tvec2 end_before = position + 0.5 * across - along;\n"
	"\tvec2 end_after = position + 0.5 * across + along;\n"
	"\tfloat luma_before = luma_at(end_before) - edge_luma;\n"
	"\tfloat luma_after = luma_at(end_after) - edge_luma;\n"
	"\tbool reached_before = abs(luma_before) >= gradient;\n"
	"\tbool reached_after = abs(luma_after) >= gradient;\n"
	"\tfor (int index = 1; index < STEPS && !(reached_before && reached_after); index++)\n"
	"\t{\n"
	"\t\tif (!reached_before)\n"
	"\t\t{\n"
	"\t\t\tend_before -= along * STEP_LENGTHS[index];\n"
	"\t\t\tluma_before = luma_at(end_before) - edge_luma;\n"
	"\t\t\treached_before = abs(luma_before) >= gradient;\n"
	"\t\t}\n"
	"\t\tif (!reached_after)\n"
	"\t\t{\n"
	"\t\t\tend_after += along * STEP_LENGTHS[index];\n"
	"\t\t\tluma_after = luma_at(end_after) - edge_luma;\n"
	"\t\t\treached_after = abs(luma_after) >= gradient;\n"
	"\t\t}\n"
	"\t}\n"
	"\n"
	"\t/* blended across the edge the more, the nearer the pixel is to an end;\n"
	"\tonly where the luma at that end changes the way the pixel's does */\n"
	"\tfloat distance_before = horizontal ? position.x - end_before.x : position.y - end_before.y;\n"
	"\tfloat distance_after = horizontal ? end_after.x - position.x : end_after.y - position.y;\n"
	"\tfloat pixel_offset = 0.5 - min(distance_before, distance_after) / (distance_before + distance_after);\n"
	"\tbool varies = ((distance_before < distance_after ? luma_before : luma_after) < 0.0) != (luma < edge_luma);\n"
	"\tresult = vec4(color_at(position + max(varies ? pixel_offset : 0.0, subpixel) * across), 1.0);\n"
	"}\n";

static void barrier(VkCommandBuffer command, VkImage image, VkImageLayout from, VkImageLayout to)
{
	VkImageMemoryBarrier info = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

	info.srcAccessMask = from == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	info.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	info.oldLayout = from;
	info.newLayout = to;
	info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	info.image = image;
	info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL,
		1, &info);
}

/* the pass's pipeline, its layout, sampler and descriptor set; 0 if they cannot be made (said once) */
static int pass_make(void)
{
	VkDescriptorSetLayoutBinding binding;
	VkDescriptorSetLayoutCreateInfo set_layout = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	VkPushConstantRange range;
	VkPipelineLayoutCreateInfo layout = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	VkSamplerCreateInfo sampler = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	VkDescriptorPoolSize size;
	VkDescriptorPoolCreateInfo pool = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	VkDescriptorSetAllocateInfo allocate = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
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

	if (post.made)
		return 1;
	if (post.failed)
		return 0;
	post.failed = 1;
	post.vertex = host_vk_shader_module(vertex_source, 0, "anti-aliasing pass's vertex shader");
	post.fragment = host_vk_shader_module(fragment_source, 1, "anti-aliasing pass's fragment shader (FXAA)");
	if (!post.vertex || !post.fragment)
		goto failed;

	memset(&binding, 0, sizeof(binding));
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	set_layout.bindingCount = 1;
	set_layout.pBindings = &binding;
	if (!HOST_VK_CHECK(vkCreateDescriptorSetLayout(B.device, &set_layout, NULL, &post.set_layout)))
		goto failed;
	range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	range.offset = 0;
	range.size = 32;
	layout.setLayoutCount = 1;
	layout.pSetLayouts = &post.set_layout;
	layout.pushConstantRangeCount = 1;
	layout.pPushConstantRanges = &range;
	if (!HOST_VK_CHECK(vkCreatePipelineLayout(B.device, &layout, NULL, &post.layout)))
		goto failed;
	/* linear and clamped, as FXAA samples */
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.maxLod = 0.25f;
	if (!HOST_VK_CHECK(vkCreateSampler(B.device, &sampler, NULL, &post.sampler)))
		goto failed;
	size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	size.descriptorCount = 1;
	pool.maxSets = 1;
	pool.poolSizeCount = 1;
	pool.pPoolSizes = &size;
	if (!HOST_VK_CHECK(vkCreateDescriptorPool(B.device, &pool, NULL, &post.pool)))
		goto failed;
	allocate.descriptorPool = post.pool;
	allocate.descriptorSetCount = 1;
	allocate.pSetLayouts = &post.set_layout;
	if (!HOST_VK_CHECK(vkAllocateDescriptorSets(B.device, &allocate, &post.set)))
		goto failed;

	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = post.vertex;
	stages[0].pName = "main";
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = post.fragment;
	stages[1].pName = "main";
	assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	viewport.viewportCount = 1;
	viewport.scissorCount = 1;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = VK_CULL_MODE_NONE;
	raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	raster.lineWidth = 1.0f;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	/* colour only: the game's own values in destination alpha are kept */
	memset(&blend_attachment, 0, sizeof(blend_attachment));
	blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
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
	info.pDepthStencilState = &depth_stencil;
	info.pColorBlendState = &blend;
	info.pDynamicState = &dynamic;
	info.layout = post.layout;
	info.basePipelineIndex = -1;
	if (!HOST_VK_CHECK(vkCreateGraphicsPipelines(B.device, B.pipeline_cache, 1, &info, NULL, &post.pipeline)))
		goto failed;
	post.made = 1;
	post.failed = 0;
	host_logf(HOST_LOG_INFO, "vk: the anti-aliasing pass (FXAA) is ready");
	return 1;
failed:
	host_logf(HOST_LOG_ERROR, "vk: the anti-aliasing pass cannot be made; the 3D view is drawn as it is");
	return 0;
}

/* the copy image at the target's size, its view in the pass's descriptor set; 0 if it cannot be made */
static int copy_fit(uint32_t width, uint32_t height)
{
	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	VkImageViewCreateInfo view = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocation = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	VkDescriptorImageInfo image;
	VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

	if (post.copy && post.width == width && post.height == height)
		return 1;
	/* (a new size comes with a new screen, between frames: what used the old image has only to finish) */
	if (post.copy)
	{
		HOST_VK_CHECK(vkDeviceWaitIdle(B.device));
		vkDestroyImageView(B.device, post.copy_view, NULL);
		vkDestroyImage(B.device, post.copy, NULL);
		vkFreeMemory(B.device, post.copy_memory, NULL);
		post.copy_view = VK_NULL_HANDLE;
		post.copy = VK_NULL_HANDLE;
		post.copy_memory = VK_NULL_HANDLE;
	}
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = B.color_format;
	info.extent.width = width;
	info.extent.height = height;
	info.extent.depth = 1;
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (!HOST_VK_CHECK(vkCreateImage(B.device, &info, NULL, &post.copy)))
		return 0;
	vkGetImageMemoryRequirements(B.device, post.copy, &requirements);
	allocation.allocationSize = requirements.size;
	allocation.memoryTypeIndex = host_vk_memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocation.memoryTypeIndex == UINT32_MAX ||
		!HOST_VK_CHECK(vkAllocateMemory(B.device, &allocation, NULL, &post.copy_memory)) ||
		!HOST_VK_CHECK(vkBindImageMemory(B.device, post.copy, post.copy_memory, 0)))
		return 0;
	view.image = post.copy;
	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.format = info.format;
	view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.levelCount = 1;
	view.subresourceRange.layerCount = 1;
	if (!HOST_VK_CHECK(vkCreateImageView(B.device, &view, NULL, &post.copy_view)))
		return 0;
	post.copy_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	post.width = width;
	post.height = height;
	image.sampler = post.sampler;
	image.imageView = post.copy_view;
	image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	write.dstSet = post.set;
	write.dstBinding = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.pImageInfo = &image;
	vkUpdateDescriptorSets(B.device, 1, &write, 0, NULL);
	return 1;
}

void host_vk_anti_alias_command(const struct vk_command_anti_alias *command)
{
	struct host_vk_target *target;
	VkCommandBuffer cmd;
	VkImageCopy copy;
	VkRenderingAttachmentInfo color = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	VkRenderingInfo info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
	VkViewport viewport;
	VkRect2D rectangle;
	float constants[8];
	int64_t x0, y0, x1, y1;

	if (command->target.kind != VK_SURFACE_COLOR)
		return;
	target = host_vk_target_get(&command->target);
	/* (multisampled, it is not the pass's: the setting has one or the other) */
	if (!target || target->samples)
		return;
	x0 = command->rectangle[0];
	y0 = command->rectangle[1];
	x1 = x0 + command->rectangle[2];
	y1 = y0 + command->rectangle[3];
	if (x0 < 0)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (x1 > (int64_t)target->key.pixel_width)
		x1 = target->key.pixel_width;
	if (y1 > (int64_t)target->key.pixel_height)
		y1 = target->key.pixel_height;
	if (x0 >= x1 || y0 >= y1)
		return;
	if (!pass_make())
		return;
	host_vk_rendering_end();
	if (!copy_fit(target->key.pixel_width, target->key.pixel_height))
		return;
	cmd = host_vk_frame_command();

	/* the target's pixels in the rectangle, read while it is drawn into */
	host_vk_target_transition(cmd, target, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	barrier(cmd, post.copy, post.copy_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	memset(&copy, 0, sizeof(copy));
	copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.srcSubresource.layerCount = 1;
	copy.srcOffset.x = (int32_t)x0;
	copy.srcOffset.y = (int32_t)y0;
	copy.dstSubresource = copy.srcSubresource;
	copy.dstOffset = copy.srcOffset;
	copy.extent.width = (uint32_t)(x1 - x0);
	copy.extent.height = (uint32_t)(y1 - y0);
	copy.extent.depth = 1;
	vkCmdCopyImage(cmd, target->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, post.copy, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
		&copy);
	barrier(cmd, post.copy, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	post.copy_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	/* the pass, into the rectangle */
	host_vk_target_transition(cmd, target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	color.imageView = target->view;
	color.imageLayout = target->layout;
	color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	info.renderArea.extent.width = target->key.pixel_width;
	info.renderArea.extent.height = target->key.pixel_height;
	info.layerCount = 1;
	info.colorAttachmentCount = 1;
	info.pColorAttachments = &color;
	host_vk_cmd_begin_rendering(cmd, &info);
	viewport.x = (float)x0;
	viewport.y = (float)y0;
	viewport.width = (float)(x1 - x0);
	viewport.height = (float)(y1 - y0);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	rectangle.offset.x = (int32_t)x0;
	rectangle.offset.y = (int32_t)y0;
	rectangle.extent.width = (uint32_t)(x1 - x0);
	rectangle.extent.height = (uint32_t)(y1 - y0);
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &rectangle);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, post.pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, post.layout, 0, 1, &post.set, 0, NULL);
	/* metrics, and the rectangle's outermost texel centres (split screen: no texel of the next window is read) */
	constants[0] = 1.0f / (float)post.width;
	constants[1] = 1.0f / (float)post.height;
	constants[2] = (float)post.width;
	constants[3] = (float)post.height;
	constants[4] = ((float)x0 + 0.5f) / (float)post.width;
	constants[5] = ((float)y0 + 0.5f) / (float)post.height;
	constants[6] = ((float)x1 - 0.5f) / (float)post.width;
	constants[7] = ((float)y1 - 0.5f) / (float)post.height;
	vkCmdPushConstants(cmd, post.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants), constants);
	vkCmdDraw(cmd, 3, 1, 0, 0);
	host_vk_cmd_end_rendering(cmd);
}
