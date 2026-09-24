#ifndef D3D8_STATE_INTERNAL_H
#define D3D8_STATE_INTERNAL_H
#include "device_layout.h"
namespace D3D
{
/* Original device-state flag: pure devices do not retain shadow constants. */
enum DeviceStateFlags
{
    STATE_PUREDEVICE = 0x10
};
void WINAPI CheckLightParameters(
    const D3DLIGHT8 *light);
extern const DWORD g_DirtyFromRenderState[];
extern const DWORD g_TransformDirtyTable[];
extern const float g_LODBias2x[];
extern DWORD g_DirtyFromTextureState[];
typedef void (WINAPI *SetComplexRenderStateFunction)(
    DWORD value);
extern const SetComplexRenderStateFunction g_ComplexRenderStateFunctionTable[];
DWORD *__fastcall CommonSetControl0(
    CDevice *device,
    DWORD *push);
DWORD *__fastcall CommonSetOccludeZStencilEn(
    CDevice *device,
    DWORD *push);
void WINAPI UpdateProjectionViewportTransform(
    void);
void __fastcall CommonSetSurfaceFormat(
    CDevice *device);
void __fastcall CommonSetPassthroughProgram(
    CDevice *device);
DWORD *__fastcall CommonSetViewport(
    CDevice *device,
    DWORD *push);
void __fastcall CommonSetDebugRegisters(
    void);
DWORD *__fastcall CommonSetTextureBumpEnv(
    CDevice *device,
    DWORD *push);
void __fastcall CommonSetAntiAliasingControl(
    CDevice *device);
}
#endif
