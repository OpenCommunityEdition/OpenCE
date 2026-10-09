/*
HOST_VK_TEXTURE.C

The Vulkan renderer's host half: the guest's textures as images. The guest numbers an
image and describes it (VK_COMMAND_TEXTURE), then sends its texels a level of a face at a time (VK_COMMAND_TEXTURE_DATA), which
were put in the frame's upload ring when the cache uploaded the texture; the host copies them from the ring into the image
(vkCmdCopyBufferToImage) outside any rendering, after a barrier from the image's last use (draws of this frame recorded
before may still sample its old texels) and before one to the shader-read layout, made when the next draw needs the image.

Every image is tracked in one of three layouts (undefined, transfer destination, shader read). A texture the cache drops is
destroyed once the frames that may use it have passed (the frame slot's fence, host_vk_texture_frame_reset). The images'
memory is suballocated from blocks, because a map has more textures than a device allows allocations (4096 on Adreno).
*/

#include "host.h"
#include "host_vk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B host_vkb

/* ---------- memory: blocks, first fit, the free ranges kept sorted and merged */

struct range
{
	VkDeviceSize offset, size;
};

struct block
{
	VkDeviceMemory memory;
	uint32_t type;
	VkDeviceSize size;
	struct range *free;
	unsigned count, capacity;
};

#define BLOCK_SIZE (64ull * 1024 * 1024)

static struct block *blocks;
static unsigned block_count, block_capacity;
static VkDeviceSize memory_bytes;

static VkDeviceSize align_up(VkDeviceSize value, VkDeviceSize align)
{
	return (value + align - 1) / align * align;
}

static int range_insert(struct block *block, unsigned at, VkDeviceSize offset, VkDeviceSize size)
{
	if (block->count == block->capacity)
	{
		unsigned capacity = block->capacity ? block->capacity * 2 : 16;
		struct range *free = realloc(block->free, sizeof(*free) * capacity);

		if (!free)
			return 0;
		block->free = free;
		block->capacity = capacity;
	}
	memmove(&block->free[at + 1], &block->free[at], (block->count - at) * sizeof(*block->free));
	block->free[at].offset = offset;
	block->free[at].size = size;
	block->count++;
	return 1;
}

/* a place for size bytes at align in the block, or 0 */
static int block_alloc(struct block *block, VkDeviceSize size, VkDeviceSize align, VkDeviceSize *offset)
{
	unsigned index;

	for (index = 0; index < block->count; index++)
	{
		VkDeviceSize start = align_up(block->free[index].offset, align);
		VkDeviceSize end = start + size, old_end = block->free[index].offset + block->free[index].size;

		if (end > old_end)
			continue;
		if (start > block->free[index].offset)
		{
			/* the part before stays; the part after is a range of its own */
			block->free[index].size = start - block->free[index].offset;
			if (end < old_end && !range_insert(block, index + 1, end, old_end - end))
				return 0;
		}
		else if (end < old_end)
		{
			block->free[index].offset = end;
			block->free[index].size = old_end - end;
		}
		else
		{
			memmove(&block->free[index], &block->free[index + 1], (block->count - index - 1) * sizeof(*block->free));
			block->count--;
		}
		*offset = start;
		return 1;
	}
	return 0;
}

static void block_free(struct block *block, VkDeviceSize offset, VkDeviceSize size)
{
	unsigned at = 0;

	while (at < block->count && block->free[at].offset < offset)
		at++;
	if (at > 0 && block->free[at - 1].offset + block->free[at - 1].size == offset)
	{
		block->free[at - 1].size += size;
		if (at < block->count && block->free[at - 1].offset + block->free[at - 1].size == block->free[at].offset)
		{
			block->free[at - 1].size += block->free[at].size;
			memmove(&block->free[at], &block->free[at + 1], (block->count - at - 1) * sizeof(*block->free));
			block->count--;
		}
		return;
	}
	if (at < block->count && offset + size == block->free[at].offset)
	{
		block->free[at].offset = offset;
		block->free[at].size += size;
		return;
	}
	range_insert(block, at, offset, size);
}

