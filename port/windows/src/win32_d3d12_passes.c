/*
WIN32_D3D12_PASSES.C

The Direct3D 12 renderer's own passes (win32_d3d12.h), which draw one
triangle over a rectangle with programs of their own: the back buffer into
the window's swap buffer, clears of some channels only, display.anti_aliasing's
FXAA and SMAA (as xgpu_post.c draws them for OpenGL, from the same text), and
mip levels made from the ones before.

They have a root signature of their own (win32_d3d12_device.c): 16 constants (b0),
three textures (t0 to t2), a linear and a point sampler clamped (s0, s1). A
pass leaves the command list's state as it is, and the draws set theirs
again (d3d12_draw_state_invalidate).
*/

#include "win32_d3d12.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

enum
{
	_pass_blit,
	_pass_clear,
	_pass_mipmap,
	_pass_fxaa,
	_pass_smaa_edges,
	_pass_smaa_weights,
	_pass_smaa_blend,
	NUMBER_OF_PASS_PROGRAMS
};

/* ---------- programs */

/* every pass's constants: the rectangle's metrics (as each pass takes
them), FXAA's bounds, a clear's color */
static const char pass_header[] =
	"cbuffer pass_constants : register(b0)\n"
	"{\n"
	"\tfloat4 metrics;\n"
	"\tfloat4 bounds;\n"
	"\tfloat4 pass_color;\n"
	"\tfloat4 pass_unused;\n"
	"};\n"
	"SamplerState linear_sampler : register(s0);\n"
	"SamplerState point_sampler : register(s1);\n";

/* one triangle over the whole viewport */
static const char vertex_source[] =
	"float4 main(uint id : SV_VertexID) : SV_Position\n"
	"{\n"
	"\treturn float4(float((id & 1) << 2) - 1.0, float((id & 2) << 1) - 1.0, 0.0, 1.0);\n"
	"}\n";

/* the back buffer over the viewport: metrics is its corner and 1 / its size */
static const char blit_source[] =
	"Texture2D source_texture : register(t0);\n"
	"float4 main(float4 position : SV_Position) : SV_Target\n"
	"{\n"
	"\treturn float4(source_texture.SampleLevel(linear_sampler, (position.xy - metrics.xy) * metrics.zw, 0.0).rgb, 1.0);\n"
	"}\n";

static const char clear_source[] =
	"float4 main(float4 position : SV_Position) : SV_Target\n"
	"{\n"
	"\treturn pass_color;\n"
	"}\n";

/* a level from the one before (t0's only level): metrics.xy is 1 / its size,
the four texels under a pixel averaged by sampling between them */
static const char mipmap_source[] =
	"Texture2D source_texture : register(t0);\n"
	"float4 main(float4 position : SV_Position) : SV_Target\n"
	"{\n"
	"\treturn source_texture.SampleLevel(linear_sampler, position.xy * metrics.xy, 0.0);\n"
	"}\n";

