/*
D3D8_METAL_DEVICE.C

The macOS app's Direct3D 8 device: what port/linux/src/d3d8_gl.c does with
OpenGL, written as the host's ring commands (port/macos/include/opence_render.h)
that the app's renderer draws with Metal. Its resources, the ring and the
render targets are d3d8_metal.c's; the shaders are nv2a_vsh.c's and
nv2a_psh.c's Metal Shading Language.

The ring has no state of its own to change a piece at a time: each draw
names its whole pipeline (its shaders, blending and vertex layout), its
depth-stencil and sampler objects, made once and found again by their
words, and sends its raster state and uniforms again. Metal draws no fans,
quads, polygons or line loops: they are drawn with indices. Render targets
are drawn at the game's own size; the window shows the back buffer
letterboxed (Present).

The state setters that only store the state, the vertex shader objects and
their declarations are d3d8_gl.c's, unchanged.
*/

#include "xgpu.h"
#include "opence_render.h"
#include "opence_render_writer.h"
#include "sdl_platform.h"
#include "halo_ui_pointer.h"
#include "port_config.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void d3d8_surface_initialize(D3DSurface *surface, D3DFORMAT format, unsigned long width, unsigned long height);
void d3d8_surface_resize(D3DSurface *surface, D3DFORMAT format, unsigned long width, unsigned long height);

DWORD D3D__RenderState[D3DRS_MAX];
DWORD D3D__TextureState[D3DTSS_MAXSTAGES][D3DTSS_MAX];
WORD *D3D__IndexData;
BYTE D3D__StateBlockDirty[1024];

/* ---------- vertex shaders (d3d8_gl.c's objects) */

#define VERTEX_SHADER_SIGNATURE 0x76736864UL /* 'vshd' */
#define VERTEX_PROGRAM_SLOTS 136

struct vertex_element
{
	unsigned char reg;
	unsigned char stream;
	unsigned char type;
	unsigned char bytes;
	unsigned short offset;
};

/* a program's Metal library for one set of attributes */
struct vertex_library
{
	struct vertex_library *next;
	unsigned long packed_mask, present_mask;
	BOOL lit;
	unsigned int library;
};

struct vertex_shader_object
{
	unsigned long signature;
	unsigned long id;
	DWORD *instructions;
	unsigned long instruction_count;
	struct vertex_element elements[XGPU_VERTEX_ATTRIBUTE_COUNT];
	unsigned long element_count;
	unsigned long packed_mask;
	struct nv2a_vertex_lighting lighting;
	BOOL lighting_failed;
	struct vertex_library *libraries;
};

/* ---------- the device */

#define STREAM_COUNT 16
#define IMMEDIATE_VERTEX_LIMIT 4096
#define SCREEN_HEIGHT 480
#define SCREEN_MAXIMUM_WIDTH 1920
/* what a visibility test reports: every test passes */
#define VISIBILITY_ALL_SAMPLES 1000000

static struct
{
	BOOL created;
	BOOL ready;
	D3DPRESENT_PARAMETERS presentation;
	D3DSurface back_buffer;
	D3DSurface depth_buffer;
	D3DSurface *render_target;
	D3DSurface *depth_stencil;
	D3DVIEWPORT8 viewport;
	D3DMATRIX transforms[D3DTS_MAX];
	D3DSHADERCONSTANTMODE shader_constant_mode;
	float constants[XGPU_VERTEX_CONSTANT_COUNT][4];
	float viewport_scale[4];
	float viewport_offset[4];
	float attributes[XGPU_VERTEX_ATTRIBUTE_COUNT][4];
	struct
	{
		DWORD data;
		UINT stride;
	} streams[STREAM_COUNT];
	UINT base_vertex_index;
	D3DBaseTexture *textures[D3DTSS_MAXSTAGES];
	D3DPalette *palettes[D3DTSS_MAXSTAGES];
	struct vertex_shader_object *vertex_shader;
	struct vertex_shader_object *program_slots[VERTEX_PROGRAM_SLOTS];
	unsigned long program_address;
	unsigned long next_vertex_shader_id;
	/* Begin and End */
	BOOL immediate_active;
	D3DPRIMITIVETYPE immediate_type;
	unsigned long immediate_written;
	unsigned long immediate_count;
	unsigned long immediate_capacity;
	float (*immediate_vertices)[XGPU_VERTEX_ATTRIBUTE_COUNT][4];
	/* the pass the draws go to: its targets' ring textures (depth 0 is
	none) */
	BOOL in_pass;
	unsigned int pass_color, pass_depth;
	unsigned long pass_width, pass_height;
	/* display.per_pixel_lighting, read when the settings change */
	unsigned long config_serial;
	BOOL per_pixel_lighting;
	/* a 1x1 texture for stages the shader samples without one */
	unsigned int blank_texture;
	unsigned long frame;
} device;

static float dword_to_float(DWORD value)
{
	union { DWORD d; float f; } u;

	u.d = value;
	return u.f;
}

static void color_to_vec4(D3DCOLOR color, float *out)
{
	out[0] = ((color >> 16) & 0xff) / 255.0f;
	out[1] = ((color >> 8) & 0xff) / 255.0f;
	out[2] = (color & 0xff) / 255.0f;
	out[3] = ((color >> 24) & 0xff) / 255.0f;
}

static void constants_store(unsigned long first, const void *data, unsigned long count)
{
	memcpy(device.constants[first], data, count * sizeof(device.constants[0]));
}

static void surface_dimensions(const D3DSurface *surface, unsigned long *width, unsigned long *height, BOOL *depth)
{
	struct xgpu_texture_description description;
	DWORD format;

	xgpu_texture_describe(surface->Format, surface->Size, &description);
	*width = description.width;
	*height = description.height;
	format = description.format;
	*depth = format == D3DFMT_D24S8 || format == D3DFMT_F24S8 || format == D3DFMT_D16 || format == D3DFMT_F16 ||
		format == D3DFMT_LIN_D24S8 || format == D3DFMT_LIN_F24S8 || format == D3DFMT_LIN_D16 || format == D3DFMT_LIN_F16;
}

static void viewport_update_constants(void)
{
	/* Direct3D's reserved constants c[-38] and c[-37] map clip space to
	the screen; zscale is the depth buffer's range */
	float zscale = 16777215.0f;

	if (device.depth_stencil)
	{
		struct xgpu_texture_description description;

		xgpu_texture_describe(device.depth_stencil->Format, device.depth_stencil->Size, &description);
		if (description.format == D3DFMT_D16 || description.format == D3DFMT_LIN_D16 ||
			description.format == D3DFMT_F16 || description.format == D3DFMT_LIN_F16)
		{
			zscale = 65535.0f;
		}
	}
	device.viewport_scale[0] = device.viewport.Width * 0.5f;
	device.viewport_scale[1] = -(float)device.viewport.Height * 0.5f;
	device.viewport_scale[2] = zscale * (device.viewport.MaxZ - device.viewport.MinZ);
	device.viewport_scale[3] = 0.0f;
	device.viewport_offset[0] = device.viewport.X + device.viewport.Width * 0.5f;
	device.viewport_offset[1] = device.viewport.Y + device.viewport.Height * 0.5f;
	device.viewport_offset[2] = zscale * device.viewport.MinZ;
	device.viewport_offset[3] = 0.0f;
	if (!(device.shader_constant_mode & D3DSCM_NORESERVEDCONSTANTS))
	{
		memcpy(device.constants[XGPU_VERTEX_CONSTANT_BIAS - 38], device.viewport_scale, sizeof(device.viewport_scale));
		memcpy(device.constants[XGPU_VERTEX_CONSTANT_BIAS - 37], device.viewport_offset, sizeof(device.viewport_offset));
	}
}

/* ---------- state to the ring's values (tools/harness/tests/metal_state.c) */

/* (0 is D3DCMP_NEVER's low bits, as d3d8_gl.c reads it) */
static unsigned int ring_compare(DWORD function)
{
	if (!function)
		return OPENCE_RENDER_COMPARE_NEVER;
	return function >= D3DCMP_NEVER && function <= D3DCMP_ALWAYS ?
		(unsigned int)(function - D3DCMP_NEVER) + OPENCE_RENDER_COMPARE_NEVER : OPENCE_RENDER_COMPARE_ALWAYS;
}

static unsigned int ring_blend_factor(DWORD factor)
{
	switch (factor)
	{
	case D3DBLEND_ZERO: return OPENCE_RENDER_BLEND_ZERO;
	case D3DBLEND_SRCCOLOR: return OPENCE_RENDER_BLEND_SOURCE_COLOR;
	case D3DBLEND_INVSRCCOLOR: return OPENCE_RENDER_BLEND_ONE_MINUS_SOURCE_COLOR;
	case D3DBLEND_SRCALPHA: return OPENCE_RENDER_BLEND_SOURCE_ALPHA;
	case D3DBLEND_INVSRCALPHA: return OPENCE_RENDER_BLEND_ONE_MINUS_SOURCE_ALPHA;
	case D3DBLEND_DESTALPHA: return OPENCE_RENDER_BLEND_DESTINATION_ALPHA;
	case D3DBLEND_INVDESTALPHA: return OPENCE_RENDER_BLEND_ONE_MINUS_DESTINATION_ALPHA;
	case D3DBLEND_DESTCOLOR: return OPENCE_RENDER_BLEND_DESTINATION_COLOR;
	case D3DBLEND_INVDESTCOLOR: return OPENCE_RENDER_BLEND_ONE_MINUS_DESTINATION_COLOR;
	case D3DBLEND_SRCALPHASAT: return OPENCE_RENDER_BLEND_SOURCE_ALPHA_SATURATED;
	case D3DBLEND_CONSTANTCOLOR: return OPENCE_RENDER_BLEND_BLEND_COLOR;
	case D3DBLEND_INVCONSTANTCOLOR: return OPENCE_RENDER_BLEND_ONE_MINUS_BLEND_COLOR;
	case D3DBLEND_CONSTANTALPHA: return OPENCE_RENDER_BLEND_BLEND_ALPHA;
	case D3DBLEND_INVCONSTANTALPHA: return OPENCE_RENDER_BLEND_ONE_MINUS_BLEND_ALPHA;
	default: return OPENCE_RENDER_BLEND_ONE;
	}
}

/* (the signed operations of the NV2A have no Metal equal: drawn as their
unsigned ones) */
static unsigned int ring_blend_operation(DWORD operation)
{
	switch (operation)
	{
	case D3DBLENDOP_SUBTRACT: return OPENCE_RENDER_BLEND_SUBTRACT;
	case D3DBLENDOP_REVSUBTRACT: case D3DBLENDOP_REVSUBTRACTSIGNED: return OPENCE_RENDER_BLEND_REVERSE_SUBTRACT;
	case D3DBLENDOP_MIN: return OPENCE_RENDER_BLEND_MIN;
	case D3DBLENDOP_MAX: return OPENCE_RENDER_BLEND_MAX;
	default: return OPENCE_RENDER_BLEND_ADD;
	}
}

static unsigned int ring_stencil_operation(DWORD operation)
{
	switch (operation)
	{
	case D3DSTENCILOP_ZERO: return OPENCE_RENDER_STENCIL_ZERO;
	case D3DSTENCILOP_REPLACE: return OPENCE_RENDER_STENCIL_REPLACE;
	case D3DSTENCILOP_INCRSAT: return OPENCE_RENDER_STENCIL_INCREMENT_CLAMP;
	case D3DSTENCILOP_DECRSAT: return OPENCE_RENDER_STENCIL_DECREMENT_CLAMP;
	case D3DSTENCILOP_INVERT: return OPENCE_RENDER_STENCIL_INVERT;
	case D3DSTENCILOP_INCR: return OPENCE_RENDER_STENCIL_INCREMENT_WRAP;
	case D3DSTENCILOP_DECR: return OPENCE_RENDER_STENCIL_DECREMENT_WRAP;
	default: return OPENCE_RENDER_STENCIL_KEEP;
	}
}

static unsigned int ring_address(DWORD address)
{
	switch (address)
	{
	case D3DTADDRESS_MIRROR: return OPENCE_RENDER_ADDRESS_MIRROR;
	case D3DTADDRESS_CLAMP: case D3DTADDRESS_CLAMPTOEDGE: return OPENCE_RENDER_ADDRESS_CLAMP_TO_EDGE;
	case D3DTADDRESS_BORDER: return OPENCE_RENDER_ADDRESS_CLAMP_TO_BORDER;
	default: return OPENCE_RENDER_ADDRESS_REPEAT;
	}
}

/* a filter (NONE only for the mip filter: no mipmaps read) */
static unsigned int ring_filter(DWORD filter, BOOL mip)
{
	if (filter == D3DTEXF_NONE)
		return mip ? OPENCE_RENDER_FILTER_NONE : OPENCE_RENDER_FILTER_NEAREST;
	return filter == D3DTEXF_POINT ? OPENCE_RENDER_FILTER_NEAREST : OPENCE_RENDER_FILTER_LINEAR;
}

/* Metal has three border colours: the nearest of them */
static unsigned int ring_border(D3DCOLOR color)
{
	if (!(color >> 24))
		return OPENCE_RENDER_BORDER_TRANSPARENT_BLACK;
	return (color & 0xffffff) == 0xffffff ? OPENCE_RENDER_BORDER_OPAQUE_WHITE : OPENCE_RENDER_BORDER_OPAQUE_BLACK;
}

/* faces are wound as the screen sees them (y down), as D3D winds them */
static void ring_cull(DWORD cull_mode, DWORD front_face, unsigned int *cull, unsigned int *winding)
{
	*winding = front_face == D3DFRONT_CCW ? OPENCE_RENDER_WINDING_COUNTER_CLOCKWISE : OPENCE_RENDER_WINDING_CLOCKWISE;
	if (cull_mode == D3DCULL_NONE)
		*cull = OPENCE_RENDER_CULL_NONE;
	else
		*cull = cull_mode == front_face ? OPENCE_RENDER_CULL_FRONT : OPENCE_RENDER_CULL_BACK;
}

