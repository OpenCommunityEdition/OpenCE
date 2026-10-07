/*
WIN32_D3D12_RESOURCES.C

The Direct3D 12 renderer's resources (d3d12_renderer.h, win32_d3d12.h):
textures behind the renderer's handles (xgpu.h's) and render targets, each
with its state as the command list leaves it; the mirror's buffers;
samplers.

A texture's SRV keeps its slot of the shader-visible heap for as long as the
texture lives, so that draws point at it. Writes go through upload memory
and a copy recorded where the write is, between the draws before it and
those after, as OpenGL's would be.
*/

#include "win32_d3d12.h"

#include <stdlib.h>
#include <string.h>

/* ---------- textures */

#define TEXTURE_HANDLES 65536

static struct d3d12_texture *textures[TEXTURE_HANDLES];
/* handles never handed out start at next; freed ones are stacked */
static unsigned int texture_next = 1;
static unsigned int *texture_freed;
static unsigned long texture_freed_count;

long d3d12_null_srv[4] = { -1, -1, -1, -1 };

struct d3d12_texture *d3d12_texture_get(unsigned int handle)
{
	return handle && handle < TEXTURE_HANDLES ? textures[handle] : NULL;
}

static unsigned int handle_allocate(void)
{
	if (texture_freed_count)
		return texture_freed[--texture_freed_count];
	if (texture_next < TEXTURE_HANDLES)
		return texture_next++;
	return 0;
}

void d3d12_texture_transition(struct d3d12_texture *texture, D3D12_RESOURCE_STATES state)
{
	D3D12_RESOURCE_BARRIER barrier;

	if (!texture || texture->state == state)
		return;
	memset(&barrier, 0, sizeof(barrier));
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = texture->resource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = texture->state;
	barrier.Transition.StateAfter = state;
	ID3D12GraphicsCommandList_ResourceBarrier(d3d12.list, 1, &barrier);
	texture->state = state;
}

static void srv_create(struct d3d12_texture *texture)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC view;

	memset(&view, 0, sizeof(view));
	view.Format = texture->view_format;
	view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (texture->samples > 1)
	{
		view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	else if (texture->type == _d3d12r_texture_cube)
	{
		view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		view.TextureCube.MipLevels = (UINT)texture->levels;
	}
	else if (texture->type == _d3d12r_texture_3d)
	{
		view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		view.Texture3D.MipLevels = (UINT)texture->levels;
	}
	else
	{
		view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		view.Texture2D.MipLevels = (UINT)texture->levels;
	}
	ID3D12Device_CreateShaderResourceView(d3d12.device, texture->resource, &view,
		d3d12_descriptor_cpu(_d3d12_heap_srv, texture->srv));
}