/* xgpu_post.c's FXAA in HLSL */
static const char fxaa_source[] =
	"Texture2D color_texture : register(t0);\n"
	"\n"
	"static const float EDGE_THRESHOLD = 0.125;\n"
	"static const float EDGE_THRESHOLD_MINIMUM = 0.0312;\n"
	"static const float SUBPIXEL_QUALITY = 0.75;\n"
	"static const int STEPS = 12;\n"
	"static const float STEP_LENGTHS[12] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };\n"
	"\n"
	"float3 color_at(float2 position)\n"
	"{\n"
	"\treturn color_texture.SampleLevel(linear_sampler, clamp(position, bounds.xy, bounds.zw), 0.0).rgb;\n"
	"}\n"
	"\n"
	"float luma_at(float2 position)\n"
	"{\n"
	"\treturn dot(color_at(position), float3(0.299, 0.587, 0.114));\n"
	"}\n"
	"\n"
	"float4 main(float4 fragment : SV_Position) : SV_Target\n"
	"{\n"
	"\tfloat2 position = fragment.xy * metrics.xy;\n"
	"\tfloat luma = luma_at(position);\n"
	"\tfloat up = luma_at(position + float2(0.0, -metrics.y));\n"
	"\tfloat down = luma_at(position + float2(0.0, metrics.y));\n"
	"\tfloat left = luma_at(position + float2(-metrics.x, 0.0));\n"
	"\tfloat right = luma_at(position + float2(metrics.x, 0.0));\n"
	"\tfloat highest = max(luma, max(max(up, down), max(left, right)));\n"
	"\tfloat range = highest - min(luma, min(min(up, down), min(left, right)));\n"
	"\n"
	"\tif (range < max(EDGE_THRESHOLD_MINIMUM, highest * EDGE_THRESHOLD))\n"
	"\t\tdiscard;\n"
	"\tfloat up_left = luma_at(position + float2(-metrics.x, -metrics.y));\n"
	"\tfloat up_right = luma_at(position + float2(metrics.x, -metrics.y));\n"
	"\tfloat down_left = luma_at(position + float2(-metrics.x, metrics.y));\n"
	"\tfloat down_right = luma_at(position + float2(metrics.x, metrics.y));\n"
	"\n"
	"\tfloat average = (2.0 * (up + down + left + right) + up_left + up_right + down_left + down_right) / 12.0;\n"
	"\tfloat subpixel = smoothstep(0.0, 1.0, clamp(abs(average - luma) / range, 0.0, 1.0));\n"
	"\tsubpixel = subpixel * subpixel * SUBPIXEL_QUALITY;\n"
	"\n"
	"\tbool horizontal = abs(up_left + down_left - 2.0 * left) + 2.0 * abs(up + down - 2.0 * luma) +\n"
	"\t\tabs(up_right + down_right - 2.0 * right) >=\n"
	"\t\tabs(up_left + up_right - 2.0 * up) + 2.0 * abs(left + right - 2.0 * luma) +\n"
	"\t\tabs(down_left + down_right - 2.0 * down);\n"
	"\tfloat before = horizontal ? up : left;\n"
	"\tfloat after = horizontal ? down : right;\n"
	"\tbool before_steeper = abs(before - luma) >= abs(after - luma);\n"
	"\tfloat gradient = 0.25 * max(abs(before - luma), abs(after - luma));\n"
	"\tfloat edge_luma = 0.5 * ((before_steeper ? before : after) + luma);\n"
	"\tfloat2 across = (horizontal ? float2(0.0, metrics.y) : float2(metrics.x, 0.0)) * (before_steeper ? -1.0 : 1.0);\n"
	"\tfloat2 along = horizontal ? float2(metrics.x, 0.0) : float2(0.0, metrics.y);\n"
	"\n"
	"\tfloat2 end_before = position + 0.5 * across - along;\n"
	"\tfloat2 end_after = position + 0.5 * across + along;\n"
	"\tfloat luma_before = luma_at(end_before) - edge_luma;\n"
	"\tfloat luma_after = luma_at(end_after) - edge_luma;\n"
	"\tbool reached_before = abs(luma_before) >= gradient;\n"
	"\tbool reached_after = abs(luma_after) >= gradient;\n"
	"\t[loop] for (int index = 1; index < STEPS && !(reached_before && reached_after); index++)\n"
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
	"\tfloat distance_before = horizontal ? position.x - end_before.x : position.y - end_before.y;\n"
	"\tfloat distance_after = horizontal ? end_after.x - position.x : end_after.y - position.y;\n"
	"\tfloat pixel_offset = 0.5 - min(distance_before, distance_after) / (distance_before + distance_after);\n"
	"\tbool varies = ((distance_before < distance_after ? luma_before : luma_after) < 0.0) != (luma < edge_luma);\n"
	"\treturn float4(color_at(position + max(varies ? pixel_offset : 0.0, subpixel) * across), 1.0);\n"
	"}\n";

