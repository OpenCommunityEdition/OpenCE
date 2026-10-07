/*
WIN32_D3D12.H

The Direct3D 12 renderer's host half (d3d12_renderer.h), shared by its
files, which see the Windows SDK:
- win32_d3d12_device.c: the device, the swap chain, the frames in flight and
  their command list, upload memory, descriptors, presentation;
- win32_d3d12_resources.c: textures, render targets, the mirror's buffers,
  samplers;
- win32_d3d12_draw.c: pipeline states, the recording of the draws, clears
  and visibility tests;
- win32_d3d12_passes.c: the renderer's own passes (the window blit, partial
  clears, FXAA and SMAA, mipmaps).
*/

#ifndef __HALO_WINDOWS_WIN32_D3D12_H
#define __HALO_WINDOWS_WIN32_D3D12_H

#define COBJMACROS
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "d3d12_renderer.h"

/* frames recorded while the GPU draws the ones before */
#define D3D12_FRAME_COUNT 3
#define D3D12_SWAP_BUFFER_COUNT 3

/* ---------- descriptors

Shader-visible heaps of SRVs and samplers whose slots are kept by what they
describe (each texture its SRV, each sampler state its sampler), so that a
draw points its tables at them and copies nothing; and CPU heaps of render
target and depth views. */

#define D3D12_SRV_DESCRIPTORS 32768
#define D3D12_SAMPLER_DESCRIPTORS 2048
#define D3D12_RTV_DESCRIPTORS 4096
#define D3D12_DSV_DESCRIPTORS 1024

enum
{
	_d3d12_heap_srv,
	_d3d12_heap_sampler,
	_d3d12_heap_rtv,
	_d3d12_heap_dsv,
	NUMBER_OF_D3D12_HEAPS
};

/* a slot of a heap, or -1 when it is full */
long d3d12_descriptor_allocate(int heap);
/* a slot back to its heap once the GPU has finished the frames that may read
it (a shader-visible descriptor is read as the draw runs) */
void d3d12_descriptor_free(int heap, long slot);
D3D12_CPU_DESCRIPTOR_HANDLE d3d12_descriptor_cpu(int heap, long slot);
D3D12_GPU_DESCRIPTOR_HANDLE d3d12_descriptor_gpu(int heap, long slot);

/* ---------- textures (the renderer's handles, and render targets) */

struct d3d12_texture
{
	ID3D12Resource *resource;
	D3D12_RESOURCE_STATES state;
	/* the resource's format, and the views' */
	DXGI_FORMAT format, view_format;
	int type, renderer_format;
	unsigned long width, height, depth, levels;
	int samples;
	BOOL render_target, depth_stencil;
	/* the SRV's slot (-1: none); a render target's view's or a depth
	stencil's */
	long srv, rtv, dsv;
	/* each level's SRV and RTV, made for the passes that make mip levels
	(NULL until then) */
	long *level_srv, *level_rtv;
};

/* the texture of a handle; NULL for 0 or one deleted */
struct d3d12_texture *d3d12_texture_get(unsigned int handle);
/* a texture made, its handle; 0 if it cannot be */
unsigned int d3d12_texture_create(int type, DXGI_FORMAT format, DXGI_FORMAT view_format, unsigned long width,
	unsigned long height, unsigned long depth, unsigned long levels, int samples, BOOL render_target,
	BOOL depth_stencil);
void d3d12_texture_destroy(unsigned int handle);
/* the texture taken to state, a barrier recorded if it is in another */
void d3d12_texture_transition(struct d3d12_texture *texture, D3D12_RESOURCE_STATES state);
/* data written into a subresource of the texture at row first_row (rows of
row_bytes, packed) */
void d3d12_texture_upload(struct d3d12_texture *texture, UINT subresource, unsigned long first_row,
	unsigned long rows, unsigned long row_bytes, const void *data);

/* the null SRVs, by sampler type (none and 2D, 3D, cube), of stages with no
texture */
extern long d3d12_null_srv[4];

/* the slot of a sampler; -1 if the heap is full */
long d3d12_sampler_get(const D3D12_SAMPLER_DESC *description);

/* ---------- upload memory

Each frame's uploads (constants, streamed vertices and indices, texels)
come from chunks of an upload heap, mapped once, which go back to the pool
when the GPU has finished the frame. */

struct d3d12_allocation
{
	ID3D12Resource *resource;
	UINT64 offset;
	unsigned char *cpu;
	D3D12_GPU_VIRTUAL_ADDRESS gpu;
};

/* size bytes aligned to alignment (a power of two, at most 65536); FALSE if
there is no memory. Memory running short hands the GPU what is recorded
(d3d12_flush_and_wait), which a draw must not have half recorded: */
BOOL d3d12_upload_allocate(UINT64 size, UINT64 alignment, struct d3d12_allocation *allocation);
/* ... so it reserves what its allocations take first, which then come from
the same chunk without a flush */
void d3d12_upload_reserve(UINT64 size);