unsigned int d3d12_texture_create(int type, DXGI_FORMAT format, DXGI_FORMAT view_format, unsigned long width,
	unsigned long height, unsigned long depth, unsigned long levels, int samples, BOOL render_target,
	BOOL depth_stencil)
{
	struct d3d12_texture *texture;
	D3D12_HEAP_PROPERTIES heap;
	D3D12_RESOURCE_DESC description;
	unsigned int handle;

	if (!texture_freed && !(texture_freed = calloc(TEXTURE_HANDLES, sizeof(*texture_freed))))
		return 0;
	if (!(handle = handle_allocate()))
	{
		platform_log("Direct3D 12: more than %d textures", TEXTURE_HANDLES - 1);
		return 0;
	}
	texture = calloc(1, sizeof(*texture));
	if (!texture)
	{
		texture_freed[texture_freed_count++] = handle;
		return 0;
	}
	memset(&heap, 0, sizeof(heap));
	heap.Type = D3D12_HEAP_TYPE_DEFAULT;
	memset(&description, 0, sizeof(description));
	description.Dimension = type == _d3d12r_texture_3d ? D3D12_RESOURCE_DIMENSION_TEXTURE3D :
		D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	description.Width = width;
	description.Height = (UINT)height;
	description.DepthOrArraySize = (UINT16)(type == _d3d12r_texture_3d ? depth : type == _d3d12r_texture_cube ? 6 : 1);
	description.MipLevels = (UINT16)levels;
	description.Format = format;
	description.SampleDesc.Count = samples > 1 ? (UINT)samples : 1;
	description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	if (render_target)
		description.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (depth_stencil)
		description.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	texture->state = render_target ? D3D12_RESOURCE_STATE_RENDER_TARGET :
		depth_stencil ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_COPY_DEST;
	if (FAILED(ID3D12Device_CreateCommittedResource(d3d12.device, &heap, D3D12_HEAP_FLAG_NONE, &description,
		texture->state, NULL, &IID_ID3D12Resource, (void **)&texture->resource)))
	{
		platform_log("Direct3D 12: cannot make a %lux%lux%lu texture (format %d)", width, height, depth, (int)format);
		free(texture);
		texture_freed[texture_freed_count++] = handle;
		return 0;
	}
	texture->format = format;
	texture->view_format = view_format;
	texture->type = type;
	texture->width = width;
	texture->height = height;
	texture->depth = depth;
	texture->levels = levels;
	texture->samples = samples > 1 ? samples : 1;
	texture->render_target = render_target;
	texture->depth_stencil = depth_stencil;
	texture->srv = texture->rtv = texture->dsv = -1;
	if (view_format != DXGI_FORMAT_UNKNOWN && (texture->srv = d3d12_descriptor_allocate(_d3d12_heap_srv)) >= 0)
		srv_create(texture);
	if (render_target && (texture->rtv = d3d12_descriptor_allocate(_d3d12_heap_rtv)) >= 0)
	{
		D3D12_RENDER_TARGET_VIEW_DESC view;

		memset(&view, 0, sizeof(view));
		view.Format = format;
		view.ViewDimension = texture->samples > 1 ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
		ID3D12Device_CreateRenderTargetView(d3d12.device, texture->resource, &view,
			d3d12_descriptor_cpu(_d3d12_heap_rtv, texture->rtv));
	}
	if (depth_stencil && (texture->dsv = d3d12_descriptor_allocate(_d3d12_heap_dsv)) >= 0)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC view;

		memset(&view, 0, sizeof(view));
		view.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		view.ViewDimension = texture->samples > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
		ID3D12Device_CreateDepthStencilView(d3d12.device, texture->resource, &view,
			d3d12_descriptor_cpu(_d3d12_heap_dsv, texture->dsv));
	}
	textures[handle] = texture;
	return handle;
}

void d3d12_texture_destroy(unsigned int handle)
{
	struct d3d12_texture *texture = d3d12_texture_get(handle);

	if (!texture)
		return;
	d3d12_release_later((IUnknown *)texture->resource);
	d3d12_descriptor_free(_d3d12_heap_srv, texture->srv);
	d3d12_descriptor_free(_d3d12_heap_rtv, texture->rtv);
	d3d12_descriptor_free(_d3d12_heap_dsv, texture->dsv);
	if (texture->level_srv)
	{
		unsigned long level;

		for (level = 0; level < texture->levels; level++)
		{
			d3d12_descriptor_free(_d3d12_heap_srv, texture->level_srv[level]);
			d3d12_descriptor_free(_d3d12_heap_rtv, texture->level_rtv[level]);
		}
		free(texture->level_srv);
		free(texture->level_rtv);
	}
	free(texture);
	textures[handle] = NULL;
	texture_freed[texture_freed_count++] = handle;
}