/* a declaration's element type as a vertex format (0 for none), and whether
it is unpacked by the shader */
static unsigned int ring_vertex_format(unsigned long type, BOOL *packed)
{
	*packed = FALSE;
	switch (type)
	{
	case D3DVSDT_FLOAT1: return OPENCE_RENDER_VERTEX_FLOAT;
	case D3DVSDT_FLOAT2: return OPENCE_RENDER_VERTEX_FLOAT2;
	case D3DVSDT_FLOAT3: return OPENCE_RENDER_VERTEX_FLOAT3;
	case D3DVSDT_FLOAT4: return OPENCE_RENDER_VERTEX_FLOAT4;
	case D3DVSDT_FLOAT2H: return OPENCE_RENDER_VERTEX_FLOAT3;
	case D3DVSDT_D3DCOLOR: return OPENCE_RENDER_VERTEX_UCHAR4_NORMALIZED_BGRA;
	case D3DVSDT_SHORT1: return OPENCE_RENDER_VERTEX_SHORT;
	case D3DVSDT_SHORT2: return OPENCE_RENDER_VERTEX_SHORT2;
	case D3DVSDT_SHORT3: return OPENCE_RENDER_VERTEX_SHORT3;
	case D3DVSDT_SHORT4: return OPENCE_RENDER_VERTEX_SHORT4;
	case D3DVSDT_NORMSHORT1: return OPENCE_RENDER_VERTEX_SHORT_NORMALIZED;
	case D3DVSDT_NORMSHORT2: return OPENCE_RENDER_VERTEX_SHORT2_NORMALIZED;
	case D3DVSDT_NORMSHORT3: return OPENCE_RENDER_VERTEX_SHORT3_NORMALIZED;
	case D3DVSDT_NORMSHORT4: return OPENCE_RENDER_VERTEX_SHORT4_NORMALIZED;
	case D3DVSDT_PBYTE1: return OPENCE_RENDER_VERTEX_UCHAR_NORMALIZED;
	case D3DVSDT_PBYTE2: return OPENCE_RENDER_VERTEX_UCHAR2_NORMALIZED;
	case D3DVSDT_PBYTE3: return OPENCE_RENDER_VERTEX_UCHAR3_NORMALIZED;
	case D3DVSDT_PBYTE4: return OPENCE_RENDER_VERTEX_UCHAR4_NORMALIZED;
	case D3DVSDT_NORMPACKED3:
		*packed = TRUE;
		return OPENCE_RENDER_VERTEX_UINT;
	case D3DVSDT_NONE: return 0;
	default: return OPENCE_RENDER_VERTEX_FLOAT4;
	}
}

/* a primitive as one Metal draws, and whether its vertices go through
indices (primitive_indices) */
static unsigned int ring_primitive(D3DPRIMITIVETYPE type, BOOL *indexed)
{
	*indexed = FALSE;
	switch (type)
	{
	case D3DPT_POINTLIST: return OPENCE_RENDER_PRIMITIVE_POINT;
	case D3DPT_LINELIST: return OPENCE_RENDER_PRIMITIVE_LINE;
	case D3DPT_LINESTRIP: return OPENCE_RENDER_PRIMITIVE_LINE_STRIP;
	case D3DPT_LINELOOP:
		*indexed = TRUE;
		return OPENCE_RENDER_PRIMITIVE_LINE_STRIP;
	case D3DPT_TRIANGLESTRIP: case D3DPT_QUADSTRIP: return OPENCE_RENDER_PRIMITIVE_TRIANGLE_STRIP;
	case D3DPT_TRIANGLEFAN: case D3DPT_POLYGON: case D3DPT_QUADLIST:
		*indexed = TRUE;
		return OPENCE_RENDER_PRIMITIVE_TRIANGLE;
	default: return OPENCE_RENDER_PRIMITIVE_TRIANGLE;
	}
}

/* the indices drawing count vertices of type (as the indices given, or
0 to count - 1 for none) as ring_primitive's primitive; out has room for
count * 3 + 3 of them. Returns how many. */
static unsigned long primitive_indices(D3DPRIMITIVETYPE type, const WORD *in, unsigned long count, WORD *out)
{
	unsigned long index, written = 0;

#define VERTEX(n) (in ? in[n] : (WORD)(n))
	switch (type)
	{
	case D3DPT_LINELOOP:
		for (index = 0; index < count; index++)
			out[written++] = VERTEX(index);
		if (count)
			out[written++] = VERTEX(0);
		break;
	case D3DPT_TRIANGLEFAN: case D3DPT_POLYGON:
		for (index = 1; index + 1 < count; index++)
		{
			out[written++] = VERTEX(0);
			out[written++] = VERTEX(index);
			out[written++] = VERTEX(index + 1);
		}
		break;
	case D3DPT_QUADLIST:
		for (index = 0; index + 3 < count; index += 4)
		{
			out[written++] = VERTEX(index);
			out[written++] = VERTEX(index + 1);
			out[written++] = VERTEX(index + 2);
			out[written++] = VERTEX(index);
			out[written++] = VERTEX(index + 2);
			out[written++] = VERTEX(index + 3);
		}
		break;
	default:
		for (index = 0; index < count; index++)
			out[written++] = VERTEX(index);
		break;
	}
#undef VERTEX
	return written;
}

/* ---------- render and texture states (d3d8_gl.c's) */

void D3DFASTCALL D3DDevice_SetRenderState_Simple(DWORD method, DWORD value)
{
	/* callers also store the value in D3D__RenderState themselves */
	(void)method;
	(void)value;
}

void D3DFASTCALL D3DDevice_SetRenderState_Deferred(D3DRENDERSTATETYPE state, DWORD value)
{
	if ((unsigned long)state < D3DRS_MAX)
		D3D__RenderState[state] = value;
}

void WINAPI D3DDevice_SetRenderState_ZBias(DWORD value);

void WINAPI D3DDevice_SetRenderStateNotInline(D3DRENDERSTATETYPE state, DWORD value)
{
	if (state == D3DRS_ZBIAS)
		D3DDevice_SetRenderState_ZBias(value);
	else if ((unsigned long)state < D3DRS_MAX)
		D3D__RenderState[state] = value;
}

/* As the Xbox's D3D8 does it: a z bias is a polygon offset of -bias depth
units plus -bias/4 times the polygon's depth slope, enabled for every fill
mode. Without the slope term, decals (biased by 8) fight with the surface
under them wherever it is seen at an angle. */
void WINAPI D3DDevice_SetRenderState_ZBias(DWORD value)
{
	float offset = -(float)value;
	float slope = offset * 0.25f;
	DWORD enable = value != 0;

	memcpy(&D3D__RenderState[D3DRS_POLYGONOFFSETZSLOPESCALE], &slope, sizeof(slope));
	memcpy(&D3D__RenderState[D3DRS_POLYGONOFFSETZOFFSET], &offset, sizeof(offset));
	D3D__RenderState[D3DRS_POINTOFFSETENABLE] = enable;
	D3D__RenderState[D3DRS_WIREFRAMEOFFSETENABLE] = enable;
	D3D__RenderState[D3DRS_SOLIDOFFSETENABLE] = enable;
	D3D__RenderState[D3DRS_ZBIAS] = value;
}

#define COMPLEX_RENDER_STATE(name, state) \
	void WINAPI D3DDevice_SetRenderState_##name(DWORD value) { D3D__RenderState[state] = value; }

COMPLEX_RENDER_STATE(PSTextureModes, D3DRS_PSTEXTUREMODES)
COMPLEX_RENDER_STATE(VertexBlend, D3DRS_VERTEXBLEND)
COMPLEX_RENDER_STATE(FogColor, D3DRS_FOGCOLOR)
COMPLEX_RENDER_STATE(FillMode, D3DRS_FILLMODE)
COMPLEX_RENDER_STATE(BackFillMode, D3DRS_BACKFILLMODE)
COMPLEX_RENDER_STATE(TwoSidedLighting, D3DRS_TWOSIDEDLIGHTING)
COMPLEX_RENDER_STATE(NormalizeNormals, D3DRS_NORMALIZENORMALS)
COMPLEX_RENDER_STATE(ZEnable, D3DRS_ZENABLE)
COMPLEX_RENDER_STATE(StencilEnable, D3DRS_STENCILENABLE)
COMPLEX_RENDER_STATE(StencilFail, D3DRS_STENCILFAIL)
COMPLEX_RENDER_STATE(FrontFace, D3DRS_FRONTFACE)
COMPLEX_RENDER_STATE(CullMode, D3DRS_CULLMODE)
COMPLEX_RENDER_STATE(TextureFactor, D3DRS_TEXTUREFACTOR)
COMPLEX_RENDER_STATE(LogicOp, D3DRS_LOGICOP)
COMPLEX_RENDER_STATE(EdgeAntiAlias, D3DRS_EDGEANTIALIAS)
COMPLEX_RENDER_STATE(MultiSampleAntiAlias, D3DRS_MULTISAMPLEANTIALIAS)
COMPLEX_RENDER_STATE(MultiSampleMask, D3DRS_MULTISAMPLEMASK)
COMPLEX_RENDER_STATE(MultiSampleType, D3DRS_MULTISAMPLETYPE)
COMPLEX_RENDER_STATE(ShadowFunc, D3DRS_SHADOWFUNC)
COMPLEX_RENDER_STATE(LineWidth, D3DRS_LINEWIDTH)
COMPLEX_RENDER_STATE(Dxt1NoiseEnable, D3DRS_DXT1NOISEENABLE)
COMPLEX_RENDER_STATE(YuvEnable, D3DRS_YUVENABLE)
COMPLEX_RENDER_STATE(OcclusionCullEnable, D3DRS_OCCLUSIONCULLENABLE)
COMPLEX_RENDER_STATE(StencilCullEnable, D3DRS_STENCILCULLENABLE)
COMPLEX_RENDER_STATE(RopZCmpAlwaysRead, D3DRS_ROPZCMPALWAYSREAD)
COMPLEX_RENDER_STATE(RopZRead, D3DRS_ROPZREAD)
COMPLEX_RENDER_STATE(DoNotCullUncompressed, D3DRS_DONOTCULLUNCOMPRESSED)

void D3DFASTCALL D3DDevice_SetTextureState_Deferred(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES && (unsigned long)type < D3DTSS_MAX)
		D3D__TextureState[stage][type] = value;
}

void WINAPI D3DDevice_SetTextureState_TexCoordIndex(DWORD stage, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES)
		D3D__TextureState[stage][D3DTSS_TEXCOORDINDEX] = value;
}

void WINAPI D3DDevice_SetTextureState_BorderColor(DWORD stage, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES)
		D3D__TextureState[stage][D3DTSS_BORDERCOLOR] = value;
}

void WINAPI D3DDevice_SetTextureState_ColorKeyColor(DWORD stage, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES)
		D3D__TextureState[stage][D3DTSS_COLORKEYCOLOR] = value;
}

void WINAPI D3DDevice_SetTextureState_BumpEnv(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES && (unsigned long)type < D3DTSS_MAX)
		D3D__TextureState[stage][type] = value;
}

void WINAPI D3DDevice_SetTexture(DWORD stage, D3DBaseTexture *texture)
{
	if (stage < D3DTSS_MAXSTAGES)
		device.textures[stage] = texture;
}

void WINAPI D3DDevice_SetPalette(DWORD stage, D3DPalette *palette)
{
	if (stage < D3DTSS_MAXSTAGES)
		device.palettes[stage] = palette;
}

void WINAPI D3DDevice_SetPixelShaderProgram(D3DPIXELSHADERDEF *definition)
{
	/* the definition's members are the pixel shader render states */
	if (!definition)
		return;
	memcpy(&D3D__RenderState[D3DRS_PSALPHAINPUTS0], definition->PSAlphaInputs, sizeof(definition->PSAlphaInputs));
	D3D__RenderState[D3DRS_PSFINALCOMBINERINPUTSABCD] = definition->PSFinalCombinerInputsABCD;
	D3D__RenderState[D3DRS_PSFINALCOMBINERINPUTSEFG] = definition->PSFinalCombinerInputsEFG;
	memcpy(&D3D__RenderState[D3DRS_PSCONSTANT0_0], definition->PSConstant0, sizeof(definition->PSConstant0));
	memcpy(&D3D__RenderState[D3DRS_PSCONSTANT1_0], definition->PSConstant1, sizeof(definition->PSConstant1));
	memcpy(&D3D__RenderState[D3DRS_PSALPHAOUTPUTS0], definition->PSAlphaOutputs, sizeof(definition->PSAlphaOutputs));
	memcpy(&D3D__RenderState[D3DRS_PSRGBINPUTS0], definition->PSRGBInputs, sizeof(definition->PSRGBInputs));
	D3D__RenderState[D3DRS_PSCOMPAREMODE] = definition->PSCompareMode;
	D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0] = definition->PSFinalCombinerConstant0;
	D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1] = definition->PSFinalCombinerConstant1;
	memcpy(&D3D__RenderState[D3DRS_PSRGBOUTPUTS0], definition->PSRGBOutputs, sizeof(definition->PSRGBOutputs));
	D3D__RenderState[D3DRS_PSCOMBINERCOUNT] = definition->PSCombinerCount;
	D3D__RenderState[D3DRS_PSTEXTUREMODES] = definition->PSTextureModes;
	D3D__RenderState[D3DRS_PSDOTMAPPING] = definition->PSDotMapping;
	D3D__RenderState[D3DRS_PSINPUTTEXTURE] = definition->PSInputTexture;
}

/* ---------- vertex shaders */

