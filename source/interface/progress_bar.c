/*
PROGRESS_BAR.C

symbols in this file:
000D0D10 01b0:
	_code_000d0d10 (0000)
000D0EC0 0050:
	_code_000d0ec0 (0000)
000D0F10 0010:
	_code_000d0f10 (0000)
000D0F20 0020:
	_code_000d0f20 (0000)
000D0F40 0010:
	_code_000d0f40 (0000)
000D0F50 00d0:
	_tgaLoadHeader (0000)
000D1020 0010:
	_code_000d1020 (0000)
000D1030 0020:
	_code_000d1030 (0000)
000D1050 0090:
	_tgaLoadImageData (0000)
000D10E0 0010:
	_code_000d10e0 (0000)
000D10F0 0010:
	_code_000d10f0 (0000)
000D1100 0050:
	_tgaLoad (0000)
000D1150 0010:
	_code_000d1150 (0000)
000D1160 0220:
	_code_000d1160 (0000)
000D1380 0010:
	_code_000d1380 (0000)
000D1390 0060:
	_code_000d1390 (0000)
000D13F0 0090:
	_progress_bar_initialize (0000)
000D1480 0010:
	_progress_bar_dispose (0000)
000D1490 0030:
	_progress_bar_begin (0000)
000D14C0 0010:
	_code_000d14c0 (0000)
000D14D0 0020:
	_progress_bar_end (0000)
000D14F0 0010:
	_progress_bar_is_active (0000)
000D1500 0050:
	_to_screen (0000)
000D1550 0110:
	_generate_gravy_rect (0000)
000D1660 0010:
	_code_000d1660 (0000)
000D1670 0010:
	_code_000d1670 (0000)
000D1680 0020:
	_code_000d1680 (0000)
000D16A0 0030:
	_code_000d16a0 (0000)
000D16D0 0010:
	_code_000d16d0 (0000)
000D16E0 0010:
	_code_000d16e0 (0000)
000D16F0 0010:
	_code_000d16f0 (0000)
000D1700 0030:
	_gen_cloud_coord (0000)
000D1730 0040:
	_gen_mask_coord (0000)
000D1770 0160:
	_code_000d1770 (0000)
000D18D0 0060:
	_code_000d18d0 (0000)
000D1930 0060:
	_code_000d1930 (0000)
000D1990 0030:
	_code_000d1990 (0000)
000D19C0 00a0:
	_code_000d19c0 (0000)
000D1A60 0130:
	_code_000d1a60 (0000)
000D1B90 0010:
	_progress_bar_enable (0000)
000D1BA0 0010:
	_code_000d1ba0 (0000)
000D1BB0 0010:
	_code_000d1bb0 (0000)
000D1BC0 0010:
	_code_000d1bc0 (0000)
000D1BD0 0020:
	_code_000d1bd0 (0000)
000D1BF0 0010:
	_code_000d1bf0 (0000)
000D1C00 0040:
	_D3DXMatrixIdentity (0000)
000D1C40 0250:
	_SetRenderStateSmart (0000)
000D1E90 0070:
	_SetTextureStageStateSmart (0000)
000D1F00 04b0:
	_code_000d1f00 (0000)
000D23B0 00b0:
	_code_000d23b0 (0000)
000D2460 00d0:
	_code_000d2460 (0000)
000D2530 00c0:
	_progress_bar_eachframe (0000)
000D25F0 0490:
	_code_000d25f0 (0000)
000D2A80 00a0:
	_progress_bar_display (0000)
00271088 0148:
	_D3DSIMPLERENDERSTATEENCODE (0000)
002711D0 0003:
	??_C@_02JDPG@rb?$AA@ (0000)
002711D4 0004:
	??_C@_03OAKNMOCF@z?$CFs?$AA@ (0000)
002711D8 000f:
	??_C@_0P@HMFGDDOL@z?$CFsloading?4tga?$AA@ (0000)
002711E8 000f:
	??_C@_0P@NFEHLENB@d?$CFsloading?4tga?$AA@ (0000)
002711F8 0014:
	??_C@_0BE@GLEHOOPO@rect?9?$DOy0?5?$DM?5rect?9?$DOy1?$AA@ (0000)
0027120C 0014:
	??_C@_0BE@PHIKGFLP@rect?9?$DOx0?5?$DM?5rect?9?$DOx1?$AA@ (0000)
00271220 0028:
	??_C@_0CI@MFDDAKJL@c?3?2halo?2SOURCE?2interface?2progres@ (0000)
00271248 0004:
	__real@42000000 (0000)
0027124C 0004:
	__real@44400000 (0000)
00271250 0004:
	__real@44200000 (0000)
00271258 0008:
	__real@406e000000000000 (0000)
00271260 0008:
	__real@4074000000000000 (0000)
00271268 0008:
	__real@3fd3333340000000 (0000)
00271270 0008:
	__real@400921cac083126f (0000)
00271278 0008:
	__real@3fecccccc0000000 (0000)
00271280 0008:
	__real@400921f9f01b866e (0000)
00271288 0008:
	__real@3fc999999999999a (0000)
00271290 0008:
	__real@3fa999999999999a (0000)
00271298 0004:
	__real@3ca0a0a1 (0000)
0027129C 0004:
	__real@3f7afafb (0000)
002712A0 0008:
	__real@3fe8000000000000 (0000)
002712A8 0004:
	__real@40490e56 (0000)
002712AC 0023:
	??_C@_0CD@KJFNFKIK@?$CIprogress?$DO?$DN0?4f?$CJ?5?$CG?$CG?5?$CIprogress?$DM?$DN1?4@ (0000)
002FD5A8 47f1:
	_data_002fd5a8 (0000)
	_compressed_monk_tone (0008)
	_delta_z (47e4)
	_num_layers (47e8)
00454030 01f4:
	_bss_00454030 (0000)
	_progress_bar_mode (01d8)
	_last_t (01ec)
*/