/* memory for an image: the block and the offset in it; 0 if none can be made */
static int memory_alloc(const VkMemoryRequirements *requirements, unsigned *block_index, VkDeviceSize *offset)
{
	uint32_t type = host_vk_memory_type(requirements->memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	unsigned index;
	struct block *block;
	VkMemoryAllocateInfo allocation = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	VkDeviceSize size;

	if (type == UINT32_MAX)
		return 0;
	for (index = 0; index < block_count; index++)
	{
		if (blocks[index].type == type && block_alloc(&blocks[index], requirements->size, requirements->alignment, offset))
		{
			*block_index = index;
			return 1;
		}
	}
	if (block_count == block_capacity)
	{
		unsigned capacity = block_capacity ? block_capacity * 2 : 8;
		struct block *grown = realloc(blocks, sizeof(*grown) * capacity);

		if (!grown)
			return 0;
		blocks = grown;
		block_capacity = capacity;
	}
	size = requirements->size > BLOCK_SIZE ? requirements->size : BLOCK_SIZE;
	allocation.allocationSize = size;
	allocation.memoryTypeIndex = type;
	block = &blocks[block_count];
	memset(block, 0, sizeof(*block));
	if (!HOST_VK_CHECK(vkAllocateMemory(B.device, &allocation, NULL, &block->memory)))
		return 0;
	block->type = type;
	block->size = size;
	if (!range_insert(block, 0, 0, size) || !block_alloc(block, requirements->size, requirements->alignment, offset))
	{
		vkFreeMemory(B.device, block->memory, NULL);
		return 0;
	}
	memory_bytes += size;
	*block_index = block_count++;
	return 1;
}

/* ---------- images */

static struct host_vk_image *images[VK_TEXTURE_LIMIT + 1];
static unsigned image_count;
/* in the transfer destination layout, to be made readable before a draw samples them */
static struct host_vk_image **dirty;
static unsigned dirty_count, dirty_capacity;
/* destroyed when the slot's frame has passed */
static struct host_vk_image **graveyard[HOST_VK_FRAMES];
static unsigned graveyard_count[HOST_VK_FRAMES], graveyard_capacity[HOST_VK_FRAMES];

static VkFormat format_of(uint32_t format)
{
	switch (format)
	{
	case VK_IMAGE_FORMAT_BC1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case VK_IMAGE_FORMAT_BC2: return VK_FORMAT_BC2_UNORM_BLOCK;
	case VK_IMAGE_FORMAT_BC3: return VK_FORMAT_BC3_UNORM_BLOCK;
	default: return VK_FORMAT_B8G8R8A8_UNORM;
	}
}

static unsigned level_dimension(unsigned base, unsigned level)
{
	unsigned value = base >> level;

	return value ? value : 1;
}

/* the bytes of a level (all its slices) as the guest sends them */
static uint32_t level_bytes(const struct host_vk_image *image, unsigned level)
{
	unsigned width = level_dimension(image->width, level), height = level_dimension(image->height, level);
	unsigned depth = level_dimension(image->depth, level);

	if (image->format == VK_IMAGE_FORMAT_BGRA)
		return width * height * depth * 4;
	return ((width + 3) / 4) * ((height + 3) / 4) * (image->format == VK_IMAGE_FORMAT_BC1 ? 8 : 16) * depth;
}

static void barrier(VkCommandBuffer command, struct host_vk_image *image, VkImageLayout to)
{
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

	barrier.srcAccessMask = image->layout == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.oldLayout = image->layout;
	barrier.newLayout = to;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image->image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = image->levels;
	barrier.subresourceRange.layerCount = image->layers;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL,
		1, &barrier);
	image->layout = to;
}

static void image_destroy(struct host_vk_image *image)
{
	if (image->view)
		vkDestroyImageView(B.device, image->view, NULL);
	if (image->image)
		vkDestroyImage(B.device, image->image, NULL);
	if (image->has_memory)
		block_free(&blocks[image->block], image->offset, image->size);
	free(image);
}

