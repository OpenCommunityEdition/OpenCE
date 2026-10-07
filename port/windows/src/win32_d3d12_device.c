/*
WIN32_D3D12_DEVICE.C

The Direct3D 12 renderer's device (d3d12_renderer.h, win32_d3d12.h): the
device and the swap chain in the game's window, the frames in flight with
their command list, upload memory and released objects, the descriptor
heaps, the root signatures, and presentation.

Frames are recorded while the GPU draws the ones before, up to
D3D12_FRAME_COUNT: each has its command allocator and the fence value its
work signals, and what it used (upload memory, objects released while it
was recorded) is reclaimed only once the GPU has passed that value.
debug.d3d12_debug turns on the debug layer, whose messages go to the log.
*/

#include "win32_d3d12.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct d3d12_renderer d3d12;

/* ---------- descriptors */

static struct
{
	long capacity;
	/* slots never handed out yet start at next; freed ones are stacked */
	long next;
	long *freed;
	long freed_count;
} descriptor_heaps[NUMBER_OF_D3D12_HEAPS];

static const D3D12_DESCRIPTOR_HEAP_TYPE heap_types[NUMBER_OF_D3D12_HEAPS] =
{
	D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
	D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
	D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
};

static const long heap_capacities[NUMBER_OF_D3D12_HEAPS] =
{
	D3D12_SRV_DESCRIPTORS, D3D12_SAMPLER_DESCRIPTORS, D3D12_RTV_DESCRIPTORS, D3D12_DSV_DESCRIPTORS,
};

long d3d12_descriptor_allocate(int heap)
{
	if (descriptor_heaps[heap].freed_count)
		return descriptor_heaps[heap].freed[--descriptor_heaps[heap].freed_count];
	if (descriptor_heaps[heap].next < descriptor_heaps[heap].capacity)
		return descriptor_heaps[heap].next++;
	platform_log("Direct3D 12: descriptor heap %d is full", heap);
	return -1;
}

/* each frame's slots freed while it was recorded */
static struct
{
	struct freed_descriptor
	{
		int heap;
		long slot;
	} *slots;
	unsigned long count, capacity;
} frame_freed_descriptors[D3D12_FRAME_COUNT];

void d3d12_descriptor_free(int heap, long slot)
{
	unsigned long frame = d3d12.frame_index;

	if (slot < 0)
		return;
	if (frame_freed_descriptors[frame].count == frame_freed_descriptors[frame].capacity)
	{
		unsigned long capacity = frame_freed_descriptors[frame].capacity ? frame_freed_descriptors[frame].capacity * 2 : 256;
		struct freed_descriptor *slots = realloc(frame_freed_descriptors[frame].slots, capacity * sizeof(*slots));

		/* (better lost than handed out while the GPU may read it) */
		if (!slots)
			return;
		frame_freed_descriptors[frame].slots = slots;
		frame_freed_descriptors[frame].capacity = capacity;
	}
	frame_freed_descriptors[frame].slots[frame_freed_descriptors[frame].count].heap = heap;
	frame_freed_descriptors[frame].slots[frame_freed_descriptors[frame].count].slot = slot;
	frame_freed_descriptors[frame].count++;
}

static void descriptors_reclaim(unsigned long frame)
{
	unsigned long index;

	for (index = 0; index < frame_freed_descriptors[frame].count; index++)
	{
		int heap = frame_freed_descriptors[frame].slots[index].heap;

		descriptor_heaps[heap].freed[descriptor_heaps[heap].freed_count++] = frame_freed_descriptors[frame].slots[index].slot;
	}
	frame_freed_descriptors[frame].count = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_descriptor_cpu(int heap, long slot)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle;

	ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(d3d12.heaps[heap], &handle);
	handle.ptr += (SIZE_T)slot * d3d12.descriptor_sizes[heap];
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE d3d12_descriptor_gpu(int heap, long slot)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle;

	ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(d3d12.heaps[heap], &handle);
	handle.ptr += (UINT64)slot * d3d12.descriptor_sizes[heap];
	return handle;
}