/* ---------- headers */

#include "interface/progress_bar.h"
#include "cache/cache_files.h"
/* The January object retains out-of-line copies of the D3D and D3DX inline
 * wrappers.  The stock XDK definitions of D3DINLINE (static __forceinline) and
 * D3DXINLINE (_inline) reproduce every one of them, including
 * IDirect3DDevice8_SetRenderState's 0x220-byte body.  Do not replace them with
 * handwritten Microsoft dispatchers or override the XDK's inline policy:
 * taking an address or weakening the inline specifier changes their emitted
 * ABI and code shape.
 */
#include <xtl.h>

/* ---------- constants */

enum
{
	NUMBER_OF_PROGRESS_BAR_SOUNDS= 4
};

/* ---------- macros */

/* ---------- structures */

struct progress_bar_globals
{
	unsigned long start_time;
	real initial_progress;
	D3DXMATRIX saved_projection;
	D3DXMATRIX saved_world;
	D3DXMATRIX saved_view;
	D3DXMATRIX screen_transform;
	D3DXMATRIX projection;
	D3DXMATRIX world;
	D3DXMATRIX view;
	IDirectSoundBuffer *sounds[NUMBER_OF_PROGRESS_BAR_SOUNDS];
};

struct progress_bar_mode
{
	long state;
	boolean capture_frame;
	unsigned char padding0[3];
	IDirect3DTexture8 *texture0;
	IDirect3DTexture8 *texture1;
	boolean active;
	unsigned char padding1[3];
};

struct pixel_shader_definition
{
	unsigned long alpha_inputs[8];
	unsigned long final_combiner_inputs_abcd;
	unsigned long final_combiner_inputs_efg;
	unsigned long constant_0[8];
	unsigned long constant_1[8];
	unsigned long alpha_outputs[8];
	unsigned long rgb_inputs[8];
	unsigned long compare_mode;
	unsigned long final_combiner_constant_0;
	unsigned long final_combiner_constant_1;
	unsigned long rgb_outputs[8];
	unsigned long combiner_count;
	unsigned long texture_modes;
	unsigned long dot_mapping;
	unsigned long input_texture;
	unsigned long c0_mapping;
	unsigned long c1_mapping;
	unsigned long final_combiner_constants;
};

struct gravy_layer
{
	real texture_width;
	real texture_height;
	real x;
	real y;
	real half_width;
	real half_height;
	real distance;
};

struct gravy_rectangle2d
{
	real x0;
	real y0;
	real x1;
	real y1;
};

struct tga_image
{
	unsigned char reserved[4];
	unsigned char image_type;
	unsigned char pixel_depth;
	short width;
	short height;
	unsigned char padding[2];
	unsigned long *pixels;
};

/* ---------- prototypes */

void generate_gravy_rect(
	struct gravy_layer const *layer,
	struct gravy_rectangle2d *rect);

static void draw_gravy_layer(
	struct gravy_layer const *layer,
	real_rgb_color const *color,
	real alpha,
	real progress);

void draw_gravy_screen_layer(
	real distance,
	real alpha,
	real_rgb_color const *color,
	real progress);

void set_blur_texture_coordinates(
	real x,
	real y);

void draw_fullscreen_blur(
	real x_offset,
	real y_offset,
	real alpha);

static void progress_bar_load_loading_texture(
	IDirect3DTexture8 **texture);

static void progress_bar_build_glow_texture(
	void);

static void progress_bar_setup(
	void);

static void progress_bar_alias_surface_texture(
	D3DBaseTexture *texture,
	D3DSurface const *surface);

static void progress_bar_draw(
	real progress);

void tgaLoadHeader(
	FILE *file,
	struct tga_image *image);

void tgaLoadImageData(
	FILE *file,
	struct tga_image *image);

/* ---------- globals */

extern void *global_d3d_device;
extern struct pixel_shader_definition blur_shader;
extern struct pixel_shader_definition regular_shader;
extern byte compressed_monk_tone[18396];
extern unsigned long current_time;

real delta_z= 5.f;
real num_layers= 40.f;
static real blur_offset= 0.1f;
static boolean progress_bar_restart= TRUE;

