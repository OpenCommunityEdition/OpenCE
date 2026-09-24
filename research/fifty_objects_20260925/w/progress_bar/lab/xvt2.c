#include "cseries/cseries.h"
#include "real_math.h"
#include <xtl.h>

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

extern void *global_d3d_device;

void generate_gravy_rect(
	struct gravy_layer const *layer,
	struct gravy_rectangle2d *rect);

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
	real x0, y0, x1, y1;

	generate_gravy_rect(layer, &rect);
	IDirect3DDevice8_Begin(global_d3d_device, D3DPT_TRIANGLEFAN);
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_DIFFUSE,
		color->red, color->green, color->blue, alpha);
	mask_position= progress*768.f - 64.f - 64.f;
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD1, 0.f - mask_position, 0.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD0, 0.f, half_height);
	x0= rect.x0;
	y0= rect.y0;
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, x0, y0, 0.5f, 1.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD1, 640.f - mask_position, 0.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD0, half_width, half_height);
	x1= rect.x1;
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, x1, y0, 0.5f, 1.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD1, 640.f - mask_position, 16.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD0, half_width, 0.f);
	y1= rect.y1;
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, x1, y1, 0.5f, 1.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD1, 0.f - mask_position, 16.f);
	IDirect3DDevice8_SetVertexData2f(global_d3d_device, D3DVSDE_TEXCOORD0, 0.f, 0.f);
	IDirect3DDevice8_SetVertexData4f(global_d3d_device, D3DVSDE_VERTEX, x0, y1, 0.5f, 1.f);
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
	layer.half_width= 320.f;
	layer.half_height= 240.f;
	layer.distance= distance;
	draw_gravy_layer(&layer, color, alpha, progress);

	return;
}

void draw_gravy_base_layer(
	real_rgb_color const *color,
	real progress)
{
	struct gravy_layer layer;

	layer.texture_width= 640.f;
	layer.texture_height= 480.f;
	layer.x= 0.f;
	layer.y= 0.f;
	layer.half_width= 320.f;
	layer.half_height= 240.f;
	layer.distance= 0.f;
	draw_gravy_layer(&layer, color, 1.f, progress);

	return;
}