static BOOL descriptor_heaps_create(void)
{
	int heap;

	for (heap = 0; heap < NUMBER_OF_D3D12_HEAPS; heap++)
	{
		D3D12_DESCRIPTOR_HEAP_DESC description;

		memset(&description, 0, sizeof(description));
		description.Type = heap_types[heap];
		description.NumDescriptors = (UINT)heap_capacities[heap];
		description.Flags = heap == _d3d12_heap_srv || heap == _d3d12_heap_sampler ?
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (FAILED(ID3D12Device_CreateDescriptorHeap(d3d12.device, &description, &IID_ID3D12DescriptorHeap,
			(void **)&d3d12.heaps[heap])))
		{
			platform_log("Direct3D 12: cannot make descriptor heap %d", heap);
			return FALSE;
		}
		d3d12.descriptor_sizes[heap] = ID3D12Device_GetDescriptorHandleIncrementSize(d3d12.device, heap_types[heap]);
		descriptor_heaps[heap].capacity = heap_capacities[heap];
		descriptor_heaps[heap].next = 0;
		descriptor_heaps[heap].freed = calloc((size_t)heap_capacities[heap], sizeof(long));
		descriptor_heaps[heap].freed_count = 0;
		if (!descriptor_heaps[heap].freed)
			return FALSE;
	}
	return TRUE;
}

void d3d12_bind_heaps(void)
{
	ID3D12DescriptorHeap *heaps[2];

	heaps[0] = d3d12.heaps[_d3d12_heap_srv];
	heaps[1] = d3d12.heaps[_d3d12_heap_sampler];
	ID3D12GraphicsCommandList_SetDescriptorHeaps(d3d12.list, 2, heaps);
	ID3D12GraphicsCommandList_SetGraphicsRootSignature(d3d12.list, d3d12.root_signature);
}

/* ---------- upload memory */

#define UPLOAD_CHUNK_SIZE (4 * 1024 * 1024)
/* more than this many bytes of uploads not yet reclaimed (a map loading its
textures in one frame), and the GPU is waited for so that they are */
#define UPLOAD_OUTSTANDING_LIMIT ((UINT64)384 * 1024 * 1024)

struct upload_chunk
{
	struct upload_chunk *next;
	ID3D12Resource *resource;
	unsigned char *cpu;
	D3D12_GPU_VIRTUAL_ADDRESS gpu;
	UINT64 size, used;
};

static struct upload_chunk *free_chunks;
/* each frame's chunks, the one in use first */
static struct upload_chunk *frame_chunks[D3D12_FRAME_COUNT];
static UINT64 outstanding_upload_bytes;

static struct upload_chunk *upload_chunk_create(UINT64 size)
{
	struct upload_chunk *chunk = calloc(1, sizeof(*chunk));
	D3D12_HEAP_PROPERTIES heap;
	D3D12_RESOURCE_DESC description;
	D3D12_RANGE nothing = { 0, 0 };

	if (!chunk)
		return NULL;
	memset(&heap, 0, sizeof(heap));
	heap.Type = D3D12_HEAP_TYPE_UPLOAD;
	memset(&description, 0, sizeof(description));
	description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	description.Width = size;
	description.Height = 1;
	description.DepthOrArraySize = 1;
	description.MipLevels = 1;
	description.SampleDesc.Count = 1;
	description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	if (FAILED(ID3D12Device_CreateCommittedResource(d3d12.device, &heap, D3D12_HEAP_FLAG_NONE, &description,
			D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, (void **)&chunk->resource)) ||
		FAILED(ID3D12Resource_Map(chunk->resource, 0, &nothing, (void **)&chunk->cpu)))
	{
		platform_log("Direct3D 12: no upload memory for %llu bytes", (unsigned long long)size);
		if (chunk->resource)
			ID3D12Resource_Release(chunk->resource);
		free(chunk);
		return NULL;
	}
	chunk->gpu = ID3D12Resource_GetGPUVirtualAddress(chunk->resource);
	chunk->size = size;
	return chunk;
}

/* a frame's chunks back to the pool (the GPU has finished it); chunks larger
than the usual size are let go */
static void upload_chunks_reclaim(unsigned long frame)
{
	struct upload_chunk *chunk = frame_chunks[frame], *next;

	for (; chunk; chunk = next)
	{
		next = chunk->next;
		outstanding_upload_bytes -= chunk->size;
		if (chunk->size != UPLOAD_CHUNK_SIZE)
		{
			ID3D12Resource_Release(chunk->resource);
			free(chunk);
			continue;
		}
		chunk->used = 0;
		chunk->next = free_chunks;
		free_chunks = chunk;
	}
	frame_chunks[frame] = NULL;
}