boolean progress_bar_rendering_enabled= TRUE;
static struct progress_bar_globals progress_bar_globals= {0};
struct progress_bar_mode progress_bar_mode= {0};
real last_t= 0.f;
static real wobble_phase= 0.f;

/* ---------- public code */

void tgaLoadHeader(
	FILE *file,
	struct tga_image *image)
{
	unsigned char ignored_byte;
	short ignored_short;

	fread(&ignored_byte, 1, 1, file);
	fread(&ignored_byte, 1, 1, file);
	fread(&image->image_type, 1, 1, file);
	fread(&ignored_short, 2, 1, file);
	fread(&ignored_short, 2, 1, file);
	fread(&ignored_byte, 1, 1, file);
	fread(&ignored_short, 2, 1, file);
	fread(&ignored_short, 2, 1, file);
	fread(&image->width, 2, 1, file);
	fread(&image->height, 2, 1, file);
	fread(&image->pixel_depth, 1, 1, file);
	fread(&ignored_byte, 1, 1, file);

	return;
}

void tgaLoadImageData(
	FILE *file,
	struct tga_image *image)
{
	long y;

	for (y= 0; y<image->height; y++)
	{
		long x;
		unsigned long *pixel;

		pixel= image->pixels + (image->height - y - 1)*image->width;
		for (x= 0; x<image->width; x++)
		{
			unsigned char color[3];
			unsigned long intensity;

			fread(color, 1, 3, file);
			intensity= color[0];
			*pixel= ((intensity<<9) | (intensity & ~1))<<7 | intensity>>2;
			pixel++;
		}
	}

	return;
}

void tgaLoad(
	const char *path,
	unsigned long *pixels)
{
	struct tga_image image;
	FILE *file;

	file= fopen(path, "rb");
	if (file)
	{
		tgaLoadHeader(file, &image);
		image.pixels= pixels;
		tgaLoadImageData(file, &image);
		fclose(file);
	}

	return;
}

void progress_bar_initialize(
	void)
{
	char destination_directory[40];
	char map_directory[40];
	char destination_path[40];
	char source_path[40];

	csstrcpy(map_directory, cache_files_map_directory());
	sprintf(source_path, "d%sloading.tga", map_directory + 1);
	sprintf(destination_path, "z%sloading.tga", map_directory + 1);
	sprintf(destination_directory, "z%s", map_directory + 1);
	destination_directory[csstrlen(destination_directory) - 1]= 0;
	CreateDirectoryA(destination_directory, NULL);
	CopyFileA(source_path, destination_path, TRUE);

	return;
}

void progress_bar_dispose(
	void)
{
	return;
}

void progress_bar_begin(
	boolean skip_frame_capture)
{
	progress_bar_mode.capture_frame= !skip_frame_capture;
	progress_bar_mode.active= TRUE;
	SetThreadPriority((void *)-2, 2);

	return;
}

void progress_bar_end(
	void)
{
	progress_bar_mode.active= FALSE;
	SetThreadPriority((void *)-2, 0);

	return;
}

boolean progress_bar_is_active(
	void)
{
	return progress_bar_mode.active;
}

void progress_bar_enable(
	boolean enabled)
{
	progress_bar_rendering_enabled= enabled;

	return;
}