static void image_dirty_remove(struct host_vk_image *image)
{
	unsigned index;

	for (index = 0; index < dirty_count; index++)
	{
		if (dirty[index] == image)
		{
			dirty[index] = dirty[--dirty_count];
			return;
		}
	}
}

/* the image goes to the graveyard of this frame's slot: the frames that may use it are the two in flight, and the slot's
fence passing means both are done (the other was submitted before it) */
static void image_retire(struct host_vk_image *image)
{
	unsigned slot = (unsigned)B.frame;

	image_dirty_remove(image);
	if (graveyard_count[slot] == graveyard_capacity[slot])
	{
		unsigned capacity = graveyard_capacity[slot] ? graveyard_capacity[slot] * 2 : 64;
		struct host_vk_image **grown = realloc(graveyard[slot], sizeof(*grown) * capacity);

		if (!grown)
		{
			image_destroy(image);
			return;
		}
		graveyard[slot] = grown;
		graveyard_capacity[slot] = capacity;
	}
	graveyard[slot][graveyard_count[slot]++] = image;
}

void host_vk_texture_frame_reset(unsigned slot)
{
	unsigned index;

	for (index = 0; index < graveyard_count[slot]; index++)
		image_destroy(graveyard[slot][index]);
	graveyard_count[slot] = 0;
}

struct host_vk_image *host_vk_image_get(uint32_t id)
{
	return id >= 1 && id <= VK_TEXTURE_LIMIT ? images[id] : NULL;
}