static unsigned long vertex_type_bytes(unsigned long type)
{
	switch (type)
	{
	case D3DVSDT_FLOAT1: return 4;
	case D3DVSDT_FLOAT2: return 8;
	case D3DVSDT_FLOAT3: return 12;
	case D3DVSDT_FLOAT4: return 16;
	case D3DVSDT_D3DCOLOR: return 4;
	case D3DVSDT_SHORT1: return 2;
	case D3DVSDT_SHORT2: return 4;
	case D3DVSDT_SHORT3: return 6;
	case D3DVSDT_SHORT4: return 8;
	case D3DVSDT_NORMSHORT1: return 2;
	case D3DVSDT_NORMSHORT2: return 4;
	case D3DVSDT_NORMSHORT3: return 6;
	case D3DVSDT_NORMSHORT4: return 8;
	case D3DVSDT_NORMPACKED3: return 4;
	case D3DVSDT_PBYTE1: return 1;
	case D3DVSDT_PBYTE2: return 2;
	case D3DVSDT_PBYTE3: return 3;
	case D3DVSDT_PBYTE4: return 4;
	case D3DVSDT_FLOAT2H: return 12;
	default: return 0;
	}
}

static void parse_declaration(struct vertex_shader_object *object, const DWORD *declaration)
{
	unsigned long stream = 0;
	unsigned long offsets[16] = { 0 };

	for (; declaration && *declaration != D3DVSD_END(); declaration++)
	{
		DWORD token = *declaration;
		unsigned long token_type = (token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT;

		switch (token_type)
		{
		case D3DVSD_TOKEN_STREAM:
			stream = token & D3DVSD_STREAMNUMBERMASK;
			break;
		case D3DVSD_TOKEN_STREAMDATA:
			if (token & D3DVSD_DATALOADTYPEMASK)
			{
				/* skip: the count is in dwords, or in bytes with bit 27 */
				unsigned long count = (token & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT;

				offsets[stream] += (token & 0x08000000) ? count : count * 4;
			}
			else if (object->element_count < XGPU_VERTEX_ATTRIBUTE_COUNT)
			{
				struct vertex_element *element = &object->elements[object->element_count++];

				element->reg = (unsigned char)(token & D3DVSD_VERTEXREGMASK);
				element->stream = (unsigned char)stream;
				element->type = (unsigned char)((token & D3DVSD_DATATYPEMASK) >> D3DVSD_DATATYPESHIFT);
				element->bytes = (unsigned char)vertex_type_bytes(element->type);
				element->offset = (unsigned short)offsets[stream];
				offsets[stream] += element->bytes;
				if (element->type == D3DVSDT_NORMPACKED3)
					object->packed_mask |= 1UL << element->reg;
			}
			break;
		case D3DVSD_TOKEN_CONSTMEM:
			declaration += ((token & D3DVSD_CONSTCOUNTMASK) >> D3DVSD_CONSTCOUNTSHIFT) * 4;
			break;
		case D3DVSD_TOKEN_EXT:
			declaration += (token & D3DVSD_EXTCOUNTMASK) >> D3DVSD_EXTCOUNTSHIFT;
			break;
		default:
			break;
		}
	}
}


HRESULT WINAPI D3DDevice_CreateVertexShader(CONST DWORD *declaration, CONST DWORD *function, DWORD *handle, DWORD usage)
{
	struct vertex_shader_object *object = calloc(1, sizeof(*object));

	(void)usage;
	if (!object)
		return E_OUTOFMEMORY;
	object->signature = VERTEX_SHADER_SIGNATURE;
	object->id = device.next_vertex_shader_id++;
	if (function)
	{
		/* header: program type in the low word, instruction count in the high */
		object->instruction_count = function[0] >> 16;
		object->instructions = malloc(object->instruction_count * 4 * sizeof(DWORD));
		memcpy(object->instructions, function + 1, object->instruction_count * 4 * sizeof(DWORD));
	}
	parse_declaration(object, declaration);
	/* odd values are FVF codes; programmable shader handles are even */
	*handle = (DWORD)object;
	return S_OK;
}

static struct vertex_shader_object *vertex_shader_from_handle(DWORD handle)
{
	struct vertex_shader_object *object = (struct vertex_shader_object *)handle;

	if (!handle || (handle & 1) || object->signature != VERTEX_SHADER_SIGNATURE)
		return NULL;
	return object;
}

/* the game names its model lighting programs as it creates them
(rasterizer_xbox_vertex_shaders_initialize.c), so that their draws can be lit
for each pixel (display.per_pixel_lighting); one whose lighting is not as
the pixel shader computes it stays lit for each vertex */
void halo_vertex_shader_lighting(unsigned long handle)
{
	struct vertex_shader_object *object = vertex_shader_from_handle((DWORD)handle);

	if (!object || !object->instructions)
		return;
	if (!nv2a_vertex_shader_lighting(object->instructions, object->instruction_count, &object->lighting))
	{
		memset(&object->lighting, 0, sizeof(object->lighting));
		platform_log("GPU: vertex shader %lu is not lit as the pixel shader would light it: lit for each vertex",
			object->id);
	}
}

void WINAPI D3DDevice_DeleteVertexShader(DWORD handle)
{
	/* programs stay cached; the object is small */
	(void)handle;
}

void WINAPI D3DDevice_SetVertexShader(DWORD handle)
{
	struct vertex_shader_object *object = vertex_shader_from_handle(handle);

	if (object)
	{
		device.vertex_shader = object;
		device.program_address = 0;
		device.program_slots[0] = object;
	}
}

void WINAPI D3DDevice_LoadVertexShader(DWORD handle, DWORD address)
{
	if (address < VERTEX_PROGRAM_SLOTS)
		device.program_slots[address] = vertex_shader_from_handle(handle);
}

void WINAPI D3DDevice_SelectVertexShader(DWORD handle, DWORD address)
{
	struct vertex_shader_object *object = vertex_shader_from_handle(handle);

	if (object)
		device.vertex_shader = object;
	if (address < VERTEX_PROGRAM_SLOTS)
		device.program_address = address;
}

void WINAPI D3DDevice_GetVertexShaderSize(DWORD handle, UINT *size)
{
	struct vertex_shader_object *object = vertex_shader_from_handle(handle);

	*size = object ? object->instruction_count : 0;
}

void WINAPI D3DDevice_SetVertexShaderConstant(INT reg, CONST void *constant_data, DWORD constant_count)
{
	long first = reg + XGPU_VERTEX_CONSTANT_BIAS;

	if (first < 0 || first >= XGPU_VERTEX_CONSTANT_COUNT)
		return;
	if (first + (long)constant_count > XGPU_VERTEX_CONSTANT_COUNT)
		constant_count = XGPU_VERTEX_CONSTANT_COUNT - first;
	constants_store((unsigned long)first, constant_data, constant_count);
}

/* the program that runs: the one loaded at the selected address, else the
current shader's own */
static struct vertex_shader_object *current_program(void)
{
	struct vertex_shader_object *program = device.program_slots[device.program_address];

	return program ? program : device.vertex_shader;
}


/* ---------- the screen (d3d8_gl.c's, at a scale of 1)

The screen is 480 lines and as wide as the window's shape makes them, from
640 to 1920 columns. Its targets are drawn at that size: Present scales the
back buffer to the window. */

static long screen_width;
static long ui_offset;

static long screen_mode_choose(void)
{
	/* display.screen_width, or 0 for the window's shape, which the app
	passes (as Android's does) */
	const char *display = getenv("HALO_DISPLAY_WIDTH");
	long width = config_integer("display.screen_width");

	if (width <= 0)
		width = display ? atol(display) : 640;
	width = width < 640 ? 640 : width > SCREEN_MAXIMUM_WIDTH ? SCREEN_MAXIMUM_WIDTH : width;
	return width & ~1L;
}

long halo_screen_width(void)
{
	if (!screen_width)
	{
		screen_width = screen_mode_choose();
		platform_log("screen: %ldx%d drawn at %ldx%d", screen_width, SCREEN_HEIGHT, screen_width, SCREEN_HEIGHT);
	}
	return screen_width;
}

float halo_screen_pixel_scale(void)
{
	return 1.0f;
}

float halo_screen_scale(void)
{
	return 1.0f;
}

/* (display.shadow_resolution needs targets drawn larger than the game's
size: the maps are drawn at the Xbox's 128) */
long halo_shadow_map_scale(void)
{
	return 1;
}

void halo_screen_ui_offset(unsigned char centered)
{
	ui_offset = centered ? (halo_screen_width() - 640) / 2 : 0;
}

long halo_screen_commit(void)
{
	long width;

	if (!screen_width)
		return halo_screen_width();
	width = screen_mode_choose();
	if (width != screen_width)
	{
		platform_log("screen: %ldx%d drawn at %ldx%d", width, SCREEN_HEIGHT, width, SCREEN_HEIGHT);
		screen_width = width;
		if (device.created)
		{
			device.presentation.BackBufferWidth = (UINT)width;
			d3d8_surface_resize(&device.back_buffer, D3DFMT_LIN_A8R8G8B8, (unsigned long)width, SCREEN_HEIGHT);
			d3d8_surface_resize(&device.depth_buffer, D3DFMT_LIN_D24S8, (unsigned long)width, SCREEN_HEIGHT);
		}
	}
	return screen_width;
}

/* d3d8_gl.c's anti-aliasing of a rectangle of the screen (xgpu_post.c) is
not drawn here yet */
void halo_screen_anti_alias(short x0, short y0, short x1, short y1)
{
	(void)x0;
	(void)y0;
	(void)x1;
	(void)y1;
}

/* ---------- vertical blanks (d3d8_gl.c's) */

#define VERTICAL_BLANK_NANOSECONDS (1000000000L / 60)

static pthread_mutex_t vertical_blank_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t vertical_blank_condition = PTHREAD_COND_INITIALIZER;
static D3DCALLBACK vertical_blank_callback;
static unsigned long vertical_blank_count;
static volatile unsigned int flip_count;
static unsigned long pending_flips;
static BOOL vertical_blank_thread_started = FALSE;

/* (the guest has nanosleep, not clock_nanosleep: the time left until the
next blank is slept) */
static void sleep_until(const struct timespec *when)
{
	struct timespec now, left;

	clock_gettime(CLOCK_MONOTONIC, &now);
	left.tv_sec = when->tv_sec - now.tv_sec;
	left.tv_nsec = when->tv_nsec - now.tv_nsec;
	if (left.tv_nsec < 0)
	{
		left.tv_nsec += 1000000000L;
		left.tv_sec--;
	}
	if (left.tv_sec >= 0 && (left.tv_sec || left.tv_nsec))
		nanosleep(&left, NULL);
}

static void *vertical_blank_thread(void *unused)
{
	struct timespec next;

	(void)unused;
	clock_gettime(CLOCK_MONOTONIC, &next);
	for (;;)
	{
		D3DCALLBACK callback;

		next.tv_nsec += VERTICAL_BLANK_NANOSECONDS;
		if (next.tv_nsec >= 1000000000L)
		{
			next.tv_nsec -= 1000000000L;
			next.tv_sec++;
		}
		sleep_until(&next);

		pthread_mutex_lock(&vertical_blank_lock);
		vertical_blank_count++;
		if (pending_flips)
		{
			pending_flips--;
			flip_count++;
		}
		callback = vertical_blank_callback;
		pthread_cond_broadcast(&vertical_blank_condition);
		pthread_mutex_unlock(&vertical_blank_lock);

		if (callback)
			callback(0);
	}
	return NULL;
}

static void vertical_blank_start(void)
{
	pthread_mutex_lock(&vertical_blank_lock);
	if (!vertical_blank_thread_started)
	{
		pthread_t thread;

		if (pthread_create(&thread, NULL, vertical_blank_thread, NULL) == 0)
		{
			pthread_detach(thread);
			vertical_blank_thread_started = TRUE;
		}
		else
		{
			platform_log("cannot start the vertical blank thread");
		}
	}
	pthread_mutex_unlock(&vertical_blank_lock);
}

volatile unsigned int *d3d_find_flipcount(void)
{
	return &flip_count;
}

void WINAPI D3DDevice_SetVerticalBlankCallback(D3DCALLBACK callback)
{
	pthread_mutex_lock(&vertical_blank_lock);
	vertical_blank_callback = callback;
	pthread_mutex_unlock(&vertical_blank_lock);
	vertical_blank_start();
}

void WINAPI D3DDevice_BlockUntilVerticalBlank(void)
{
	unsigned long count;

	vertical_blank_start();
	pthread_mutex_lock(&vertical_blank_lock);
	count = vertical_blank_count;
	while (vertical_blank_count == count)
		pthread_cond_wait(&vertical_blank_condition, &vertical_blank_lock);
	pthread_mutex_unlock(&vertical_blank_lock);
}

/* ---------- objects found again by their words */

#define CACHE_BUCKETS 1024

enum
{
	_cache_pipeline,
	_cache_depth_stencil,
	_cache_sampler,
	_cache_fragment_library,
	NUMBER_OF_CACHES
};

struct cache_entry
{
	struct cache_entry *next;
	unsigned long size;
	unsigned int id;
	unsigned char key[1];
};

static struct cache_entry *caches[NUMBER_OF_CACHES][CACHE_BUCKETS];

static unsigned long cache_hash(const void *key, unsigned long size)
{
	const unsigned char *bytes = key;
	unsigned long hash = 2166136261UL, index;

	for (index = 0; index < size; index++)
		hash = (hash ^ bytes[index]) * 16777619UL;
	return hash;
}

/* the id made for these bytes, or 0 */
static unsigned int cache_find(int cache, const void *key, unsigned long size)
{
	struct cache_entry *entry;

	for (entry = caches[cache][cache_hash(key, size) % CACHE_BUCKETS]; entry; entry = entry->next)
		if (entry->size == size && !memcmp(entry->key, key, size))
			return entry->id;
	return 0;
}

static void cache_insert(int cache, const void *key, unsigned long size, unsigned int id)
{
	struct cache_entry *entry = malloc(sizeof(*entry) + size);
	struct cache_entry **bucket = &caches[cache][cache_hash(key, size) % CACHE_BUCKETS];

	if (!entry)
		return;
	entry->size = size;
	entry->id = id;
	memcpy(entry->key, key, size);
	entry->next = *bucket;
	*bucket = entry;
}

/* the object a create command's fields make (fields[0] its id, given here),
found again by the other fields; 0 if none can be made */
static unsigned int object_get(int cache, unsigned int kind, unsigned int op, unsigned int *fields, unsigned int words)
{
	unsigned long size = (words - 2) * sizeof(*fields);
	unsigned int id = cache_find(cache, fields + 1, size);
	unsigned int *out;

	if (id)
		return id;
	id = xgpu_metal_new_id(kind);
	if (!id || !(out = xgpu_metal_command(op, words)))
		return 0;
	fields[0] = id;
	memcpy(out, fields, (words - 1) * sizeof(*fields));
	cache_insert(cache, fields + 1, size, id);
	return id;
}

/* a library of a malloc'd source (freed here); 0 if it cannot be made */
static unsigned int library_create(char *source)
{
	unsigned long length = source ? strlen(source) : 0;
	const void *staged = length && length <= 0x100000 ? xgpu_metal_stage(source, length) : NULL;
	unsigned int id = staged ? xgpu_metal_new_id(OPENCE_RENDER_OBJECT_LIBRARY) : 0;
	unsigned int *fields;

	free(source);
	if (!id || !(fields = xgpu_metal_command(OPENCE_RENDER_OP_CREATE_LIBRARY, 4)))
		return 0;
	fields[0] = id;
	fields[1] = (unsigned int)(unsigned long)staged;
	fields[2] = (unsigned int)length;
	return id;
}

/* ---------- the pass

The state a pass was last sent: a command the same as the last is not sent
again. A new pass has none of it. */

#define SENT_WORDS 8

enum
{
	_sent_pipeline,
	_sent_depth_stencil,
	_sent_raster,
	_sent_viewport,
	_sent_scissor,
	_sent_blend_color,
	_sent_texture0,
	_sent_sampler0 = _sent_texture0 + D3DTSS_MAXSTAGES,
	NUMBER_OF_SENT = _sent_sampler0 + D3DTSS_MAXSTAGES
};

static struct
{
	BOOL valid;
	unsigned int fields[SENT_WORDS];
} sent[NUMBER_OF_SENT];

static void command_send(int which, unsigned int op, unsigned int words, const unsigned int *fields)
{
	unsigned long size = (words - 1) * sizeof(*fields);
	unsigned int *out;

	if (which >= 0 && sent[which].valid && !memcmp(sent[which].fields, fields, size))
		return;
	if (!(out = xgpu_metal_command(op, words)))
		return;
	memcpy(out, fields, size);
	if (which >= 0 && words - 1 <= SENT_WORDS)
	{
		sent[which].valid = TRUE;
		memcpy(sent[which].fields, fields, size);
	}
}

static void bytes_send(unsigned int stage, unsigned int slot, const void *data, unsigned long length)
{
	unsigned int *fields = xgpu_metal_command(OPENCE_RENDER_OP_SET_BYTES, 4 + (unsigned int)(length / 4));

	if (!fields)
		return;
	fields[0] = stage;
	fields[1] = slot;
	fields[2] = (unsigned int)length;
	memcpy(fields + 3, data, length);
}

static void pass_end(void)
{
	if (device.in_pass)
		xgpu_metal_command(OPENCE_RENDER_OP_END_PASS, 1);
	device.in_pass = FALSE;
}

void xgpu_metal_pass_end(void)
{
	pass_end();
}

/* begins a pass on the current targets (the window's is 0), keeping what
they hold */
static BOOL pass_begin(unsigned int color, unsigned int depth, unsigned long width, unsigned long height,
	unsigned int color_load)
{
	unsigned int *fields;

	pass_end();
	if (!(fields = xgpu_metal_command(OPENCE_RENDER_OP_BEGIN_PASS, 14)))
		return FALSE;
	memset(fields, 0, 13 * sizeof(*fields));
	fields[0] = color;
	fields[1] = depth;
	fields[2] = color_load;
	fields[3] = depth ? OPENCE_RENDER_LOAD_LOAD : OPENCE_RENDER_LOAD_DONT_CARE;
	fields[4] = fields[3];
	fields[8] = opence_render_float(1.0f);
	fields[9] = opence_render_float(1.0f);
	memset(sent, 0, sizeof(sent));
	device.in_pass = TRUE;
	device.pass_color = color;
	device.pass_depth = depth;
	device.pass_width = width;
	device.pass_height = height;
	return TRUE;
}

/* the pass of the device's targets, begun if it is not the open one; FALSE
for a draw with nowhere to go. A depth target of another size than the
colour one is left out (Metal's attachments are one size). */
static BOOL pass_ensure(void)
{
	struct xgpu_render_target *color, *depth = NULL;

	if (!device.ready || !device.render_target)
		return FALSE;
	color = xgpu_metal_render_target_get(device.render_target);
	if (!color || color->depth)
		return FALSE;
	if (device.depth_stencil)
	{
		depth = xgpu_metal_render_target_get(device.depth_stencil);
		if (depth && (!depth->depth || depth->width != color->width || depth->height != color->height))
			depth = NULL;
	}
	color->written++;
	if (depth)
		depth->written++;
	if (device.in_pass && device.pass_color == color->texture && device.pass_depth == (depth ? depth->texture : 0))
		return TRUE;
	return pass_begin(color->texture, depth ? depth->texture : 0, color->width, color->height,
		OPENCE_RENDER_LOAD_LOAD);
}

/* ---------- textures and samplers (d3d8_gl.c's bind_textures) */

static unsigned int sampler_get(DWORD stage, BOOL mipmapped, BOOL hires)
{
	const DWORD *state = D3D__TextureState[stage];
	unsigned int fields[11];
	DWORD anisotropy = state[D3DTSS_MAXANISOTROPY];
	BOOL anisotropic = state[D3DTSS_MINFILTER] == D3DTEXF_ANISOTROPIC || state[D3DTSS_MAGFILTER] == D3DTEXF_ANISOTROPIC;

	fields[1] = hires ? OPENCE_RENDER_FILTER_LINEAR : ring_filter(state[D3DTSS_MINFILTER], FALSE);
	fields[2] = hires ? OPENCE_RENDER_FILTER_LINEAR : ring_filter(state[D3DTSS_MAGFILTER], FALSE);
	fields[3] = hires ? OPENCE_RENDER_FILTER_LINEAR :
		mipmapped ? ring_filter(state[D3DTSS_MIPFILTER], TRUE) : OPENCE_RENDER_FILTER_NONE;
	fields[4] = ring_address(state[D3DTSS_ADDRESSU]);
	fields[5] = ring_address(state[D3DTSS_ADDRESSV]);
	fields[6] = ring_address(state[D3DTSS_ADDRESSW]);
	fields[7] = anisotropic && !hires ? (unsigned int)(anisotropy < 1 ? 1 : anisotropy > 16 ? 16 : anisotropy) : 1;
	fields[8] = opence_render_float(hires ? 0.0f : (float)state[D3DTSS_MAXMIPLEVEL]);
	fields[9] = opence_render_float(1000.0f);
	fields[10] = ring_border(state[D3DTSS_BORDERCOLOR]);
	return object_get(_cache_sampler, OPENCE_RENDER_OBJECT_SAMPLER, OPENCE_RENDER_OP_CREATE_SAMPLER, fields, 12);
}

/* each stage's texture and sampler, the key's sampler types and the
uniforms' texture scales; textures[stage] is 0 for a stage not sampled */
static void textures_prepare(struct nv2a_pixel_shader_key *key, struct nv2a_pixel_uniforms *uniforms,
	unsigned int *textures, unsigned int *samplers)
{
	DWORD stage;

	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		D3DBaseTexture *texture = device.textures[stage];
		DWORD mode = (D3D__RenderState[D3DRS_PSTEXTUREMODES] >> (5 * stage)) & 0x1f;
		struct xgpu_texture_description description;
		struct xgpu_render_target *target;
		BOOL mipmapped = FALSE, hires = FALSE;
		unsigned int id = 0;

		uniforms->texture_scale[stage][0] = 1.0f;
		uniforms->texture_scale[stage][1] = 1.0f;
		uniforms->texture_scale[stage][2] = 1.0f;
		uniforms->texture_scale[stage][3] = 1.0f;
		key->sampler_type[stage] = _xgpu_sampler_2d;
		if (!texture || !texture->Data || mode == 0 || mode == 4 || mode == 5 || mode == 0x11)
		{
			if (mode != 0x11)
				key->sampler_type[stage] = _xgpu_sampler_none;
		}
		else if ((target = xgpu_render_target_find(texture->Data)) != NULL)
		{
			xgpu_texture_describe(texture->Format, texture->Size, &description);
			if (description.linear)
			{
				uniforms->texture_scale[stage][0] = 1.0f / (float)target->width;
				uniforms->texture_scale[stage][1] = 1.0f / (float)target->height;
			}
			id = target->texture;
		}
		else
		{
			D3DPalette *palette = device.palettes[stage];
			const D3DCOLOR *colors = palette && palette->Data ? PLATFORM_PHYSICAL_TO_VIRTUAL(palette->Data) : NULL;
			GLenum type = GL_TEXTURE_2D;

			id = xgpu_texture_get((const DWORD *)texture, colors, &type, &description);
			if (id)
			{
				if (description.linear)
				{
					uniforms->texture_scale[stage][0] = 1.0f / (float)description.width;
					uniforms->texture_scale[stage][1] = 1.0f / (float)description.height;
				}
				mipmapped = description.levels > 1;
				hires = description.hires;
				if (stage == 0)
				{
					key->coverage_alpha = (unsigned char)description.hires_coverage;
					key->point_threshold = (unsigned char)description.hires_point_threshold;
				}
				key->sampler_type[stage] = type == GL_TEXTURE_CUBE_MAP ? _xgpu_sampler_cube :
					type == GL_TEXTURE_3D ? _xgpu_sampler_3d : _xgpu_sampler_2d;
			}
		}
		/* a stage sampled without a texture reads the blank one; so does one
		reading the target being drawn into, which Metal cannot */
		if (key->sampler_type[stage] != _xgpu_sampler_none && (!id || id == device.pass_color))
		{
			id = device.blank_texture;
			key->sampler_type[stage] = _xgpu_sampler_2d;
		}
		textures[stage] = key->sampler_type[stage] != _xgpu_sampler_none ? id : 0;
		samplers[stage] = textures[stage] ? sampler_get(stage, mipmapped, hires) : 0;
	}
}

/* ---------- the pixel shader's key and uniforms (d3d8_gl.c's) */

static BOOL per_pixel_lighting(void)
{
	if (device.config_serial != config_changes() + 1)
	{
		device.config_serial = config_changes() + 1;
		device.per_pixel_lighting = config_boolean("display.per_pixel_lighting");
	}
	return device.per_pixel_lighting;
}

static long model_light_register(unsigned long light)
{
	return XGPU_VERTEX_CONSTANT_BIAS + (light ? -80 + (long)light : -82);
}

static void pixel_key_build(struct nv2a_pixel_shader_key *key, const struct vertex_shader_object *program)
{
	DWORD stage;

	memset(key, 0, sizeof(*key));
	memcpy(key->combiner_state, D3D__RenderState, sizeof(key->combiner_state));
	memset(&key->combiner_state[D3DRS_PSCONSTANT0_0], 0, 16 * sizeof(DWORD));
	key->combiner_state[D3DRS_PSFINALCOMBINERCONSTANT0] = 0;
	key->combiner_state[D3DRS_PSFINALCOMBINERCONSTANT1] = 0;
	key->texture_modes = D3D__RenderState[D3DRS_PSTEXTUREMODES];
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		key->alpha_kill[stage] = D3D__TextureState[stage][D3DTSS_ALPHAKILL] == D3DTALPHAKILL_ENABLE;
		key->color_sign[stage] = (unsigned char)((D3D__TextureState[stage][D3DTSS_COLORSIGN] >> 28) & 0xf);
	}
	key->alpha_test_function = D3D__RenderState[D3DRS_ALPHATESTENABLE] ? D3D__RenderState[D3DRS_ALPHAFUNC] : 0;
	key->fog_enable = D3D__RenderState[D3DRS_FOGENABLE] != 0;
	key->fog_table_mode = (unsigned char)D3D__RenderState[D3DRS_FOGTABLEMODE];
	if (program && program->lighting.lights && !program->lighting_failed && per_pixel_lighting())
		key->per_pixel_lighting = (unsigned char)program->lighting.lights;
}