void SetRenderStateSmart(
	D3DRENDERSTATETYPE state,
	unsigned long value)
{
	if (state < D3DRS_SIMPLE_MAX)
	{
		D3DDevice_SetRenderState_Simple(D3DSIMPLERENDERSTATEENCODE[state], value);
		D3D__RenderState[state]= value;
	}
	else if (state < D3DRS_DEFERRED_MAX)
	{
		D3DDevice_SetRenderState_Deferred(state, value);
	}
	else if (state == D3DRS_PSTEXTUREMODES)
	{
		D3DDevice_SetRenderState_PSTextureModes(value);
	}
	else if (state == D3DRS_VERTEXBLEND)
	{
		D3DDevice_SetRenderState_VertexBlend(value);
	}
	else if (state == D3DRS_FOGCOLOR)
	{
		D3DDevice_SetRenderState_FogColor(value);
	}
	else if (state == D3DRS_FILLMODE)
	{
		D3DDevice_SetRenderState_FillMode(value);
	}
	else if (state == D3DRS_BACKFILLMODE)
	{
		D3DDevice_SetRenderState_BackFillMode(value);
	}
	else if (state == D3DRS_TWOSIDEDLIGHTING)
	{
		D3DDevice_SetRenderState_TwoSidedLighting(value);
	}
	else if (state == D3DRS_NORMALIZENORMALS)
	{
		D3DDevice_SetRenderState_NormalizeNormals(value);
	}
	else if (state == D3DRS_ZENABLE)
	{
		D3DDevice_SetRenderState_ZEnable(value);
	}
	else if (state == D3DRS_STENCILENABLE)
	{
		D3DDevice_SetRenderState_StencilEnable(value);
	}
	else if (state == D3DRS_STENCILFAIL)
	{
		D3DDevice_SetRenderState_StencilFail(value);
	}
	else if (state == D3DRS_CULLMODE)
	{
		D3DDevice_SetRenderState_CullMode(value);
	}
	else if (state == D3DRS_FRONTFACE)
	{
		D3DDevice_SetRenderState_FrontFace(value);
	}
	else if (state == D3DRS_TEXTUREFACTOR)
	{
		D3DDevice_SetRenderState_TextureFactor(value);
	}
	else if (state == D3DRS_ZBIAS)
	{
		D3DDevice_SetRenderState_ZBias(value);
	}
	else if (state == D3DRS_LOGICOP)
	{
		D3DDevice_SetRenderState_LogicOp(value);
	}
	else if (state == D3DRS_EDGEANTIALIAS)
	{
		D3DDevice_SetRenderState_EdgeAntiAlias(value);
	}
	else if (state == D3DRS_MULTISAMPLEANTIALIAS)
	{
		D3DDevice_SetRenderState_MultiSampleAntiAlias(value);
	}
	else if (state == D3DRS_MULTISAMPLEMASK)
	{
		D3DDevice_SetRenderState_MultiSampleMask(value);
	}
	else if (state == D3DRS_MULTISAMPLETYPE)
	{
		D3DDevice_SetRenderState_MultiSampleType(value);
	}
	else if (state == D3DRS_SHADOWFUNC)
	{
		D3DDevice_SetRenderState_ShadowFunc(value);
	}
	else if (state == D3DRS_LINEWIDTH)
	{
		D3DDevice_SetRenderState_LineWidth(value);
	}
	else if (state == D3DRS_DXT1NOISEENABLE)
	{
		D3DDevice_SetRenderState_Dxt1NoiseEnable(value);
	}
	else if (state == D3DRS_YUVENABLE)
	{
		D3DDevice_SetRenderState_YuvEnable(value);
	}
	else if (state == D3DRS_OCCLUSIONCULLENABLE)
	{
		D3DDevice_SetRenderState_OcclusionCullEnable(value);
	}
	else if (state == D3DRS_STENCILCULLENABLE)
	{
		D3DDevice_SetRenderState_StencilCullEnable(value);
	}
	else if (state == D3DRS_ROPZCMPALWAYSREAD)
	{
		D3DDevice_SetRenderState_RopZCmpAlwaysRead(value);
	}
	else if (state == D3DRS_ROPZREAD)
	{
		D3DDevice_SetRenderState_RopZRead(value);
	}
	else if (state == D3DRS_DONOTCULLUNCOMPRESSED)
	{
		D3DDevice_SetRenderState_DoNotCullUncompressed(value);
	}

	return;
}

void SetTextureStageStateSmart(
	unsigned long stage,
	D3DTEXTURESTAGESTATETYPE type,
	unsigned long value)
{
	if (type < D3DTSS_DEFERRED_MAX)
	{
		D3DDevice_SetTextureState_Deferred(stage, type, value);
	}
	else if (type == D3DTSS_TEXCOORDINDEX)
	{
		D3DDevice_SetTextureState_TexCoordIndex(stage, value);
	}
	else if (type == D3DTSS_BORDERCOLOR)
	{
		D3DDevice_SetTextureState_BorderColor(stage, value);
	}
	else if (type == D3DTSS_COLORKEYCOLOR)
	{
		D3DDevice_SetTextureState_ColorKeyColor(stage, value);
	}
	else if ((type >= D3DTSS_BUMPENVMAT00) && (type <= D3DTSS_BUMPENVLOFFSET))
	{
		D3DDevice_SetTextureState_BumpEnv(stage, type, value);
	}

	return;
}

void progress_bar_eachframe(
	void)
{
	unsigned long time= GetTickCount();
	long sound_index;

	if (progress_bar_restart)
	{
		progress_bar_globals.start_time= GetTickCount();
		progress_bar_restart= FALSE;
		progress_bar_mode.state= 1;
	}

	if (time - progress_bar_globals.start_time>1000)
	{
		last_t= 0;
		if (progress_bar_mode.texture0)
		{
			IDirect3DResource8_Release((D3DResource *)progress_bar_mode.texture0);
			progress_bar_mode.texture0= NULL;
		}
		if (progress_bar_mode.texture1)
		{
			IDirect3DResource8_Release((D3DResource *)progress_bar_mode.texture1);
			progress_bar_mode.texture1= NULL;
		}
		if (progress_bar_mode.texture0)
		{
			IDirect3DResource8_Release((D3DResource *)progress_bar_mode.texture0);
			progress_bar_mode.texture0= NULL;
		}
		for (sound_index= 0; sound_index<NUMBER_OF_PROGRESS_BAR_SOUNDS; sound_index++)
		{
			if (progress_bar_globals.sounds[sound_index])
			{
				IDirectSoundBuffer_Stop(progress_bar_globals.sounds[sound_index]);
				IDirectSoundBuffer_Release(progress_bar_globals.sounds[sound_index]);
				progress_bar_globals.sounds[sound_index]= NULL;
			}
		}
	}

	return;
}