void host_vk_texture_command(const struct vk_command_texture *command)
{
	struct host_vk_image *image;
	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	VkImageViewCreateInfo view = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	VkMemoryRequirements requirements;
	VkFormat format;
	uint32_t layers;

	if (command->id < 1 || command->id > VK_TEXTURE_LIMIT || !command->width || !command->height || !command->depth ||
		!command->levels || command->levels > 16 || command->kind < VK_IMAGE_2D || command->kind > VK_IMAGE_CUBE)
	{
		host_logf(HOST_LOG_ERROR, "vk: a texture command is bad (id %u, kind %u, %ux%ux%u, %u levels)", (unsigned)command->id,
			(unsigned)command->kind, (unsigned)command->width, (unsigned)command->height, (unsigned)command->depth,
			(unsigned)command->levels);
		return;
	}
	image = images[command->id];
	if (image && image->kind == command->kind && image->format == command->format && image->width == command->width &&
		image->height == command->height && image->depth == command->depth && image->levels == command->levels)
		return; /* the same image: its texels are replaced by the data that follows */
	if (image)
	{
		images[command->id] = NULL;
		image_count--;
		image_retire(image);
	}
	format = format_of(command->format);
	layers = command->kind == VK_IMAGE_CUBE ? 6 : 1;
	image = calloc(1, sizeof(*image));
	if (!image)
		return;
	image->id = command->id;
	image->kind = command->kind;
	image->format = command->format;
	image->width = command->width;
	image->height = command->height;
	image->depth = command->kind == VK_IMAGE_3D ? command->depth : 1;
	image->levels = command->levels;
	image->layers = layers;
	image->layout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.imageType = command->kind == VK_IMAGE_3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	info.flags = command->kind == VK_IMAGE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	info.format = format;
	info.extent.width = command->width;
	info.extent.height = command->height;
	info.extent.depth = image->depth;
	info.mipLevels = command->levels;
	info.arrayLayers = layers;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (!HOST_VK_CHECK(vkCreateImage(B.device, &info, NULL, &image->image)))
		goto fail;
	vkGetImageMemoryRequirements(B.device, image->image, &requirements);
	if (!memory_alloc(&requirements, &image->block, &image->offset))
	{
		host_logf(HOST_LOG_ERROR, "vk: no memory for the %ux%u texture %u", (unsigned)command->width, (unsigned)command->height,
			(unsigned)command->id);
		goto fail;
	}
	image->size = requirements.size;
	image->has_memory = 1;
	if (!HOST_VK_CHECK(vkBindImageMemory(B.device, image->image, blocks[image->block].memory, image->offset)))
		goto fail;
	view.image = image->image;
	view.viewType = command->kind == VK_IMAGE_3D ? VK_IMAGE_VIEW_TYPE_3D : command->kind == VK_IMAGE_CUBE ?
		VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	view.format = format;
	view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.levelCount = command->levels;
	view.subresourceRange.layerCount = layers;
	if (!HOST_VK_CHECK(vkCreateImageView(B.device, &view, NULL, &image->view)))
		goto fail;
	/* readable at once (outside a rendering), its texels whatever they are until the data comes */
	host_vk_rendering_end();
	barrier(host_vk_frame_command(), image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	images[command->id] = image;
	image_count++;
	return;
fail:
	image_destroy(image);
}

void host_vk_texture_data_command(const struct vk_command_texture_data *command)
{
	struct host_vk_image *image = host_vk_image_get(command->id);
	VkBuffer buffer;
	VkDeviceSize offset;
	VkBufferImageCopy copy;
	VkCommandBuffer cmd;
	uint32_t bytes;

	if (!image || command->level >= image->levels || command->face >= image->layers)
	{
		B.counts.textures_skipped++;
		return;
	}
	bytes = level_bytes(image, command->level);
	if (command->rows)
	{
		/* rows of a 2D BGRA level */
		unsigned height = level_dimension(image->height, command->level);

		if (image->format != VK_IMAGE_FORMAT_BGRA || image->kind != VK_IMAGE_2D || command->top >= height ||
			command->rows > height - command->top)
		{
			B.counts.textures_skipped++;
			return;
		}
		bytes = level_dimension(image->width, command->level) * command->rows * 4;
	}
	if (!host_vk_data_find(command->data.id, command->data.offset, bytes, &buffer, &offset))
	{
		B.counts.textures_skipped++;
		return;
	}
	host_vk_rendering_end();
	cmd = host_vk_frame_command();
	if (image->layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		if (dirty_count == dirty_capacity)
		{
			unsigned capacity = dirty_capacity ? dirty_capacity * 2 : 64;
			struct host_vk_image **grown = realloc(dirty, sizeof(*grown) * capacity);

			if (!grown)
				return;
			dirty = grown;
			dirty_capacity = capacity;
		}
		dirty[dirty_count++] = image;
	}
	else
	{
		/* another copy into it since the last draw (the atlas's rows, a texture sent again): copies are not ordered without a
		barrier between them */
		barrier(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	}
	memset(&copy, 0, sizeof(copy));
	copy.bufferOffset = offset;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.mipLevel = command->level;
	copy.imageSubresource.baseArrayLayer = command->face;
	copy.imageSubresource.layerCount = 1;
	copy.imageExtent.width = level_dimension(image->width, command->level);
	copy.imageExtent.height = command->rows ? command->rows : level_dimension(image->height, command->level);
	copy.imageOffset.y = command->rows ? (int32_t)command->top : 0;
	copy.imageExtent.depth = level_dimension(image->depth, command->level);
	vkCmdCopyBufferToImage(cmd, buffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
	B.counts.texture_bytes += bytes;
}

void host_vk_texture_free_command(const struct vk_command_texture_free *command)
{
	struct host_vk_image *image = host_vk_image_get(command->id);

	if (!image)
		return;
	images[command->id] = NULL;
	image_count--;
	image_retire(image);
}

/* the images whose texels were just written are made readable (outside a rendering: the one open is ended) */
void host_vk_textures_flush(void)
{
	VkCommandBuffer cmd;
	unsigned index;

	if (!dirty_count)
		return;
	host_vk_rendering_end();
	cmd = host_vk_frame_command();
	for (index = 0; index < dirty_count; index++)
		barrier(cmd, dirty[index], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	dirty_count = 0;
}

unsigned host_vk_texture_images(void)
{
	return image_count;
}

unsigned host_vk_texture_megabytes(void)
{
	return (unsigned)(memory_bytes / (1024 * 1024));
}
