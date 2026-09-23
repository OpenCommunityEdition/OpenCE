#ifndef D3D8_DXGCREATE_INTERNAL_H
#define D3D8_DXGCREATE_INTERNAL_H
#include "device_layout.h"
namespace D3D
{
extern const DWORD g_InitialRenderStates[87];
extern const BYTE g_InitialTextureStates[32];
void WINAPI InitializeKelvin(
    void);
void WINAPI InitializeHardware(
    void);
void WINAPI InitializeD3dState(
    void);
HRESULT WINAPI PresentationParametersParameterCheck(
    D3DPRESENT_PARAMETERS *parameters);
/* Original static GPU context handles, distinct from class numbers/methods. */
enum DeviceContextHandle
{
    NV_DD_DMA_PUSHER_SYNC_NOTIFIER_CONTEXT_DMA_TO_MEMORY = 2,
    NV_DD_DMA_CONTEXT_DMA_IN_VIDEO_MEMORY = 3,
    NV_DD_DMA_CONTEXT_DMA_TO_VIDEO_MEMORY = 4,
    D3D_MEMCOPY_NOTIFIER_CONTEXT_DMA_TO_MEMORY = 7,
    D3D_SEMAPHORE_CONTEXT_DMA_IN_MEMORY = 8,
    D3D_COLOR_CONTEXT_DMA_IN_VIDEO_MEMORY = 9,
    D3D_ZETA_CONTEXT_DMA_IN_VIDEO_MEMORY = 10,
    D3D_COPY_CONTEXT_DMA_IN_VIDEO_MEMORY = 11,
    D3D_CONTEXT_IN_CACHED_MEMORY = 12,
    D3D_KELVIN_PRIMITIVE = 13,
    D3D_MEMORY_TO_MEMORY_COPY = 14,
    D3D_RECTANGLE_COPY = 16,
    D3D_RECTANGLE_COPY_SURFACES = 17
};
}
#endif