void progress_bar_display(
	real progress)
{
#line 827 "c:\\halo\\SOURCE\\interface\\progress_bar.c"
	match_assert(__FILE__, __LINE__, (progress>=0.f) && (progress<=1.f));

	if (progress_bar_mode.active && progress>0.f)
	{
		if (!progress_bar_mode.texture0)
		{
			progress_bar_setup();
			progress_bar_globals.initial_progress= progress;
		}

		if (progress_bar_rendering_enabled)
		{
			progress_bar_draw((progress - progress_bar_globals.initial_progress) /
				(1.f - progress_bar_globals.initial_progress));
		}
	}

	return;
}

/* ---------- private code */

void to_screen(
	real x,
	real y,
	real z,
	real *screen_x,
	real *screen_y)
{
	D3DXVECTOR4 input;
	D3DXVECTOR4 output;

	input.x = x;
	input.y = y;
	input.z = z;
	input.w = 1.0f;
	D3DXVec4Transform(&output, &input, &progress_bar_globals.screen_transform);
	*screen_x = output.x / output.w;
	*screen_y = output.y / output.w;
	return;
}

static void draw_gravy_layer(
	struct gravy_layer const *layer,
	real_rgb_color const *color,
	real alpha,
	real progress)
{
	struct gravy_rectangle2d rect;
	real half_width= layer->texture_width*0.5f;
	real half_height= layer->texture_height*0.5f;
	real mask_position;

	generate_gravy_rect(layer, &rect);
	IDirect3DDevice8_Begin(global_d3d_device, D3DPT_TRIANGLEFAN);
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_DIFFUSE,
		color->red, color->green, color->blue, alpha);
	mask_position= progress*768.f - 64.f - 64.f;
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD1, 0.f - mask_position, 0.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD0, 0.f, half_height);
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, rect.x0, rect.y0, 0.5f, 1.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD1, 640.f - mask_position, 0.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD0, half_width, half_height);
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, rect.x1, rect.y0, 0.5f, 1.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD1, 640.f - mask_position, 16.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD0, half_width, 0.f);
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, rect.x1, rect.y1, 0.5f, 1.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD1, 0.f - mask_position, 16.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD0, 0.f, 0.f);
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, rect.x0, rect.y1, 0.5f, 1.f);
	IDirect3DDevice8_End(global_d3d_device);

	return;
}

void draw_gravy_screen_layer(
	real distance,
	real alpha,
	real_rgb_color const *color,
	real progress)
{
	struct gravy_layer layer;

	layer.texture_width= 640.f;
	layer.texture_height= 480.f;
	layer.x= 0.f;
	layer.y= 0.f;
	layer.half_width= (real)fabs(640.0/2.0);
	layer.half_height= (real)fabs(480.0/2.0);
	layer.distance= distance;
	draw_gravy_layer(&layer, color, alpha, progress);

	return;
}

void set_blur_texture_coordinates(
	real x,
	real y)
{
	D3DDevice_SetVertexData2f(D3DVSDE_TEXCOORD0, x - blur_offset, blur_offset + y);
	D3DDevice_SetVertexData2f(D3DVSDE_TEXCOORD1, x - blur_offset, y - blur_offset);
	D3DDevice_SetVertexData2f(D3DVSDE_TEXCOORD2, blur_offset + x, y - blur_offset);
	D3DDevice_SetVertexData2f(D3DVSDE_TEXCOORD3, blur_offset + x, blur_offset + y);

	return;
}

void draw_fullscreen_blur(
	real x_offset,
	real y_offset,
	real alpha)
{
	real left, right, top, bottom;

	IDirect3DDevice8_Begin(global_d3d_device, D3DPT_TRIANGLEFAN);
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_DIFFUSE,
		254.f/255.f, 254.f/255.f, 1.f, alpha);
	set_blur_texture_coordinates(3.f, 0.f);
	top= 1.f + (1.f - y_offset)/128.f;
	left= (-1.f - x_offset)/128.f - 1.f;
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, left, top, 0.5f, 1.f);
	set_blur_texture_coordinates(637.f, 0.f);
	right= 1.f + (1.f - x_offset)/128.f;
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, right, top, 0.5f, 1.f);
	set_blur_texture_coordinates(637.f, 480.f);
	bottom= (-1.f - y_offset)/128.f - 1.f;
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, right, bottom, 0.5f, 1.f);
	set_blur_texture_coordinates(3.f, 480.f);
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, left, bottom, 0.5f, 1.f);
	IDirect3DDevice8_End(global_d3d_device);

	return;
}

void generate_gravy_rect(
	struct gravy_layer const *layer,
	struct gravy_rectangle2d *rect)
{
	to_screen(layer->x - layer->half_width, layer->y - layer->half_height,
		1000.f - layer->distance, &rect->x0, &rect->y0);
	to_screen(layer->x + layer->half_width, layer->y + layer->half_height,
		1000.f - layer->distance, &rect->x1, &rect->y1);
#line 898 "c:\\halo\\SOURCE\\interface\\progress_bar.c"
	match_assert(__FILE__, __LINE__, rect->x0 < rect->x1);
	match_assert(__FILE__, __LINE__, rect->y0 < rect->y1);