/* after textures_prepare, which sets coverage_alpha from the texture */
static void pixel_key_finish(struct nv2a_pixel_shader_key *key)
{
	key->coverage_alpha &= D3D__RenderState[D3DRS_ALPHABLENDENABLE] &&
		D3D__RenderState[D3DRS_SRCBLEND] == D3DBLEND_CONSTANTCOLOR && D3D__RenderState[D3DRS_DESTBLEND] == D3DBLEND_SRCALPHA;
	key->point_threshold &= key->coverage_alpha;
}

static void pixel_uniforms_build(struct nv2a_pixel_uniforms *uniforms)
{
	unsigned long index;

	for (index = 0; index < 8; index++)
	{
		color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT0_0 + index], uniforms->ps_c0[index]);
		color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT1_0 + index], uniforms->ps_c1[index]);
	}
	color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0], uniforms->ps_final_c0);
	color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1], uniforms->ps_final_c1);
	color_to_vec4(D3D__RenderState[D3DRS_FOGCOLOR], uniforms->fog_color);
	uniforms->fog_parameters[0] = dword_to_float(D3D__RenderState[D3DRS_FOGSTART]);
	uniforms->fog_parameters[1] = dword_to_float(D3D__RenderState[D3DRS_FOGEND]);
	uniforms->fog_parameters[2] = dword_to_float(D3D__RenderState[D3DRS_FOGDENSITY]);
	uniforms->fog_parameters[3] = 0.0f;
	for (index = 0; index < D3DTSS_MAXSTAGES; index++)
	{
		const DWORD *state = D3D__TextureState[index];

		uniforms->bump_matrix[index][0] = dword_to_float(state[D3DTSS_BUMPENVMAT00]);
		uniforms->bump_matrix[index][1] = dword_to_float(state[D3DTSS_BUMPENVMAT01]);
		uniforms->bump_matrix[index][2] = dword_to_float(state[D3DTSS_BUMPENVMAT10]);
		uniforms->bump_matrix[index][3] = dword_to_float(state[D3DTSS_BUMPENVMAT11]);
		uniforms->bump_luminance[index][0] = dword_to_float(state[D3DTSS_BUMPENVLSCALE]);
		uniforms->bump_luminance[index][1] = dword_to_float(state[D3DTSS_BUMPENVLOFFSET]);
		uniforms->texture_lod_bias[index] = dword_to_float(state[D3DTSS_MIPMAPLODBIAS]);
	}
	color_to_vec4(D3D__RenderState[D3DRS_BLENDCOLOR], uniforms->blend_constant);
	for (index = 0; index < XGPU_MODEL_LIGHT_COUNT; index++)
		memcpy(uniforms->model_lights[index], device.constants[model_light_register(index)],
			sizeof(uniforms->model_lights[index]));
	uniforms->alpha_reference = (float)(D3D__RenderState[D3DRS_ALPHAREF] & 0xff);
}