BOOL d3d12_upload_allocate(UINT64 size, UINT64 alignment, struct d3d12_allocation *allocation)
{
	struct upload_chunk *chunk = frame_chunks[d3d12.frame_index];
	UINT64 offset;

	if (!size)
		size = 1;
	if (chunk)
	{
		offset = (chunk->used + alignment - 1) & ~(alignment - 1);
		if (offset + size > chunk->size)
			chunk = NULL;
	}
	if (!chunk)
	{
		UINT64 chunk_size = size + alignment > UPLOAD_CHUNK_SIZE ? (size + alignment + 0xffff) & ~(UINT64)0xffff :
			UPLOAD_CHUNK_SIZE;

		if (outstanding_upload_bytes + chunk_size > UPLOAD_OUTSTANDING_LIMIT)
			d3d12_flush_and_wait();
		if (chunk_size == UPLOAD_CHUNK_SIZE && free_chunks)
		{
			chunk = free_chunks;
			free_chunks = chunk->next;
		}
		else if (!(chunk = upload_chunk_create(chunk_size)))
		{
			return FALSE;
		}
		outstanding_upload_bytes += chunk->size;
		chunk->used = 0;
		chunk->next = frame_chunks[d3d12.frame_index];
		frame_chunks[d3d12.frame_index] = chunk;
		offset = 0;
	}
	chunk->used = offset + size;
	allocation->resource = chunk->resource;
	allocation->offset = offset;
	allocation->cpu = chunk->cpu + offset;
	allocation->gpu = chunk->gpu + offset;
	return TRUE;
}

void d3d12_upload_reserve(UINT64 size)
{
	struct upload_chunk *chunk = frame_chunks[d3d12.frame_index];
	struct d3d12_allocation allocation;

	if (chunk && chunk->used + size <= chunk->size)
		return;
	/* a chunk with room (made now, while nothing is half recorded), its
	bytes given back */
	if (d3d12_upload_allocate(size, 256, &allocation))
		frame_chunks[d3d12.frame_index]->used = allocation.offset;
}

/* ---------- objects released later */

struct release_entry
{
	struct release_entry *next;
	IUnknown *object;
};

static struct release_entry *frame_releases[D3D12_FRAME_COUNT];

void d3d12_release_later(IUnknown *object)
{
	struct release_entry *entry;

	if (!object)
		return;
	entry = malloc(sizeof(*entry));
	if (!entry)
	{
		/* (better leaked than released while the GPU may use it) */
		return;
	}
	entry->object = object;
	entry->next = frame_releases[d3d12.frame_index];
	frame_releases[d3d12.frame_index] = entry;
}

static void releases_reclaim(unsigned long frame)
{
	struct release_entry *entry = frame_releases[frame], *next;

	for (; entry; entry = next)
	{
		next = entry->next;
		IUnknown_Release(entry->object);
		free(entry);
	}
	frame_releases[frame] = NULL;
}

/* ---------- frames */

void d3d12_fence_wait(UINT64 value)
{
	if (ID3D12Fence_GetCompletedValue(d3d12.fence) >= value)
		return;
	ID3D12Fence_SetEventOnCompletion(d3d12.fence, value, d3d12.fence_event);
	WaitForSingleObject(d3d12.fence_event, INFINITE);
}

UINT64 d3d12_fence_next(void)
{
	return d3d12.fence_next + 1;
}

/* the command list closed and handed to the GPU; the fence value it signals */
static UINT64 submit(void)
{
	ID3D12CommandList *lists[1];
	HRESULT result;

	d3d12_events_close();
	result = ID3D12GraphicsCommandList_Close(d3d12.list);

	if (FAILED(result))
		platform_log("Direct3D 12: the command list does not close (%08lx)", (unsigned long)result);
	lists[0] = (ID3D12CommandList *)d3d12.list;
	ID3D12CommandQueue_ExecuteCommandLists(d3d12.queue, 1, lists);
	d3d12_resources_submitted();
	d3d12.fence_next++;
	ID3D12CommandQueue_Signal(d3d12.queue, d3d12.fence, d3d12.fence_next);
	d3d12.frame_fences[d3d12.frame_index] = d3d12.fence_next;
	return d3d12.fence_next;
}

/* the current frame's command list recording again */
static void list_restart(void)
{
	ID3D12CommandAllocator_Reset(d3d12.allocators[d3d12.frame_index]);
	ID3D12GraphicsCommandList_Reset(d3d12.list, d3d12.allocators[d3d12.frame_index], NULL);
	d3d12_bind_heaps();
	d3d12_draw_list_begin();
}