void d3d12_texture_upload(struct d3d12_texture *texture, UINT subresource, unsigned long first_row,
	unsigned long rows, unsigned long row_bytes, const void *data)
{
	D3D12_RESOURCE_DESC description;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	D3D12_TEXTURE_COPY_LOCATION destination, source;
	struct d3d12_allocation allocation;
	unsigned long level = subresource % texture->levels;
	unsigned long slices = texture->type == _d3d12r_texture_3d ?
		(texture->depth >> level ? texture->depth >> level : 1) : 1;
	BOOL compressed = texture->format == DXGI_FORMAT_BC1_UNORM || texture->format == DXGI_FORMAT_BC2_UNORM ||
		texture->format == DXGI_FORMAT_BC3_UNORM;
	unsigned long row, slice, block_rows = compressed ? (rows + 3) / 4 : rows;
	UINT64 pitch;
	D3D12_BOX box;

	ID3D12Resource_GetDesc(texture->resource, &description);
	ID3D12Device_GetCopyableFootprints(d3d12.device, &description, subresource, 1, 0, &footprint, NULL, NULL, NULL);
	pitch = footprint.Footprint.RowPitch;
	/* (a part of the rows: the footprint of that many) */
	footprint.Footprint.Height = (UINT)(compressed ? block_rows * 4 : rows);
	if (!d3d12_upload_allocate(pitch * block_rows * slices, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, &allocation))
		return;
	for (slice = 0; slice < slices; slice++)
	{
		for (row = 0; row < block_rows; row++)
		{
			memcpy(allocation.cpu + (slice * block_rows + row) * pitch,
				(const unsigned char *)data + (slice * block_rows + row) * row_bytes, row_bytes);
		}
	}
	d3d12_texture_transition(texture, D3D12_RESOURCE_STATE_COPY_DEST);
	memset(&destination, 0, sizeof(destination));
	destination.pResource = texture->resource;
	destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destination.SubresourceIndex = subresource;
	memset(&source, 0, sizeof(source));
	source.pResource = allocation.resource;
	source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	footprint.Offset = allocation.offset;
	footprint.Footprint.Depth = (UINT)slices;
	source.PlacedFootprint = footprint;
	box.left = 0;
	box.top = 0;
	box.front = 0;
	box.right = footprint.Footprint.Width;
	box.bottom = footprint.Footprint.Height;
	box.back = (UINT)slices;
	ID3D12GraphicsCommandList_CopyTextureRegion(d3d12.list, &destination, 0, (UINT)first_row, 0, &source, &box);
}

/* ---------- the renderer's textures (xgpu.h) */

static DXGI_FORMAT texture_format(int format)
{
	switch (format)
	{
	case _d3d12r_format_rgba8: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case _d3d12r_format_dxt1: return DXGI_FORMAT_BC1_UNORM;
	case _d3d12r_format_dxt3: return DXGI_FORMAT_BC2_UNORM;
	case _d3d12r_format_dxt5: return DXGI_FORMAT_BC3_UNORM;
	case _d3d12r_format_rg8: return DXGI_FORMAT_R8G8_UNORM;
	case _d3d12r_format_r8: return DXGI_FORMAT_R8_UNORM;
	default: return DXGI_FORMAT_B8G8R8A8_UNORM;
	}
}

unsigned int d3d12r_texture_new(int type, int format, unsigned long width, unsigned long height, unsigned long depth,
	unsigned long levels)
{
	unsigned int handle = d3d12_texture_create(type, texture_format(format), texture_format(format), width, height,
		depth, levels, 1, FALSE, FALSE);
	struct d3d12_texture *texture = d3d12_texture_get(handle);

	if (texture)
		texture->renderer_format = format;
	return handle;
}

static unsigned long level_size(unsigned long base, unsigned long level)
{
	return base >> level ? base >> level : 1;
}

/* a row's bytes and the rows (of blocks, compressed) of a level */
static void level_layout(const struct d3d12_texture *texture, unsigned long level, unsigned long *row_bytes,
	unsigned long *rows)
{
	unsigned long width = level_size(texture->width, level), height = level_size(texture->height, level);

	switch (texture->renderer_format)
	{
	case _d3d12r_format_dxt1:
		*row_bytes = (width + 3) / 4 * 8;
		*rows = height;
		break;
	case _d3d12r_format_dxt3:
	case _d3d12r_format_dxt5:
		*row_bytes = (width + 3) / 4 * 16;
		*rows = height;
		break;
	case _d3d12r_format_rg8:
		*row_bytes = width * 2;
		*rows = height;
		break;
	case _d3d12r_format_r8:
		*row_bytes = width;
		*rows = height;
		break;
	default:
		*row_bytes = width * 4;
		*rows = height;
		break;
	}
}