/* ---------- the mirror's buffers */

/* the GPU address of a segment's offset; 0 if the segment has no buffer */
D3D12_GPU_VIRTUAL_ADDRESS d3d12_mirror_address(unsigned long segment, unsigned long offset);
/* the segment's buffer made readable by draws (as vertices and indices), as
a draw that reads it is recorded */
void d3d12_mirror_use(unsigned long segment);
/* the command list handed to the GPU: buffers' states decay to COMMON there */
void d3d12_resources_submitted(void);

/* ---------- the device */

struct d3d12_renderer
{
	IDXGIFactory4 *factory;
	IDXGIAdapter1 *adapter;
	ID3D12Device *device;
	ID3D12CommandQueue *queue;
	IDXGISwapChain3 *swap_chain;
	ID3D12Resource *swap_buffers[D3D12_SWAP_BUFFER_COUNT];
	long swap_rtv[D3D12_SWAP_BUFFER_COUNT];
	UINT swap_width, swap_height;
	BOOL tearing;
	HWND window;

	ID3D12Fence *fence;
	UINT64 fence_next;
	HANDLE fence_event;
	ID3D12CommandAllocator *allocators[D3D12_FRAME_COUNT];
	UINT64 frame_fences[D3D12_FRAME_COUNT];
	unsigned long frame_index;
	ID3D12GraphicsCommandList *list;

	ID3D12DescriptorHeap *heaps[NUMBER_OF_D3D12_HEAPS];
	UINT descriptor_sizes[NUMBER_OF_D3D12_HEAPS];

	ID3D12RootSignature *root_signature;
	ID3D12RootSignature *pass_root_signature;

	/* the most samples a pixel of B8G8R8A8 and D24S8 both */
	int maximum_samples;
	BOOL debug;
};

extern struct d3d12_renderer d3d12;

/* everything recorded so far handed to the GPU, waited for, and a new
command list begun (the upload memory and released objects reclaimed); the
draws' state is set again from the next draw */
void d3d12_flush_and_wait(void);
/* everything recorded so far handed to the GPU, not waited for, and the
command list begun again in the same frame (the GPU starts on a frame's work
before the frame is presented, as OpenGL's drivers stream it) */
void d3d12_submit(void);
/* the GPU waited for until it has signalled value; the value the next
submission signals */
void d3d12_fence_wait(UINT64 value);
UINT64 d3d12_fence_next(void);
/* an object released once the GPU has finished the frames that may use it */
void d3d12_release_later(IUnknown *object);
/* the command list's state as it was set by the draws, forgotten (a pass
that set its own) */
void d3d12_draw_state_invalidate(void);
/* a new command list: its state, and the uploads the draws shared (their
memory reclaimed), forgotten */
void d3d12_draw_list_begin(void);
/* the root signature and heaps set on the command list again */
void d3d12_bind_heaps(void);

/* HLSL compiled (vs_5_0, ps_5_0); NULL with the log written */
ID3DBlob *d3d12_compile(const char *source, const char *target, const char *what);

/* size bytes a multiple of 4 */
static inline unsigned long d3d12_hash(const void *data, unsigned long size)
{
	const DWORD *words = data;
	unsigned long hash = 2166136261UL;

	for (size /= 4; size; size--)
		hash = (hash ^ *words++) * 16777619UL;
	return hash;
}

/* ---------- the draws (win32_d3d12_draw.c) */

BOOL d3d12_draw_initialize(void);
/* the frame's visibility counts written where the CPU reads them */
void d3d12_visibility_resolve(void);
/* a test under way as the command list is handed to the GPU mid-frame ends
in it and goes on in the next */
void d3d12_visibility_before_flush(void);
void d3d12_visibility_after_flush(void);

/* the command list's event of the game's pass ended (before it is closed) */
void d3d12_events_close(void);

/* ---------- the renderer's passes (win32_d3d12_passes.c) */

BOOL d3d12_passes_initialize(void);
/* the back buffer, letterboxed, into the window's swap buffer */
void d3d12_pass_blit(struct d3d12_texture *source, ID3D12Resource *destination, long destination_rtv,
	UINT destination_width, UINT destination_height);
/* channels of a target cleared to color over rectangles (D3D12's clears
take every channel) */
void d3d12_pass_clear_channels(struct d3d12_texture *target, UINT channel_mask, const float color[4],
	const D3D12_RECT *rectangles, UINT count);
/* levels from first_level on made from the ones before (2D, render target) */
void d3d12_pass_mipmaps(struct d3d12_texture *texture, unsigned long first_level);

#endif