void d3d12_submit(void)
{
	d3d12_visibility_before_flush();
	submit();
	/* (the frame's allocator again: it keeps what the lists before recorded
	until the frame is reclaimed, and so does upload memory) */
	ID3D12GraphicsCommandList_Reset(d3d12.list, d3d12.allocators[d3d12.frame_index], NULL);
	d3d12_bind_heaps();
	d3d12_draw_state_invalidate();
	d3d12_visibility_after_flush();
}

void d3d12_flush_and_wait(void)
{
	unsigned long frame;

	d3d12_visibility_before_flush();
	d3d12_fence_wait(submit());
	for (frame = 0; frame < D3D12_FRAME_COUNT; frame++)
	{
		upload_chunks_reclaim(frame);
		releases_reclaim(frame);
		descriptors_reclaim(frame);
	}
	list_restart();
	d3d12_visibility_after_flush();
}

/* the next frame begun: its slot's last work finished and reclaimed */
static void frame_advance(void)
{
	d3d12.frame_index = (d3d12.frame_index + 1) % D3D12_FRAME_COUNT;
	d3d12_fence_wait(d3d12.frame_fences[d3d12.frame_index]);
	upload_chunks_reclaim(d3d12.frame_index);
	releases_reclaim(d3d12.frame_index);
	descriptors_reclaim(d3d12.frame_index);
	list_restart();
}

/* ---------- shaders */

ID3DBlob *d3d12_compile(const char *source, const char *target, const char *what)
{
	ID3DBlob *code = NULL, *errors = NULL;
	HRESULT result = D3DCompile(source, strlen(source), what, NULL, NULL, "main", target,
		D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &errors);

	if (FAILED(result))
	{
		platform_log("Direct3D 12: cannot compile the %s shader:\n%s\n%s", what,
			errors ? (const char *)ID3D10Blob_GetBufferPointer(errors) : "(no log)", source);
		if (code)
			ID3D10Blob_Release(code);
		code = NULL;
	}
	if (errors)
		ID3D10Blob_Release(errors);
	return code;
}

/* ---------- root signatures */

static ID3D12RootSignature *root_signature_create(const D3D12_ROOT_SIGNATURE_DESC *description, const char *what)
{
	ID3DBlob *blob = NULL, *errors = NULL;
	ID3D12RootSignature *signature = NULL;

	if (FAILED(D3D12SerializeRootSignature(description, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errors)) ||
		FAILED(ID3D12Device_CreateRootSignature(d3d12.device, 0, ID3D10Blob_GetBufferPointer(blob),
			ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, (void **)&signature)))
	{
		platform_log("Direct3D 12: cannot make the %s root signature: %s", what,
			errors ? (const char *)ID3D10Blob_GetBufferPointer(errors) : "");
		signature = NULL;
	}
	if (blob)
		ID3D10Blob_Release(blob);
	if (errors)
		ID3D10Blob_Release(errors);
	return signature;
}