/* SMAA.hlsl's porting functions, with the samplers in the passes' registers */
static const char smaa_header[] =
	"#define SMAA_CUSTOM_SL 1\n"
	"#define SMAA_RT_METRICS metrics\n"
	"#define SMAA_PRESET_HIGH 1\n"
	"#define SMAATexture2D(tex) Texture2D tex\n"
	"#define SMAATexturePass2D(tex) tex\n"
	"#define SMAASampleLevelZero(tex, coord) tex.SampleLevel(linear_sampler, coord, 0)\n"
	"#define SMAASampleLevelZeroPoint(tex, coord) tex.SampleLevel(point_sampler, coord, 0)\n"
	"#define SMAASampleLevelZeroOffset(tex, coord, offset) tex.SampleLevel(linear_sampler, coord, 0, offset)\n"
	"#define SMAASample(tex, coord) tex.Sample(linear_sampler, coord)\n"
	"#define SMAASamplePoint(tex, coord) tex.Sample(point_sampler, coord)\n"
	"#define SMAASampleOffset(tex, coord, offset) tex.Sample(linear_sampler, coord, offset)\n"
	"#define SMAA_FLATTEN [flatten]\n"
	"#define SMAA_BRANCH [branch]\n"
	"#define SMAAGather(tex, coord) tex.Gather(linear_sampler, coord, 0)\n";

/* the main functions of SMAA's passes, as xgpu_post.c's */
static const char *const smaa_mains[] =
{
	/* edges */
	"Texture2D color_texture : register(t0);\n"
	"float4 main(float4 fragment : SV_Position) : SV_Target\n"
	"{\n"
	"\tfloat2 texcoord = fragment.xy * metrics.xy;\n"
	"\tfloat4 offset[3];\n"
	"\tSMAAEdgeDetectionVS(texcoord, offset);\n"
	"\treturn float4(SMAALumaEdgeDetectionPS(texcoord, offset, color_texture), 0.0, 0.0);\n"
	"}\n",
	/* weights */
	"Texture2D edges_texture : register(t0);\n"
	"Texture2D area_texture : register(t1);\n"
	"Texture2D search_texture : register(t2);\n"
	"float4 main(float4 fragment : SV_Position) : SV_Target\n"
	"{\n"
	"\tfloat2 texcoord = fragment.xy * metrics.xy;\n"
	"\tfloat2 pixcoord;\n"
	"\tfloat4 offset[3];\n"
	"\tSMAABlendingWeightCalculationVS(texcoord, pixcoord, offset);\n"
	"\treturn SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset, edges_texture, area_texture,\n"
	"\t\tsearch_texture, float4(0.0, 0.0, 0.0, 0.0));\n"
	"}\n",
	/* blend */
	"Texture2D color_texture : register(t0);\n"
	"Texture2D weights_texture : register(t1);\n"
	"float4 main(float4 fragment : SV_Position) : SV_Target\n"
	"{\n"
	"\tfloat2 texcoord = fragment.xy * metrics.xy;\n"
	"\tfloat4 offset;\n"
	"\tSMAANeighborhoodBlendingVS(texcoord, offset);\n"
	"\treturn SMAANeighborhoodBlendingPS(texcoord, offset, color_texture, weights_texture);\n"
	"}\n",
};

static struct
{
	ID3DBlob *vertex_shader;
	ID3DBlob *pixel_shaders[NUMBER_OF_PASS_PROGRAMS];
	/* FXAA's, and SMAA's, programs or textures could not be made */
	BOOL failed[2];
	/* the size of the textures below */
	unsigned long width, height;
	/* the target's pixels before the pass, and SMAA's edges and blending
	weights */
	unsigned int color, edges, weights;
	/* SMAA's lookup textures */
	unsigned int area, search;
} passes;

/* the texts given joined, malloc'd (NULL ends them) */
static char *text_join(const char *first, ...)
{
	va_list arguments;
	const char *part;
	size_t length = 0;
	char *text;

	va_start(arguments, first);
	for (part = first; part; part = va_arg(arguments, const char *))
		length += strlen(part);
	va_end(arguments);
	if (!(text = malloc(length + 1)))
		return NULL;
	text[0] = 0;
	va_start(arguments, first);
	for (part = first; part; part = va_arg(arguments, const char *))
		strcat(text, part);
	va_end(arguments);
	return text;
}

