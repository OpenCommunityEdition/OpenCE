/*
WIN32_D3D12_DRAW.C

The Direct3D 12 renderer's draws (d3d12_renderer.h, win32_d3d12.h): each
described whole by d3d12_device.c, recorded here.

A draw is recorded in two steps. First what it needs memory for is found:
a copy of the color target for a stage that samples it, then room in upload
memory for its constants, streamed vertices and indices, reserved at once.
Upload memory running short in this step hands what is recorded to the GPU
and begins the command list anew (d3d12_flush_and_wait), which no draw is
then half way through. Then the draw is recorded: the resources' barriers,
the targets, the pipeline state, the root parameters, the vertex and index
buffers and the draw itself, each set only when it differs from the draw
before.
*/

#include "win32_d3d12.h"

#include <stdlib.h>
#include <string.h>

/* d3d12_renderer.h's numbers are Direct3D 12's */
#define SAME(a, b) _Static_assert((int)(a) == (int)(b), #a)
SAME(D3D12R_BLEND_ZERO, D3D12_BLEND_ZERO);
SAME(D3D12R_BLEND_SRC_ALPHA_SAT, D3D12_BLEND_SRC_ALPHA_SAT);
SAME(D3D12R_BLEND_BLEND_FACTOR, D3D12_BLEND_BLEND_FACTOR);
SAME(D3D12R_BLEND_INV_BLEND_FACTOR, D3D12_BLEND_INV_BLEND_FACTOR);
SAME(D3D12R_BLEND_INV_DEST_COLOR, D3D12_BLEND_INV_DEST_COLOR);
SAME(D3D12R_BLEND_OP_MAX, D3D12_BLEND_OP_MAX);
SAME(D3D12R_COMPARISON_NEVER, D3D12_COMPARISON_FUNC_NEVER);
SAME(D3D12R_COMPARISON_ALWAYS, D3D12_COMPARISON_FUNC_ALWAYS);
SAME(D3D12R_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP);
SAME(D3D12R_STENCIL_OP_DECR, D3D12_STENCIL_OP_DECR);
SAME(D3D12R_STENCIL_OP_INVERT, D3D12_STENCIL_OP_INVERT);
SAME(D3D12R_CULL_BACK, D3D12_CULL_MODE_BACK);
SAME(D3D12R_FILL_WIREFRAME, D3D12_FILL_MODE_WIREFRAME);
SAME(D3D12R_FILL_SOLID, D3D12_FILL_MODE_SOLID);
SAME(D3D12R_TOPOLOGY_TYPE_TRIANGLE, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
SAME(D3D12R_TOPOLOGY_POINTLIST, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
SAME(D3D12R_TOPOLOGY_LINESTRIP, D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
SAME(D3D12R_TOPOLOGY_TRIANGLESTRIP, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
SAME(D3D12R_COLOR_WRITE_ALPHA, D3D12_COLOR_WRITE_ENABLE_ALPHA);
SAME(D3D12R_FILTER_ANISOTROPIC, D3D12_FILTER_ANISOTROPIC);
SAME(D3D12R_BASIC_FILTER(1, 1, 1), D3D12_FILTER_MIN_MAG_MIP_LINEAR);
SAME(D3D12R_BASIC_FILTER(1, 0, 1), D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
SAME(D3D12R_ADDRESS_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER);
SAME(D3D12R_ADDRESS_MIRROR, D3D12_TEXTURE_ADDRESS_MODE_MIRROR);
SAME(D3D12R_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT);
SAME(D3D12R_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT);
SAME(D3D12R_FORMAT_R16G16B16A16_SNORM, DXGI_FORMAT_R16G16B16A16_SNORM);
SAME(D3D12R_FORMAT_R16G16B16A16_SINT, DXGI_FORMAT_R16G16B16A16_SINT);
SAME(D3D12R_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32_FLOAT);
SAME(D3D12R_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM);
SAME(D3D12R_FORMAT_R16G16_SNORM, DXGI_FORMAT_R16G16_SNORM);
SAME(D3D12R_FORMAT_R16G16_SINT, DXGI_FORMAT_R16G16_SINT);
SAME(D3D12R_FORMAT_R32_FLOAT, DXGI_FORMAT_R32_FLOAT);
SAME(D3D12R_FORMAT_R32_UINT, DXGI_FORMAT_R32_UINT);
SAME(D3D12R_FORMAT_R8G8_UNORM, DXGI_FORMAT_R8G8_UNORM);
SAME(D3D12R_FORMAT_R16_SNORM, DXGI_FORMAT_R16_SNORM);
SAME(D3D12R_FORMAT_R16_SINT, DXGI_FORMAT_R16_SINT);
SAME(D3D12R_FORMAT_R8_UNORM, DXGI_FORMAT_R8_UNORM);
SAME(D3D12R_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM);
typedef char pipeline_key_size[sizeof(struct d3d12r_pipeline) % 4 == 0 ? 1 : -1];

/* the visibility test under way (-1: none); whether tests have ended since
the GPU was last handed work, and the draws since (submit_when_due) */
static long active_query;
static BOOL tests_ended;
static unsigned long draws_since_submit;

/* ---------- pipeline states, by their description's bytes */

struct pipeline_entry
{
	struct pipeline_entry *next;
	unsigned long hash;
	struct d3d12r_pipeline key;
	ID3D12PipelineState *state;
};

#define PIPELINE_BUCKETS 4096

static struct pipeline_entry *pipeline_buckets[PIPELINE_BUCKETS];

static ID3D12PipelineState *pipeline_get(const struct d3d12r_draw *draw)
{
	static struct pipeline_entry *last;
	const struct d3d12r_pipeline *key = &draw->pipeline;
	unsigned long hash, index;
	struct pipeline_entry **bucket, *entry;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC description;
	D3D12_RENDER_TARGET_BLEND_DESC *blend;
	D3D12_INPUT_ELEMENT_DESC elements[D3D12R_ATTRIBUTES];
	ID3DBlob *vertex_shader = draw->vertex_shader, *pixel_shader = draw->pixel_shader;

	if (last && !memcmp(&last->key, key, sizeof(*key)))
		return last->state;
	hash = d3d12_hash(key, sizeof(*key));
	bucket = &pipeline_buckets[hash % PIPELINE_BUCKETS];
	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->hash == hash && !memcmp(&entry->key, key, sizeof(*key)))
		{
			last = entry;
			return entry->state;
		}
	}
	entry = calloc(1, sizeof(*entry));
	if (!entry)
		return NULL;
	entry->hash = hash;
	entry->key = *key;

	memset(elements, 0, sizeof(elements));
	for (index = 0; index < draw->element_count && index < D3D12R_ATTRIBUTES; index++)
	{
		elements[index].SemanticName = "ATTRIBUTE";
		elements[index].SemanticIndex = draw->elements[index].reg;
		elements[index].Format = (DXGI_FORMAT)draw->elements[index].format;
		elements[index].InputSlot = draw->elements[index].stream;
		elements[index].AlignedByteOffset = draw->elements[index].offset;
		elements[index].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	}
	memset(&description, 0, sizeof(description));
	description.pRootSignature = d3d12.root_signature;
	description.VS.pShaderBytecode = ID3D10Blob_GetBufferPointer(vertex_shader);
	description.VS.BytecodeLength = ID3D10Blob_GetBufferSize(vertex_shader);
	description.PS.pShaderBytecode = ID3D10Blob_GetBufferPointer(pixel_shader);
	description.PS.BytecodeLength = ID3D10Blob_GetBufferSize(pixel_shader);
	blend = &description.BlendState.RenderTarget[0];
	blend->BlendEnable = key->blend_enable;
	blend->SrcBlend = (D3D12_BLEND)key->blend_source;
	blend->DestBlend = (D3D12_BLEND)key->blend_destination;
	blend->BlendOp = (D3D12_BLEND_OP)key->blend_operation;
	blend->SrcBlendAlpha = (D3D12_BLEND)key->blend_source_alpha;
	blend->DestBlendAlpha = (D3D12_BLEND)key->blend_destination_alpha;
	blend->BlendOpAlpha = (D3D12_BLEND_OP)key->blend_operation;
	blend->LogicOp = D3D12_LOGIC_OP_NOOP;
	blend->RenderTargetWriteMask = key->write_mask;
	description.SampleMask = UINT_MAX;
	description.RasterizerState.FillMode = (D3D12_FILL_MODE)key->fill_mode;
	description.RasterizerState.CullMode = (D3D12_CULL_MODE)key->cull_mode;
	description.RasterizerState.FrontCounterClockwise = key->front_counter_clockwise;
	description.RasterizerState.DepthBias = key->depth_bias;
	description.RasterizerState.SlopeScaledDepthBias = key->slope_scaled_depth_bias;
	description.RasterizerState.DepthClipEnable = TRUE;
	description.RasterizerState.MultisampleEnable = key->samples > 1;
	description.DepthStencilState.DepthEnable = key->depth_enable;
	description.DepthStencilState.DepthWriteMask = key->depth_write ? D3D12_DEPTH_WRITE_MASK_ALL :
		D3D12_DEPTH_WRITE_MASK_ZERO;
	description.DepthStencilState.DepthFunc = (D3D12_COMPARISON_FUNC)key->depth_function;
	description.DepthStencilState.StencilEnable = key->stencil_enable;
	description.DepthStencilState.StencilReadMask = key->stencil_read_mask;
	description.DepthStencilState.StencilWriteMask = key->stencil_write_mask;
	description.DepthStencilState.FrontFace.StencilFunc = (D3D12_COMPARISON_FUNC)key->stencil_function;
	description.DepthStencilState.FrontFace.StencilFailOp = (D3D12_STENCIL_OP)key->stencil_fail;
	description.DepthStencilState.FrontFace.StencilDepthFailOp = (D3D12_STENCIL_OP)key->stencil_depth_fail;
	description.DepthStencilState.FrontFace.StencilPassOp = (D3D12_STENCIL_OP)key->stencil_pass;
	description.DepthStencilState.BackFace = description.DepthStencilState.FrontFace;
	description.InputLayout.pInputElementDescs = draw->element_count ? elements : NULL;
	description.InputLayout.NumElements = (UINT)index;
	description.PrimitiveTopologyType = (D3D12_PRIMITIVE_TOPOLOGY_TYPE)key->topology_type;
	description.NumRenderTargets = key->has_color ? 1 : 0;
	description.RTVFormats[0] = key->has_color ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_UNKNOWN;
	description.DSVFormat = key->has_depth ? DXGI_FORMAT_D24_UNORM_S8_UINT : DXGI_FORMAT_UNKNOWN;
	description.SampleDesc.Count = key->samples ? key->samples : 1;
	if (FAILED(ID3D12Device_CreateGraphicsPipelineState(d3d12.device, &description, &IID_ID3D12PipelineState,
		(void **)&entry->state)))
	{
		platform_log("Direct3D 12: cannot make a pipeline state (shaders %lu and %lu, input layout %lu)",
			key->vertex_shader, key->pixel_shader, key->input_layout);
		for (index = 0; index < draw->element_count; index++)
		{
			platform_log("  register %u: stream %u (stride %lu), offset %u, format %u", draw->elements[index].reg,
				draw->elements[index].stream, draw->streams[draw->elements[index].stream].stride,
				draw->elements[index].offset, draw->elements[index].format);
		}
		entry->state = NULL;
	}
	entry->next = *bucket;
	*bucket = entry;
	last = entry;
	return entry->state;
}

/* ---------- samplers, each stage's last remembered */

static long sampler_slot(int stage, const struct d3d12r_sampler *sampler)
{
	static struct d3d12r_sampler last[D3D12R_STAGES];
	static long last_slot[D3D12R_STAGES] = { -1, -1, -1, -1 };
	D3D12_SAMPLER_DESC description;
	long slot;

	if (last_slot[stage] >= 0 && !memcmp(&last[stage], sampler, sizeof(*sampler)))
		return last_slot[stage];
	memset(&description, 0, sizeof(description));
	description.Filter = (D3D12_FILTER)sampler->filter;
	description.AddressU = (D3D12_TEXTURE_ADDRESS_MODE)sampler->address_u;
	description.AddressV = (D3D12_TEXTURE_ADDRESS_MODE)sampler->address_v;
	description.AddressW = (D3D12_TEXTURE_ADDRESS_MODE)sampler->address_w;
	description.MipLODBias = sampler->mip_lod_bias;
	description.MaxAnisotropy = sampler->maximum_anisotropy ? sampler->maximum_anisotropy : 1;
	description.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	memcpy(description.BorderColor, sampler->border_color, sizeof(description.BorderColor));
	description.MinLOD = sampler->minimum_lod;
	description.MaxLOD = sampler->maximum_lod;
	slot = d3d12_sampler_get(&description);
	if (slot < 0)
		slot = 0;
	last[stage] = *sampler;
	last_slot[stage] = slot;
	return slot;
}

/* ---------- the color target sampled

A texture that is also the draw's color target is sampled from a copy of it
(a resource cannot be both in Direct3D 12): its pixels as they were before
the draw, which OpenGL's feedback loop reads too. */

static struct d3d12_texture *target_copy(struct d3d12_texture *target)
{
	static unsigned int copy;
	struct d3d12_texture *texture = d3d12_texture_get(copy);

	if (!texture || texture->width != target->width || texture->height != target->height)
	{
		d3d12_texture_destroy(copy);
		copy = d3d12_texture_create(_d3d12r_texture_2d, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM,
			target->width, target->height, 1, 1, 1, FALSE, FALSE);
		if (!(texture = d3d12_texture_get(copy)))
			return NULL;
	}
	d3d12_texture_transition(target, D3D12_RESOURCE_STATE_COPY_SOURCE);
	d3d12_texture_transition(texture, D3D12_RESOURCE_STATE_COPY_DEST);
	ID3D12GraphicsCommandList_CopyResource(d3d12.list, texture->resource, target->resource);
	return texture;
}

/* ---------- what the command list has set (set again only when it changes) */

static struct
{
	BOOL valid;
	ID3D12PipelineState *pipeline;
	D3D12_GPU_VIRTUAL_ADDRESS constants[2];
	long srv[D3D12R_STAGES], sampler[D3D12R_STAGES];
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissor;
	UINT stencil_reference;
	float blend_factor[4];
	struct d3d12_texture *color, *depth;
	D3D12_PRIMITIVE_TOPOLOGY topology;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffers[D3D12R_STREAMS];
	D3D12_INDEX_BUFFER_VIEW index_buffer;
} bound;

/* the constants uploaded last, and their serials; shared by the draws after
them until the serials change or the command list begins again */
static struct
{
	BOOL valid;
	unsigned long serial;
	D3D12_GPU_VIRTUAL_ADDRESS address;
} uploaded_constants[2];

void d3d12_draw_state_invalidate(void)
{
	memset(&bound, 0, sizeof(bound));
}

void d3d12_draw_list_begin(void)
{
	d3d12_draw_state_invalidate();
	memset(uploaded_constants, 0, sizeof(uploaded_constants));
	tests_ended = FALSE;
	draws_since_submit = 0;
}

static BOOL constants_upload(int which, const void *data, unsigned long size, unsigned long serial)
{
	struct d3d12_allocation allocation;

	if (uploaded_constants[which].valid && uploaded_constants[which].serial == serial)
		return TRUE;
	if (!d3d12_upload_allocate(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, &allocation))
		return FALSE;
	memcpy(allocation.cpu, data, size);
	uploaded_constants[which].valid = TRUE;
	uploaded_constants[which].serial = serial;
	uploaded_constants[which].address = allocation.gpu;
	return TRUE;
}

/* ---------- vertices

Bytes past a stream's last vertex that its views take in: an element of
three components is read through a format of four (nv2a_vertex_inputs
w_one_mask), whose last would otherwise lie past the view's end for the
last vertex, and D3D12 reads the whole element as 0 then. */
#define STREAM_SLACK 8

/* the views of a draw's streams, and the mirror's segments they read */
static BOOL streams_set_up(const struct d3d12r_draw *draw, D3D12_VERTEX_BUFFER_VIEW views[D3D12R_STREAMS],
	long segments[D3D12R_STREAMS])
{
	int index;

	memset(views, 0, D3D12R_STREAMS * sizeof(*views));
	for (index = 0; index < D3D12R_STREAMS; index++)
	{
		const struct d3d12r_stream *stream = &draw->streams[index];
		D3D12_GPU_VIRTUAL_ADDRESS address;
		struct d3d12_allocation allocation;

		segments[index] = -1;
		if (!stream->size)
			continue;
		if (stream->segment >= 0 && (address = d3d12_mirror_address((unsigned long)stream->segment, stream->offset)) != 0)
		{
			views[index].BufferLocation = address;
			views[index].SizeInBytes = (UINT)(stream->size +
				(stream->offset + stream->size + STREAM_SLACK <= D3D12R_MIRROR_SEGMENT_SIZE ? STREAM_SLACK : 0));
			views[index].StrideInBytes = (UINT)stream->stride;
			segments[index] = stream->segment;
			continue;
		}
		if (!stream->data || !d3d12_upload_allocate(stream->size + STREAM_SLACK, 16, &allocation))
			return FALSE;
		memcpy(allocation.cpu, stream->data, stream->size);
		memset(allocation.cpu + stream->size, 0, STREAM_SLACK);
		views[index].BufferLocation = allocation.gpu;
		views[index].SizeInBytes = (UINT)(stream->size + STREAM_SLACK);
		views[index].StrideInBytes = (UINT)stream->stride;
	}
	return TRUE;
}

/* the upload memory a draw takes at most */
static UINT64 draw_upload_size(const struct d3d12r_draw *draw)
{
	UINT64 size = draw->vertex_constants_size + draw->pixel_constants_size +
		2 * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	int index;

	for (index = 0; index < D3D12R_STREAMS; index++)
	{
		if (draw->streams[index].size && draw->streams[index].segment < 0)
			size += draw->streams[index].size + STREAM_SLACK + 16;
	}
	if (draw->index_count && draw->index_segment < 0)
		size += draw->index_count * sizeof(unsigned short) + 16;
	/* (and a segment of the mirror the draw expected that has no buffer yet,
	streamed instead) */
	return size + 4096;
}

/* ---------- handing the GPU work before the frame ends

The game reads a visibility test's count at the start of the next frame
(lens flares are as bright as their tests found them visible), and takes
what the GPU has written by then. A frame recorded whole and handed to the
GPU as it is presented gives its counts a frame later than OpenGL's drivers,
which stream the work as it comes: so the work is handed over once the tests
of a batch have ended, as the game draws something else, and every few
hundred draws. */

#define SUBMIT_DRAWS 512

static void submit_when_due(void)
{
	if (active_query < 0 && (tests_ended || draws_since_submit >= SUBMIT_DRAWS))
	{
		d3d12_submit();
		tests_ended = FALSE;
		draws_since_submit = 0;
	}
	draws_since_submit++;
}

/* ---------- what the game draws

Each run of draws of one of the game's passes is an event of the command
list named after the pass, which PIX and RenderDoc show the draws under. */

/* (PIX's metadata for an event named by a string of chars) */
#define PIX_EVENT_ANSI_VERSION 1

static short event_pass = -1;
static BOOL event_open;
/* the latest camera of each window */
static struct d3d12r_view views[4];

void d3d12_events_close(void)
{
	if (event_open)
		ID3D12GraphicsCommandList_EndEvent(d3d12.list);
	event_open = FALSE;
	event_pass = -1;
}

static void event_pass_set(short pass, const char *name)
{
	if (pass == event_pass && (event_open || pass < 0))
		return;
	d3d12_events_close();
	if (pass >= 0 && name && name[0])
	{
		ID3D12GraphicsCommandList_BeginEvent(d3d12.list, PIX_EVENT_ANSI_VERSION, name, (UINT)strlen(name) + 1);
		event_open = TRUE;
	}
	event_pass = pass;
}

void d3d12r_view_begin(const struct d3d12r_view *view)
{
	if (view->window >= 0 && view->window < (short)(sizeof(views) / sizeof(views[0])))
		views[view->window] = *view;
}

void d3d12r_opaque_done(void)
{
	if (active_query >= 0)
		return;
	d3d12_submit();
	tests_ended = FALSE;
	draws_since_submit = 0;
}

/* ---------- the draws */

void d3d12r_draw(const struct d3d12r_draw *draw)
{
	ID3D12GraphicsCommandList *list = d3d12.list;
	struct d3d12_texture *textures[D3D12R_STAGES], *color, *depth;
	long srv[D3D12R_STAGES], sampler[D3D12R_STAGES], segments[D3D12R_STREAMS];
	D3D12_VERTEX_BUFFER_VIEW views[D3D12R_STREAMS];
	D3D12_INDEX_BUFFER_VIEW index_view;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissor;
	ID3D12PipelineState *pipeline;
	int stage, index;

	if (!draw->vertex_shader || !draw->pixel_shader || (!draw->vertex_count && !draw->index_count))
		return;
	color = d3d12_texture_get(draw->color);
	depth = d3d12_texture_get(draw->depth);
	if (!color && !depth)
		return;
	if (!(pipeline = pipeline_get(draw)))
		return;
	submit_when_due();
	for (stage = 0; stage < D3D12R_STAGES; stage++)
	{
		textures[stage] = d3d12_texture_get(draw->textures[stage]);
		if (textures[stage] && textures[stage] == color)
			textures[stage] = target_copy(color);
		srv[stage] = textures[stage] && textures[stage]->srv >= 0 ? textures[stage]->srv :
			d3d12_null_srv[draw->sampler_types[stage] & 3];
		sampler[stage] = draw->textures[stage] ? sampler_slot(stage, &draw->samplers[stage]) : 0;
	}

	/* memory, while nothing of the draw is recorded */
	d3d12_upload_reserve(draw_upload_size(draw));
	if (!constants_upload(0, draw->vertex_constants, draw->vertex_constants_size, draw->vertex_constants_serial) ||
		!constants_upload(1, draw->pixel_constants, draw->pixel_constants_size, draw->pixel_constants_serial) ||
		!streams_set_up(draw, views, segments))
	{
		return;
	}
	memset(&index_view, 0, sizeof(index_view));
	if (draw->index_count)
	{
		D3D12_GPU_VIRTUAL_ADDRESS address = draw->index_segment >= 0 ?
			d3d12_mirror_address((unsigned long)draw->index_segment, draw->index_offset) : 0;
		UINT size = (UINT)(draw->index_count * sizeof(unsigned short));

		if (!address)
		{
			struct d3d12_allocation allocation;

			if (!draw->indices || !d3d12_upload_allocate(size, 16, &allocation))
				return;
			memcpy(allocation.cpu, draw->indices, size);
			address = allocation.gpu;
		}
		else
		{
			d3d12_mirror_use((unsigned long)draw->index_segment);
		}
		index_view.BufferLocation = address;
		index_view.SizeInBytes = size;
		index_view.Format = DXGI_FORMAT_R16_UINT;
	}

	/* the draw recorded */
	event_pass_set(draw->pass, draw->pass_name);
	for (index = 0; index < D3D12R_STREAMS; index++)
	{
		if (segments[index] >= 0)
			d3d12_mirror_use((unsigned long)segments[index]);
	}
	for (stage = 0; stage < D3D12R_STAGES; stage++)
	{
		if (textures[stage])
			d3d12_texture_transition(textures[stage], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	if (color)
		d3d12_texture_transition(color, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (depth)
		d3d12_texture_transition(depth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	if (!bound.valid)
	{
		/* (a new command list, or one a pass has used) */
		d3d12_bind_heaps();
		memset(&bound, 0, sizeof(bound));
		bound.valid = TRUE;
		memset(bound.srv, 0xff, sizeof(bound.srv));
		memset(bound.sampler, 0xff, sizeof(bound.sampler));
		memset(&bound.viewport, 0xff, sizeof(bound.viewport));
		memset(&bound.scissor, 0xff, sizeof(bound.scissor));
		bound.stencil_reference = UINT_MAX;
		memset(bound.blend_factor, 0xff, sizeof(bound.blend_factor));
		bound.color = bound.depth = (struct d3d12_texture *)(INT_PTR)-1;
		memset(&bound.index_buffer, 0xff, sizeof(bound.index_buffer));
	}
	if (bound.color != color || bound.depth != depth)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, dsv;

		if (color)
			rtv = d3d12_descriptor_cpu(_d3d12_heap_rtv, color->rtv);
		if (depth)
			dsv = d3d12_descriptor_cpu(_d3d12_heap_dsv, depth->dsv);
		ID3D12GraphicsCommandList_OMSetRenderTargets(list, color ? 1 : 0, color ? &rtv : NULL, FALSE,
			depth ? &dsv : NULL);
		bound.color = color;
		bound.depth = depth;
	}
	if (bound.pipeline != pipeline)
	{
		ID3D12GraphicsCommandList_SetPipelineState(list, pipeline);
		bound.pipeline = pipeline;
	}
	for (index = 0; index < 2; index++)
	{
		if (bound.constants[index] != uploaded_constants[index].address)
		{
			ID3D12GraphicsCommandList_SetGraphicsRootConstantBufferView(list, (UINT)index,
				uploaded_constants[index].address);
			bound.constants[index] = uploaded_constants[index].address;
		}
	}
	for (stage = 0; stage < D3D12R_STAGES; stage++)
	{
		if (bound.srv[stage] != srv[stage])
		{
			ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(list, (UINT)(2 + stage),
				d3d12_descriptor_gpu(_d3d12_heap_srv, srv[stage]));
			bound.srv[stage] = srv[stage];
		}
		if (bound.sampler[stage] != sampler[stage])
		{
			ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(list, (UINT)(6 + stage),
				d3d12_descriptor_gpu(_d3d12_heap_sampler, sampler[stage]));
			bound.sampler[stage] = sampler[stage];
		}
	}
	viewport.TopLeftX = draw->viewport[0];
	viewport.TopLeftY = draw->viewport[1];
	viewport.Width = draw->viewport[2];
	viewport.Height = draw->viewport[3];
	viewport.MinDepth = draw->viewport[4];
	viewport.MaxDepth = draw->viewport[5];
	if (memcmp(&bound.viewport, &viewport, sizeof(viewport)))
	{
		ID3D12GraphicsCommandList_RSSetViewports(list, 1, &viewport);
		bound.viewport = viewport;
	}
	scissor.left = draw->scissor[0];
	scissor.top = draw->scissor[1];
	scissor.right = draw->scissor[2];
	scissor.bottom = draw->scissor[3];
	if (memcmp(&bound.scissor, &scissor, sizeof(scissor)))
	{
		ID3D12GraphicsCommandList_RSSetScissorRects(list, 1, &scissor);
		bound.scissor = scissor;
	}
	if (bound.stencil_reference != draw->stencil_reference)
	{
		ID3D12GraphicsCommandList_OMSetStencilRef(list, draw->stencil_reference);
		bound.stencil_reference = draw->stencil_reference;
	}
	if (memcmp(bound.blend_factor, draw->blend_factor, sizeof(bound.blend_factor)))
	{
		ID3D12GraphicsCommandList_OMSetBlendFactor(list, draw->blend_factor);
		memcpy(bound.blend_factor, draw->blend_factor, sizeof(bound.blend_factor));
	}
	if (bound.topology != (D3D12_PRIMITIVE_TOPOLOGY)draw->topology)
	{
		ID3D12GraphicsCommandList_IASetPrimitiveTopology(list, (D3D12_PRIMITIVE_TOPOLOGY)draw->topology);
		bound.topology = (D3D12_PRIMITIVE_TOPOLOGY)draw->topology;
	}
	if (memcmp(bound.vertex_buffers, views, sizeof(bound.vertex_buffers)))
	{
		ID3D12GraphicsCommandList_IASetVertexBuffers(list, 0, D3D12R_STREAMS, views);
		memcpy(bound.vertex_buffers, views, sizeof(bound.vertex_buffers));
	}
	if (draw->index_count)
	{
		if (memcmp(&bound.index_buffer, &index_view, sizeof(index_view)))
		{
			ID3D12GraphicsCommandList_IASetIndexBuffer(list, &index_view);
			bound.index_buffer = index_view;
		}
		ID3D12GraphicsCommandList_DrawIndexedInstanced(list, (UINT)draw->index_count, 1, 0, (INT)draw->base_vertex, 0);
	}
	else
	{
		ID3D12GraphicsCommandList_DrawInstanced(list, (UINT)draw->vertex_count, 1, 0, 0);
	}
}

/* ---------- clearing */

void d3d12r_clear(unsigned int color_handle, unsigned int depth_handle, unsigned int channels, const float rgba[4],
	int clear_depth, float z, int clear_stencil, unsigned int stencil, const long (*rectangles)[4],
	unsigned int count)
{
	struct d3d12_texture *color = d3d12_texture_get(color_handle);
	struct d3d12_texture *depth = d3d12_texture_get(depth_handle);
	D3D12_RECT rects[16];
	UINT rect_count = 0, index;

	for (index = 0; index < count && rect_count < 16; index++)
	{
		rects[rect_count].left = rectangles[index][0];
		rects[rect_count].top = rectangles[index][1];
		rects[rect_count].right = rectangles[index][2];
		rects[rect_count].bottom = rectangles[index][3];
		if (rects[rect_count].right > rects[rect_count].left && rects[rect_count].bottom > rects[rect_count].top)
			rect_count++;
	}
	if (!rect_count)
		return;
	if (color && channels)
	{
		if (channels == D3D12_COLOR_WRITE_ENABLE_ALL)
		{
			d3d12_texture_transition(color, D3D12_RESOURCE_STATE_RENDER_TARGET);
			ID3D12GraphicsCommandList_ClearRenderTargetView(d3d12.list, d3d12_descriptor_cpu(_d3d12_heap_rtv, color->rtv),
				rgba, rect_count, rects);
		}
		else
		{
			d3d12_pass_clear_channels(color, channels, rgba, rects, rect_count);
		}
	}
	if (depth && (clear_depth || clear_stencil))
	{
		D3D12_CLEAR_FLAGS flags = (D3D12_CLEAR_FLAGS)((clear_depth ? D3D12_CLEAR_FLAG_DEPTH : 0) |
			(clear_stencil ? D3D12_CLEAR_FLAG_STENCIL : 0));

		d3d12_texture_transition(depth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		ID3D12GraphicsCommandList_ClearDepthStencilView(d3d12.list, d3d12_descriptor_cpu(_d3d12_heap_dsv, depth->dsv),
			flags, z, (UINT8)stencil, rect_count, rects);
	}
}

/* ---------- visibility (occlusion) tests

Each test is a query of a ring; its count is resolved, as the frame is
presented, into the readback buffer at its slot, which the CPU reads when
the game asks, whatever the GPU has written there by then. */

#define QUERY_RING 8192
#define FRAME_QUERIES 4096

static ID3D12QueryHeap *query_heap;
static ID3D12Resource *query_results;
static volatile UINT64 *query_values;
static unsigned long query_next;
static long active_query = -1;
static struct
{
	unsigned long query, slot;
} frame_queries[FRAME_QUERIES];
static unsigned long frame_query_count;
/* each slot's last test: not handed to the GPU yet, or the fence value of the
submission that writes its count */
static BOOL slot_unsubmitted[D3D12R_VISIBILITY_SLOTS];
static UINT64 slot_fences[D3D12R_VISIBILITY_SLOTS];

static BOOL visibility_initialize(void)
{
	D3D12_QUERY_HEAP_DESC heap_description;
	D3D12_HEAP_PROPERTIES heap;
	D3D12_RESOURCE_DESC description;

	memset(&heap_description, 0, sizeof(heap_description));
	heap_description.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
	heap_description.Count = QUERY_RING;
	if (FAILED(ID3D12Device_CreateQueryHeap(d3d12.device, &heap_description, &IID_ID3D12QueryHeap, (void **)&query_heap)))
		return FALSE;
	memset(&heap, 0, sizeof(heap));
	heap.Type = D3D12_HEAP_TYPE_READBACK;
	memset(&description, 0, sizeof(description));
	description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	description.Width = D3D12R_VISIBILITY_SLOTS * sizeof(UINT64);
	description.Height = 1;
	description.DepthOrArraySize = 1;
	description.MipLevels = 1;
	description.SampleDesc.Count = 1;
	description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	if (FAILED(ID3D12Device_CreateCommittedResource(d3d12.device, &heap, D3D12_HEAP_FLAG_NONE, &description,
			D3D12_RESOURCE_STATE_COPY_DEST, NULL, &IID_ID3D12Resource, (void **)&query_results)) ||
		FAILED(ID3D12Resource_Map(query_results, 0, NULL, (void **)&query_values)))
	{
		return FALSE;
	}
	memset((void *)query_values, 0, D3D12R_VISIBILITY_SLOTS * sizeof(UINT64));
	return TRUE;
}

static void query_begin(void)
{
	active_query = (long)(query_next++ % QUERY_RING);
	ID3D12GraphicsCommandList_BeginQuery(d3d12.list, query_heap, D3D12_QUERY_TYPE_OCCLUSION, (UINT)active_query);
}

void d3d12r_visibility_begin(void)
{
	if (active_query < 0)
		query_begin();
}

void d3d12r_visibility_end(unsigned long slot)
{
	if (active_query < 0 || slot >= D3D12R_VISIBILITY_SLOTS)
		return;
	ID3D12GraphicsCommandList_EndQuery(d3d12.list, query_heap, D3D12_QUERY_TYPE_OCCLUSION, (UINT)active_query);
	if (frame_query_count < FRAME_QUERIES)
	{
		frame_queries[frame_query_count].query = (unsigned long)active_query;
		frame_queries[frame_query_count].slot = slot;
		frame_query_count++;
		slot_unsubmitted[slot] = TRUE;
	}
	active_query = -1;
	tests_ended = TRUE;
}

unsigned long long d3d12r_visibility_samples(unsigned long slot, int wait)
{
	if (slot >= D3D12R_VISIBILITY_SLOTS)
		return 0;
	if (wait)
	{
		if (slot_unsubmitted[slot] && active_query < 0)
			d3d12_submit();
		if (!slot_unsubmitted[slot])
			d3d12_fence_wait(slot_fences[slot]);
	}
	return query_values[slot];
}

void d3d12_visibility_resolve(void)
{
	unsigned long index;

	for (index = 0; index < frame_query_count; index++)
	{
		ID3D12GraphicsCommandList_ResolveQueryData(d3d12.list, query_heap, D3D12_QUERY_TYPE_OCCLUSION,
			(UINT)frame_queries[index].query, 1, query_results, frame_queries[index].slot * sizeof(UINT64));
		/* (a resolve is handed to the GPU next: d3d12r_present, d3d12_submit) */
		slot_unsubmitted[frame_queries[index].slot] = FALSE;
		slot_fences[frame_queries[index].slot] = d3d12_fence_next();
	}
	frame_query_count = 0;
}

void d3d12_visibility_before_flush(void)
{
	if (active_query >= 0)
		ID3D12GraphicsCommandList_EndQuery(d3d12.list, query_heap, D3D12_QUERY_TYPE_OCCLUSION, (UINT)active_query);
	d3d12_visibility_resolve();
}

void d3d12_visibility_after_flush(void)
{
	if (active_query >= 0)
		query_begin();
}

/* ---------- initialization */

BOOL d3d12_draw_initialize(void)
{
	static const D3D12_SRV_DIMENSION dimensions[4] =
	{
		D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_SRV_DIMENSION_TEXTURE3D,
		D3D12_SRV_DIMENSION_TEXTURECUBE,
	};
	D3D12_SAMPLER_DESC sampler;
	int index;

	/* the null SRVs of stages without a texture, of each dimension */
	for (index = 0; index < 4; index++)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC view;

		if ((d3d12_null_srv[index] = d3d12_descriptor_allocate(_d3d12_heap_srv)) < 0)
			return FALSE;
		memset(&view, 0, sizeof(view));
		view.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		view.ViewDimension = dimensions[index];
		view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		if (dimensions[index] == D3D12_SRV_DIMENSION_TEXTURE2D)
			view.Texture2D.MipLevels = 1;
		else if (dimensions[index] == D3D12_SRV_DIMENSION_TEXTURE3D)
			view.Texture3D.MipLevels = 1;
		else
			view.TextureCube.MipLevels = 1;
		ID3D12Device_CreateShaderResourceView(d3d12.device, NULL, &view,
			d3d12_descriptor_cpu(_d3d12_heap_srv, d3d12_null_srv[index]));
	}
	/* the sampler of the stages without a texture: the heap's first */
	memset(&sampler, 0, sizeof(sampler));
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.MaxAnisotropy = 1;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	if (d3d12_sampler_get(&sampler) != 0)
		platform_log("Direct3D 12: the default sampler is not the heap's first");
	if (!visibility_initialize())
	{
		platform_log("Direct3D 12: no visibility tests");
		return FALSE;
	}
	d3d12_draw_list_begin();
	return TRUE;
}