/* the draws': the vertex shader's constants (b0) and the pixel shader's
(b1), each a root CBV; then the four stages' textures (t0 to t3) and
samplers (s0 to s3), a table of one each, pointed at their persistent
slots */
static BOOL root_signatures_create(void)
{
	D3D12_DESCRIPTOR_RANGE ranges[8];
	D3D12_ROOT_PARAMETER parameters[10];
	D3D12_ROOT_SIGNATURE_DESC description;
	D3D12_STATIC_SAMPLER_DESC samplers[2];
	int index;

	memset(ranges, 0, sizeof(ranges));
	memset(parameters, 0, sizeof(parameters));
	parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	parameters[0].Descriptor.ShaderRegister = 0;
	parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	parameters[1].Descriptor.ShaderRegister = 1;
	parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	for (index = 0; index < 8; index++)
	{
		ranges[index].RangeType = index < 4 ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV : D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		ranges[index].NumDescriptors = 1;
		ranges[index].BaseShaderRegister = (UINT)(index & 3);
		parameters[2 + index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[2 + index].DescriptorTable.NumDescriptorRanges = 1;
		parameters[2 + index].DescriptorTable.pDescriptorRanges = &ranges[index];
		parameters[2 + index].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}
	memset(&description, 0, sizeof(description));
	description.NumParameters = 10;
	description.pParameters = parameters;
	description.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	if (!(d3d12.root_signature = root_signature_create(&description, "draws'")))
		return FALSE;

	/* the passes': 16 constants (b0), three textures (t0 to t2), a linear
	and a point sampler clamped (s0, s1) */
	memset(ranges, 0, sizeof(ranges));
	memset(parameters, 0, sizeof(parameters));
	parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	parameters[0].Constants.ShaderRegister = 0;
	parameters[0].Constants.Num32BitValues = 16;
	parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	for (index = 0; index < 3; index++)
	{
		ranges[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[index].NumDescriptors = 1;
		ranges[index].BaseShaderRegister = (UINT)index;
		parameters[1 + index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[1 + index].DescriptorTable.NumDescriptorRanges = 1;
		parameters[1 + index].DescriptorTable.pDescriptorRanges = &ranges[index];
		parameters[1 + index].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}
	memset(samplers, 0, sizeof(samplers));
	for (index = 0; index < 2; index++)
	{
		samplers[index].Filter = index ? D3D12_FILTER_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplers[index].AddressU = samplers[index].AddressV = samplers[index].AddressW =
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplers[index].MaxLOD = D3D12_FLOAT32_MAX;
		samplers[index].ShaderRegister = (UINT)index;
		samplers[index].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}
	memset(&description, 0, sizeof(description));
	description.NumParameters = 4;
	description.pParameters = parameters;
	description.NumStaticSamplers = 2;
	description.pStaticSamplers = samplers;
	return (d3d12.pass_root_signature = root_signature_create(&description, "passes'")) != NULL;
}

/* ---------- the swap chain */

static void swap_buffers_get(void)
{
	UINT index;

	for (index = 0; index < D3D12_SWAP_BUFFER_COUNT; index++)
	{
		D3D12_RENDER_TARGET_VIEW_DESC view;

		IDXGISwapChain3_GetBuffer(d3d12.swap_chain, index, &IID_ID3D12Resource, (void **)&d3d12.swap_buffers[index]);
		if (d3d12.swap_rtv[index] < 0)
			d3d12.swap_rtv[index] = d3d12_descriptor_allocate(_d3d12_heap_rtv);
		memset(&view, 0, sizeof(view));
		view.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		ID3D12Device_CreateRenderTargetView(d3d12.device, d3d12.swap_buffers[index], &view,
			d3d12_descriptor_cpu(_d3d12_heap_rtv, d3d12.swap_rtv[index]));
	}
}

static BOOL swap_chain_create(int width, int height)
{
	DXGI_SWAP_CHAIN_DESC1 description;
	IDXGISwapChain1 *swap_chain = NULL;
	IDXGIFactory5 *factory5 = NULL;
	UINT index;

	if (width <= 0 || height <= 0)
	{
		width = 640;
		height = 480;
	}
	if (SUCCEEDED(IDXGIFactory4_QueryInterface(d3d12.factory, &IID_IDXGIFactory5, (void **)&factory5)))
	{
		BOOL allowed = FALSE;

		if (SUCCEEDED(IDXGIFactory5_CheckFeatureSupport(factory5, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowed,
			sizeof(allowed))))
		{
			d3d12.tearing = allowed;
		}
		IDXGIFactory5_Release(factory5);
	}
	memset(&description, 0, sizeof(description));
	description.Width = (UINT)width;
	description.Height = (UINT)height;
	description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	description.SampleDesc.Count = 1;
	description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	description.BufferCount = D3D12_SWAP_BUFFER_COUNT;
	description.Scaling = DXGI_SCALING_STRETCH;
	description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	description.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	description.Flags = d3d12.tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	if (FAILED(IDXGIFactory4_CreateSwapChainForHwnd(d3d12.factory, (IUnknown *)d3d12.queue, d3d12.window, &description,
			NULL, NULL, &swap_chain)) ||
		FAILED(IDXGISwapChain1_QueryInterface(swap_chain, &IID_IDXGISwapChain3, (void **)&d3d12.swap_chain)))
	{
		platform_log("Direct3D 12: cannot make the swap chain");
		if (swap_chain)
			IDXGISwapChain1_Release(swap_chain);
		return FALSE;
	}
	IDXGISwapChain1_Release(swap_chain);
	/* (SDL switches the window to and from fullscreen, F11) */
	IDXGIFactory4_MakeWindowAssociation(d3d12.factory, d3d12.window, DXGI_MWA_NO_ALT_ENTER);
	d3d12.swap_width = (UINT)width;
	d3d12.swap_height = (UINT)height;
	for (index = 0; index < D3D12_SWAP_BUFFER_COUNT; index++)
		d3d12.swap_rtv[index] = -1;
	swap_buffers_get();
	return TRUE;
}

/* the swap chain made the window's size, when it has changed */
static void swap_chain_fit(int width, int height)
{
	UINT index;

	if (width <= 0 || height <= 0 || ((UINT)width == d3d12.swap_width && (UINT)height == d3d12.swap_height))
		return;
	d3d12_flush_and_wait();
	for (index = 0; index < D3D12_SWAP_BUFFER_COUNT; index++)
	{
		ID3D12Resource_Release(d3d12.swap_buffers[index]);
		d3d12.swap_buffers[index] = NULL;
	}
	if (FAILED(IDXGISwapChain3_ResizeBuffers(d3d12.swap_chain, D3D12_SWAP_BUFFER_COUNT, (UINT)width, (UINT)height,
		DXGI_FORMAT_B8G8R8A8_UNORM, d3d12.tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0)))
	{
		platform_log("Direct3D 12: cannot resize the swap chain to %dx%d", width, height);
	}
	else
	{
		d3d12.swap_width = (UINT)width;
		d3d12.swap_height = (UINT)height;
	}
	swap_buffers_get();
}

/* ---------- the debug layer's messages */

static ID3D12InfoQueue *info_queue;

static void debug_messages_log(void)
{
	UINT64 count, index;

	if (!info_queue)
		return;
	count = ID3D12InfoQueue_GetNumStoredMessages(info_queue);
	for (index = 0; index < count && index < 64; index++)
	{
		SIZE_T size = 0;
		D3D12_MESSAGE *message;

		ID3D12InfoQueue_GetMessage(info_queue, index, NULL, &size);
		message = malloc(size);
		if (!message)
			break;
		if (SUCCEEDED(ID3D12InfoQueue_GetMessage(info_queue, index, message, &size)) &&
			message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING)
		{
			static const char *const severities[] = { "CORRUPTION", "ERROR", "WARNING", "INFO", "MESSAGE" };

			platform_log("Direct3D 12 %s: %.*s", severities[message->Severity < 5 ? message->Severity : 4],
				(int)message->DescriptionByteLength, message->pDescription);
		}
		free(message);
	}
	ID3D12InfoQueue_ClearStoredMessages(info_queue);
}

/* ---------- the device */

static void log_adapter(IDXGIAdapter1 *adapter)
{
	DXGI_ADAPTER_DESC1 description;
	char name[128];

	if (FAILED(IDXGIAdapter1_GetDesc1(adapter, &description)))
		return;
	WideCharToMultiByte(CP_UTF8, 0, description.Description, -1, name, sizeof(name), NULL, NULL);
	/* (not its memory: a 32-bit process is told at most 4 GB of it) */
	platform_log("Direct3D 12 on %s", name);
}

static BOOL device_create(BOOL debug, BOOL gpu_validation)
{
	IDXGIFactory6 *factory6 = NULL;
	D3D12_COMMAND_QUEUE_DESC queue;
	UINT index;

	d3d12.debug = debug;
	if (d3d12.debug)
	{
		ID3D12Debug *debug = NULL;

		if (SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug, (void **)&debug)))
		{
			ID3D12Debug1 *debug1 = NULL;

			ID3D12Debug_EnableDebugLayer(debug);
			if (gpu_validation &&
				SUCCEEDED(ID3D12Debug_QueryInterface(debug, &IID_ID3D12Debug1, (void **)&debug1)))
			{
				ID3D12Debug1_SetEnableGPUBasedValidation(debug1, TRUE);
				ID3D12Debug1_Release(debug1);
			}
			ID3D12Debug_Release(debug);
		}
		else
		{
			platform_log("Direct3D 12: no debug layer (the Graphics Tools feature of Windows)");
		}
	}
	if (FAILED(CreateDXGIFactory2(d3d12.debug ? DXGI_CREATE_FACTORY_DEBUG : 0, &IID_IDXGIFactory4,
		(void **)&d3d12.factory)))
	{
		platform_log("Direct3D 12: no DXGI factory");
		return FALSE;
	}
	/* the high-performance GPU, else the first that is no software adapter */
	if (SUCCEEDED(IDXGIFactory4_QueryInterface(d3d12.factory, &IID_IDXGIFactory6, (void **)&factory6)))
	{
		for (index = 0; !d3d12.device && SUCCEEDED(IDXGIFactory6_EnumAdapterByGpuPreference(factory6, index,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, &IID_IDXGIAdapter1, (void **)&d3d12.adapter)); index++)
		{
			DXGI_ADAPTER_DESC1 description;

			if (SUCCEEDED(IDXGIAdapter1_GetDesc1(d3d12.adapter, &description)) &&
				!(description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
				SUCCEEDED(D3D12CreateDevice((IUnknown *)d3d12.adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device,
					(void **)&d3d12.device)))
			{
				break;
			}
			IDXGIAdapter1_Release(d3d12.adapter);
			d3d12.adapter = NULL;
		}
		IDXGIFactory6_Release(factory6);
	}
	for (index = 0; !d3d12.device && SUCCEEDED(IDXGIFactory4_EnumAdapters1(d3d12.factory, index, &d3d12.adapter));
		index++)
	{
		DXGI_ADAPTER_DESC1 description;

		if (SUCCEEDED(IDXGIAdapter1_GetDesc1(d3d12.adapter, &description)) &&
			!(description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
			SUCCEEDED(D3D12CreateDevice((IUnknown *)d3d12.adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device,
				(void **)&d3d12.device)))
		{
			break;
		}
		IDXGIAdapter1_Release(d3d12.adapter);
		d3d12.adapter = NULL;
	}
	if (!d3d12.device)
	{
		platform_log("Direct3D 12: no GPU that has it");
		return FALSE;
	}
	log_adapter(d3d12.adapter);
	if (d3d12.debug &&
		SUCCEEDED(ID3D12Device_QueryInterface(d3d12.device, &IID_ID3D12InfoQueue, (void **)&info_queue)))
	{
		/* (what the renderer does on purpose: clears to the game's colors,
		which no target can be made for in advance; buffers made in a state
		they are said to take anyway) */
		static D3D12_MESSAGE_ID muted[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED,
		};
		D3D12_INFO_QUEUE_FILTER filter;

		memset(&filter, 0, sizeof(filter));
		filter.DenyList.NumIDs = sizeof(muted) / sizeof(muted[0]);
		filter.DenyList.pIDList = muted;
		ID3D12InfoQueue_AddStorageFilterEntries(info_queue, &filter);
		ID3D12InfoQueue_SetMuteDebugOutput(info_queue, FALSE);
	}

	memset(&queue, 0, sizeof(queue));
	queue.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if (FAILED(ID3D12Device_CreateCommandQueue(d3d12.device, &queue, &IID_ID3D12CommandQueue, (void **)&d3d12.queue)) ||
		FAILED(ID3D12Device_CreateFence(d3d12.device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence,
			(void **)&d3d12.fence)))
	{
		platform_log("Direct3D 12: no command queue");
		return FALSE;
	}
	d3d12.fence_event = CreateEventA(NULL, FALSE, FALSE, NULL);
	for (index = 0; index < D3D12_FRAME_COUNT; index++)
	{
		if (FAILED(ID3D12Device_CreateCommandAllocator(d3d12.device, D3D12_COMMAND_LIST_TYPE_DIRECT,
			&IID_ID3D12CommandAllocator, (void **)&d3d12.allocators[index])))
		{
			return FALSE;
		}
	}
	if (FAILED(ID3D12Device_CreateCommandList(d3d12.device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d12.allocators[0],
		NULL, &IID_ID3D12GraphicsCommandList, (void **)&d3d12.list)))
	{
		return FALSE;
	}
	if (!descriptor_heaps_create() || !root_signatures_create())
		return FALSE;

	/* the most samples a pixel both the color and depth formats take */
	d3d12.maximum_samples = 1;
	for (index = 2; index <= 8; index *= 2)
	{
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levels;
		DXGI_FORMAT formats[2] = { DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT };
		int format;

		for (format = 0; format < 2; format++)
		{
			memset(&levels, 0, sizeof(levels));
			levels.Format = formats[format];
			levels.SampleCount = index;
			if (FAILED(ID3D12Device_CheckFeatureSupport(d3d12.device, D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &levels,
				sizeof(levels))) || !levels.NumQualityLevels)
			{
				break;
			}
		}
		if (format == 2)
			d3d12.maximum_samples = (int)index;
	}
	d3d12_bind_heaps();
	return TRUE;
}

void d3d12r_shutdown(void)
{
	/* (only after a failed start: nothing has been drawn) */
	if (d3d12.swap_chain)
		IDXGISwapChain3_Release(d3d12.swap_chain);
	if (d3d12.list)
		ID3D12GraphicsCommandList_Release(d3d12.list);
	if (d3d12.queue)
		ID3D12CommandQueue_Release(d3d12.queue);
	if (d3d12.device)
		ID3D12Device_Release(d3d12.device);
	if (d3d12.adapter)
		IDXGIAdapter1_Release(d3d12.adapter);
	if (d3d12.factory)
		IDXGIFactory4_Release(d3d12.factory);
	memset(&d3d12, 0, sizeof(d3d12));
}

int d3d12r_initialize(void *window, int width, int height, int debug, int gpu_validation, int *maximum_samples,
	long *maximum_target_size)
{
	d3d12.window = (HWND)window;
	if (!d3d12.window || !device_create(debug, gpu_validation) || !swap_chain_create(width, height) ||
		!d3d12_passes_initialize() || !d3d12_draw_initialize())
	{
		d3d12r_shutdown();
		return FALSE;
	}
	*maximum_samples = d3d12.maximum_samples;
	*maximum_target_size = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	return TRUE;
}

void *d3d12r_shader_compile(const char *source, int pixel, const char *name)
{
	return d3d12_compile(source, pixel ? "ps_5_0" : "vs_5_0", name);
}

/* ---------- presentation */

void d3d12r_present(unsigned int back_buffer, int window_width, int window_height, int vsync)
{
	struct d3d12_texture *source = d3d12_texture_get(back_buffer);
	UINT index;
	HRESULT result;

	swap_chain_fit(window_width, window_height);
	index = IDXGISwapChain3_GetCurrentBackBufferIndex(d3d12.swap_chain);
	if (source)
	{
		d3d12_pass_blit(source, d3d12.swap_buffers[index], d3d12.swap_rtv[index], d3d12.swap_width,
			d3d12.swap_height);
	}
	d3d12_visibility_resolve();
	submit();
	result = IDXGISwapChain3_Present(d3d12.swap_chain, vsync ? 1 : 0,
		!vsync && d3d12.tearing ? DXGI_PRESENT_ALLOW_TEARING : 0);
	if (FAILED(result))
	{
		platform_log("Direct3D 12: Present failed (%08lx; the device: %08lx)", (unsigned long)result,
			(unsigned long)ID3D12Device_GetDeviceRemovedReason(d3d12.device));
	}
	debug_messages_log();
	frame_advance();
}

int d3d12r_read_pixels(unsigned int texture, unsigned char *pixels, unsigned long width, unsigned long height)
{
	struct d3d12_texture *source = d3d12_texture_get(texture);
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	D3D12_RESOURCE_DESC description;
	D3D12_HEAP_PROPERTIES heap;
	D3D12_TEXTURE_COPY_LOCATION destination, from;
	ID3D12Resource *readback = NULL;
	UINT64 total = 0;
	unsigned char *mapped = NULL;
	unsigned long row;
	int read = 0;

	if (!source || source->width != width || source->height != height || source->samples > 1)
		return 0;
	ID3D12Resource_GetDesc(source->resource, &description);
	ID3D12Device_GetCopyableFootprints(d3d12.device, &description, 0, 1, 0, &footprint, NULL, NULL, &total);
	memset(&heap, 0, sizeof(heap));
	heap.Type = D3D12_HEAP_TYPE_READBACK;
	memset(&description, 0, sizeof(description));
	description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	description.Width = total;
	description.Height = 1;
	description.DepthOrArraySize = 1;
	description.MipLevels = 1;
	description.SampleDesc.Count = 1;
	description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	if (FAILED(ID3D12Device_CreateCommittedResource(d3d12.device, &heap, D3D12_HEAP_FLAG_NONE, &description,
		D3D12_RESOURCE_STATE_COPY_DEST, NULL, &IID_ID3D12Resource, (void **)&readback)))
	{
		return 0;
	}
	d3d12_texture_transition(source, D3D12_RESOURCE_STATE_COPY_SOURCE);
	memset(&destination, 0, sizeof(destination));
	destination.pResource = readback;
	destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	destination.PlacedFootprint = footprint;
	memset(&from, 0, sizeof(from));
	from.pResource = source->resource;
	from.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	from.SubresourceIndex = 0;
	ID3D12GraphicsCommandList_CopyTextureRegion(d3d12.list, &destination, 0, 0, 0, &from, NULL);
	d3d12_flush_and_wait();
	if (SUCCEEDED(ID3D12Resource_Map(readback, 0, NULL, (void **)&mapped)))
	{
		/* (B8G8R8A8: the BMP's byte order already) */
		for (row = 0; row < height; row++)
			memcpy(pixels + row * width * 4, mapped + footprint.Offset + row * footprint.Footprint.RowPitch, width * 4);
		ID3D12Resource_Unmap(readback, 0, NULL);
		read = 1;
	}
	ID3D12Resource_Release(readback);
	return read;
}