/* a pass's pixel shader; smaa_source for SMAA's */
static ID3DBlob *program_build(int which, const char *smaa_source)
{
	static const char *const names[NUMBER_OF_PASS_PROGRAMS] =
	{
		"blit", "clear", "mipmap", "FXAA", "SMAA edge detection", "SMAA blending weight", "SMAA neighborhood blending"
	};
	char *text;
	ID3DBlob *code;

	switch (which)
	{
	case _pass_blit: text = text_join(pass_header, blit_source, NULL); break;
	case _pass_clear: text = text_join(pass_header, clear_source, NULL); break;
	case _pass_mipmap: text = text_join(pass_header, mipmap_source, NULL); break;
	case _pass_fxaa: text = text_join(pass_header, fxaa_source, NULL); break;
	default:
		if (!smaa_source || !smaa_source[0])
		{
			platform_log("anti-aliasing: the build has no SMAA");
			return NULL;
		}
		text = text_join(pass_header, smaa_header, smaa_source, "\n", smaa_mains[which - _pass_smaa_edges], NULL);
		break;
	}
	if (!text)
		return NULL;
	code = d3d12_compile(text, "ps_5_0", names[which]);
	free(text);
	return code;
}

/* ---------- pipeline states, by program, format, samples and channels */

struct pass_pipeline
{
	struct pass_pipeline *next;
	int program;
	DXGI_FORMAT format;
	int samples;
	UINT write_mask;
	ID3D12PipelineState *state;
};

static struct pass_pipeline *pass_pipelines;

static ID3D12PipelineState *pass_pipeline_get(int program, DXGI_FORMAT format, int samples, UINT write_mask)
{
	struct pass_pipeline *entry;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC description;
	ID3DBlob *pixel_shader = passes.pixel_shaders[program];

	for (entry = pass_pipelines; entry; entry = entry->next)
	{
		if (entry->program == program && entry->format == format && entry->samples == samples &&
			entry->write_mask == write_mask)
		{
			return entry->state;
		}
	}
	if (!passes.vertex_shader || !pixel_shader || !(entry = calloc(1, sizeof(*entry))))
		return NULL;
	entry->program = program;
	entry->format = format;
	entry->samples = samples;
	entry->write_mask = write_mask;
	memset(&description, 0, sizeof(description));
	description.pRootSignature = d3d12.pass_root_signature;
	description.VS.pShaderBytecode = ID3D10Blob_GetBufferPointer(passes.vertex_shader);
	description.VS.BytecodeLength = ID3D10Blob_GetBufferSize(passes.vertex_shader);
	description.PS.pShaderBytecode = ID3D10Blob_GetBufferPointer(pixel_shader);
	description.PS.BytecodeLength = ID3D10Blob_GetBufferSize(pixel_shader);
	description.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	description.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	description.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	description.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	description.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	description.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	description.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	description.BlendState.RenderTarget[0].RenderTargetWriteMask = (UINT8)write_mask;
	description.SampleMask = UINT_MAX;
	description.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	description.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	description.RasterizerState.DepthClipEnable = TRUE;
	description.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	description.NumRenderTargets = 1;
	description.RTVFormats[0] = format;
	description.SampleDesc.Count = (UINT)samples;
	if (FAILED(ID3D12Device_CreateGraphicsPipelineState(d3d12.device, &description, &IID_ID3D12PipelineState,
		(void **)&entry->state)))
	{
		platform_log("Direct3D 12: cannot make pass %d's pipeline state", program);
		entry->state = NULL;
	}
	entry->next = pass_pipelines;
	pass_pipelines = entry;
	return entry->state;
}

