/*
VM_DRAW.C

A guest program for tools/test_macos_draw.py: a draw through the macOS
device (port/macos/guest/d3d8_metal_device.c) as the game makes one. The
device is made at 64x64; the back buffer is cleared to blue and the left
half drawn red with immediate mode, by a vertex program of the NV2A's
encoding (mov oPos, v0; mov oD0, v3) and a pixel shader that passes the
diffuse colour through; then presented, for the app's frame capture, and
"ok" printed.

	vm_draw strip    a triangle strip
	vm_draw quads    a quad list
	vm_draw clear    the clear alone (no draw)
*/

#include "xgpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { printf("failed: %s (line %d)\n", #condition, __LINE__); exit(1); } } while (0)

/* combiner inputs (port/linux/src/nv2a_psh.c's registers) */
#define INPUT_ZERO 0x00
#define INPUT_V0 0x04
#define INPUT_R0 0x0c
#define INPUT_ALPHA 0x10
#define INPUT_INVERT 0x20

/* mov <output>, v<input>: the MAC's mov of an input register to an output
register, all of its components, unswizzled */
#define MOV_WORD1(input) ((1UL << 21) | ((unsigned long)(input) << 9) | 0x1B)
#define MOV_WORD2 (2UL << 26)
#define MOV_WORD3(output) ((0xFUL << 12) | (1UL << 11) | ((unsigned long)(output) << 3))

static const DWORD program[] = {
	(2UL << 16) | 0x2078,
	0, MOV_WORD1(0), MOV_WORD2, MOV_WORD3(0),
	0, MOV_WORD1(3), MOV_WORD2, MOV_WORD3(3) | 1
};

static const DWORD declaration[] = {
	(1UL << 29) | 0,
	(2UL << 29) | ((DWORD)D3DVSDT_FLOAT4 << 16) | 0,
	(2UL << 29) | ((DWORD)D3DVSDT_D3DCOLOR << 16) | 3,
	D3DVSD_END()
};

/* r0 = v0 * 1; out = r0 */
static void diffuse_pixel_shader(void)
{
	DWORD a = INPUT_V0, b = INPUT_ZERO | INPUT_INVERT;

	D3D__RenderState[D3DRS_PSRGBINPUTS0] = (a << 24) | (b << 16);
	D3D__RenderState[D3DRS_PSALPHAINPUTS0] = ((a | INPUT_ALPHA) << 24) | ((b | INPUT_ALPHA) << 16);
	D3D__RenderState[D3DRS_PSRGBOUTPUTS0] = INPUT_R0 << 4;
	D3D__RenderState[D3DRS_PSALPHAOUTPUTS0] = INPUT_R0 << 4;
	D3D__RenderState[D3DRS_PSCOMBINERCOUNT] = (D3D__RenderState[D3DRS_PSCOMBINERCOUNT] & ~0xffUL) | 1;
	D3D__RenderState[D3DRS_PSFINALCOMBINERINPUTSABCD] = INPUT_R0;
	D3D__RenderState[D3DRS_PSFINALCOMBINERINPUTSEFG] = (INPUT_R0 | INPUT_ALPHA) << 8;
	D3D__RenderState[D3DRS_PSTEXTUREMODES] = 0;
}

static void vertex(float x, float y)
{
	D3DDevice_SetVertexDataColor(D3DVSDE_DIFFUSE, 0xffff0000UL);
	D3DDevice_SetVertexData4f(D3DVSDE_VERTEX, x, y, 0.0f, 1.0f);
}

int main(int argc, char **argv)
{
	D3DPRESENT_PARAMETERS parameters;
	D3DDevice *device = NULL;
	DWORD handle = 0;

	if (argc < 2)
	{
		printf("usage: vm_draw strip|quads|clear\n");
		return 1;
	}
	memset(&parameters, 0, sizeof(parameters));
	parameters.BackBufferWidth = 64;
	parameters.BackBufferHeight = 64;
	CHECK(Direct3D_CreateDevice(0, D3DDEVTYPE_HAL, NULL, 0, &parameters, &device) == S_OK);
	CHECK(D3DDevice_CreateVertexShader(declaration, program, &handle, 0) == S_OK);
	CHECK(handle && !(handle & 1));
	D3DDevice_SetVertexShader(handle);
	diffuse_pixel_shader();
	D3D__RenderState[D3DRS_ZENABLE] = FALSE;
	D3D__RenderState[D3DRS_CULLMODE] = D3DCULL_NONE;
	D3D__RenderState[D3DRS_ALPHABLENDENABLE] = FALSE;

	D3DDevice_Clear(0, NULL, D3DCLEAR_TARGET, 0xff0000ffUL, 1.0f, 0);
	if (!strcmp(argv[1], "strip"))
	{
		D3DDevice_Begin(D3DPT_TRIANGLESTRIP);
		vertex(0.0f, 0.0f);
		vertex(32.0f, 0.0f);
		vertex(0.0f, 64.0f);
		vertex(32.0f, 64.0f);
		D3DDevice_End();
	}
	else if (!strcmp(argv[1], "quads"))
	{
		D3DDevice_Begin(D3DPT_QUADLIST);
		vertex(0.0f, 0.0f);
		vertex(32.0f, 0.0f);
		vertex(32.0f, 64.0f);
		vertex(0.0f, 64.0f);
		D3DDevice_End();
	}
	else if (strcmp(argv[1], "clear"))
	{
		printf("no case %s\n", argv[1]);
		return 1;
	}
	D3DDevice_Present(NULL, NULL, NULL, NULL);
	printf("ok\n");
	return 0;
}