	return;
}

static void progress_bar_alias_surface_texture(
	D3DBaseTexture *texture,
	D3DSurface const *surface)
{
	texture->Common= 0x00040005;
	texture->Data= surface->Data;
	texture->Lock= 0;
	texture->Size= 0x271df27f;
	texture->Format= 0x00011229;

	return;
}

static void progress_bar_draw(
	real progress)
{
	real ranges[NUMBER_OF_PROGRESS_BAR_SOUNDS][2]= {{0.f, 1.f}, {0.4f, 1.f}, {0.5f, 1.f}, {0.55f, 1.f}};
	real volumes[NUMBER_OF_PROGRESS_BAR_SOUNDS]= {3500.f, 4500.f, 3500.f, 4500.f};
	D3DBaseTexture back_buffer_texture;
	struct gravy_layer layer;
	real_rgb_color color;
	D3DSurface *back_buffer;
	real t;
	long sound_index;
	long stage;

	wobble_phase+= 0.01f;
	progress= (real)((sin(wobble_phase)*0.05 + progress - last_t)*0.2 + last_t);
	if (progress>1.0)
		progress= 1.f;
	last_t= progress;
	color.red= color.green= color.blue= (real)pow(sin(progress*3.14159), 0.9f);
	for (sound_index= 0; sound_index<NUMBER_OF_PROGRESS_BAR_SOUNDS; sound_index++)
	{
		if (progress<ranges[sound_index][0])
			t= 0.f;
		else if (progress>ranges[sound_index][1])
			t= 0.f;
		else
			t= (progress - ranges[sound_index][0])/(ranges[sound_index][1] - ranges[sound_index][0]);
		if (progress_bar_globals.sounds[sound_index])
		{
			IDirectSoundBuffer_SetVolume(progress_bar_globals.sounds[sound_index],
				(long)((real)pow(sin(t*3.1415), 0.3f)*volumes[sound_index] - 5000.f));
		}
	}

	current_time= GetTickCount();
	IDirect3DDevice8_GetTransform(global_d3d_device, D3DTS_WORLD, &progress_bar_globals.saved_world);
	IDirect3DDevice8_GetTransform(global_d3d_device, D3DTS_VIEW, &progress_bar_globals.saved_view);
	IDirect3DDevice8_GetTransform(global_d3d_device, D3DTS_PROJECTION, &progress_bar_globals.saved_projection);
	IDirect3DDevice8_SetTransform(global_d3d_device, D3DTS_WORLD, &progress_bar_globals.world);
	IDirect3DDevice8_SetTransform(global_d3d_device, D3DTS_VIEW, &progress_bar_globals.view);
	IDirect3DDevice8_SetTransform(global_d3d_device, D3DTS_PROJECTION, &progress_bar_globals.projection);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_DESTBLEND, D3DBLEND_ONE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_CULLMODE, D3DCULL_NONE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_ZENABLE, FALSE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_LIGHTING, FALSE);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 0, D3DTSS_MIPFILTER, D3DTEXF_POINT);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 1, D3DTSS_MIPFILTER, D3DTEXF_POINT);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 2, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 2, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 2, D3DTSS_MIPFILTER, D3DTEXF_POINT);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 2, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 2, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
	IDirect3DDevice8_SetTexture(global_d3d_device, 0, (D3DBaseTexture *)progress_bar_mode.texture0);
	IDirect3DDevice8_Clear(global_d3d_device, 0, NULL, 0xf0, 0, 1.f, 0);
	IDirect3DDevice8_SetVertexShader(global_d3d_device, 0);
	IDirect3DDevice8_SetPixelShaderProgram(global_d3d_device, (D3DPIXELSHADERDEF *)&blur_shader);
	IDirect3DDevice8_GetBackBuffer(global_d3d_device, -1, 0, &back_buffer);
	progress_bar_alias_surface_texture(&back_buffer_texture, back_buffer);
	for (stage= 0; stage<4; stage++)
	{
		IDirect3DDevice8_SetTextureStageState(global_d3d_device, stage, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
		IDirect3DDevice8_SetTextureStageState(global_d3d_device, stage, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
		IDirect3DDevice8_SetTextureStageState(global_d3d_device, stage, D3DTSS_MIPFILTER, D3DTEXF_POINT);
		IDirect3DDevice8_SetTextureStageState(global_d3d_device, stage, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
		IDirect3DDevice8_SetTextureStageState(global_d3d_device, stage, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
		IDirect3DDevice8_SetTexture(global_d3d_device, stage, &back_buffer_texture);
	}

	if (progress<0.9f)
		draw_fullscreen_blur(0.f, 0.f, 0.9f);
	else
		draw_fullscreen_blur(0.f, 0.f, 0.9f - (progress - 0.9f)*9.f);
	IDirect3DDevice8_SetTexture(global_d3d_device, 0, (D3DBaseTexture *)progress_bar_mode.texture0);
	IDirect3DDevice8_SetTexture(global_d3d_device, 1, (D3DBaseTexture *)progress_bar_mode.texture1);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 1, D3DTSS_BORDERCOLOR, 0x05050505);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 1, D3DTSS_ADDRESSU, D3DTADDRESS_BORDER);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
	IDirect3DDevice8_SetPixelShaderProgram(global_d3d_device, (D3DPIXELSHADERDEF *)&regular_shader);
	layer.texture_width= 640.f;
	layer.texture_height= 480.f;
	layer.x= 0.f;
	layer.y= 0.f;
	layer.half_width= (real)fabs(640.0/2.0);
	layer.half_height= (real)fabs(480.0/2.0);
	layer.distance= 0.f;
	draw_gravy_layer(&layer, &color, 1.f, progress);
	for (stage= 0; stage<4; stage++)
		IDirect3DDevice8_SetTexture(global_d3d_device, stage, NULL);
	IDirect3DDevice8_SetTransform(global_d3d_device, D3DTS_WORLD, &progress_bar_globals.saved_world);
	IDirect3DDevice8_SetTransform(global_d3d_device, D3DTS_VIEW, &progress_bar_globals.saved_view);
	IDirect3DDevice8_SetTransform(global_d3d_device, D3DTS_PROJECTION, &progress_bar_globals.saved_projection);
	IDirect3DDevice8_BlockUntilVerticalBlank(global_d3d_device);

	return;
}

static void progress_bar_build_glow_texture(
	void)
{
	D3DLOCKED_RECT locked_rect;
	unsigned long *pixel;
	long x, y;

	IDirect3DDevice8_CreateTexture(global_d3d_device, 128, 16, 1, 0, D3DFMT_LIN_A8B8G8R8, 0,
		&progress_bar_mode.texture1);
	IDirect3DTexture8_LockRect(progress_bar_mode.texture1, 0, &locked_rect, NULL, 0);
	pixel= locked_rect.pBits;
	for (y= 0; y<16; y++)
	{
		double glow= pow(sin(3.1415f*(y*(1.f/15.f))), 0.75);

		for (x= 0; x<128; x++)
		{
			byte intensity= (byte)(((real)(sin(3.1415f*(x*(1.f/127.f)))*
				glow)*(250.f/255.f) + 5.f/255.f)*255.f);

			*pixel= (intensity<<24) | (intensity<<16) | (intensity<<8) | intensity;
			pixel++;
		}
	}
	IDirect3DTexture8_UnlockRect(progress_bar_mode.texture1, 0);

	return;
}

static void progress_bar_setup(
	void)
{
	DSBUFFERDESC buffer_description;
	XBOXADPCMWAVEFORMAT wave_format;
	IDirect3DSurface8 *front_buffer;
	IDirect3DSurface8 *back_buffer;
	IDirect3DSurface8 *depth_buffer;
	long sound_index;

	D3DXMatrixIdentity(&progress_bar_globals.world);
	D3DXMatrixIdentity(&progress_bar_globals.view);
	D3DXMatrixOrthoLH(&progress_bar_globals.projection, 2.f, 2.f, -1.f, 2.f);
	D3DXMatrixPerspectiveLH(&progress_bar_globals.screen_transform, 0.64f, 0.48f, 1.f, 1000.f);
	progress_bar_load_loading_texture(&progress_bar_mode.texture0);
	progress_bar_build_glow_texture();
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_DESTBLEND, D3DBLEND_ONE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_CULLMODE, D3DCULL_NONE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_ZENABLE, FALSE);
	IDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_LIGHTING, FALSE);
	IDirect3DDevice8_SetTexture(global_d3d_device, 0, (D3DBaseTexture *)progress_bar_mode.texture0);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 0, D3DTSS_MIPFILTER, D3DTEXF_POINT);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
	IDirect3DDevice8_SetTextureStageState(global_d3d_device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);

	csmemset(&blur_shader, 0, sizeof(struct pixel_shader_definition));
	blur_shader.texture_modes= 0x8421;
	blur_shader.combiner_count= 3;
	blur_shader.rgb_inputs[0]= 0x08a009a0;
	blur_shader.rgb_outputs[0]= 0x00000c00;
	blur_shader.rgb_inputs[1]= 0x0aa00ba0;
	blur_shader.rgb_outputs[1]= 0x00000d00;
	blur_shader.rgb_inputs[2]= 0x0ca00da0;
	blur_shader.rgb_outputs[2]= 0x00000c00;
	blur_shader.alpha_inputs[0]= 0x14200000;
	blur_shader.alpha_outputs[0]= 0x000000c0;
	blur_shader.final_combiner_inputs_abcd= 0x0000000c;
	blur_shader.final_combiner_inputs_efg= 0x00001c00;

	csmemset(&regular_shader, 0, sizeof(struct pixel_shader_definition));
	regular_shader.texture_modes= 0x21;
	regular_shader.combiner_count= 1;
	regular_shader.rgb_inputs[0]= 0x08040000;
	regular_shader.rgb_outputs[0]= 0x000000c0;
	regular_shader.alpha_inputs[0]= 0x14190000;
	regular_shader.alpha_outputs[0]= 0x000000c0;
	regular_shader.final_combiner_inputs_abcd= 0x200c0000;
	regular_shader.final_combiner_inputs_efg= 0x00001c00;

	wave_format.wfx.wFormatTag= WAVE_FORMAT_XBOX_ADPCM;
	wave_format.wfx.wBitsPerSample= 4;
	wave_format.wfx.nChannels= 1;
	wave_format.wfx.nBlockAlign= 36;
	wave_format.wfx.nSamplesPerSec= 22050;
	wave_format.wfx.nAvgBytesPerSec= 12384;
	wave_format.wfx.cbSize= 2;
	wave_format.wSamplesPerBlock= 64;
	csmemset(&buffer_description, 0, sizeof(buffer_description));
	buffer_description.dwSize= sizeof(buffer_description);
	buffer_description.dwFlags= DSBCAPS_LOCDEFER|DSBCAPS_CTRLPOSITIONNOTIFY;
	buffer_description.dwBufferBytes= 0;
	buffer_description.lpwfxFormat= &wave_format.wfx;

	for (sound_index= 0; sound_index<NUMBER_OF_PROGRESS_BAR_SOUNDS; sound_index++)
	{
		if (DirectSoundCreateBuffer(&buffer_description, &progress_bar_globals.sounds[sound_index])<0)
			return;
		IDirectSoundBuffer_SetBufferData(progress_bar_globals.sounds[sound_index], compressed_monk_tone,
			sizeof(compressed_monk_tone));
		IDirectSoundBuffer_SetLoopRegion(progress_bar_globals.sounds[sound_index], 0,
			sizeof(compressed_monk_tone));
		IDirectSoundBuffer_SetCurrentPosition(progress_bar_globals.sounds[sound_index], 0);
		IDirectSoundBuffer_SetVolume(progress_bar_globals.sounds[sound_index], -5000);
		IDirectSoundBuffer_Play(progress_bar_globals.sounds[sound_index], 0, 0, DSBPLAY_LOOPING);
	}

	IDirectSoundBuffer_SetPitch(progress_bar_globals.sounds[0], -4681);
	IDirectSoundBuffer_SetPitch(progress_bar_globals.sounds[1], -6386);
	IDirectSoundBuffer_SetPitch(progress_bar_globals.sounds[2], -3999);
	IDirectSoundBuffer_SetPitch(progress_bar_globals.sounds[3], -8773);

	if (progress_bar_mode.capture_frame)
	{
		progress_bar_mode.capture_frame= FALSE;
		IDirect3DDevice8_GetDepthStencilSurface(global_d3d_device, &depth_buffer);
		IDirect3DDevice8_GetBackBuffer(global_d3d_device, 0, 0, &front_buffer);
		IDirect3DDevice8_GetBackBuffer(global_d3d_device, -1, 0, &back_buffer);
		IDirect3DDevice8_SetRenderTarget(global_d3d_device, back_buffer, depth_buffer);
		IDirect3DDevice8_Clear(global_d3d_device, 0, NULL, 0xf0, 0, 0.f, 0);
		IDirect3DDevice8_SetRenderTarget(global_d3d_device, front_buffer, depth_buffer);
	}

	return;
}