/* the command list set up for a pass into rtv over viewport, its textures
(-1: none) and constants; FALSE if its pipeline cannot be made */
static BOOL pass_begin(int program, DXGI_FORMAT format, int samples, UINT write_mask, long rtv,
	const D3D12_VIEWPORT *viewport, const long srv[3], const float constants[16])
{
	ID3D12GraphicsCommandList *list = d3d12.list;
	ID3D12PipelineState *pipeline = pass_pipeline_get(program, format, samples, write_mask);
	D3D12_CPU_DESCRIPTOR_HANDLE target;
	D3D12_RECT scissor;
	UINT index;

	if (!pipeline)
		return FALSE;
	d3d12_draw_state_invalidate();
	ID3D12GraphicsCommandList_SetGraphicsRootSignature(list, d3d12.pass_root_signature);
	ID3D12GraphicsCommandList_SetPipelineState(list, pipeline);
	ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants(list, 0, 16, constants, 0);
	for (index = 0; index < 3; index++)
	{
		ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(list, 1 + index,
			d3d12_descriptor_gpu(_d3d12_heap_srv, srv && srv[index] >= 0 ? srv[index] : d3d12_null_srv[_d3d12r_sampler_2d]));
	}
	target = d3d12_descriptor_cpu(_d3d12_heap_rtv, rtv);
	ID3D12GraphicsCommandList_OMSetRenderTargets(list, 1, &target, FALSE, NULL);
	ID3D12GraphicsCommandList_RSSetViewports(list, 1, viewport);
	scissor.left = (LONG)viewport->TopLeftX;
	scissor.top = (LONG)viewport->TopLeftY;
	scissor.right = (LONG)(viewport->TopLeftX + viewport->Width);
	scissor.bottom = (LONG)(viewport->TopLeftY + viewport->Height);
	ID3D12GraphicsCommandList_RSSetScissorRects(list, 1, &scissor);
	ID3D12GraphicsCommandList_IASetPrimitiveTopology(list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	return TRUE;
}

static void pass_draw(void)
{
	ID3D12GraphicsCommandList_DrawInstanced(d3d12.list, 3, 1, 0, 0);
}

static void viewport_of(D3D12_VIEWPORT *viewport, long x0, long y0, long x1, long y1)
{
	viewport->TopLeftX = (float)x0;
	viewport->TopLeftY = (float)y0;
	viewport->Width = (float)(x1 - x0);
	viewport->Height = (float)(y1 - y0);
	viewport->MinDepth = 0.0f;
	viewport->MaxDepth = 1.0f;
}

/* ---------- the window */

void d3d12_pass_blit(struct d3d12_texture *source, ID3D12Resource *destination, long destination_rtv,
	UINT destination_width, UINT destination_height)
{
	static const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	D3D12_RESOURCE_BARRIER barrier;
	D3D12_VIEWPORT viewport;
	float constants[16];
	long srv[3] = { -1, -1, -1 };
	long width, height, x, y;

	memset(&barrier, 0, sizeof(barrier));
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = destination;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	ID3D12GraphicsCommandList_ResourceBarrier(d3d12.list, 1, &barrier);
	ID3D12GraphicsCommandList_ClearRenderTargetView(d3d12.list, d3d12_descriptor_cpu(_d3d12_heap_rtv, destination_rtv),
		black, 0, NULL);
	/* letterboxed to the back buffer's aspect ratio, as gl_present */
	width = (long)destination_width;
	height = (long)((long long)destination_width * (long long)source->height / (long long)source->width);
	if (height > (long)destination_height)
	{
		height = (long)destination_height;
		width = (long)((long long)destination_height * (long long)source->width / (long long)source->height);
	}
	x = ((long)destination_width - width) / 2;
	y = ((long)destination_height - height) / 2;
	viewport_of(&viewport, x, y, x + width, y + height);
	memset(constants, 0, sizeof(constants));
	constants[0] = (float)x;
	constants[1] = (float)y;
	constants[2] = 1.0f / (float)width;
	constants[3] = 1.0f / (float)height;
	srv[0] = source->srv;
	d3d12_texture_transition(source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	if (width > 0 && height > 0 &&
		pass_begin(_pass_blit, DXGI_FORMAT_B8G8R8A8_UNORM, 1, D3D12_COLOR_WRITE_ENABLE_ALL, destination_rtv, &viewport,
			srv, constants))
	{
		pass_draw();
	}
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	ID3D12GraphicsCommandList_ResourceBarrier(d3d12.list, 1, &barrier);
}

/* ---------- clears of some channels */

void d3d12_pass_clear_channels(struct d3d12_texture *target, UINT channel_mask, const float color[4],
	const D3D12_RECT *rectangles, UINT count)
{
	D3D12_VIEWPORT viewport;
	float constants[16];
	UINT index;

	viewport_of(&viewport, 0, 0, (long)target->width, (long)target->height);
	memset(constants, 0, sizeof(constants));
	memcpy(constants + 8, color, 4 * sizeof(float));
	d3d12_texture_transition(target, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (!pass_begin(_pass_clear, DXGI_FORMAT_B8G8R8A8_UNORM, target->samples, channel_mask, target->rtv, &viewport,
		NULL, constants))
	{
		return;
	}
	for (index = 0; index < count; index++)
	{
		ID3D12GraphicsCommandList_RSSetScissorRects(d3d12.list, 1, &rectangles[index]);
		pass_draw();
	}
}

/* ---------- mip levels */

/* the views of each level of a texture, made the first time */
static BOOL level_views(struct d3d12_texture *texture)
{
	unsigned long level;

	if (texture->level_srv)
		return TRUE;
	texture->level_srv = malloc(texture->levels * sizeof(long));
	texture->level_rtv = malloc(texture->levels * sizeof(long));
	if (!texture->level_srv || !texture->level_rtv)
	{
		free(texture->level_srv);
		free(texture->level_rtv);
		texture->level_srv = texture->level_rtv = NULL;
		return FALSE;
	}
	for (level = 0; level < texture->levels; level++)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srv;
		D3D12_RENDER_TARGET_VIEW_DESC rtv;

		texture->level_srv[level] = d3d12_descriptor_allocate(_d3d12_heap_srv);
		texture->level_rtv[level] = d3d12_descriptor_allocate(_d3d12_heap_rtv);
		memset(&srv, 0, sizeof(srv));
		srv.Format = texture->view_format;
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Texture2D.MostDetailedMip = (UINT)level;
		srv.Texture2D.MipLevels = 1;
		if (texture->level_srv[level] >= 0)
		{
			ID3D12Device_CreateShaderResourceView(d3d12.device, texture->resource, &srv,
				d3d12_descriptor_cpu(_d3d12_heap_srv, texture->level_srv[level]));
		}
		memset(&rtv, 0, sizeof(rtv));
		rtv.Format = texture->format;
		rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtv.Texture2D.MipSlice = (UINT)level;
		if (texture->level_rtv[level] >= 0)
		{
			ID3D12Device_CreateRenderTargetView(d3d12.device, texture->resource, &rtv,
				d3d12_descriptor_cpu(_d3d12_heap_rtv, texture->level_rtv[level]));
		}
	}
	return TRUE;
}

static void subresource_transition(struct d3d12_texture *texture, UINT subresource, D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier;

	memset(&barrier, 0, sizeof(barrier));
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = texture->resource;
	barrier.Transition.Subresource = subresource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	ID3D12GraphicsCommandList_ResourceBarrier(d3d12.list, 1, &barrier);
}

void d3d12_pass_mipmaps(struct d3d12_texture *texture, unsigned long first_level)
{
	unsigned long level;

	if (!texture->render_target || texture->type != _d3d12r_texture_2d || first_level >= texture->levels ||
		!first_level || !level_views(texture))
	{
		return;
	}
	/* each level a render target while it is made, and read by the next */
	d3d12_texture_transition(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
	for (level = first_level; level < texture->levels; level++)
	{
		unsigned long width = texture->width >> level ? texture->width >> level : 1;
		unsigned long height = texture->height >> level ? texture->height >> level : 1;
		D3D12_VIEWPORT viewport;
		float constants[16];
		long srv[3] = { -1, -1, -1 };

		subresource_transition(texture, (UINT)(level - 1), D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		viewport_of(&viewport, 0, 0, (long)width, (long)height);
		memset(constants, 0, sizeof(constants));
		constants[0] = 1.0f / (float)width;
		constants[1] = 1.0f / (float)height;
		srv[0] = texture->level_srv[level - 1];
		if (pass_begin(_pass_mipmap, texture->format, 1, D3D12_COLOR_WRITE_ENABLE_ALL, texture->level_rtv[level],
			&viewport, srv, constants))
		{
			pass_draw();
		}
		subresource_transition(texture, (UINT)(level - 1), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
}

/* ---------- anti-aliasing */

/* a lookup texture of SMAA's from its texels; 0 if it cannot be made */
static unsigned int lookup_texture(const void *texels, DXGI_FORMAT format, unsigned long width, unsigned long height,
	unsigned long texel_size)
{
	unsigned int handle = 0;

	if (texels &&
		(handle = d3d12_texture_create(_d3d12r_texture_2d, format, format, width, height, 1, 1, 1, FALSE, FALSE)) != 0)
	{
		d3d12_texture_upload(d3d12_texture_get(handle), 0, 0, height, width * texel_size, texels);
	}
	return handle;
}

int d3d12r_anti_aliasing_prepare(int smaa, const char *smaa_source, const void *area, const void *search)
{
	int first = smaa ? _pass_smaa_edges : _pass_fxaa;
	int last = smaa ? _pass_smaa_blend : _pass_fxaa;
	int which;

	if (passes.failed[smaa != FALSE])
		return FALSE;
	for (which = first; which <= last; which++)
	{
		if (!passes.pixel_shaders[which] && !(passes.pixel_shaders[which] = program_build(which, smaa_source)))
		{
			passes.failed[smaa != FALSE] = TRUE;
			return FALSE;
		}
	}
	if (smaa && !passes.area)
	{
		passes.area = lookup_texture(area, DXGI_FORMAT_R8G8_UNORM, 160, 560, 2);
		passes.search = lookup_texture(search, DXGI_FORMAT_R8_UNORM, 64, 16, 1);
		if (!passes.area || !passes.search)
		{
			platform_log("anti-aliasing: cannot make SMAA's lookup textures");
			passes.failed[1] = TRUE;
			return FALSE;
		}
	}
	return TRUE;
}

/* the passes' textures, of the target's size (SMAA's only for SMAA); color
is the target's format, so that the copy is exact */
static BOOL textures_fit(BOOL smaa, unsigned long width, unsigned long height)
{
	if (passes.width != width || passes.height != height)
	{
		d3d12_texture_destroy(passes.color);
		d3d12_texture_destroy(passes.edges);
		d3d12_texture_destroy(passes.weights);
		passes.color = passes.edges = passes.weights = 0;
		passes.width = width;
		passes.height = height;
		passes.color = d3d12_texture_create(_d3d12r_texture_2d, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM,
			width, height, 1, 1, 1, FALSE, FALSE);
	}
	if (smaa && !passes.edges)
	{
		passes.edges = d3d12_texture_create(_d3d12r_texture_2d, DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_R8G8_UNORM,
			width, height, 1, 1, 1, TRUE, FALSE);
		passes.weights = d3d12_texture_create(_d3d12r_texture_2d, DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, 1, TRUE, FALSE);
	}
	return passes.color && (!smaa || (passes.edges && passes.weights));
}

/* an intermediate emptied, all of it, so that no edge of another window's
pass is found: SMAA's passes write only where there is an edge */
static void intermediate_clear(struct d3d12_texture *texture)
{
	static const float nothing[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	d3d12_texture_transition(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
	ID3D12GraphicsCommandList_ClearRenderTargetView(d3d12.list, d3d12_descriptor_cpu(_d3d12_heap_rtv, texture->rtv),
		nothing, 0, NULL);
}

void d3d12r_anti_alias(int smaa, unsigned int target_handle, const long corners[4])
{
	struct d3d12_texture *target = d3d12_texture_get(target_handle);
	struct d3d12_texture *color, *edges, *weights;
	D3D12_TEXTURE_COPY_LOCATION to, from;
	D3D12_VIEWPORT viewport;
	D3D12_BOX box;
	float constants[16];
	long srv[3] = { -1, -1, -1 };

	/* (its programs made as the setting was chosen) */
	if (!target || target->samples > 1 || corners[2] <= corners[0] || corners[3] <= corners[1] ||
		passes.failed[smaa != 0] || !passes.pixel_shaders[smaa ? _pass_smaa_blend : _pass_fxaa] ||
		(smaa && !passes.search) || !textures_fit(smaa, target->width, target->height))
	{
		return;
	}
	color = d3d12_texture_get(passes.color);
	edges = d3d12_texture_get(passes.edges);
	weights = d3d12_texture_get(passes.weights);

	/* the target's pixels, read while the target is drawn into */
	d3d12_texture_transition(target, D3D12_RESOURCE_STATE_COPY_SOURCE);
	d3d12_texture_transition(color, D3D12_RESOURCE_STATE_COPY_DEST);
	memset(&to, 0, sizeof(to));
	to.pResource = color->resource;
	to.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	memset(&from, 0, sizeof(from));
	from.pResource = target->resource;
	from.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	box.left = (UINT)(corners[0] > 0 ? corners[0] : 0);
	box.top = (UINT)(corners[1] > 0 ? corners[1] : 0);
	box.right = (UINT)(corners[2] < (long)target->width ? corners[2] : (long)target->width);
	box.bottom = (UINT)(corners[3] < (long)target->height ? corners[3] : (long)target->height);
	box.front = 0;
	box.back = 1;
	if (box.right <= box.left || box.bottom <= box.top)
		return;
	ID3D12GraphicsCommandList_CopyTextureRegion(d3d12.list, &to, box.left, box.top, 0, &from, &box);
	d3d12_texture_transition(color, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	viewport_of(&viewport, corners[0], corners[1], corners[2], corners[3]);
	memset(constants, 0, sizeof(constants));
	constants[0] = 1.0f / (float)target->width;
	constants[1] = 1.0f / (float)target->height;
	constants[2] = (float)target->width;
	constants[3] = (float)target->height;
	if (!smaa)
	{
		/* (split screen: no texel of the next window is read) */
		constants[4] = ((float)corners[0] + 0.5f) / (float)target->width;
		constants[5] = ((float)corners[1] + 0.5f) / (float)target->height;
		constants[6] = ((float)corners[2] - 0.5f) / (float)target->width;
		constants[7] = ((float)corners[3] - 0.5f) / (float)target->height;
		srv[0] = color->srv;
		d3d12_texture_transition(target, D3D12_RESOURCE_STATE_RENDER_TARGET);
		/* only color is written back: the game keeps values of its own in
		destination alpha */
		if (pass_begin(_pass_fxaa, DXGI_FORMAT_B8G8R8A8_UNORM, 1,
			D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE, target->rtv,
			&viewport, srv, constants))
		{
			pass_draw();
		}
		return;
	}
	intermediate_clear(edges);
	srv[0] = color->srv;
	if (pass_begin(_pass_smaa_edges, DXGI_FORMAT_R8G8_UNORM, 1, D3D12_COLOR_WRITE_ENABLE_ALL, edges->rtv, &viewport,
		srv, constants))
	{
		pass_draw();
	}
	intermediate_clear(weights);
	d3d12_texture_transition(edges, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	d3d12_texture_transition(d3d12_texture_get(passes.area), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	d3d12_texture_transition(d3d12_texture_get(passes.search), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	srv[0] = edges->srv;
	srv[1] = d3d12_texture_get(passes.area)->srv;
	srv[2] = d3d12_texture_get(passes.search)->srv;
	if (pass_begin(_pass_smaa_weights, DXGI_FORMAT_R8G8B8A8_UNORM, 1, D3D12_COLOR_WRITE_ENABLE_ALL, weights->rtv,
		&viewport, srv, constants))
	{
		pass_draw();
	}
	d3d12_texture_transition(weights, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	d3d12_texture_transition(target, D3D12_RESOURCE_STATE_RENDER_TARGET);
	srv[0] = color->srv;
	srv[1] = weights->srv;
	srv[2] = -1;
	if (pass_begin(_pass_smaa_blend, DXGI_FORMAT_B8G8R8A8_UNORM, 1,
		D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE, target->rtv,
		&viewport, srv, constants))
	{
		pass_draw();
	}
}

/* ---------- initialization */

BOOL d3d12_passes_initialize(void)
{
	int which;

	if (!(passes.vertex_shader = d3d12_compile(vertex_source, "vs_5_0", "pass vertex")))
		return FALSE;
	/* (FXAA's and SMAA's as the setting is chosen) */
	for (which = _pass_blit; which <= _pass_mipmap; which++)
	{
		if (!(passes.pixel_shaders[which] = program_build(which, NULL)))
			return FALSE;
	}
	return TRUE;
}