static void vertex_uniforms_build(struct nv2a_vertex_uniforms *uniforms)
{
	memcpy(uniforms->c, device.constants, sizeof(uniforms->c));
	memcpy(uniforms->viewport_scale, device.viewport_scale, sizeof(uniforms->viewport_scale));
	memcpy(uniforms->viewport_offset, device.viewport_offset, sizeof(uniforms->viewport_offset));
	memcpy(uniforms->attributes, device.attributes, sizeof(uniforms->attributes));
	uniforms->point_size = D3D__RenderState[D3DRS_POINTSIZE] ? dword_to_float(D3D__RenderState[D3DRS_POINTSIZE]) : 1.0f;
	uniforms->screen_offset = (float)ui_offset;
	uniforms->pad[0] = uniforms->pad[1] = 0.0f;
}

/* ---------- depth, stencil and raster state */

static void depth_stencil_send(void)
{
	unsigned int fields[16];
	unsigned int ref[2];
	BOOL depth = device.pass_depth != 0;
	BOOL stencil = depth && D3D__RenderState[D3DRS_STENCILENABLE];
	unsigned int face;

	memset(fields, 0, sizeof(fields));
	if (depth && D3D__RenderState[D3DRS_ZENABLE])
	{
		fields[1] = ring_compare(D3D__RenderState[D3DRS_ZFUNC]);
		fields[2] = D3D__RenderState[D3DRS_ZWRITEENABLE] != 0;
	}
	else
	{
		fields[1] = OPENCE_RENDER_COMPARE_ALWAYS;
	}
	fields[3] = stencil;
	for (face = 0; face < 2; face++)
	{
		unsigned int *side = fields + 4 + face * 6;

		side[0] = stencil ? ring_stencil_operation(D3D__RenderState[D3DRS_STENCILFAIL]) : OPENCE_RENDER_STENCIL_KEEP;
		side[1] = stencil ? ring_stencil_operation(D3D__RenderState[D3DRS_STENCILZFAIL]) : OPENCE_RENDER_STENCIL_KEEP;
		side[2] = stencil ? ring_stencil_operation(D3D__RenderState[D3DRS_STENCILPASS]) : OPENCE_RENDER_STENCIL_KEEP;
		side[3] = stencil ? ring_compare(D3D__RenderState[D3DRS_STENCILFUNC]) : OPENCE_RENDER_COMPARE_ALWAYS;
		side[4] = stencil ? D3D__RenderState[D3DRS_STENCILMASK] & 0xff : 0xff;
		side[5] = stencil ? D3D__RenderState[D3DRS_STENCILWRITEMASK] & 0xff : 0xff;
	}
	ref[0] = object_get(_cache_depth_stencil, OPENCE_RENDER_OBJECT_DEPTH_STENCIL, OPENCE_RENDER_OP_CREATE_DEPTH_STENCIL,
		fields, 17);
	ref[1] = stencil ? D3D__RenderState[D3DRS_STENCILREF] & 0xff : 0;
	if (ref[0])
		command_send(_sent_depth_stencil, OPENCE_RENDER_OP_SET_DEPTH_STENCIL, 3, ref);
}

static void raster_send(void)
{
	unsigned int fields[7];
	BOOL offset = D3D__RenderState[D3DRS_SOLIDOFFSETENABLE] != 0;

	ring_cull(D3D__RenderState[D3DRS_CULLMODE], D3D__RenderState[D3DRS_FRONTFACE], &fields[0], &fields[1]);
	fields[2] = D3D__RenderState[D3DRS_FILLMODE] == D3DFILL_WIREFRAME ? OPENCE_RENDER_FILL_LINES : OPENCE_RENDER_FILL_SOLID;
	fields[3] = opence_render_float(offset ? dword_to_float(D3D__RenderState[D3DRS_POLYGONOFFSETZOFFSET]) : 0.0f);
	fields[4] = opence_render_float(offset ? dword_to_float(D3D__RenderState[D3DRS_POLYGONOFFSETZSLOPESCALE]) : 0.0f);
	fields[5] = opence_render_float(0.0f);
	fields[6] = 1;
	command_send(_sent_raster, OPENCE_RENDER_OP_SET_RASTER, 8, fields);
}

/* the viewport, x0 to x1 and y0 to y1, and a scissor of it inside the
pass's target */
static void viewport_send(float x0, float y0, float width, float height, float near_z, float far_z)
{
	unsigned int fields[6];
	long left = (long)x0, top = (long)y0, right = (long)(x0 + width), bottom = (long)(y0 + height);

	fields[0] = opence_render_float(x0);
	fields[1] = opence_render_float(y0);
	fields[2] = opence_render_float(width);
	fields[3] = opence_render_float(height);
	fields[4] = opence_render_float(near_z);
	fields[5] = opence_render_float(far_z);
	command_send(_sent_viewport, OPENCE_RENDER_OP_SET_VIEWPORT, 7, fields);
	left = left < 0 ? 0 : left;
	top = top < 0 ? 0 : top;
	right = right > (long)device.pass_width ? (long)device.pass_width : right;
	bottom = bottom > (long)device.pass_height ? (long)device.pass_height : bottom;
	fields[0] = (unsigned int)left;
	fields[1] = (unsigned int)top;
	fields[2] = right > left ? (unsigned int)(right - left) : 0;
	fields[3] = bottom > top ? (unsigned int)(bottom - top) : 0;
	command_send(_sent_scissor, OPENCE_RENDER_OP_SET_SCISSOR, 5, fields);
}

static unsigned int write_mask(DWORD enable)
{
	return ((enable & D3DCOLORWRITEENABLE_RED) ? OPENCE_RENDER_WRITE_RED : 0) |
		((enable & D3DCOLORWRITEENABLE_GREEN) ? OPENCE_RENDER_WRITE_GREEN : 0) |
		((enable & D3DCOLORWRITEENABLE_BLUE) ? OPENCE_RENDER_WRITE_BLUE : 0) |
		((enable & D3DCOLORWRITEENABLE_ALPHA) ? OPENCE_RENDER_WRITE_ALPHA : 0);
}

/* ---------- vertex streams

What a draw reads: its attributes' formats and places, each stream's
buffer, and the attributes it has (present) and the packed normals among
them. A stream is read where the contiguous window's mirror holds it when
its vertices are 4-byte aligned, and is copied to the stream buffer
otherwise (repacked to 4-byte elements when they are not). */

#define PIPELINE_WORDS (17 + 4 * XGPU_VERTEX_ATTRIBUTE_COUNT + 4 * STREAM_COUNT)

struct draw_streams
{
	unsigned long present_mask, packed_mask;
	unsigned int attribute_count, layout_count;
	unsigned int attributes[XGPU_VERTEX_ATTRIBUTE_COUNT][4];
	unsigned int layouts[STREAM_COUNT][4];
	unsigned int buffers[STREAM_COUNT][3];
};

static unsigned char *repack_buffer;
static unsigned long repack_capacity;

static unsigned char *repack_space(unsigned long size)
{
	if (size > repack_capacity)
	{
		unsigned char *grown = realloc(repack_buffer, size);

		if (!grown)
			return NULL;
		repack_buffer = grown;
		repack_capacity = size;
	}
	return repack_buffer;
}

/* the declaration's streams for vertices first to first + count - 1 */
static BOOL streams_setup(struct draw_streams *streams, const struct vertex_shader_object *declaration,
	unsigned long first, unsigned long count)
{
	unsigned long stream;

	memset(streams, 0, sizeof(*streams));
	if (!declaration || !count)
		return FALSE;
	for (stream = 0; stream < STREAM_COUNT; stream++)
	{
		unsigned long stride = device.streams[stream].stride, index, aligned = TRUE, packed_stride = 0;
		unsigned long base, bytes, vertices = stride ? count : 1;
		unsigned int buffer, *layout;
		unsigned long offset;
		BOOL used = FALSE;

		if (!device.streams[stream].data)
			continue;
		for (index = 0; index < declaration->element_count; index++)
		{
			const struct vertex_element *element = &declaration->elements[index];

			if (element->stream != stream || element->type == D3DVSDT_NONE)
				continue;
			used = TRUE;
			aligned = aligned && !(element->offset & 3);
			packed_stride += (element->bytes + 3) & ~3UL;
		}
		if (!used)
			continue;
		base = (unsigned long)PLATFORM_PHYSICAL_TO_VIRTUAL(device.streams[stream].data) + first * stride;
		aligned = aligned && stride && !(stride & 3) && !(base & 3);
		bytes = aligned ? count * stride : vertices * packed_stride;
		if (aligned && xgpu_metal_mirror_range(base, bytes, &buffer, &offset, NULL))
		{
		}
		else if (aligned)
		{
			if (!xgpu_metal_stream((const void *)base, bytes, 4, &buffer, &offset))
				return FALSE;
		}
		else
		{
			unsigned char *out = repack_space(bytes);
			unsigned long vertex;

			if (!out)
				return FALSE;
			for (vertex = 0; vertex < vertices; vertex++)
			{
				const unsigned char *in = (const unsigned char *)base + vertex * stride;
				unsigned long place = vertex * packed_stride;

				for (index = 0; index < declaration->element_count; index++)
				{
					const struct vertex_element *element = &declaration->elements[index];

					if (element->stream != stream || element->type == D3DVSDT_NONE)
						continue;
					memcpy(out + place, in + element->offset, element->bytes);
					memset(out + place + element->bytes, 0, ((element->bytes + 3) & ~3UL) - element->bytes);
					place += (element->bytes + 3) & ~3UL;
				}
			}
			if (!xgpu_metal_stream(out, bytes, 4, &buffer, &offset))
				return FALSE;
		}
		/* the attributes, at their places in the stream as it is read */
		packed_stride = 0;
		for (index = 0; index < declaration->element_count; index++)
		{
			const struct vertex_element *element = &declaration->elements[index];
			unsigned int *attribute;
			BOOL packed;

			if (element->stream != stream || element->type == D3DVSDT_NONE ||
				streams->attribute_count >= XGPU_VERTEX_ATTRIBUTE_COUNT)
				continue;
			attribute = streams->attributes[streams->attribute_count++];
			attribute[0] = element->reg;
			attribute[1] = ring_vertex_format(element->type, &packed);
			attribute[2] = aligned ? element->offset : (unsigned int)packed_stride;
			attribute[3] = (unsigned int)stream;
			streams->present_mask |= 1UL << element->reg;
			packed_stride += (element->bytes + 3) & ~3UL;
		}
		layout = streams->layouts[streams->layout_count];
		layout[0] = (unsigned int)stream;
		layout[1] = (unsigned int)(aligned ? stride : packed_stride);
		layout[2] = stride ? OPENCE_RENDER_STEP_VERTEX : OPENCE_RENDER_STEP_CONSTANT;
		layout[3] = stride ? 1 : 0;
		streams->buffers[streams->layout_count][0] = (unsigned int)stream;
		streams->buffers[streams->layout_count][1] = buffer;
		streams->buffers[streams->layout_count][2] = (unsigned int)offset;
		streams->layout_count++;
	}
	streams->packed_mask = declaration->packed_mask & streams->present_mask;
	return TRUE;
}

/* ---------- a draw's state */

static unsigned int vertex_library_get(struct vertex_shader_object *program, unsigned long packed, unsigned long present,
	BOOL lit)
{
	struct vertex_library *library;

	for (library = program->libraries; library; library = library->next)
		if (library->packed_mask == packed && library->present_mask == present && library->lit == lit)
			return library->library;
	if (!(library = calloc(1, sizeof(*library))))
		return 0;
	library->packed_mask = packed;
	library->present_mask = present;
	library->lit = lit;
	library->library = library_create(nv2a_vertex_shader_to_msl(program->instructions, program->instruction_count,
		packed, present, lit ? &program->lighting : NULL));
	library->next = program->libraries;
	program->libraries = library;
	if (!library->library)
		platform_log("GPU: vertex shader %lu has no Metal library", program->id);
	return library->library;
}

static unsigned int fragment_library_get(const struct nv2a_pixel_shader_key *key)
{
	unsigned int library = cache_find(_cache_fragment_library, key, sizeof(*key));

	if (!library && (library = library_create(nv2a_pixel_shader_to_msl(key))) != 0)
		cache_insert(_cache_fragment_library, key, sizeof(*key), library);
	return library;
}