void d3d12r_texture_write(unsigned int handle, unsigned long face, unsigned long level, const void *data)
{
	struct d3d12_texture *texture = d3d12_texture_get(handle);
	unsigned long row_bytes, rows;

	if (!texture || level >= texture->levels)
		return;
	level_layout(texture, level, &row_bytes, &rows);
	d3d12_texture_upload(texture, (UINT)(face * texture->levels + level), 0, rows, row_bytes, data);
}

void d3d12r_texture_write_rows(unsigned int handle, unsigned long first_row, unsigned long rows, const void *data)
{
	struct d3d12_texture *texture = d3d12_texture_get(handle);

	if (!texture || texture->type != _d3d12r_texture_2d)
		return;
	d3d12_texture_upload(texture, 0, first_row, rows, texture->width * 4, data);
}

/* levels 1 and up, each the average of four texels of the one before (as
OpenGL's glGenerateMipmap makes them for a power-of-two texture) */
void d3d12r_texture_mipmaps(unsigned int handle, const void *level0)
{
	struct d3d12_texture *texture = d3d12_texture_get(handle);
	unsigned char *previous, *next;
	unsigned long level, width, height;

	if (!texture || texture->type != _d3d12r_texture_2d || texture->levels < 2 ||
		(texture->renderer_format != _d3d12r_format_rgba8 && texture->renderer_format != _d3d12r_format_bgra8))
	{
		return;
	}
	width = texture->width;
	height = texture->height;
	previous = malloc(width * height * 4);
	if (!previous)
		return;
	memcpy(previous, level0, width * height * 4);
	for (level = 1; level < texture->levels; level++)
	{
		unsigned long next_width = width > 1 ? width / 2 : 1, next_height = height > 1 ? height / 2 : 1;
		unsigned long x, y, channel;

		next = malloc(next_width * next_height * 4);
		if (!next)
			break;
		for (y = 0; y < next_height; y++)
		{
			for (x = 0; x < next_width; x++)
			{
				unsigned long x0 = x * 2 < width ? x * 2 : width - 1, x1 = x * 2 + 1 < width ? x * 2 + 1 : width - 1;
				unsigned long y0 = y * 2 < height ? y * 2 : height - 1, y1 = y * 2 + 1 < height ? y * 2 + 1 : height - 1;

				for (channel = 0; channel < 4; channel++)
				{
					unsigned long sum = previous[(y0 * width + x0) * 4 + channel] + previous[(y0 * width + x1) * 4 + channel] +
						previous[(y1 * width + x0) * 4 + channel] + previous[(y1 * width + x1) * 4 + channel];

					next[(y * next_width + x) * 4 + channel] = (unsigned char)((sum + 2) / 4);
				}
			}
		}
		d3d12_texture_upload(texture, (UINT)level, 0, next_height, next_width * 4, next);
		free(previous);
		previous = next;
		width = next_width;
		height = next_height;
	}
	free(previous);
}

void d3d12r_texture_delete(unsigned int handle)
{
	d3d12_texture_destroy(handle);
}

/* block-compressed textures are kept so only at sizes Direct3D 12 takes
(their top level's a multiple of 4 each way) */
int d3d12r_texture_compressed_supported(unsigned long width, unsigned long height)
{
	return !(width & 3) && !(height & 3);
}

/* ---------- render targets

Each is a texture of B8G8R8A8 (as OpenGL's RGBA8) or of D24S8, its depth
sampled through R24_UNORM_X8; with multisampling, a multisampled texture of
its own that draws go to, resolved into the first before it is read. */