static void progress_bar_load_loading_texture(
	IDirect3DTexture8 **texture)
{
	D3DSURFACE_DESC surface_description;
	char map_directory[40];
	char source_path[40];
	struct tga_image image;
	D3DLOCKED_RECT locked_rect;
	unsigned long *pixels;
	FILE *file;

	IDirect3DDevice8_CreateTexture(global_d3d_device, 320, 240, 1, 0, D3DFMT_LIN_A8B8G8R8, 0, texture);
	IDirect3DTexture8_LockRect(*texture, 0, &locked_rect, NULL, 0);
	IDirect3DTexture8_GetLevelDesc(*texture, 0, &surface_description);
	pixels= locked_rect.pBits;
	csstrcpy(map_directory, cache_files_map_directory());
	sprintf(source_path, "z%sloading.tga", map_directory + 1);
	file= fopen(source_path, "rb");
	if (file)
	{
		tgaLoadHeader(file, &image);
		image.pixels= pixels;
		tgaLoadImageData(file, &image);
		fclose(file);
	}
	IDirect3DTexture8_UnlockRect(*texture, 0);

	return;
}

boolean progress_bar_screen_initialized(
	void)
{
	boolean result;

	result= progress_bar_mode.texture0 != NULL;

	return result;
}

void gen_cloud_coord(
	real x,
	real y,
	real offset)
{
	D3DDevice_SetVertexData2f(10, x*32.f + offset, y*20.f);

	return;
}

void gen_mask_coord(
	real x,
	real y,
	real mask_position)
{
	D3DDevice_SetVertexData2f(10, x - (mask_position*768.f - 64.f - 64.f), y*0.033333335f);

	return;
}