/* the pipeline of two libraries, the device's blending and these streams
(NULL for none), drawn into the open pass */
static unsigned int pipeline_get(unsigned int vertex, unsigned int fragment, const struct draw_streams *streams,
	BOOL blend, unsigned int mask)
{
	unsigned int fields[PIPELINE_WORDS];
	unsigned int words = 17, index;

	memset(fields, 0, sizeof(fields));
	fields[1] = vertex;
	fields[2] = fragment;
	fields[3] = OPENCE_RENDER_FORMAT_BGRA8;
	fields[4] = device.pass_depth ? OPENCE_RENDER_FORMAT_DEPTH32F_STENCIL8 : 0;
	fields[5] = 1;
	fields[6] = blend;
	fields[7] = blend ? ring_blend_factor(D3D__RenderState[D3DRS_SRCBLEND]) : OPENCE_RENDER_BLEND_ONE;
	fields[8] = blend ? ring_blend_factor(D3D__RenderState[D3DRS_DESTBLEND]) : OPENCE_RENDER_BLEND_ZERO;
	fields[9] = blend ? ring_blend_operation(D3D__RenderState[D3DRS_BLENDOP]) : OPENCE_RENDER_BLEND_ADD;
	fields[10] = fields[7];
	fields[11] = fields[8];
	fields[12] = fields[9];
	fields[13] = mask;
	if (streams)
	{
		fields[14] = streams->attribute_count;
		fields[15] = streams->layout_count;
		for (index = 0; index < streams->attribute_count; index++, words += 4)
			memcpy(fields + words - 1, streams->attributes[index], 4 * sizeof(*fields));
		for (index = 0; index < streams->layout_count; index++, words += 4)
			memcpy(fields + words - 1, streams->layouts[index], 4 * sizeof(*fields));
	}
	return object_get(_cache_pipeline, OPENCE_RENDER_OBJECT_PIPELINE, OPENCE_RENDER_OP_CREATE_PIPELINE, fields, words);
}

static void pipeline_send(unsigned int pipeline)
{
	command_send(_sent_pipeline, OPENCE_RENDER_OP_SET_PIPELINE, 2, &pipeline);
}

/* everything a draw of these streams needs sent; FALSE to leave it out */
static BOOL draw_prepare(const struct draw_streams *streams)
{
	struct vertex_shader_object *program = current_program();
	struct nv2a_pixel_shader_key key;
	struct nv2a_pixel_uniforms pixel;
	struct nv2a_vertex_uniforms *vertex;
	unsigned int textures[D3DTSS_MAXSTAGES], samplers[D3DTSS_MAXSTAGES];
	unsigned int vertex_library, fragment_library, pipeline, fields[4];
	unsigned int index;
	float blend_color[4];

	if (!program || !program->instructions)
		return FALSE;
	memset(&pixel, 0, sizeof(pixel));
	pixel_key_build(&key, program);
	/* (before the pass: a texture made for the draw can end it, to make its
	levels: xgpu_metal_pass_end) */
	textures_prepare(&key, &pixel, textures, samplers);
	if (!pass_ensure())
		return FALSE;
	pixel_key_finish(&key);
	pixel_uniforms_build(&pixel);
	vertex_library = vertex_library_get(program, streams->packed_mask, streams->present_mask,
		key.per_pixel_lighting != 0);
	fragment_library = fragment_library_get(&key);
	if (!vertex_library || !fragment_library)
		return FALSE;
	pipeline = pipeline_get(vertex_library, fragment_library, streams, D3D__RenderState[D3DRS_ALPHABLENDENABLE] != 0,
		write_mask(D3D__RenderState[D3DRS_COLORWRITEENABLE]));
	if (!pipeline)
		return FALSE;
	pipeline_send(pipeline);
	depth_stencil_send();
	raster_send();
	viewport_send((float)device.viewport.X, (float)device.viewport.Y, (float)device.viewport.Width,
		(float)device.viewport.Height, device.viewport.MinZ, device.viewport.MaxZ);
	color_to_vec4(D3D__RenderState[D3DRS_BLENDCOLOR], blend_color);
	for (index = 0; index < 4; index++)
		fields[index] = opence_render_float(blend_color[index]);
	command_send(_sent_blend_color, OPENCE_RENDER_OP_SET_BLEND_COLOR, 5, fields);
	for (index = 0; index < D3DTSS_MAXSTAGES; index++)
	{
		if (!textures[index] || !samplers[index])
			continue;
		fields[0] = OPENCE_RENDER_STAGE_FRAGMENT;
		fields[1] = index;
		fields[2] = textures[index];
		command_send(_sent_texture0 + (int)index, OPENCE_RENDER_OP_SET_TEXTURE, 4, fields);
		fields[2] = samplers[index];
		command_send(_sent_sampler0 + (int)index, OPENCE_RENDER_OP_SET_SAMPLER, 4, fields);
	}
	for (index = 0; index < streams->layout_count; index++)
		command_send(-1, OPENCE_RENDER_OP_SET_VERTEX_BUFFER, 4, streams->buffers[index]);
	bytes_send(OPENCE_RENDER_STAGE_FRAGMENT, 0, &pixel, sizeof(pixel));
	vertex = xgpu_metal_stage_space(sizeof(*vertex));
	if (vertex)
	{
		vertex_uniforms_build(vertex);
		bytes_send(OPENCE_RENDER_STAGE_VERTEX, XGPU_METAL_VERTEX_UNIFORM_BUFFER, vertex, sizeof(*vertex));
	}
	return TRUE;
}

/* ---------- draws

A primitive Metal does not draw is drawn from indices made for it; the
game's own indices are read where the mirror holds them, or copied to the
stream buffer. Index i of an indexed draw is vertex base + i: the streams
start at its smallest index, and the draw's base vertex takes it off. */

static WORD *index_scratch;
static unsigned long index_scratch_capacity;

static void primitives_draw(D3DPRIMITIVETYPE type, unsigned long count, const WORD *indices, long base_vertex)
{
	BOOL make_indices;
	unsigned int primitive = ring_primitive(type, &make_indices);
	unsigned int buffer, *fields;
	unsigned long offset, index_count = count;

	if (!make_indices && !indices)
	{
		if ((fields = xgpu_metal_command(OPENCE_RENDER_OP_DRAW, OPENCE_RENDER_DRAW_WORDS)) != NULL)
		{
			fields[0] = primitive;
			fields[1] = 0;
			fields[2] = (unsigned int)count;
			fields[3] = 1;
		}
		return;
	}
	if (make_indices)
	{
		unsigned long needed = count * 3 + 3;

		if (needed > index_scratch_capacity)
		{
			WORD *grown = realloc(index_scratch, needed * sizeof(WORD));

			if (!grown)
				return;
			index_scratch = grown;
			index_scratch_capacity = needed;
		}
		index_count = primitive_indices(type, indices, count, index_scratch);
		if (!index_count || !xgpu_metal_stream(index_scratch, index_count * sizeof(WORD), 4, &buffer, &offset))
			return;
	}
	else if (!xgpu_metal_mirror_range((unsigned long)indices, count * sizeof(WORD), &buffer, &offset, NULL) &&
		!xgpu_metal_stream(indices, count * sizeof(WORD), 4, &buffer, &offset))
	{
		return;
	}
	if ((fields = xgpu_metal_command(OPENCE_RENDER_OP_DRAW_INDEXED, OPENCE_RENDER_DRAW_INDEXED_WORDS)) != NULL)
	{
		fields[0] = primitive;
		fields[1] = (unsigned int)index_count;
		fields[2] = OPENCE_RENDER_INDEX_UINT16;
		fields[3] = buffer;
		fields[4] = (unsigned int)offset;
		fields[5] = (unsigned int)base_vertex;
		fields[6] = 1;
	}
}

void WINAPI D3DDevice_SetStreamSource(UINT stream_number, D3DVertexBuffer *stream_data, UINT stride)
{
	if (stream_number >= STREAM_COUNT)
		return;
	device.streams[stream_number].data = stream_data ? stream_data->Data : 0;
	device.streams[stream_number].stride = stride;
}

void WINAPI D3DDevice_SetIndices(D3DIndexBuffer *index_data, UINT base_vertex_index)
{
	device.base_vertex_index = base_vertex_index;
	D3D__IndexData = index_data ? (WORD *)index_data->Data : NULL;
}

void WINAPI D3DDevice_DrawVertices(D3DPRIMITIVETYPE primitive_type, UINT start_vertex, UINT vertex_count)
{
	struct draw_streams streams;

	if (!vertex_count || !device.ready || !streams_setup(&streams, device.vertex_shader, start_vertex, vertex_count) ||
		!draw_prepare(&streams))
		return;
	primitives_draw(primitive_type, vertex_count, NULL, 0);
}

void WINAPI D3DDevice_DrawIndexedVertices(D3DPRIMITIVETYPE primitive_type, UINT vertex_count, CONST WORD *index_data)
{
	struct draw_streams streams;
	unsigned long minimum = 0xffff, maximum = 0, index;

	if (!vertex_count || !index_data || !device.ready)
		return;
	for (index = 0; index < vertex_count; index++)
	{
		minimum = index_data[index] < minimum ? index_data[index] : minimum;
		maximum = index_data[index] > maximum ? index_data[index] : maximum;
	}
	if (!streams_setup(&streams, device.vertex_shader, device.base_vertex_index + minimum, maximum - minimum + 1) ||
		!draw_prepare(&streams))
		return;
	primitives_draw(primitive_type, vertex_count, index_data, -(long)minimum);
}

/* ---------- immediate mode: the registers written since Begin, in one
stream of four floats each; the others are the uniforms' constants */

void WINAPI D3DDevice_Begin(D3DPRIMITIVETYPE primitive_type)
{
	device.immediate_active = TRUE;
	device.immediate_type = primitive_type;
	device.immediate_count = 0;
	device.immediate_written = 0;
}

static void immediate_emit(void)
{
	if (device.immediate_count == device.immediate_capacity)
	{
		unsigned long capacity = device.immediate_capacity ? device.immediate_capacity * 2 : 256;
		void *vertices;

		/* (too many, or out of memory: the vertex is dropped) */
		if (capacity > IMMEDIATE_VERTEX_LIMIT * 16 ||
			!(vertices = realloc(device.immediate_vertices, capacity * sizeof(*device.immediate_vertices))))
			return;
		device.immediate_vertices = vertices;
		device.immediate_capacity = capacity;
	}
	memcpy(device.immediate_vertices[device.immediate_count], device.attributes, sizeof(device.attributes));
	device.immediate_count++;
}

void WINAPI D3DDevice_End(void)
{
	struct draw_streams streams;
	unsigned long registers[XGPU_VERTEX_ATTRIBUTE_COUNT];
	unsigned long register_count = 0, vertex, slot, count = device.immediate_count, stride;
	float *packed;
	unsigned int buffer;
	unsigned long offset;

	device.immediate_active = FALSE;
	if (!count || !device.ready)
		return;
	memset(&streams, 0, sizeof(streams));
	for (slot = 0; slot < XGPU_VERTEX_ATTRIBUTE_COUNT; slot++)
	{
		if (!(device.immediate_written & (1UL << slot)))
			continue;
		streams.attributes[register_count][0] = (unsigned int)slot;
		streams.attributes[register_count][1] = OPENCE_RENDER_VERTEX_FLOAT4;
		streams.attributes[register_count][2] = (unsigned int)(register_count * 4 * sizeof(float));
		streams.attributes[register_count][3] = 0;
		registers[register_count++] = slot;
	}
	if (!register_count)
		return;
	stride = register_count * 4 * sizeof(float);
	if (!(packed = (float *)repack_space(count * stride)))
		return;
	for (vertex = 0; vertex < count; vertex++)
		for (slot = 0; slot < register_count; slot++)
			memcpy(packed + (vertex * register_count + slot) * 4, device.immediate_vertices[vertex][registers[slot]],
				4 * sizeof(float));
	if (!xgpu_metal_stream(packed, count * stride, 4, &buffer, &offset))
		return;
	streams.attribute_count = (unsigned int)register_count;
	streams.layout_count = 1;
	streams.layouts[0][0] = 0;
	streams.layouts[0][1] = (unsigned int)stride;
	streams.layouts[0][2] = OPENCE_RENDER_STEP_VERTEX;
	streams.layouts[0][3] = 1;
	streams.buffers[0][0] = 0;
	streams.buffers[0][1] = buffer;
	streams.buffers[0][2] = (unsigned int)offset;
	streams.present_mask = device.immediate_written;
	if (draw_prepare(&streams))
		primitives_draw(device.immediate_type, count, NULL, 0);
}

static void set_attribute(INT reg, float a, float b, float c, float d)
{
	BOOL emit = FALSE;

	if (reg == D3DVSDE_VERTEX)
	{
		reg = 0;
		emit = TRUE;
	}
	if (reg < 0 || reg >= XGPU_VERTEX_ATTRIBUTE_COUNT)
		return;
	device.attributes[reg][0] = a;
	device.attributes[reg][1] = b;
	device.attributes[reg][2] = c;
	device.attributes[reg][3] = d;
	if (device.immediate_active)
		device.immediate_written |= 1UL << reg;
	/* like the hardware, writing register 0 completes a vertex */
	if (device.immediate_active && (emit || reg == 0))
		immediate_emit();
}

void WINAPI D3DDevice_SetVertexData2f(INT reg, FLOAT a, FLOAT b)
{
	set_attribute(reg, a, b, 0.0f, 1.0f);
}

void WINAPI D3DDevice_SetVertexData4f(INT reg, FLOAT a, FLOAT b, FLOAT c, FLOAT d)
{
	set_attribute(reg, a, b, c, d);
}

void WINAPI D3DDevice_SetVertexData2s(INT reg, SHORT a, SHORT b)
{
	set_attribute(reg, (float)a, (float)b, 0.0f, 1.0f);
}

void WINAPI D3DDevice_SetVertexData4ub(INT reg, BYTE a, BYTE b, BYTE c, BYTE d)
{
	set_attribute(reg, a / 255.0f, b / 255.0f, c / 255.0f, d / 255.0f);
}

void WINAPI D3DDevice_SetVertexDataColor(INT reg, D3DCOLOR color)
{
	float value[4];

	color_to_vec4(color, value);
	set_attribute(reg, value[0], value[1], value[2], value[3]);
}

/* ---------- the device's own shaders: a clear's quad and Present's blit */