unsigned int d3d12r_target_create(int depth, unsigned long width, unsigned long height, int samples)
{
	unsigned int handle;
	struct d3d12_texture *texture;

	if (samples <= 1)
	{
		return depth ?
			d3d12_texture_create(_d3d12r_texture_2d, DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
				width, height, 1, 1, 1, FALSE, TRUE) :
			d3d12_texture_create(_d3d12r_texture_2d, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM,
				width, height, 1, 1, 1, TRUE, FALSE);
	}
	/* (cleared, not given the texture's pixels: a change of the setting is
	taken up between frames, and a frame clears the screen's targets before
	it draws) */
	if (depth)
	{
		handle = d3d12_texture_create(_d3d12r_texture_2d, DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_UNKNOWN, width,
			height, 1, 1, samples, FALSE, TRUE);
		if ((texture = d3d12_texture_get(handle)) != NULL)
		{
			ID3D12GraphicsCommandList_ClearDepthStencilView(d3d12.list, d3d12_descriptor_cpu(_d3d12_heap_dsv, texture->dsv),
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
		}
	}
	else
	{
		static const float nothing[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		handle = d3d12_texture_create(_d3d12r_texture_2d, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, width,
			height, 1, 1, samples, TRUE, FALSE);
		if ((texture = d3d12_texture_get(handle)) != NULL)
		{
			ID3D12GraphicsCommandList_ClearRenderTargetView(d3d12.list, d3d12_descriptor_cpu(_d3d12_heap_rtv, texture->rtv),
				nothing, 0, NULL);
		}
	}
	return handle;
}

void d3d12r_target_resolve(unsigned int multisample_handle, unsigned int texture_handle)
{
	struct d3d12_texture *texture = d3d12_texture_get(texture_handle);
	struct d3d12_texture *multisample = d3d12_texture_get(multisample_handle);

	if (!texture || !multisample || texture->depth_stencil)
		return;
	d3d12_texture_transition(multisample, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
	d3d12_texture_transition(texture, D3D12_RESOURCE_STATE_RESOLVE_DEST);
	ID3D12GraphicsCommandList_ResolveSubresource(d3d12.list, texture->resource, 0, multisample->resource, 0,
		DXGI_FORMAT_B8G8R8A8_UNORM);
}

/* ---------- render targets sampled with their mip chain (d3d12_device.c) */

unsigned int d3d12r_composite_create(unsigned long width, unsigned long height, unsigned long levels)
{
	/* (a render target too: its levels can be drawn from each other) */
	return d3d12_texture_create(_d3d12r_texture_2d, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, width,
		height, 1, levels, 1, TRUE, FALSE);
}

void d3d12r_composite_copy_level(unsigned int composite_handle, unsigned long level, unsigned int target_handle)
{
	struct d3d12_texture *composite = d3d12_texture_get(composite_handle);
	struct d3d12_texture *target = d3d12_texture_get(target_handle);
	D3D12_TEXTURE_COPY_LOCATION to, from;

	if (!composite || !target || level >= composite->levels)
		return;
	d3d12_texture_transition(target, D3D12_RESOURCE_STATE_COPY_SOURCE);
	d3d12_texture_transition(composite, D3D12_RESOURCE_STATE_COPY_DEST);
	memset(&to, 0, sizeof(to));
	to.pResource = composite->resource;
	to.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	to.SubresourceIndex = (UINT)level;
	memset(&from, 0, sizeof(from));
	from.pResource = target->resource;
	from.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	from.SubresourceIndex = 0;
	ID3D12GraphicsCommandList_CopyTextureRegion(d3d12.list, &to, 0, 0, 0, &from, NULL);
}

void d3d12r_composite_mipmaps(unsigned int composite_handle, unsigned long first_level)
{
	struct d3d12_texture *composite = d3d12_texture_get(composite_handle);

	if (composite)
		d3d12_pass_mipmaps(composite, first_level);
}

/* ---------- the mirror's buffers */

static struct
{
	ID3D12Resource *buffer;
	D3D12_RESOURCE_STATES state;
	D3D12_GPU_VIRTUAL_ADDRESS address;
} segments[D3D12R_MIRROR_SEGMENT_COUNT];

#define MIRROR_READ_STATE (D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER)

static void segment_transition(unsigned long segment, D3D12_RESOURCE_STATES state)
{
	D3D12_RESOURCE_BARRIER barrier;

	if (segments[segment].state == state)
		return;
	memset(&barrier, 0, sizeof(barrier));
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = segments[segment].buffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = segments[segment].state;
	barrier.Transition.StateAfter = state;
	ID3D12GraphicsCommandList_ResourceBarrier(d3d12.list, 1, &barrier);
	segments[segment].state = state;
}

void d3d12r_mirror_upload(unsigned long segment, unsigned long offset, unsigned long size, const void *data)
{
	struct d3d12_allocation allocation;

	if (segment >= D3D12R_MIRROR_SEGMENT_COUNT)
		return;
	if (!segments[segment].buffer)
	{
		D3D12_HEAP_PROPERTIES heap;
		D3D12_RESOURCE_DESC description;

		memset(&heap, 0, sizeof(heap));
		heap.Type = D3D12_HEAP_TYPE_DEFAULT;
		memset(&description, 0, sizeof(description));
		description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		description.Width = D3D12R_MIRROR_SEGMENT_SIZE;
		description.Height = 1;
		description.DepthOrArraySize = 1;
		description.MipLevels = 1;
		description.SampleDesc.Count = 1;
		description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		if (FAILED(ID3D12Device_CreateCommittedResource(d3d12.device, &heap, D3D12_HEAP_FLAG_NONE, &description,
			D3D12_RESOURCE_STATE_COPY_DEST, NULL, &IID_ID3D12Resource, (void **)&segments[segment].buffer)))
		{
			platform_log("Direct3D 12: cannot make the mirror's segment %lu", segment);
			return;
		}
		segments[segment].state = D3D12_RESOURCE_STATE_COPY_DEST;
		segments[segment].address = ID3D12Resource_GetGPUVirtualAddress(segments[segment].buffer);
	}
	if (!d3d12_upload_allocate(size, 16, &allocation))
		return;
	memcpy(allocation.cpu, data, size);
	segment_transition(segment, D3D12_RESOURCE_STATE_COPY_DEST);
	ID3D12GraphicsCommandList_CopyBufferRegion(d3d12.list, segments[segment].buffer, offset, allocation.resource,
		allocation.offset, size);
}

D3D12_GPU_VIRTUAL_ADDRESS d3d12_mirror_address(unsigned long segment, unsigned long offset)
{
	if (segment >= D3D12R_MIRROR_SEGMENT_COUNT || !segments[segment].buffer)
		return 0;
	return segments[segment].address + offset;
}

void d3d12_mirror_use(unsigned long segment)
{
	if (segment < D3D12R_MIRROR_SEGMENT_COUNT && segments[segment].buffer)
		segment_transition(segment, MIRROR_READ_STATE);
}

void d3d12_resources_submitted(void)
{
	unsigned long segment;

	for (segment = 0; segment < D3D12R_MIRROR_SEGMENT_COUNT; segment++)
	{
		if (segments[segment].buffer)
			segments[segment].state = D3D12_RESOURCE_STATE_COMMON;
	}
}

/* ---------- samplers */

#define SAMPLER_BUCKETS 256

struct sampler_entry
{
	struct sampler_entry *next;
	D3D12_SAMPLER_DESC description;
	long slot;
};

static struct sampler_entry *sampler_buckets[SAMPLER_BUCKETS];

long d3d12_sampler_get(const D3D12_SAMPLER_DESC *description)
{
	unsigned long hash = d3d12_hash(description, sizeof(*description));
	struct sampler_entry *entry;

	for (entry = sampler_buckets[hash % SAMPLER_BUCKETS]; entry; entry = entry->next)
	{
		if (!memcmp(&entry->description, description, sizeof(*description)))
			return entry->slot;
	}
	entry = calloc(1, sizeof(*entry));
	if (!entry)
		return -1;
	entry->description = *description;
	entry->slot = d3d12_descriptor_allocate(_d3d12_heap_sampler);
	if (entry->slot < 0)
	{
		free(entry);
		return -1;
	}
	ID3D12Device_CreateSampler(d3d12.device, description, d3d12_descriptor_cpu(_d3d12_heap_sampler, entry->slot));
	entry->next = sampler_buckets[hash % SAMPLER_BUCKETS];
	sampler_buckets[hash % SAMPLER_BUCKETS] = entry;
	return entry->slot;
}
