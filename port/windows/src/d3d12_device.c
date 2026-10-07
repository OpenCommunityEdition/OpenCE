/*
D3D12_DEVICE.C

The Direct3D 12 renderer of the Xbox Direct3D 8 device (xgpu_device.h),
Windows only (display.renderer "d3d12").
*/

#include "xgpu_device.h"

static BOOL d3d12_initialize(unsigned long width, unsigned long height)
{
	(void)width;
	(void)height;
	platform_log("Direct3D 12: not built yet");
	return FALSE;
}

const struct xgpu_backend xgpu_backend_d3d12 =
{
	.name = "Direct3D 12",
	.initialize = d3d12_initialize,
};