static const char clear_source[] =
	"#include <metal_stdlib>\n"
	"using namespace metal;\n"
	"struct clear_uniforms { float4 color; float depth; };\n"
	"struct clear_out { float4 position [[position]]; };\n"
	"vertex clear_out vertex_main(uint id [[vertex_id]], constant clear_uniforms &u [[buffer(0)]])\n"
	"{\n"
	"\tclear_out out;\n"
	"\tout.position = float4((id & 1) ? 1.0 : -1.0, (id & 2) ? -1.0 : 1.0, u.depth, 1.0);\n"
	"\treturn out;\n"
	"}\n"
	"fragment float4 fragment_main(clear_out input [[stage_in]], constant clear_uniforms &u [[buffer(0)]])\n"
	"{\n"
	"\treturn u.color;\n"
	"}\n";

static const char blit_source[] =
	"#include <metal_stdlib>\n"
	"using namespace metal;\n"
	"struct blit_out { float4 position [[position]]; float2 uv; };\n"
	"vertex blit_out vertex_main(uint id [[vertex_id]])\n"
	"{\n"
	"\tblit_out out;\n"
	"\tout.position = float4((id & 1) ? 1.0 : -1.0, (id & 2) ? -1.0 : 1.0, 0.0, 1.0);\n"
	"\tout.uv = float2((id & 1) ? 1.0 : 0.0, (id & 2) ? 1.0 : 0.0);\n"
	"\treturn out;\n"
	"}\n"
	"fragment float4 fragment_main(blit_out input [[stage_in]], texture2d<float> picture [[texture(0)]],\n"
	"\tsampler linear [[sampler(0)]])\n"
	"{\n"
	"\treturn float4(picture.sample(linear, input.uv).rgb, 1.0);\n"
	"}\n";

static unsigned int clear_library, blit_library;

static unsigned int inline_library(unsigned int *library, const char *source)
{
	char *copy;

	if (!*library && (copy = malloc(strlen(source) + 1)) != NULL)
	{
		strcpy(copy, source);
		*library = library_create(copy);
	}
	return *library;
}

/* depth always passing, written or not; the stencil replaced by ref, or
left */
static void depth_stencil_plain(BOOL write_depth, BOOL write_stencil, unsigned int stencil_ref)
{
	unsigned int fields[16], ref[2], face;

	memset(fields, 0, sizeof(fields));
	fields[1] = OPENCE_RENDER_COMPARE_ALWAYS;
	fields[2] = write_depth && device.pass_depth;
	fields[3] = write_stencil && device.pass_depth;
	for (face = 0; face < 2; face++)
	{
		unsigned int *side = fields + 4 + face * 6;
		unsigned int pass = fields[3] ? OPENCE_RENDER_STENCIL_REPLACE : OPENCE_RENDER_STENCIL_KEEP;

		side[0] = OPENCE_RENDER_STENCIL_KEEP;
		side[1] = pass;
		side[2] = pass;
		side[3] = OPENCE_RENDER_COMPARE_ALWAYS;
		side[4] = 0xff;
		side[5] = 0xff;
	}
	ref[0] = object_get(_cache_depth_stencil, OPENCE_RENDER_OBJECT_DEPTH_STENCIL, OPENCE_RENDER_OP_CREATE_DEPTH_STENCIL,
		fields, 17);
	ref[1] = stencil_ref & 0xff;
	if (ref[0])
		command_send(_sent_depth_stencil, OPENCE_RENDER_OP_SET_DEPTH_STENCIL, 3, ref);
}

static void raster_plain(void)
{
	unsigned int fields[7];

	fields[0] = OPENCE_RENDER_CULL_NONE;
	fields[1] = OPENCE_RENDER_WINDING_CLOCKWISE;
	fields[2] = OPENCE_RENDER_FILL_SOLID;
	fields[3] = opence_render_float(0.0f);
	fields[4] = opence_render_float(0.0f);
	fields[5] = opence_render_float(0.0f);
	fields[6] = 1;
	command_send(_sent_raster, OPENCE_RENDER_OP_SET_RASTER, 8, fields);
}

static void quad_draw(void)
{
	unsigned int *fields = xgpu_metal_command(OPENCE_RENDER_OP_DRAW, OPENCE_RENDER_DRAW_WORDS);

	if (!fields)
		return;
	fields[0] = OPENCE_RENDER_PRIMITIVE_TRIANGLE_STRIP;
	fields[1] = 0;
	fields[2] = 4;
	fields[3] = 1;
}

/* ---------- clearing

A clear of the whole target, of all its colour channels or none, begins a
pass that clears its attachments; any other (a rectangle, the viewport of
a split screen, the fog's alpha alone) draws a quad through the channels
named. Like the NV2A, a clear is kept inside the viewport. */

static void clear_quad(long left, long top, long right, long bottom, unsigned int mask, BOOL depth, BOOL stencil,
	const float *rgba, float z, DWORD stencil_value)
{
	struct
	{
		float color[4];
		float depth;
		float pad[3];
	} uniforms;
	unsigned int library = inline_library(&clear_library, clear_source);
	unsigned int pipeline;

	if (!library || !(pipeline = pipeline_get(library, library, NULL, FALSE, mask)))
		return;
	memcpy(uniforms.color, rgba, sizeof(uniforms.color));
	uniforms.depth = z;
	uniforms.pad[0] = uniforms.pad[1] = uniforms.pad[2] = 0.0f;
	pipeline_send(pipeline);
	depth_stencil_plain(depth, stencil, stencil_value);
	raster_plain();
	viewport_send((float)left, (float)top, (float)(right - left), (float)(bottom - top), 0.0f, 1.0f);
	bytes_send(OPENCE_RENDER_STAGE_VERTEX, 0, &uniforms, sizeof(uniforms));
	bytes_send(OPENCE_RENDER_STAGE_FRAGMENT, 0, &uniforms, sizeof(uniforms));
	quad_draw();
}

void WINAPI D3DDevice_Clear(DWORD count, CONST D3DRECT *rectangles, DWORD flags, D3DCOLOR color, float z, DWORD stencil)
{
	float rgba[4];
	unsigned int mask = 0;
	BOOL depth, clear_stencil;
	DWORD index;
	long viewport_left = (long)device.viewport.X, viewport_top = (long)device.viewport.Y;
	long viewport_right = viewport_left + (long)device.viewport.Width;
	long viewport_bottom = viewport_top + (long)device.viewport.Height;

	if (!pass_ensure())
		return;
	depth = device.pass_depth && (flags & D3DCLEAR_ZBUFFER);
	clear_stencil = device.pass_depth && (flags & D3DCLEAR_STENCIL);
	color_to_vec4(color, rgba);
	if (flags & D3DCLEAR_TARGET)
	{
		mask = ((flags & D3DCLEAR_TARGET_R) ? OPENCE_RENDER_WRITE_RED : 0) |
			((flags & D3DCLEAR_TARGET_G) ? OPENCE_RENDER_WRITE_GREEN : 0) |
			((flags & D3DCLEAR_TARGET_B) ? OPENCE_RENDER_WRITE_BLUE : 0) |
			((flags & D3DCLEAR_TARGET_A) ? OPENCE_RENDER_WRITE_ALPHA : 0);
	}
	if (!mask && !depth && !clear_stencil)
		return;
	if (!count || !rectangles)
	{
		BOOL whole = viewport_left <= 0 && viewport_top <= 0 && viewport_right >= (long)device.pass_width &&
			viewport_bottom >= (long)device.pass_height;
		unsigned int all = OPENCE_RENDER_WRITE_RED | OPENCE_RENDER_WRITE_GREEN | OPENCE_RENDER_WRITE_BLUE |
			OPENCE_RENDER_WRITE_ALPHA;

		if (whole && (mask == 0 || mask == all))
		{
			unsigned int color_texture = device.pass_color, depth_texture = device.pass_depth;
			unsigned int *fields;

			pass_end();
			if (!(fields = xgpu_metal_command(OPENCE_RENDER_OP_BEGIN_PASS, OPENCE_RENDER_BEGIN_PASS_WORDS)))
				return;
			memset(fields, 0, 13 * sizeof(*fields));
			fields[0] = color_texture;
			fields[1] = depth_texture;
			fields[2] = mask ? OPENCE_RENDER_LOAD_CLEAR : OPENCE_RENDER_LOAD_LOAD;
			fields[3] = !depth_texture ? OPENCE_RENDER_LOAD_DONT_CARE : depth ? OPENCE_RENDER_LOAD_CLEAR :
				OPENCE_RENDER_LOAD_LOAD;
			fields[4] = !depth_texture ? OPENCE_RENDER_LOAD_DONT_CARE : clear_stencil ? OPENCE_RENDER_LOAD_CLEAR :
				OPENCE_RENDER_LOAD_LOAD;
			for (index = 0; index < 4; index++)
				fields[5 + index] = opence_render_float(rgba[index]);
			fields[9] = opence_render_float(z);
			fields[10] = stencil & 0xff;
			memset(sent, 0, sizeof(sent));
			device.in_pass = TRUE;
			return;
		}
		clear_quad(viewport_left, viewport_top, viewport_right, viewport_bottom, mask, depth, clear_stencil, rgba, z,
			stencil);
		return;
	}
	for (index = 0; index < count; index++)
	{
		long left = rectangles[index].x1 > viewport_left ? rectangles[index].x1 : viewport_left;
		long top = rectangles[index].y1 > viewport_top ? rectangles[index].y1 : viewport_top;
		long right = rectangles[index].x2 < viewport_right ? rectangles[index].x2 : viewport_right;
		long bottom = rectangles[index].y2 < viewport_bottom ? rectangles[index].y2 : viewport_bottom;

		if (left >= right || top >= bottom)
			continue;
		clear_quad(left + ui_offset, top, right + ui_offset, bottom, mask, depth, clear_stencil, rgba, z, stencil);
	}
}

/* ---------- presenting: the back buffer letterboxed in the window */

static void back_buffer_blit(void)
{
	struct xgpu_render_target *back_buffer = xgpu_metal_render_target_get(&device.back_buffer);
	unsigned int library = inline_library(&blit_library, blit_source);
	unsigned int fields[11], pipeline, sampler;
	int window_width, window_height, width, height;

	platform_video_drawable_size(&window_width, &window_height);
	if (!back_buffer || !library || window_width <= 0 || window_height <= 0 ||
		!pass_begin(0, 0, (unsigned long)window_width, (unsigned long)window_height, OPENCE_RENDER_LOAD_CLEAR))
		return;
	if (!(pipeline = pipeline_get(library, library, NULL, FALSE, OPENCE_RENDER_WRITE_RED | OPENCE_RENDER_WRITE_GREEN |
		OPENCE_RENDER_WRITE_BLUE | OPENCE_RENDER_WRITE_ALPHA)))
		return;
	memset(fields, 0, sizeof(fields));
	fields[1] = OPENCE_RENDER_FILTER_LINEAR;
	fields[2] = OPENCE_RENDER_FILTER_LINEAR;
	fields[3] = OPENCE_RENDER_FILTER_NONE;
	fields[4] = fields[5] = fields[6] = OPENCE_RENDER_ADDRESS_CLAMP_TO_EDGE;
	fields[7] = 1;
	fields[8] = opence_render_float(0.0f);
	fields[9] = opence_render_float(0.0f);
	fields[10] = OPENCE_RENDER_BORDER_OPAQUE_BLACK;
	if (!(sampler = object_get(_cache_sampler, OPENCE_RENDER_OBJECT_SAMPLER, OPENCE_RENDER_OP_CREATE_SAMPLER, fields,
		12)))
		return;
	width = window_width;
	height = (int)((long)window_width * back_buffer->gl_height / back_buffer->gl_width);
	if (height > window_height)
	{
		height = window_height;
		width = (int)((long)window_height * back_buffer->gl_width / back_buffer->gl_height);
	}
	pipeline_send(pipeline);
	depth_stencil_plain(FALSE, FALSE, 0);
	raster_plain();
	viewport_send((float)((window_width - width) / 2), (float)((window_height - height) / 2), (float)width,
		(float)height, 0.0f, 1.0f);
	fields[0] = OPENCE_RENDER_STAGE_FRAGMENT;
	fields[1] = 0;
	fields[2] = back_buffer->texture;
	command_send(_sent_texture0, OPENCE_RENDER_OP_SET_TEXTURE, 4, fields);
	fields[2] = sampler;
	command_send(_sent_sampler0, OPENCE_RENDER_OP_SET_SAMPLER, 4, fields);
	quad_draw();
	pass_end();
}

void WINAPI D3DDevice_Present(CONST RECT *source_rectangle, CONST RECT *destination_rectangle,
	void *unused, void *unused2)
{
	(void)source_rectangle;
	(void)destination_rectangle;
	(void)unused;
	(void)unused2;
	if (device.ready)
	{
		pass_end();
		back_buffer_blit();
		xgpu_metal_command(OPENCE_RENDER_OP_PRESENT, OPENCE_RENDER_PRESENT_WORDS);
		xgpu_metal_flush();
		xgpu_metal_begin_frame();
		xgpu_texture_cache_begin_frame();
	}
	device.frame++;
	platform_pump_events();

	pthread_mutex_lock(&vertical_blank_lock);
	/* the Xbox keeps at most two frames queued behind its 60 Hz display;
	with interpolation, frames come at the real display's rate instead,
	paced by the layer's drawables */
	if (halo_interpolation_enabled())
	{
		flip_count++;
	}
	else
	{
		while (pending_flips >= 2)
			pthread_cond_wait(&vertical_blank_condition, &vertical_blank_lock);
		pending_flips++;
	}
	pthread_mutex_unlock(&vertical_blank_lock);
}

HRESULT WINAPI D3DDevice_PersistDisplay(void)
{
	return S_OK;
}

/* ---------- the device */

Direct3D *WINAPI Direct3DCreate8(UINT sdk_version)
{
	(void)sdk_version;
	return (Direct3D *)1;
}

void WINAPI Direct3D_SetPushBufferSize(DWORD push_buffer_size, DWORD segment_count)
{
	(void)push_buffer_size;
	(void)segment_count;
}

/* a 1x1 white texture */
static unsigned int blank_texture_create(void)
{
	static const unsigned char white[4] = { 0xff, 0xff, 0xff, 0xff };
	unsigned int id = xgpu_metal_new_id(OPENCE_RENDER_OBJECT_TEXTURE);
	unsigned int *fields;

	if (!id || !(fields = xgpu_metal_command(OPENCE_RENDER_OP_CREATE_TEXTURE, OPENCE_RENDER_CREATE_TEXTURE_WORDS)))
		return 0;
	fields[0] = id;
	fields[1] = OPENCE_RENDER_TEXTURE_2D;
	fields[2] = OPENCE_RENDER_FORMAT_BGRA8;
	fields[3] = 1;
	fields[4] = 1;
	fields[5] = 1;
	fields[6] = 1;
	fields[7] = OPENCE_RENDER_USAGE_SAMPLED;
	fields[8] = 1;
	if (!(fields = xgpu_metal_command(OPENCE_RENDER_OP_UPLOAD_TEXTURE, OPENCE_RENDER_UPLOAD_TEXTURE_WORDS)))
		return id;
	/* (level 0 at the origin, read from the guest's memory) */
	memset(fields, 0, (OPENCE_RENDER_UPLOAD_TEXTURE_WORDS - 1) * sizeof(*fields));
	fields[0] = id;
	fields[6] = 1;
	fields[7] = 1;
	fields[8] = 1;
	fields[9] = (unsigned int)(unsigned long)white;
	fields[10] = sizeof(white);
	fields[11] = sizeof(white);
	fields[12] = sizeof(white);
	return id;
}

HRESULT WINAPI Direct3D_CreateDevice(UINT adapter, D3DDEVTYPE device_type, void *unused, DWORD behavior_flags,
	D3DPRESENT_PARAMETERS *presentation_parameters, D3DDevice **returned_device)
{
	unsigned long width, height;
	int index;

	(void)adapter;
	(void)device_type;
	(void)unused;
	(void)behavior_flags;
	if (!device.created)
	{
		memset(&device, 0, sizeof(device));
		if (presentation_parameters)
			device.presentation = *presentation_parameters;
		width = device.presentation.BackBufferWidth ? device.presentation.BackBufferWidth : 640;
		height = device.presentation.BackBufferHeight ? device.presentation.BackBufferHeight : 480;
		/* room for the widest screen (halo_screen_commit) */
		d3d8_surface_initialize(&device.back_buffer, D3DFMT_LIN_A8R8G8B8, SCREEN_MAXIMUM_WIDTH, height);
		d3d8_surface_initialize(&device.depth_buffer, D3DFMT_LIN_D24S8, SCREEN_MAXIMUM_WIDTH, height);
		d3d8_surface_resize(&device.back_buffer, D3DFMT_LIN_A8R8G8B8, width, height);
		d3d8_surface_resize(&device.depth_buffer, D3DFMT_LIN_D24S8, width, height);
		device.render_target = &device.back_buffer;
		device.depth_stencil = &device.depth_buffer;
		for (index = 0; index < D3DTS_MAX; index++)
		{
			device.transforms[index]._11 = 1.0f;
			device.transforms[index]._22 = 1.0f;
			device.transforms[index]._33 = 1.0f;
			device.transforms[index]._44 = 1.0f;
		}
		device.viewport.Width = width;
		device.viewport.Height = height;
		device.viewport.MaxZ = 1.0f;
		device.next_vertex_shader_id = 1;
		D3D__RenderState[D3DRS_ZENABLE] = TRUE;
		D3D__RenderState[D3DRS_ZWRITEENABLE] = TRUE;
		D3D__RenderState[D3DRS_ZFUNC] = D3DCMP_LESSEQUAL;
		D3D__RenderState[D3DRS_COLORWRITEENABLE] = D3DCOLORWRITEENABLE_ALL;
		D3D__RenderState[D3DRS_SRCBLEND] = D3DBLEND_ONE;
		D3D__RenderState[D3DRS_DESTBLEND] = D3DBLEND_ZERO;
		D3D__RenderState[D3DRS_BLENDOP] = D3DBLENDOP_ADD;
		D3D__RenderState[D3DRS_CULLMODE] = D3DCULL_CCW;
		D3D__RenderState[D3DRS_FRONTFACE] = D3DFRONT_CW;
		D3D__RenderState[D3DRS_FILLMODE] = D3DFILL_SOLID;
		D3D__RenderState[D3DRS_ALPHAFUNC] = D3DCMP_ALWAYS;
		D3D__RenderState[D3DRS_STENCILFUNC] = D3DCMP_ALWAYS;
		D3D__RenderState[D3DRS_STENCILMASK] = 0xff;
		D3D__RenderState[D3DRS_STENCILWRITEMASK] = 0xff;
		D3D__RenderState[D3DRS_STENCILFAIL] = D3DSTENCILOP_KEEP;
		D3D__RenderState[D3DRS_STENCILZFAIL] = D3DSTENCILOP_KEEP;
		D3D__RenderState[D3DRS_STENCILPASS] = D3DSTENCILOP_KEEP;
		for (index = 0; index < D3DTSS_MAXSTAGES; index++)
		{
			D3D__TextureState[index][D3DTSS_ADDRESSU] = D3DTADDRESS_WRAP;
			D3D__TextureState[index][D3DTSS_ADDRESSV] = D3DTADDRESS_WRAP;
			D3D__TextureState[index][D3DTSS_ADDRESSW] = D3DTADDRESS_WRAP;
			D3D__TextureState[index][D3DTSS_MAGFILTER] = D3DTEXF_POINT;
			D3D__TextureState[index][D3DTSS_MINFILTER] = D3DTEXF_POINT;
			D3D__TextureState[index][D3DTSS_MAXANISOTROPY] = 1;
		}
		viewport_update_constants();

		if (!config_boolean("debug.null_renderer") && platform_video_initialize(width, height))
		{
			device.blank_texture = blank_texture_create();
			device.ready = TRUE;
			vertical_blank_start();
		}
		else
		{
			platform_log("Direct3D: running without a window (nothing is displayed)");
		}
		device.created = TRUE;
	}
	*returned_device = (D3DDevice *)1;
	return S_OK;
}

ULONG WINAPI D3DDevice_Release(void)
{
	return 1;
}

void WINAPI D3DDevice_GetDeviceCaps(D3DCAPS8 *caps)
{
	memset(caps, 0, sizeof(*caps));
	caps->DeviceType = D3DDEVTYPE_HAL;
	caps->MaxTextureWidth = 4096;
	caps->MaxTextureHeight = 4096;
	caps->MaxVolumeExtent = 512;
	caps->MaxTextureRepeat = 8192;
	caps->MaxTextureAspectRatio = 4096;
	caps->MaxAnisotropy = 4;
	caps->MaxTextureBlendStages = 4;
	caps->MaxSimultaneousTextures = 4;
	caps->MaxActiveLights = 8;
	caps->MaxVertexBlendMatrices = 4;
	caps->MaxPointSize = 64.0f;
	caps->MaxPrimitiveCount = 0xfffff;
	caps->MaxVertexIndex = 0xffff;
	caps->MaxStreams = 16;
	caps->MaxStreamStride = 255;
	caps->VertexShaderVersion = D3DVS_VERSION(1, 1);
	caps->MaxVertexShaderConst = 192;
	caps->PixelShaderVersion = D3DPS_VERSION(1, 1);
	caps->MaxPixelShaderValue = 1.0f;
}

void WINAPI D3DDevice_GetBackBuffer(INT back_buffer, D3DBACKBUFFER_TYPE type, D3DSurface **result)
{
	(void)back_buffer;
	(void)type;
	/* like Direct3D, the caller gets a reference it must release */
	device.back_buffer.Common++;
	*result = &device.back_buffer;
}

HRESULT WINAPI D3DDevice_GetDepthStencilSurface(D3DSurface **result)
{
	*result = device.depth_stencil;
	if (!*result)
		return D3DERR_NOTFOUND;
	(*result)->Common++;
	return S_OK;
}

void WINAPI D3DDevice_SetRenderTarget(D3DSurface *render_target, D3DSurface *depth_stencil)
{
	if (render_target)
		device.render_target = render_target;
	device.depth_stencil = depth_stencil;
	/* like Direct3D, reset the viewport to the whole new target */
	if (device.render_target)
	{
		unsigned long width, height;
		BOOL depth;

		surface_dimensions(device.render_target, &width, &height, &depth);
		device.viewport.X = 0;
		device.viewport.Y = 0;
		device.viewport.Width = width;
		device.viewport.Height = height;
		device.viewport.MinZ = 0.0f;
		device.viewport.MaxZ = 1.0f;
	}
	viewport_update_constants();
}

void WINAPI D3DDevice_SetViewport(CONST D3DVIEWPORT8 *viewport)
{
	device.viewport = *viewport;
	viewport_update_constants();
}

void WINAPI D3DDevice_SetTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX *matrix)
{
	if ((unsigned long)state < D3DTS_MAX)
		device.transforms[state] = *matrix;
}

void WINAPI D3DDevice_GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
	if ((unsigned long)state < D3DTS_MAX)
		*matrix = device.transforms[state];
}

void WINAPI D3DDevice_SetFlickerFilter(DWORD filter) { (void)filter; }
void WINAPI D3DDevice_SetSoftDisplayFilter(BOOL enable) { (void)enable; }

void WINAPI D3DDevice_SetShaderConstantMode(D3DSHADERCONSTANTMODE mode)
{
	device.shader_constant_mode = mode;
	viewport_update_constants();
}

/* ---------- synchronisation: the ring keeps its own order */

BOOL WINAPI D3DDevice_IsBusy(void)
{
	return FALSE;
}

void WINAPI D3DDevice_KickPushBuffer(void)
{
	if (device.ready)
	{
		pass_end();
		xgpu_metal_flush();
	}
}

void WINAPI D3DDevice_InsertCallback(D3DCALLBACKTYPE type, D3DCALLBACK callback, DWORD context)
{
	(void)type;
	/* the "GPU" reaches the callback immediately */
	if (callback)
		callback(context);
}

/* ---------- visibility (occlusion) tests: until the ring reads results
back, every test passes (all its samples drawn) */

#define VISIBILITY_TEST_SLOTS 4096

static BOOL visibility_active;
static BOOL query_pending[VISIBILITY_TEST_SLOTS];

void WINAPI D3DDevice_BeginVisibilityTest(void)
{
	visibility_active = device.ready;
}

HRESULT WINAPI D3DDevice_EndVisibilityTest(DWORD index)
{
	if (!visibility_active)
		return S_OK;
	visibility_active = FALSE;
	query_pending[index % VISIBILITY_TEST_SLOTS] = TRUE;
	return S_OK;
}

HRESULT WINAPI D3DDevice_GetVisibilityTestResult(DWORD index, UINT *result, ULONGLONG *time_stamp)
{
	if (time_stamp)
		*time_stamp = 0;
	index %= VISIBILITY_TEST_SLOTS;
	if (result)
		*result = query_pending[index] ? VISIBILITY_ALL_SAMPLES : 0;
	query_pending[index] = FALSE;
	return S_OK;
}

/* ---------- the menus' pointer (d3d8_gl.c's) */

static short floor_short(float value)
{
	long whole = (long)value;

	return (short)(value < (float)whole ? whole - 1 : whole);
}

/* a point in the window, as SDL reports it, in the menus' coordinates (the
inverse of Present's letterbox and the menus' centering), or the screen's
when not centered; -1 and -1 without a back buffer or window */
static void ui_point_from_window_on(float window_x, float window_y, int centered, short *x, short *y)
{
	struct xgpu_render_target *back_buffer = xgpu_metal_render_target_get(&device.back_buffer);
	int window_width, window_height, pixel_width, pixel_height, width, height, left, top;
	float screen_x, screen_y;

	*x = *y = -1;
	if (!back_buffer)
		return;
	platform_video_window_size(&window_width, &window_height);
	platform_video_drawable_size(&pixel_width, &pixel_height);
	if (window_width <= 0 || window_height <= 0)
		return;
	width = pixel_width;
	height = (int)((long)pixel_width * back_buffer->gl_height / back_buffer->gl_width);
	if (height > pixel_height)
	{
		height = pixel_height;
		width = (int)((long)pixel_height * back_buffer->gl_width / back_buffer->gl_height);
	}
	left = (pixel_width - width) / 2;
	top = (pixel_height - height) / 2;
	screen_x = (window_x * pixel_width / window_width - left) * (float)back_buffer->width / (float)width;
	screen_y = (window_y * pixel_height / window_height - top) * (float)back_buffer->height / (float)height;
	*x = floor_short(screen_x - (centered ? (float)(halo_screen_width() - 640) / 2.0f : 0.0f));
	*y = floor_short(screen_y);
}

static void ui_point_from_window(float window_x, float window_y, short *x, short *y)
{
	ui_point_from_window_on(window_x, window_y, TRUE, x, y);
}

int halo_scoreboard_pointer_update(int offered, struct halo_ui_pointer *pointer)
{
	(void)offered;
	(void)pointer;
	return -1;
}

int halo_ui_pointer_update(int menus_active, struct halo_ui_pointer *pointer)
{
	struct platform_ui_pointer state;

	platform_menus_set_active(menus_active != 0);
	platform_ui_pointer_set_active(menus_active != 0);
	if (!menus_active || !device.ready || !platform_ui_pointer_read(&state))
		return 0;
	memset(pointer, 0, sizeof(*pointer));
	ui_point_from_window(state.x, state.y, &pointer->x, &pointer->y);
	ui_point_from_window(state.click_x, state.click_y, &pointer->click_x, &pointer->click_y);
	pointer->moved = state.moved != FALSE;
	pointer->left_clicks = (unsigned char)(state.left_clicks < 255 ? state.left_clicks : 255);
	pointer->right_clicks = (unsigned char)(state.right_clicks < 255 ? state.right_clicks : 255);
	pointer->wheel_steps = (signed char)(state.wheel_steps < -8 ? -8 : state.wheel_steps > 8 ? 8 : state.wheel_steps);
	pointer->touch = state.touch != FALSE;
	pointer->downs = (unsigned char)(state.downs < 255 ? state.downs : 255);
	ui_point_from_window(state.down_x, state.down_y, &pointer->down_x, &pointer->down_y);
	return 1;
}
